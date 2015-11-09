/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_SD.h
//描述:SD protocol api file
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_SD.h,v $
****************************************************************/
#ifdef DRIVERS_SDMMC

#ifndef _SDP_API_H_
#define _SDP_API_H_

/****************************************************************/
//对外函数声明
/****************************************************************/
void   SD1X_Init(void *pCardInfo);
void   SD20_Init(void *pCardInfo);

#endif //end of #ifndef _SDP_API_H

#endif //end of #ifdef DRIVERS_SDMMC
