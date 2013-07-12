;/******************************************************************/ 
;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	CacheCleanFlushWhole.s                                               
;Desc   :	cache整体清理和清除操作文件                                               
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log: CacheCleanFlushWhole.s,v $
;Revision 1.2  2011/01/26 09:36:10  Administrator
;*** empty log message ***
;
;Revision 1.1  2010/12/06 02:43:49  Administrator
;*** empty log message ***
;
;Revision 1.00  2008/11/11 	nizy                            
;********************************************************************/ 
        CODE32

        AREA cacheWholeArea, CODE, READONLY 
;------------------------------------------------------------------------
; /* Ensure that it can't be interrupted when performing clean or flush */
; C-prototypes
; void  CacheFlushBoth(void);  
;------------------------------------------------------------------------        
          
        INCLUDE CacheMMU.h

        EXPORT CacheFlushBoth
        EXPORT CacheInvBoth
;==================================================================
; Cache Invalidation code for Cortex-A8
;==================================================================     
CacheInvBoth
        mov   r0,1
        B CacheFlushInvBoth
CacheFlushBoth
        mov   r0,0
CacheFlushInvBoth
        ;MOV     pc, lr  ;////////////////////////////////
        STMFD	SP!,	{R0-R12, LR}
        mov  r8 , r0
        ; Invalidate L1 Instruction Cache
        dmb					       ; ensure ordering with previous memory accesses
        MRC p15, 1, r0, c0, c0, 1   ; Read CLIDR
        TST r0, #0x3                ; Harvard Cache?
        MOV r0, #0
        MCRNE p15, 0, r0, c7, c5, 0 ; Invalidate Instruction Cache
        
        ; Invalidate Data/Unified Caches
        
        MRC p15, 1, r0, c0, c0, 1   ; Read CLIDR
        ANDS r3, r0, #0x7000000
        MOV r3, r3, LSR #23         ; Total cache levels << 1
        BEQ Finished
    
        MOV r10, #0                 ; R10 holds current cache level << 1
Loop1   ADD r2, r10, r10, LSR #1    ; R2 holds cache "Set" position 
        MOV r1, r0, LSR r2          ; Bottom 3 bits are the Cache-type for this level
        AND r1, R1, #7              ; Get those 3 bits alone
        CMP r1, #2
        BLT Skip                    ; No cache or only instruction cache at this level
        
        MCR p15, 2, r10, c0, c0, 0  ; Write the Cache Size selection register
        MOV r1, #0
        MCR p15, 0, r1, c7, c5, 4   ; PrefetchFlush to sync the change to the CacheSizeID reg
        MRC p15, 1, r1, c0, c0, 0   ; Reads current Cache Size ID register
        AND r2, r1, #&7             ; Extract the line length field
        ADD r2, r2, #4              ; Add 4 for the line length offset (log2 16 bytes)
        LDR r4, =0x3FF
        ANDS r4, r4, r1, LSR #3     ; R4 is the max number on the way size (right aligned)
        CLZ r5, r4                  ; R5 is the bit position of the way size increment
        LDR r7, =0x00007FFF
        ANDS r7, r7, r1, LSR #13    ; R7 is the max number of the index size (right aligned)

Loop2   MOV r9, r4                  ; R9 working copy of the max way size (right aligned)

Loop3   ORR r11, r10, r9, LSL r5    ; Factor in the Way number and cache number into R11
        ORR r11, r11, r7, LSL r2    ; Factor in the Set number
        CMP r8,#0x0
        MCREQ p15, 0, r11, c7, c6, 2 ; Invalidate by set/way ; Clean and c14
        MCRNE p15, 0, r11, c7, c14, 2 ; Invalidate by set/way ; Clean and c14
        SUBS r9, r9, #1             ; Decrement the Way number
        BGE Loop3
        SUBS r7, r7, #1             ; Decrement the Set number
        BGE Loop2
Skip    ADD r10, r10, #2            ; increment the cache number
        CMP r3, r10
        BGT Loop1
Finished
	    dsb	
	    isb   
	    LDMFD	SP!, {R0-R12, PC}

        END
