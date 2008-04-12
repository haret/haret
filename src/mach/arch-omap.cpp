#include "memory.h" // memPhysMap
#include "script.h" // runMemScript
#include "arch-arm.h" // cpuFlushCache_arm926
#include "arch-omap.h"


#define OMAP_BASE		0xfffe0000
#define __REG16(x) (*(volatile uint16 *)(&base[(x)-OMAP_BASE]))
#define __REG32(x) (*(volatile uint32 *)(&base[(x)-OMAP_BASE]))


/****************************************************************
 * OMAP DMA
 ****************************************************************/

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

#define OMAP_DMA_CCR_EN			(1 << 7)

static void
omapResetDMA(volatile uint8 *base, int chancount)
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


/****************************************************************
 * OMAP IRQS
 ****************************************************************/

#define OMAP_IH1_BASE           0xfffecb00
#define OMAP_IH2_BASE           0xfffe0000

#define OMAP_IH1_ITR            __REG32(OMAP_IH1_BASE + 0x00)
#define OMAP_IH1_MIR            __REG32(OMAP_IH1_BASE + 0x04)
#define OMAP_IH1_SIR_IRQ        __REG32(OMAP_IH1_BASE + 0x10)
#define OMAP_IH1_SIR_FIQ        __REG32(OMAP_IH1_BASE + 0x14)
#define OMAP_IH1_CONTROL        __REG32(OMAP_IH1_BASE + 0x18)
#define OMAP_IH1_ILR0           __REG32(OMAP_IH1_BASE + 0x1c)
#define OMAP_IH1_ISR            __REG32(OMAP_IH1_BASE + 0x9c)

#define OMAP_IH2_ITR            __REG32(OMAP_IH2_BASE + 0x00)
#define OMAP_IH2_MIR            __REG32(OMAP_IH2_BASE + 0x04)
#define OMAP_IH2_SIR_IRQ        __REG32(OMAP_IH2_BASE + 0x10)
#define OMAP_IH2_SIR_FIQ        __REG32(OMAP_IH2_BASE + 0x14)
#define OMAP_IH2_CONTROL        __REG32(OMAP_IH2_BASE + 0x18)
#define OMAP_IH2_ILR0           __REG32(OMAP_IH2_BASE + 0x1c)
#define OMAP_IH2_ISR            __REG32(OMAP_IH2_BASE + 0x9c)

static void
omapResetIRQ(volatile uint8 *base)
{
        OMAP_IH1_MIR = 0xFFFFFFFF;
        OMAP_IH1_ITR = 0;
        OMAP_IH1_CONTROL = 0x03;

        OMAP_IH2_MIR = 0xFFFFFFFF;
        OMAP_IH2_ITR = 0;
        OMAP_IH2_CONTROL = 0x03;
}


/****************************************************************
 * OMAP init
 ****************************************************************/

// Setup ramaddr, irq, and gpio variables.
static void
initOmap(void)
{
    runMemScript(
        "set ramaddr 0x10000000\n"
        // GPIO IRQs
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
        );
}


/****************************************************************
 * OMAP850
 ****************************************************************/

MachineOMAP850::MachineOMAP850()
{
    name = "Generic TI OMAP";
    flushCache = cpuFlushCache_arm926;
    archname = "OMAP850";
    CPUInfo[0] = L"OMAP850";
}

void
MachineOMAP850::init()
{
    runMemScript(
        // IRQs
        "addlist irqs P2V(0xfffecb00) 0x40601c2 32 0\n"
        "addlist irqs P2V(0xfffe0000) 0x0 32 0\n"
        "addlist irqs P2V(0xfffe0100) 0 32 0\n"
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
    initOmap();
}

int
MachineOMAP850::preHardwareShutdown()
{
    /* Map now everything we'll need later */
    base = memPhysMap(OMAP_BASE);
    if (! base)
        return -1;
    return 0;
}

void
MachineOMAP850::hardwareShutdown(struct fbinfo *fbi)
{
    omapResetDMA(base, 17);
}

REGMACHINE(MachineOMAP850)


/****************************************************************
 * OMAP15xx
 ****************************************************************/

MachineOMAP15xx::MachineOMAP15xx()
{
    name = "Generic TI OMAP15xx";
    archname = "OMAP15xx";
    CPUInfo[0] = L"OMAP15";
    flushCache = cpuFlushCache_arm925;
}

void
MachineOMAP15xx::init()
{
    initOmap();
}

int
MachineOMAP15xx::preHardwareShutdown()
{
    /* Map now everything we'll need later */
    base = memPhysMap(OMAP_BASE);
    if (! base)
        return -1;
    return 0;
}

void
MachineOMAP15xx::hardwareShutdown(struct fbinfo *fbi)
{
    omapResetIRQ(base);
    omapResetDMA(base, 8);
}

REGMACHINE(MachineOMAP15xx)
