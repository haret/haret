/*
    Memory and MMU access routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stddef.h>
#include <string.h>
#include <windows.h>

#include "xtypes.h"
#include "cpu.h"
#include "memory.h"
#include "output.h"
#include "util.h"
#include "haret.h"

// RAM start physical address
uint32 memPhysAddr = 0xa0000000;
// RAM size (autodetected)
uint32 memPhysSize;

/* Autodetect RAM physical size at startup */
static struct __mem_dummy
{
  __mem_dummy ()
  {
    MEMORYSTATUS mst;
    STORE_INFORMATION sti;
    mst.dwLength = sizeof (mst);
    GlobalMemoryStatus (&mst);
    GetStoreInformation(&sti);
    /* WinCE is returning ~1Mb less memory, let's suppose minus kernel size,
       so we'll round the result up to nearest 8Mb boundary. */
    memPhysSize = (mst.dwTotalPhys + sti.dwStoreSize + 0x7fffff) & ~0x7fffff;
  }
} __mem_dummy_obj;

#if 1

/**
 * This version of memPhysMap sometimes locks up because of VirtualFree()
 * I don't know what's the real case, but I had to write a different
 * implementation using low-level operations instead of this one.
 * The second implementation, however, locks up on program exit
 * (haven't figured out why) so you have to choose between the two
 * implementations depending on your needs :-)
 */

// The size of physical memory to map at once
#define PHYS_CACHE_SIZE 0x10000
#define PHYS_CACHE_MASK (PHYS_CACHE_SIZE - 1)
// The amount of physical memory locations to cache
#define PHYS_CACHE_COUNT 4
// Cache several last mapped physical memory for better effectivity
static uint8 *phys_mem [PHYS_CACHE_COUNT];
// The physical address (multiple of 32K, if 1 then slot is free)
static uint32 phys_base [PHYS_CACHE_COUNT] = { 1, 1, 1, 1 };

/* We allocate windows in virtual address space for physical memory
 * in 64K chunks, however we always ensure there are at least 32K ahead
 * the address user requested.
 */
uint8 *memPhysMap (uint32 paddr)
{
  // Address should be aligned
  paddr &= ~3;

  uint32 base = paddr & ~(PHYS_CACHE_MASK >> 1);
  uint32 offs = paddr & (PHYS_CACHE_MASK >> 1);

  int slot = -1;
  for (int i = 0; i < PHYS_CACHE_COUNT; i++)
    if ((phys_base [i] == 1)
     || (phys_base [i] == base))
    {
      slot = i;
      break;
    }

  // If there is no empty slot, make it
  if (slot == -1)
  {
    slot = PHYS_CACHE_COUNT - 1;
    // This can lock up -- dunno why :-(
    VirtualFree (phys_mem [slot], 0, MEM_RELEASE);
    phys_base [slot] = 1;
  }

  // Move the cache element hit to the front (LRU)
  if (slot)
  {
    uint32 tmp1 = phys_base [slot];
    memmove (phys_base + 1, phys_base, slot * sizeof (uint32));
    phys_base [0] = tmp1;

    uint8 *tmp2 = phys_mem [slot];
    memmove (phys_mem + 1, phys_mem, slot * sizeof (uint8 *));
    phys_mem [0] = tmp2;
  }

  // If slot has not been allocated yet, allocate it
  if (phys_base [0] == 1)
  {
    phys_base [0] = base;
    phys_mem [0] = (uint8 *)VirtualAlloc (NULL, PHYS_CACHE_SIZE,
                                          MEM_RESERVE, PAGE_NOACCESS);
    // Map requested physical memory to our virtual address hole
    if (!VirtualCopy ((void *)phys_mem [0], (void *)(base / 256),
                      PHYS_CACHE_SIZE, PAGE_READWRITE | PAGE_PHYSICAL | PAGE_NOCACHE))
    {
      VirtualFree (phys_mem [0], 0, MEM_RELEASE);
      phys_mem [0] = NULL;
    }
  }

  // In the case of failure ...
  if (!phys_mem [0])
  {
    phys_base [0] = 1;
    return NULL;
  }

  return phys_mem [0] + offs;
}

void memPhysReset ()
{
  for (int i = 0; i < PHYS_CACHE_COUNT; i++)
    if (phys_mem [i])
    {
      VirtualFree (phys_mem [i], 0, MEM_RELEASE);
      phys_mem [i] = NULL;
      phys_base [i] = 1;
    }
}

