/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDM_API_
#define _SDM_API_

#include "hw_SDCommon.h"

/* SDM return value */
#define SDM_SUCCESS              (0)                     /* 操作成功 */
#define SDM_FALSE                (0x1 << 0)              /* 操作失败 */
#define SDM_CARD_NOTPRESENT      (0x1 << 1)              /* 要操作的卡不在卡槽上 */
#define SDM_PARAM_ERROR          (0x1 << 2)              /* 参数有错 */
#define SDM_RESP_ERROR           (0x1 << 3)              /* 卡的回复错误 */
#define SDM_RESP_CRC_ERROR       (0x1 << 4)              /* 卡的回复CRC校验错误 */
#define SDM_RESP_TIMEOUT         (0x1 << 5)              /* 卡的回复timeout */
#define SDM_DATA_CRC_ERROR       (0x1 << 6)              /* 卡的数据CRC错误 */
#define SDM_DATA_READ_TIMEOUT    (0x1 << 7)              /* 读卡的数据timeout */
#define SDM_END_BIT_ERROR        (0x1 << 8)              /* 数据结束位错误 */
#define SDM_START_BIT_ERROR      (0x1 << 9)              /* 数据起始位错误 */
#define SDM_BUSY_TIMEOUT         (0x1 << 10)             /* busy时间太久了 */
#define SDM_DMA_BUSY             (0x1 << 11)             /* dma busy */
#define SDM_ERROR                (0x1 << 12)             /* SDMMC host controller error */
#define SDM_VOLTAGE_NOT_SUPPORT  (0x1 << 13)             /* 卡的工作电压不在host的提供范围内，所以无法正常工作 */
#define SDM_FUNC_NOT_SUPPORT     (0x1 << 14)             /* 要求进行的操作，卡不支持 */
#define SDM_UNKNOWABLECARD       (0x1 << 15)             /* 不认识的卡 */
#define SDM_CARD_WRITE_PROT      (0x1 << 16)             /* 卡被写保护 */
#define SDM_CARD_LOCKED          (0x1 << 17)             /* 卡被锁住了 */
#define SDM_CARD_CLOSED          (0x1 << 18)             /* 卡已经被调用SDM_Close关闭了 */

/* SDM IOCTRL cmd */
#define SDM_IOCTRL_REGISTER_CARD         (0x0)           /* 注册一张卡 */
#define SDM_IOCTRL_UNREGISTER_CARD       (0x1)           /* 注销一张卡 */
#define SDM_IOCTRL_SET_PASSWORD          (0x2)           /* 设置密码 */
#define SDM_IOCTRL_CLEAR_PASSWORD        (0x3)           /* 清除密码 */
#define SDM_IOCTRL_FORCE_ERASE_PASSWORD  (0x4)           /* 强制擦除密码，卡内的所有数据都将丢失 */
#define SDM_IOCTRL_LOCK_CARD             (0x5)           /* 锁卡，没有密码的卡锁不会成功 */
#define SDM_IOCTRL_UNLOCK_CARD           (0x6)           /* 解锁 */
#define SDM_IOCTR_GET_CAPABILITY         (0x7)           /* 获取容量 */
#define SDM_IOCTR_GET_PSN                (0x8)           /* 得到卡的Product serial number */
#define SDM_IOCTR_IS_CARD_READY          (0x9)           /* 得到卡是否准备就绪，可用 */
#define SDM_IOCTR_FLUSH                  (0xA)           /* 对卡进行flush操作 */
#define SDM_IOCTR_GET_BOOT_CAPABILITY    (0xB)           /* 获取Boot partition容量 */
#define SDM_IOCTR_INIT_BOOT_PARTITION    (0xC)           /* 切换到R/W Boot partition,具体切换到哪个boot partition有IOCTL参数决定 */
#define SDM_IOCTR_DEINIT_BOOT_PARTITION  (0xD)           /* 关闭对boot partition的访问，切换到user area */
#define SDM_IOCTR_ACCESS_BOOT_PARTITION  (0xE)           /* Access boot partition or user area */
#define SDM_IOCTR_SET_BOOT_BUSWIDTH      (0xF)           /* 设置启动模式下的线宽 */
#define SDM_IOCTR_SET_BOOT_PART_SIZE     (0x10)          /* 设置启动模式下的线宽 */


/****************************************************************
			对外函数声明
****************************************************************/
void   SDM_Init(uint32 CardId);
int32  SDM_Open(int32 cardId);
int32  SDM_Close(int32 cardId);
int32  SDM_Read(int32 cardId, uint32 blockNum, uint32 blockCount, void *pBuf);
int32  SDM_Write(int32 cardId, uint32 blockNum, uint32 blockCount, void *pBuf);
int32  SDM_IOCtrl(uint32 cmd, void *param);

/* 专门给CMMB使用的 */
int32 SDM_SendCmd(int32 cardId,
		uint32 cmd,
		uint32 cmdArg,
		uint32 *responseBuf,
		uint32  blockSize,
		uint32  dataLen,
		void   *pDataBuf);

#endif /* end of #ifndef _SDM_API_ */

#endif /* endi of #ifdef DRIVERS_SDMMC */
