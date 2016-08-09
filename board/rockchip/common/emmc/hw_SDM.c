/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define SDM_DRIVER
#include "sdmmc_config.h"

#define RK29_eMMC_Debug 0

#if RK29_eMMC_Debug
static int eMMC_debug = 5;
#define eMMC_printk(n, format, arg...) \
	if (n <= eMMC_debug) \
		PRINT_E(format, ##arg);
#else
#define eMMC_printk(n, arg...)
static const int eMMC_debug;
#endif

#ifdef DRIVERS_SDMMC

/****************************************************************
* 函数名:_RegisterCard
* 描述:注册一张新的卡到SD manager
* 参数说明:cardInfo   输入参数    卡的所有信息
* 返回值:
* 相关全局变量:写gSDMDriver[i].cardInfo
* 注意:
****************************************************************/
static void _RegisterCard(pSDM_CARD_INFO_T pCardInfo)
{
	uint32 i = pCardInfo->cardId;

	if (pCardInfo == NULL)
		return;

	if (i < SDM_MAX_MANAGER_PORT)
		if (gSDMDriver[i].cardInfo.cardId == SDM_INVALID_CARDID)
			SDOAM_Memcpy(&gSDMDriver[i].cardInfo, pCardInfo, sizeof(SDM_CARD_INFO_T));
}

/****************************************************************
* 函数名:_IsCardRegistered
* 描述:查找卡是否已经注册过，并把注册的端口号通过port返回
* 参数说明:cardId   输入参数   需要查找的卡号
*          pPort    输出参数   返回卡注册的端口号
* 返回值:TRUE   卡已经注册过
*        FALSE  未注册的卡
* 相关全局变量:读取gSDMDriver[i].cardInfo.cardId
* 注意:
****************************************************************/
static uint32 _IsCardRegistered(int32 cardId, uint32 *pPort)
{
	*pPort = SDM_MAX_MANAGER_PORT;
	if (gSDMDriver[cardId].cardInfo.cardId == cardId) {
		*pPort = cardId;
		return TRUE;
	}
	return FALSE;
}

/****************************************************************
* 函数名:_ResponseTimeoutHandle
* 描述:发送命令出现timeout的处理
* 参数说明:pCardInfo     输入参数  指向卡信息的指针
* 返回值:SDM_CARD_NOTPRESENT  卡不存在
*        SDM_RESP_TIMEOUT     卡存在，但却不作出回复
*        SDM_CARD_LOCKED      卡被锁住
*        SDM_SUCCESS          成功，可以重新发命令
*        SDM_FALSE            未知错误
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _ResponseTimeoutHandle(int32 cardId)
{
	uint32 status  = 0;
	uint32 repeatCount = 0;
	int32  ret = SDC_SUCCESS;
	uint32 port = SDM_MAX_MANAGER_PORT;

	if (!_IsCardRegistered(cardId, &port))
		return SDM_PARAM_ERROR;

	SDOAM_Printf("HANDLING:card presence\n");
	while (repeatCount < SDM_CMD_RESENT_COUNT) {
		SDOAM_Printf("HANDLING:read status\n");
		ret = SDC_SendCommand(cardId,
				(SD_SEND_STATUS | SD_NODATA_OP | SD_RSP_R1 | NO_WAIT_PREV),
				(gSDMDriver[port].cardInfo.rca << 16),
				&status);
		if (SDC_SUCCESS == ret) {
			if (status & CARD_IS_LOCKED) {
				SDOAM_Printf("HANDLING:card locked\n");
				ret = SDM_CARD_LOCKED;
				break;
			} else if (status & COM_CRC_ERROR) {
				SDOAM_Printf("HANDLING:previous cmd CRC er\n");
				ret = SDM_SUCCESS;
				break;
			} else {
				SDOAM_Printf("HANDLING:other er %x\n", status);
				ret = SDM_FALSE;
				break;
			}
		} else if (ret == SDC_RESP_TIMEOUT) {
			SDOAM_Printf("HANDLING:read status timeout\n");
			//if (TRUE == SDC_IsCardPresence(cardId)) {
				ret = SDM_RESP_TIMEOUT;
				break;
			/*} else {
				ret = SDM_CARD_NOTPRESENT;
				break;
			}*/
		} else if ((ret == SDC_RESP_CRC_ERROR) || (ret == SDC_RESP_ERROR)) {
			SDOAM_Printf("HANDLING:read status Rsp er or CRC er, Resend Read status\n");
			repeatCount++;
			continue;
		} else {
			SDOAM_Printf("HANDLING:read status other errror %d\n", ret);
			/* 其他错误，直接返回 */
			break;
		}
	}
	if ((repeatCount == SDM_CMD_RESENT_COUNT) && (ret != SDM_SUCCESS)) {
		SDOAM_Printf("HANDLING:retry two times, but not success\n");
		ret = SDM_FALSE;
	}

	return ret;
}

