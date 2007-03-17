/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _CPU_H
#define _CPU_H

#include "xtypes.h" // uint32

// Size of pages used in wince
#define PAGE_SIZE 4096
// Set a variable to be aligned on a page boundary.
#define PAGE_ALIGNED __attribute__ ((aligned (4096)))
// Return an integer rounded up to the nearest page
#define PAGE_ALIGN(v) (((v)+PAGE_SIZE-1) / PAGE_SIZE * PAGE_SIZE)

// Macros useful for defining CPU coprocessor accessor functions
#define DEF_GETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2, Attr)     \
static inline uint32 Attr Name (void) {                         \
    uint32 val;                                                 \
    asm volatile("mrc " #Cpr ", " #Op1 ", %0, "                 \
                 #CRn ", " #CRm ", " #Op2 : "=r" (val));        \
    return val;                                                 \
}
#define DEF_SETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2, Attr)     \
static inline void Attr Name (uint32 val) {                     \
    asm volatile("mcr " #Cpr ", " #Op1 ", %0, "                 \
                 #CRn ", " #CRm ", " #Op2 : : "r"(val));        \
}

// Create a CPU coprocessor accessor functions
#define DEF_GETCPR(Name, Cpr, Op1, CRn, CRm, Op2)     \
    DEF_GETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2,)
#define DEF_SETCPR(Name, Cpr, Op1, CRn, CRm, Op2)     \
    DEF_SETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2,)

// Get physical address of MMU 1st level descriptor tables
extern uint32 cpuGetMMU ();
// Get Program Status Register value
static inline uint32 cpuGetPSR(void) {
    uint32 val;
    asm volatile("mrs %0, cpsr" : "=r" (val));
    return val;
}

// Get pid register
DEF_GETCPR(getPIDReg, p15, 0, c13, c0, 0)

// Return the Modified Virtual Address (MVA) of a given virtual address
static inline uint32 MVAddr(uint32 addr) {
    if (addr <= 0x01ffffff)
        // Need to turn virtual address in to modified virtual address.
        addr |= getPIDReg() & 0xfe000000;
    return addr;
}

void printWelcome();
// Take control of the cpu -- don't let wince interrupt.
void take_control();
// Return to normal processing.
void return_control();
// Force wince to map in all haret application pages.
void touchAppPages(void);

#endif /* _CPU_H */
