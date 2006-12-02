#include "cpu.h" // DEF_GETCPR
#include "memory.h" // memPhysMap
#include "arch-pxa.h"
#define CONFIG_PXA25x
#include "pxa2xx.h"

DEF_GETCPR(get_p15r0, p15, 0, c0, c0, 0)

MachinePXA::MachinePXA()
{
    name = "Generic PXA";
    dcsr_count = 16;
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

static void
pxaResetDMA(volatile pxaDMA *dma, int chancount)
{
    // Set DMAs to Stop state
    for (int i = 0; i < chancount; i++)
        dma->DCSR[i] = DCSR_NODESC | DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR;

    // Wait for DMAs to complete
    for (int i = 0; i < chancount; i++)
        while ((dma->DCSR[i] & DCSR_STOPSTATE) == 0)
            ;
}

static void
pxaResetUDC(volatile pxaUDC *udc)
{
    udc->_UDCCR = 0;
}

void
MachinePXA::hardwareShutdown()
{
    pxaResetDMA((pxaDMA*)dma, dcsr_count);
    pxaResetUDC((pxaUDC*)udc);
}
