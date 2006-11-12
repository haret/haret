/*
    Memory & MMU access routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _MEMORY_H
#define _MEMORY_H

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

// Autodetect physical memory size.
void mem_autodetect(void);

// Map physical memory to a virtual address. The function ensures
// that at least 32K memory ahead of given address is available
extern uint8 *memPhysMap (uint32 paddr);
// Free the virtual memory pointers cache used by memPhysMap
extern void memPhysReset ();
// Read a word from given physical address; return (uint32)-1 on error
extern uint32 memPhysRead (uint32 paddr);
// Write a word and return success status
extern bool memPhysWrite (uint32 paddr, uint32 value);
// Translate a virtual address to physical
extern uint32 memVirtToPhys (uint32 vaddr);

// Dump the MMU to the file mmap.txt
extern bool memDumpMMU (void (*out) (void *data, const char *, ...),
                        void *data, uint32 *args);
// Dump a portion of memory to file
extern void memDump (const char *fn, uint8 *vaddr, uint32 size, uint32 base = (uint32)-1);
// Dump a portion of physical memory to file
extern void memPhysDump (const char *fn, uint32 paddr, uint32 size);

// Fill given number of words in virtual memory with given value
extern void memFill (uint32 *vaddr, uint32 wcount, uint32 value);
// Fill given number of words in physical memory with given value
extern void memPhysFill (uint32 paddr, uint32 wcount, uint32 value);

// Write a portion of virtual memory to file
extern bool memVirtWriteFile (const char *fn, uint32 addr, uint32 size);
// Write a portion of physical memory to file
extern bool memPhysWriteFile (const char *fn, uint32 addr, uint32 size);

// Memory read/write routines for scripting
extern uint32 memScrVMB (bool setval, uint32 *args, uint32 val);
extern uint32 memScrVMH (bool setval, uint32 *args, uint32 val);
extern uint32 memScrVMW (bool setval, uint32 *args, uint32 val);
extern uint32 memScrPMB (bool setval, uint32 *args, uint32 val);
extern uint32 memScrPMH (bool setval, uint32 *args, uint32 val);
extern uint32 memScrPMW (bool setval, uint32 *args, uint32 val);

// Physical memory location (default 0xa0000000 as per Intel specs)
extern uint32 memPhysAddr;
// Physical memory size (detected at startup)
extern uint32 memPhysSize;

#endif /* _MEMORY_H */
