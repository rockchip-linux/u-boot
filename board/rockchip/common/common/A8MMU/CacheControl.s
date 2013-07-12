;/******************************************************************/ 
;/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */    
;/******************************************************************* 
;File 	:	CacheControl.s                                                
;Desc   :	cache状态设置操作文件                                               
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log: CacheControl.s,v $
;Revision 1.1  2010/12/06 02:43:49  Administrator
;*** empty log message ***
;
;Revision 1.00  2008/11/11 	nizy                            
;********************************************************************/ 
        CODE32
        
        AREA controlcachearea, CODE, READONLY
;----------------------------------------------------------------
; C-prototype
; void CacheRoundRobinReplace(void);   
; void CacheRandomReplace(void);      
; void CacheEnableBoth(void);             
; void CacheDisableBoth(void);          
; void CacheEnableD(void);                   
; void CacheDisableD(void);                   
; void CacheEnableI(void);                    
; void CacheDisableI(void); 
; void WriteBufferDrain(void);
;------------------------------------------------------------------------        
   
        EXPORT CacheRoundRobinReplace
        EXPORT CacheRandomReplace
        EXPORT CacheEnableBoth
        EXPORT CacheDisableBoth
        EXPORT CacheEnableD
        EXPORT CacheDisableD
        EXPORT CacheEnableI
        EXPORT CacheDisableI
        EXPORT WriteBufferDrain
        

c1f       RN  0

;------------------------------------
CacheRoundRobinReplace
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00004000        
        MCR     p15, 0, c1f, c1, c0, 0 
        MOV     pc, lr  
;------------------------------------
CacheRandomReplace
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00004000        
        MCR     p15, 0, c1f, c1, c0, 0 
        MOV     pc, lr            
;------------------------------------
CacheEnableBoth
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00001000        
        ORR     c1f, c1f, #0x00000004
        MCR     p15, 0, c1f, c1, c0, 0  

        ;MRC     p15, 0, c1f, c1, c0, 1
        ;ORR     c1f, c1f, #0x00000006       ;//prefetch L1 L2 
        ;MCR     p15, 0, c1f, c1, c0, 1  

        MOV     pc, lr          
;------------------------------------
CacheDisableBoth
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00001000   
        BIC     c1f, c1f, #0x00000004   
        MCR     p15, 0, c1f, c1, c0, 0     
        MOV     pc, lr                                
;------------------------------------
CacheEnableD
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00000004        
        MCR     p15, 0, c1f, c1, c0, 0      
        MOV     pc, lr          
;------------------------------------
CacheDisableD
        MRC     p15, 0, c1f, c1, c0, 0
        BIC     c1f, c1f, #0x00000004   
        MCR     p15, 0, c1f, c1, c0, 0     
        MOV     pc, lr          
;------------------------------------
CacheEnableI
        MRC     p15, 0, c1f, c1, c0, 0
        ORR     c1f, c1f, #0x00001000        
        MCR     p15, 0, c1f, c1, c0, 0
        MOV     pc, lr          
;------------------------------------
CacheDisableI
        MRC     p15, 0, c1f, c1, c0, 0       
        BIC     c1f, c1f, #0x00001000   
        MCR     p15, 0, c1f, c1, c0, 0             
        MOV     pc, lr     
;------------------------------------
WriteBufferDrain
        MOV     c1f, #0x00000000     
        MCR     p15, 0, c1f, c7, c10, 4       
        MOV     pc, lr     
;------------------------------------

        END