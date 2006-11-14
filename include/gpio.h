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

#endif /* _IO_H */
