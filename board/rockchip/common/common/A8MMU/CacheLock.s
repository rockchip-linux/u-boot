;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	CacheLock.s                                                
;Desc   :	cacheËø¶¨ÎÄ¼þ                                               
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log: CacheLock.s,v $
;Revision 1.1  2010/12/06 02:43:49  Administrator
;*** empty log message ***
;
;Revision 1.00  2008/11/11 	nizy                            
;********************************************************************/ 
        CODE32

        AREA lockcacheblock, CODE, READONLY ; Start of Area block
;-----------------------------------------------------------------
; /* Should cleanflush Dcache or flush Icache beford locking cache way,
;    but pay attention to code and data already locked in other way,
;    so need to manage lock program prevent from missing locked code and data */
; C-prototypes
; uint32 CacheLockD(uint32 *adr, uint32 size); 
; uint32 CacheLockI(uint32 *adr, uint32 size); 
; uint32 CacheUnLockD(uint32 way); 
; uint32 CacheUnLockI(uint32 way);
; uint32 CacheGetLockD(void);
; uint32 CacheGetLockI(void); 
;-----------------------------------------------------------------

        INCLUDE CacheMMU.h

        EXPORT CacheLockD
        EXPORT CacheLockI
        EXPORT CacheUnLockD
        EXPORT CacheUnLockI
        EXPORT CacheGetLockD
        EXPORT CacheGetLockI
                    
adr   RN 0    ; current address of code or data
way   RN 0    ; unlock way number
rul   RN 0 
size  RN 1    ; memory size in bytes
tmp   RN 2    ; scratch register
tmp1  RN 3    ; scratch register 
c9f   RN 12   ; CP15:c9 register format

;------------------------------------
        MACRO
        CACHELOCKBYLBIT $op

        CMP     size, #0                  
        MOVEQ   tmp1, #0x10                
        BEQ     %FT1                      ; ERROR: no lockdown requested
                                          ; exit return 0x10     
                                          
        ADD     size, adr, size           ; size = end address
        BIC     adr, adr, #(1<<CLINE)-1   ; align to CLINE
        MOV     tmp, #(1<<CLINE)-1        ; scratch CLINE mask
        TST     size, tmp                 ; CLINE end fragment ?
        SUB     size, size, adr           ; add alignment bytes
        MOV     size, size, LSR #CLINE    ; convert size 2 # CLINE
        ADDNE   size, size, #1            ; add CLINE for fragment
                                          
        CMP     size, #(1<<NSET)          ; 
        MOVHI   tmp1, #0x20               
        BHI     %FT1                      ; ERROR: size beyond maxsize
                                          ; exit return 0x20

        IF "$op" = "Icache"
          MRC     p15, 0, c9f, c9, c0, 1  ; get i-cache lock bits
        ENDIF 
        IF "$op" = "Dcache"
          MRC     p15, 0, c9f, c9, c0, 0  ; get d-cache lock bits
        ENDIF
        
        AND     tmp, c9f, #0xf            ; tmp = state of Lbits
        MOV     tmp1, #1                  
        TST     c9f, tmp1                 ; test lock bit 0
        MOVNE   tmp1, tmp1, LSL #1        
        TSTNE   c9f, tmp1                 ; test lock bit 1
        MOVNE   tmp1, tmp1, LSL #1        
        TSTNE   c9f, tmp1                 ; test lock bit 2
        MOVNE   tmp1, tmp1, LSL #1        
        BNE     %FT1                      ; ERROR: no available ways
                                          ; exit return 0x8
                                          
        MVN     tmp1, tmp1                ; select L bit 
        AND     tmp1, tmp1, #0xf          ; mask off non L bits
        BIC     c9f, c9f, #0xf            ; construct c9f
        ADD     c9f, c9f, tmp1
     
        IF "$op" = "Icache"
          MCR     p15, 0, c9f, c9, c0, 1  ; set lock I page
        ENDIF                             
                                          
        IF "$op" = "Dcache"               
          MCR     p15, 0, c9f, c9, c0, 0  ; set lock D page
        ENDIF
5
        IF "$op" = "Icache"
         MCR     p15, 0, adr, c7, c13, 1  ; load code cacheline
         ADD     adr, adr, #1<<CLINE      ; cline addr =+ 1
        ENDIF   
        
        IF "$op" = "Dcache"
          LDR     tmp1, [adr], #1<<CLINE  ; load data cacheline,cline addr =+ 1 
        ENDIF

        SUBS    size, size, #1            ; cline =- 1
        BNE     %BT5                      ; loop thru clines
                                          
        MVN     tmp1, c9f                 ; lock selected L-bit
        AND     tmp1, tmp1, #0xf          ; mask off non L-bits
        ORR     tmp, tmp, tmp1            ; merge with orig L-bits
        BIC     c9f, c9f, #0xf            ; clear all L-bits
        ADD     c9f, c9f, tmp             ; set L-bits in c9f
        
        IF "$op" = "Icache"
          MCR     p15, 0, c9f, c9, c0, 1  ; set i-cache lock bits
        ENDIF                             
                                          
        IF "$op" = "Dcache"               
          MCR     p15, 0, c9f, c9, c0, 0  ; set d-cache lock bits
        ENDIF
1
        MOV     rul, tmp1                 ; return allocated way
        MOV     pc, lr
        
        MEND
;------------------------------------
CacheLockD
        CACHELOCKBYLBIT Dcache
CacheLockI
        CACHELOCKBYLBIT Icache


 
;------------------------------------
        MACRO
        CACHEGETLOCKBYLBIT $op
                
               
        IF "$op" = "Icache"
          MRC     p15, 0, way, c9, c0, 1  ; set i-cache lock bits
        ENDIF
        
        IF "$op" = "Dcache"
          MRC     p15, 0, way, c9, c0, 0  ; set i-cache lock bits
        ENDIF
        
        AND     rul, way, #0xf            ; return allocated way
        MOV     pc, lr
                        
        MEND
;------------------------------------
CacheGetLockD
        CACHEGETLOCKBYLBIT Dcache
CacheGetLockI
        CACHEGETLOCKBYLBIT Icache
 
  
        
;------------------------------------
        MACRO
        CACHEUNLOCKBYLBIT $op
                
        CMP     way, #3
        MOVHI   rul, #ERROR             
        BHI     %FT7
        
        MVN     way, way
        AND     way, way, #0xf
               
        IF "$op" = "Icache"
          MRC     p15, 0, c9f, c9, c0, 1  ; set i-cache lock bits
        ENDIF                             
                                          
        IF "$op" = "Dcache"               
          MRC     p15, 0, c9f, c9, c0, 0  ; set i-cache lock bits
        ENDIF
        
        AND     c9f, c9f, way 
        
        IF "$op" = "Icache"        
          MCR     p15, 0, c9f, c9, c0, 1  ; set d-cache lock bits
        ENDIF                             
                                          
        IF "$op" = "Dcache"               
          MCR     p15, 0, c9f, c9, c0, 0  ; set d-cache lock bits
        ENDIF
        
        AND     c9f, c9f, #0xf
        MOV     rul, #SUCCESS             
7        
        MOV     pc, lr
        
        MEND
;------------------------------------
CacheUnLockD
        CACHEUNLOCKBYLBIT Dcache
CacheUnLockI
        CACHEUNLOCKBYLBIT Icache


        END