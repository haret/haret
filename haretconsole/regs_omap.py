# Register definitions for PXA processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
regOneBits = memalias.regOneBits


######################################################################
# omap850
######################################################################

Regs_omap850 = {
    0xfffecb00: ("IH1", regOneBits("IH1-")),
    0xfffe0000: ("IH2", regOneBits("IH2-")),
    0xfffe0100: ("IH2", regOneBits("IH2-", 32)),
    0xfffbc014: ("GIRQ1", regOneBits("GPIO")),
    0xfffbc814: ("GIRQ2", regOneBits("GPIO", 32)),
    0xfffbd014: ("GIRQ3", regOneBits("GPIO", 64)),
    0xfffbd814: ("GIRQ4", regOneBits("GPIO", 96)),
    0xfffbe014: ("GIRQ5", regOneBits("GPIO", 128)),
    0xfffbe814: ("GIRQ6", regOneBits("GPIO", 160)),

    0xfffbc000: ("GPIO1", regOneBits("GPIO")),
    0xfffbc800: ("GPIO2", regOneBits("GPIO", 32)),
    0xfffbd000: ("GPIO3", regOneBits("GPIO", 64)),
    0xfffbd800: ("GPIO4", regOneBits("GPIO", 96)),
    0xfffbe000: ("GPIO5", regOneBits("GPIO", 128)),
    0xfffbe800: ("GPIO6", regOneBits("GPIO", 160)),
    }
memalias.RegsList['ARCH:OMAP850'] = Regs_omap850