#else

extern "C" BOOL SetKMode (BOOL fMode);

// The amount of physical memory locations to cache
#define PHYS_CACHE_COUNT 4
// The size of physical memory to map at once
#define PHYS_CACHE_SIZE 0x10000
#define PHYS_CACHE_MASK (PHYS_CACHE_SIZE - 1)

static bool pmInited = false;
// The window in virtual address space we use to map physical memory into
static void *pmWindow;
// The first address inside the pmWindow block which is 64K aligned
static uint32 pmAlignedWindow;
// Old contents of 2nd level page table
static uint32 pmOldPT [PHYS_CACHE_COUNT * 16];
// The 2nd level page table (4*16 entries)
static uint32 *pmL2PT, *pmPT;
// The LRU table for allocated cache entries
static uint32 pmLRU [PHYS_CACHE_COUNT];
// The domain used by Windows for VirtualCopy'ed pages
static uint32 pmDomain;
// The virtual address where the entire physical RAM is mapped
static uint32 pmMemoryMapAddr = 0;

uint8 *memPhysMap (uint32 paddr)
{
  if (!pmInited)
  {
    // At initialization we grab a virtual memory window of (n+1)*64K size.
    // This address space is used to map into it (by direct writes into
    // the L1 descriptor table - duh) the requested physical memory.
    // We cannot guarantee the allocated window starts at a multiple of
    // 64K address, however, since we request more than we need we can
    // just ignore the unaligned space at the start of block.
    pmWindow = VirtualAlloc (NULL, (PHYS_CACHE_COUNT + 1) * 64 * 1024,
                             MEM_RESERVE, PAGE_READWRITE);
    // Map anything to it (doesnt matter what)
    if (!VirtualCopy (pmWindow, (void *)0, (PHYS_CACHE_COUNT + 1) * 64 * 1024,
                      PAGE_READWRITE | PAGE_PHYSICAL | PAGE_NOCACHE))
    {
      Complain (C_ERROR ("Failed (%d) to map a dummy memory area to virtual address window"),
                GetLastError ());
err:  VirtualFree (pmWindow, 0, MEM_RELEASE);
      return NULL;
    }

    pmAlignedWindow = (PHYS_CACHE_MASK + (uint32)pmWindow) & ~PHYS_CACHE_MASK;
    // The offset inside the MMU L1 descriptor table
    uint32 pmBase = pmAlignedWindow & 0xfff00000;
    // Apply current process ID register, if applicable
    if ((pmBase & 0xfe000000) == 0)
      pmBase |= cpuGetPID () << 25;

    //--- Read the L1 descriptor ---//

    uint32 *l1desc = (uint32 *)VirtualAlloc (NULL, 4 * 4096, MEM_RESERVE, PAGE_READONLY);
    if (!VirtualCopy (l1desc, (void *)(cpuGetMMU () >> 8), 4 * 4096,
                      PAGE_READONLY | PAGE_PHYSICAL | PAGE_NOCACHE))
    {
      Complain (C_ERROR ("VirtualCopy() failed (%d) when accessing MMU L1 table"),
                GetLastError ());
      VirtualFree (l1desc, 0, MEM_RELEASE);
      goto err;
    }

#if 1
    /* Interestingly enough, WinCE have set up a memory area (at virtual
       address 0xa4000000 in my case) which maps the ENTIRE physical memory
       in read-write mode for system programs (inaccessible for user programs).
       But!!! it seems that WinCE (at least PPC 2003) runs all programs in
       SYSTEM mode (!!! security? what's that ???). So we can try to
       look the address of that area, and if we find it, we can keep
       ourselves from doing a lot of extra work. */
    if ((cpuGetPSR () & 0x1f) == 0x1f)
    {
      for (uint i = 0; i < 4096; i++)
        if ((l1desc [i] & (MMU_L1_SECTION_MASK | MMU_L1_TYPE_MASK)) ==
            (memPhysAddr | MMU_L1_SECTION))
        {
          bool ok = true;
          for (uint j = 1024 * 1024; j < memPhysSize; j += 1024 * 1024)
            if ((l1desc [i + (j >> 20)] & (MMU_L1_SECTION_MASK | MMU_L1_TYPE_MASK)) !=
                ((memPhysAddr + j) | MMU_L1_SECTION))
            {
              ok = false;
              break;
            }
          if (ok)
          {
            pmMemoryMapAddr = i << 20;
            break;
          }
        }
    }
#endif

    uint32 l2pt = l1desc [pmBase >> 20];
    VirtualFree (l1desc, 0, MEM_RELEASE);

    if ((l2pt & MMU_L1_TYPE_MASK) != MMU_L1_COARSE_L2)
    {
      Complain (C_ERROR ("Ooops... 2nd level table is not coarse. This is not implemented so far"));
      goto err;
    }

    // Now find the address of our entries in the 2nd level coarse page table
    l2pt &= MMU_L1_COARSE_MASK;

    // VirtualCopy will fail if l2pt is not on a 4K boundary
    uint32 delta = l2pt & 0xfff;
    l2pt &= 0xfffff000;

    pmL2PT = (uint32 *)VirtualAlloc (NULL, 4096, MEM_RESERVE, PAGE_READWRITE);
    if (!VirtualCopy (pmL2PT, (void *)(l2pt >> 8), 4096,
                      PAGE_READWRITE | PAGE_PHYSICAL))
    {
      Complain (C_ERROR ("VirtualCopy() failed (%d) when accessing MMU L2 table"),
                GetLastError ());
      VirtualFree (pmL2PT, 0, MEM_RELEASE);
      goto err;
    }

    pmPT = pmL2PT + (delta >> 2) + ((pmAlignedWindow & 0xff000) >> 12);

    // Remember the page table entries
    int i;
    for (i = 0; i < 16 * PHYS_CACHE_COUNT; i++)
      pmOldPT [i] = pmPT [i];
    // Compute the correct domain for our mappings
    pmDomain = pmPT [0] & (MMU_L2_AP0_MASK | MMU_L2_AP1_MASK |
                           MMU_L2_AP2_MASK | MMU_L2_AP3_MASK);

    for (i = 0; i < PHYS_CACHE_COUNT; i++)
      pmLRU [i] = i;

    pmInited = true;
  }

  if (pmMemoryMapAddr
   && (paddr >= memPhysAddr)
   && (paddr < memPhysAddr + memPhysSize))
    return (uint8 *)(paddr - memPhysAddr + pmMemoryMapAddr);

  uint32 base = paddr & ~(PHYS_CACHE_MASK >> 1);
  uint32 offs = paddr & (PHYS_CACHE_MASK >> 1);

  // Check if the requested address is already mapped
  int slot;
  for (slot = 0; slot < PHYS_CACHE_COUNT; slot++)
    if ((pmPT [pmLRU [slot] * 16] & (MMU_L2_TYPE_MASK | MMU_L2_SMALL_MASK)) ==
        (MMU_L2_SMALLPAGE | base))
      break;

  if (slot >= PHYS_CACHE_COUNT)
  {
    // Go into supervisor mode
    SetKMode (TRUE);
    cli ();
    cpuFlushCache ();

    // Fill the least recently used slot with the new values
    slot = PHYS_CACHE_COUNT - 1;
    unsigned x = pmLRU [slot] * 16;
    // Non-cacheable, non-bufferable
    for (int i = 0; i < 16; i++)
      pmPT [x + i] = MMU_L2_SMALLPAGE | pmDomain | (base + i * 4096);

    // Back to user mode
    sti ();
    SetKMode (FALSE);
  }

  // Move least recently used slot to front
  if (slot)
  {
    uint32 x = pmLRU [slot];
    for (; slot > 0; slot--)
      pmLRU [slot] = pmLRU [slot - 1];
    pmLRU [0] = x;
  }

  return (uint8 *)(pmAlignedWindow + pmLRU [0] * PHYS_CACHE_SIZE + offs);
}

