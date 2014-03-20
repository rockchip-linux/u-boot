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

ENTRY(ResetCpu_asm)
	#LDR 	R2, =0x20020004
	#LDR 	R1, =0x00000003
	STR 	R1, [R2,#0]
    ;//cruReg->CRU_MODE_CON = 0x33330000;    //cpu enter slow mode
	    #LDR		R8, =0x20000040
	    #LDR		R9, =0x33330000
	    STR	R9, [R8,#0]
	
	#LDR 	R1, =0xeca8
	#LDR 	R2, =0x20000104
	#LDR 	R3, =0x10000000
	STR 	R5, [R7,#0]
	STR 	R3, [R4,#0]
ResetCpu_loop:
	 B	ResetCpu_loop
ENDPROC(ResetCpu_asm)

    /*
    * void relocate_code (addr_sp, gd, addr_moni)
    *
    * This "function" does not return, instead it continues in RAM
    * after relocating the monitor code.
    *
    */
ENTRY(ResetCpu)
     #mov r1,#0
stop_loop:
     #cmp r1,#0
     #beq  stop_loop
     mov r7,r0
     mov r6, #0x30  /* save addr of destination */

     adr r0, ResetCpu_asm
     mov r1, r6          /* r1 <- scratch for copy_loop */
     adr r2, ResetCpu    /* r2 <- source end address     */

reset_copy_loop:
     ldmia   r0!, {r9-r10}       /* copy from source address [r0]    */
     stmia   r1!, {r9-r10}       /* copy to   target address [r1]    */
     cmp r0, r2          /* until source end address [r2]    */
     blo reset_copy_loop

     LDR 	R2, =0x20020004
     LDR 	R1, =0x00000003
     #STR 	R1, [R2,#0]
	
     LDR 	R3, =0xeca8
     LDR 	R4, =0x20000104
     LDR 	R5, =0x10000000
     
    ;//cruReg->CRU_MODE_CON = 0x33330000;    //cpu enter slow mode
    LDR		R8, =0x20000040
    LDR		R9, =0x33330000
    #STR	R9, [R8,#0]

     
     
     #STR 	R5, [R7,#0]
     #STR 	R3, [R4,#0]

     bx r6
ENDPROC(ResetCpu)




ENTRY(ResetCpu_3026)
     #mov r1,#0
stop_loop_3026:
     mov r7,r0
     mov r6, #0x30  /* save addr of destination */

     adr r0, ResetCpu_asm
     mov r1, r6          /* r1 <- scratch for copy_loop */
     adr r2, ResetCpu_3026    /* r2 <- source end address     */

reset_copy_loop_3026:
     ldmia   r0!, {r9-r10}       /* copy from source address [r0]    */
     stmia   r1!, {r9-r10}       /* copy to   target address [r1]    */
     cmp r0, r2          /* until source end address [r2]    */
     blo reset_copy_loop_3026

     #LDR 	R2, =0x20020004
     LDR 	R2, =0x20004004
     LDR 	R1, =0x00000003
     #STR 	R1, [R2,#0]
	
     LDR 	R3, =0xeca8
     LDR 	R4, =0x20000104
     LDR 	R5, =0x10000000
     
    ;//cruReg->CRU_MODE_CON = 0x33330000;    //cpu enter slow mode
    LDR		R8, =0x20000040
    LDR		R9, =0x33330000
    #STR	R9, [R8,#0]

     
     
     #STR 	R5, [R7,#0]
     #STR 	R3, [R4,#0]

     bx r6
ENDPROC(ResetCpu_3026)

