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
// Power Manager GPIO Sleep State Register
#define PGSR		0x40F00020
// AC97 Controller base address
#define AC97_BASE	0x40500000

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
#define POSR_FIFOE	0x00000010
  uint32 PISR;			// PCM In Status Register
#define PISR_FIFOE	0x00000010
  uint32 MCSR;			// Mic In Status Register
#define MCSR_FIFOE	0x00000010
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
  // +0x100
  uint32 MOCR;			// Modem Out Control Register
#define MOCR_FEIE	0x00000008
  uint32 Reserved4;
  uint32 MICR;			// Modem In Control Register
#define MICR_FEIE	0x00000008
  uint32 Reserved5;
  uint32 MOSR;			// Modem Out Status Register
#define MOSR_FIFOE	0x00000010
  uint32 Reserved6;
  uint32 MISR;			// Modem In Status Register
#define MISR_FIFOE	0x00000010
  uint32 Reserved7 [9];
  uint32 MODR;			// Modem FIFO Data Register
  uint32 Reserved8 [47];
  // +0x200
  uint32 codec [4][64];		// Audio Codec registers
};

// Base address of UDC
#define UDC_BASE_ADDR	0x40600000

// UDC registers structure
struct pxaUDC
{
  uint32 UDCCR;
#define UDCCR_UDE	(1 << 0)	/* UDC enable */
#define UDCCR_UDA	(1 << 1)	/* UDC active */
#define UDCCR_RSM	(1 << 2)	/* Device resume */
#define UDCCR_RESIR	(1 << 3)	/* Resume interrupt request */
#define UDCCR_SUSIR	(1 << 4)	/* Suspend interrupt request */
#define UDCCR_SRM	(1 << 5)	/* Suspend/resume interrupt mask */
#define UDCCR_RSTIR	(1 << 6)	/* Reset interrupt request */
#define UDCCR_REM	(1 << 7)	/* Reset interrupt mask */
  uint32 Reserved1 [3];
  uint32 UDCCS [15];
#define UDCCS0_OPR	(1 << 0)	/* OUT packet ready */
#define UDCCS0_IPR	(1 << 1)	/* IN packet ready */
#define UDCCS0_FTF	(1 << 2)	/* Flush Tx FIFO */
#define UDCCS0_DRWF	(1 << 3)	/* Device remote wakeup feature */
#define UDCCS0_SST	(1 << 4)	/* Sent stall */
#define UDCCS0_FST	(1 << 5)	/* Force stall */
#define UDCCS0_RNE	(1 << 6)	/* Receive FIFO no empty */
#define UDCCS0_SA	(1 << 7)	/* Setup active */
/* Bulk IN - Endpoint 1,6,11 */
#define UDCCS_BI_TFS	(1 << 0)	/* Transmit FIFO service */
#define UDCCS_BI_TPC	(1 << 1)	/* Transmit packet complete */
#define UDCCS_BI_FTF	(1 << 2)	/* Flush Tx FIFO */
#define UDCCS_BI_TUR	(1 << 3)	/* Transmit FIFO underrun */
#define UDCCS_BI_SST	(1 << 4)	/* Sent stall */
#define UDCCS_BI_FST	(1 << 5)	/* Force stall */
#define UDCCS_BI_TSP	(1 << 7)	/* Transmit short packet */
/* Bulk OUT - Endpoint 2,7,12 */
#define UDCCS_BO_RFS	(1 << 0)	/* Receive FIFO service */
#define UDCCS_BO_RPC	(1 << 1)	/* Receive packet complete */
#define UDCCS_BO_DME	(1 << 3)	/* DMA enable */
#define UDCCS_BO_SST	(1 << 4)	/* Sent stall */
#define UDCCS_BO_FST	(1 << 5)	/* Force stall */
#define UDCCS_BO_RNE	(1 << 6)	/* Receive FIFO not empty */
#define UDCCS_BO_RSP	(1 << 7)	/* Receive short packet */
/* Isochronous IN - Endpoint 3,8,13 */
#define UDCCS_II_TFS	(1 << 0)	/* Transmit FIFO service */
#define UDCCS_II_TPC	(1 << 1)	/* Transmit packet complete */
#define UDCCS_II_FTF	(1 << 2)	/* Flush Tx FIFO */
#define UDCCS_II_TUR	(1 << 3)	/* Transmit FIFO underrun */
#define UDCCS_II_TSP	(1 << 7)	/* Transmit short packet */
/* Isochronous OUT - Endpoint 4,9,14 */
#define UDCCS_IO_RFS	(1 << 0)	/* Receive FIFO service */
#define UDCCS_IO_RPC	(1 << 1)	/* Receive packet complete */
#define UDCCS_IO_ROF	(1 << 3)	/* Receive overflow */
#define UDCCS_IO_DME	(1 << 3)	/* DMA enable */
#define UDCCS_IO_RNE	(1 << 6)	/* Receive FIFO not empty */
#define UDCCS_IO_RSP	(1 << 7)	/* Receive short packet */
/* Interrupt IN - Endpoint 5,10,15 */
#define UDCCS_INT_TFS	(1 << 0)	/* Transmit FIFO service */
#define UDCCS_INT_TPC	(1 << 1)	/* Transmit packet complete */
#define UDCCS_INT_FTF	(1 << 2)	/* Flush Tx FIFO */
#define UDCCS_INT_TUR	(1 << 3)	/* Transmit FIFO underrun */
#define UDCCS_INT_SST	(1 << 4)	/* Sent stall */
#define UDCCS_INT_FST	(1 << 5)	/* Force stall */
#define UDCCS_INT_TSP	(1 << 7)	/* Transmit short packet */
  uint32 UICR[2];
#define UICR0_IM0	(1 << 0)	/* Interrupt mask ep 0 */
#define UICR0_IM1	(1 << 1)	/* Interrupt mask ep 1 */
#define UICR0_IM2	(1 << 2)	/* Interrupt mask ep 2 */
#define UICR0_IM3	(1 << 3)	/* Interrupt mask ep 3 */
#define UICR0_IM4	(1 << 4)	/* Interrupt mask ep 4 */
#define UICR0_IM5	(1 << 5)	/* Interrupt mask ep 5 */
#define UICR0_IM6	(1 << 6)	/* Interrupt mask ep 6 */
#define UICR0_IM7	(1 << 7)	/* Interrupt mask ep 7 */
#define UICR1_IM8	(1 << 0)	/* Interrupt mask ep 8 */
#define UICR1_IM9	(1 << 1)	/* Interrupt mask ep 9 */
#define UICR1_IM10	(1 << 2)	/* Interrupt mask ep 10 */
#define UICR1_IM11	(1 << 3)	/* Interrupt mask ep 11 */
#define UICR1_IM12	(1 << 4)	/* Interrupt mask ep 12 */
#define UICR1_IM13	(1 << 5)	/* Interrupt mask ep 13 */
#define UICR1_IM14	(1 << 6)	/* Interrupt mask ep 14 */
#define UICR1_IM15	(1 << 7)	/* Interrupt mask ep 15 */
  uint32 USIR[2];
  // +0x60
  uint32 UFNRH;
  uint32 UFNRL;
  uint32 UBCR2;
  uint32 UBCR4;
  uint32 UBCR7;
  uint32 UBCR9;
  uint32 UBCR12;
  uint32 UBCR14;
  // +0x80
  uint32 UDDR0 [8];	/* UDC Endpoint 0 Data Register */
  uint32 UDDR5 [8];	/* UDC Endpoint 5 Data Register */
  uint32 UDDR10 [8];	/* UDC Endpoint 10 Data Register */
  uint32 UDDR15 [8];	/* UDC Endpoint 15 Data Register */
  uint32 UDDR1 [32];	/* UDC Endpoint 1 Data Register */
  uint32 UDDR2 [32];	/* UDC Endpoint 2 Data Register */
  uint32 UDDR3 [128];	/* UDC Endpoint 3 Data Register */
  uint32 UDDR4 [128];	/* UDC Endpoint 4 Data Register */
  uint32 UDDR6 [32];	/* UDC Endpoint 6 Data Register */
  uint32 UDDR7 [32];	/* UDC Endpoint 7 Data Register */
  uint32 UDDR8 [64];	/* UDC Endpoint 8 Data Register */
  uint32 UDDR9 [128];	/* UDC Endpoint 9 Data Register */
  uint32 UDDR11 [32];	/* UDC Endpoint 11 Data Register */
  uint32 UDDR12 [32];	/* UDC Endpoint 12 Data Register */
  uint32 UDDR13 [128];	/* UDC Endpoint 13 Data Register */
  uint32 UDDR14 [128];	/* UDC Endpoint 14 Data Register */
};