void memPhysReset ()
{
  if (pmInited)
  {
    // Go into supervisor mode
    SetKMode (TRUE);
    cpuFlushCache ();
    // Restore the page table entries
    for (int i = 0; i < 16 * PHYS_CACHE_COUNT; i++)
      pmPT [i] = pmOldPT [i];
    SetKMode (FALSE);

    VirtualFree (pmL2PT, 0, MEM_RELEASE);
    VirtualFree (pmWindow, 0, MEM_RELEASE);
    pmInited = false;
  }
}

#endif

uint32 memPhysRead (uint32 paddr)
{
  uint8 *pm;

  if (!(pm = memPhysMap (paddr)))
    return (uint32)-1;

  return *(uint32 *)pm;
}

bool memPhysWrite (uint32 paddr, uint32 value)
{
  uint8 *pm;

  if (!(pm = memPhysMap (paddr)))
    return false;

  *(uint32 *)pm = value;
  return true;
}

uint32 memVirtToPhys (uint32 vaddr)
{
  uint32 mmu = cpuGetMMU ();

  // First of all, if vaddr < 32Mb, PID replaces top 7 bits
  if (vaddr <= 0x01ffffff)
    vaddr |= (cpuGetPID () << 25);

  // Bits 20..32 select the address of 1st level descriptor
  uint32 paddr = mmu + ((vaddr >> 18) & ~3);
  // We don't need anymore bits 20..32
  vaddr &= 0x000fffff;

  mmuL1Desc l1d = memPhysRead (paddr);
  switch (l1d & MMU_L1_TYPE_MASK)
  {
    case MMU_L1_UNMAPPED:
      return (uint32)-1;
    case MMU_L1_SECTION:
      paddr = (l1d & MMU_L1_SECTION_MASK) + vaddr;
      return paddr;
    case MMU_L1_COARSE_L2:
      // Bits 12..19 select the 2nd level descriptor
      paddr = (l1d & MMU_L1_COARSE_MASK) + ((vaddr >> 10) & ~3);
      vaddr &= 0xfff;
      break;
    case MMU_L1_FINE_L2:
      // Bits 10..19 select the 2nd level descriptor
      paddr = (l1d & MMU_L1_FINE_MASK) + ((vaddr >> 8) & ~3);
      vaddr &= 0x3ff;
      break;
  }

  mmuL2Desc l2d = memPhysRead (paddr);
  switch (l2d & MMU_L2_TYPE_MASK)
  {
    case MMU_L2_UNMAPPED:
      return (uint32)-1;
    case MMU_L2_LARGEPAGE:
      paddr = (l2d & MMU_L2_LARGE_MASK) + vaddr;
      break;
    case MMU_L2_SMALLPAGE:
      paddr = (l2d & MMU_L2_SMALL_MASK) + vaddr;
      break;
    case MMU_L2_TINYPAGE:
      paddr = (l2d & MMU_L2_TINY_MASK) + vaddr;
      break;
  }

  return paddr;
}

