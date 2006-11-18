/* This assembler file imports a kernel and initrd (along with a
 * command line) into a binary object.  This allows a kernel to be
 * linked into an executable and extracted at run time. */
        .section .rdata
        .global  kernel_data
kernel_data:
        .incbin LIN_KERNEL
        .global kernel_data_end
kernel_data_end:
        .global initrd_data
initrd_data:
        .incbin LIN_INITRD
        .global initrd_data_end
initrd_data_end:
        .global cmdline_data
cmdline_data:
        .asciz  LIN_CMD
