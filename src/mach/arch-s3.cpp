#include <string.h> // strncmp
#include "arch-s3.h"
#include "arch-arm.h" // cpuFlushCache_arm920
#include "s3c.h"
#include "memory.h" // memPhysMap
#include "script.h" // runMemScript
#include "fbwrite.h" // fb_printf

MachineS3c2442::MachineS3c2442()
{
    name = "Generic Samsung s3c24xx";
    flushCache = cpuFlushCache_arm920;
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
MachineS3c2442::hardwareShutdown(struct fbinfo *fbi)
{
    s3c24xxShutdownDMA(channels);
    ResetUHC(uhcmap);
}

// Returns true if the current machine was found to be S3C64xx based.
int
testS3C24xx()
{
    return strncmp(Mach->archname, "s3c24", 5) == 0;
}

REGMACHINE(MachineS3c2442)

/****************************************************************
 * S3c64x0 - Tanguy Pruvot on Gmail
 ****************************************************************/

//#define FB_PRINTF(fbi,fmt,args...) fb_printf((fbi), STR_IN_CODE(.text.preload, fmt) , ##args )

MachineS3c6400::MachineS3c6400()
{
    name = "Generic Samsung s3c64xx";
    flushCache = cpuFlushCache_arm6;
    archname = "s3c64xx";
    CPUInfo[0] = L"SC364xx";
}

/*
VIC0                                 	 VIC1
31  INT_LCD2                       	 63*  INT_ADC
30  INT_LCD1            	         62*  INT_PENDNUP
29  INT_LCD0                       	 61  INT_SEC
28  INT_TIMER4                       	 60*  INT_RTC_ALARM
27  INT_TIMER3                       	 59  INT_IrDA
26  INT_WDT                          	 58  INT_OTG             <--- disabled, but used when using haret console :p
25  INT_TIMER2                       	 57  INT_HSMMC1
24  INT_TIMER1                       	 56  INT_HSMMC0
23  INT_TIMER0                       	 55  INT_HOSTIF
22*  INT_KEYPAD                      	 54  INT_MSM
21  INT_ARM_DMAS                     	 53*  INT_EINT4
20  INT_ARM_DMA                      	 52  INT_HSIrx
19  INT_ARM_DMAER                    	 51  INT_HSItx
18  INT_SDMA1                        	 50  INT_I2C0
17  INT_SDMA0                        	 49  INT_SPI1/INT_HSMMC2
16  INT_MFC                          	 48  INT_SPI0
15  INT_JPEG                         	 47*  INT_UHOST
14*  INT_BATF                         	 46  INT_CFC
13  INT_SCALER                       	 45  INT_NFC
12  INT_TVENC                        	 44  INT_ONENAND1
11  INT_2D                           	 43  INT_ONENAND0
10*  INT_ROTATOR                     	 42  INT_DMA1
9   INT_POST0                        	 41  INT_DMA0
8   INT_3D                           	 40  INT_UART3
7   Reserved                         	 39  INT_UART2
6   INT_I2S0 | INT_I2S1 | INT_I2SV40 	 38  INT_UART1
5   INT_I2C1                         	 37  INT_UART0
4*   INT_CAMIF_P                      	 36  INT_AC97
3*   INT_CAMIF_C                      	 35  INT_PCM1
2   INT_RTC_TIC                      	 34  INT_PCM0
1*   INT_EINT1                       	 33*  INT_EINT3
0*   INT_EINT0                       	 32  INT_EINT2           <--- disabled for wirq tracing (too much)

VIC0: 0b00000000010000000100010000011011 = 0x0040441b --> NOT = 0xffbfbbe4
VIC1: 0b11010000001000001000000000000010 = 0xd0208002           0x2fdf7ffd
*/

void
MachineS3c6400::init()
{
    runMemScript(
        "set ramaddr 0x60000000\n"
        // IRQs
        "addlist IRQS p2v(0x71200000) 0xffbfbbe4 32 0\n" //VIC0 IRQ Status [31:0]
        "addlist IRQS p2v(0x71200004) 0xffbfbbe4 32 0\n" //     FIQ Status [31:0]
        "addlist IRQS p2v(0x71200010) 0xffbfbbe4 32\n"   //     IRQ Enable [31:0]
        "addlist IRQS p2v(0x71200018) 0xffbfbbe4 32 0\n" //     Software IRQ Status [31:0]

        "addlist IRQS p2v(0x71300000) 0x2fdf7ffd 32 0\n" //VIC1 IRQ Status [31:0]
        "addlist IRQS p2v(0x71300004) 0x2fdf7ffd 32 0\n" //     FIQ Status [31:0]
        "addlist IRQS p2v(0x71300010) 0x2fdf7ffd 32\n"   //     IRQ Enable [31:0]
        "addlist IRQS p2v(0x71300018) 0x2fdf7ffd 32 0\n" //     Software IRQ Status [31:0]

        // GPIOs Data

        "addlist GPIOS p2v(0x7f008004) 0xffffff00\n " // GPADAT
        "addlist GPIOS p2v(0x7f008024) 0xffffff80\n " // GPBDAT
        "addlist GPIOS p2v(0x7f008044) 0xffffff00\n " // GPCDAT
        "addlist GPIOS p2v(0x7f008064) 0xffffffe0\n " // GPDDAT
        "addlist GPIOS p2v(0x7f008084) 0xffffffe0\n " // GPEDAT
        "addlist GPIOS p2v(0x7f0080a4) 0xffff0000\n " // GPFDAT
        "addlist GPIOS p2v(0x7f0080c4) 0xffffff80\n " // GPGDAT
        "addlist GPIOS p2v(0x7f0080e4) 0xfffffc00\n " // GPHDAT
/* LCD
        "addlist GPIOS p2v(0x7f008104) 0xffff0000\n " // GPIDAT
        "addlist GPIOS p2v(0x7f008124) 0xffff0000\n " // GPJDAT
*/
        "addlist GPIOS p2v(0x7f008808) 0xffff0000\n " // GPKDAT
        "addlist GPIOS p2v(0x7f008818) 0xffff8000\n " // GPLDAT
        "addlist GPIOS p2v(0x7f008824) 0xffffffc0\n " // GPMDAT
        "addlist GPIOS p2v(0x7f008834) 0xffff0000\n " // GPNDAT
        "addlist GPIOS p2v(0x7f008144) 0xffff0000\n " // GPODAT
        "addlist GPIOS p2v(0x7f008164) 0xffff8000\n " // GPPDAT
        "addlist GPIOS p2v(0x7f008184) 0xfffffe00\n " // GPQDAT

        // GPIO control registers
        "addlist GPIOS p2v(0x7f008000)\n"	// GPACON
        "addlist GPIOS p2v(0x7f008020)\n"	// GPBCON
        "addlist GPIOS p2v(0x7f008040)\n"	// GPCCON
        "addlist GPIOS p2v(0x7f008060)\n"	// GPDCON
        "addlist GPIOS p2v(0x7f008080)\n"	// GPECON
        "addlist GPIOS p2v(0x7f0080a0)\n"	// GPFCON
        "addlist GPIOS p2v(0x7f0080c0)\n"	// GPGCON
        "addlist GPIOS p2v(0x7f0080e0)\n"	// GPHCON
        "addlist GPIOS p2v(0x7f008100)\n"	// GPICON
        "addlist GPIOS p2v(0x7f008120)\n"	// GPJCON
        "addlist GPIOS p2v(0x7f008800)\n"	// GPKCON0
        "addlist GPIOS p2v(0x7f008804)\n"	// GPKCON1
        "addlist GPIOS p2v(0x7f008810)\n"	// GPLCON0
        "addlist GPIOS p2v(0x7f008814)\n"	// GPLCON1
        "addlist GPIOS p2v(0x7f008820)\n"	// GPMCON
        "addlist GPIOS p2v(0x7f008830)\n"	// GPNCON
        "addlist GPIOS p2v(0x7f008140)\n"	// GPOCON
        "addlist GPIOS p2v(0x7f008160)\n"	// GPPCON
        "addlist GPIOS p2v(0x7f008180)\n"	// GPQCON

        // Clock & Power registers
        "newvar CLOCKS GPIOS 'Architecture clock and power registers'\n"
        "addlist CLOCKS p2v(0x7e00f000)\n" // APLL_LOCK
        "addlist CLOCKS p2v(0x7e00f004)\n" // MPLL_LOCK
        "addlist CLOCKS p2v(0x7e00f008)\n" // EPLL_LOCK
        "addlist CLOCKS p2v(0x7e00f00c)\n" // APLL_CON
        "addlist CLOCKS p2v(0x7e00f010)\n" // MPLL_CON
        "addlist CLOCKS p2v(0x7e00f014)\n" // EPLL_CON0
        "addlist CLOCKS p2v(0x7e00f018)\n" // EPLL_CON1
        "addlist CLOCKS p2v(0x7e00f01c)\n" // CLK_SRC
        "addlist CLOCKS p2v(0x7e00f020)\n" // CLK_DIV0
        "addlist CLOCKS p2v(0x7e00f024)\n" // CLK_DIV1
        "addlist CLOCKS p2v(0x7e00f028)\n" // CLK_DIV2
        "addlist CLOCKS p2v(0x7e00f02c)\n" // CLK_OUT
        );
}

int
MachineS3c6400::preHardwareShutdown()
{
	dma_base[0] = (uint32*)memPhysMap(S3C6400_PA_DMA0);
	dma_base[1] = (uint32*)memPhysMap(S3C6400_PA_DMA1);
/*	dma_base[2] = (uint32*)memPhysMap(S3C6400_PA_SDMA0);
	dma_base[3] = (uint32*)memPhysMap(S3C6400_PA_SDMA1);
*/
	usb_sigmask = (unsigned char *)memPhysMap(S3C6410_USB_SIG_MASK);
    	usb_otgsfr = (uint32*)memPhysMap(S3C6410_PA_OTGSFR);

	if //(!dma_base[0] || !dma_base[1] || !dma_base[2] || !dma_base[3]) ||
	   (!usb_sigmask || !usb_otgsfr)
		return -1;

	return 0;
}

static inline void s3c_udelay(uint32 count)
{
	/* to be replaced by a real delay function later */
	uint32 dummy, dummy2;

	for (uint32 n=0; n<count; n++) {
		for (uint32 t=0; t<0xFFFFFFFF; t++) {
			dummy2 = dummy;
			dummy = t;
			dummy = dummy2;
		}
	}
}

void
MachineS3c6400::s3c6400ShutdownDMA(struct fbinfo *fbi)
{
	volatile uint32 * SDMA_SEL;
	volatile uint32 * DMA_CTRL;

	uint32 ch_conf, config;
	int timeout;

	int dma_ctrl, dma_ch, sdma_sel;

	int ctrl_count=4;

	const uint32 S3C6410_DMA_CTRL_LIST[] =
	{ S3C6400_PA_DMA0, S3C6400_PA_DMA1, S3C6400_PA_SDMA0, S3C6400_PA_SDMA1 };

	SDMA_SEL = (uint32*)memPhysMap(0x7E00F110);
	if (SDMA_SEL) {
		sdma_sel = SDMA_SEL[0];
		fb_printf(fbi,"%s: SDMA_SEL=%x", __func__, sdma_sel);
		if (sdma_sel == 0xcfffffff)
			//SDMA disabled
			ctrl_count = 2;

		//set to standard DMA
		SDMA_SEL[0]=0xcfffffff;
	}

	fb_printf(fbi, " set\n");

	/* 6410 : we have 2 (+2 secure) controllers with 8 channels each */
	for (dma_ctrl = 0; dma_ctrl < ctrl_count; dma_ctrl++) {

		DMA_CTRL = (uint32*) memPhysMap(S3C6410_DMA_CTRL_LIST[dma_ctrl]);
		if (DMA_CTRL) {

			config = s3c_readl(DMA_CTRL, PL080_EN_CHAN);
			if (config == 0) {
				fb_printf(fbi, "%s: controller %d already disabled\n", __func__, dma_ctrl);
				fb_printf(fbi, "%s: clear controller %d irq\n", __func__, dma_ctrl);
				s3c_writel(DMA_CTRL, PL080_TC_CLEAR, 0xFF);
				s3c_writel(DMA_CTRL, PL080_ERR_CLEAR, 0xFF);
				fb_printf(fbi, "%s: clear controller %d irq done\n", __func__, dma_ctrl);
				//continue;
			}

			for (dma_ch = 0; dma_ch < 8; dma_ch ++) {
				int offset = 0x114 + (dma_ch*0x20);

				//if (dma_ctrl < 2)
				//	ch_conf = PL080_Cx_CONFIG(dma_ch);
				//else
					ch_conf = PL080S_Cx_CONFIG(dma_ch);

				config = s3c_readl(DMA_CTRL, ch_conf);
				config |= PL080_CONFIG_HALT;
				s3c_writel(DMA_CTRL, ch_conf, config);

				timeout = 1000;
				do {
					config = s3c_readl(DMA_CTRL, ch_conf);
					fb_printf(fbi, "%s: dma_ctrl %d, dma_ch %d : t%d - config@%x/%x = %x\n", __func__, dma_ctrl, dma_ch, timeout, ch_conf, offset, config);
					if (config & PL080_CONFIG_ACTIVE)
						s3c_udelay(10);
					else
						break;
					} while (--timeout > 0);

				if (config & PL080_CONFIG_ACTIVE) {
					fb_printf(fbi, "%s: channel still active\n", __func__);
				//	return -EFAULT;
				}
				config = s3c_readl(DMA_CTRL, ch_conf);
				config &= ~PL080_CONFIG_ENABLE;
				s3c_writel(DMA_CTRL, ch_conf, config);
			}
		}

		/* clear controller interrupts */
		fb_printf(fbi, "%s: clear controller %d irq\n", __func__, dma_ctrl);
		s3c_writel(DMA_CTRL, PL080_TC_CLEAR, 0xFF);
		s3c_writel(DMA_CTRL, PL080_ERR_CLEAR, 0xFF);
		fb_printf(fbi, "%s: clear controller %d irq done\n", __func__, dma_ctrl);
	}
	fb_printf(fbi, "%s: done\n", __func__);

	return;
}

// Reset USB OTG.
void
MachineS3c6400::resetUSB(volatile unsigned char * usb_sigmask)
{
    //Set bit[16] to 0
    usb_sigmask[2] = 0;

    usb_otgsfr[0] = 0x19; //Reset Value OPHYPWR
    usb_otgsfr[2] = 1; //Reset Value ORSTCON

    //usb_sigmask[2] = 1;
}

void
MachineS3c6400::clearIRQS(void)
{
	volatile uint32 * VIC;

	//VICxINTENCLEAR = VICxINTENABLE
	VIC=(uint32*)memPhysMap(S3C6400_PA_VIC0);
	VIC[0x14/4] = 0xFFFFFF7F; //VIC[0x10/4];
	VIC=(uint32*)memPhysMap(S3C6400_PA_VIC1);
	VIC[0x14/4] = 0xFFFFFFFF; //VIC[0x10/4];
}

void
MachineS3c6400::hardwareShutdown(struct fbinfo *fbi)
{
	fb_printf(fbi, "\n\n MachineS3c6400::hardwareShutdown USB...\n");
	resetUSB(usb_sigmask);
	fb_printf(fbi, " MachineS3c6400::hardwareShutdown DMA...\n");
	s3c6400ShutdownDMA(fbi);
	fb_printf(fbi, " MachineS3c6400::hardwareShutdown Clear IRQ...\n");
	clearIRQS();
}

// Returns true if the current machine was found to be S3C64xx based.
int
testS3C64xx()
{
    return strncmp(Mach->archname, "s3c64", 5) == 0;
}

// Returns true if the current machine was found to be S3C based.
int
testS3C()
{
    return strncmp(Mach->archname, "s3c", 3) == 0;
}

REGMACHINE(MachineS3c6400)
