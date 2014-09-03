/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_MMC.h
//描述:MMC protocol api file
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_MMC.h,v $
****************************************************************/
#ifdef DRIVERS_SDMMC

#ifndef _MMCP_API_H_
#define _MMCP_API_H_

/****************************************************************/
//对外函数声明
/****************************************************************/
void   MMC_Init(void *pCardInfo);
int32  MMC_SwitchBoot(void *pCardInfo, uint32 enable, uint32 partition);
int32  MMC_AccessBootPartition(void *pCardInfo, uint32 partition);

uint8 MMC_GetMID(void);

#endif //end of #ifndef _MMCP_API_H

#endif //end of #ifdef DRIVERS_SDMMC