static uchar dump_char (uchar c)
{
  if ((c < 32) || (c >= 127))
    return '.';
  else
    return c;
}

void memDump (const char *fn, uint8 *vaddr, uint32 size, uint32 base)
{
  char fnbuff [200];
  FILE *f = NULL;

  if (fn)
  {
    fnprepare (fn, fnbuff, sizeof (fnbuff));
    f = fopen (fnbuff, "a+");
    if (!f)
    {
      Complain (C_ERROR ("Cannot open output file\n%hs"), fnbuff);
      return;
    }
  }

  uint32 offs = 0;
  char chrdump [17];
  chrdump [16] = 0;

  __try
  {
    while (offs < size)
    {
      if ((offs & 15) == 0)
      {
        uint32 addr = (base != (uint32)-1) ? base + offs : (uint32)vaddr + offs;
        if (f)
        {
          if (offs)
            fprintf (f, " | %s\n", chrdump);
          fprintf (f, "%08x |", addr);
        }
        else
        {
          if (offs)
            Output (L" | %hs", chrdump);
          Output (L"%08x |\t", addr);
        }
      }
  
      uint32 d = *(uint32 *)(vaddr + offs);
      if (f)
        fprintf (f, " %08x", d);
      else
        Output (L" %08x\t", d);
  
      chrdump [(offs & 15) + 0] = dump_char ((d      ) & 0xff);
      chrdump [(offs & 15) + 1] = dump_char ((d >>  8) & 0xff);
      chrdump [(offs & 15) + 2] = dump_char ((d >> 16) & 0xff);
      chrdump [(offs & 15) + 3] = dump_char ((d >> 24) & 0xff);
  
      offs += 4;
    }

    while (offs & 15)
    {
      if (f)
        fprintf (f, "         ");
      else
        Output (L"         \t");
      chrdump [offs & 15] = 0;
      offs += 4;
    }

    if (f)
    {
      fprintf (f, " | %s\n", chrdump);
      fclose (f);
    }
    else
      Output (L" | %hs", chrdump);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION while reading from address %08x"),
      vaddr + offs);
  }
}