#define DMA_BASE_ADDR	0x40000000

struct pxaDMA
{
  uint32 DCSR [16];
#define DCSR_RUN	(1 << 31)	/* Run Bit (read / write) */
#define DCSR_NODESC	(1 << 30)	/* No-Descriptor Fetch (read / write) */
#define DCSR_STOPIRQEN	(1 << 29)	/* Stop Interrupt Enable (read / write) */
#define DCSR_REQPEND	(1 << 8)	/* Request Pending (read-only) */
#define DCSR_STOPSTATE	(1 << 3)	/* Stop State (read-only) */
#define DCSR_ENDINTR	(1 << 2)	/* End Interrupt (read / write) */
#define DCSR_STARTINTR	(1 << 1)	/* Start Interrupt (read / write) */
#define DCSR_BUSERR	(1 << 0)	/* Bus Error Interrupt (read / write) */
  uint32 Reserved1 [44];
  uint32 DINT;
  uint32 Reserved2 [3];
  // +0x100
  uint32 DRCMR [40];	/* Request to Channel Map Register for DREQ 0 */
#define DRCMR_MAPVLD	(1 << 7)	/* Map Valid (read / write) */
#define DRCMR_CHLNUM	0x0f		/* mask for Channel Number (read / write) */
  uint32 Reserved3 [24];
  // + 0x200
  struct
  {
    uint32 DDADR;
#define DDADR_DESCADDR	0xfffffff0	/* Address of next descriptor (mask) */
#define DDADR_STOP	(1 << 0)	/* Stop (read / write) */
    uint32 DSADR;
    uint32 DTADR;
    uint32 DCMD;
#define DCMD_INCSRCADDR	(1 << 31)	/* Source Address Increment Setting. */
#define DCMD_INCTRGADDR	(1 << 30)	/* Target Address Increment Setting. */
#define DCMD_FLOWSRC	(1 << 29)	/* Flow Control by the source. */
#define DCMD_FLOWTRG	(1 << 28)	/* Flow Control by the target. */
#define DCMD_STARTIRQEN	(1 << 22)	/* Start Interrupt Enable */
#define DCMD_ENDIRQEN	(1 << 21)	/* End Interrupt Enable */
#define DCMD_ENDIAN	(1 << 18)	/* Device Endian-ness. */
#define DCMD_BURST8	(1 << 16)	/* 8 byte burst */
#define DCMD_BURST16	(2 << 16)	/* 16 byte burst */
#define DCMD_BURST32	(3 << 16)	/* 32 byte burst */
#define DCMD_WIDTH1	(1 << 14)	/* 1 byte width */
#define DCMD_WIDTH2	(2 << 14)	/* 2 byte width (HalfWord) */
#define DCMD_WIDTH4	(3 << 14)	/* 4 byte width (Word) */
#define DCMD_LENGTH	0x01fff		/* length mask (max = 8K - 1) */
  } Desc [16];
};

extern struct cpu_fns cpu_pxa;


#endif // _PXA_H