/****************************************************************
* 函数名:_DataErrorHandle
* 描述:当读或写出现数据错误时，用于出错处理，主要发送STOP命令
* 参数说明:pCardInfo     输入参数  指向卡信息的指针
* 返回值:SDM_CARD_NOTPRESENT  卡不存在
*        SDM_RESP_TIMEOUT     卡存在，但却不作出回复
*        SDM_CARD_LOCKED      卡被锁住
*        SDM_SUCCESS          成功，可以重新发命令
*        SDM_FALSE            未知错误
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _DataErrorHandle(int32 cardId)
{
	uint32 status  = 0;
	int32  handleRet, ret = SDM_SUCCESS;

	ret = SDC_SendCommand(cardId,
			(SD_STOP_TRANSMISSION | SD_NODATA_OP | SD_RSP_R1B | STOP_CMD | NO_WAIT_PREV),
			0, &status);
	if (ret == SDC_RESP_TIMEOUT) {
		//PRINT_E("_DataErrorHandle STOP cmd timeout\n");
		handleRet = SDC_SendCommand(cardId,
				(SD_SEND_STATUS | SD_NODATA_OP | SD_RSP_R1 | NO_WAIT_PREV),
				(gSDMDriver[cardId].cardInfo.rca << 16),
				&status);
		if (handleRet == SDC_RESP_TIMEOUT) {
			PRINT_E("_DataErrorHandle SEND STATUS timeout\n");
			ret = SDC_RESP_TIMEOUT;
		} else {
			ret = SDM_SUCCESS;
		}
	} else if ((ret == SDC_SUCCESS) || (ret == SDC_RESP_ERROR) || (ret == SDC_RESP_CRC_ERROR)) {
		//SDOAM_Printf("HANDLING:Send STOP cmd not timeout, ReSend cmd\n");
		/* STOP command 成功或response错误，都认为是正确的重发 */
		ret = SDM_SUCCESS;
	}

	return ret;
}

/****************************************************************
* 函数名:_SDMMC_Read
* 描述:SD\MMC卡的读操作，读的最小单位是block(512字节)
* 参数说明:cardId     输入参数  需要操作的卡
*          dataAddr   输入参数  需要读取的起始block地址
*          blockCount 输入参数  需要连续读取多少个block
*          pBuf       输出参数  读到数据存放的buffer地址
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _SDMMC_Read(int32 cardId, uint32 dataAddr, uint32 blockCount, void *pBuf)
{
	int32            ret = SDM_SUCCESS;
	int32            handleRet = SDM_SUCCESS;
	uint32           repeatCount = 0;
	uint32           status = 0;
	uint32           bStopCmd = 0;

	while (repeatCount < SDM_CMD_RESENT_COUNT) {
		if (blockCount == 1) {
			ret = SDC_ReadBlockData(cardId,
					(SD_READ_SINGLE_BLOCK | SD_READ_OP | SD_RSP_R1 | WAIT_PREV),
					dataAddr,
					&status,
					(blockCount << 9),
					pBuf);
		} else {
			bStopCmd = 1;
			if (gSDMDriver[cardId].cardInfo.type & eMMC2G && gSDMDriver[cardId].cardInfo.bootSize) {
				ret = SDC_BusRequest(cardId,
						(SD_SET_BLOCK_COUNT | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
						blockCount, &status, 0, 0, NULL);
				if (ret == SDC_SUCCESS)
					bStopCmd = 0;
			}
			ret = SDC_ReadBlockData(cardId,
					(SD_READ_MULTIPLE_BLOCK | SD_READ_OP | SD_RSP_R1 | WAIT_PREV),
					dataAddr,
					&status,
					(blockCount << 9),
					pBuf);
			if ((bStopCmd == 1) && (ret == SDC_SUCCESS)) {
				/*
				 * hcy 09-06-12发现CMMB的变态大卡，在多块读完后发送STOP命令，
				 * 如果这边没有延时，也就是如果STOP跟数据结束相隔太短，卡工作会不正常，
				 * 表现出来的现象是:读写都没报错，只是读到的数据不是我们想要的数据。
				 * 原本想读大卡CA部分第4扇区的数据，结果读到的是大卡内自带存储
				 * 空间内的第4个扇区开始的数据，而且存储区的第4扇区会被修改成全0，后面的扇区不会。
				 */
				//SDOAM_Delay(200);
				handleRet = SDC_SendCommand(cardId,
						(SD_STOP_TRANSMISSION | SD_NODATA_OP | SD_RSP_R1B | STOP_CMD | NO_WAIT_PREV),
						0, &status);
				if (handleRet != SDC_SUCCESS)
					eMMC_printk(1, "HANDLING:Send STOP cmd err:0x%x\n", handleRet);
			}
		}

		if (ret == SDC_SUCCESS) {
			break;
		} else if ((ret == SDC_START_BIT_ERROR)
				|| (ret == SDC_END_BIT_ERROR)
				|| (ret == SDC_DATA_READ_TIMEOUT)
				|| (ret == SDC_DATA_CRC_ERROR)) {
			handleRet = _DataErrorHandle(cardId);
			if (handleRet != SDC_SUCCESS) {
				PRINT_E("_DataErrorHandle handleRet = 0x%x\n", handleRet);
				break;
			}

			if ((gSDMDriver[cardId].cardInfo.tran_speed - 4000) <= 0)
				break;

			/* 频率降4MHz */
			handleRet = SDC_UpdateCardFreq(cardId, gSDMDriver[cardId].cardInfo.tran_speed - 4000);
			if (handleRet != SDC_SUCCESS) {
				PRINT_E("SDC_UpdateCardFreq handleRet = 0x%x\n", handleRet);
				break;
			}
			repeatCount++;
			gSDMDriver[cardId].cardInfo.tran_speed -= 4000;
			continue;
		} else if (ret == SDC_RESP_TIMEOUT) {
			eMMC_printk(1, "HANDLING:Read cmd Rsp timeout\n", "");
			handleRet = _ResponseTimeoutHandle(cardId);
			if (handleRet == SDM_SUCCESS) {
				eMMC_printk(1, "HANDLING:Rsp timeout handle success, ReSend Read cmd\n", "");
				/* 重发一次 */
				repeatCount++;
				continue;
			} else {
				break;
			}
		} else if ((ret == SDC_RESP_CRC_ERROR) || (ret == SDC_RESP_ERROR)) {
			/* 回复出错不管，只要数据不出错都没问题 */
			handleRet = _DataErrorHandle(cardId);
			ret = SDM_SUCCESS;
			break;
		} else {
			/* 其他错误，直接返回 */
			handleRet = _DataErrorHandle(cardId);
			break;
		}
	}

	return ret;
}

