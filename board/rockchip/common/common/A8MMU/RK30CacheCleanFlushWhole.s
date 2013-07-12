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
CacheFlushBoth                
        STMFD   SP!,    {R0-R12, LR}
        dmb                        ; ensure ordering with previous memory accesses Read cache size from the Cache Size Identification Register
        MRC p15, 1, r3, c0, c0, 0           ; Read current Cache Size Identification Register
        MOV r1, #0x1FF
        AND r3, r1, r3, LSR #13             ; r3 = (number of sets - 1)
        MOV  r0, #0                         ; r0 -> way counter
Flush_way_loop
        MOV  r1, #0                         ; r1 -> set counter
Flush_set_loop
        MOV  r2, r0, LSL #30                ;
        ORR  r2, r1, LSL #5                 ; r2 -> set/way cache-op format
        MCR  p15, 0, r2, c7, c14, 2          ; clean and invalidate
        ADD  r1, r1, #1                     ; Increment set counter
        CMP  r1, r3                         ; Check if the last set is reached...
        BLE  Flush_set_loop                       ; ...if not, continue the set_loop...
        ADD  r0, r0, #1                     ; ...else, Increment way counter
        CMP  r0, #4                         ; Check if the last way is reached...
        BLT  Flush_way_loop                       ; ...if not, continue the way_loop
        dsb 
        isb   
        LDMFD   SP!, {R0-R12, PC}
        
CacheInvBoth                
        STMFD   SP!,    {R0-R12, LR}
        dmb                        ; ensure ordering with previous memory accesses Read cache size from the Cache Size Identification Register
        MRC p15, 1, r3, c0, c0, 0           ; Read current Cache Size Identification Register
        MOV r1, #0x1FF
        AND r3, r1, r3, LSR #13             ; r3 = (number of sets - 1)
        MOV  r0, #0                         ; r0 -> way counter
Inv_way_loop
        MOV  r1, #0                         ; r1 -> set counter
Inv_set_loop
        MOV  r2, r0, LSL #30                ;
        ORR  r2, r1, LSL #5                 ; r2 -> set/way cache-op format
        MCR  p15, 0, r2, c7, c6, 2          ; Invalidate line described by r2
        ADD  r1, r1, #1                     ; Increment set counter
        CMP  r1, r3                         ; Check if the last set is reached...
        BLE  Inv_set_loop                       ; ...if not, continue the set_loop...
        ADD  r0, r0, #1                     ; ...else, Increment way counter
        CMP  r0, #4                         ; Check if the last way is reached...
        BLT  Inv_way_loop                       ; ...if not, continue the way_loop
        dsb 
        isb   
        LDMFD   SP!, {R0-R12, PC}
        END


        
