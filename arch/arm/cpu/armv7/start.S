/*
 * armboot - Startup Code for OMAP3530/ARM Cortex CPU-core
 *
 * Copyright (c) 2004	Texas Instruments <r-woodruff2@ti.com>
 *
 * Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 * Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 * Copyright (c) 2002	Gary Jennejohn <garyj@denx.de>
 * Copyright (c) 2003	Richard Woodruff <r-woodruff2@ti.com>
 * Copyright (c) 2003	Kshitij <kshitij@ti.com>
 * Copyright (c) 2006-2008 Syed Mohammed Khasim <x0khasim@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <version.h>
#include <asm/system.h>
#include <linux/linkage.h>


#ifdef CONFIG_ROCKCHIP
.globl _start
_start: msr cpsr_c,#0xd3 
        b _vector
.globl RSA_KEY_TAG
RSA_KEY_TAG:
		.word 0x4B415352
.globl RSA_KEY_LENGTH
RSA_KEY_LENGTH:
		.word 0x200-0x08
.globl RSA_KEY_DATA
RSA_KEY_DATA:
		.space 0x200-0x08
.globl loader_tag_check
loader_tag_check:
		.word 0x4B415351
.globl RSA_PAD
RSA_PAD:
		.space 20

	.align	5
.globl _vector
_vector: b	reset
#else
.globl _start
_start: b	reset
#endif
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq
	ldr	pc, _fiq
#ifdef CONFIG_SPL_BUILD
_undefined_instruction: .word _undefined_instruction
_software_interrupt:	.word _software_interrupt
_prefetch_abort:	.word _prefetch_abort
_data_abort:		.word _data_abort
_not_used:		.word _not_used
_irq:			.word _irq
_fiq:			.word _fiq
_pad:			.word 0x12345678 /* now 16*4=64 */
#else
_undefined_instruction: .word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq
_pad:			.word 0x12345678 /* now 16*4=64 */
#endif	/* CONFIG_SPL_BUILD */

.global _end_vect
_end_vect:

	.balignl 16,0xdeadbeef
/*************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * setup Memory and board specific bits prior to relocation.
 * relocate armboot to ram
 * setup stack
 *
 *************************************************************************/

.globl _TEXT_BASE
_TEXT_BASE:
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_TEXT_BASE)
	.word	CONFIG_SPL_TEXT_BASE
#else
	.word	CONFIG_SYS_TEXT_BASE
#endif

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start_ofs
_bss_start_ofs:
	.word __bss_start - _start

.globl _bss_end_ofs
_bss_end_ofs:
	.word __bss_end - _start

.globl _end_ofs
_end_ofs:
	.word _end - _start

#ifdef CONFIG_USE_IRQ
/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word 0x0badc0de
#endif

/* IRQ stack memory (calculated at run-time) + 8 bytes */
.globl IRQ_STACK_START_IN
IRQ_STACK_START_IN:
	.word	0x0badc0de

/*
 * the actual reset code
 */

reset:
	bl	save_boot_params
	/*
	 * disable interrupts (FIQ and IRQ), also set the cpu to SVC32 mode,
	 * except if in HYP mode already
	 */
	mrs	r0, cpsr
	and	r1, r0, #0x1f		@ mask mode bits
	teq	r1, #0x1a		@ test for HYP mode
	bicne	r0, r0, #0x1f		@ clear all mode bits
	orrne	r0, r0, #0x13		@ set SVC mode
	orr	r0, r0, #0xc0		@ disable FIQ and IRQ
	msr	cpsr,r0

