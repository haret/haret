# Register definitions for miscellaneous chips
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
import regs_s3c
import regs_pxa
import regs_imx
import regs_msm
import regs_omap
import regs_ati
import regs_asic3

regOneBits = memalias.regOneBits


######################################################################
# HTC cpld egpio chip
######################################################################

def getEGPIOdefs(base, count, start=0):
    out = {}
    for i in range(count):
        char = chr(ord('A')+i+start)
        out[base+2*i] = ("cpld" + char, regOneBits("C" + char))
    return out


######################################################################
# Various machine definitions
######################################################################

# HTC Apache specific registers
Regs_Apache = regs_pxa.Regs_pxa27x.copy()
Regs_Apache.update(getEGPIOdefs(0x0a000000, 3))
memalias.RegsList['Apache'] = Regs_Apache

# HTC Hermes specific registers
Regs_Hermes = regs_s3c.Regs_s3c2442.copy()
Regs_Hermes.update(getEGPIOdefs(0x08000000, 5))
Regs_Hermes.update(regs_ati.getWxxxxDefs(0x10000000))
memalias.RegsList['Hermes'] = Regs_Hermes

# HTC Kaiser specific registers
Regs_Kaiser = regs_msm.Regs_msm7xxx.copy()
Regs_Kaiser.update(getEGPIOdefs(0x98000000, 10))
memalias.RegsList['Kaiser'] = Regs_Kaiser

# HTC Roadster (HP iPAQ hx4700) specific registers
Regs_HX4700 = regs_pxa.Regs_pxa27x.copy()
Regs_HX4700.update(getEGPIOdefs(0x14000000, 1))
Regs_HX4700.update(regs_asic3.getASIC3Defs(base=0x0c000000, sd_base=0x0e000000
                                           , shift=1))
Regs_HX4700.update(regs_ati.getWxxxxDefs(0x08000000))
memalias.RegsList['HX4700'] = Regs_HX4700

# HTC Athena specific registers
Regs_Athena = regs_pxa.Regs_pxa27x.copy()
Regs_Athena.update(getEGPIOdefs(0x08000000, 8))
Regs_Athena.update(getEGPIOdefs(0x09000000, 1, 8))
Regs_Athena.update(regs_ati.getWxxxxDefs(0x04000000))
memalias.RegsList['Athena'] = Regs_Athena

# HTC Universal specific registers
Regs_Universal = regs_pxa.Regs_pxa27x.copy()
Regs_Universal.update(getEGPIOdefs(0x0a000000, 1))
Regs_Universal.update(regs_asic3.getASIC3Defs(base=0x10000000, sd_base=0x0c000000
                                              , shift=1))
memalias.RegsList['Universal'] = Regs_Universal

# HTC Sable specific registers
Regs_Sable = regs_pxa.Regs_pxa27x.copy()
#Regs_Universal.update(getEGPIOdefs(0x0a000000, 1))
Regs_Sable.update(regs_asic3.getASIC3Defs(base=0x10000000, sd_base=0x0c000000
                                              , shift=1))
memalias.RegsList['Sable'] = Regs_Sable

# HTC Dresden (aka FSC Loox N560) specific registers
Regs_Loox5xx = regs_pxa.Regs_pxa27x.copy()
Regs_Loox5xx.update(getEGPIOdefs(0x10000000, 8))
memalias.RegsList['Loox5xx'] = Regs_Loox5xx

# HTC Artemis specific registers
Regs_Artemis = regs_omap.Regs_omap850.copy()
Regs_Artemis.update(getEGPIOdefs(0x04000000, 9))
memalias.RegsList['Artemis'] = Regs_Artemis
