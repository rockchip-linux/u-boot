;/******************************************************************/ 
;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	CacheCleanFlushWay.s                                               
;Desc   :	cache路清理和清除操作文件                                               
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log: CacheCleanFlushWay.s,v $
;Revision 1.1  2010/12/06 02:43:49  Administrator
;*** empty log message ***
;
;Revision 1.00  2008/11/11 	nizy                            
;********************************************************************/ 
        CODE32

        AREA cacheWayArea, CODE, READONLY 
;------------------------------------------------------------------------
; /* Ensure that it can't be interrupted when performing clean or flush */
; C-prototypes
; uint32  CacheFlushIWay(uint32 way);     
; uint32  CacheFlushDWay(uint32 way);     
; uint32  CacheFlushBothWay(uint32 way);  
; uint32  CacheCleanDWay(uint32 way);     
; uint32  CacheCleanFlushDWay(uint32 way);
; uint32  CacheCleanFlushWay(uint32 way); 
;------------------------------------------------------------------------        
          
        INCLUDE CacheMMU.h

        EXPORT CacheFlushIWay
        EXPORT CacheFlushDWay
        EXPORT CacheFlushBothWay          
        EXPORT CacheCleanDWay
        EXPORT CacheCleanFlushDWay
        EXPORT CacheCleanFlushWay
                

way  RN  0
rul  RN  0                                                     
c7f  RN  1  
                    
;------------------------------------ 
        MACRO
        CACHEFLUSHCLEANWAY $op
       
        CMP     way, #3
        MOVHI   rul, #ERROR
        BHI     %FT1   
        MOV     c7f, way, LSL #I7WAY        ; create c7 format
5
        IF      "$op" = "IFlushWay"
          MCR     p15, 0, c7f, c7, c5, 2    ; flush I-cline
        ENDIF 
        
        IF      "$op" = "DFlushWay"
          MCR     p15, 0, c7f, c7, c6, 2    ; flush D-cline
        ENDIF
        
        IF      "$op" = "IDFlushWay"
          MCR     p15, 0, c7f, c7, c5, 2    ; flush I-cline
          MCR     p15, 0, c7f, c7, c6, 2    ; flush D-cline
        ENDIF 
               
        IF      "$op" = "Dclean"
          MCR     p15, 0, c7f, c7, c10, 2   ; clean D-cline
        ENDIF 
        
        IF      "$op" = "Dcleanflush"
          MCR     p15, 0, c7f, c7, c14, 2   ; cleanflush D-cline
        ENDIF

        IF      "$op" = "IDcleanflush"
          MCR     p15, 0, c7f, c7, c14, 2   ; cleanflush D-cline
          MCR     p15, 0, c7f, c7, c5, 2    ; flush I-cline
        ENDIF
        
        ADD     c7f, c7f, #1<<I7SET         ; +1 set index
        TST     c7f, #1<<(NSET+I7SET)       ; test index overflow
        BEQ     %BT5         
        MOV     rul, #SUCCESS
1        
        MOV     pc, lr
                
        MEND
;------------------------------------                             
CacheFlushIWay
        CACHEFLUSHCLEANWAY IFlushWay
CacheFlushDWay
        CACHEFLUSHCLEANWAY DFlushWay    
CacheFlushBothWay
        CACHEFLUSHCLEANWAY IDFlushWay
CacheCleanDWay
        CACHEFLUSHCLEANWAY Dclean   
CacheCleanFlushDWay
        CACHEFLUSHCLEANWAY Dcleanflush
CacheCleanFlushWay
        CACHEFLUSHCLEANWAY IDcleanflush    
        

         
        END