/*
    Periferial I/O devices interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _IO_H
#define _IO_H

// --------------------- // GPIO control registers (three of each) // ------ //
// General Pin Level Registers (three)
#define GPLR		0x40E00000
// General Pin Direction Registers (three)
#define GPDR		0x40E0000C
// General Pin output Set registers (three)
#define GPSR		0x40E00018
// General Pin output Clear registers (three)
#define GPCR		0x40E00024
// General pin Rising Edge detect enable Register
#define GRER		0x40E00030
// General pin Falling Edge detect enable Register
#define GFER		0x40E0003C
// General pin Edge Detect status Register
#define GEDR		0x40E00048
// General pin Alternate Function Register
#define GAFR		0x40E00054

// -------------------- // CF memory card controller (two of each) // ------ //
#define MCMEM		0x48000028
#define MCATT		0x48000030
#define MCIO		0x48000038

// -------------- // Direct Memory Access controller (16 channels) // ------ //
// DMA interrupt register (one and only)
#define DINT		0x400000F0
// DMA Channel Status Register (16 registers - one for every channel)
#define DCSR		0x40000000
// DMA Request to Channel Map Register (64 registers - one for every DRQ pin)
#define DRCMR		0x40000100
// DDADR, DSADR, DTADR and DCMD are repeated in sequence 16 times
// DMA Descriptor Address Register
#define DDADR		0x40000200
// DMA Source ADdress Register
#define DSADR		0x40000204
// DMA Target ADdress Register
#define DTADR		0x40000208
// DMA Command Register
#define DCMD		0x4000020C

// Power Manager Control Register
#define PMCR            0x40F00000
// Power Manager Sleep Status Register
#define PSSR            0x40F00004
// Power Manager Scratch Pad Register
#define PSPR            0x40F00008
// Power Manager Wake-up Enable Register
#define PWER            0x40F0000C
// Power Manager GPIO Rising-Edge Detect
#define PRER            0x40F00010
// Power Manager GPIO Falling-Edge Detect
#define PFER            0x40F00014
// Power Manager GPIO Edge Detect Status
#define PEDR            0x40F00018
// Power Manager General Configuration Register
#define PCFR            0x40F0001C
// Power Manager GPIO Sleep State Register (three)
#define PGSR		0x40F00020
// Reset Controller Status Register
#define RCSR            0x40F00030
 
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

extern uint32 gpioIgnore [];

#endif /* _IO_H */
