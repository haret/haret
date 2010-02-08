/*
    GPIO interface for Samsung S3Cx4xx Cpu
    Copyright (C) 2009 Tanguy Pruvot

    For conditions of use see file COPYING
*/

#ifndef _IO_H
#define _IO_H

#include "s3c.h"

// Read GPIO pin state (value)
extern int s3c_gpioGetState(int num);
// Set GPIO pin state
extern void s3c_gpioSetState(int num, int state);

// read whether GPIO is configured as output or input
extern int s3c_gpioGetDir(int num);
// set GPIO as output (true) or input (false)
extern void s3c_gpioSetDir(int num, int dir);

// Read GPIO sleep mode pin state
extern int s3c_gpioGetSleepDir(int num);
// Set GPIO sleep mode pin state
extern void s3c_gpioSetSleepDir(int num, int state);

// Get PullUp/Down pin state
extern int s3c_gpioGetPUD(int num);
// Set PullUp/Down pin state
extern void s3c_gpioSetPUD(int num, int state);

// Get PullUp/Down pin state for sleep mode
extern int s3c_gpioGetPUD(int num);
// Set PullUp/Down pin state for sleep mode
void s3c_gpioSetSleepPUD(int num, int state);

#endif /* _IO_H */