/****************************************************************
* 函数名:_SDMMC_Write
* 描述:SD\MMC卡的写操作，写的最小单位是block(512字节)
* 参数说明:cardId     输入参数  需要操作的卡
*          dataAddr   输入参数  需要写入的起始block地址
*          blockCount 输入参数  需要连续写入多少个block
*          pBuf       输入参数  需要写入的数据存放的buffer地址
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _SDMMC_Write(int32 cardId, uint32 dataAddr, uint32 blockCount, void *pBuf)
{
	int32            ret = SDM_SUCCESS;
	int32            handleRet = SDM_SUCCESS;
	uint32           status  = 0;
	uint32           repeatCount = 0;

	while (repeatCount < SDM_CMD_RESENT_COUNT) {
		if (blockCount == 1) {
			ret = SDC_WriteBlockData(cardId,
				(SD_WRITE_BLOCK | SD_WRITE_OP | SD_RSP_R1 | WAIT_PREV),
				dataAddr,
				&status,
				(blockCount << 9),
				pBuf);
		} else {
			ret = SDC_WriteBlockData(cardId,
				(SD_WRITE_MULTIPLE_BLOCK | SD_WRITE_OP | SD_RSP_R1 | WAIT_PREV),
				dataAddr,
				&status,
				(blockCount << 9),
				pBuf);
			if (ret == SDC_SUCCESS) {
				ret = SDC_SendCommand(cardId,
					(SD_STOP_TRANSMISSION | SD_NODATA_OP | SD_RSP_R1B | STOP_CMD | NO_WAIT_PREV),
					0, &status);
				if (ret == SDC_RESP_TIMEOUT) {
					SDOAM_Printf("HANDLING:Send STOP cmd timeout\n");
					//if (TRUE == SDC_IsCardPresence(cardId)) {
						SDOAM_Printf("HANDLING:card presence but timeout\n");
						ret = SDC_RESP_TIMEOUT;
						break;
					/*} else {
						SDOAM_Printf("HANDLING:card not presence\n");
						ret = SDM_CARD_NOTPRESENT;
						break;
					}*/
				} else {
					break;
				}
			}
		}

		if (ret == SDC_SUCCESS) {
			break;
		} else if (ret == SDC_DATA_CRC_ERROR) {
			SDOAM_Printf("HANDLING:DATA CRC ER, DataEr handle\n", ret);
			handleRet = _DataErrorHandle(cardId);
			if (handleRet == SDM_SUCCESS) {
				SDOAM_Printf("HANDLING:DataEr handle success, ReSend Write cmd\n");
				repeatCount++;
				/* 频率降2MHz */
				handleRet = SDC_UpdateCardFreq(cardId,
						(gSDMDriver[cardId].cardInfo.tran_speed - 2000));
				if (handleRet != SDC_SUCCESS)
					break;
				continue;
			} else {
				/* 其他错误，直接返回 */
				break;
			}
		} else if (ret == SDC_END_BIT_ERROR) {
			SDOAM_Printf("HANDLING:END BIT ERRPR\n");
			//if (TRUE == SDC_IsCardPresence(cardId)) {
				SDOAM_Printf("HANDLING:card presence, DataEr handle\n");
				handleRet = _DataErrorHandle(cardId);
				if (handleRet == SDM_SUCCESS) {
					SDOAM_Printf("HANDLING:DataEr handle success, ReSend Write cmd\n");
					repeatCount++;
					/* 频率降2MHz */
					handleRet = SDC_UpdateCardFreq(cardId,
						(gSDMDriver[cardId].cardInfo.tran_speed - 2000));
					if (handleRet != SDC_SUCCESS)
						break;
					continue;
				} else {
					/* 其他错误，直接返回 */
					break;
				}
			/*} else {
				SDOAM_Printf("HANDLING:card not presence\n");
				ret = SDM_CARD_NOTPRESENT;
				break;
			}*/
		} else if (ret == SDC_RESP_TIMEOUT) {
			SDOAM_Printf("HANDLING:Write cmd Rsp timeout\n");
			handleRet = _ResponseTimeoutHandle(cardId);
			if (handleRet == SDM_SUCCESS) {
				SDOAM_Printf("HANDLING:Rsp timeout handle success, ReSend Write cmd\n");
				/* 重发一次 */
				repeatCount++;
				continue;
			} else {
				break;
			}
		} else if ((ret == SDC_RESP_CRC_ERROR) || (ret == SDC_RESP_ERROR)) {
			/* 回复出错不管，只要数据不出错都没问题 */
			ret = SDM_SUCCESS;
			break;
		} else {
			/* 其他错误，直接返回 */
			break;
		}
	}
	if ((repeatCount == SDM_CMD_RESENT_COUNT) && (ret != SDM_SUCCESS))
		ret = SDM_FALSE;

	if (handleRet == SDM_SUCCESS)
		return ret;
	else
		return handleRet;
}

/****************************************************************
* 函数名:_RegisterFunction
* 描述:根据不同卡类型注册不同的函数
* 参数说明:pCardInfo     输入参数  指向卡信息的指针
* 返回值:
* 相关全局变量:
* 注意:这个函数只被IdentifyCard调用
****************************************************************/
static void _RegisterFunction(pSDM_CARD_INFO_T pCardInfo)
{
	pCardInfo->fun.read  = _SDMMC_Read;
	pCardInfo->fun.write = _SDMMC_Write;
}

