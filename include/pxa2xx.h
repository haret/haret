/*
    Intel PXA CPU definitions
    Copyright (C) 2004 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _PXA_H
#define _PXA_H

// Interrupt Controller Mask Register
#define ICMR		0x40D00004

// RTC Alarm Match Register Interrupt Pending
#define IRQ_RTCALARM	(1 << 31)
// RTC HZ Clock Tick Interrupt Pending
#define IRQ_RTCHZ	(1 << 30)
// OS Timer Match Register 3 Interrupt Pending
#define IRQ_OST_R3	(1 << 29)
// OS Timer Match Register 2 Interrupt Pending
#define IRQ_OST_R2	(1 << 28)
// OS Timer Match Register 1 Interrupt Pending
#define IRQ_OST_R1	(1 << 27)
// OS Timer Match Register 0 Interrupt Pending
#define IRQ_OST_R0	(1 << 26)
// DMA Channel Service Request Interrupt Pending
#define IRQ_DMA		(1 << 25)
// SSP Service Request Interrupt Pending
#define IRQ_SSP		(1 << 24)
// MMC Status/Error Detection Interrupt Pending
#define IRQ_MMC		(1 << 23)
// FFUART Transmit/Receive/Error Interrupt Pending
#define IRQ_FFUART	(1 << 22)
// BTUART Transmit/Receive/Error Interrupt Pending
#define IRQ_BTUART	(1 << 21)
// STUART Transmit/Receive/Error Interrupt Pending
#define IRQ_STUART	(1 << 20)
// ICP Transmit/Receive/Error Interrupt Pending
#define IRQ_ICP		(1 << 19)
// I2C Service Request Interrupt Pending
#define IRQ_I2C		(1 << 18)
// LCD Controller Service Request Interrupt Pending
#define IRQ_LCD		(1 << 17)
// AC97 Interrupt Pending
#define IRQ_AC97	(1 << 14)
// I2S Interrupt Pending
#define IRQ_I2S		(1 << 13)
// Performance Monitoring Unit (PMU) Interrupt Pending
#define IRQ_PMU		(1 << 12)
// USB Service Interrupt Pending
#define IRQ_USB		(1 << 11)
// GPIO[80:2] Edge Detect Interrupt Pending
#define IRQ_GPIO80_2	(1 << 10)
// GPIO[1] Edge Detect Interrupt Pending
#define IRQ_GPIO1	(1 << 9)
// GPIO[0] Edge Detect Interrupt Pending
#define IRQ_GPIO0	(1 << 8)


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
 
#define AC97_BASE		0x40500000

/* AC97 controller registers */
struct pxaAC97
{
  uint32 POCR;			// PCM Out Control Register
#define POCR_FEIE	0x00000008
  uint32 PICR;			// PCM In Control Register
#define PICR_FEIE	0x00000008
  uint32 MCCR;			// Mic In Control Register
#define MCCR_FEIE	0x00000008
  uint32 GCR;			// Global Control Register
#define GCR_CDONE_IE	0x00800000
#define GCR_SDONE_IE	0x00400000
#define GCR_SECRDY_IEN	0x00000200
#define GCR_PRIRDY_IEN	0x00000100
#define GCR_SECRES_IEN	0x00000020
#define GCR_PRIRES_IEN	0x00000010
#define GCR_ACLINK_OFF	0x00000008
#define GCR_WARM_RST	0x00000004
#define GCR_COLD_RST	0x00000002
#define GCR_GIE		0x00000001
  uint32 POSR;			// PCM Out Status Register
#define POSR_FIFOE	0x00000008
  uint32 PISR;			// PCM In Status Register
#define PISR_FIFOE	0x00000008
  uint32 MCSR;			// Mic In Status Register
#define MCSR_FIFOE	0x00000008
  uint32 GSR;			// Global Status Register
#define GSR_CDONE	0x00080000
#define GSR_SDONE	0x00040000
#define GSR_RDCS	0x00008000
#define GSR_BIT3SLT12	0x00004000
#define GSR_BIT2SLT12	0x00002000
#define GSR_BIT1SLT12	0x00001000
#define GSR_SECRES	0x00000800
#define GSR_PRIRES	0x00000400
#define GSR_SCR		0x00000200
#define GSR_PCR		0x00000100
#define GSR_MINT	0x00000080
#define GSR_POINT	0x00000040
#define GSR_PIINT	0x00000020
#define GSR_MOINT	0x00000004
#define GSR_MIINT	0x00000002
#define GSR_GSCI	0x00000001
  uint32 CAR;			// Codec Access Register
#define CAR_CAIP	0x00000001
  uint32 Reserved1 [7];
  uint32 PCDR;			// PCM FIFO Data Register
  uint32 Reserved2 [7];
  uint32 MCDR;			// Mic-in FIFO Data Register
  uint32 Reserved3 [39];
  uint32 MOCR;			// Modem Out Control Register
#define MOCR_FEIE	0x00000008
  uint32 Reserved4;
  uint32 MICR;			// Modem In Control Register
#define MICR_FEIE	0x00000008
  uint32 Reserved5;
  uint32 MOSR;			// Modem Out Status Register
#define MOSR_FIFOE	0x00000008
  uint32 Reserved6;
  uint32 MISR;			// Modem In Status Register
#define MISR_FIFOE	0x00000008
  uint32 Reserved7 [9];
  uint32 MODR;			// Modem FIFO Data Register
  uint32 Reserved8 [47];
  uint32 codec [4][64];		// Audio Codec registers
};

#endif // _PXA_H
