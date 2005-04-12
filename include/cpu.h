/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _CPU_H
#define _CPU_H

#include "pxa2xx.h"
#include "s3c24xx.h"

// Read one register of coprocessor
extern uint32 cpuGetCP (uint cp, uint regno);
// Set a coprocessor register
extern bool cpuSetCP (uint cp, uint regno, uint32 val);
// Dump the 16 registers of coprocessor #cp
extern bool cpuDumpCP (void (*out) (void *data, const char *, ...),
                       void *data, uint32 *args);
// Dump the state of on-chip AC97 controller
extern bool cpuDumpAC97 (void (*out) (void *data, const char *, ...),
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

// control type of CPU
extern char *cpuTypeString;

extern void cpuType(void);

struct cpu_fns {
	char *name;

	// claim any resources that will be needed by the cpu
	int (*setup_load)(void);

	// shutdown peripheral blocks ready for load
	int (*shutdown_peripherals)(void);

	// try and recover the situation if things fail
	int (*attempt_recovery)(void);
};

extern struct cpu_fns *cpu;

#endif /* _CPU_H */