/****************************************************************
* 函数名:_AccessBootPartition
* 描述:读写boot partition或者user area
* 参数说明:cardId     输入参数  具体操作的卡
*          enable     输入参数  使能
*          partition  输入参数  具体操作哪个boot partition
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _AccessBootPartition(int32 cardId, uint32 partition)
{
	pSDM_CARD_INFO_T pCardInfo = NULL;
	uint32           port = SDM_MAX_MANAGER_PORT;

	if (!_IsCardRegistered(cardId, &port))
		return SDM_PARAM_ERROR;

	pCardInfo = &gSDMDriver[port].cardInfo;
	return MMC_AccessBootPartition(pCardInfo, partition);
}


/****************************************************************
* 函数名:_SetBootWidth
* 描述:设置启动模式的线宽，以及复位后是否保持该线宽
* 参数说明:cardId     输入参数  具体操作的卡
*          enable     输入参数  保持线宽设置
*          width      输入参数  boot模式下的线宽
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _SetBootWidth(int32 cardId, uint32 enable, uint8 width)
{
	pSDM_CARD_INFO_T pCardInfo = NULL;
	uint32           port = SDM_MAX_MANAGER_PORT;
	HOST_BUS_WIDTH_E bootBusWidth = (HOST_BUS_WIDTH_E)width;

	if (!_IsCardRegistered(cardId, &port))
		return SDM_PARAM_ERROR;

	pCardInfo = &gSDMDriver[port].cardInfo;
	return MMC_SetBootBusWidth(pCardInfo, enable, bootBusWidth);
}


/****************************************************************
* 函数名:_SwitchBoot
* 描述:切换boot partition或者user area
* 参数说明:cardId     输入参数  具体操作的卡
*          enable     输入参数  使能
*          partition  输入参数  具体操作哪个boot partition
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _SwitchBoot(int32 cardId, uint32 enable, uint32 partition)
{
	pSDM_CARD_INFO_T pCardInfo = NULL;
	uint32           port = SDM_MAX_MANAGER_PORT;

	if (!_IsCardRegistered(cardId, &port))
		return SDM_PARAM_ERROR;

	pCardInfo = &gSDMDriver[port].cardInfo;
	return MMC_SwitchBoot(pCardInfo, enable, partition);
}

/****************************************************************
* 函数名:_IdentifyCard
* 描述:识别cardId指定的卡
* 参数说明:cardId     输入参数  需要识别的卡
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _IdentifyCard(int32 cardId)
{
	SDM_CARD_INFO_T  cardInfo;
	uint32           status = 0;
	uint32           nf;
	uint32           mp;
	int32            ret = SDC_SUCCESS;

	SDOAM_Memset(&cardInfo, 0x00, sizeof(SDM_CARD_INFO_T));
	cardInfo.type = UNKNOW_CARD;
	cardInfo.cardId = cardId;
	#ifdef SDM_PROT_INFO_DEBUG
	gSDMDriver[cardId].step = 0;
	gSDMDriver[cardId].error = 0;
	#endif
	/* reset all card */
	do {
		ret = SDC_ControlPower(cardId, TRUE);
		if (SDC_SUCCESS != ret) {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[cardId].step = 0x11;
			gSDMDriver[cardId].error = ret;
			#endif
			break;
		}
		ret = SDC_SetHostBusWidth(cardId, BUS_WIDTH_1_BIT);
		if (SDC_SUCCESS != ret) {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[cardId].step = 0x12;
			gSDMDriver[cardId].error = ret;
			#endif
			break;
		}
		ret = SDC_UpdateCardFreq(cardId, FOD_FREQ);
		if (SDC_SUCCESS != ret) {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[cardId].step = 0x13;
			gSDMDriver[cardId].error = ret;
			#endif
			break;
		}
		SDOAM_Delay(500);  /* 等待电源和时钟稳定 */
		ret = _Identify_SendCmd(cardId,
			(SD_GO_IDLE_STATE | SD_NODATA_OP | SD_RSP_NONE | NO_WAIT_PREV | SEND_INIT),
			0, NULL, 0, 0, NULL);
		if (SDC_SUCCESS != ret) {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[cardId].step = 0x14;
			gSDMDriver[cardId].error = ret;
			#endif
			break;
		}
		SDOAM_Delay(100); /* 27有发现CMD0发送完以后延时一下再发其他命令能提高卡的识别率 */

		/**************************************************
					卡识别
		**************************************************/
#if (SD_CARD_Support)
		ret = _Identify_SendCmd(cardId,
			(SD2_SEND_IF_COND | SD_NODATA_OP | SD_RSP_R7 | WAIT_PREV),
			0x1AA, &status, 0, 0, NULL);
		if (SDC_SUCCESS == ret) {
			/* SDIO-only Card or SDIO-SDHC/SD2.0 Combo Card
				or SDIO-SDHC/SD2.0 only Card or SD2.0 or SDHC */
			nf = 0;
			mp = 0;
			ret = _Identify_SendCmd(cardId,
				(SDIO_IO_SEND_OP_COND | SD_NODATA_OP | SD_RSP_R4 | WAIT_PREV),
				0, &status, 0, 0, NULL);
			if (SDC_SUCCESS == ret) {
				nf = (status >> 28) & 0x7;
				mp = (status >> 27) & 0x1;
				if ((mp == 1) && (nf > 0) && (status & 0xFFFF00)) {
					/* SDIO-SDHC/SD2.0 Combo Card */
					//SDIOHC_SD20_ComboInit(&cardInfo);
					cardInfo.type = UNKNOW_CARD;
				} else if ((mp == 0) && (nf > 0) && (status & 0xFFFF00)) {
					/* SDIO-only Card */
					//SDIO_OnlyInit(&cardInfo);
					cardInfo.type = UNKNOW_CARD;
				} else if (mp == 1) {
					/* SDIO-SDHC/SD2.0 only Card */
					SD20_Init(&cardInfo);
				} else {
					/* unknow card */
				}
			} else if (ret == SDC_RESP_TIMEOUT) {
				/* SD2.0 or SDHC */
				SD20_Init(&cardInfo);
			} else {
				/* must be error occured */
				#ifdef SDM_PROT_INFO_DEBUG
				gSDMDriver[cardId].step = 0x14;
				gSDMDriver[cardId].error = ret;
				#endif
			}
		} else if (SDC_RESP_TIMEOUT == ret) {
			/* SDIO-only Card or SDIO-SD1.X Combo Card or SDIO-SD1.X only Card
				or SD1.X or MMC or SD2.0 or later with voltage mismatch */
			nf = 0;
			mp = 0;
			ret = _Identify_SendCmd(cardId,
				(SDIO_IO_SEND_OP_COND | SD_NODATA_OP | SD_RSP_R4 | WAIT_PREV),
				0, &status, 0, 0, NULL);
			if (SDC_SUCCESS == ret) {
				nf = (status >> 28) & 0x7;
				mp = (status >> 27) & 0x1;
				if ((mp == 1) && (nf > 0) && (status & 0xFFFF00)) {
					/* SDIO-SD1.X Combo Card */
					//SDIO_SD1X_ComboInit(&cardInfo);
					cardInfo.type = UNKNOW_CARD;
				} else if ((mp == 0) && (nf > 0) && (status & 0xFFFF00)) {
					/* SDIO-only Card */
					//SDIO_OnlyInit(&cardInfo);
					cardInfo.type = UNKNOW_CARD;
				} else if (mp == 1) {
					/* SDIO-SD1.X only Card */
					SD1X_Init(&cardInfo);
				} else {
					/* unknow card */
				}
			} else if (ret == SDC_RESP_TIMEOUT) {
				/* SD1.X or MMC or SD2.0 or later with voltage mismatch */
				ret = _Identify_SendCmd(cardId,
					(SD_APP_CMD | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
					0, &status, 0, 0, NULL);
				if (SDC_SUCCESS == ret) {
					/* SD1.X or SD2.0 or later with voltage mismatch */
					SD1X_Init(&cardInfo);
				} else if (SDC_RESP_TIMEOUT == ret) {
#endif
					/* must be MMC */
					MMC_Init(&cardInfo);
#if (SD_CARD_Support)
				} else {
					/* must be error occured */
					#ifdef SDM_PROT_INFO_DEBUG
					gSDMDriver[cardId].step = 0x15;
					gSDMDriver[cardId].error = ret;
					#endif
				}
			} else {
				/* must be error occured */
				#ifdef SDM_PROT_INFO_DEBUG
				gSDMDriver[cardId].step = 0x16;
				gSDMDriver[cardId].error = ret;
				#endif
			}
		} else {
			/* must be error occured */
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[cardId].step = 0x17;
			gSDMDriver[cardId].error = ret;
			#endif
		}
#endif
	} while (0);

	if (cardInfo.type == UNKNOW_CARD) {
		SDC_ResetController(cardId);
		//SDC_SetHostBusWidth(cardId, BUS_WIDTH_1_BIT);
		//SDC_ControlClock(cardId, FALSE);
		//SDC_ControlPower(cardId, FALSE);
		PRINTF("ERROR:Card Identify Failed %x\n", ret);
		return ret;
	} else {
		_RegisterFunction(&cardInfo);
		_RegisterCard(&cardInfo);
#if 0
		PRINT_I("MSG:Card Identify SUCCESS\n", "");
		if (cardInfo.type & SDIO) {
			if (cardInfo.type & (SDHC | SD20 | SD1X))
				PRINT_I("MSG:SDIO Combo Card\n", "");
			else
				PRINT_I("MSG:SDIO only Card\n", "");
		} else {
			if (cardInfo.type & SDHC)
				PRINT_I("MSG:SDHC Card\n", "");
			if (cardInfo.type & SD20)
				PRINT_I("MSG:SD2.0 Card\n", "");
			if (cardInfo.type & SD1X)
				PRINT_I("MSG:SD1.x Card\n", "");
			if (cardInfo.type & MMC4)
				PRINT_I("MSG:MMC4 Card\n", "");
			if (cardInfo.type & MMC)
				PRINT_I("MSG:MMC Card\n", "");
			if (cardInfo.type & eMMC2G)
				PRINT_I("MSG:eMMC2G Card\n", "");
		}
		PRINT_I("MSG:Manufacture Data:%d.%d\n", cardInfo.year, cardInfo.month);
		if (cardInfo.workMode & SDM_WIDE_BUS_MODE)
			PRINT_I("MSG:Use Wide bus mode\n", "");
		if (cardInfo.workMode & SDM_HIGH_SPEED_MODE)
			PRINT_I("MSG:Use High speed mode\n", "");
#endif
		return SDM_SUCCESS;
	}
}

