@
@   Linux loader for Windows CE
@   Copyright (C) 2003 Andrew Zabolotny
@
@   For conditions of use see file COPYING
@

		.bss

junkdata:	.space 0x10000		@ Spare space for data cache flush

		.text

		.macro	CPWAIT
		mrc	p15, 0, r0, c2, c0, 0
		mov	r0, r0
		sub 	pc, pc, #4
		.endm

@ Disable interrupts
		.global	cli
cli:
		mrs	r0, cpsr
		orr	r0, r0, #0xc0
		msr	cpsr_c, r0
		mov	pc, lr

@ Enable interrupts
		.global	sti
sti:
		mrs	r0, cpsr
		bic	r0, r0, #0xc0
		msr	cpsr_c, r0
		mov	pc, lr

@ Get Program Status Register
		.global	cpuGetPSR
cpuGetPSR:
		mrs	r0, cpsr
		mov	pc, lr

@ Get Domain Access Control Register
		.global	cpuGetDACR
cpuGetDACR:
		mrc	p15, 0, r0, c3, c0, 0
		mov	pc, lr

@ Set Domain Access Control Register
		.global	cpuSetDACR
cpuSetDACR:
		mcr	p15, 0, r0, c3, c0, 0
		CPWAIT
		mov	pc, lr

@ this next section is to help remove the amount of
@ self modifying code done by HaRET itself

		.global	cpuGetPid
cpuGetPid:
		mrc	p15, 0, r0, c13, c0, 0
		mov	pc, r14

		.global	cpuGetIDCode
cpuGetIDCode:
		mrc	p15, 0, r0, c0, c0, 0
		mov	pc, r14

		.global cpuGetTTBase
cpuGetTTBase:
		mrc	p15, 0, r0, c2, c0, 0
		mov	pc, r14

@ Flush CPU caches
		.global	cpuFlushCache
cpuFlushCache:
@ Flush both data cache and mini-data cache
		ldr	r0, =junkdata
		add	r1, r0, #0x10000
fc_loop:	ldr	r2, [r0], #32
		teq	r0, r1
		bne	fc_loop

		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	@ Drain write buffer
		mcr	p15, 0, r0, c8, c7, 0	@ Invalidate I+D TLB
		mcr	p15, 0, r0, c7, c7, 0	@ Invalidate I+D caches & BTB
		CPWAIT

		mov	pc, lr

@ Flush caches and jump to the preloader.
@ IN:	r0 = boot machine type
@	r1 = number of pages
@	r2 = physical RAM address
@	r3 = physical address of the preloader
@	r4 = number of pages for tags + kernel
		.global	linux_start
linux_start:
		mov	r9, r0			@ save machine type
		ldr	r4, [sp]		@ get fifth arg from stack

		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	@ Drain write buffer
		mcr	p15, 0, r0, c8, c7, 0	@ Invalidate I+D TLB
		mcr	p15, 0, r0, c7, c7, 0	@ Invalidate I+D caches & BTB
		CPWAIT

		mov	pc, r3			@ jump to linux_preloader

@ Turn off MMU, copy kernel to proper location and jump to kernel loader.
@ This procedure is copied to a virtual address that has a 1:1 mapping with
@ the same physical address, thus we don't have to hold the breath while
@ turning MMU off. The table of physical address of every 4K of kernel 
@ is at linux_preloader + 0x100
@ IN:	r9 = boot machine type
@	r1 = number of pages
@	r2 = physical RAM address 
@	r4 = number of pages for tags + kernel
		.global	linux_preloader
		.global	linux_preloader_end
linux_preloader:
		add	r3, pc, #0xf8		@ r3 = linux_preloader+0x100
		add	r2, r2, #0x100		@ tag list address (0xa0000100)
		mov	r10, r2			@ r10 = tag list address

		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	@ Drain write buffer
		mcr	p15, 0, r0, c7, c7, 0	@ invalidate I+D & BTB

		mrc	p15, 0, r0, c1, c0, 0
		bic	r0, r0, #5		@ MMU & Dcache off
		bic	r0, r0, #0x1000         @ Icache off
		mcr	p15, 0, r0, c1, c0, 0	@ disable the MMU & caches
		CPWAIT

		mov	r0, #0
		mcr	p15, 0, r0, c13, c0, 0	@ clear PID
		mcr	p15, 0, r0, c8, c7, 0	@ invalidate I+D TLB
		CPWAIT

nextpage:	ldr	r7, [r3], #4		@ Get next physical address
		add	r8, r7, #0x1000		@ Copy 4K

copypage:	ldr	r0, [r7], #4
		str	r0, [r2], #4
		teq	r7, r8
		bne	copypage

		tst	r2, #0x00000100		@ tag list should be at 0xa0000100
		addne	r2, r2, #0x6f00		@ but kernel should be at 0xa0008000
		movne	r5, r2			@ save kernel physical address in r5

		subs	r4, r4, #1		@ skip tags and kernel pages
		addeq	r2, r5, #0x00500000	@ place initrd at kernel + 5Mb

		subs	r1, r1, #1
		bne	nextpage

@ turn off LCD controller...
@		mov	r1, #0x44000000
@		ldr	r2, [r1]
@		orr	r2, r2, #0x400
@		str	r2, [r1]

		mov	r0, #0
		mov	r1, r9			@ saved machine type
		mov	r2, r10			@ saved tag list address

@ Jump into the kernel (which is 0x8000-0x0100 past the tags)
		add	pc, r10, #0x7f00
linux_preloader_end:

		.end
