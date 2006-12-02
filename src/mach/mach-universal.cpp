#include "arch-pxa.h" // MachinePXA27x
#include "mach-types.h"
//#include "asic3.h"
#include "memory.h" // memPhysSize
#include "pxa-regs.h"

class MachUniversal : public MachinePXA27x {
public:
    uint16 *gpscr;
    MachUniversal() {
        name = "HTC Universal";
        OEMInfo[0] = L"PU10";
        machType = MACH_TYPE_HTCUNIVERSAL;
        //fbDuringBoot = 0;
    }
    void init() {
#if 0
        asic3_gpio_base=0x10000000;
        asic3_sdio_base=0x0c000000;
        asic3_bus_shift=1;
#endif
        memPhysSize=64*1024*1024;
    }
#if 0
    int getBoardID() {
        return (asic3gpioGetState(('C'-'A')*16+ 3)*8+
                asic3gpioGetState(('C'-'A')*16+11)*4+
                asic3gpioGetState(('C'-'A')*16+12)*2+
                asic3gpioGetState(('C'-'A')*16+13));
    }
#endif
#if 0
    int preHardwareShutdown() {
        MachinePXA27x::preHardwareShutdown();
        gpscr = (uint16 *)memPhysMap (asic3_gpio_base);
        return 0;
    }
    void hardwareShutdown() {
        MachinePXA27x::hardwareShutdown();

//	 ipaq_asic3_set_gpio_out_d (&htcuniversal_asic3.dev, 1<<GPIOD_FL_PWR_ON, 0);
//       pxa_set_cken(CKEN1_PWM1, 0);
            
//     vasic3gpioSetState (gpscr,'D'-'A',GPIOD_FL_PWR_ON, 0);
        (gpscr[0x4/2+(0x80*3)/2])&=~(1<<GPIOD_FL_PWR_ON);

        *cken=(*cken)&(~CKEN1_PWM1); /* disable backlight clock*/

#if 0
//		ipaq_asic3_set_gpio_out_c(&htcuniversal_asic3.dev, 1<<GPIOC_LCD_PWR2_ON, 0);
//		mdelay(100);

        asic3gpioSetState (('C'-'A')*16+GPIOC_LCD_PWR2_ON, 0);
        mdelay(100);

//		ipaq_asic3_set_gpio_out_d(&htcuniversal_asic3.dev, 1<<GPIOD_LCD_PWR4_ON, 0);
//		mdelay(10);

        asic3gpioSetState (('D'-'A')*16+GPIOD_LCD_PWR4_ON, 0);

//		ipaq_asic3_set_gpio_out_a(&htcuniversal_asic3.dev, 1<<GPIOA_LCD_PWR5_ON, 0);
//		mdelay(1);

        asic3gpioSetState (('A'-'A')*16+GPIOA_LCD_PWR5_ON, 0);

//		ipaq_asic3_set_gpio_out_b(&htcuniversal_asic3.dev, 1<<GPIOB_LCD_PWR3_ON, 0);
//		mdelay(1);

        asic3gpioSetState (('B'-'A')*16+GPIOB_LCD_PWR3_ON, 0);

//		ipaq_asic3_set_gpio_out_c(&htcuniversal_asic3.dev, 1<<GPIOC_LCD_PWR1_ON, 0);

        asic3gpioSetState (('C'-'A')*16+GPIOC_LCD_PWR1_ON, 0);

//		SET_HTCUNIVERSAL_GPIO(LCD1,0);
//		SET_HTCUNIVERSAL_GPIO(LCD2,0);
#endif

        *cken&=~CKEN16_LCD; /* disable LCD clock */
    }
#endif
};

REGMACHINE(MachUniversal)