/****************************************************************
* 函数名:_GenerateRCA   (RCA:Relative Card Address)
* 描述:生成一个新的RCA，保证这个RCA与已有的RCA不会重复
*      实现方法是，扫描所有的RCA，取出最大值来加1，得到新的RCA
* 参数说明:
* 返回值:返回生成的新RCA
* 相关全局变量:读取gSDMDriver[i].cardInfo.rca
* 注意:只有MMC卡要用到这个函数，SD卡的RCA是由卡自动生成的。
****************************************************************/
uint16 _GenerateRCA(void)
{
	uint16 max = 2; /* rca = 0001是MMC上电后初始化时使用的默认地址，所以从2开始 */
	uint32 i;

	for (i = 0; i < SDM_MAX_MANAGER_PORT; i++)
		if (gSDMDriver[i].cardInfo.rca > max)
			max = gSDMDriver[i].cardInfo.rca;

	return max + 1; /* 不知道会不会溢出 */
}

/****************************************************************
* 函数名:_IsRCAUsable
* 描述:判断当前给出的RCA是否可用，如果与已有的卡RCA冲突，则不可用
*      如果没有冲突，则可用
* 参数说明:rca   输入参数    需要检查的RCA
* 返回值:TRUE     可用
*        FALSE    不可用
* 相关全局变量:读取gSDMDriver[i].cardInfo.rca
* 注意:
****************************************************************/
uint32 _IsRCAUsable(uint16 rca)
{
	uint32 i;

	for (i = 0; i < SDM_MAX_MANAGER_PORT; i++)
		if (gSDMDriver[i].cardInfo.rca == rca)
			return FALSE;

	return TRUE;
}

