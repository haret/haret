#include "memory.h" // memPhysMap
#include "script.h" // runMemScript
#include "arch-omap.h"

MachineOMAP850::MachineOMAP850()
{
    name = "Generic TI OMAP";
    archname = "OMAP850";
    CPUInfo[0] = L"OMAP850";
}

#define OMAP_DMA_BASE			0xfffed800

#define OMAP_DMA_CSDP_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x00)
#define OMAP_DMA_CCR_REG(n)             __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x02)
#define OMAP_DMA_CICR_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x04)
#define OMAP_DMA_CSR_REG(n)             __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x06)
#define OMAP_DMA_CEN_REG(n)             __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x10)
#define OMAP_DMA_CFN_REG(n)             __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x12)
#define OMAP_DMA_CSFI_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x14)
#define OMAP_DMA_CSEI_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x16)
#define OMAP_DMA_CSAC_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x18)
#define OMAP_DMA_CDAC_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x1a)
#define OMAP_DMA_CDEI_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x1c)
#define OMAP_DMA_CDFI_REG(n)            __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x1e)
#define OMAP_DMA_CLNK_CTRL_REG(n)       __REG16(OMAP_DMA_BASE + 0x40 * (n) + 0x28)

#define __REG16(x) (dma[(x-OMAP_DMA_BASE)>>1])

#define OMAP_DMA_CCR_EN			(1 << 7)

void
MachineOMAP850::init()
{
    runMemScript("set ramaddr 0x10000000\n"
                 // IRQs
                 "addlist irqs P2V(0xfffecb00) 0x40601c2 32 0\n"
                 "addlist irqs P2V(0xfffe0000) 0x0 32 0\n"
                 "addlist irqs P2V(0xfffe0100) 0 32 0\n"
                 "addlist irqs P2V(0xfffbc014) 0 32 0\n"
                 "addlist irqs P2V(0xfffbc814) 0 32 0\n"
                 "addlist irqs P2V(0xfffbd014) 0 32 0\n"
                 "addlist irqs P2V(0xfffbd814) 0 32 0\n"
                 "addlist irqs P2V(0xfffbe014) 0 32 0\n"
                 "addlist irqs P2V(0xfffbe814) 0 32 0\n"
                 // GPIOs
                 // DATA_INPUT
                 "addlist gpios P2V(0xfffbc000)\n"
                 "addlist gpios P2V(0xfffbc800)\n"
                 "addlist gpios P2V(0xfffbd000)\n"
                 "addlist gpios P2V(0xfffbd800)\n"
                 "addlist gpios P2V(0xfffbe000)\n"
                 "addlist gpios P2V(0xfffbe800)\n"
                 // DATA_OUTPUT
                 "addlist gpios P2V(0xfffbc004)\n"
                 "addlist gpios P2V(0xfffbc804)\n"
                 "addlist gpios P2V(0xfffbd004)\n"
                 "addlist gpios P2V(0xfffbd804)\n"
                 "addlist gpios P2V(0xfffbe004)\n"
                 "addlist gpios P2V(0xfffbe804)\n"
                 // DIR_CONTROL
                 "addlist gpios P2V(0xfffbc008)\n"
                 "addlist gpios P2V(0xfffbc808)\n"
                 "addlist gpios P2V(0xfffbd008)\n"
                 "addlist gpios P2V(0xfffbd808)\n"
                 "addlist gpios P2V(0xfffbe008)\n"
                 "addlist gpios P2V(0xfffbe808)\n"
                 // INT_CONTROL
                 "addlist gpios P2V(0xfffbc00c)\n"
                 "addlist gpios P2V(0xfffbc80c)\n"
                 "addlist gpios P2V(0xfffbd00c)\n"
                 "addlist gpios P2V(0xfffbd80c)\n"
                 "addlist gpios P2V(0xfffbe00c)\n"
                 "addlist gpios P2V(0xfffbe80c)\n"
                 // INT_MASK
                 "addlist gpios P2V(0xfffbc010)\n"
                 "addlist gpios P2V(0xfffbc810)\n"
                 "addlist gpios P2V(0xfffbd010)\n"
                 "addlist gpios P2V(0xfffbd810)\n"
                 "addlist gpios P2V(0xfffbe010)\n"
                 "addlist gpios P2V(0xfffbe810)\n"
                 // A few clock registers
                 "newvar CLOCKS GPIOS 'Architecture clock registers'\n"
                 "addlist clocks P2V(0xfffece00)\n" // ARM_CKCTL
                 "addlist clocks P2V(0xfffece04)\n" // ARM_IDLECT1
                 "addlist clocks P2V(0xfffece08)\n" // ARM_IDLECT2
                 "addlist clocks P2V(0xfffece0C)\n" // ARM_EWUPCT
                 "addlist clocks P2V(0xfffece10)\n" // ARM_RSTCT1
                 "addlist clocks P2V(0xfffece14)\n" // ARM_RSTCT2
                 "addlist clocks P2V(0xfffece18)\n" // ARM_SYSST
                 "addlist clocks P2V(0xfffece24)\n" // ARM_IDLECT3
                 "addlist clocks P2V(0xfffecf00)\n" // DPLL_CTL
                 "addlist clocks P2V(0xfffe0830)\n" // ULPD_CLOCK_CTRL
                 "addlist clocks P2V(0xfffe0834)\n" // ULPD_SOFT_REQ
                 "addlist clocks P2V(0xfffe1080)\n" // MOD_CONF_CTRL_0
                 "addlist clocks P2V(0xfffe1110)\n" // MOD_CONF_CTRL_1
                 "addlist clocks P2V(0xfffe0874)\n" // SWD_CLK_DIV_CTRL_SEL
                 "addlist clocks P2V(0xfffe0878)\n" // COM_CLK_DIV_CTRL_SEL
                 "addlist clocks P2V(0xfffe0900)\n" // OMAP730_PCC_UPLD_CTRL
                 );
}

int
MachineOMAP850::preHardwareShutdown()
{
    /* Map now everything we'll need later */
    dma = (uint16 *)memPhysMap(OMAP_DMA_BASE);
    if (! dma)
        return -1;
    return 0;
}

static void
omapResetDMA(volatile uint16 *dma, int chancount)
{
    uint16 status;
    for (int i = 0; i < chancount; i++) {
        /* Disable all DMA interrupts for the channel. */
        OMAP_DMA_CICR_REG(i) = 0;

        /* Set the STOP_LNK bit */
        OMAP_DMA_CLNK_CTRL_REG(i) |= 1 << 14;

        /* Make sure the DMA transfer is stopped. */
        OMAP_DMA_CCR_REG(i) = 0;
        OMAP_DMA_CCR_REG(i) &= ~OMAP_DMA_CCR_EN;

        /* Clear pending interrupts */
        status = OMAP_DMA_CSR_REG(i);
    }
}

void
MachineOMAP850::hardwareShutdown()
{
    omapResetDMA(dma, 17);
}

REGMACHINE(MachineOMAP850)
