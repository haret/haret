@ Assembler code to monitor WinCE interrupts from HaRET.
@
@ (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
@
@ This file may be distributed under the terms of the GNU GPL license.

        .text

@ This handler is copied to a malloc'd memory area in the irq.c C
@ code.  The following variables are designed to overlap with the
@ struct defined in the C code.
dataMVA:                .long 0
cIrqCodeMVA:            .long 0
cAbortCodeMVA:          .long 0
cPrefetchCodeMVA:       .long 0
winceIrqHandler:        .long 0
winceAbortHandler:      .long 0
wincePrefetchHandler:   .long 0
        .globl irq_chained_handler
irq_chained_handler:
	@ Store registers on WINCE stack.
        stmdb   sp!, {r0-r12, r14}

        @ Setup params to C code (sp=stack, r0=data, r1=saved regs)
        mov     r1, sp
        mov     r4, sp
        ldr     r0, [pc, #(dataMVA - . - 8)]
        mov     sp, r0

        @ Call C code
        ldr     r3, [pc, #(cIrqCodeMVA - . - 8)]
        blx     r3

        @ Restore registers from WinCE stack and call WinCE handler.
        mov     sp, r4
        ldmia   sp!, {r0-r12, r14}
        ldr     pc, [pc, #(winceIrqHandler - . - 8)]

        .globl abort_chained_handler
abort_chained_handler:
        @ Store r14 on wince stack
        stmdb   sp!, {r14}

        @ Check if this is a debug event.
        mrc     p15, 0, r14, c5, c0, 0  @ FSR register
        ands    r14, r14, #(1<<9)       @ Debug event flag
        bne     1f

        @ Not a debug event; restore r14 and call wince handler
        ldmia   sp!, {r14}
        ldr     pc, [pc, #(winceAbortHandler - . - 8)]

1:      @ This is a debug event.  Call into C code.

        @ Place remaining registers on wince stack
        stmdb   sp!, {r0-r12}

        @ Setup params to C code (sp=stack, r0=data, r1=saved regs)
        mov     r1, sp
        mov     r4, sp
        ldr     r0, [pc, #(dataMVA - . - 8)]
        sub     sp, r0, #4096

        @ Call C code
        ldr     r3, [pc, #(cAbortCodeMVA - . - 8)]
        blx     r3

        @ Return to user code
        mov     sp, r4
        ldmia   sp!, {r0-r12, r14}
        subs    pc, lr, #4

        .globl prefetch_chained_handler
prefetch_chained_handler:
        @ Store r14 on wince stack
        stmdb   sp!, {r14}

        @ Check if this is a debug event.
        mrc     p15, 0, r14, c5, c0, 0  @ FSR register
        ands    r14, r14, #(1<<9)       @ Debug event flag
        bne     1f

        @ Not a debug event; restore r14 and call wince handler
        ldmia   sp!, {r14}
        ldr     pc, [pc, #(wincePrefetchHandler - . - 8)]

1:      @ This is a debug event.  Call into C code.

        @ Place remaining registers on wince stack
        stmdb   sp!, {r0-r12}

        @ Setup params to C code (sp=stack, r0=data, r1=saved regs)
        mov     r1, sp
        mov     r4, sp
        ldr     r0, [pc, #(dataMVA - . - 8)]
        sub     sp, r0, #(2 * 4096)

        @ Call C code
        ldr     r3, [pc, #(cPrefetchCodeMVA - . - 8)]
        blx     r3

        @ Return to user code
        mov     sp, r4
        ldmia   sp!, {r0-r12, r14}
        subs    pc, lr, #4

        .globl end_chained_handlers
end_chained_handlers:
