/*
    Memory & MMU access routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _MEMORY_H
#define _MEMORY_H

#include "xtypes.h" // uint32

/* --------------------------------------------- MMU tables structures ----- */

// Level 1 Descriptor
typedef uint32 mmuL1Desc;

// Entry type (lower 2 bits)
#define MMU_L1_TYPE_MASK	3
#define MMU_L1_UNMAPPED		0
#define MMU_L1_COARSE_L2	1
#define MMU_L1_SECTION		2
#define MMU_L1_FINE_L2		3
// Domain field is present in all fields except Type
#define MMU_L1_DOMAIN_MASK	0x1E0
#define MMU_L1_DOMAIN_SHIFT	5
// Coarse page table address
#define MMU_L1_COARSE_MASK	0xfffffc00
// Section descriptor
#define MMU_L1_SECTION_MASK	0xfff00000
#define MMU_L1_SUPER_SECTION_MASK 0xff000000
#define MMU_L1_SUPER_SECTION_FLAG (1<<18)
#define MMU_L1_AP_MASK		0x00000c00
#define MMU_L1_AP_SHIFT		10
#define MMU_L1_CACHEABLE	0x00000008
#define MMU_L1_BUFFERABLE	0x00000004
// Fine page table address
#define MMU_L1_FINE_MASK	0xfffff000

// Level 2 Descriptor
typedef uint32 mmuL2Desc;

// Entry type (lower 2 bits)
#define MMU_L2_TYPE_MASK	3
#define MMU_L2_UNMAPPED		0
#define MMU_L2_LARGEPAGE	1
#define MMU_L2_SMALLPAGE	2
#define MMU_L2_TINYPAGE		3
// These bits are present in all fields except Type
#define MMU_L2_CACHEABLE	0x00000008
#define MMU_L2_BUFFERABLE	0x00000004
#define MMU_L2_AP0_SHIFT	4
#define MMU_L2_AP0_MASK		0x00000030
#define MMU_L2_AP1_MASK		0x000000c0
#define MMU_L2_AP2_MASK		0x00000300
#define MMU_L2_AP3_MASK		0x00000c00
// Large page address
#define MMU_L2_LARGE_MASK	0xffff0000
// Small page address
#define MMU_L2_SMALL_MASK	0xfffff000
// Tiny page address
#define MMU_L2_TINY_MASK	0xfffffc00

void setupMemory();

// Types of memory accesses.
enum MemOps {
    MO_SIZE8 = 0,
    MO_SIZE16 = 1,
    MO_SIZE32 = 2,
};

extern uint8 *memPhysMap(uint32 paddr);
extern void memPhysReset();
extern uint32 memPhysRead(uint32 paddr);
extern bool memPhysWrite(uint32 paddr, uint32 value);
extern uint32 memVirtToPhys(uint32 vaddr);
uint32 retryVirtToPhys(uint32 vaddr);
void *cachedMVA(void *addr);

struct pageinfo {
    const char name[16];
    void (*flagfunc)(char *p, uint32 d);
    uint8 isMapped;
    uint32 mask;
    uint16 L2MapShift;
};

const struct pageinfo *getL1Desc(uint32 l1d);
const struct pageinfo *getL2Desc(uint32 l2d);

struct pageAddrs {
    uint32 physLoc;
    char *virtLoc;
};

void freePages(void *data);
void *allocPages(struct pageAddrs *pages, int pageCount);

struct continuousPageInfo;
void freeContPages(struct continuousPageInfo *info);
void *allocContPages(int pageCount, struct continuousPageInfo **info);

// Test if 'addr' is in the range from 'start'..'start+size'
#define IN_RANGE(addr, start, size) ({   \
            uint32 __addr = (addr);      \
            uint32 __start = (start);    \
            uint32 __size = (size);      \
            (__addr - __start < __size); \
        })

// Test if two ranges overlap - the caller must ensure the 'size'
// parameters are greater than zero and not near 2**32.
#define RANGES_OVERLAP(start1, size1, start2, size2) ({                 \
            uint32 __start1 = (start1), __size1 = (size1);              \
            uint32 __start2 = (start2), __size2 = (size2);              \
            IN_RANGE(__start1 + __size1 - 1, __start2, __size2 + __size1 - 1); \
        })

// The size of physical memory to map at once
#define PHYS_CACHE_SIZE 0x10000
#define PHYS_CACHE_MASK (PHYS_CACHE_SIZE - 1)

extern uint32 memPhysAddr;
extern uint32 memPhysSize;

#endif /* _MEMORY_H */
