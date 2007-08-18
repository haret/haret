# Register definitions for Samsung processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias


######################################################################
# s3c24xx
######################################################################

irqs = (
    (31, "INT_ADC"), (30, "INT_RTC"), (29, "INT_SPI1"), (28, "INT_UART0"),
    (27, "INT_IIC"), (26, "INT_USBH"), (25, "INT_USBD"), (24, "INT_NFCON"),
    (23, "INT_UART1"), (22, "INT_SPI0"), (21, "INT_SDI"), (20, "INT_DMA3"),
    (19, "INT_DMA2"), (18, "INT_DMA1"), (17, "INT_DMA0"), (16, "INT_LCD"),
    (15, "INT_UART2"), (14, "INT_TIMER4"),
    (13, "INT_TIMER3"), (12, "INT_TIMER2"),
    (11, "INT_TIMER1"), (10, "INT_TIMER0"), (9, "INT_WDT"), (8, "INT_TICK"),
    (7, "nBATT_FLT"), (6, "INT_CAM"), (5, "EINT8_23"), (4, "EINT4_7"),
    (3, "EINT3"), (2, "EINT2"), (1, "EINT1"), (0, "EINT0"))

Regs_s3c2442 = {
    0x4A000010: ("INTPND", irqs),
    0x56000004: ("GPADAT", (lambda bit: "GPA%d" % bit)),
    0x56000014: ("GPBDAT", (lambda bit: "GPB%d" % bit)),
    0x56000024: ("GPCDAT", (lambda bit: "GPC%d" % bit)),
    0x56000034: ("GPDDAT", (lambda bit: "GPD%d" % bit)),
    0x56000044: ("GPEDAT", (lambda bit: "GPE%d" % bit)),
    0x56000054: ("GPFDAT", (lambda bit: "GPF%d" % bit)),
    0x56000064: ("GPGDAT", (lambda bit: "GPG%d" % bit)),
    0x56000074: ("GPHDAT", (lambda bit: "GPH%d" % bit)),
    0x560000d4: ("GPJDAT", (lambda bit: "GPJ%d" % bit)),
    0x56000080: ("MISCCR", (("22-20", "BATT_FUNC"),
                            (19, "OFFREFRESH"),
                            (18, "nEN_SCLK1"))),
    0x560000a8: ("EINTPEND", (lambda bit: "EINT%d" % bit)),
    }
memalias.RegsList['ARCH:s3c2442'] = Regs_s3c2442

# HTC Hermes specific registers
Regs_Hermes = Regs_s3c2442.copy()
Regs_Hermes.update({
    0x08000004: ("cpldirq", (lambda bit: "CPLD%d" % bit)),
    })
memalias.RegsList['Hermes'] = Regs_Hermes
