#include "arch-s3.h"
#include "s3c24xx.h"
#include "memory.h" // memPhysMap
#include "script.h" // runMemScript

MachineS3c2442::MachineS3c2442()
{
    name = "Generic Samsung s3c24xx";
    archname = "s3c2442";
    CPUInfo[0] = L"SC32442";
}

void
MachineS3c2442::init()
{
    runMemScript(
        "set ramaddr 0x30000000\n"
        // IRQs
        "addlist IRQS p2v(0x4A000010) 0x4030 32 0\n"
        "addlist IRQS p2v(0x560000a8) 0x0 32 0\n"
        // GPIOs
        "addlist GPIOS p2v(0x56000004)\n"
        "addlist GPIOS p2v(0x56000014)\n"
        "addlist GPIOS p2v(0x56000024)\n"
        "addlist GPIOS p2v(0x56000034)\n"
        "addlist GPIOS p2v(0x56000044)\n"
        "addlist GPIOS p2v(0x56000054)\n"
        "addlist GPIOS p2v(0x56000064)\n"
        "addlist GPIOS p2v(0x56000074)\n"
        "addlist GPIOS p2v(0x560000d4)\n"
        // GPIO functions
        "addlist GPIOS p2v(0x56000000)\n"
        "addlist GPIOS p2v(0x56000010)\n"
        "addlist GPIOS p2v(0x56000020)\n"
        "addlist GPIOS p2v(0x56000030)\n"
        "addlist GPIOS p2v(0x56000040)\n"
        "addlist GPIOS p2v(0x56000050)\n"
        "addlist GPIOS p2v(0x56000060)\n"
        "addlist GPIOS p2v(0x56000070)\n"
        "addlist GPIOS p2v(0x560000d0)\n"
        // Clock & Power registers
        "newvar CLOCKS GPIOS 'Architecture clock and power registers'\n"
        "addlist CLOCKS p2v(0x4C000000)\n" // LOCKTIME
        "addlist CLOCKS p2v(0x4C000004)\n" // MPLLCON
        "addlist CLOCKS p2v(0x4C000008)\n" // UPLLCON
        "addlist CLOCKS p2v(0x4C00000C)\n" // CLKCON
        "addlist CLOCKS p2v(0x4C000010)\n" // CLKSLOW
        "addlist CLOCKS p2v(0x4C000014)\n" // CLKDIVN
        "addlist CLOCKS p2v(0x4C000018)\n" // CAMDIVN
        );
}

static inline uint32 s3c_readl(volatile uint32 *base, uint32 reg)
{
    return base[reg/4];
}

static inline void s3c_writel(volatile uint32 *base, uint32 reg, uint32 val)
{
    base[(reg/4)] = val;
}

int
MachineS3c2442::preHardwareShutdown()
{
    channels = (uint32*)memPhysMap(S3C2410_PA_DMA);
    uhcmap = (uint32 *)memPhysMap(S3C2410_PA_USBHOST);
    if (! channels || ! uhcmap)
        return -1;
    return 0;
}

static void
s3c24xxShutdownDMA(volatile uint32 *channels)
{
    int dma_ch;
    volatile uint32 *ch;
    uint32 dmasktrig;
    int timeo;

    for (dma_ch = 0; dma_ch < 4; dma_ch++) {
        ch = channels + ((0x40 / 4) * dma_ch);

        dmasktrig = s3c_readl(ch, S3C2410_DMA_DMASKTRIG);
        if (dmasktrig & S3C2410_DMASKTRIG_ON) {
            s3c_writel(ch, S3C2410_DMA_DMASKTRIG, S3C2410_DMASKTRIG_STOP);

            timeo = 0x100000;
            while (timeo > 0) {
                dmasktrig = s3c_readl(ch, S3C2410_DMA_DMASKTRIG);

                if ((dmasktrig & S3C2410_DMASKTRIG_ON) == 0)
                    break;

                if (timeo-- <= 0)
                    break;
            }
        }
    }
}

// Reset USB host.
static void
ResetUHC(volatile uint32 *uhcmap)
{
    uhcmap[2] = 1;
}

void
MachineS3c2442::hardwareShutdown()
{
    s3c24xxShutdownDMA(channels);
    ResetUHC(uhcmap);
}

REGMACHINE(MachineS3c2442)
