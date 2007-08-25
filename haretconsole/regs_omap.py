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
    0xfffbc014: ("GIRQ1", regOneBits("IS")),
    0xfffbc814: ("GIRQ2", regOneBits("IS", 32)),
    0xfffbd014: ("GIRQ3", regOneBits("IS", 64)),
    0xfffbd814: ("GIRQ4", regOneBits("IS", 96)),
    0xfffbe014: ("GIRQ5", regOneBits("IS", 128)),
    0xfffbe814: ("GIRQ6", regOneBits("IS", 160)),

    0xfffbc000: ("GPIO1-IL", regOneBits("IL")),
    0xfffbc800: ("GPIO2-IL", regOneBits("IL", 32)),
    0xfffbd000: ("GPIO3-IL", regOneBits("IL", 64)),
    0xfffbd800: ("GPIO4-IL", regOneBits("IL", 96)),
    0xfffbe000: ("GPIO5-IL", regOneBits("IL", 128)),
    0xfffbe800: ("GPIO6-IL", regOneBits("IL", 160)),

    0xfffbc004: ("GPIO1-OL", regOneBits("OL")),
    0xfffbc804: ("GPIO2-OL", regOneBits("OL", 32)),
    0xfffbd004: ("GPIO3-OL", regOneBits("OL", 64)),
    0xfffbd804: ("GPIO4-OL", regOneBits("OL", 96)),
    0xfffbe004: ("GPIO5-OL", regOneBits("OL", 128)),
    0xfffbe804: ("GPIO6-OL", regOneBits("OL", 160)),

    0xfffbc008: ("GPIO1-DC", regOneBits("DC")),
    0xfffbc808: ("GPIO2-DC", regOneBits("DC", 32)),
    0xfffbd008: ("GPIO3-DC", regOneBits("DC", 64)),
    0xfffbd808: ("GPIO4-DC", regOneBits("DC", 96)),
    0xfffbe008: ("GPIO5-DC", regOneBits("DC", 128)),
    0xfffbe808: ("GPIO6-DC", regOneBits("DC", 160)),

    0xfffbc00c: ("GPIO1-IC", regOneBits("IC")),
    0xfffbc80c: ("GPIO2-IC", regOneBits("IC", 32)),
    0xfffbd00c: ("GPIO3-IC", regOneBits("IC", 64)),
    0xfffbd80c: ("GPIO4-IC", regOneBits("IC", 96)),
    0xfffbe00c: ("GPIO5-IC", regOneBits("IC", 128)),
    0xfffbe80c: ("GPIO6-IC", regOneBits("IC", 160)),

    0xfffbc010: ("GPIO1-IM", regOneBits("IM")),
    0xfffbc810: ("GPIO2-IM", regOneBits("IM", 32)),
    0xfffbd010: ("GPIO3-IM", regOneBits("IM", 64)),
    0xfffbd810: ("GPIO4-IM", regOneBits("IM", 96)),
    0xfffbe010: ("GPIO5-IM", regOneBits("IM", 128)),
    0xfffbe810: ("GPIO6-IM", regOneBits("IM", 160)),
    }
memalias.RegsList['ARCH:OMAP850'] = Regs_omap850