#ifdef CONFIG_ROCKCHIP
        ldr 	r1, =0x20004000             /* cpu 1 2 3 power down */
        mov 	r0, #0x0E
        str	r0, [r1,#8]
#endif

#ifdef CONFIG_USE_RK30IRQ
	/*;chip detect RK30 or RK3066B
	;set remap bit in grf enabled, remap 0x0000 to imem, 
	;*(unsigned long volatile *)(GRF_BASE + GRF_SOC_CON0) 0x20008150  = 0x10001000; 30
	;*(unsigned long volatile *)(GRF_BASE + GRF_SOC_CON0) 0x200080A0  = 0x10001000; 3066B 3168 3188
	*/
	
	LDR R0,=0x101027FC
	LDR R2,=0x56313031
	LDR R1,[R0]
	CMP R1,R2
	LDREQ R0,=0x20008150 ;//rk30
	LDRNE R0,=0x200080A0 ;//rk3066B 3168 3188
	LDR R1,=0x10001000
	STR R1,[R0]
#if 0

	adr r0, _vector
	adr r1, _end_vect
	
	sub r2, r1, r0
	mov r1, #0
1:	ldr r3, [r0], #4
	str r3, [r1], #4 
	cmp r1, r2
	bne 1b
#endif
#endif

/*
 * Setup vector:
 * (OMAP4 spl TEXT_BASE is not 32 byte aligned.
 * Continue to use ROM code vector only in OMAP4 spl)
 */
#if !(defined(CONFIG_OMAP44XX) && defined(CONFIG_SPL_BUILD))
	/* Set V=0 in CP15 SCTRL register - for VBAR to point to vector */
	mrc	p15, 0, r0, c1, c0, 0	@ Read CP15 SCTRL Register
	bic	r0, #CR_V		@ V = 0
	mcr	p15, 0, r0, c1, c0, 0	@ Write CP15 SCTRL Register

	/* Set vector address in CP15 VBAR register */
	ldr	r0, =_vector
	mcr	p15, 0, r0, c12, c0, 0	@Set VBAR
#endif

	/* the mask ROM code should have PLL and others stable */
#ifndef CONFIG_SKIP_LOWLEVEL_INIT
	bl	cpu_init_cp15
	bl	cpu_init_crit
#endif

	bl	_main

/*------------------------------------------------------------------------------*/

ENTRY(c_runtime_cpu_setup)
/*
 * If I-cache is enabled invalidate it
 */
#ifndef CONFIG_SYS_ICACHE_OFF
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c5, 4	@ ISB
#endif
/*
 * Move vector table
 */
	/* Set vector address in CP15 VBAR register */
	ldr     r0, =_vector
	mcr     p15, 0, r0, c12, c0, 0  @Set VBAR

	bx	lr

ENDPROC(c_runtime_cpu_setup)

/*************************************************************************
 *
 * void save_boot_params(u32 r0, u32 r1, u32 r2, u32 r3)
 *	__attribute__((weak));
 *
 * Stack pointer is not yet initialized at this moment
 * Don't save anything to stack even if compiled with -O0
 *
 *************************************************************************/
ENTRY(save_boot_params)
	bx	lr			@ back to my caller
ENDPROC(save_boot_params)
	.weak	save_boot_params

/*************************************************************************
 *
 * cpu_init_cp15
 *
 * Setup CP15 registers (cache, MMU, TLBs). The I-cache is turned on unless
 * CONFIG_SYS_ICACHE_OFF is defined.
 *
 *************************************************************************/
ENTRY(cpu_init_cp15)
	/*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			@ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate BP array
	mcr     p15, 0, r0, c7, c10, 4	@ DSB
	mcr     p15, 0, r0, c7, c5, 4	@ ISB

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 11 (Z---) BTB
#ifdef CONFIG_SYS_ICACHE_OFF
	bic	r0, r0, #0x00001000	@ clear bit 12 (I) I-cache
#else
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-cache
#endif
	mcr	p15, 0, r0, c1, c0, 0

#ifdef CONFIG_ARM_ERRATA_716044
	mrc	p15, 0, r0, c1, c0, 0	@ read system control register
	orr	r0, r0, #1 << 11	@ set bit #11
	mcr	p15, 0, r0, c1, c0, 0	@ write system control register
#endif

#ifdef CONFIG_ARM_ERRATA_742230
	mrc	p15, 0, r0, c15, c0, 1	@ read diagnostic register
	orr	r0, r0, #1 << 4		@ set bit #4
	mcr	p15, 0, r0, c15, c0, 1	@ write diagnostic register
#endif

#ifdef CONFIG_ARM_ERRATA_743622
	mrc	p15, 0, r0, c15, c0, 1	@ read diagnostic register
	orr	r0, r0, #1 << 6		@ set bit #6
	mcr	p15, 0, r0, c15, c0, 1	@ write diagnostic register
#endif

#ifdef CONFIG_ARM_ERRATA_751472
	mrc	p15, 0, r0, c15, c0, 1	@ read diagnostic register
	orr	r0, r0, #1 << 11	@ set bit #11
	mcr	p15, 0, r0, c15, c0, 1	@ write diagnostic register
#endif

	mov	pc, lr			@ back to my caller
ENDPROC(cpu_init_cp15)

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
/*************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************/
ENTRY(cpu_init_crit)
	/*
	 * Jump to board specific initialization...
	 * The Mask ROM will have already initialized
	 * basic memory. Go here to bump up clock rate and handle
	 * wake up conditions.
	 */
	b	lowlevel_init		@ go setup pll,mux,memory
ENDPROC(cpu_init_crit)
#endif

#ifndef CONFIG_SPL_BUILD
/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */
@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */

	.macro	bad_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE		@ carve out a frame on current
						@ user stack
	stmia	sp, {r0 - r12}			@ Save user registers (now in
						@ svc mode) r0-r12
	ldr	r2, IRQ_STACK_START_IN		@ set base 2 words into abort
						@ stack
	ldmia	r2, {r2 - r3}			@ get values for "aborted" pc
						@ and cpsr (into parm regs)
	add	r0, sp, #S_FRAME_SIZE		@ grab pointer to old stack

	add	r5, sp, #S_SP
	mov	r1, lr
	stmia	r5, {r0 - r3}			@ save sp_SVC, lr_SVC, pc, cpsr
	mov	r0, sp				@ save current stack into r0
						@ (param register)
	.endm

	.macro	irq_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	add	r8, sp, #S_PC			@ !! R8 NEEDS to be saved !!
						@ a reserved stack spot would
						@ be good.
	stmdb	r8, {sp, lr}^			@ Calling SP, LR
	str	lr, [r8, #0]			@ Save calling PC
	mrs	r6, spsr
	str	r6, [r8, #4]			@ Save CPSR
	str	r0, [r8, #8]			@ Save OLD_R0
	mov	r0, sp
	.endm

	.macro	irq_restore_user_regs
	ldmia	sp, {r0 - lr}^			@ Calling r0 - lr
	mov	r0, r0
	ldr	lr, [sp, #S_PC]			@ Get PC
	add	sp, sp, #S_FRAME_SIZE
	subs	pc, lr, #4			@ return & move spsr_svc into
						@ cpsr
	.endm

	.macro get_bad_stack
	ldr	r13, IRQ_STACK_START_IN		@ setup our mode stack (enter
						@ in banked mode)

	str	lr, [r13]			@ save caller lr in position 0
						@ of saved stack
	mrs	lr, spsr			@ get the spsr
	str	lr, [r13, #4]			@ save spsr in position 1 of
						@ saved stack

	mov	r13, #MODE_SVC			@ prepare SVC-Mode
	@ msr	spsr_c, r13
	msr	spsr, r13			@ switch modes, make sure
						@ moves will execute
	mov	lr, pc				@ capture return pc
	movs	pc, lr				@ jump to next instruction &
						@ switch modes.
	.endm

	.macro get_bad_stack_swi
	sub	r13, r13, #4			@ space on current stack for
						@ scratch reg.
	str	r0, [r13]			@ save R0's value.
	ldr	r0, IRQ_STACK_START_IN		@ get data regions start
						@ spots for abort stack
	str	lr, [r0]			@ save caller lr in position 0
						@ of saved stack
	mrs	lr, spsr			@ get the spsr
	str	lr, [r0, #4]			@ save spsr in position 1 of
						@ saved stack
	ldr	lr, [r0]			@ restore lr
	ldr	r0, [r13]			@ restore r0
	add	r13, r13, #4			@ pop stack entry
	.endm

	.macro get_irq_stack			@ setup IRQ stack
	ldr	sp, IRQ_STACK_START
	.endm

	.macro get_fiq_stack			@ setup FIQ stack
	ldr	sp, FIQ_STACK_START
	.endm

/*
 * exception handlers
 */
	.align	5
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl	do_undefined_instruction

	.align	5
software_interrupt:
	get_bad_stack_swi
	bad_save_user_regs
	bl	do_software_interrupt

	.align	5
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_prefetch_abort

	.align	5
data_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_data_abort

	.align	5
not_used:
	get_bad_stack
	bad_save_user_regs
	bl	do_not_used

#ifdef CONFIG_USE_IRQ

	.align	5
irq:
	get_irq_stack
	irq_save_user_regs
	bl	do_irq
	irq_restore_user_regs

	.align	5
fiq:
	get_fiq_stack
	/* someone ought to write a more effective fiq_save_user_regs */
	irq_save_user_regs
	bl	do_fiq
	irq_restore_user_regs

#else

	.align	5
irq:
	get_bad_stack
	bad_save_user_regs
	bl	do_irq

	.align	5
fiq:
	get_bad_stack
	bad_save_user_regs
	bl	do_fiq

#endif /* CONFIG_USE_IRQ */
#endif /* CONFIG_SPL_BUILD */
