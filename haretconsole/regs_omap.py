# Register definitions for OMAP processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
from memalias import regOneBits, regFourBits

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

    0xfffe1010: ("OMAP730_MODE1", regOneBits("M1")),
    0xfffe1014: ("OMAP730_MODE2", regOneBits("M2")),

    0xfffe1070: ("OMAP730_IO_CONF0", regFourBits("C")),
    0xfffe1074: ("OMAP730_IO_CONF1", regFourBits("C")),
    0xfffe1078: ("OMAP730_IO_CONF2", regFourBits("C")),
    0xfffe107c: ("OMAP730_IO_CONF3", regFourBits("C")),
    0xfffe1080: ("OMAP730_IO_CONF4", regFourBits("C")),
    0xfffe1084: ("OMAP730_IO_CONF5", regFourBits("C")),
    0xfffe1088: ("OMAP730_IO_CONF6", regFourBits("C")),
    0xfffe108c: ("OMAP730_IO_CONF7", regFourBits("C")),
    0xfffe1090: ("OMAP730_IO_CONF8", regFourBits("C")),
    0xfffe1094: ("OMAP730_IO_CONF9", regFourBits("C")),
    0xfffe1098: ("OMAP730_IO_CONF10", regFourBits("C")),
    0xfffe109c: ("OMAP730_IO_CONF11", regFourBits("C")),
    0xfffe10a0: ("OMAP730_IO_CONF12", regFourBits("C")),
    0xfffe10a4: ("OMAP730_IO_CONF13", regFourBits("C")),

    # Clocks
    0xfffece00: ("ARM_CKCTL", (("0-1", "PERDIV"), ("2-3", "LCDDIV"), ("4-5", "ARMDIV"),
                               ("6-7", "DSPDIV"), ("8-9", "TCDIV"), ("10-11", "DSPMMUDIV"),
                               (13, "EN_DSPCK"))),
    0xfffece08: ("ARM_IDLECT1", ((10, "dsp/mpui related"),)),
    0xfffece04: ("ARM_IDLECT2", ((0, "EN_WDTCK"), (1, "EN_XORPCK"), (2, "EN_PERCK"),
                                 (3, "EN_LCDCK"), (4, "EN_LBCK"),
                                 (6, "EN_APICK"), (7, "EN_TIMCK"), (8, "DMACK_REQ"),
                                 (9, "EN_GPIOCK"), (11, "EN_CKOUT_ARM"))),
    0xfffece0c: ("ARM_EWUPCT", regOneBits("EWUPCT")),
    0xfffece10: ("ARM_RSTCT1", ((0, "dsp/mpui related"), (1, "DSP_EN"))),
    0xfffece14: ("ARM_RSTCT2", ((1,"dsp/mpui related"),)),
    0xfffece18: ("ARM_SYSST", ((12, "sync_scal"),)),
    0xfffece24: ("ARM_IDLECT3", ((0, "EN_OCPI_CK"), (2, "EN_TC1_CK"), (4, "EN_TC2_CK"))),
    0xfffecf00: ("DPLL_CTL", (("2-3", "BYPASS_DIV"), (4, "EN_PLL"),
                              ("5-7", "PLL_DIV"),  ("7-11", "PLL_MULT"))),
    0xfffe0830: ("ULPD_CLOCK_CTRL", ((4, "USB_MCLK_EN"),)),
    0xfffe0834: ("ULPD_SOFT_REQ", ((4, "USB_DC_CK"), (9, "UART1_CK"),
                                   (11, "UART2_CK"), (12, "MMC_CK"))),
    0xfffe1080: ("MOD_CONF_CTRL_0", ((9, "USB_HOST_HHC_UHOST_EN"),)),
    0xfffe1110: ("MOD_CONF_CTRL_1", ((16, "SOSSI_CLK_EN"),)),
    0xfffe0874: ("SWD_CLK_DIV_CTRL_SEL", ((1, "SWD_ULPD_PLL_CLK_REQ"), (2, "SDW_MCLK_INV_BIT"))),
    0xfffe0878: ("COM_CLK_DIV_CTRL_SEL", ((1, "COM_ULPD_PLL_CLK_REQ"),)),
    0xfffe0900: ("OMAP730_PCC_UPLD_CTRL", ((0, "omap input clock switch"),)),
    }
memalias.RegsList['ARCH:OMAP850'] = Regs_omap850
