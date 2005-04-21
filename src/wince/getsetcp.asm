;
;   Linux loader for Windows CE
;   Copyright (C) 2003 Andrew Zabolotny
;
;   For conditions of use see file COPYING
;

		area	|.text|, CODE

; u32 _cpu_get_cp (u32 cp, u32 reg)
		export _cpu_get_cp
|_cpu_get_cp|	proc
		ldr	r3, =mov_reg_cp
		add	r3, r3,	r0 lsl #7
		add	pc, r3,	r1 lsl #3
		endp

; void _cpu_set_cp (u32 cp, u32 reg, u32 val)
		export _cpu_set_cp
|_cpu_set_cp|	proc
		ldr	r3, =mov_cp_reg
		add	r3, r3,	r0 lsl #7
		add	pc, r3,	r1 lsl #3
		endp

		ltorg

|mov_reg_cp|	mrc	p0, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p0, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p1, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p2, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p3, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p4, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p5, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p6, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p7, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p8, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c0,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c1,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c2,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c3,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c4,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c5,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c6,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c7,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c8,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c9,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c10,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c11,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c12,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c13,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c14,c0, 0
		mov	pc,lr	
		mrc	p9, 0, r0,c15,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c0,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c1,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c2,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c3,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c4,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c5,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c6,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c7,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c8,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c9,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c10,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c11,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c12,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c13,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c14,c0, 0
		mov	pc,lr	
		mrc	p10, 0,	r0,c15,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c0,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c1,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c2,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c3,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c4,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c5,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c6,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c7,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c8,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c9,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c10,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c11,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c12,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c13,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c14,c0, 0
		mov	pc,lr	
		mrc	p11, 0,	r0,c15,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c0,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c1,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c2,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c3,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c4,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c5,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c6,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c7,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c8,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c9,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c10,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c11,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c12,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c13,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c14,c0, 0
		mov	pc,lr	
		mrc	p12, 0,	r0,c15,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c0,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c1,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c2,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c3,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c4,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c5,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c6,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c7,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c8,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c9,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c10,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c11,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c12,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c13,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c14,c0, 0
		mov	pc,lr	
		mrc	p13, 0,	r0,c15,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c0,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c1,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c2,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c3,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c4,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c5,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c6,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c7,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c8,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c9,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c10,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c11,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c12,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c13,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c14,c0, 0
		mov	pc,lr	
		mrc	p14, 0,	r0,c15,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c0,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c1,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c2,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c3,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c4,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c5,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c6,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c7,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c8,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c9,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c10,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c11,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c12,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c13,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c14,c0, 0
		mov	pc,lr	
		mrc	p15, 0,	r0,c15,c0, 0
		mov	pc,lr	

|mov_cp_reg|	mcr	p0, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p0, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p1, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p2, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p3, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p4, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p5, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p6, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p7, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p8, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c0,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c1,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c2,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c3,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c4,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c5,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c6,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c7,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c8,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c9,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c10,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c11,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c12,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c13,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c14,c0, 0
		mov	pc,lr	
		mcr	p9, 0, r2,c15,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c0,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c1,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c2,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c3,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c4,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c5,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c6,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c7,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c8,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c9,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c10,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c11,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c12,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c13,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c14,c0, 0
		mov	pc,lr	
		mcr	p10, 0,	r2,c15,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c0,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c1,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c2,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c3,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c4,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c5,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c6,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c7,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c8,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c9,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c10,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c11,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c12,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c13,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c14,c0, 0
		mov	pc,lr	
		mcr	p11, 0,	r2,c15,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c0,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c1,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c2,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c3,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c4,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c5,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c6,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c7,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c8,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c9,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c10,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c11,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c12,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c13,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c14,c0, 0
		mov	pc,lr	
		mcr	p12, 0,	r2,c15,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c0,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c1,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c2,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c3,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c4,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c5,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c6,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c7,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c8,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c9,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c10,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c11,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c12,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c13,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c14,c0, 0
		mov	pc,lr	
		mcr	p13, 0,	r2,c15,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c0,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c1,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c2,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c3,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c4,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c5,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c6,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c7,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c8,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c9,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c10,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c11,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c12,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c13,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c14,c0, 0
		mov	pc,lr	
		mcr	p14, 0,	r2,c15,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c0,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c1,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c2,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c3,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c4,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c5,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c6,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c7,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c8,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c9,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c10,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c11,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c12,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c13,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c14,c0, 0
		mov	pc,lr	
		mcr	p15, 0,	r2,c15,c0, 0
		mov	pc,lr	

		end
