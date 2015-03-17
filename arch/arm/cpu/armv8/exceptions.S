/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/ptrace.h>
#include <asm/macro.h>
#include <linux/linkage.h>


/*
 * Enter Exception.
 * This will save the processor state that is ELR/X0~X30
 * to the stack frame.
 */
.macro	exception_entry
	stp	x0, x1, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x6, x7, [sp, #-16]!
	stp	x8, x9, [sp, #-16]!
	stp	x10, x11, [sp, #-16]!
	stp	x12, x13, [sp, #-16]!
	stp	x14, x15, [sp, #-16]!
	stp	x16, x17, [sp, #-16]!
	stp	x18, x30, [sp, #-16]!

	/* Could be running at EL3/EL2/EL1 */
	switch_el x11, 3f, 2f, 1f
3:	mrs	x0, esr_el3
	mrs	x1, elr_el3
	b	0f
2:	mrs	x0, esr_el2
	mrs	x1, elr_el2
	b	0f
1:	mrs	x0, esr_el1
	mrs	x1, elr_el1
0:
	stp	x0, x1, [sp, #-16]!
.endm

/*
 * Exit Exception.
 * This will restore the processor state that is ELR/X0~X30
 * from the stack frame.
 */
.macro	exception_exit
	ldp	x0, x1, [sp], #16

	ldp	x18, x30, [sp], #16
	ldp	x16, x17, [sp], #16
	ldp	x14, x15, [sp], #16
	ldp	x12, x13, [sp], #16
	ldp	x10, x11, [sp], #16

	/* Could be running at EL3/EL2/EL1 */
	switch_el x11, 3f, 2f, 1f
3:	msr	esr_el3, x0
	msr	elr_el3, x1
	b	0f
2:	msr	esr_el2, x0
	msr	elr_el2, x1
	b	0f
1:	msr	esr_el1, x0
	msr	elr_el1, x1
0:

	ldp	x8, x9, [sp], #16
	ldp	x6, x7, [sp], #16
	ldp	x4, x5, [sp], #16
	ldp	x2, x3, [sp], #16
	ldp	x0, x1, [sp], #16

	eret
.endm



/*
 * Exception vectors.
 */
	.align	11
	.globl	vectors
vectors:
	.align	7
	b	_do_bad_sync	/* Current EL Synchronous Thread */

	.align	7
	b	_do_bad_irq	/* Current EL IRQ Thread */

	.align	7
	b	_do_bad_fiq	/* Current EL FIQ Thread */

	.align	7
	b	_do_bad_error	/* Current EL Error Thread */

	.align	7
	b	_do_sync	/* Current EL Synchronous Handler */

	.align	7
	b	_do_irq		/* Current EL IRQ Handler */

	.align	7
	b	_do_fiq		/* Current EL FIQ Handler */

	.align	7
	b	_do_error	/* Current EL Error Handler */


_do_bad_sync:
	exception_entry
	bl	do_bad_sync

_do_bad_irq:
	exception_entry
	bl	do_bad_irq

_do_bad_fiq:
	exception_entry
	bl	do_bad_fiq

_do_bad_error:
	exception_entry
	bl	do_bad_error

_do_sync:
	exception_entry
	bl	do_sync

_do_irq:
	exception_entry
	bl	do_irq
	exception_exit

_do_fiq:
	exception_entry
	bl	do_fiq

_do_error:
	exception_entry
	bl	do_error
