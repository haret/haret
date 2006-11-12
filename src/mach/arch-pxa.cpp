#include "machines.h"
#include "cpu.h" // DEF_GETCPR
#include "memory.h" // memPhysMap
#define CONFIG_PXA25x
#include "pxa2xx.h"

DEF_GETCPR(get_p15r0, p15, 0, c0, c0, 0)

MachinePXA::MachinePXA()
{
    name = "Generic PXA";
}

int
MachinePXA::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'i'
            && (
                (((p15r0 >> 13) & 7) == 1) ||   // XScale core version 1
                (((p15r0 >> 13) & 7) == 2)      // XScale core version 2
                ));
}

int
MachinePXA::preHardwareShutdown()
{
    /* Map now everything we'll need later */
    dma = (uint32 *)memPhysMap(DMA_BASE_ADDR);
    udc = (uint32 *)memPhysMap(UDC_BASE_ADDR);
    return 0;
}

static void pxaResetDMA (pxaDMA *dma)
{
  int i;
  // Disable DMA interrupts
  dma->_DINT = 0;
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
  udc->_UDCCS [ 2] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->_UDCCS [ 7] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->_UDCCS [12] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->_UDCCS [ 1] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->_UDCCS [ 6] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->_UDCCS [11] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->_UDCCS [ 3] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->_UDCCS [ 8] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->_UDCCS [13] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->_UDCCS [ 4] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->_UDCCS [ 9] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->_UDCCS [11] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->_UDCCS [ 5] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
  udc->_UDCCS [10] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
  udc->_UDCCS [15] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
}

void
MachinePXA::hardwareShutdown()
{
    *icmr = 0;

    pxaResetDMA((pxaDMA *)dma);
    pxaResetUDC((pxaUDC *)udc);
}
