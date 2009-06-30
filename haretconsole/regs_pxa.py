# Register definitions for PXA processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
# (C) Copyright 2009 Stefan Schmidt <stefan@datenfreihafen.org>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
regOneBits = memalias.regOneBits
regTwoBits = memalias.regTwoBits


######################################################################
# PXA
######################################################################

# PXA 3xx registers
irqs1 = (
    (0, "SSP3"), (2, "USBh2"), (3, "USBh1"), (4, "Keypad"), (6, "pI2C"),
    (7, "OS Timer"), (8, "GPIO0"), (9, "GPIO1"), (10, "GPIOx"),
    (11, "USBc"), (12, "PML"), (13, "SSP4"), (14, "AC97"), # 11 not PXA31x
    (15, "USIM"), (16, "SSP2"), (17, "LCD"), (18, "I2C"), (20, "UART3"),
    (21, "UART2"), (22, "UART1"), (23, "MMC1"), (24, "SSP"), (25, "DMA"),
    (26, "TMR0"), (27, "TMR1"), (28, "TMR2"), (29, "TMR3"), (30, "RTC0"),
    (31, "RTC1"))
irqs2 = (
    (1, "QCap"), (2, "CIR"), (4, "Touchscreen"), # 4 PXA32x only
    (6, "USIM2"), (7, "GraphicsController"), (9, "MMC2"), (12, "1-Wire"),
    (13, "NAND"), (14, "U2DC"), (15, "SGP"), (16, "MVED_DMA"), # 16 PXA31x only
    (17, "EXT_WAKEUP0"), (18, "EXTERNAL_WAKEUP1"), (19, "DMC"), # 18 PXA32x only
    (20, "CLOCK"), (21, "BPB2IMG"), (22, "MVED"), (23, "MMC3")) # 21, 22, 23 PXA31x only

ckenA = (
    (29, "SSP4"), (28, "SSP3"), (27, "SSP2"), (26, "SSP1"),
    (25, "Touchscreen"), (24, "AC97"), (23, "UART3"), (22, "UART1"), # 25 PXA32x only
    (21, "UART2"), (20, "UDC"), (19, "WTM"), (18, "USIM1"), # 20 not PXA31x
    (17, "USIM0"), (15, "CIR"), (14, "Keypad"), (13, "MMC1"),
    (12, "MMC0"), (11, "BootROM"), (10, "IM"), (9, "SMC"), (8, "DMC"),
    (7, "Graphic"), (6, "U2DC"), (5, "MMC3"), (4, "NAND"), # 7 PXA32x only, 5 PXA31x only
    (3, "CI"), (2, "USBh"), (1, "LCD"))
ckenB = (
    (17, "MiniLCD"), (16, "MiniIM"), (11, "Video"), # 11 PXA31x only
    (10, "Graphic"), (9, "SystemBus"), (8, "1-Wire"), (7, "GPIO"), # 10 not PXA32x
    (4, "I2C"), (1, "PWM3+1"), (0, "PWM2+0"))

Regs_pxa3xx = {
    0x40D00000: ("ICIP", irqs1), 0x40D0009C: ("ICIP2", irqs2),
    0x40E00048: ("GEDR0", regOneBits("GPIO")),
    0x40E0004c: ("GEDR1", regOneBits("GPIO", 32)),
    0x40E00050: ("GEDR2", regOneBits("GPIO", 64)),
    0x40E00148: ("GEDR3", regOneBits("GPIO", 96)),

    0x40E00000: ("GPLR0", regOneBits("GPIO")),
    0x40E00004: ("GPLR1", regOneBits("GPIO", 32)),
    0x40E00008: ("GPLR2", regOneBits("GPIO", 64)),
    0x40E00100: ("GPLR3", regOneBits("GPIO", 96)),

    0x40E0000C: ("GPDR0", regOneBits("GPIO")),
    0x40E00010: ("GPDR1", regOneBits("GPIO", 32)),
    0x40E00014: ("GPDR2", regOneBits("GPIO", 64)),
    0x40E0010C: ("GPDR3", regOneBits("GPIO", 96)),

# FIXME: Add AF description

    # Clocks, FIXME: a lot is missing for PXA3xx here
    0x4134000C: ("CKEN_A", ckenA),
    0x41340010: ("CKEN_B", ckenB),
    0x41350000: ("OSCC", (("12", "ROS"), (11, "PEN"), (10, "TENS3"),
                          (9, "TENS2"), (8, "TENS0"), ("0-7", "VCXOST"))),
    }
memalias.RegsList['ARCH:PXA3xx'] = Regs_pxa3xx