/****************************************************************
* 函数名:_Identify_SendCmd
* 描述:发送命令
* 参数说明:
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 _Identify_SendCmd(int32 cardId,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf)
{
	int32 ret = SDM_SUCCESS;
	int32 retry = SDM_CMD_ERROR_RETRY_COUNT;

	do {
		ret = SDC_BusRequest(cardId, cmd, cmdArg, responseBuf, blockSize, dataLen, pDataBuf);
		retry--;
	} while ((ret & (SDC_RESP_ERROR | SDC_RESP_CRC_ERROR | SDC_RESP_TIMEOUT)) && (retry > 0));

	return ret;
}

#if (SD_CARD_Support)
/****************************************************************
* 函数名:_Identify_SendAppCmd
* 描述:发送application命令
* 参数说明:
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 _Identify_SendAppCmd(int32 cardId,
			uint16 rca,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf)
{
	int32  ret = SDM_SUCCESS;
	uint32 status = 0;
	int32  retry = SDM_CMD_ERROR_RETRY_COUNT;

	do {
		ret = SDC_BusRequest(cardId,
			(SD_APP_CMD | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
			(rca << 16),
			&status,
			0,
			0,
			NULL);
		if ((ret == SDC_SUCCESS) && (status & 0x20))
			ret = SDC_BusRequest(cardId, cmd, cmdArg, responseBuf, blockSize, dataLen, pDataBuf);
		retry--;
	} while ((ret & (SDC_RESP_ERROR | SDC_RESP_CRC_ERROR | SDC_RESP_TIMEOUT)) && (retry > 0));

	return ret;
}
#endif

/****************************************************************
* 函数名:SDM_Init
* 描述:SDM模块初始化
* 参数说明:
* 返回值:
* 相关全局变量:写gSDMDriver[i]
* 注意:
****************************************************************/
void SDM_Init(uint32 CardId)
{
	/* mutex的创建过程已经在RockCreateSems函数中做了 */
	gSDMDriver[CardId].mutex  = SDOAM_CreateMutex();
	gSDMDriver[CardId].bOpen  = FALSE;
	SDOAM_Memset(&gSDMDriver[CardId].cardInfo, 0x00, sizeof(SDM_CARD_INFO_T));
	gSDMDriver[CardId].cardInfo.cardId = SDM_INVALID_CARDID;

	SDC_Init(CardId);
}

/****************************************************************
* 函数名:SDM_Open
* 描述:开启cardId指定的卡
* 参数说明:cardId     输入参数  需要打开的卡
* 返回值:返回句柄
* 相关全局变量:读写gSDMDriver[i].bOpen
* 注意:
****************************************************************/
int32 SDM_Open(int32 cardId)
{
	uint32 port = SDM_MAX_MANAGER_PORT;

	if ((!SDC_IsCardIdValid(cardId)) || (!_IsCardRegistered(cardId, &port)))
		return SDM_PARAM_ERROR;

	if (gSDMDriver[port].bOpen)
		return SDM_SUCCESS;

	gSDMDriver[port].bOpen = TRUE;
	return SDM_SUCCESS;
}

/****************************************************************
* 函数名:SDM_Close
* 描述:关闭cardId指定的卡
* 参数说明:cardId     输入参数  需要关闭的卡
* 返回值:
* 相关全局变量:读写gSDMDriver[i].bOpen
* 注意:
****************************************************************/
int32 SDM_Close(int32 cardId)
{
	return SDM_SUCCESS;
}

/****************************************************************
* 函数名:SDM_Read
* 描述:对cardId指定的卡进行读操作，读的最小单位是block(512字节)
* 参数说明:cardId     输入参数  需要操作的卡
*          blockNum   输入参数  需要读取的起始block号
*          blockCount 输入参数  需要连续读取多少个block
*          pBuf       输出参数  读到数据存放的buffer地址，要求地址4字节对齐
* 返回值:
* 相关全局变量:读gSDMDriver[i]
* 注意:pBuf地址要求地址4字节对齐
****************************************************************/
int32 SDM_Read(int32 cardId, uint32 blockNum, uint32 blockCount, void *pBuf)
{
	SDM_PORT_INFO_T *pSDMDriver;
	uint32           port;
	int32            ret = SDM_SUCCESS;
	uint32           mul;

	Assert((blockCount != 0), "SDM_Read:read count = 0\n", blockCount);
	if (blockCount == 0)
		return SDM_PARAM_ERROR;

	if (SDC2 == cardId) {
		port = SDC2;
	} else {
		if ((!SDC_IsCardIdValid(cardId)) || (!_IsCardRegistered(cardId, &port)))
			return SDM_PARAM_ERROR;
	}

	pSDMDriver = &gSDMDriver[port];
	if (((blockNum + blockCount) > (pSDMDriver->cardInfo.capability)))
		return SDM_PARAM_ERROR;

	if ((pSDMDriver->cardInfo.type) & (SDHC | eMMC2G))
		mul = 0; /* SDHC地址是以block(512)为单位的，而早期协议地址是以byte为单位 */
	else
		mul = 9;

	SDOAM_RequestMutex(pSDMDriver->mutex);
	if (pSDMDriver->bOpen) {
#if EN_SDC_INTERAL_DMA
		int i;
		int mod;
		char *pu8buf = pBuf;

		mod = (MAX_DATA_SIZE_IDMAC >> 9);
		for (i = 0; i < blockCount; i += mod) {
			if (blockCount - i < mod)
				mod = blockCount - i;
			ret = (pSDMDriver->cardInfo.fun.read)(cardId, ((blockNum + i) << mul), mod, pu8buf + i * 512);
			if (ret != SDM_SUCCESS) {
				SDM_Close(cardId);
				break;
			}
		}
#else
		/* 地址4字节对齐 */
		ret = (pSDMDriver->cardInfo.fun.read)(cardId, (blockNum << mul), blockCount, pBuf);
#endif
	} else {
		ret = SDM_CARD_CLOSED;
	}
	if (ret != SDM_SUCCESS) {
		PRINT_E("SDM_Read error = 0x%x ret = 0x%x\n", blockNum, ret);
		//SDM_Close(cardId);
	}
	SDOAM_ReleaseMutex(pSDMDriver->mutex);
	return ret;
}

