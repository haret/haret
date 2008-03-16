# Register definitions for ASIC3 chips
#
# (C) Copyright 2007 Philipp Zabel <philipp.zabel@gmail.com>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
regOneBits = memalias.regOneBits


######################################################################
# HTC ASIC3
######################################################################

ASIC3_GPIO_OFFSET = {
    0x00: ("Mask", ()),
    0x04: ("Direction", ()),
    0x08: ("Out", ()),
    0x0c: ("TriggerType", ()),
    0x10: ("EdgeTrigger", ()),
    0x14: ("LevelTrigger", ()),
    0x18: ("SleepMask", ()),
    0x1c: ("SleepOut", ()),
    0x20: ("BattFaultOut", ()),
    0x24: ("IntStatus", ()),
    0x28: ("AltFunction", ()),
    0x2c: ("SleepConf", ()),
    0x30: ("Status", regOneBits("")),
}

ASIC3_SPI_OFFSET = {
    0x00: ("Control", ()),
    0x04: ("TxData", ()),
    0x08: ("RxData", ()),
    0x0c: ("Int", ()),
    0x10: ("Status", ()),
}

ASIC3_PWM_OFFSET = {
    0x00: ("TimeBase", ()),
    0x04: ("PeriodTime", ()),
    0x08: ("DutyTime", ()),
}

ASIC3_LED_OFFSET = {
    0x00: ("TimeBase", ()),
    0x04: ("PeriodTime", ()),
    0x08: ("DutyTime", ()),
    0x0c: ("AutoStopCount", ()),
}

ASIC3_CLOCK_OFFSET = {
    0x00: ("CDEX", ()),
    0x04: ("SEL", ()),
}

ASIC3_INTR_OFFSET = {
    0x00: ("IntMask", ()),
    0x04: ("PIntStat", ()),
    0x08: ("IntCPS", ()),
    0x0c: ("IntTBS", ()),
}

ASIC3_OWM_OFFSET = {
    0x00: ("CMD", ()),
    0x04: ("DAT", ()),
    0x08: ("INTR", ()),
    0x0c: ("INTEN", ()),
    0x10: ("CLKDIV", ()),
}

ASIC3_SDHWCTRL_OFFSET = {
    0x00: ("SDConf", ()),
}

ASIC3_EXTCF_OFFSET = {
    0x00: ("Select", ()),
    0x04: ("Reset", ()),
}

ASIC3_BANK = {
    0x0000: ("GPIO_A",    ASIC3_GPIO_OFFSET),
    0x0100: ("GPIO_B",    ASIC3_GPIO_OFFSET),
    0x0200: ("GPIO_C",    ASIC3_GPIO_OFFSET),
    0x0300: ("GPIO_D",    ASIC3_GPIO_OFFSET),
    0x0400: ("SPI",       ASIC3_SPI_OFFSET),
    0x0500: ("PWM_0",     ASIC3_PWM_OFFSET),
    0x0600: ("PWM_1",     ASIC3_PWM_OFFSET),
    0x0700: ("LED_0",     ASIC3_LED_OFFSET),
    0x0800: ("LED_1",     ASIC3_LED_OFFSET),
    0x0900: ("LED_2",     ASIC3_LED_OFFSET),
    0x0a00: ("CLOCK",     ASIC3_CLOCK_OFFSET),
    0x0b00: ("INTR",      ASIC3_INTR_OFFSET),
    0x0c00: ("OWM",       ASIC3_OWM_OFFSET),
    0x0e00: ("SDHWCTRL",  ASIC3_SDHWCTRL_OFFSET),
#   0x1000: ("HWPROTECT", ASIC3_HWPROTECT_OFFSET), # 12 words
    0x1100: ("EXTCF",     ASIC3_EXTCF_OFFSET),
}

# FIXME: add SD banks (SD_CONFIG, SD_CTRL, SDIO_CTRL)

def getASIC3Defs(base, sd_base, shift=0):
    out = {}
    for bank, btuple in ASIC3_BANK.items():
        for offset, function in btuple[1].items():
            name = btuple[0] + "_" + function[0]
            bits = [(bit, btuple[0] + bname)
                    for bit, bname in function[1]]
            out[base + ((bank+offset)>>shift)] = name, bits
    return out
