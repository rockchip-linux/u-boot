/*
 * (C) Copyright 2013
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
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
 */

.align 5

#include <linux/linkage.h>

#define SUCCESS		0
#define ERROR		!0

#define id        r0
#define c1f       r0 
#define romSystm  r0
#define ttb       r0
#define processID r0
#define rul       r0

#define domain    r1
#define c1ft      r1
#define c2f       r1
#define c13f      r1

#define c3f       r2

#define mask      r3


/*
 * dcache_line_size - get the minimum D-cache line size from the CTR register
 * on ARMv7.
 */
	.macro	dcache_line_size, reg, tmp
	mrc	p15, 0, \tmp, c0, c0, 1		@ read ctr
	lsr	\tmp, \tmp, #16
	and	\tmp, \tmp, #0xf		@ cache line size encoding
	mov	\reg, #4			@ bytes per word
	mov	\reg, \reg, lsl \tmp		@ actual cache line size
	.endm

/*
 * icache_line_size - get the minimum I-cache line size from the CTR register
 * on ARMv7.
 */
	.macro	icache_line_size, reg, tmp
	mrc	p15, 0, \tmp, c0, c0, 1		@ read ctr
	and	\tmp, \tmp, #0xf		@ cache line size encoding
	mov	\reg, #4			@ bytes per word
	mov	\reg, \reg, lsl \tmp		@ actual cache line size
	.endm



/*
 *	v7_flush_icache_all()
 *
 *	Flush the whole I-cache.
 *
 *	Registers:
 *	r0 - set to 0
 */
ENTRY(v7_flush_icache_all)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c1, 0		@ invalidate I-cache inner shareable
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
	mov	pc, lr
ENDPROC(v7_flush_icache_all)


/*
 *	v7_flush_dcache_all()
 *
 *	Flush the whole D-cache.
 *
 *	Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
 *
 *	- mm    - mm_struct describing address space
 */
ENTRY(v7_flush_dcache_all)
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
ENDPROC(v7_flush_dcache_all)



/*
 *	v7_flush_dcache_area(void *addr, size_t size)
 *
 *	Ensure that the data held in the page kaddr is written back
 *	to the page in question.
 *
 *	- addr	- kernel address
 *	- size	- region size
 */
ENTRY(v7_flush_dcache_area)
	dcache_line_size r2, r3
	add	r1, r0, r1
	sub	r3, r2, #1
	bic	r0, r0, r3
1:
	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D line / unified line
	add	r0, r0, r2
	cmp	r0, r1
	blo	1b
	dsb
	mov	pc, lr
ENDPROC(v7_flush_dcache_area)


ENTRY(MMUEnable)
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00000001
        ORR     c1f, c1f, #0x00000800
        MCR     p15, 0, c1f, c1, c0, 0
        MOV     pc, lr
ENDPROC(MMUEnable)

ENTRY(MMUDisable)
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00000001  
        BIC     c1f, c1f, #0x00000800
        MCR     p15, 0, c1f, c1, c0, 0 
        MOV     pc, lr
ENDPROC(MMUDisable)

ENTRY(MMUProtection)
        MRC     p15, 0, c1ft, c1, c0, 0
        AND     romSystm, romSystm, #0x00000003
        BIC     c1ft, c1ft, #0x00000300
        MOV     romSystm, romSystm, LSL #8
        ORR     c1ft, c1ft, romSystm       
        MCR     p15, 0, c1ft, c1, c0, 0
        MOV     pc, lr
ENDPROC(MMUProtection)

ENTRY(MMUSetDomain)
        CMP     id, #15
        MOVHI   rul, #ERROR      
        BHI     ret1
        MOV     id, id, LSL #1   
        MOV     mask, #0x00000003
        AND     domain, domain, #0x00000003                        
        MOV     mask, mask, LSL id          
        MOV     domain, domain, LSL id                
        MRC     p15, 0, c3f, c3, c0, 0
        MVN     mask, mask
        AND     c3f, c3f, mask
        ORR     c3f, c3f, domain       
        MCR     p15, 0, c3f, c3, c0, 0
        MOVHI   rul, #SUCCESS
ret1:      
        MOV     pc, lr
ENDPROC(MMUSetDomain)

ENTRY(MMUSetTTB)
        MOV     c2f, ttb, LSR #14 
        MOV     c2f, c2f, LSL #14       
        MCR     p15, 0, c2f, c2, c0, 0 
        MOV     pc, lr 
ENDPROC(MMUSetTTB)

ENTRY(MMUSetProcessID)
        CMP     processID, #127
        MOVHI   rul, #ERROR
        BHI     ret2
        MOV     c13f, processID, LSL #25       
        MCR     p15, 0, c13f, c13, c0, 0 
        MOV     rul, #SUCCESS 
ret2:                 
        MOV     pc, lr
ENDPROC(MMUSetProcessID)

ENTRY(CacheRoundRobinReplace)
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00004000 
        MCR     p15, 0, c1f, c1, c0, 0
        MOV     pc, lr
ENDPROC(CacheRoundRobinReplace)

ENTRY(CacheRandomReplace)
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00004000     
        MCR     p15, 0, c1f, c1, c0, 0
        MOV     pc, lr
ENDPROC(CacheRandomReplace)

ENTRY(CacheEnableBoth)
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00001000        
        ORR     c1f, c1f, #0x00000004
        MCR     p15, 0, c1f, c1, c0, 0  
        MOV     pc, lr          
ENDPROC(CacheEnableBoth)

ENTRY(CacheDisableBoth)
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00001000   
        BIC     c1f, c1f, #0x00000004   
        MCR     p15, 0, c1f, c1, c0, 0     
        MOV     pc, lr                                
ENDPROC(CacheDisableBoth)

ENTRY(CacheEnableD)
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00000004        
        MCR     p15, 0, c1f, c1, c0, 0      
        MOV     pc, lr          
ENDPROC(CacheEnableD)

ENTRY(CacheDisableD)
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00000004   
        MCR     p15, 0, c1f, c1, c0, 0     
        MOV     pc, lr          


ENTRY(CacheEnableI)
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00001000        
        MCR     p15, 0, c1f, c1, c0, 0
        MOV     pc, lr          
ENDPROC(CacheEnableI)

ENTRY(CacheDisableI)
        MRC     p15, 0, c1f, c1, c0, 0       
        BIC     c1f, c1f, #0x00001000   
        MCR     p15, 0, c1f, c1, c0, 0             
        MOV     pc, lr     
ENDPROC(CacheDisableI)

ENTRY(WriteBufferDrain)
        MOV     c1f, #0x00000000     
        MCR     p15, 0, c1f, c7, c10, 4       
        MOV     pc, lr     
ENDPROC(WriteBufferDrain)


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

