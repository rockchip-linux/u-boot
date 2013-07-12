;/************************************************************************************************/
;代码加载部分,包括 初始化堆栈，初始化 CPU，加载代码.
;07-09-17,huangsl,每个阶段的代码加载由模块本身来完成，以减少代码交叉情况。
;									增加 KERNEL 各个段的起始和大小，以便 C 代码引用.
;/************************************************************************************************/


	IMPORT		||Load$$CODE1$$Base||
	IMPORT		||Load$$CODE$$Base||
	IMPORT		||Image$$CODE1$$Base||
	IMPORT		||Image$$CODE1$$Limit||
	
	IMPORT		||Load$$CODE2$$Base||
	IMPORT		||Image$$CODE2$$Base||
	IMPORT		||Image$$CODE2$$Limit||

	IMPORT		||Image$$CODE1$$ZI$$Base||
	IMPORT		||Image$$CODE1$$ZI$$Limit||
	
	IMPORT 		||Image$$CODE2$$Length||
	IMPORT		||Image$$CODE2$$ZI$$Base||
	IMPORT		||Image$$CODE2$$ZI$$Limit||
	
	IMPORT		||Load$$LOADERTAG$$Base||
	IMPORT 		gLoaderTag
	
;/************************************************************************************************/
    EXPORT  	RomLoader
    EXPORT      RSA_KEY_TAG
    EXPORT      RSA_KEY_LENGTH
    EXPORT      RSA_KEY_DATA

;/************************************************************************************************/
		CODE32
		AREA   LOADER, CODE, READONLY

RomLoader
        MSR CPSR_c,#0xd3        ; No interrupts,SVC MODE.huangsl,071226
        B runNext
RSA_KEY_TAG
		dcd		 0x4B415352
RSA_KEY_LENGTH
		dcd		 (0x200-0x08)
RSA_KEY_DATA	SPACE	 (0x200-0x08)

LoaderTagCheck
        dcd      0x4B415351
        
runNext
        LDR     R0, =0x10080000 ; /*set cpu freq tag to 24Mhz for ddr init*/
        MOV     R1, 24
        STR     R1,[R0]
        
     	LDR     R0, =||Load$$LOADERTAG$$Base|| ;/*check loader code */
	LDR     R1,[R0]
	LDR     r0,LoaderTagCheck
        CMP     R1,R0
	MOVNE   PC,R14
	
        MOV r0, #0                          ; SBZ
        MCR  p15, 0, r0, c7, c5, 0          ; Invalidate all instruction caches

		; disable mmu first .
		mrc p15 , 0 , r0 , c1 , c0 , 0
        BIC R0 , R0 , #0x00000005   ;C MMU
        BIC R0 , R0 , #0x10000000   ;TRE 
        BIC R0 , R0 , #0x00003800   ;V I Z
		MCR	p15 , 0 , r0 , c1 , c0 , 0

       ; MCR     p15, 0, r0, c8, c7, 0      ; Invalidate entire Unified main TLB (Note: r0 value is ignored)

        ;LDR R1, =0x2004C000                ; WTD 
        ;MOV R0, #0x0A
        ;STR	R0, [R1,#4]
        
        ;MOV R0, #0x76
        ;STR R0, [R1,#0x0C]
        
        ;MOV R0, #0x19
        ;STR R0, [R1,#0x00]

        ;LDR R1, =0x20004000             ;  cpu 1 power down
        ;MOV R0, #0x0E
        ;STR	R0, [R1,#8]
	
		
  	    ;set remap bit in grf enabled, remap 0x0000 to imem, 
	    ;*(unsigned long volatile *)(GRF_BASE + GRF_SOC_CON0) 0x20080140  = 0x10001000;
	    LDR R0,=0x20008140
	    LDR R1,=0x10001000
	    STR R1,[R0]
	
		LDR     R0, =||Load$$CODE1$$Base||			;源
		LDR     R1, =||Image$$CODE1$$Base||			;目标
		LDR     R2, =||Image$$CODE1$$Limit||		;长度
		BL	LoadCodeSection

		LDR     R0, =||Load$$CODE2$$Base||			;源
		LDR     R1, =||Image$$CODE2$$Base||			;目标
		LDR     R2, =||Image$$CODE2$$Limit||		;长度
		BL	LoadCodeSection
	    LDR	pc, =||Image$$CODE1$$Base||   			; Jump to SRAM CODE

;1.源地址，2.目标地址,3.结束地址
LoadCodeSection	PROC
LoopRelCode
		CMP     a2, a3			;a2<a3
		LDRNE   a4, [a1], #4
		STRNE   a4, [a2], #4
		BNE     LoopRelCode 
			
		MOV	PC , LR
		ENDP
	
;1.源地址，2.结束地址
ZeroZISection
		MOV	a3,#0
LoopRelZI
		CMP     a1, a2		;a1<a2
		STRNE   a3, [a1], #4
		BNE     LoopRelZI 
		MOV	PC , LR		

		EXPORT  SET_ROM_SP

; for call to maskrom usb 		
;*((int*)0x18019014) = 0;         un  remap , set 0 at maskrom 
; usb entry: 0x000011f4
SET_ROM_SP
		ldr	r0,=0x80200280
		MSR	CPSR_c,#0xd2
		MOV	r1,#0x200   ; 0x2fc
		ADD     R13,r0,R1
		MSR     CPSR_cxsf,#0xd3
		MOV     r2,#0x600
		ADD     r13,r0,r2
		LDR     R0, =0x18019014
		MOV	r1 , #0
		STR	r1 , [r0]
MemCpy
		MOV	R8, #0x10000000
		MOV	R9, #0x00
		MOV	R10, #256		;copy 256*32B=8192B

CPY_LOOP
		LDMIA	R8!, {R0-R7}
		STMIA	R9!, {R0-R7}
		SUBS	R10, R10, #1
		BNE	CPY_LOOP
		ldr	r1,=0x000011f4
		ldr	r14,=0x000011e0 
		MOV     PC, r1
		     		   	  
    END
;/*********************************************************************************************************
;**                            End Of File
;********************************************************************************************************/
