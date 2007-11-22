# Register definitions for ATI chips
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias


######################################################################
# ATI Wxxxx
######################################################################

Regs_ati = {
    0x10200: "mmGEN_INT_CNTL",
    0x10204: "mmGEN_INT_STATUS",
    }

def getWxxxxDefs(base):
    out = {}
    for k, v in Regs_ati.items():
        out[k+base] = v
    return out
