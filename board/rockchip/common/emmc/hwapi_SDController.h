/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDC_API_
#define _SDC_API_

/* SDC return value */
#define SDC_SUCCESS              SDM_SUCCESS             /* 操作成功 */
#define SDC_FALSE                SDM_FALSE               /* 操作失败 */
#define SDC_CARD_NOTPRESENT      SDM_CARD_NOTPRESENT     /* 卡不存在或被拔出 */
#define SDC_PARAM_ERROR          SDM_PARAM_ERROR         /* 参数错误 */
#define SDC_RESP_ERROR           SDM_RESP_ERROR          /* 卡的回复错误 */
#define SDC_RESP_CRC_ERROR       SDM_RESP_CRC_ERROR      /* 卡的回复CRC校验错误 */
#define SDC_RESP_TIMEOUT         SDM_RESP_TIMEOUT        /* 卡的回复timeout */
#define SDC_DATA_CRC_ERROR       SDM_DATA_CRC_ERROR      /* 卡的数据CRC错误 */
#define SDC_DATA_READ_TIMEOUT    SDM_DATA_READ_TIMEOUT   /* 读卡的数据timeout */
#define SDC_END_BIT_ERROR        SDM_END_BIT_ERROR       /* 数据结束位错误 */
#define SDC_START_BIT_ERROR      SDM_START_BIT_ERROR     /* 数据起始位错误 */
#define SDC_BUSY_TIMEOUT         SDM_BUSY_TIMEOUT        /* busy时间太久了 */
#define SDC_DMA_BUSY             SDM_DMA_BUSY            /* dma busy */
#define SDC_SDC_ERROR            SDM_ERROR               /* SDMMC host controller error */

/* Host Bus Width */
typedef enum HOST_BUS_WIDTH_Enum {
	BUS_WIDTH_INVALID = 0,
	BUS_WIDTH_1_BIT,
	BUS_WIDTH_4_BIT,
	BUS_WIDTH_8_BIT,
	BUS_WIDTH_MAX
} HOST_BUS_WIDTH_E;

/****************************************************************
			对外函数声明
****************************************************************/
void  SDC_Init(uint32 CardId);
uint32  SDC_IsCardIdValid(int32 cardId);
uint32  SDC_IsCardPresence(int32 cardId);
uint32  SDC_IsCardWriteProtected(int32 cardId);
int32 SDC_UpdateCardFreq(int32 cardId, uint32 freqKHz);
void  SDC_UpdateAhbFreq(uint32 newAhbFreq);
int32 SDC_GetHostBusWidth(int32 cardId, HOST_BUS_WIDTH_E *pWidth);
int32 SDC_SetHostBusWidth(int32 cardId, HOST_BUS_WIDTH_E width);
int32 SDC_ResetController(int32 cardId);
int32 SDC_EnableSdioInt(int32 cardId, pFunc cb);
int32 SDC_DisableSdioInt(int32 cardId);
int32 SDC_ControlClock(int32 cardId, uint32 enable);
int32 SDC_ControlPower(int32 cardId, uint32 enable);
int32 SDC_SetDDRMode(int32 cardId, uint32 enable);
int32 SDC_SetDDRTuning(int32 cardId, uint32 step);
int32 SDC_WaitCardBusy(int32 cardId);
int32 SDC_BusRequest(int32 cardId,
		uint32 cmd,
		uint32 cmdArg,
		uint32 *responseBuf,
		uint32  blockSize,
		uint32  dataLen,
		void   *pDataBuf);
#define SDC_SendCommand(cardId, cmd, cmdArg, responseBuf)  \
		SDC_BusRequest(cardId, cmd, cmdArg, responseBuf, 0, 0, NULL);
#define SDC_ReadBlockData(cardId, cmd, cmdArg, responseBuf, dataLen, pDataBuf)  \
		SDC_BusRequest(cardId, cmd, cmdArg, responseBuf, 512, dataLen, pDataBuf);
#define SDC_WriteBlockData(cardId, cmd, cmdArg, responseBuf, dataLen, pDataBuf)  \
		SDC_BusRequest(cardId, cmd, cmdArg, responseBuf, 512, dataLen, pDataBuf);

#endif /* end of #ifndef _SDC_API_ */

#endif /* end of #ifdef DRIVERS_SDMMC */
