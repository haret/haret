# Register definitions for Samsung processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
regOneBits = memalias.regOneBits
regTwoBits = memalias.regTwoBits


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

cken = (
    (19, "Camera"), (18, "SPI"), (17, "IIS"), (16, "IIC"), (15, "ADC"),
    (14, "RTC"), (13, "GPIO"), (12, "UART2"), (11, "UART1"), (10, "UART0"),
    (9, "SDI"), (8, "PWMTIMER"), (7, "USBc"), (6, "USBh"), (5, "LCDC"),
    (4, "NAND"), (3, "SLEEP"), (2, "IDLE BIT"), (0, "STOP"))

Regs_s3c2442 = {
    0x4A000010: ("INTPND", irqs),
    0x56000004: ("GPADAT", regOneBits("GPA")),
    0x56000014: ("GPBDAT", regOneBits("GPB")),
    0x56000024: ("GPCDAT", regOneBits("GPC")),
    0x56000034: ("GPDDAT", regOneBits("GPD")),
    0x56000044: ("GPEDAT", regOneBits("GPE")),
    0x56000054: ("GPFDAT", regOneBits("GPF")),
    0x56000064: ("GPGDAT", regOneBits("GPG")),
    0x56000074: ("GPHDAT", regOneBits("GPH")),
    0x560000d4: ("GPJDAT", regOneBits("GPJ")),
    0x56000000: ("GPACON", regTwoBits("GDA")),
    0x56000010: ("GPBCON", regTwoBits("GDB")),
    0x56000020: ("GPCCON", regTwoBits("GDC")),
    0x56000030: ("GPDCON", regTwoBits("GDD")),
    0x56000040: ("GPECON", regTwoBits("GDE")),
    0x56000050: ("GPFCON", regTwoBits("GDF")),
    0x56000060: ("GPGCON", regTwoBits("GDG")),
    0x56000070: ("GPHCON", regTwoBits("GDH")),
    0x560000d0: ("GPJCON", regTwoBits("GDJ")),
    0x56000080: ("MISCCR", (("22-20", "BATT_FUNC"),
                            (19, "OFFREFRESH"),
                            (18, "nEN_SCLK1"))),
    0x560000a8: ("EINTPEND", regOneBits("EINT")),

    # Clocks
    0x4c000000: ("LOCKTIME", (("0-15", "M_LTIME"), ("16-31", "U_LTIME"))),
    0x4c000004: ("MPLLCON", (("0-1", "SDIV"), ("4-9", "PDIV")
                             , ("12-19", "MDIV"))),
    0x4c000008: ("UPLLCON", (("0-1", "SDIV"), ("4-9", "PDIV")
                             , ("12-19", "MDIV"))),
    0x4c00000c: ("CLKCON", cken),
    0x4c000010: ("CLKSLOW", ((7, "UCLK_ON"), (5, "MPLL_OFF"), (4, "SLOW_BIT"),
                             ("0-2", "SLOW_VAL"))),
    0x4c000014: ("CLKDIVN", ((3, "DIVN_UPLL"), ("1,2", "HDIVN"), (0, "PDIVN"))),
    0x4c000018: ("CAMDIVN", ((12, "DVS_EN"), (9, "HCLK4_HALF"),
                             (8, "HCLK3_HALF"), (5, "CAM3CK_SEL"),
                             (4, "CAMCLK_SEL"), ("0-3", "CAMCLK_DIV"))),
    }
memalias.RegsList['ARCH:s3c2442'] = Regs_s3c2442
