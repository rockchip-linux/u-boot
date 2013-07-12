;/****************************************************************** 
;*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  *    
;******************************************************************* 
;File 	:	CacheMMU.h                                                
;Desc   :	cache和MMU操作汇编头文件                                              
;Author : nizy                                                       
;Date 	:	2008-11-11                                                
;Notes  :                                                             
;$Log: CacheMMU.h,v $
;Revision 1.1  2010/12/06 02:43:50  Administrator
;*** empty log message ***
;
;Revision 1.00  2008/11/11 	nizy                            
********************************************************************/

;------------------------------------         
CSIZE   EQU 14  ; cache size as 2**CSIZE (16 K assumed)
CLINE   EQU  5  ; cache line size in bytes as 2**CLINE
NWAY    EQU  2  ; set associativity = 2**NWAY (4 way)
I7SET   EQU  5  ; CP15 c7 set incrementer as 2**ISET
I7WAY   EQU 30  ; CP15 c7 way incrementer as 2**IWAY

SWAY    EQU (CSIZE-NWAY)       ; size of way = 2**SWAY
NLINE   EQU (CSIZE-CLINE)      ; size if line = 2**NLINE
NSET    EQU (CSIZE-NWAY-CLINE) ; cachelines per way = 2**NSET
;------------------------------------  

;------------------------------------          
SUCCESS EQU  0
ERROR   EQU  1       
;------------------------------------ 

        END