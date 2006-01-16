/*
    Linux loader for Windows CE
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING

	Split from linboot.cpp by Ben Dooks

	$Id: cpu-pxa.cpp,v 1.4 2006/01/16 19:13:15 zap Exp $
*/


#include "haret.h"
#include "xtypes.h"
#define CONFIG_ACCEPT_GPL
#include "setup.h"
#include "memory.h"
#include "util.h"
#include "output.h"
#include "gpio.h"
#include "video.h"
#include "cpu.h"
#include "resource.h"

// see Intel XScale Core developers manual, CP15 register 0
static bool pxaDetect ()
{
  uint32 p15r0 = cpuGetCP (15, 0);

  if ((p15r0 >> 24) == 'i'
   && (
        (((p15r0 >> 13) & 7) == 1) ||   // XScale core version 1
        (((p15r0 >> 13) & 7) == 2)      // XScale core version 2
      ))
    return true;

  return false;
}

static void pxaResetDMA (pxaDMA *dma)
{
  int i;
  // Disable DMA interrupts
  dma->DINT = 0;
  // Clear DDADRx
  for (i = 0; i < 16; i++)
  {
    dma->Desc [i].DDADR = DDADR_STOP;
    dma->Desc [i].DCMD = 0;
  }
  // Set DMAs to Stop state
  for (i = 0; i < 16; i++)
    dma->DCSR [i] = DCSR_NODESC | DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR;
  // Clear DMA requests to channel map registers (just in case)
  for(i = 0; i < 40; i ++)
    dma->DRCMR [i] = 0;
}

static void pxaResetUDC (pxaUDC *udc)
{
  udc->UDCCS [ 2] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->UDCCS [ 7] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->UDCCS [12] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->UDCCS [ 1] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->UDCCS [ 6] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->UDCCS [11] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->UDCCS [ 3] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->UDCCS [ 8] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->UDCCS [13] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->UDCCS [ 4] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->UDCCS [ 9] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->UDCCS [11] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->UDCCS [ 5] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
  udc->UDCCS [10] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
  udc->UDCCS [15] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
}

static uint32 old_icmr;
static uint32 *icmr;
static pxaDMA *dma;
static pxaUDC *udc;

static int pxaSetupLoad(void)
{
  /* Map now everything we'll need later */
  icmr = (uint32 *)memPhysMap (ICMR);
  dma = (pxaDMA *)memPhysMap (0x40000000);
  udc = (pxaUDC *)memPhysMap (UDC_BASE_ADDR);

  return 0;
}

static int pxaShutdownPeripherals(void)
{
  old_icmr = *icmr;
  *icmr = 0;

  pxaResetDMA (dma);
  pxaResetUDC (udc);

  return 0;
}

static int pxaAttemptRecovery (void)
{
  *icmr = old_icmr;
  return 0;
}

cpu_fns cpu_pxa =
{
  L"PXA2xx",
  pxaDetect,
  pxaSetupLoad,
  pxaShutdownPeripherals,
  pxaAttemptRecovery
};
