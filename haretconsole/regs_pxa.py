# Register definitions for PXA processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias

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
    0x40E00048: ("GEDR0", (lambda bit: "GPIO%d" % bit)),
    0x40E0004c: ("GEDR1", (lambda bit: "GPIO%d" % (bit+32))),
    0x40E00050: ("GEDR2", (lambda bit: "GPIO%d" % (bit+64))),
    0x40E00148: ("GEDR3", (lambda bit: "GPIO%d" % (bit+96))),

    0x40E00000: ("GPLR0", (lambda bit: "GPIO%d" % bit)),
    0x40E00004: ("GPLR1", (lambda bit: "GPIO%d" % (bit+32))),
    0x40E00008: ("GPLR2", (lambda bit: "GPIO%d" % (bit+64))),
    0x40E00100: ("GPLR3", (lambda bit: "GPIO%d" % (bit+96))),

    0x40E0000C: ("GPDR0", (lambda bit: "GPIO%d" % bit)),
    0x40E00010: ("GPDR1", (lambda bit: "GPIO%d" % (bit+32))),
    0x40E00014: ("GPDR2", (lambda bit: "GPIO%d" % (bit+64))),
    0x40E0010C: ("GPDR3", (lambda bit: "GPIO%d" % (bit+96))),
    }
memalias.RegsList['ARCH:PXA27x'] = Regs_pxa27x

# HTC Apache specific registers
Regs_Apache = Regs_pxa27x.copy()
Regs_Apache.update({
    0x0a000000: ("cpldirq", (lambda bit: "CPLD%d" % bit)),
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
