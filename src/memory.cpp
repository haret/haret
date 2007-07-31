/*
    Memory and MMU access routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include "pkfuncs.h" // VirtualCopy

#include "xtypes.h"
#include "cpu.h" // cpuGetMMU
#include "memory.h"
#include "output.h" // Output
#include "script.h" // REG_VAR_INT


/****************************************************************
 * Memory setup
 ****************************************************************/

// RAM start physical address
uint32 memPhysAddr = 0xFFFFFFFF;
// RAM size (autodetected)
uint32 memPhysSize;

REG_VAR_INT(0, "RAMADDR", memPhysAddr
            , "Physical RAM start address")
REG_VAR_INT(0, "RAMSIZE", memPhysSize
            , "Physical RAM size (default = autodetected)")

/* Autodetect RAM physical size at startup */
void
mem_autodetect(void)
{
    // Get OS info
    OSVERSIONINFOW vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionEx(&vi);

    // Get memory info
    MEMORYSTATUS mst;
    STORE_INFORMATION sti;
    mst.dwLength = sizeof(mst);
    GlobalMemoryStatus(&mst);
    GetStoreInformation(&sti);

#define ALIGN(x, d) (((x) + (d) - 1) / (d) * (d))

    if (vi.dwMajorVersion >= 5)
        // WinCE 5 uses dwTotalPhys to represent memory - align to
        // nearest 16MB
        memPhysSize = ALIGN(mst.dwTotalPhys, 16*1024*1024);
    else
        // WinCE is returning ~1Mb less memory, let's suppose minus
        // kernel size, so we'll round the result up to nearest 8MB
        // boundary.
        memPhysSize = ALIGN(mst.dwTotalPhys + sti.dwStoreSize, 8*1024*1024);
    Output("WinCE reports memory size %d (phys=%ld store=%ld)"
           , memPhysSize, mst.dwTotalPhys, sti.dwStoreSize);
}


/****************************************************************
 * Mapping physical memory
 ****************************************************************/

#if 1

/**
 * This version of memPhysMap sometimes locks up because of VirtualFree()
 * I don't know what's the real case, but I had to write a different
 * implementation using low-level operations instead of this one.
 * The second implementation, however, locks up on program exit
 * (haven't figured out why) so you have to choose between the two
 * implementations depending on your needs :-)
 */

// The amount of physical memory locations to cache
#define PHYS_CACHE_COUNT 8
// Cache several last mapped physical memory for better effectivity
static uint8 *phys_mem [PHYS_CACHE_COUNT];
// The physical address (multiple of 32K, if 1 then slot is free)
static uint32 phys_base [PHYS_CACHE_COUNT] = { 1, 1, 1, 1, 1, 1, 1, 1 };

/* We allocate windows in virtual address space for physical memory
 * in 64K chunks, however we always ensure there are at least 32K ahead
 * the address user requested.
 */
uint8 *memPhysMap_wm(uint32 paddr)
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

// The amount of physical memory locations to cache
#define PHYS_CACHE_COUNT 8
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

uint8 *memPhysMap_bruteforce(uint32 paddr)
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
      Output(C_ERROR "Failed (%d) to map a dummy memory area to virtual address window",
                GetLastError ());
