# Register definitions for qualcomm processors
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import memalias
from memalias import regOneBits


######################################################################
# MSM7xxx
######################################################################

irqs0 = (
    (0, "M2A_0"), (1, "M2A_1"),(2, "M2A_2"),(3, "M2A_3"),(4, "M2A_4"),(5, "M2A_5"),(6, "M2A_6"),
    (7, "GP_TIMER"), (8,"DBG_TIMER"), (9, "UART1"), (10, "UART2"), (11, "UART3"),
    (12, "UART1_RX"), (13, "UART2_RX"), (14, "UART3_RX"), (15, "USB_OTG"), (16, "MDDI_PRI"),
    (17, "MDDI_EXT"), (18, "MDDI_CLIENT"), (19, "MDP"), (20, "GRAPHICS"), (21, "ADM_AARM"),
    (22, "ADSP_A11"), (23, "ADSP_A11_A9"), 
    (24, "SDC10"), (25, "SDC11"), (26, "SDC20"), (27, "SDC21"),
    (28, "KEYB"), (29, "TOUCH_SSBI"), (30, "TOUCH1"),
    (31, "TOUCH2"))

irqs1 = (
    (0, "GPIO1"), (1, "GPIO2"),(2, "PWB_I2C"),(3, "SOFTRST"),(4, "NAND_WR_ER"),(5, "NAND_OP"),(6, "PBUS_ARM11"),
    (7, "AXI_MPU_SMI"), (8,"AXI_MPU_EBI1"), (9, "AD_HSSD"), (10, "A11_PMU"), (11, "A11_DMA"),
    (12, "TSIF_IRQ"), (13, "UART1DM_IRQ"), (14, "UART1DM_RX"), (15, "USB_HS"), (16, "SDC30"),
    (17, "SDC31"), (18, "SDC40"), (19, "SDC41"), (20, "UART2DM_RX"),
    (21, "UART2DM_IRQ"))

cken = (
    (28, "SDC4"), (27, "SDC3"), (26, "uartDM2"), (17, "uartDM1"), (8, "SDC2"), (7, "SDC1"))

Regs_msm_gpio = {
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

    0xa8600000: ("CKEN", cken),

#    0xa92008a0: ("irq0", regOneBits("GPIO", 0)),
#    0xa9300c70: ("irq1", regOneBits("GPIO", 16)),
#    0xa92008a4: ("irq2", regOneBits("GPIO", 43)),
#    0xa92008a8: ("irq3", regOneBits("GPIO", 68)),
#    0xa92008ac: ("irq4", regOneBits("GPIO", 95)),
    }

Regs_msm7xxx = Regs_msm_gpio.copy()
Regs_msm7xxx.update({
        0xc0000000: ("IRQ0", irqs0),
        0xc0000004: ("IRQ1", irqs1),
    })
memalias.RegsList['ARCH:MSM7xxx'] = Regs_msm7xxx

Regs_msm7xxxA = Regs_msm_gpio.copy()
Regs_msm7xxxA.update({
        0xc0000080: ("IRQ0", irqs0),
        0xc0000084: ("IRQ1", irqs1),
#        0xa92008b0: ("irq5", regOneBits("GPIO", 107)),
    })
memalias.RegsList['ARCH:MSM7xxxA'] = Regs_msm7xxxA
