/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _CPU_H
#define _CPU_H

// Read one register of coprocessor
extern uint32 cpuGetCP (uint cp, uint regno);
// Set a coprocessor register
extern bool cpuSetCP (uint cp, uint regno, uint32 val);
// Dump the 16 registers of coprocessor #cp
extern bool cpuDumpCP (void (*out) (void *data, const char *, ...),
                       void *data, uint32 *args);
// Get physical address of MMU 1st level descriptor tables
extern uint32 cpuGetMMU ();
// Get current Process Identifier register (0-127)
extern uint32 cpuGetPID ();
// Flush all CPU data caches (must be in supervisor mode)
extern "C" void cpuFlushCache ();
// Get Domain Access Control Register to given value
extern "C" uint32 cpuGetDACR ();
// Set Domain Access Control Register to given value (in supervisor mode)
extern "C" void cpuSetDACR (uint32 value);
// Get Program Status Register value
extern "C" uint32 cpuGetPSR ();
// Disable interrupts
extern "C" void cli ();
// Enable interrupts
extern "C" void sti ();
// Coprocessor register access for scripting
extern uint32 cpuScrCP (bool setval, uint32 *args, uint32 val);

#endif /* _CPU_H */