/****************************************************************
* 函数名:SDM_Write
* 描述:对cardId指定的卡进行写操作，写的最小单位是block(512字节)
* 参数说明:cardId     输入参数  需要操作的卡
*          blockNum   输入参数  需要写入的起始block号
*          blockCount 输入参数  需要连续写入多少个block
*          pBuf       输入参数  需要写入的数据存放的buffer地址，要求地址4字节对齐
* 返回值:
* 相关全局变量:读gSDMDriver[i]
* 注意:pBuf地址要求地址4字节对齐
****************************************************************/
int32 SDM_Write(int32 cardId, uint32 blockNum, uint32 blockCount, void *pBuf)
{
	SDM_PORT_INFO_T *pSDMDriver;
	uint32           port;
	int32            ret = SDM_SUCCESS;
	uint32           mul;

	Assert((blockCount != 0), "SDM_Write:read count = 0\n", blockCount);
	if (blockCount == 0)
		return SDM_PARAM_ERROR;

	if (SDC2 == cardId) {
		port = SDC2;
	} else {
		if ((!SDC_IsCardIdValid(cardId)) || (!_IsCardRegistered(cardId, &port)))
			return SDM_PARAM_ERROR;
	}

	pSDMDriver = &gSDMDriver[port];
	if (((blockNum + blockCount) > (pSDMDriver->cardInfo.capability)))
		return SDM_PARAM_ERROR;

	if (pSDMDriver->cardInfo.WriteProt) /* 只有SD卡才有写保护,MMC卡这个值总是为0 */
		return SDM_CARD_WRITE_PROT;

	if ((pSDMDriver->cardInfo.type) & (SDHC | eMMC2G))
		mul = 0; /* SDHC地址是以block(512)为单位的，而早期协议地址是以byte为单位 */
	else
		mul = 9;

	SDOAM_RequestMutex(pSDMDriver->mutex);
	if (pSDMDriver->bOpen) {
#if EN_SDC_INTERAL_DMA
		int i;
		int mod;
		char *pu8buf = pBuf;

		mod = (MAX_DATA_SIZE_IDMAC >> 9);
		for (i = 0; i < blockCount; i += mod) {
			if (blockCount - i < mod)
				mod = blockCount - i;
			ret = (pSDMDriver->cardInfo.fun.write)(cardId, ((blockNum + i) << mul), mod, pu8buf + i * 512);
			if (ret != SDM_SUCCESS) {
				SDM_Close(cardId);
				break;
			}
		}
#else
		ret = (pSDMDriver->cardInfo.fun.write)(cardId, (blockNum << mul), blockCount, pBuf);
#endif
	} else {
		ret = SDM_CARD_CLOSED;
	}
	if (ret != SDM_SUCCESS) {
		PRINT_E("SDM_Write error = 0x%x ret = 0x%x\n", blockNum, ret);
		//SDM_Close(cardId);
	}
	SDOAM_ReleaseMutex(pSDMDriver->mutex);

	return ret;
}

