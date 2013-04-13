;/******************************************************************/ 
;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	MMUOperationTLB.s                                                
;Desc   :	TLB²Ù×÷ÎÄ¼þ                                               
;Author : nizy                                                       
;Date 	:	2008-11-20                                                
;Notes  :                                                             
;$Log	  : Revision 1.00  2008/11/20 	nizy                            
;********************************************************************/ 
        CODE32

        AREA operationtlb, CODE, READONLY 
;----------------------------------------------------------------
; /* Don't need to flush singal TLB entry beford calling function MMULockTLB,
;    but should manage the TLB lokedown region that ensure it don't cover any other PTE */
; C-prototype
; void  MMUFlushTLB(void);  
; void  MMUFlushSingleTLB(uint32 *adr)
; void  MMULockTLB(uint32 *adr)
;----------------------------------------------------------------
        
        INCLUDE CacheMMU.h

        EXPORT MMUFlushTLB
        EXPORT MMUFlushSingleTLB
        EXPORT MMULockTLB

        
c7f    RN  0
adr    RN  0
rul    RN  0
c10f   RN  1
tmp    RN  2


;------------------------------------        
MMUFlushTLB
        MOV     c7f, #0
        MCR     p15, 0, c7f, c8, c7, 0 
        MOV     pc, lr
;------------------------------------
     

;------------------------------------        
MMUFlushSingleTLB
        MOV     adr, adr, LSR #10
        MOV     adr, adr, LSL #10       
        MCR     p15, 0, adr, c8, c7, 1 
        MOV     pc, lr
;------------------------------------     


;------------------------------------
MMULockTLB            
        MRC     p15, 0, c10f, c10, c0, 0   ; read the lockdown register
        MCR     p15, 0, adr, c8, c7, 1     ; flush TLB single entry to ensure that lockAddr is not already in the TLB
        ORR     c10f, c10f, #1             ; set the preserve bit
        MCR     p15, 0, c10f, c10, c0, 0   ; write to the lockdown register        
        LDR     c10f, [adr]                ; TLB will miss, and entry will be loaded to lockdown region         
        MRC     p15, 0, c10f, c10, c0, 0   ; read the lockdown register (victim will have incremented)
        BIC     c10f, c10f, #1             ; clear preserve bit
        MCR     p15, 0, c10f, c10, c0, 0   ; write to the lockdown register        
        MOV     pc, lr
;------------------------------------ 

 
        END
