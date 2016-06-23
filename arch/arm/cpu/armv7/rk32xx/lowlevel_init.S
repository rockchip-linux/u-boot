/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <linux/linkage.h>


	.align 5
ENTRY(lowlevel_init)
	/*
	 * Setup a temporary stack
	 */
	ldr	sp, =CONFIG_SYS_INIT_SP_ADDR
	bic	sp, sp, #7 /* 8-byte alignment for ABI compliance */
#ifdef CONFIG_SPL_BUILD
	ldr	r8, =gdata
#else
	sub	sp, #GD_SIZE
	bic	sp, sp, #7
	mov	r8, sp
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
	/*
	 * TTBCR default setting 0.
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c2, c0, 2	@ TTBCR (Translation Table Base Control Register)
#endif

	/*
	 * Save the old lr(passed in ip) and the current lr to stack
	 */
	push	{ip, lr}

	bl	rkclk_set_pll

	pop	{ip, pc}
ENDPROC(lowlevel_init)

#ifndef CONFIG_SYS_L2CACHE_OFF
ENTRY(v7_outer_cache_enable)
	push	{r0, r1, r2, lr}
	mrc	15, 0, r3, cr1, cr0, 1
	orr	r3, r3, #2
	mcr	15, 0, r3, cr1, cr0, 1
	pop	{r1, r2, r3, pc}
ENDPROC(v7_outer_cache_enable)

ENTRY(v7_outer_cache_disable)
	push	{r0, r1, r2, lr}
	mrc	15, 0, r3, cr1, cr0, 1
	bic	r3, r3, #2
	mcr	15, 0, r3, cr1, cr0, 1
	pop	{r1, r2, r3, pc}
ENDPROC(v7_outer_cache_disable)
#endif