/****************************************************************
* 函数名:SDM_IOCtrl
* 描述:IO控制函数
* 参数说明:cmd     输入参数  操作命令数
*          param   输入参数  操作参数
* 返回值:
* 相关全局变量:读gSDMDriver[i]
* 注意:
****************************************************************/
int32 SDM_IOCtrl(uint32 cmd, void *param)
{
	uint32           port = SDM_MAX_MANAGER_PORT;
	int32            ret = SDM_SUCCESS;
	uint32          *pTmp = NULL;
	int32            cardId = SDM_INVALID_CARDID;

	pTmp = (uint32 *)param;
	cardId = (int32)pTmp[0];

	if (!(cmd == SDM_IOCTRL_REGISTER_CARD)) {
		if ((!SDC_IsCardIdValid(cardId)) || (!_IsCardRegistered(cardId, &port))) {
			if (cmd == SDM_IOCTR_IS_CARD_READY)
				pTmp[1] = FALSE;
			else if (cmd == SDM_IOCTR_GET_CAPABILITY)
				pTmp[1] = 0;
			else if (cmd == SDM_IOCTR_GET_PSN)
				pTmp[1] = (uint32)(unsigned long)NULL;

			return SDM_PARAM_ERROR;
		}
	} else {
		if (!SDC_IsCardIdValid(cardId))
			return SDM_PARAM_ERROR;
	}

	switch (cmd) {
	case SDM_IOCTRL_REGISTER_CARD:
		ret = _IdentifyCard(cardId);
		break;
	case SDM_IOCTRL_UNREGISTER_CARD:
		/* hcy 09-09-22 等前面的读写完成，否则直接进去关控制器会使得前面的读写while在控制器驱动中 */
		//SDOAM_RequestMutex(gSDMDriver[port].mutex);
		//_UnRegisterCard(cardId);
		//SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
#if (0/* SD_CARD_Support */)
	case SDM_IOCTRL_SET_PASSWORD:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		if (gSDMDriver[port].bOpen) {
			ret = _SetPassword(cardId, (uint8 *)pTmp[1], (uint8 *)pTmp[2], (uint32)pTmp[3]);
			if (ret == SDM_CARD_NOTPRESENT)
				SDM_Close(cardId);
		} else {
			ret = SDM_CARD_CLOSED;
		}
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTRL_CLEAR_PASSWORD:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		if (gSDMDriver[port].bOpen) {
			ret = _ClearPassword(cardId, (uint8 *)pTmp[1]);
			if (ret == SDM_CARD_NOTPRESENT)
				SDM_Close(cardId);
		} else {
			ret = SDM_CARD_CLOSED;
		}
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTRL_FORCE_ERASE_PASSWORD:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		if (gSDMDriver[port].bOpen) {
			ret = _ForceErasePassword(cardId);
			if (ret == SDM_CARD_NOTPRESENT)
				SDM_Close(cardId);
		} else {
			ret = SDM_CARD_CLOSED;
		}
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTRL_LOCK_CARD:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		if (gSDMDriver[port].bOpen) {
			ret = _LockCard(cardId, (uint8 *)pTmp[1]);
			if (ret == SDM_CARD_NOTPRESENT)
				SDM_Close(cardId);
		} else {
			ret = SDM_CARD_CLOSED;
		}
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTRL_UNLOCK_CARD:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		if (gSDMDriver[port].bOpen) {
			ret = _UnLockCard(cardId, (uint8 *)pTmp[1]);
			if (ret == SDM_CARD_NOTPRESENT)
				SDM_Close(cardId);
		} else {
			ret = SDM_CARD_CLOSED;
		}
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTR_FLUSH:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		if (gSDMDriver[port].bOpen)
			ret = SDC_WaitCardBusy(cardId);
		else
			ret = SDM_CARD_CLOSED;
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
#endif
	case SDM_IOCTR_GET_CAPABILITY:
		if (gSDMDriver[port].bOpen) {
			pTmp[1] = gSDMDriver[port].cardInfo.capability;
		} else {
			pTmp[1] = 0;
			ret = SDM_CARD_CLOSED;
		}
		break;
	case SDM_IOCTR_GET_PSN:
		if (gSDMDriver[port].bOpen) {
			pTmp[1] = gSDMDriver[port].cardInfo.psn;
		} else {
			pTmp[1] = (uint32)(unsigned long)NULL;
			ret = SDM_CARD_CLOSED;
		}
		break;
	case SDM_IOCTR_IS_CARD_READY:
		if (gSDMDriver[port].bOpen) {
			pTmp[1] = TRUE;
		} else {
			pTmp[1] = FALSE;
			ret = SDM_CARD_CLOSED;
		}
		break;
	case SDM_IOCTR_GET_BOOT_CAPABILITY:
		if (gSDMDriver[port].bOpen) {
			pTmp[1] = gSDMDriver[port].cardInfo.bootSize;
		} else {
			pTmp[1] = 0;
			ret = SDM_CARD_CLOSED;
		}
		break;
	case SDM_IOCTR_INIT_BOOT_PARTITION:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		ret = _SwitchBoot(cardId, 1, pTmp[1]);
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTR_DEINIT_BOOT_PARTITION:
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		ret = _SwitchBoot(cardId, 0, pTmp[1]);
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTR_SET_BOOT_BUSWIDTH: /* 设置boot模式下的线宽 */
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		ret = _SetBootWidth(cardId, pTmp[1], pTmp[2]);
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	case SDM_IOCTR_ACCESS_BOOT_PARTITION: /* 选择读写的区域; 0--用户区；1--boot1； 2--boot2; */
		SDOAM_RequestMutex(gSDMDriver[port].mutex);
		ret = _AccessBootPartition(cardId, pTmp[1]);
		SDOAM_ReleaseMutex(gSDMDriver[port].mutex);
		break;
	default:
		ret = SDM_PARAM_ERROR;
		break;
	}

	return ret;
}

/****************************************************************
* 函数名:SDM_SendCmd
* 描述:供外面应用调用的发送命令，SDM内部发送命令不使用这个接口
* 参数说明:
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDM_SendCmd(int32 cardId,
		uint32 cmd,
		uint32 cmdArg,
		uint32 *responseBuf,
		uint32  blockSize,
		uint32  dataLen,
		void   *pDataBuf)
{
	int32    ret = SDM_SUCCESS;
	uint32   port = SDM_MAX_MANAGER_PORT;

	if ((!SDC_IsCardIdValid(cardId)) || (!_IsCardRegistered(cardId, &port)))
		return SDM_PARAM_ERROR;

	SDOAM_RequestMutex(gSDMDriver[port].mutex);
	if (gSDMDriver[port].bOpen) {
		/* 地址4字节对齐 */
		ret = SDC_BusRequest(cardId, cmd, cmdArg, responseBuf, blockSize, dataLen, pDataBuf);
		if (ret != SDM_SUCCESS)
			SDM_Close(cardId);
	} else {
		ret = SDM_CARD_CLOSED;
	}
	SDOAM_ReleaseMutex(gSDMDriver[port].mutex);

	return ret;
}

#endif /* end of #ifdef DRIVERS_SDMMC */
