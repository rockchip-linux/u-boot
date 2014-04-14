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
Revision 1.1  2011/03/29 09:20:48  Administrator
1、支持三星8GB EMMC
2、支持emmc boot 1和boot 2都写入IDBLOCK数据

Revision 1.1  2011/01/18 07:20:30  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 11:55:39  Administrator
*** empty log message ***

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

****************************************************************/
#ifdef DRIVERS_SDMMC

#ifndef _MMCP_API_H_
#define _MMCP_API_H_

/****************************************************************/
//对外函数声明
/****************************************************************/
void   MMC_Init(void *pCardInfo);
int32  MMC_SwitchBoot(void *pCardInfo, bool enable, uint32 partition);
int32  MMC_AccessBootPartition(void *pCardInfo, uint32 partition);

uint8 MMC_GetMID(void);

#endif //end of #ifndef _MMCP_API_H

#endif //end of #ifdef DRIVERS_SDMMC
