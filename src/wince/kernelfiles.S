/* This assembler file imports a kernel, initrd, and haret script into
 * a binary object.  This allows a kernel to be linked into an
 * executable and extracted at run time. */
        .section .rdata
        .global kernel_data
kernel_data:
        .incbin LIN_KERNEL
        .global kernel_data_end
kernel_data_end:
        .global initrd_data
initrd_data:
        .incbin LIN_INITRD
        .global initrd_data_end
initrd_data_end:
        .global script_data
script_data:
        .incbin LIN_SCRIPT
        .byte 0

        .end
