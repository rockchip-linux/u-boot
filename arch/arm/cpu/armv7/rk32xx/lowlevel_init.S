/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/

#include <config.h>
#include <linux/linkage.h>


	.align 5
ENTRY(lowlevel_init)
	/*
	 * Setup a temporary stack
	 */
	ldr	sp, =CONFIG_SYS_INIT_SP_ADDR
	bic	sp, sp, #7 /* 8-byte alignment for ABI compliance */

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