void memPhysDump (const char *fn, uint32 paddr, uint32 size)
{
  while (size)
  {
    uint8 *vaddr = memPhysMap (paddr);
    uint32 bytes = PHYS_CACHE_SIZE - (PHYS_CACHE_MASK & (uint32)vaddr);
    if (bytes > size)
      bytes = size;
    memDump (fn, vaddr, bytes, paddr);
    size -= bytes;
    paddr += bytes;
  }
}

void memFill (uint32 *vaddr, uint32 wcount, uint32 value)
{
  __try
  {
    while (wcount--)
      *vaddr++ = value;
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION while writing %08x to address %08x"),
      value, vaddr);
  }
}

void memPhysFill (uint32 paddr, uint32 wcount, uint32 value)
{
  while (wcount)
  {
    uint32 *vaddr = (uint32 *)memPhysMap (paddr);
    // We are guaranteed to have 32K ahead
    uint32 words = 8 * 1024;
    if (words > wcount)
      words = wcount;
    memFill (vaddr, words, value);
    wcount -= words;
    paddr += words << 2;
  }
}

static bool memWrite (FILE *f, uint32 addr, uint32 size)
{
  while (size)
  {
    uint32 wc, sz = (size > 0x1000) ? 0x1000 : size;
    __try
    {
      wc = fwrite ((void *)addr, 1, sz, f);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      wc = 0;
      Complain (C_ERROR ("Exception caught!"));
    }

    if (wc != sz)
    {
      Complain (C_ERROR ("Short write detected while writing to file"));
      fclose (f);
      return false;
    }
    addr += sz;
    size -= sz;
  }
  return true;
}

bool memVirtWriteFile (const char *fn, uint32 addr, uint32 size)
{
  FILE *f = fopen (fn, "wb");
  if (!f)
  {
    Complain (C_ERROR ("Cannot write file %hs"), fn);
    return false;
  }

  if (!memWrite (f, addr, size))
    return false;

  fclose (f);
  return true;
}

bool memPhysWriteFile (const char *fn, uint32 addr, uint32 size)
{
  FILE *f = fopen (fn, "wb");
  if (!f)
  {
    Complain (C_ERROR ("Cannot write file %hs"), fn);
    return false;
  }

  while (size)
  {
    uint8 *vaddr = memPhysMap (addr);
    // We are guaranteed to have 32K ahead
    uint32 sz = size > 0x8000 ? 0x8000 : size;
    if (!memWrite (f, (uint32)vaddr, sz))
      return false;
    size -= sz;
    addr += sz;
  }

  fclose (f);
  return true;
}

static char *__flags_l1 (uint32 d)
{
  static char x [16];
  x [0] = (d & MMU_L1_CACHEABLE) ? 'C' : ' ';
  x [1] = (d & MMU_L1_BUFFERABLE) ? 'B' : ' ';
  x [2] = 0;
  return x;
}

static char *__flags_l2 (uint32 d)
{
  char *x = __flags_l1 (d);

  d >>= MMU_L2_AP0_SHIFT;

  int i, j = ((d & MMU_L2_TYPE_MASK) < MMU_L2_TINYPAGE) ? 4 : 1;

  for (i = 0; i < j; i++)
  {
    x [2 + i] = '0' + (d & 3);
    d >>= 2;
  }

  while (i < 5)
    x [2 + i++] = ' ';

  x [2 + i] = 0;

  return x;
}

