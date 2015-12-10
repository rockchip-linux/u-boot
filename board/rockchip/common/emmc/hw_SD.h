/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDP_API_H_
#define _SDP_API_H_

/****************************************************************
			对外函数声明
****************************************************************/
void   SD1X_Init(void *pCardInfo);
void   SD20_Init(void *pCardInfo);

#endif /* end of #ifndef _SDP_API_H */

#endif /* end of #ifdef DRIVERS_SDMMC */
