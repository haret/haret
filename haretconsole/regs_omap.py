# Register definitions for PXA processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias


######################################################################
# omap850
######################################################################

Regs_omap850 = {
    0xfffecb00: ("IH1", (lambda bit: "IH1-%d" % bit)),
    0xfffe0000: ("IH2", (lambda bit: "IH2-%d" % bit)),
    0xfffe0100: ("IH2", (lambda bit: "IH2-%d" % (bit + 32))),
    0xfffbc014: ("GIRQ1", (lambda bit: "GPIO%d" % bit)),
    0xfffbc814: ("GIRQ2", (lambda bit: "GPIO%d" % (bit + 32))),
    0xfffbd014: ("GIRQ3", (lambda bit: "GPIO%d" % (bit + 64))),
    0xfffbd814: ("GIRQ4", (lambda bit: "GPIO%d" % (bit + 96))),
    0xfffbe014: ("GIRQ5", (lambda bit: "GPIO%d" % (bit + 128))),
    0xfffbe814: ("GIRQ6", (lambda bit: "GPIO%d" % (bit + 160))),

    0xfffbc000: ("GPIO1", (lambda bit: "GPIO%d" % bit)),
    0xfffbc800: ("GPIO2", (lambda bit: "GPIO%d" % (bit + 32))),
    0xfffbd000: ("GPIO3", (lambda bit: "GPIO%d" % (bit + 64))),
    0xfffbd800: ("GPIO4", (lambda bit: "GPIO%d" % (bit + 96))),
    0xfffbe000: ("GPIO5", (lambda bit: "GPIO%d" % (bit + 128))),
    0xfffbe800: ("GPIO6", (lambda bit: "GPIO%d" % (bit + 160))),
    }
memalias.RegsList['ARCH:OMAP850'] = Regs_omap850
