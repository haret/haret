# Register definitions for qualcomm processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
from memalias import regOneBits


######################################################################
# MSM7x00
######################################################################

Regs_msm7500 = {
    0xc0000000: ("IRQ", regOneBits("IRQ")),
    0xc0000004: ("IRQ2", regOneBits("IRQ2-")),

    0xa9200800: ("out0", regOneBits("out0-")),
    0xa9200804: ("out2", regOneBits("out2-")),
    0xa9200808: ("out3", regOneBits("out3-")),
    0xa920080c: ("out4", regOneBits("out4-")),
    0xa9200810: ("out0_en", regOneBits("out0-")),
    0xa9200814: ("out2_en", regOneBits("out2_en-")),
    0xa9200818: ("out3_en", regOneBits("out3_en-")),
    0xa920081c: ("out4_en", regOneBits("out4_en-")),
    0xa9200834: ("in0", regOneBits("in0-")),
    0xa9200838: ("in2", regOneBits("in2-")),
    0xa920083c: ("in3", regOneBits("in3-")),
    0xa9200840: ("in4", regOneBits("in4-")),
    0xa9200880: ("intr0_en", regOneBits("intr0_en-")),
    0xa9200884: ("intr2_en", regOneBits("intr2_en-")),
    0xa9200888: ("intr3_en", regOneBits("intr3_en-")),
    0xa920088c: ("intr4_en", regOneBits("intr4_en-")),

    0xa9300c00: ("out1", regOneBits("out1-")),
    0xa9300c08: ("out1_en", regOneBits("out1_en-")),
    0xa9300c20: ("in1", regOneBits("in1-")),
    0xa9300c60: ("intr1_en", regOneBits("intr1_en-")),
    }
memalias.RegsList['ARCH:MSM7500'] = Regs_msm7500
