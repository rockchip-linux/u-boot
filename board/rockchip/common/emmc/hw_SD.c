/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "sdmmc_config.h"

#ifdef DRIVERS_SDMMC

#if (SD_CARD_Support)
/****************************************************************
* 函数名:SD_DecodeCID
* 描述:解析读到的CID寄存器，获取需要的信息
* 参数说明:pCID           输入参数  指向存放CID信息的指针
*          pCardInfo      输入参数  指向存放卡信息的指针
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static void _SD_DecodeCID(uint32 *pCID, pSDM_CARD_INFO_T pCard)
{
	pCard->year   = (uint16)(2000 + ((pCID[0] >> 12) & 0xFF)); /* [19:12] */
	pCard->month  = (uint8)((pCID[0] >> 8) & 0xF); /* [11:8] */
	pCard->psn    = ((pCID[1] & 0x00FFFFFF) << 8) | ((pCID[0] & 0xFF000000) >> 24); /* [55:24] */
	pCard->prv    = (uint8)((pCID[1] >> 24) & 0xFF); /* [63:56] */
	pCard->pnm[0] = (uint8)(pCID[3] & 0xFF); /* [103:64] */
	pCard->pnm[1] = (uint8)((pCID[2] >> 24) & 0xFF);
	pCard->pnm[2] = (uint8)((pCID[2] >> 16) & 0xFF);
	pCard->pnm[3] = (uint8)((pCID[2] >> 8) & 0xFF);
	pCard->pnm[4] = (uint8)(pCID[2] & 0xFF);
	pCard->pnm[5] = 0x0; /* 字符串结束符 */
	pCard->oid[0] = (uint8)((pCID[3] >> 16) & 0xFF); /* [119:104] */
	pCard->oid[1] = (uint8)((pCID[3] >> 8) & 0xFF);
	pCard->oid[2] = 0x0; /* 字符串结束符 */
	pCard->mid    = (uint8)((pCID[3] >> 24) & 0xFF); /* [127:120] */
}

/****************************************************************
* 函数名:SD_DecodeCSD
* 描述:解析读到的CSD寄存器，获取需要的信息
* 参数说明:pCSD           输入参数  指向存放CID信息的指针
*          pCardInfo      输入参数  指向存放卡信息的指针
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static void _SD_DecodeCSD(uint32 *pCSD, pSDM_CARD_INFO_T pCard)
{
	uint32           tmp = 0;
	uint32           c_size = 0;
	uint32           c_size_mult = 0;
	uint32           read_bl_len = 0;
#if 0
	uint32           taac = 0;
	uint32           nsac = 0;
	uint32           r2w_factor = 0;
#endif
	uint32           transfer_rate_unit[4] = {10, 100, 1000, 10000};
	uint32           time_value[16] = {10/*reserved*/, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};

	tmp = (pCSD[3] >> 30);
	if (tmp == 0) { /* CSD version 1.0 */
		c_size            = ((pCSD[2] & 0x3FF) << 2) | (pCSD[1] >> 30); /* [73:62] */
		c_size_mult       = (pCSD[1] >> 15) & 0x7;                      /* [49:47] */
		read_bl_len       = (pCSD[2] >> 16) & 0xF;                      /* [83:80] */
		pCard->capability = (((c_size + 1) * (0x1 << (c_size_mult + 2)) * (0x1 << read_bl_len)) >> 9);
	} else if (tmp == 1) { /* CSD version 2.0 */
		c_size            = (pCSD[1] >> 16) | ((pCSD[2] & 0x3F) << 16); /* [69:48] */
		pCard->capability = ((c_size + 1) << 10);
	} else {
		/* reserved */
	}
#if 0
	taac       = (pCSD[3] >> 16) & 0xFF; /* [119:112] */
	nsac       = (pCSD[3] >> 8) & 0xFF; /* [111:104] */
	r2w_factor = (0x1 << ((pCSD[0] >> 26) & 0x7)); /* [28:26] */
