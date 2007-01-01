/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "windows.h"

#include "xtypes.h"
#include "output.h" // Output
#include "machines.h" // Mach
#include "script.h" // REG_VAR_ROFUNC
#include "cpu.h"

REG_VAR_ROFUNC(0, "PSR", cpuGetPSR, 0, "Program Status Register")

static uint32
cpuGetFamily(bool setval, uint32 *args, uint32 val)
{
  return (uint32)Mach->name;
}
REG_VAR_ROFUNC(0, "CPU", cpuGetFamily, 0, "Autodetected CPU family")

DEF_GETCPR(get_p15r2, p15, 0, c2, c0, 0)

// Returns the address of 1st level descriptor table
uint32 cpuGetMMU ()
{
    return get_p15r2() & 0xffffc000;
}
REG_VAR_ROFUNC(
    0, "MMU", cpuGetMMU, 0,
    "Memory Management Unit level 1 descriptor table physical addr")

DEF_GETCPR(get_p15r13, p15, 0, c13, c0, 0)

// Returns the PID register contents
static uint32 cpuGetPID()
{
    return get_p15r13() >> 25;
}
REG_VAR_ROFUNC(0, "PID", cpuGetPID, 0,
               "Current Process Identifier register value")

// Symbols added by linker.
extern "C" {
    extern uint32 _text_start;
    extern uint32 _text_end;
    extern uint32 _data_start;
    extern uint32 _data_end;
    extern uint32 _rdata_start;
    extern uint32 _rdata_end;
    extern uint32 _bss_start;
    extern uint32 _bss_end;
}

// Access all the pages in a pointer range.  (This forces wince to
// make sure the page is mapped.)
static void
touchPages(uint32 *start, uint32 *end)
{
    if (PAGE_ALIGN((uint32)start) != (uint32)start)
        Output("Internal error. touchPages range not page aligned");

    while (start < end) {
        volatile uint32 dummy;
        dummy = *start;
        start += (PAGE_SIZE/sizeof(*start));
    }
}

// Touch all the code pages of the HaRET application.
//
// wm5 has been seen lazily mapping in code pages.  That is, it may
// not actually load certain functions (or parts of a function) into
// memory until they are actually used.  This presents problems for
// certain haret functions that try to take full control of the CPU,
// because part of the code could might not yet be mapped.  When this
// code is accessed it causes a fault that hands control back to wm5.
// A solution is to touch all code pages to ensure the code is really
// in memory.
void
touchAppPages(void)
{
    touchPages(&_text_start, &_text_end);
    touchPages(&_data_start, &_data_end);
    touchPages(&_rdata_start, &_rdata_end);
    touchPages(&_bss_start, &_bss_end);
}

static int controlCount;

// Take over CPU control from wince.  After calling this, the
// application should not be interrupted by any interrupts or faults.
// In general, the code should not make any OS calls until after
// return_control is invoked.
void
take_control()
{
    if (controlCount++)
        // Already in a critical section.
        return;

    // Flush log to disk (in case we don't survive)
    flushLogFile();

    // Map in pages to prevent page faults in critical section.
    touchAppPages();

    // Disable interrupts.
    unsigned long temp;
    __asm__ __volatile__(
        "mrs    %0, cpsr\n"
        "       orr    %0, %0, #0xc0\n"
        "       msr    cpsr_c, %0"
        : "=r" (temp) : : "memory");
}

void
return_control()
{
    if (--controlCount)
        // Still in the critical section.
        return;

    // Reenable interrupts.
    unsigned long temp;
    __asm__ __volatile__(
        "mrs    %0, cpsr\n"
        "       bic    %0, %0, #0xc0\n"
        "       msr    cpsr_c, %0"
        : "=r" (temp) : : "memory");
}
