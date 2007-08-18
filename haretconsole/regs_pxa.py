# Register definitions for PXA processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
regOneBits = memalias.regOneBits
regTwoBits = memalias.regTwoBits


######################################################################
# PXA
######################################################################

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
    }
memalias.RegsList['ARCH:PXA27x'] = Regs_pxa27x

# HTC Apache specific registers
Regs_Apache = Regs_pxa27x.copy()
Regs_Apache.update({
    0x0a000000: ("cpldirq", regOneBits("CPLD")),
    })
memalias.RegsList['Apache'] = Regs_Apache

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
