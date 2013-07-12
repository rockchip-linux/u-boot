;/******************************************************************/ 
;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	MMUControl.s                                                
;Desc   :	mmu状态设置操作文件                                               
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log	  : Revision 1.00  2008/11/11 	nizy                            
;********************************************************************/ 
        CODE32
        
        AREA controlmmu, CODE, READONLY
;----------------------------------------------------------------
; /* DomainID is less than 16,
;    processID is less than 128 */
; C-prototype
; uint32 MMUSetDomain(uint32 id, uint32 domain);    
; uint32 MMUSetProcessID(uint32 processID);
; void   MMUProtection(uint32 romSystm); 
; void   MMUSetTTB(uint32 *ttb); 
; void   MMUEnable(void);
; void   MMUDisable(void); 
;------------------------------------------------------------------------        

        INCLUDE CacheMMU.h
   
        EXPORT MMUSetDomain
        EXPORT MMUSetTTB
        EXPORT MMUSetProcessID
        EXPORT MMUEnable
        EXPORT MMUDisable        
        EXPORT MMUProtection
  
                      
id        RN  0 
c1f       RN  0  
romSystm  RN  0
ttb       RN  0
processID RN  0  
rul       RN  0 
domain    RN  1 
c1ft      RN  1 
c2f       RN  1
c13f      RN  1
c3f       RN  2
mask      RN  3
                            
;------------------------------------
MMUEnable
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00000001    ; mmu 
        ORR     c1f, c1f, #0x00000800    ; Z    
        MCR     p15, 0, c1f, c1, c0, 0
        MOV     pc, lr     
 ;------------------------------------
MMUDisable
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00000001  
        BIC     c1f, c1f, #0x00000800    ; Z    
        MCR     p15, 0, c1f, c1, c0, 0     
        MOV     pc, lr       
;------------------------------------
MMUProtection
        MRC     p15, 0, c1ft, c1, c0, 0
        AND     romSystm, romSystm, #0x00000003
        BIC     c1ft, c1ft, #0x00000300
        MOV     romSystm, romSystm, LSL #8
        ORR     c1ft, c1ft, romSystm       
        MCR     p15, 0, c1ft, c1, c0, 0
        MOV     pc, lr         
;------------------------------------
MMUSetDomain
        CMP     id, #15
        MOVHI   rul, #ERROR      
        BHI     %FT7
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
7        
        MOV     pc, lr          
;------------------------------------
MMUSetTTB
        MOV     c2f, ttb, LSR #14 
        MOV     c2f, c2f, LSL #14       
        MCR     p15, 0, c2f, c2, c0, 0 
        MOV     pc, lr          
;------------------------------------         
MMUSetProcessID
        CMP     processID, #127
        MOVHI   rul, #ERROR
        BHI     %FT10
        MOV     c13f, processID, LSL #25       
        MCR     p15, 0, c13f, c13, c0, 0 
        MOV     rul, #SUCCESS 
10                     
        MOV     pc, lr
;------------------------------------

        END
