;/******************************************************************/ 
;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	CacheCleanFlushRegion.s                                                
;Desc   :	cache部分清理和清除操作文件                                               
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log: CacheCleanFlushRegion.s,v $
;Revision 1.1  2010/12/06 02:43:49  Administrator
;*** empty log message ***
;
;Revision 1.00  2008/11/11 	nizy                            
;********************************************************************/ 
        CODE32

        AREA cacheRegionArea, CODE, READONLY 
;------------------------------------------------------------------------
; /* More efficient if address is sequence and size is exiguity,
;    ensure that it can't be interrupted when performing clean or flush */
; C-prototypes
; uint32 CacheFlushIRegion(uint32 *adr, uint32 size);
; uint32 CacheFlushDRegion(uint32 *adr, uint32 size);
; uint32 CacheFlushBothRegion(uint32 *adr, uint32 size);
; uint32 CacheCleanDRegion(uint32*adr, uint32 size);
; uint32 CacheCleanFlushDRegion(uint32 *adr, uint32 size);
; uint32 CacheCleanFlushRegion(uint32 *adr, uint32 size);
;------------------------------------------------------------------------

        INCLUDE CacheMMU.h
        
        ;EXPORT CacheFlushIRegion
        EXPORT __CacheFlushDRegion
        ;EXPORT CacheFlushBothRegion
        EXPORT __CacheCleanDRegion
        ;EXPORT CacheCleanFlushDRegion
        ;EXPORT CacheCleanFlushRegion           

adr   RN  0   ; active address
rul   RN  0
size  RN  1   ; size of region in bytes
tmp   RN  2
set   RN  3
        
;------------------------------------
        MACRO 
        CACHEBYREGION $op
		;MOV     pc, lr  ;////////////////////////////////

        CMP     size, #0                     ; no flush requested 
        MOVEQ   rul, #ERROR
        BEQ     %FT1                         ; exit error

        ADD     size, adr, size              ; size = end address
       
        mrc p15, 1, tmp, c0, c0, 0     ; read CSIDR
        and  tmp, tmp, #7              ; cache line size encoding
        mov  set, #16                 ; size offset
        mov  set, set, lsl tmp        ; actual cache line size
        
        sub tmp, set, #1              ;
        bic adr, adr, tmp             ;align to cache line sizes
        
10
        IF "$op" = "IcacheFlush"
          MCR     p15, 0, adr, c7, c5, 1     ; flush Icline@adr
        ENDIF
        
        IF "$op" = "DcacheFlush"
          MCR     p15, 0, adr, c7, c14, 1     ; flush Dccline@adr
        ENDIF
        
        IF "$op" = "IDcacheFlush"
          MCR     p15, 0, adr, c7, c5, 1     ; flush Icline@adr
          MCR     p15, 0, adr, c7, c14, 1     ; flush Dcline@adr
        ENDIF
        
        IF "$op" = "DcacheClean"
          MCR     p15, 0, adr, c7, c10, 1    ; clean Dcline@adr
        ENDIF
        
        IF "$op" = "DcacheCleanFlush"
          MCR     p15, 0, adr, c7, c14, 1    ; cleanflush Dcline@adr
        ENDIF
        
        IF "$op" = "IDcacheCleanFlush"
          MCR     p15, 0, adr, c7, c14, 1    ; cleanflush Dcline@adr
          MCR     p15, 0, adr, c7, c5, 1     ; flush Icline@adr 	
        ENDIF
        
        ADD     adr, adr, set          ; +1 next cline adr
        cmp     adr, size               ; 
        BLO     %BT10                  ; flush # lines +1        
        MOV     rul, #SUCCESS          ; exit success
1  
        dsb
        MOV     pc, lr
        
        MEND
;------------------------------------

;CacheFlushIRegion
;        CACHEBYREGION IcacheFlush
__CacheFlushDRegion
        CACHEBYREGION DcacheFlush
;CacheFlushBothRegion
;        CACHEBYREGION IDcacheFlush
__CacheCleanDRegion
        CACHEBYREGION DcacheClean
;CacheCleanFlushDRegion
;        CACHEBYREGION DcacheCleanFlush
;CacheCleanFlushRegion
;        CACHEBYREGION IDcacheCleanFlush   
        
        END
