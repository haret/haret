        .section .text
        .globl  kernel_data
kernel_data:
        .incbin LIN_KERNEL
        .globl  kernel_data_end
kernel_data_end:
        .globl  initrd_data
initrd_data:
        .incbin LIN_INITRD
        .globl  initrd_data_end
initrd_data_end:
        .globl  cmdline_data
cmdline_data:
        .asciz  LIN_CMD
