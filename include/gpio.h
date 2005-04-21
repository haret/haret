/*
    Periferial I/O devices interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _IO_H
#define _IO_H

#include "pxa2xx.h"

// set GPIO as output (true) or input (false)
extern void gpioSetDir (int num, bool out);
// read whether GPIO is configured as output or input
extern bool gpioGetDir (int num);
// set GPIO alternate function number
extern void gpioSetAlt (int num, int altfn);
// get GPIO alternate function number
extern uint32 gpioGetAlt (int num);
// Read GPIO pin state
extern int gpioGetState (int num);
// Set GPIO pin state
extern void gpioSetState (int num, bool state);
// Read GPIO sleep mode pin state
extern int gpioGetSleepState (int num);
// Set GPIO sleep mode pin state
extern void gpioSetSleepState (int num, bool state);
// Watch for given number of seconds which GPIO pins change
extern void gpioWatch (uint seconds);
// Dump the overall GPIO state
extern bool gpioDump (void (*out) (void *data, const char *, ...),
                      void *data, uint32 *args);
// Dump GPIO state in a linux-specific format
extern bool gpioDumpState (void (*out) (void *data, const char *, ...),
                           void *data, uint32 *args);

// GPLR access for scripting
uint32 gpioScrGPLR (bool setval, uint32 *args, uint32 val);
// GPDR access for scripting
uint32 gpioScrGPDR (bool setval, uint32 *args, uint32 val);
// GAFR access for scripting
uint32 gpioScrGAFR (bool setval, uint32 *args, uint32 val);
// Get CPU family
uint32 cpuGetFamily (bool setval, uint32 *args, uint32 val);

extern uint32 gpioIgnore [];

#endif /* _IO_H */
