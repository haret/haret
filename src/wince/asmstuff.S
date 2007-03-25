@
@   Linux loader for Windows CE
@   Copyright (C) 2003 Andrew Zabolotny
@
@   For conditions of use see file COPYING
@

		.macro	CPWAIT
		mrc	p15, 0, r0, c2, c0, 0
		mov	r0, r0
		sub 	pc, pc, #4
		.endm

		.bss
		.space  32
junkdata:	.space 0x10000		@ Spare space for data cache flush

		.text

@ Flush CPU caches - this MUST be called between
@ take_control()/return_control()
@
@ Dirties r0, r1, r2
		.global	cpuFlushCache
cpuFlushCache:
		ldr	r0, =junkdata
		add	r1, r0, #0x10000
	1:	ldr	r2, [r0], #32
		teq	r0, r1
		bne	1b

		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	@ Drain write buffer
		mcr	p15, 0, r0, c8, c7, 0	@ Invalidate I+D TLB
		mcr	p15, 0, r0, c7, c7, 0	@ Invalidate I+D caches & BTB
		CPWAIT

		mov	pc, lr

@ Flush CPU caches on arm920 - this MUST be called between
@ take_control()/return_control()
@
@ Dirties r0, r1, r2
		.global	cpuFlushCache_arm920
cpuFlushCache_arm920:
		mov	r0, #0
		mov	r1, #(8-1) << 5			@ 8 segments
1:		orr	r2, r1,#(64-1)<<26		@ 64 entries
2:		mcr	p15, 0, r2, c7, c14, 2	@ clean+invalidate D index
		subs	r2, r2, #1<<26			@
		bcs	2b				@
		subs	r1, r1, #1<<5			@
		bcs	1b				@

		mcr	p15, 0, r0, c7, c5, 0		@ invalidate I cache
		mcr	p15, 0, r0, c7, c10, 4		@ drain WB
		mcr	p15, 0, r0, c8, c7, 0		@ drain I+D TLBs
		mov	pc, r14

@ Assembler stub that (when relocated) can store the location of a
@ stack and C executable function to jump to.
        .section .text.preload
        .global stackJumper
stackJumper:
stack:          .long 0
data:           .long 0
execCode:       .long 0
asm_handler:
        ldr     r0, [pc, #(data - . - 8)]
        ldr     sp, [pc, #(stack - . - 8)]
        ldr     pc, [pc, #(execCode - . - 8)]

@ Code that can disable the MMU and jump to a function.
@ In:   r0 = Physical address of this function
@       r1 = Virtual address of MMU in non-cached ram.
@       r2 = Physical address of code to jump to
@       r3 = cpu flush cache function
        .section .text.trampoline
        .global mmu_trampoline, mmu_trampoline_end
mmu_trampoline:
	@ Store previous registers on stack.
        stmdb   sp, {r4-r14}

        @ Disable I and D caches
        mrc	p15, 0, r7, c1, c0, 0
        bic	r7, r7, #4		@ Dcache off
        bic	r7, r7, #0x1000         @ Icache off
        mcr	p15, 0, r7, c1, c0, 0

        @ Write back all cpu caches
        mov     r4, r0
        mov     r5, r1
        mov     r6, r2
        mov     lr, pc
        mov     pc, r3

        @ Turn off mapping for this code
        mov     r1, r4, lsr #20
        mov     r0, r1, asl #20         @ r0 = funcaddr & 0xfff00000
        orr     r0, r0, #0x00000C00
        orr     r0, r0, #0x00000002     @ r0 |= 0xC02
        str     r0, [r5, r1, asl #2]    @ mmu[funcaddr>>20] = r0

        @ Clear any TLBs
        mov	r0, #0
        mcr	p15, 0, r0, c8, c7, 0	@ Invalidate I+D TLB
        CPWAIT

        @ Jump to code with shared virtual/physical mapping
        add     pc, r4, #(1f - mmu_trampoline)

1:      @ Code is running at a vm address that is the same as its
        @ physical address.  Now disable the MMU.
        bic	r7, r7, #1              @ MMU off
        mcr	p15, 0, r7, c1, c0, 0

        mcr	p15, 0, r0, c13, c0, 0	@ clear PID
        mcr	p15, 0, r0, c8, c7, 0	@ invalidate I+D TLB
        CPWAIT

        @ MMU is now disabled.  Call function.
        mov     pc, r6

mmu_trampoline_end:

        .end
