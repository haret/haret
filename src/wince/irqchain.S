@ Assembler code to monitor WinCE interrupts from HaRET.
@
@ (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
@
@ This file may be distributed under the terms of the GNU GPL license.

        .section .text.irq

@ This handler is copied to a malloc'd memory area in the irq.c C
@ code.  The following variables are designed to overlap with the
@ struct defined in the C code.
        .global asmIrqVars
asmIrqVars:
dataMVA:                .long 0
winceIrqHandler:        .long 0
winceAbortHandler:      .long 0
wincePrefetchHandler:   .long 0
        .global irq_chained_handler
irq_chained_handler:
	@ Store registers on WINCE stack.
        stmdb   sp!, {r0-r12, r14}

        @ Call into C code (sp=stack, r0=data, r1=saved regs)
        mov     r1, sp
        mov     r4, sp
        ldr     r0, [pc, #(dataMVA - . - 8)]
        mov     sp, r0
        bl      irq_handler

        @ Restore registers from WinCE stack and call WinCE handler.
        mov     sp, r4
        ldmia   sp!, {r0-r12, r14}
        ldr     pc, [pc, #(winceIrqHandler - . - 8)]

        .global abort_chained_handler
abort_chained_handler:
	@ Store registers on WINCE stack.
        stmdb   sp!, {r0-r12, r14}

        @ Call into C code (sp=stack, r0=data, r1=saved regs)
        mov     r1, sp
        mov     r4, sp
        ldr     r0, [pc, #(dataMVA - . - 8)]
        sub     sp, r0, #4096
        bl      abort_handler

        @ Restore registers from WinCE stack.
        teq     r0, #0
        mov     sp, r4
        ldmia   sp!, {r0-r12, r14}

        @ Return to user code or call wince handler depending if r0 was 0
        ldreq   pc, [pc, #(winceAbortHandler - . - 8)]
        subs    pc, r14, #4

        .global prefetch_chained_handler
prefetch_chained_handler:
	@ Store registers on WINCE stack.
        stmdb   sp!, {r0-r12, r14}

        @ Call into C code (sp=stack, r0=data, r1=saved regs)
        mov     r1, sp
        mov     r4, sp
        ldr     r0, [pc, #(dataMVA - . - 8)]
        sub     sp, r0, #(2 * 4096)
        bl      prefetch_handler

        @ Restore registers from WinCE stack.
        teq     r0, #0
        mov     sp, r4
        ldmia   sp!, {r0-r12, r14}

        @ Return to user code or call wince handler depending if r0 was 0
        ldreq   pc, [pc, #(wincePrefetchHandler - . - 8)]
        subs    pc, r14, #4

        .end
