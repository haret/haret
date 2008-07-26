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
#include "exceptions.h" // TRY_EXCEPTION_HANDLER
#include "lateload.h" // LATE_LOAD
#include "machines.h" // Mach


/****************************************************************
 * Memory size detection
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
static void
mem_autodetect()
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
        // WinCE 5 uses dwTotalPhys to represent memory, but reserves
        // a good chunk for itself - align to nearest 16MB
        memPhysSize = ALIGN(mst.dwTotalPhys + 8*1024*1024, 16*1024*1024);
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

// Free the virtual memory pointers cache used by memPhysMap
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

// Free the virtual memory pointers cache used by memPhysMap
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

// Pointer to a virtual address mapping of the MMU table
static uint32 *MMUTable;

// Map the MMU table into haret's address space.
static void
mapInMMU()
{
    int ret;
    void *area;
    void *mmu;
    TRY_EXCEPTION_HANDLER {
        mmu = (void*)(cpuGetMMU() >> 8);
    } CATCH_EXCEPTION_HANDLER {
        Output("Exception on mmu table lookup");
        goto fail;
    }

    area = VirtualAlloc(NULL, 4096*4, MEM_RESERVE, PAGE_NOACCESS);
    if (! area)
        goto fail;

    // Map mmu physical location into address space.
    ret = VirtualCopy(area, mmu, 4096*4
                      , PAGE_READWRITE | PAGE_PHYSICAL | PAGE_NOCACHE);
    if (!ret)
        goto fail;

    MMUTable = (uint32*)area;
    return;

fail:
    Output("Unable to map in mmu table!  Many functions will not work.");
    // Point mmu table to a dummy array to prevent further crashes.
    MMUTable = (uint32*)calloc(4096*4, 1);
}

// Get cp15/c1 register
DEF_GETCPR(get_p15r1, p15, 0, c1, c0, 0)

static int Arm6NoSubPages;
static int16 PhysMapCached[4096];
static int16 PhysMapUncached[4096];

// Search a given mmu table for an l1 section mapping that provides a
// virtual to physical mapping for a specified physical base location.
static void
findReverseMMUmaps()
{
    if (Mach->arm6mmu) {
        // Check for arm6 subpages feature.
        uint32 r = get_p15r1();
        if (r & (1<<23))
            Arm6NoSubPages = 1;
    }

    // Clear maps.
    memset(PhysMapCached, -1, sizeof(PhysMapCached));
    memset(PhysMapUncached, -1, sizeof(PhysMapUncached));

    // Populate maps.
    int uncache_count = 0, cache_count = 0, ignore_count = 0;
    for (uint32 i=0; i<4096; i++) {
        uint32 l1d = MMUTable[i];
        if ((l1d & MMU_L1_TYPE_MASK) != MMU_L1_SECTION)
            // Only interested in section mappings.
            continue;
        if (Mach->arm6mmu && (l1d & MMU_L1_SUPER_SECTION_FLAG))
            // No support for "supersection" mappings.
            continue;
        uint16 base = l1d >> 20;
        uint32 cacheval = l1d & (MMU_L1_CACHEABLE|MMU_L1_BUFFERABLE);
        if (!cacheval && PhysMapUncached[base] == -1) {
            // Found a new uncached mapping.
            uncache_count++;
            PhysMapUncached[base] = i;
            //Output("Uncache map from %08x to %08x", i<<20, base<<20);
        } else if (cacheval == (MMU_L1_CACHEABLE|MMU_L1_BUFFERABLE)
                   && PhysMapCached[base] == -1) {
            // Found a new cached mapping.
            cache_count++;
            PhysMapCached[base] = i;
            //Output("Cache map from %08x to %08x", i<<20, base<<20);
        } else {
            ignore_count++;
            //Output("Ignoring map from %08x to %08x", i<<20, base<<20);
        }
    }

    Output("Found %d uncached and %d cached L1 mappings (ignored %d)."
           , uncache_count, cache_count, ignore_count);
}

// Try to obtain a virtual to physical map by reusing one of the wm
// 1-meg section mappings.
static uint8 *
memPhysMap_section(uint32 paddr, int cached=0)
{
    uint32 base = paddr >> 20;

    int16 *m = PhysMapUncached;
    if (cached)
        m = PhysMapCached;

    if (m[base] < 0)
        return NULL;

    return (uint8*)((((uint32)m[base]) << 20) | (paddr & ((1<<20) - 1)));
}

static uint32 PhysicalMapMethod = 1;
REG_VAR_INT(0, "PHYSMAPMETHOD", PhysicalMapMethod
            , "Physical map method (1=1meg cache, 0=VirtualCopy only)")

// Map physical memory to a virtual address. The function ensures
// that at least 32K memory ahead of given address is available
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

// This function is called at startup - initialize memory handling routines.
void
setupMemory()
{
    Output("Detecting ram size");
    mem_autodetect();

    Output("Mapping mmu table");
    mapInMMU();

    Output("Build L1 reverse map");
    findReverseMMUmaps();
}


/****************************************************************
 * MMU table flag reporting
 ****************************************************************/

static char *__flags_cb(char *p, uint32 &d)
{
    *p++ = ' ';
    *p++ = (d & MMU_L1_CACHEABLE) ? 'C' : ' ';
    *p++ = (d & MMU_L1_BUFFERABLE) ? 'B' : ' ';
    d &= ~(MMU_L1_CACHEABLE|MMU_L1_BUFFERABLE);
    return p;
}

static char *__flags_cond(char *p, uint32 &d
                          , uint32 bits, uint32 shift, char *name)
{
    uint32 mask = ((1<<bits) - 1) << shift;
    if (!(d & mask))
        return p;
    if (bits > 1)
        p += sprintf(p, " %s=%x", name, (d&mask) >> shift);
    else
        p += sprintf(p, " %s", name);
    d &= ~mask;
    return p;
}

static void __flags_other(char *p, uint32 d)
{
    d &= ~MMU_L1_TYPE_MASK;
    if (d)
        p += sprintf(p, " ?=%x", d);
    *p = 0;
}

static char *__flags_ap(char *p, uint32 &d, uint32 count, int shift
                        , int apxbit = 0)
{
    uint32 add = 0;
    if (Arm6NoSubPages && (d & (1<<apxbit))) {
        d &= ~(1<<apxbit);
        add = 4;
    }
    *p++ = ' '; *p++ = 'A'; *p++ = 'P'; *p++ = '=';
    for (uint32 i = 0; i < count; i++) {
        *p++ = '0' + ((d>>shift) & 3) + add;
        d &= ~(3<<shift);
        shift += 2;
    }
    return p;
}

static void flags_l1(char *p, uint32 d)
{
    p = __flags_cond(p, d, 4, 5, "D");

    if (Mach->arm6mmu)
        p = __flags_cond(p, d, 1, 9, "P");

    __flags_other(p, d);
}

static void section_flags(char *p, uint32 d)
{
    p = __flags_cb(p, d);
    p = __flags_ap(p, d, 1, MMU_L1_AP_SHIFT, 15);

    if (! Mach->arm6mmu || !(d & MMU_L1_SUPER_SECTION_FLAG))
        p = __flags_cond(p, d, 4, 5, "D");

    if (Mach->arm6mmu) {
        d &= ~MMU_L1_SUPER_SECTION_FLAG;

        p = __flags_cond(p, d, 1, 9, "P");
        p = __flags_cond(p, d, 3, 12, "T");

        if (Arm6NoSubPages) {
            p = __flags_cond(p, d, 1, 17, "nG");
            p = __flags_cond(p, d, 1, 16, "S");
            p = __flags_cond(p, d, 1, 4, "XN");
        }
    }

    __flags_other(p, d);
}

static void small_flags(char *p, uint32 d)
{
    p = __flags_cb(p, d);
    p = __flags_ap(p, d, 4, MMU_L2_AP0_SHIFT);
    __flags_other(p, d);
}

static void tiny_flags(char *p, uint32 d)
{
    p = __flags_cb(p, d);
    p = __flags_ap(p, d, 1, MMU_L2_AP0_SHIFT);
    __flags_other(p, d);
}

