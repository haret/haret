;
;   Linux loader for Windows CE
;   Copyright (C) 2003 Andrew Zabolotny
;
;   For conditions of use see file COPYING
;

		area	|.text|, CODE

; This is used as a chained handler which intercepts all IRQs.
; It is copied to some location which has the same address across
; all processes, to avoid passing control to an invalid memory
; location when a different process is active.
		export	irq_chained_handler
irq_chained_handler proc
		stmdb	r13!, {r0-r3, r12, pc}
		adr	r0, old_irq_handler
		ldr	r0, [r0]		; Replace PC on stack with the
		str	r0, [r13,#20]		; address of the old handler
		ldmia	r13!, {r0-r3, r12, pc}
		export	old_irq_handler
old_irq_handler dcd	0
		export	irq_count
irq_count	dcd	0,0,0,0,0,0,0,0
		export	gpio_irq_count
gpio_irq_count	dcd	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		export	end_irq_chained_handler
|end_irq_chained_handler|
		endp

end
