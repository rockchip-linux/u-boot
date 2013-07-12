;=========================================
; NAME: LDK3.1 INIT
; DESC: start up codes
;       Configure memory, ISR ,stacks
;	Initialize C-variables
; Author: huangxinyu
; Date: 2007-04-11
;=========================================
;define the stack size
;定义堆栈的大小
FIQ_STACK_LEGTH         EQU         0				;not use FIQ 
IRQ_STACK_LEGTH         EQU         512             ;every layer need 9 bytes stack , permit 8 layer .每层嵌套需要9个字堆栈，允许8层嵌套
ABT_STACK_LEGTH         EQU         0
UND_STACK_LEGTH         EQU         0

NoInt       EQU 0x80

USR32Mode   EQU 0x10
SVC32Mode   EQU 0x13
SYS32Mode   EQU 0x1f
IRQ32Mode   EQU 0x12
FIQ32Mode   EQU 0x11

PINSEL2     EQU 0xE002C014

BCFG0       EQU 0xFFE00000
BCFG1       EQU 0xFFE00004
BCFG2       EQU 0xFFE00008
BCFG3       EQU 0xFFE0000C

    
;引入的外部标号在这声明	
	IMPORT IrqHandler
    IMPORT  Main                        ;The entry point to the main function C语言主程序入口 
     
;给外部使用的标号在这声明
   
    EXPORT  ResetHandler

	IMPORT		||Image$$CODE$$Base||
	IMPORT		||Image$$CODE$$Limit||
    IMPORT		||Image$$CODE2$$ZI$$Limit||
    
    CODE32

    AREA    vectors,CODE,READONLY
        ENTRY
;中断向量表
Reset
        LDR     PC, ResetAddr
        LDR     PC, UndefinedAddr
        LDR     PC, SWI_Addr
        LDR     PC, PrefetchAddr
        LDR     PC, DataAbortAddr
        DCD     0xb9205f80
        LDR     PC, IRQ_Addr
        LDR     PC, FIQ_Addr
        
ResetAddr           DCD     ResetHandler
UndefinedAddr       DCD     Undefined
SWI_Addr            DCD     SoftwareInterrupt
PrefetchAddr        DCD     PrefetchAbort
DataAbortAddr       DCD     DataAbort
Nouse               DCD     0
IRQ_Addr            DCD     IrqExce
FIQ_Addr            DCD     FIQ_Handler

;未定义指令
Undefined
        B       Undefined

SoftwareInterrupt
        B       SoftwareInterrupt
        
;取指令中止
PrefetchAbort
        SUBS	PC, R14, #4
        ;B       PrefetchAbort

;取数据中止
DataAbort
        SUBS	PC, R14, #4

;快速中断
FIQ_Handler
        B       FIQ_Handler

;IRQ中断
IrqExce
        SUB	LR, LR, #4			;保存实际的返回地址
        STMFD	SP!,	{R0-R12, LR}	;
        
        MRS		R1, SPSR
        STMFD	SP!, {R1}			;保存IRQ 出现前 CPSR 的值到堆栈
        PRESERVE8
        BL IrqHandler
;        LDR     LR, =ret
;        LDR     PC, =IrqHandler
;ret 
        ;BL		IrqHandler
        LDMFD	SP!, {R1}
        MSR		SPSR_cxsf,	R1
        LDMFD	SP!, {R0-R12, PC}^

	
;/*********************************************************************************************************
;** Initialize the stacks  初始化堆栈
;********************************************************************************************************/
;		IMPORT		StackSvc

InitStack    
        MOV     R0, LR
;Build the SVC stack
; 设置中断模式堆栈
        MSR     CPSR_c, #0xd2
        LDR     R1, =||Image$$CODE2$$ZI$$Limit||
        LDR		R2, =0x4000
        ADD		R1, R1, R2
        MOV     SP, R1

;Build the SVC stack
; 设置中断模式堆栈
        MSR     CPSR_c, #0xd7
        LDR		R2, =0x4000
        ADD		R1, R1, R2
        MOV     SP, R1
        
;设置超级用户模式堆栈
        MSR     CPSR_c, #0xd3	;SVC32Mode
        LDR		R2, =0x50000
        ADD		R1, R1, R2
        MOV     SP, R1
        MOV     PC, R0


	EXPORT		BootDisableInt	
BootDisableInt 
				MRS r0, cpsr 				;由于任务和内核都运行在svc模式下，因此可方便地操作cpsr 
				ORR r1, r0, #0xc0 		;屏蔽FIQ，IRQ中断 
				MSR cpsr_c, r1 			;回写cpsr，只屏蔽IRQ中断 
				MOV pc, lr 					;返回 
	
;/********************************************************************************************************
;** RESET  复位入口
;********************************************************************************************************/
ResetHandler
		bl  InitStack               	; Initialize the stack
        
    ldr	pc, initloops     			; Jump to the entry point of C program 跳转到c语言入口
initloops   DCD     Main


	
     END
;/*********************************************************************************************************
;**                            End Of File
;********************************************************************************************************/