static void large_flags(char *p, uint32 d)
{
    p = __flags_cb(p, d);
    if (Mach->arm6mmu) {
        p = __flags_cond(p, d, 3, 12, "T");
        if (Arm6NoSubPages) {
            p = __flags_ap(p, d, 1, MMU_L2_AP0_SHIFT, 9);
            p = __flags_cond(p, d, 1, 11, "nG");
            p = __flags_cond(p, d, 1, 10, "S");
            p = __flags_cond(p, d, 1, 15, "XN");
        } else {
            p = __flags_ap(p, d, 4, MMU_L2_AP0_SHIFT);
        }
    } else {
        p = __flags_ap(p, d, 4, MMU_L2_AP0_SHIFT);
    }
    __flags_other(p, d);
}

static void extended_flags(char *p, uint32 d)
{
    p = __flags_cb(p, d);
    p = __flags_ap(p, d, 1, MMU_L2_AP0_SHIFT, 9);
    p = __flags_cond(p, d, 3, 6, "T");
    if (Arm6NoSubPages) {
        p = __flags_cond(p, d, 1, 11, "nG");
        p = __flags_cond(p, d, 1, 10, "S");
        p = __flags_cond(p, d, 1, 0, "XN");
    }
    __flags_other(p, d);
}

static void noflags(char *p, uint32 d)
{
    *p = '\0';
}


/****************************************************************
 * Virtual to Physical mage mapping
 ****************************************************************/

static const struct pageinfo L1PageInfo[] = {
    { "UNMAPPED", noflags},
    { "Coarse", flags_l1, 1, MMU_L1_COARSE_MASK, 12},
    { "1MB section", section_flags, 1, MMU_L1_SECTION_MASK},
    { "Fine", flags_l1, 1, MMU_L1_FINE_MASK, 10},
};

static const struct pageinfo L2PageInfo[] = {
    { "UNMAPPED", noflags},
    { "Large (64K)", large_flags, 1, MMU_L2_LARGE_MASK},
    { "Small (4K)", small_flags, 1, MMU_L2_SMALL_MASK},
    { "Tiny (1K)", tiny_flags, 1, MMU_L2_TINY_MASK},
};

static const struct pageinfo Arm6SuperSection = {
    "16MB section", section_flags, 1, MMU_L1_SUPER_SECTION_MASK
};
static const struct pageinfo Arm6Reserved = {
    "Reserved", noflags
};
static const struct pageinfo Arm6Extended = {
    "Extended (4K)", extended_flags, 1, MMU_L2_SMALL_MASK
};

const struct pageinfo *
getL1Desc(uint32 l1d)
{
    uint32 type = l1d & 3;
    if (Mach->arm6mmu) {
        if (type == 3)
            return &Arm6Reserved;
        if (type == 2 && (l1d & MMU_L1_SUPER_SECTION_FLAG))
            return &Arm6SuperSection;
    }
    return &L1PageInfo[type];
}

const struct pageinfo *
getL2Desc(uint32 l2d)
{
    uint32 type = l2d & 3;
    if (Mach->arm6mmu) {
        if (Arm6NoSubPages && (l2d & 2))
            return &Arm6Extended;
        if (type == 3)
            return &Arm6Extended;
    }
    return &L2PageInfo[type];
}

// Translate a virtual address to physical
uint32
memVirtToPhys(uint32 vaddr)
{
    vaddr = MVAddr(vaddr);
    uint32 desc = MMUTable[vaddr >> 20];
    const struct pageinfo *pi = getL1Desc(desc);
    if (pi->L2MapShift) {
        desc = memPhysRead((desc & pi->mask)
                           + ((vaddr & 0xfffff) >> pi->L2MapShift) * 4);
        pi = getL2Desc(desc);
    }
    if (! pi->isMapped)
        return (uint32)-1;
    return (desc & pi->mask) | (vaddr & ~pi->mask);
}


/****************************************************************
 * Page allocation
 ****************************************************************/

