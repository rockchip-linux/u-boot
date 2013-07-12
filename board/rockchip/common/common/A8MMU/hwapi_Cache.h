/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . Both Rights Reserved.  	  */
/*******************************************************************
File 	 :	hwapi_cache.h
Desc   :	cache输出头文件 
Author :  nizy
Date 	 :	2008-11-11
Notes  : 
$Log: hwapi_Cache.h,v $
Revision 1.1  2010/12/06 02:43:50  Administrator
*** empty log message ***

Revision 1.00  2008/11/11 	nizy
********************************************************************/

#ifdef DRIVERS_CACHE

#ifndef __HWAPI_CACHE_H__
#define __HWAPI_CACHE_H__



extern void CacheCleanD(void);       
extern void CacheCleanFlushD(void);  
extern void CacheCleanFlush(void);
extern void CacheFlushBoth(void);
extern void CacheInvBoth(void);
extern void CacheFlushD(void);   
extern void CacheFlushI(void);   
extern void CacheRoundRobinReplace(void);    
extern void CacheRandomReplace(void);        
extern void CacheEnableBoth(void);       
extern void CacheDisableBoth(void);      
extern void CacheEnableD(void);          
extern void CacheDisableD(void);         
extern void CacheEnableI(void);          
extern void CacheDisableI(void);  
extern void WriteBufferDrain(void);
extern uint32 CacheFlushIRegion(uint32 adr, uint32 size);
extern uint32 CacheFlushDRegion(uint32 adr, uint32 size);
extern uint32 CacheFlushBothRegion(uint32 adr, uint32 size);
extern uint32 CacheCleanDRegion(uint32 adr, uint32 size);
extern uint32 CacheCleanFlushDRegion(uint32 *adr, uint32 size);
extern uint32 CacheCleanFlushRegion(uint32 *adr, uint32 size);
extern uint32  CacheFlushIWay(uint32 way);     
extern uint32  CacheFlushDWay(uint32 way);     
extern uint32  CacheFlushBothWay(uint32 way);  
extern uint32  CacheCleanDWay(uint32 way);     
extern uint32  CacheCleanFlushDWay(uint32 way);
extern uint32  CacheCleanFlushWay(uint32 way); 
extern uint32 CacheLockD(uint32 *adr, uint32 size);
extern uint32 CacheLockI(uint32 *adr, uint32 size);
extern uint32 CacheUnLockD(uint32 way); 
extern uint32 CacheUnLockI(uint32 way); 
extern uint32 CacheGetLockD(void); 
extern uint32 CacheGetLockI(void); 



#endif

#endif