# PXA 27x registers
irqs1 = (
    (0, "SSP3"), (1, "MSL"), (2, "USBh2"), (3, "USBh1"),
    (4, "Keypad"), (5, "MemoryStick"), (6, "pI2C"), (7, "OS Timer"),
    (8, "GPIO0"), (9, "GPIO1"), (10, "GPIOx"), (11, "USBc"),
    (12, "PMU"), (13, "I2S"), (14, "AC97"), (15, "USIM"),
    (16, "SSP2"), (17, "LCD"), (18, "I2C"), (19, "ICP"),
    (20, "STUART"), (21, "BTUART"), (22, "FFUART"), (23, "MMC"),
    (24, "SSP"), (25, "DMA"), (26, "TMR0"), (27, "TMR1"),
    (28, "TMR2"), (29, "TMR3"), (30, "RTC0"), (31, "RTC1"))
irqs2 = ((0, "TPM"), (1, "QCap"))

cken = (
    (31, "AC97conf"), (25, "TPM"), (24, "QCap"), (23, "SSP1"), (22, "mem"),
    (21, "MemoryStick"), (20, "imem"), (19, "Keypad"), (18, "USIM"),
    (17, "MSL"), (16, "LCD"), (15, "pI2C"), (14, "I2C"), (13, "IR"),
    (12, "MMC"), (11, "USBc"), (10, "USBh"), (9, "TMR"), (8, "I2S"),
    (7, "BTUART"), (6, "FFUART"), (5, "STUART"), (4, "SSP3"),
    (3, "SSP2"), (2, "AC97"), ("0,1", "PWM"))

Regs_pxa27x = {
    0x40D00000: ("ICIP", irqs1), 0x40D0009C: ("ICIP2", irqs2),
    0x40E00048: ("GEDR0", regOneBits("GPIO")),
    0x40E0004c: ("GEDR1", regOneBits("GPIO", 32)),
    0x40E00050: ("GEDR2", regOneBits("GPIO", 64)),
    0x40E00148: ("GEDR3", regOneBits("GPIO", 96)),

    0x40E00000: ("GPLR0", regOneBits("GPIO")),
    0x40E00004: ("GPLR1", regOneBits("GPIO", 32)),
    0x40E00008: ("GPLR2", regOneBits("GPIO", 64)),
    0x40E00100: ("GPLR3", regOneBits("GPIO", 96)),

    0x40E0000C: ("GPDR0", regOneBits("GPIO")),
    0x40E00010: ("GPDR1", regOneBits("GPIO", 32)),
    0x40E00014: ("GPDR2", regOneBits("GPIO", 64)),
    0x40E0010C: ("GPDR3", regOneBits("GPIO", 96)),

    0x40E00054: ("GAFR0_L", regTwoBits("AF")),
    0x40E00058: ("GAFR0_U", regTwoBits("AF", 16)),
    0x40E0005c: ("GAFR1_L", regTwoBits("AF", 32)),
    0x40E00060: ("GAFR1_U", regTwoBits("AF", 48)),
    0x40E00064: ("GAFR2_L", regTwoBits("AF", 64)),
    0x40E00068: ("GAFR2_U", regTwoBits("AF", 80)),
    0x40E0006c: ("GAFR3_L", regTwoBits("AF", 96)),
    0x40E00070: ("GAFR3_U", regTwoBits("AF", 112)),

    # Clocks
    0x41300000: ("CCCR", (("0-4", "L"), ("7-10", "2N"), (25, "A"),
                          (26, "PLL_EARLY_EN"), (27, "LCD_26"), (30, "PPDIS"),
                          (31, "CPDIS"))),
    0x41300004: ("CKEN", cken),
    0x41300008: ("OSCC", (("5,6", "OSD"), (4, "CRI"), (3, "PIO_EN"),
                          (2, "TOUT_EN"), (1, "OON"), (0, "OOK"))),
    0x4130000c: ("CCSR", (("0-4", "L_S"), ("7-9", "2N_S"),
                          (28, "PPLCK"), (29, "CPLCK"), (30, "PPDIS_S"),
                          (31, "CPDIS_S"))),
    "insn:ee160e10": ("CLKCFG", ((3, "B"), (2, "HT"), (1, "F"), (0, "T"))),
    "insn:ee170e10": ("PWRMODE", ((3, "VC"), ("0-2", "M"))),
    }
memalias.RegsList['ARCH:PXA27x'] = Regs_pxa27x

# PXA 26x registers
irqs1 = (
    (7, "HW uart"),
    (8, "GPIO0"), (9, "GPIO1"), (10, "GPIOx"), (11, "USB"),
    (12, "PMU"), (13, "I2S"), (14, "AC97"), (15, "aSSP"),
    (16, "nSSP"), (17, "LCD"), (18, "I2C"), (19, "ICP"),
    (20, "STUART"), (21, "BTUART"), (22, "FFUART"), (23, "MMC"),
    (24, "SSP"), (25, "DMA"), (26, "TMR0"), (27, "TMR1"),
    (28, "TMR2"), (29, "TMR3"), (30, "RTC0"), (31, "RTC1"))
Regs_pxa = Regs_pxa27x.copy()
Regs_pxa.update({
    0x40D00000: ("ICIP", irqs1)
    })
memalias.RegsList['ARCH:PXA'] = Regs_pxa
