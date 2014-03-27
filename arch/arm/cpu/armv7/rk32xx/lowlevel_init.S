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

#include <linux/linkage.h>

	.align 5
ENTRY(lowlevel_init)
	/*
	 * cpu 1 2 3 power down
	 */
	ldr 	r1, =0xFF730000
	mov 	r0, #0x0E
	str r0, [r1,#8]
	
	/*
	 * disable remap
	 */
	LDR R0,=0xff740000
	LDR R1,=0x08000000
	STR R1,[R0]

	/*
	 * Invalidate L2 Latency
	 */
	mrc p15, 1, r0, c9, c0, 2	@ Read L2 Control Register
	bic r0, r0, #0x000000FF @ clear bit 17:0
	bic r0, r0, #0x0000FF00 @ clear bit 17:0
	bic r0, r0, #0x00030000 @ clear bit 17:0
	orr r0, r0, #0x00000008 @ set bit 3
	mcr p15, 1, r0, c9, c0, 2	@ config L2 Control Register

	/*
	 * Setup a temporary stack
	 */
	ldr	sp, =CONFIG_SYS_INIT_SP_ADDR
	bic	sp, sp, #7 /* 8-byte alignment for ABI compliance */

	/*
	 * Save the old lr(passed in ip) and the current lr to stack
	 */
	push	{ip, lr}

	//bl	rkclk_set_pll

	pop	{ip, pc}
ENDPROC(lowlevel_init)


