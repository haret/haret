/*
    Intel PXA CPU definitions
    Copyright (C) 2004 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _PXA_H
#define _PXA_H

#include "xtypes.h"

#define __REG(x) (x)
#define __REG2(x,y)     (*(volatile u32 *)((u32)&(__REG(x)) + (y)))

#include "pxa-regs.h"

// AC97 Controller base address
#define AC97_BASE	0x40500000

/* AC97 controller registers */
struct pxaAC97
{
  uint32 _POCR;			// PCM Out Control Register
  uint32 _PICR;			// PCM In Control Register
  uint32 _MCCR;			// Mic In Control Register
  uint32 _GCR;			// Global Control Register
  uint32 _POSR;			// PCM Out Status Register
  uint32 _PISR;			// PCM In Status Register
  uint32 _MCSR;			// Mic In Status Register
  uint32 _GSR;			// Global Status Register
  uint32 _CAR;			// Codec Access Register
  uint32 Reserved1 [7];
  uint32 _PCDR;			// PCM FIFO Data Register
  uint32 Reserved2 [7];
  uint32 _MCDR;			// Mic-in FIFO Data Register
  uint32 Reserved3 [39];
  // +0x100
  uint32 _MOCR;			// Modem Out Control Register
  uint32 Reserved4;
  uint32 _MICR;			// Modem In Control Register
  uint32 Reserved5;
  uint32 _MOSR;			// Modem Out Status Register
  uint32 Reserved6;
  uint32 _MISR;			// Modem In Status Register
  uint32 Reserved7 [9];
  uint32 _MODR;			// Modem FIFO Data Register
  uint32 Reserved8 [47];
  // +0x200
  uint32 codec [4][64];		// Audio Codec registers
};

// Base address of UDC
#define UDC_BASE_ADDR	0x40600000

// UDC registers structure
struct pxaUDC
{
  uint32 _UDCCR;
  uint32 Reserved1 [3];
  uint32 _UDCCS [15];
  uint32 _UICR[2];
  uint32 _USIR[2];
  // +0x60
  uint32 _UFNRH;
  uint32 _UFNRL;
  uint32 _UBCR2;
  uint32 _UBCR4;
  uint32 _UBCR7;
  uint32 _UBCR9;
  uint32 _UBCR12;
  uint32 _UBCR14;
  // +0x80
  uint32 _UDDR0 [8];	/* UDC Endpoint 0 Data Register */
  uint32 _UDDR5 [8];	/* UDC Endpoint 5 Data Register */
  uint32 _UDDR10 [8];	/* UDC Endpoint 10 Data Register */
  uint32 _UDDR15 [8];	/* UDC Endpoint 15 Data Register */
  uint32 _UDDR1 [32];	/* UDC Endpoint 1 Data Register */
  uint32 _UDDR2 [32];	/* UDC Endpoint 2 Data Register */
  uint32 _UDDR3 [128];	/* UDC Endpoint 3 Data Register */
  uint32 _UDDR4 [128];	/* UDC Endpoint 4 Data Register */
  uint32 _UDDR6 [32];	/* UDC Endpoint 6 Data Register */
  uint32 _UDDR7 [32];	/* UDC Endpoint 7 Data Register */
  uint32 _UDDR8 [64];	/* UDC Endpoint 8 Data Register */
  uint32 _UDDR9 [128];	/* UDC Endpoint 9 Data Register */
  uint32 _UDDR11 [32];	/* UDC Endpoint 11 Data Register */
  uint32 _UDDR12 [32];	/* UDC Endpoint 12 Data Register */
  uint32 _UDDR13 [128];	/* UDC Endpoint 13 Data Register */
  uint32 _UDDR14 [128];	/* UDC Endpoint 14 Data Register */
};

#define DMA_BASE_ADDR	0x40000000

struct pxaDMA
{
  uint32 DCSR [16];
  uint32 Reserved1 [44];
  uint32 _DINT;
  uint32 Reserved2 [3];
  // +0x100
  uint32 DRCMR [40];	/* Request to Channel Map Register for DREQ 0 */
  uint32 Reserved3 [24];
  // + 0x200
  struct
  {
    uint32 DDADR;
    uint32 DSADR;
    uint32 DTADR;
    uint32 DCMD;
  } Desc [16];
};

#endif // _PXA_H
