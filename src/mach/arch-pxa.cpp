#include <string.h> // strncmp
#include "cpu.h" // DEF_GETCPR
#include "memory.h" // memPhysMap
#include "script.h" // runMemScript
#include "arch-pxa.h"
#define CONFIG_PXA25x
#include "pxa2xx.h"

// Assembler functions
extern "C" {
    void cpuFlushCache_xscale();
}

MachinePXA::MachinePXA()
{
    name = "Generic Intel PXA";
    archname = "PXA";
    dcsr_count = 16;
    flushCache = cpuFlushCache_xscale;
}

DEF_GETCPR(get_p15r0, p15, 0, c0, c0, 0)

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

void
MachinePXA::init()
{
    runMemScript("set ramaddr 0xa0000000\n"
                 // IRQs
                 "addlist IRQS p2v(0x40D00000) 0x7f 32 0\n"
                 "addlist IRQS p2v(0x40E00048) 0 32 0\n"
                 "addlist IRQS p2v(0x40E0004c) 0 32 0\n"
                 "addlist IRQS p2v(0x40E00050) 0 32 0\n"
                 // GPIO levels
                 "addlist GPIOS p2v(0x40E00000)\n"
                 "addlist GPIOS p2v(0x40E00004)\n"
                 "addlist GPIOS p2v(0x40E00008)\n"
                 // GPIO directions
                 "addlist GPIOS p2v(0x40E0000C)\n"
                 "addlist GPIOS p2v(0x40E00010)\n"
                 "addlist GPIOS p2v(0x40E00014)\n"
                 // GPIO alt functions
                 "addlist GPIOS p2v(0x40E00054)\n"
                 "addlist GPIOS p2v(0x40E00058)\n"
                 "addlist GPIOS p2v(0x40E0005c)\n"
                 "addlist GPIOS p2v(0x40E00060)\n"
                 "addlist GPIOS p2v(0x40E00064)\n"
                 "addlist GPIOS p2v(0x40E00068)\n");
}

int
MachinePXA::preHardwareShutdown()
{
    /* Map now everything we'll need later */
    dma = (uint32 *)memPhysMap(DMA_BASE_ADDR);
    udc = (uint32 *)memPhysMap(UDC_BASE_ADDR);
    if (! dma || ! udc)
        return -1;
    return 0;
}

static void
pxaResetDMA(volatile pxaDMA *dma, int chancount)
{
    // Set DMAs to Stop state
    for (int i = 0; i < chancount; i++)
        dma->DCSR[i] = DCSR_NODESC | DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR;

    // Wait for DMAs to complete
    for (int i = 0; i < chancount; i++) {
	int timeout = 100000;
        while ((dma->DCSR[i] & DCSR_STOPSTATE) == 0 && timeout--) ;
    }
}

static void
pxaResetUDC(volatile pxaUDC *udc)
{
    udc->_UDCCR = 0;
}

void
MachinePXA::hardwareShutdown(struct fbinfo *fbi)
{
    pxaResetDMA((pxaDMA*)dma, dcsr_count);
    pxaResetUDC((pxaUDC*)udc);
}

// Returns true if the current machine was found to be PXA based.
int
testPXA()
{
    return strncmp(Mach->archname, "PXA", 3) == 0;
}

REGMACHINE(MachinePXA)