bool memDumpMMU (void (*out) (void *data, const char *, ...),
                 void *data, uint32 *args)
{
  out (data, "----- Virtual address map -----\n\n");
  out (data, "Descriptor flags legend:\n"
             " C: Cacheable\n"
             " B: Bufferable\n"
             " 0..3: Access Permissions (for up to 4 slices):\n"
             "       0: Supervisor mode Read\n"
             "       1: Supervisor mode Read/Write\n"
             "       2: User mode Read\n"
             "       3: User mode Read/Write\n\n");

  uint32 mmu = cpuGetMMU ();
  out (data, " MMU 1st level descriptor table is at %p\n", mmu);

  out (data, "  Virtual | Physical |  Descr  | Description\n");
  out (data, "  address | address  |  flags  |\n");
  out (data, "----------+----------+---------+-----------------------------\n");

  // Previous 1st and 2nd level descriptors
  uint32 pL1 = 0xffffffff;
  uint32 pL2 = 0xffffffff;
  uint mb;

  // Walk down the 1st level descriptor table
  InitProgress (0x1000);
  __try
  {
    for (mb = 0; mb < 0x1000; mb++)
    {
      SetProgress (mb);

      mmuL1Desc l1d = memPhysRead (mmu + mb * 4);

      uint32 paddr, pss;
      uint l2_count = 0;

      // Ok, now we have a 1st level descriptor
      switch (l1d & MMU_L1_TYPE_MASK)
      {
        case MMU_L1_UNMAPPED:
          if ((l1d ^ pL1) & MMU_L1_TYPE_MASK)
            out (data, " %08x |          |         | UNMAPPED\n", mb << 20);
          break;
        case MMU_L1_SECTION:
          paddr = (l1d & MMU_L1_SECTION_MASK);
          out (data, " %08x | %08x | %s      | 1MB section\n", mb << 20, paddr,
                  __flags_l1 (l1d));
          break;
        case MMU_L1_COARSE_L2:
          // Bits 12..19 select the 2nd level descriptor
          paddr = (l1d & MMU_L1_COARSE_MASK);
          l2_count = 256; pss = 12;
          break;
        case MMU_L1_FINE_L2:
          // Bits 10..19 select the 2nd level descriptor
          paddr = (l1d & MMU_L1_FINE_MASK);
          l2_count = 1024; pss = 10;
          break;
      }

      if (l2_count)
      {
        // There is a possibility of crash here. I don't know what's the
        // cause but that's why L2 is commented out for now
        for (uint d = 0; d < l2_count; d++)
        {
          mmuL2Desc l2d = memPhysRead (paddr + d * 4);

          uint32 l2paddr;
          switch (l2d & MMU_L2_TYPE_MASK)
          {
            case MMU_L2_UNMAPPED:
              if ((l2d ^ pL2) & MMU_L2_TYPE_MASK)
                out (data, " %08x |          |         | UNMAPPED\n",
                     (mb << 20) + (d << pss));
              break;
            case MMU_L2_LARGEPAGE:
              l2paddr = (l2d & MMU_L2_LARGE_MASK);
              out (data, " %08x | %08x | %s | Large page (64K)\n",
                   (mb << 20) + (d << pss), l2paddr, __flags_l2 (l2d));
              break;
            case MMU_L2_SMALLPAGE:
              l2paddr = (l2d & MMU_L2_SMALL_MASK);
              out (data, " %08x | %08x | %s | Small page (4K)\n",
                   (mb << 20) + (d << pss), l2paddr, __flags_l2 (l2d));
              break;
            case MMU_L2_TINYPAGE:
              l2paddr = (l2d & MMU_L2_TINY_MASK);
              out (data, " %08x | %08x | %s | Tiny page (1K)\n",
                   (mb << 20) + (d << pss), l2paddr, __flags_l2 (l2d));
              break;
          }

          pL2 = l2d;
        }
      }

      pL1 = l1d;
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION CAUGHT AT MEGABYTE %d!"), mb);
  }

  out (data, " ffffffff |          |         | End of virtual address space\n");
  DoneProgress ();
  return true;
}

uint32 memScrVMB (bool setval, uint32 *args, uint32 val)
{
  uint8 *mem = (uint8 *)args [0];
  if (setval)
  {
    *mem = val;
    return 0;
  }
  return *mem;
}

uint32 memScrVMH (bool setval, uint32 *args, uint32 val)
{
  uint16 *mem = (uint16 *)args [0];
  if (setval)
  {
    *mem = val;
    return 0;
  }
  return *mem;
}

uint32 memScrVMW (bool setval, uint32 *args, uint32 val)
{
  uint32 *mem = (uint32 *)args [0];
  if (setval)
  {
    *mem = val;
    return 0;
  }
  return *mem;
}

uint32 memScrPMB (bool setval, uint32 *args, uint32 val)
{
  uint8 *mem = (uint8 *)memPhysMap (args [0]);
  if (setval)
  {
    *mem = val;
    return 0;
  }
  return *mem;
}

uint32 memScrPMH (bool setval, uint32 *args, uint32 val)
{
  uint16 *mem = (uint16 *)memPhysMap (args [0]);
  if (setval)
  {
    *mem = val;
    return 0;
  }
  return *mem;
}

uint32 memScrPMW (bool setval, uint32 *args, uint32 val)
{
  uint32 *mem = (uint32 *)memPhysMap (args [0]);
  if (setval)
  {
    *mem = val;
    return 0;
  }
  return *mem;
}
