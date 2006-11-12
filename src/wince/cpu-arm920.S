;
;   Linux loader for Windows CE
;	file added by Ben Dooks
;
;   For conditions of use see file COPYING
;
;   $Id: cpu-arm920.S,v 1.1 2006/11/12 02:20:49 koconnor Exp $

;; taken from linux/arch/arm/mm/proc-arm920.S: MMU functions for ARM920
;; 
;;   Copyright (C) 1999,2000 ARM Limited
;;   Copyright (C) 2000 Deep Blue Solutions Ltd.

		
		area	|.text|, CODE
	
; Flush CPU caches

		export	|arm920_flush_cache|

|arm920_flush_cache|	proc
; DSEGMENTS 8
; DENTRIES 64
		mov	r0, #0
		mov	r1, #(8-1) << 5			; 8 segments
|fls1|		orr	r3, r1,#(64-1)<<26		; 64 entries
|fls2|		mcr	p15, 0, r3, c7, c14, 2	; clean+invalidate D index
		subs	r3, r3, #1<<26			;
		bcs	fls2				;
		subs	r1, r1, #1<<5			;
		bcs	fls1				;

		mcr	p15, 0, r0, c7, c5, 0		; invalidate I cache
		mcr	p15, 0, r0, c7, c10, 4		; drain WB
		mcr	p15, 0, r0, c8, c7, 0		; drain I+D TLBs
		mov	pc, r14

		end