#endif
	pCard->tran_speed = transfer_rate_unit[pCSD[3] & 0x3] * time_value[(pCSD[3] >> 3) & 0x7]; /* [103:96] */
	pCard->dsr_imp    = (pCSD[2] >> 12) & 0x1;             /* [76] */
	pCard->ccc        = (uint16)((pCSD[2] >> 20) & 0xFFF); /* [95:84] */
}

/****************************************************************
* 函数名:SD_SwitchFunction
* 描述:读取SCR寄存器，根据卡是否支持宽数据线，改变数据线的宽度
*      以及根据卡是否支持高速模式，切换到高速模式
* 参数说明:pCardInfo     输入参数  指向卡信息的指针
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static void _SD_SwitchFunction(pSDM_CARD_INFO_T pCard)
{
	HOST_BUS_WIDTH_E wide = BUS_WIDTH_INVALID;
	uint32           data[(512 / (8 * 4))] __attribute__((aligned(ARCH_DMA_MINALIGN)));
	uint8           *pDataBuf = (uint8 *)data;
	uint8            tmp = 0;
	uint32           status = 0;
	int32            ret = SDM_SUCCESS;
	uint32           i = 0;

	/* read card SCR, get SD specification version and check whether wide bus supported */
	ret = SDC_SendCommand(pCard->cardId,
			(SD_APP_CMD | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
			(pCard->rca << 16), &status);
	if (SDC_SUCCESS != ret)
		return;

	SDOAM_Memset(data, 0x00, (64 >> 3));
	ret = SDC_BusRequest(pCard->cardId,
			(SDA_SEND_SCR | SD_READ_OP | SD_RSP_R1 | WAIT_PREV),
			0,
			&status,
			(64 >> 3),
			(64 >> 3),
			data);
	if (SDC_SUCCESS != ret)
		return;
	/* 我们SDMMC控制器接收到Data Packet Format为Wide Width Data的数据时，数据顺序会颠倒，
	   最高字节变成最低字节，最低字节变成最高字节，因此我们这边需要颠回来 */
	for (i = 0; i < (64 >> 4); i++) {
		tmp         = pDataBuf[i];
		pDataBuf[i] = pDataBuf[(64 >> 3) - 1 - i];
		pDataBuf[(64 >> 3) - 1 - i] = tmp;
	}

	switch (pDataBuf[7] & 0xF) { /* bit 59-56:SD specification version */
	case 0:
		pCard->specVer = SD_SPEC_VER_10;
		break;
	case 1:
		pCard->specVer = SD_SPEC_VER_11;
		break;
	case 2:
		pCard->specVer = SD_SPEC_VER_20;
		break;
	default:
		pCard->specVer = SPEC_VER_INVALID;
		break;
	}

	do {
		ret = SDC_GetHostBusWidth(pCard->cardId, &wide);
		if (SDC_SUCCESS != ret)
			break;
		Assert((wide != BUS_WIDTH_INVALID), "SD_SwitchFunction:Host bus width error\n", wide);
		if ((wide == BUS_WIDTH_INVALID) || (wide == BUS_WIDTH_MAX)) {
			ret = SDC_SDC_ERROR;
			break;
		}
		/* whether SDC iomux support wide bus and card internal support wide bus */
		if ((wide >= BUS_WIDTH_4_BIT) && (pDataBuf[6] & 0x4)) { /* bit 50:whether wide bus support */
			ret = SDC_SendCommand(pCard->cardId,
					(SD_APP_CMD | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
					(pCard->rca << 16), &status);
			if (SDC_SUCCESS != ret)
				break;
			ret = SDC_SendCommand(pCard->cardId,
					(SDA_SET_BUS_WIDTH | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
					2, &status);
			if (SDC_SUCCESS != ret)
				break;
			ret = SDC_SetHostBusWidth(pCard->cardId, BUS_WIDTH_4_BIT);
			if (SDC_SUCCESS != ret)
				break;

			/* 再读一下SCR，用于验证4bit数据线宽是否有问题，因为发现有的卡能支持4bit线宽，
			   但是一旦切换到4bit线宽，就发生data start bit error */
			ret = SDC_SendCommand(pCard->cardId,
					(SD_APP_CMD | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
					(pCard->rca << 16), &status);
			if (SDC_SUCCESS != ret)
				break;

			ret = SDC_BusRequest(pCard->cardId,
					(SDA_SEND_SCR | SD_READ_OP | SD_RSP_R1 | WAIT_PREV),
					0,
					&status,
					(64 >> 3),
					(64 >> 3),
					data);
			if (SDC_SUCCESS == ret) {
				pCard->workMode |= SDM_WIDE_BUS_MODE;
			} else {
				ret = SDC_SendCommand(pCard->cardId,
						(SD_APP_CMD | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
						(pCard->rca << 16), &status);
				if (SDC_SUCCESS != ret)
					break;
				ret = SDC_SendCommand(pCard->cardId,
						(SDA_SET_BUS_WIDTH | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
						0, &status);
				if (SDC_SUCCESS != ret)
					break;
				ret = SDC_SetHostBusWidth(pCard->cardId, BUS_WIDTH_1_BIT);
				if (SDC_SUCCESS != ret)
					break;
			}
		}
	} while (0);

	/* 切换线宽有不成功不直接return，高速模式的切换可以继续 */
	/* switch to high speed mode */
	if (pCard->specVer >= SD_SPEC_VER_11) {
		SDOAM_Memset(data, 0x00, (512 >> 3));
		ret = SDC_BusRequest(pCard->cardId,
				(SD2_SWITCH_FUNC | SD_READ_OP | SD_RSP_R1 | WAIT_PREV),
				((0x0U << 31) | (0xFFFFF0) | (0x1)),
				&status,
				(512 >> 3),
				(512 >> 3),
				data);
		if (SDC_SUCCESS != ret)
			return;

		/* SD2_SWITCH_FUNC命令收到的数据也是Data Packet Format为Wide Width Data，数据也必须颠倒过来 */
		for (i = 0; i < (512 >> 4); i++) {
			tmp         = pDataBuf[i];
			pDataBuf[i] = pDataBuf[(512 >> 3) - 1 - i];
			pDataBuf[(512 >> 3) - 1 - i] = tmp;
		}

		/* bit 401:High Speed support, bit 379-376:whether function can be switched */
		if ((pDataBuf[50] & 0x2) && ((pDataBuf[47] & 0xF) == 0x1)) {
			/* bit 375-368:indicate bit 273 defined */
			/* bit 273 defined:check whether High Speed ready */
			if ((pDataBuf[47] == 0x0) || ((pDataBuf[47] == 0x1) && (!(pDataBuf[35] & 0x2)))) {
				SDOAM_Memset(data, 0x00, (512 >> 3));
				ret = SDC_BusRequest(pCard->cardId,
					(SD2_SWITCH_FUNC | SD_READ_OP | SD_RSP_R1 | WAIT_PREV),
					((0x1U << 31) | (0xFFFFF0) | (0x1)),
					&status,
					(512 >> 3),
					(512 >> 3),
					data);
				if (SDC_SUCCESS != ret)
					return;

				for (i = 0; i < (512 >> 4); i++) {
					tmp         = pDataBuf[i];
					pDataBuf[i] = pDataBuf[(512 >> 3) - 1 - i];
					pDataBuf[(512 >> 3) - 1 - i] = tmp;
				}
				/* bit 379-376:whether function switched successful */
				if (((pDataBuf[47] & 0xF) == 0x1)
						&& ((pDataBuf[47] & 0xF0) != 0xF0)
						&& ((pDataBuf[48] & 0xF) != 0xF)
						&& ((pDataBuf[48] & 0xF0) != 0xF0)
						&& ((pDataBuf[49] & 0xF) != 0xF)
						&& ((pDataBuf[49] & 0xF0) != 0xF0)) {
					ret = SDC_UpdateCardFreq(pCard->cardId, SDHC_FPP_FREQ);
					if (SDC_SUCCESS != ret)
						return;
					pCard->tran_speed = SDHC_FPP_FREQ;
					pCard->workMode |= SDM_HIGH_SPEED_MODE;
				}
			}
		}
	}
}

/****************************************************************
* 函数名:SD_IsCardWriteProtected
* 描述:检查指定cardId的卡是否被机械开关写写保护
* 参数说明:cardId           输入参数  需要检查的卡
* 返回值:TRUE      卡被写保护
*        FALSE     卡没有被写保护
* 相关全局变量:
* 注意:
****************************************************************/
static uint32 _SD_IsCardWriteProtected(int32 cardId)
{
    return SDC_IsCardWriteProtected(cardId);
}

/****************************************************************
* 函数名:SD1X_Init
* 描述:SD1X卡的初始化
* 参数说明:pCardInfo 输入参数  卡信息的指针
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void   SD1X_Init(void *pCardInfo)
{
	pSDM_CARD_INFO_T pCard;
	uint32           type = UNKNOW_CARD;
	uint32           longResp[4];
	uint32           status = 0;
	uint16           rca = 0;
	uint32           i = 0;
	int32            ret = SDC_SUCCESS;

	pCard = (pSDM_CARD_INFO_T)pCardInfo;
	/**************************************************/
	/* 让卡进入Ready State */
	/**************************************************/
	/* 按照协议的规定，这里最长可以有1s的时间，我们根据每条命令执行的时间，把它换算成循环次数 */
	for (i = 0; i < ((FOD_FREQ * 1000) / ((48 + 48 + 2) << 1)); i++) {
		ret = _Identify_SendAppCmd(pCard->cardId,
				0,
				(SDA_SD_APP_OP_COND | SD_NODATA_OP | SD_RSP_R3 | WAIT_PREV),
				0x00ff8000, &status, 0, 0, NULL);
		if (SDC_SUCCESS == ret) {
			if (status & 0x80000000) {
				type = SD1X;
				break;
			} else {
				/* continue until ready */
			}
		} else if (SDC_RESP_TIMEOUT == ret) {
			/* SD1.X / SD2.0 / later version Card can not perform data transfer in the specified
			   voltage range, so it discard themselves and go into "Inactive State" */
			//if (TRUE == SDC_IsCardPresence(pCard->cardId)) {
				#ifdef SDM_PROT_INFO_DEBUG
				gSDMDriver[pCard->cardId].step = 0x21;
				gSDMDriver[pCard->cardId].error = ret;
				#endif
				ret = SDM_VOLTAGE_NOT_SUPPORT;
				break;
			/*} else {
				#ifdef SDM_PROT_INFO_DEBUG
				gSDMDriver[pCard->cardId].step = 0x22;
				gSDMDriver[pCard->cardId].error = ret;
				#endif
				ret = SDM_CARD_NOTPRESENT;
				break;
			}*/
		} else {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[pCard->cardId].step = 0x23;
			gSDMDriver[pCard->cardId].error = ret;
			#endif
			/* error occured */
			break;
		}
	}
	if (ret != SDC_SUCCESS) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x24;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}
	/* 长时间busy, SDM_VOLTAGE_NOT_SUPPORT */
	if (((FOD_FREQ * 1000) / ((48 + 48 + 2) << 1)) == i) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x25;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		ret = SDM_VOLTAGE_NOT_SUPPORT;
		return;
	}
	/**************************************************/
	/* 让卡进入Stand-by State */
	/**************************************************/
	longResp[0] = 0;
	longResp[1] = 0;
	longResp[2] = 0;
	longResp[3] = 0;
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_ALL_SEND_CID | SD_NODATA_OP | SD_RSP_R2 | WAIT_PREV),
			0, longResp, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x26;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/* decode CID */
	_SD_DecodeCID(longResp, pCard);

	do {
		ret = _Identify_SendCmd(pCard->cardId,
				(SD_SEND_RELATIVE_ADDR | SD_NODATA_OP | SD_RSP_R6 | WAIT_PREV),
				0, &status, 0, 0, NULL);
		if (SDC_SUCCESS != ret) {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[pCard->cardId].step = 0x27;
			gSDMDriver[pCard->cardId].error = ret;
			#endif
			break;
		}
		rca = (uint16)(status >> 16);
	} while (!_IsRCAUsable(rca));
	if (ret != SDC_SUCCESS) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x28;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	pCard->rca = rca;

	longResp[0] = 0;
	longResp[1] = 0;
	longResp[2] = 0;
	longResp[3] = 0;
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_SEND_CSD | SD_NODATA_OP | SD_RSP_R2 | WAIT_PREV),
			(rca << 16), longResp, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x29;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/* decode CSD */
	_SD_DecodeCSD(longResp, pCard);

	pCard->tran_speed = (pCard->tran_speed > SD_FPP_FREQ) ? SD_FPP_FREQ : (pCard->tran_speed);
	ret = SDC_UpdateCardFreq(pCard->cardId, pCard->tran_speed);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x2A;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/**************************************************/
	/* 让卡进入Transfer State */
	/**************************************************/
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_SELECT_DESELECT_CARD | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV),
			(rca << 16), &status, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x2B;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/* 协议规定不管是SD1.X或者SD2.0或者SDHC都必须支持block大小为512, 而且我们一般也只用512，因此这里直接设为512 */
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_SET_BLOCKLEN | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
			512, &status, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x2C;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	ret = _Identify_SendAppCmd(pCard->cardId,
			rca,
			(SDA_SET_CLR_CARD_DETECT | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
			0, &status, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x2D;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	pCard->WriteProt = _SD_IsCardWriteProtected(pCard->cardId);
	/* 卡输入开启密码在这里做 */
	if (status & CARD_IS_LOCKED) {
		pCard->bPassword = TRUE;
	} else {
		pCard->bPassword = FALSE;
		_SD_SwitchFunction(pCard);
	}

	pCard->type |= type;
}

/****************************************************************
* 函数名:SD20_Init
* 描述:SD20卡的初始化
* 参数说明:pCardInfo 输入参数  卡信息的指针
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void SD20_Init(void *pCardInfo)
{
	pSDM_CARD_INFO_T pCard;
	uint32           type = UNKNOW_CARD;
	uint32           longResp[4];
	uint32           status = 0;
	uint16           rca = 0;
	uint32           i = 0;
	int32            ret = SDC_SUCCESS;

	pCard = (pSDM_CARD_INFO_T)pCardInfo;
	/**************************************************/
	/* 让卡进入Ready State */
	/**************************************************/
	/* 按照协议的规定，这里最长可以有1s的时间，我们根据每条命令执行的时间，把它换算成循环次数 */
	for (i = 0; i < ((FOD_FREQ * 1000) / ((48 + 48 + 2) << 1)); i++) {
		ret = _Identify_SendAppCmd(pCard->cardId,
				0,
				(SDA_SD_APP_OP_COND | SD_NODATA_OP | SD_RSP_R3 | WAIT_PREV),
				0x40ff8000, &status, 0, 0, NULL);
		if (SDC_SUCCESS == ret) {
			if (status & 0x80000000) {
				if ((0xc0ff8000 == status) || (0xc0ff8080 == status)) {
					type = SDHC;
					break;
				} else if ((0x80ff8000 == status) || (0x80ff8080 == status)) {
					type = SD20;
					break;
				} else {
					#ifdef SDM_PROT_INFO_DEBUG
					gSDMDriver[pCard->cardId].step = 0x31;
					gSDMDriver[pCard->cardId].error = status;
					#endif
					ret = SDM_UNKNOWABLECARD;
					break;
				}
			}
		} else if (SDC_RESP_TIMEOUT == ret) {
			/* Card can not perform data transfer in the specified voltage range,
			   so it discard themselves and go into "Inactive State" */
			//if (TRUE == SDC_IsCardPresence(pCard->cardId)) {
				#ifdef SDM_PROT_INFO_DEBUG
				gSDMDriver[pCard->cardId].step = 0x32;
				gSDMDriver[pCard->cardId].error = ret;
				#endif
				ret = SDM_VOLTAGE_NOT_SUPPORT;
				break;
			/*} else {
				#ifdef SDM_PROT_INFO_DEBUG
				gSDMDriver[pCard->cardId].step = 0x33;
				gSDMDriver[pCard->cardId].error = ret;
				#endif
				ret = SDM_CARD_NOTPRESENT;
				break;
			}*/
		} else {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[pCard->cardId].step = 0x34;
			gSDMDriver[pCard->cardId].error = ret;
			#endif
			/* error occured */
			break;
		}
	}
	if (ret != SDC_SUCCESS) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x35;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}
	/* 长时间busy,SDM_VOLTAGE_NOT_SUPPORT */
	if (((FOD_FREQ * 1000) / ((48 + 48 + 2) << 1)) == i) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x36;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		ret = SDM_VOLTAGE_NOT_SUPPORT;
		return;
	}

	/**************************************************
		让卡进入Stand-by State
	**************************************************/
	longResp[0] = 0;
	longResp[1] = 0;
	longResp[2] = 0;
	longResp[3] = 0;
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_ALL_SEND_CID | SD_NODATA_OP | SD_RSP_R2 | WAIT_PREV),
			0, longResp, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x37;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/* decode CID */
	_SD_DecodeCID(longResp, pCard);

	do {
		ret = _Identify_SendCmd(pCard->cardId,
				(SD_SEND_RELATIVE_ADDR | SD_NODATA_OP | SD_RSP_R6 | WAIT_PREV),
				0, &status, 0, 0, NULL);
		if (SDC_SUCCESS != ret) {
			#ifdef SDM_PROT_INFO_DEBUG
			gSDMDriver[pCard->cardId].step = 0x38;
			gSDMDriver[pCard->cardId].error = ret;
			#endif
			break;
		}
		rca = (uint16)(status >> 16);
	} while (!_IsRCAUsable(rca));

	if (ret != SDC_SUCCESS) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x39;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	pCard->rca = rca;

	longResp[0] = 0;
	longResp[1] = 0;
	longResp[2] = 0;
	longResp[3] = 0;
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_SEND_CSD | SD_NODATA_OP | SD_RSP_R2 | WAIT_PREV),
			(rca << 16), longResp, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x3A;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/* decode CSD */
	_SD_DecodeCSD(longResp, pCard);

	pCard->tran_speed = (pCard->tran_speed > SD_FPP_FREQ) ? SD_FPP_FREQ : (pCard->tran_speed);
	ret = SDC_UpdateCardFreq(pCard->cardId, pCard->tran_speed);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x3B;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/**************************************************/
	/* 让卡进入Transfer State */
	/**************************************************/
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_SELECT_DESELECT_CARD | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV),
			(rca << 16), &status, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x3C;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	/* 协议规定不管是SD1.X或者SD2.0或者SDHC都必须支持block大小为512, 而且我们一般也只用512，因此这里直接设为512 */
	ret = _Identify_SendCmd(pCard->cardId,
			(SD_SET_BLOCKLEN | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
			512, &status, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x3D;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	ret = _Identify_SendAppCmd(pCard->cardId,
			rca,
			(SDA_SET_CLR_CARD_DETECT | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV),
			0, &status, 0, 0, NULL);
	if (SDC_SUCCESS != ret) {
		#ifdef SDM_PROT_INFO_DEBUG
		gSDMDriver[pCard->cardId].step = 0x3E;
		gSDMDriver[pCard->cardId].error = ret;
		#endif
		return;
	}

	pCard->WriteProt = _SD_IsCardWriteProtected(pCard->cardId);
	/* 卡输入开启密码在这里做 */
	if (status & CARD_IS_LOCKED) {
		pCard->bPassword = TRUE;
	} else {
		pCard->bPassword = FALSE;
		_SD_SwitchFunction(pCard);
	}

	pCard->type |= type;
}
#endif

#endif /* end of #ifdef DRIVERS_SDMMC */
