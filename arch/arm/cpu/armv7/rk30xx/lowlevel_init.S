/*
 * Board specific setup info
 *
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 *
 * Initial Code by:
 * Richard Woodruff <r-woodruff2@ti.com>
 * Syed Mohammed Khasim <khasim@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *there some problem in cache behavior , will fix it in the future
 *
 *please, don't modify this file
 */
#include <config.h>


#define IO_PHY_START				0x10000000
#define IO_PHY_END					0x20040000					

#define RAM_MMU_FLAGS				0xc0a
#define IO_MMU_FLAGS				0xc12

#define TTB_FLAGS	0x9
#define CR_I	(1 << 12)	/* Icache enable			*/
#define CR_C	(1 << 2)	/* Dcache enable			*/
#define CR_A	(1 << 1)	/* Alignment abort enable		*/


	.macro	crval, clear, mmuset, ucset
	.word	\clear
	.word	\mmuset
	.endm

	.macro	pgtbl, rd
	ldr	\rd, =(CONFIG_RK29SDK_TLB - 0x4000)
	.endm

	.macro	dcache_line_size, reg, tmp
	mrc	p15, 1, \tmp, c0, c0, 0		@ read CSIDR
	and	\tmp, \tmp, #7			@ cache line size encoding
	mov	\reg, #16			@ size offset
	mov	\reg, \reg, lsl \tmp		@ actual cache line size
	.endm

.global v7_dma_flush_range
v7_dma_flush_range:
	dcache_line_size r2, r3
	sub	r3, r2, #1				@r2 = cache line size, r3 = cache line size - 1
	add  r1, r0, r1					@r1  change from size to end address
	add  r1, r1, r2					@ r1 = (r1 + cache line size)
	bic   r1, r1, r3					@align end address to cache line size
	bic	r0, r0, r3					@align start address to cache line size
	
1:
	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D / U line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb
	mov	pc, lr

.global v7_flush_dcache_all
v7_flush_dcache_all:
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	ands	r3, r0, #0x7000000		@ extract loc from clidr
	mov	r3, r3, lsr #23			@ left align loc bit field
	beq	finished			@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
loop1:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	skip				@ skip if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
loop2:
	mov	r9, r4				@ create working copy of max way size
loop3:
	orr	r11, r10, r9, lsl r5		@ factor way and cache number into r11
 	orr	r11, r11, r7, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c14, 2		@ clean & invalidate by set/way
	subs	r9, r9, #1			@ decrement the way
	bge	loop3
	subs	r7, r7, #1			@ decrement the index
	bge	loop2
skip:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	loop1
finished:
	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb
	isb
	mov	pc, lr
	
.global v7_flush_dcache_all_rutine
v7_flush_dcache_all_rutine:
	push {r0-r12, r14}
	bl	v7_flush_dcache_all
	pop {r0-r12, pc}
	
__v7_setup:

	adr	r13, __v7_setup_stack		@ the local stack
	stmia	r13, {r0-r5, r7, r9, r11, lr}
	bl	v7_flush_dcache_all
	ldmia	r13, {r0-r5, r7, r9, r11, lr}

	mrc	p15, 0, r0, c0, c0, 0		@ read main ID register
	and	r10, r0, #0xff000000		@ ARM?
	teq	r10, #0x41000000
	bne	2f
	and	r5, r0, #0x00f00000		@ variant
	and	r6, r0, #0x0000000f		@ revision
	orr	r0, r6, r5, lsr #20-4		@ combine variant and revision

2:	mov	r10, #0
	mcr	p15, 0, r10, c7, c5, 0		
	dsb
	mcr	p15, 0, r10, c8, c7, 0		
	mcr	p15, 0, r10, c2, c0, 2		
	orr	r4, r4, #TTB_FLAGS
	mcr	p15, 0, r4, c2, c0, 1		
	mov	r10, #0xf			
	mcr	p15, 0, r10, c3, c0, 0		
	mrc p15, 0, r10, c1, c0, 0
	
	bic	r0,r0,#1<<28

	adr	r5, v7_crval
	ldmia	r5, {r5, r6}
   	mrc	p15, 0, r0, c1, c0, 0		
	bic	r0, r0, r5			
	orr	r0, r0, r6			
	mov	pc, lr				

	.type	v7_crval, #object
v7_crval:
	crval	clear=0x1120E302, mmuset=0xC01C7D, ucset=0x00c01c7c

__v7_setup_stack:
	.space	4 * 11				@ 11 registers

#if 1
__create_page_tables:
		pgtbl	r4
		mov	r0, r4
		mov	r3, #0
		add	r6, r0, #0x4000
1:		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
		str	r3, [r0], #4
		teq	r0, r6
		bne	1b

		ldr	r7, =RAM_MMU_FLAGS
		mov r6, #0x0
		orr r3, r7, r6
		str r3, [r4]
		
		ldr	r6, =RAM_PHY_START
		mov	r0, #0x00100000
		
2:		mov	r6, r6, lsr #20			
		orr	r3, r7, r6, lsl #20		
		str	r3, [r4, r6, lsl #2]		

		add r6, r0, r6, lsl #20
		cmp r6, #RAM_PHY_END

		bls 2b

		ldr r6, =IO_PHY_START
		ldr r7, =IO_MMU_FLAGS
		ldr	r1, =0x200a0000
		
3:		mov r6, r6, lsr #20
		orr	r3, r7, r6, lsl #20
		str	r3, [r4, r6, lsl #2]
		
		add r6, r0, r6, lsl #20
		cmp r6, r1
		bls 3b

		b	__v7_setup

#endif

	
	.align	5
__turn_mmu_on:
	mov	r0, r0
	mcr	p15, 0, r0, c1, c0, 0		@ write control reg
	mrc	p15, 0, r3, c0, c0, 0		@ read id reg
	mov	r3, r3
	mov	r3, r13
	mov	pc, lr


__enable_mmu:
	orr	r0, r0, #CR_A

	mov	r5, #0xf
	mcr	p15, 0, r5, c3, c0, 0		@ load domain access register
	mcr	p15, 0, r4, c2, c0, 0		@ load page table pointer

	b	__turn_mmu_on

 
.globl lowlevel_init
lowlevel_init:
	ldr		r0, =lowlevel_lr_spc
	str		lr, [r0]
	bl		__create_page_tables
	bl		__enable_mmu
	ldr		r0, =lowlevel_lr_spc
	ldr		pc, [r0]
lowlevel_lr_spc:
	.word	0x00000000
	
