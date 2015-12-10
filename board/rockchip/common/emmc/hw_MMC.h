/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _MMCP_API_H_
#define _MMCP_API_H_

/****************************************************************
			对外函数声明
****************************************************************/
void MMC_Init(void *pCardInfo);
int32 MMC_SwitchBoot(void *pCardInfo, uint32 enable, uint32 partition);
int32 MMC_AccessBootPartition(void *pCardInfo, uint32 partition);
int32 MMC_SetBootBusWidth(void *pCardInfo, uint32 enable, HOST_BUS_WIDTH_E width);
uint8 MMC_GetMID(void);

#endif /* end of #ifndef _MMCP_API_H */

#endif /* end of #ifdef DRIVERS_SDMMC */