// Free pages allocated with allocPages()
void
freePages(void *data)
{
    int ret = UnmapViewOfFile(data);
    if (!ret)
        Output(C_ERROR "UnmapViewOfFile failed %p (code %ld)"
               , data, GetLastError());
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
        freePages(data);
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
 * Continuous page allocation
 ****************************************************************/

// Sort compare function (compare by physical address).
static int physPageComp(const void *e1, const void *e2) {
    pageAddrs *i1 = (pageAddrs*)e1, *i2 = (pageAddrs*)e2;
    return (i1->physLoc < i2->physLoc ? -1
            : (i1->physLoc > i2->physLoc ? 1 : 0));
}

struct continuousPageInfo {
    void *rawdata;
    struct pageAddrs pages[0];
};
#define size_continuousPageInfo(nrpages) \
    ((uint32)(&((continuousPageInfo*)0)->pages[nrpages]))

static void
fcp_emulate(struct continuousPageInfo *info)
{
    if (! info)
        return;
    if (info->rawdata)
        freePages(info->rawdata);
    free(info);
}

static void *
acp_emulate(uint32 pageCount, struct continuousPageInfo **info)
{
    struct continuousPageInfo *ci = NULL;
    uint32 trycount = pageCount;
    if (pageCount < 2)
        // Huh?
        goto fail;

    for (;;) {
        ci = (struct continuousPageInfo *)malloc(
            size_continuousPageInfo(trycount));
        if (! ci)
            goto fail;
        ci->rawdata = allocPages(ci->pages, trycount);
        if (! ci->rawdata)
            goto fail;

        // Sort the pages by physical location.
        qsort(ci->pages, trycount, sizeof(ci->pages[0]), physPageComp);

        // See if a continuous range with sufficient size exists.
        uint32 cont=1;
        for (uint i=1; i<trycount; i++) {
            if (ci->pages[i].physLoc != ci->pages[i-1].physLoc + PAGE_SIZE) {
                cont = 1;
                continue;
            }
            cont++;
            if (cont >= pageCount) {
                // Success.
                Output("Found %d continuous pages by allocating %d virtual pages"
                       , pageCount, trycount);
                *info = ci;
                return ci->pages[i-(pageCount-1)].virtLoc;
            }
        }

        // Failed to find the continuous pages - retry with larger size.
        fcp_emulate(ci);
        trycount *= 2;
    }

fail:
    // Allocation failed
    Output("Unable to find %d continuous pages", pageCount);
    fcp_emulate(ci);
    *info = NULL;
    return NULL;
}

LATE_LOAD(AllocPhysMem, "coredll")
LATE_LOAD(FreePhysMem, "coredll")

// Allocate continuous pages using wince AllocPhysMem call.
static void *
acp_builtin(int pageCount, struct continuousPageInfo **info)
{
    ulong dummy;
    void *pages = late_AllocPhysMem(pageCount * PAGE_SIZE
                                    , PAGE_EXECUTE_READWRITE, 0, 0, &dummy);
    *info = (struct continuousPageInfo *)pages;
    return pages;
}

// Free continuous pages that were allocated using AllocPhysMem.
static void
fcp_builtin(struct continuousPageInfo *info)
{
    if (info)
        late_FreePhysMem(info);
}

void
freeContPages(struct continuousPageInfo *info)
{
    if (late_AllocPhysMem && late_FreePhysMem)
        return fcp_builtin(info);
    else
        return fcp_emulate(info);
}

void *
allocContPages(int pageCount, struct continuousPageInfo **info)
{
    void *data;
    if (late_AllocPhysMem && late_FreePhysMem)
        data = acp_builtin(pageCount, info);
    else
        data = acp_emulate(pageCount, info);
    if (! data)
        return NULL;
    void *vmdata = cachedMVA(data);
    if (! vmdata) {
        Output(C_INFO "Can't find vm addr of alloc'd physical ram %p"
               , vmdata);
        freeContPages(*info);
        return NULL;
    }
    return vmdata;
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
void *
cachedMVA(void *addr)
{
    uint32 paddr = memVirtToPhys((uint32)addr);
    return memPhysMap_section(paddr);
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