err:  VirtualFree (pmWindow, 0, MEM_RELEASE);
      return NULL;
    }

    pmAlignedWindow = (PHYS_CACHE_MASK + (uint32)pmWindow) & ~PHYS_CACHE_MASK;
    // The offset inside the MMU L1 descriptor table
    uint32 pmBase = pmAlignedWindow & 0xfff00000;
    // Apply current process ID register, if applicable
    pmBase = MVAddr(pmBase);

    //--- Read the L1 descriptor ---//

    uint32 *l1desc = (uint32 *)VirtualAlloc (NULL, 4 * 4096, MEM_RESERVE, PAGE_READONLY);
    if (!VirtualCopy (l1desc, (void *)(cpuGetMMU () >> 8), 4 * 4096,
                      PAGE_READONLY | PAGE_PHYSICAL | PAGE_NOCACHE))
    {
      Output(C_ERROR "VirtualCopy() failed (%d) when accessing MMU L1 table",
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
      Output(C_ERROR "Ooops... 2nd level table is not coarse. This is not implemented so far");
      goto err;
    }

    // Now find the address of our entries in the 2nd level coarse page table
    l2pt &= MMU_L1_COARSE_MASK;

    // VirtualCopy will fail if l2pt is not on a 4K boundary
    uint32 delta = l2pt & 0xfff;
    l2pt &= 0xfffff000;

    pmL2PT = (uint32 *)VirtualAlloc (NULL, 8192, MEM_RESERVE, PAGE_READWRITE);
    if (!VirtualCopy (pmL2PT, (void *)(l2pt >> 8), 8192,
                      PAGE_READWRITE | PAGE_PHYSICAL))
    {
      Output(C_ERROR "VirtualCopy() failed (%d) when accessing MMU L2 table",
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

// Search a given mmu table for an l1 section mapping that provides a
// virtual to physical mapping for a specified physical base location.
static int
searchMMUforPhys(uint32 *mmu, uint32 base, int cached=0)
{
    for (uint32 i=0; i<4096; i++) {
        uint32 l1d = mmu[i];
        if ((l1d & MMU_L1_TYPE_MASK) != MMU_L1_SECTION)
            // Only interested in section mappings.
            continue;
        if ((l1d >> 20) != base)
            // No address match.
            continue;
        uint32 cacheval = l1d & (MMU_L1_CACHEABLE|MMU_L1_BUFFERABLE);
        if (!cached && cacheval)
            // Only interested in mappings that don't use the cache.
            continue;
        if (cached && cacheval != (MMU_L1_CACHEABLE|MMU_L1_BUFFERABLE))
            // Only interested in mappings that are cached.
            continue;
        // Found a hit.
        return i;
    }
    return -1;
}

// Try to obtain a virtual to physical map by reusing one of the wm
// 1-meg section mappings.  This also maintains an index of found
// mappings to speed up future requests.
static uint8 *
memPhysMap_section(uint32 paddr)
{
    static int16 cache[4096];

    uint32 base = paddr >> 20;

    if (cache[base]) {
        if (cache[base] < 0)
            return NULL;
        // Cache hit.  Just return what is in the cache.
        return (uint8*)((((uint32)cache[base]) << 20)
                        | (paddr & ((1<<20) - 1)));
    }

    // Get virtual address of MMU table.
    static uint32 *mmuCache;
    uint32 *mmu = mmuCache;
    if (!mmu) {
        uint32 mmu_paddr = cpuGetMMU();
        mmu = (uint32*)memPhysMap_wm(mmu_paddr);
        if (!mmu)
            return NULL;
        static int triedCache;
        if (! triedCache) {
            // Try to find a persistent mmu mapping.
            triedCache = 1;
            int pos = searchMMUforPhys(mmu, mmu_paddr >> 20);
            if (pos >= 0)
                mmu = mmuCache = (uint32*)((pos << 20)
                                           | (mmu_paddr & ((1<<20) - 1)));
        }
    }

    int pos = searchMMUforPhys(mmu, base);
    cache[base] = pos;
    if (pos < 0)
        // Couldn't find a suitable section mapping.
        return NULL;
    return (uint8*)((pos << 20) | (paddr & ((1<<20) - 1)));
}

static uint32 PhysicalMapMethod = 1;
REG_VAR_INT(0, "PHYSMAPMETHOD", PhysicalMapMethod
            , "Physical map method (1=1meg cache, 0=VirtualCopy only)")

uint8 *
memPhysMap(uint32 paddr)
{
    if (PhysicalMapMethod & 1) {
        uint8 *ret = memPhysMap_section(paddr);
        if (ret)
            return ret;
    }

#if 0
    if (PhysicalMapMethod & 2)
        return memPhysMap_bruteforce(paddr);
#endif

    return memPhysMap_wm(paddr);
}


/****************************************************************
 * Page allocation
 ****************************************************************/

// Free pages allocated with allocPages()
void
freePages(void *data, int pageCount)
{
    int ret = UnmapViewOfFile(data);
    if (!ret)
        Output(C_ERROR "UnmapViewOfFile failed %p / %d (code %ld)"
               , data, pageCount, GetLastError());
}

// Allocate and pin 'pageCount' number of pages and fill 'pages'
// structure with the physical and virtual locations of those pages.
void *
allocPages(struct pageAddrs *pages, int pageCount)
{
    int pageBytes = pageCount * PAGE_SIZE;
    HANDLE h = CreateFileMapping(
        (HANDLE)INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        0, pageBytes, NULL);
    if (!h) {
        Output(C_ERROR "Failed to allocate %d pages (code %ld)"
               , pageCount, GetLastError());
        return NULL;
    }

    void *data = MapViewOfFile(h, FILE_MAP_WRITE, 0, 0, 0);
    int ret = CloseHandle(h);
    if (!data) {
        Output(C_ERROR "Failed to map %d pages (code %ld)"
               , pageCount, GetLastError());
        return NULL;
    }
    if (!ret)
        Output(C_WARN "CloseHandle failed (code %ld)", GetLastError());

    DWORD pfns[pageCount];
    ret = LockPages(data, pageBytes, pfns, LOCKFLAG_WRITE);
    if (!ret) {
        Output(C_ERROR "Failed to lock %d pages (code %ld)"
               , pageCount, GetLastError());
        freePages(data, pageCount);
        return NULL;
    }

    // Find all the physical locations of the pages.
    for (int i = 0; i < pageCount; i++) {
        struct pageAddrs *pd = &pages[i];
        pd->virtLoc = &((char *)data)[PAGE_SIZE * i];
        pd->physLoc = pfns[i]; // XXX should: x << UserKInfo[KINX_PFN_SHIFT]
    }

    return data;
}


/****************************************************************
 * Misc utilities
 ****************************************************************/

// Read a word from given physical address; return (uint32)-1 on error
uint32 memPhysRead (uint32 paddr)
{
  uint8 *pm;

  if (!(pm = memPhysMap (paddr)))
    return (uint32)-1;

  return *(uint32 *)pm;
}

// Write a word and return success status
bool memPhysWrite (uint32 paddr, uint32 value)
{
  uint8 *pm;

  if (!(pm = memPhysMap (paddr)))
    return false;

  *(uint32 *)pm = value;
  return true;
}

// Return a long lived (externally visible) virtual mapping from
// another (possibly process local) virtual mapping.
uint32
cachedMVA(void *addr)
{
    uint32 *mmu = (uint32*)memPhysMap(cpuGetMMU());
    uint32 paddr = memVirtToPhys((uint32)addr);
    uint32 base = paddr >> 20;
    int pos = searchMMUforPhys(mmu, base, 1);
    //Output(L"cachedMVA: paddr=%08x pos=%08x pos2=%08x", paddr, pos, pos2);
    if (pos == -1)
        return 0;
    return (uint32)((pos << 20) | (paddr & ((1<<20) - 1)));
}

// Translate a virtual address to physical
uint32 memVirtToPhys (uint32 vaddr)
{
  uint32 mmu = cpuGetMMU ();

  // First of all, if vaddr < 32Mb, PID replaces top 7 bits
  vaddr = MVAddr(vaddr);

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

// Invalidate D TLB line
DEF_SETCPRATTR(set_invDTLB, p15, 0, c8, c6, 1,, "memory")

// Try finding the physical address of a page owned by haret.
uint32
retryVirtToPhys(uint32 vaddr)
{
    uint count = 0;
    for (;;) {
        // Touch the page.
        uint32 dummy;
        dummy = *(volatile uint8*)vaddr;
        // Try to find the physical address.
        uint32 paddr = memVirtToPhys(vaddr);
        if (paddr != (uint32)-1)
            // Common case - address found.
            return paddr;
        if (count++ >= 100)
            // Too many tries - just give up.
            break;
        // Ughh. Some Wince versions mess with the page tables in
        // weird ways - invalidate the D TLB for the address and try
        // again.
        set_invDTLB(MVAddr(vaddr));
    }
    return (uint32)-1;
}
