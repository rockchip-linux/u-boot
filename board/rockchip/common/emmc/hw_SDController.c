/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define  SDC_DRIVER
#include "sdmmc_config.h"

#ifdef _USB_PLUG_
#define RK29_eMMC_Debug 1
#else
#define RK29_eMMC_Debug 0
#endif
extern int emmc_clk_power_save;

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

#define pSDCReg(n) \
	((n == 0) ? ((pSDC_REG_T)SDC0_ADDR) : ((n == 1) ? ((pSDC_REG_T)SDC1_ADDR) : (((pSDC_REG_T)SDC2_ADDR))))
#define pSDCFIFOADDR(n) \
	((n == 0) ? ((volatile uint32 *)SDC0_FIFO_ADDR) : ((n == 1) ? ((volatile uint32 *)SDC1_FIFO_ADDR) : ((volatile uint32 *)SDC2_FIFO_ADDR)))

#define SDC_Start(reg, cmd)	reg->SDMMC_CMD = cmd;

/* for debug */
#define CD_EVENT    (0x1 << 0)
#define DTO_EVENT   (0x1 << 1)
#define DMA_EVENT   (0x1 << 2)

/****************************************************************
* 函数名:SDC_ResetFIFO
* 描述: 清空FIFO
* 参数说明:
*
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 SDC_ResetFIFO(pSDC_REG_T pReg)
{
	volatile uint32 value = 0;
	int32 timeOut = 0;

	value = pReg->SDMMC_STATUS;
	if (!(value & FIFO_EMPTY)) {
		pReg->SDMMC_CTRL |= FIFO_RESET;
		timeOut = 10000;
		while (((value = pReg->SDMMC_CTRL) & (FIFO_RESET)) && (timeOut > 0)) {
			//SDOAM_Delay(1);
			timeOut--;
		}
		if (timeOut == 0)
			return SDC_SDC_ERROR;
	}

	return SDC_SUCCESS;
}


void eMMC_SetDataHigh(void)
{
	//SDC_ResetFIFO(pSDCReg(0));
	SDC_ResetFIFO(pSDCReg(2));
}

/****************************************************************
* 函数名:_ControlClock
* 描述:真正开启关闭卡的时钟在这个函数实现
* 参数说明:nSDCPort   输入参数   端口号
*          enable     输入参数  1:开启时钟，0:关闭时钟
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _ControlClock(SDMMC_PORT_E nSDCPort, uint32 enable)
{
	volatile uint32 value = 0;
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	int32           timeOut = 0;

	/* wait previous start to clear */
	timeOut = 1000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	if (enable)
		value = /*CCLK_LOW_POWER |*/ CCLK_ENABLE;
	else
		value = /*CCLK_LOW_POWER |*/ CCLK_DISABLE;

	pReg->SDMMC_CLKENA = value;
	SDC_Start(pReg, (START_CMD | UPDATE_CLOCK | WAIT_PREV));

	/* wait until current start clear */
	timeOut = 1000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:_ChangeFreq
* 描述:真正改变卡的工作频率在这个函数实现
* 参数说明:nSDCPort   输入参数   端口号
*          freqKHz    输入参数  需要设置的频率，单位KHz
* 返回值:
* 相关全局变量:
* 注意:freqKHz不能为0，如果想关闭时钟，就调用SDC_ControlClock
****************************************************************/
#if (EN_EMMC_DDR_MODE)
static int32 _ChangeMmcFreq(SDMMC_PORT_E nSDCPort, uint32 freqKHz)
{
	volatile uint32 value = 0;
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	int32           ClkInFreq;
	uint32          suitCclkInDiv;
	int32           timeOut = 0;
	int32           ret = SDC_SUCCESS;
	uint32          bDDRMode = 0;

	if (freqKHz == 0) /* 频率不能为0，否则后面会出现除数为0 */
		return SDC_PARAM_ERROR;

	ret = _ControlClock(nSDCPort, FALSE);
	if (ret != SDC_SUCCESS)
		return ret;

	bDDRMode = (pReg->SDMMC_UHS_REG >> 16) & 0x1;
	/* DDR mode 控制器内部需2分频 */
	bDDRMode ? (suitCclkInDiv = 2) : (suitCclkInDiv = 1);

	ClkInFreq = SDPAM_SetSrcFreq(nSDCPort, freqKHz * suitCclkInDiv);
	if (ClkInFreq <= 0)
		return SDC_PARAM_ERROR;

	if (ClkInFreq > (freqKHz * suitCclkInDiv))
		suitCclkInDiv = (ClkInFreq + freqKHz - 1) / freqKHz;

	if (((suitCclkInDiv & 0x1) == 1) && (suitCclkInDiv != 1))
		suitCclkInDiv++;  /* 除了1分频，保证是偶数倍 */
	if (suitCclkInDiv > 510)
		return SDC_SDC_ERROR;

	debug("_ChangeFreq: freqKHz = %d, ClkInFreq = %d, CclkInDiv = %d\n", \
			freqKHz, ClkInFreq, suitCclkInDiv);
	/* wait previous start to clear */
	timeOut = 1000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	/* fpga board no define CONFIG_RK_CLOCK, mmc clock should div from internal */
#ifdef CONFIG_RK_CLOCK
	pReg->SDMMC_CLKDIV = (suitCclkInDiv >> 1);
#else
	pReg->SDMMC_CLKDIV = ((SDPAM_GetAHBFreq(nSDCPort) + freqKHz - 1) / freqKHz + 1) >> 1;
#endif

	SDC_Start(pReg, (START_CMD | UPDATE_CLOCK | WAIT_PREV));
	/* wait until current start clear */
	timeOut = 1000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	return _ControlClock(nSDCPort, TRUE);
}

#endif
static int32 _ChangeFreq(SDMMC_PORT_E nSDCPort, uint32 freqKHz)
{
	volatile uint32 value = 0;
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	uint32          suitMmcClkDiv = 0;
	uint32          suitCclkInDiv = 0;
	uint32          ahbFreq;
	int32           timeOut = 0;
	int32           ret = SDC_SUCCESS;
	uint32          secondFreq;

#if (EN_EMMC_DDR_MODE)
	if (2 == nSDCPort)
		return _ChangeMmcFreq(nSDCPort, freqKHz);
#endif

	if (freqKHz == 0) /* 频率不能为0，否则后面会出现除数为0 */
		return SDC_PARAM_ERROR;

	ret = _ControlClock(nSDCPort, FALSE);
	if (ret != SDC_SUCCESS)
		return ret;

	ahbFreq = SDPAM_GetAHBFreq(nSDCPort);
	/* 先保证SDMMC控制器工作cclk_in不超过52MHz，否则后面设置寄存器会跑飞掉 */
	suitMmcClkDiv = ahbFreq / MMCHS_52_FPP_FREQ + (((ahbFreq % MMCHS_52_FPP_FREQ) > 0) ? 1 : 0);
	/* 低频下, 外面供给的clk就不能太高,不然cmd和数据的hold time不够 */
	if (freqKHz < 12000) {
		suitMmcClkDiv = ahbFreq / freqKHz;
		suitMmcClkDiv &= 0xFE; /* 偶数分频 */
	}

	if (suitMmcClkDiv > 0x3E)
		suitMmcClkDiv = 0x3E;

	secondFreq = ahbFreq / suitMmcClkDiv;
	suitCclkInDiv = (secondFreq / freqKHz) + (((secondFreq % freqKHz) > 0) ? 1 : 0);
	if (((suitCclkInDiv & 0x1) == 1) && (suitCclkInDiv != 1))
		suitCclkInDiv++; /* 除了1分频，保证是偶数倍 */
	Assert((suitCclkInDiv <= 510), "_ChangeFreq:no find suitable value\n", ahbFreq);
	if (suitCclkInDiv > 510)
		return SDC_SDC_ERROR;

	/* wait previous start to clear */
	timeOut = 1000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	if (suitCclkInDiv == 1)
		value = 0;
	else
		value = (suitCclkInDiv >> 1);

	/* fpga board no define CONFIG_RK_CLOCK, mmc clock should div from internal */
#ifdef CONFIG_RK_CLOCK
	pReg->SDMMC_CLKDIV = value;
#else
	pReg->SDMMC_CLKDIV = suitMmcClkDiv;
#endif
	SDC_Start(pReg, (START_CMD | UPDATE_CLOCK | WAIT_PREV));

	/* wait until current start clear */
	timeOut = 1000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	SDPAM_SetMmcClkDiv(nSDCPort, suitMmcClkDiv);

	return _ControlClock(nSDCPort, TRUE);
}

/****************************************************************
* 函数名:_WaitCardBusy
* 描述:等待指定端口上的卡busy完成
* 参数说明:nSDCPort   输入参数   端口号
* 返回值:SDC_BUSY_TIMEOUT      等待时间太长，时间溢出
*        SDC_SUCCESS           成功
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _WaitCardBusy(SDMMC_PORT_E       nSDCPort)
{
	volatile uint32 value = 0;
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	uint32          timeout = 0;

	/* wait busy */
	timeout = 0;
	while ((value = pReg->SDMMC_STATUS) & DATA_BUSY) {
		SDOAM_Delay(1);
		timeout++;
		if (timeout > 2500000) /* 写最长时间2500ms */
			return SDC_BUSY_TIMEOUT;
	}
	return SDC_SUCCESS;
}

#if (SD_CARD_Support)
/****************************************************************
* 函数名:_SDC0WriteCallback
* 描述:SDMMC0控制器用DMA写操作时的callback函数。
* 参数说明:
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static void _SDC0DMACallback(void)
{
	gSDCInfo[SDC0].intInfo.transLen = gSDCInfo[SDC0].intInfo.desLen;
}

/****************************************************************
* 函数名:_SDC1ReadCallback
* 描述:SDMMC1控制器用DMA读操作时的callback函数。
* 参数说明:
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static void _SDC1DMACallback(void)
{
	gSDCInfo[SDC1].intInfo.transLen = gSDCInfo[SDC1].intInfo.desLen;
}
#endif

/****************************************************************
* 函数名:_SDC1ReadCallback
* 描述:SDMMC2控制器用DMA读操作时的callback函数。
* 参数说明:
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static void _SDC2DMACallback(void)
{
	gSDCInfo[SDC2].intInfo.transLen = gSDCInfo[SDC2].intInfo.desLen;
}

/****************************************************************
* 函数名:_PrepareForWriteData
* 描述:根据需要写的数据长度，配置好DMA开始传输
* 参数说明:nSDCPort   输入参数   端口号
*          pDataBuf   输入参数   要写数据的地址
*          dataLen    输入参数   要写的数据长度，单位字节
*          cb         输入参数   DMA的callback函数
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _PrepareForWriteData(SDMMC_PORT_E nSDCPort, void *pDataBuf, uint32 dataLen, pFunc cb)
{
#if EN_SD_DMA
	volatile uint32  value = 0;
	pSDC_REG_T       pReg = pSDCReg(nSDCPort);
#endif
	volatile uint32 *pFIFO = pSDCFIFOADDR(nSDCPort);
	uint32           i = 0;
	/* 指针用32 bit的uint32指针，就可以满足SDMMC FIFO要求的32 bits对齐的要求了，
	   这样就算你的数据没有4字节对齐，也会因为用了uint32指针，每次向FIFO操作是4字节对齐的。*/
	uint32          *pBuf = (uint32 *)pDataBuf;
	/* 下面对于dataLen的操作，考虑到SDMMC控制器要求32bit对齐的要求，所以看起来对dataLen的操作比较麻烦
	   SDMMC控制器要求32bit对齐，如:要传输13 byte个数据，实际传入FIFO(写时)或从FIFO传出(读时)的数据必须要16个 */
	uint32           count = (dataLen >> 2) + ((dataLen & 0x3) ? 1 : 0);  /* 用32bit指针来传，因此数据长度要除4 */

#if EN_SD_DMA
	if (count <= FIFO_DEPTH) {
		for (i = 0; i < count; i++)
			*pFIFO = pBuf[i];
	} else {
		gSDCInfo[nSDCPort].intInfo.desLen = count;
		gSDCInfo[nSDCPort].intInfo.transLen = 0;
		gSDCInfo[nSDCPort].intInfo.pBuf = (uint32 *)pDataBuf;

		if (!SDPAM_DMAStart(nSDCPort, (uint32)pFIFO, (uint32)pBuf, count, 1, cb)) {
			Assert(0, "_PrepareForWriteData:DMA busy\n", 0);
			return SDC_DMA_BUSY;
		}
		value = pReg->SDMMC_CTRL;
		value |= ENABLE_DMA;
		pReg->SDMMC_CTRL = value;
	}
#else
	gSDCInfo[nSDCPort].intInfo.desLen = count;
	gSDCInfo[nSDCPort].intInfo.pBuf = pBuf;
	/* if write, fill FIFO to full before start */
	if (count > FIFO_DEPTH) {
		for (i = 0; i < FIFO_DEPTH; i++)
			*pFIFO = pBuf[i];
		gSDCInfo[nSDCPort].intInfo.transLen = FIFO_DEPTH;
	} else {
		for (i = 0; i < count; i++)
			*pFIFO = pBuf[i];
		gSDCInfo[nSDCPort].intInfo.transLen = count;
	}
#endif
	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:_PrepareForReadData
* 描述:根据需要读的数据长度，配置好DMA开始传输
* 参数说明:nSDCPort   输入参数   端口号
*          pDataBuf   输入参数   要读数据的地址
*          dataLen    输入参数   要读的数据长度，单位字节
*          cb         输入参数   DMA的callback函数
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
static int32 _PrepareForReadData(SDMMC_PORT_E nSDCPort, void *pDataBuf, uint32 dataLen, pFunc cb)
{
#if EN_SD_DMA
	volatile uint32 value = 0;
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	uint32          count = 0;

	/* 下面对于dataLen的操作，考虑到SDMMC控制器要求32bit对齐的要求，所以看起来对dataLen的操作比较麻烦
	   SDMMC控制器要求32bit对齐，如:要传输13 byte个数据，实际传入FIFO(写时)或从FIFO传出(读时)的数据必须要16个 */
	count = (dataLen >> 2) + ((dataLen & 0x3) ? 1 : 0);
	/* 当总的数据长度小于等于RX_WMARK+1时，不会向DMA发起请求(这个跟datasheet说的不大一样) */
	if (count > (RX_WMARK + 1)) {
		gSDCInfo[nSDCPort].intInfo.desLen = (dataLen >> 2);
		gSDCInfo[nSDCPort].intInfo.transLen = 0;
		gSDCInfo[nSDCPort].intInfo.pBuf = (uint32 *)pDataBuf;
		if (!SDPAM_DMAStart(nSDCPort, (uint32)pDataBuf, (uint32)pSDCFIFOADDR(nSDCPort), (dataLen >> 2), 0, cb)) {
			Assert(0, "_PrepareForReadData:DMA busy\n", 0);
			return SDC_DMA_BUSY;
		}
		value = pReg->SDMMC_CTRL;
		value |= ENABLE_DMA;
		pReg->SDMMC_CTRL = value;
	}
#else
	gSDCInfo[nSDCPort].intInfo.desLen = (dataLen >> 2);
	gSDCInfo[nSDCPort].intInfo.pBuf = (uint32 *)pDataBuf;
	gSDCInfo[nSDCPort].intInfo.transLen = 0;
#endif
	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:_ReadRemainData
* 描述:读取FIFO内的剩余数据，数据小于waterlevel或数据不是4字节的
*      整数倍时，剩余的小于4字节数据也在这里读取
* 参数说明:nSDCPort       输入参数   端口号
*          pDataBuf       输入参数   要读数据的地址
*          originalLen    输入参数   原始数据的长度，要通过这个长度
*                                    来计算剩余的数据，单位字节
* 返回值:
* 相关全局变量:
* 注意:originalLen是原始数据的长度，而不是减掉已经接收到的数据后的长度
****************************************************************/
static int32 _ReadRemainData(SDMMC_PORT_E nSDCPort, uint32 originalLen, void *pDataBuf)
{
	volatile uint32  value = 0;
	volatile uint32 *pFIFO = pSDCFIFOADDR(nSDCPort);
	pSDC_REG_T       pReg = pSDCReg(nSDCPort);
	uint32           i = 0;
	/* 指针用32 bit的uint32指针，就可以满足SDMMC FIFO要求的32 bits对齐的要求了，这样就算
	   你的数据没有4字节对齐，也会因为用了uint32指针，每次向FIFO操作是4字节对齐的。*/
	uint32          *pBuf = (uint32 *)pDataBuf;
	uint8           *pByteBuf = (uint8 *)pDataBuf;
	uint32           lastData = 0;
	/* 下面对于dataLen的操作，考虑到SDMMC控制器要求32bit对齐的要求，所以看起来对dataLen的操作比较麻烦
	    SDMMC控制器要求32bit对齐，如:要传输13 byte个数据，实际传入FIFO(写时)或从FIFO传出(读时)的数据必须要16个 */
	uint32           count = (originalLen >> 2) + ((originalLen & 0x3) ? 1 : 0); /* 用32bit指针来传，因此数据长度要除4 */

#if EN_SD_DMA
	/* DMA传输时，只有dataLen/4小于等于RX_WMARK+1,或者dataLen没4字节对齐的，才会有剩余数据 */
	value = pReg->SDMMC_STATUS;
	if (!(value & FIFO_EMPTY)) {
		if (count <= (RX_WMARK + 1)) {
			i = 0;
			while ((i < (originalLen >> 2)) && (!(value & FIFO_EMPTY))) {
				pBuf[i++] = *pFIFO;
				value = pReg->SDMMC_STATUS;
			}
		}

		if (count > (originalLen >> 2)) {
			Assert((!(value & FIFO_EMPTY)), "_ReadRemainData:need data, but FIFO empty\n", value);
			if (value & FIFO_EMPTY)
				return SDC_SDC_ERROR;
			lastData = *pFIFO;
			/* 填充剩余的1-3个字节, 只考虑CPU为小端，little-endian */
			for (i = 0; i < (originalLen & 0x3); i++)
				pByteBuf[(originalLen & 0xFFFFFFFC) + i] = (uint8)((lastData >> (i << 3)) & 0xFF);
		}
	}
#else
	/* 中断传输时，当dataLen/4小于等于RX_WMARK+1,或者dataLen没4字节对齐的，
	   或者最后剩余的数据达不到RX_WMARK+1都会有剩余数据 */
	value = pReg->SDMMC_STATUS;
	if (!(value & FIFO_EMPTY)) {
		while ((gSDCInfo[nSDCPort].intInfo.transLen < gSDCInfo[nSDCPort].intInfo.desLen) && (!(value & FIFO_EMPTY))) {
			pBuf[gSDCInfo[nSDCPort].intInfo.transLen++] = *pFIFO;
			value = pReg->SDMMC_STATUS;
		}
		//printf("_ReadRemainData transLen = %d originalLen = %d\n", gSDCInfo[nSDCPort].intInfo.transLen, originalLen);
		if (count > (originalLen >> 2)) {
			Assert((!(value & FIFO_EMPTY)), "_ReadRemainData:need data, but FIFO empty\n", value);
			if (value & FIFO_EMPTY)
				return SDC_SDC_ERROR;
			lastData = *pFIFO;
			/* 填充剩余的1-3个字节, 只考虑CPU为小端，little-endian */
			for (i = 0; i < (originalLen & 0x3); i++)
				pByteBuf[(originalLen & 0xFFFFFFFC) + i] = (uint8)((lastData >> (i << 3)) & 0xFF);
		}
	}
#endif
	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:_SDCISTHandle
* 描述:SDMMC controller中断服务程序
* 参数说明:
* 返回值:
* 相关全局变量:gSDCInfo[]
* 注意:
****************************************************************/
static void _SDCISTHandle(SDMMC_PORT_E nSDCPort)
{
	volatile uint32  value = 0;
	pSDC_REG_T       pReg = pSDCReg(nSDCPort);
	uint32           data = 0;
#if !(EN_SD_DMA)
	uint32          *pbuf32;
	uint32          count;
	SDC_INT_INFO_T   *p_intInfo = &gSDCInfo[nSDCPort].intInfo;
	volatile uint32 *pFIFO = pSDCFIFOADDR(nSDCPort);
	uint32 i = 0;
#endif

	value = pReg->SDMMC_MINTSTS;
#if EN_SD_DMA
	if (value & (RXDR_INT | TXDR_INT)) {
		Assert(0, "_SDCISTHandle:dma mode, but rx tx int\n", value);
	}
#else
	if (value & RXDR_INT) {
		//eMMC_printk(5, "%s, %s  %d, ========xbw===========\n",__FUNCTION__, __FILE__,__LINE__);

		//Assert((gSDCInfo[nSDCPort].intInfo.desLen > 0), "_SDCISTHandle:rx len <= 0\n", gSDCInfo[nSDCPort].intInfo.desLen);
		//Assert(!(gSDCInfo[nSDCPort].intInfo.rw), "_SDCISTHandle:read, but have TXDR int\n", gSDCInfo[nSDCPort].intInfo.rw);
		/* read data */
//Re_read:
		pbuf32 = &p_intInfo->pBuf[p_intInfo->transLen];
		for (i = 0; i < (RX_WMARK + 1); i++)
			*pbuf32++ = *pFIFO;
		p_intInfo->transLen += (RX_WMARK+1);
		pReg->SDMMC_RINISTS = RXDR_INT; /* 没有使用dma，读数据比较慢，先清中断标记再开始读数据 */
		//if (pReg->SDMMC_MINTSTS & RXDR_INT) /* 读完数据，中断又产生了 */
		//	goto Re_read;
	}
	if (value & TXDR_INT) {
		//eMMC_printk("%s, %s  %d, ========xbw===========\n",__FUNCTION__, __FILE__,__LINE__);

		//Assert((gSDCInfo[nSDCPort].intInfo.desLen > 0), "_SDCISTHandle:tx len <= 0\n", gSDCInfo[nSDCPort].intInfo.desLen);
		//Assert((gSDCInfo[nSDCPort].intInfo.rw), "_SDCISTHandle:write, but have RXDR int\n", gSDCInfo[nSDCPort].intInfo.rw);
		/* fill data */
		pbuf32 = &p_intInfo->pBuf[p_intInfo->transLen];
		if ((p_intInfo->desLen - p_intInfo->transLen) > (FIFO_DEPTH - TX_WMARK))
			count = (FIFO_DEPTH - TX_WMARK);
		else
			count = (p_intInfo->desLen - p_intInfo->transLen);

		for (i = 0; i < count; i++)
			*pFIFO = *pbuf32++;
		p_intInfo->transLen += count;
		pReg->SDMMC_RINISTS = TXDR_INT;
	}
#endif

	//eMMC_printk(5, "%s, %s  %d, ========xbw===========\n",__FUNCTION__, __FILE__,__LINE__);

	if (value & SDIO_INT) {
		data = pReg->SDMMC_INTMASK;
		data &= ~(SDIO_INT);
		pReg->SDMMC_INTMASK = data;
		pReg->SDMMC_RINISTS = SDIO_INT;

		if (gSDCInfo[nSDCPort].pSdioCb)
			gSDCInfo[nSDCPort].pSdioCb();
		if (gSDCInfo[nSDCPort].bSdioEn) {
			data |= SDIO_INT;
			pReg->SDMMC_INTMASK = data;
		}
	}
	if (value & CD_INT) {
		SDOAM_SetEvent(gSDCInfo[nSDCPort].event, CD_EVENT);
		pReg->SDMMC_RINISTS = CD_INT;
	}
	if (value & DTO_INT) {
		//eMMC_printk(5, "%s, %s  %d, ==DTO_INT ======xbw===========\n",__FUNCTION__, __FILE__,__LINE__);
		SDOAM_SetEvent(gSDCInfo[nSDCPort].event, DTO_EVENT);
		pReg->SDMMC_RINISTS = DTO_INT;
	}
	if (value & SBE_INT) {
		SDOAM_SetEvent(gSDCInfo[nSDCPort].event, DTO_EVENT);
		data = 0;
		#if !(EN_SD_DMA)
		data |= (RXDR_INT | TXDR_INT);
		#endif
		#if EN_SD_INT
		data |= (FRUN_INT | DTO_INT | CD_INT);
		#else
		data |= (FRUN_INT);
		#endif
		if (SDC0 == nSDCPort) {
			#if (SDMMC0_DET_MODE == SD_CONTROLLER_DET)
			data |= CDT_INT;
			#endif
		} else if (SDC1 == nSDCPort) {
			#if (SDMMC1_DET_MODE == SD_CONTROLLER_DET)
			data |= CDT_INT;
			#endif
		} else {
			//eMMC
			;
		}

		pReg->SDMMC_INTMASK = data;
	}
	if (value & CDT_INT) {
#if 0
		data = pReg->SDMMC_CDETECT;
		if (0 /* data & NO_CARD_DETECT */) { /* 假设卡永远存在 */
			/* 卡的拔插，如果不用发消息，而用一个全局变量的话(假设gCardState)，会出现这样的问题，
			 * 已经识别好可用的卡，在使用过程中，我快速拔出，然后快速再插入，这时查询gCardState
			 * 的程序如果来不及响应，就会认为卡没有被动过，不会做任何动作，所以卡也就不能再使用了
			 * 而如果用消息的话，就会收到卡被拔出，然后又被插入，所以会重新再初始化一下，卡又可用了 */
			SDOAM_SendMsg(MSG_CARD_REMOVE, nSDCPort);
		} else {
			SDOAM_SendMsg(MSG_CARD_INSERT, nSDCPort);
		}
#endif
		pReg->SDMMC_RINISTS = CDT_INT;
	}
	if (value & FRUN_INT) {
		pReg->SDMMC_RINISTS = FRUN_INT;
		Assert(0, "_SDCISTHandle:overrun or underrun\n", value);
	}
	if (value & HLE_INT) {
		Assert(0, "_SDCISTHandle:hardware locked write error\n", value);
	}
}

/****************************************************************
* 函数名:_SDC0IST
* 描述:SDMMC0中断服务程序
* 参数说明:
* 返回值:
* 相关全局变量:gSDCInfo[SDC0].intInfo
* 注意:
****************************************************************/
void _SDC0IST(void)
{
	_SDCISTHandle(SDC0);
}

/****************************************************************
* 函数名:_SDC1IST
* 描述:SDMMC1中断服务程序
* 参数说明:
* 返回值:
* 相关全局变量:gSDCInfo[SDC1].intInfo
* 注意:
****************************************************************/
void _SDC1IST(void)
{
	_SDCISTHandle(SDC1);
}

/****************************************************************
* 函数名:_SDC2IST
* 描述:SDMMC1中断服务程序
* 参数说明:
* 返回值:
* 相关全局变量:gSDCInfo[SDC2].intInfo
* 注意:
****************************************************************/
void _SDC2IST(void)
{
	_SDCISTHandle(SDC2);
}

/****************************************************************
* 函数名:SDC_Init
* 描述:SDMMC controller初始化函数
* 参数说明:
* 返回值:TRUE  初始化成功
*        FALSE 初始化失败
* 相关全局变量:
* 注意:
****************************************************************/
void SDC_Init(uint32 CardId)
{
	SDMMC_PORT_E     nSDCPort = SDC_MAX;
	uint32           clkvalue, clkdiv;

	if (CardId == 0) {
		/* init global variable */
		gSDCInfo[SDC0].pReg             = (pSDC_REG_T)SDC0_ADDR;
		gSDCInfo[SDC0].pReg->SDMMC_PWREN = 0; /* emmc 1 en, sdcard 0 en */
		gSDCInfo[SDC0].intInfo.transLen = 0;
		gSDCInfo[SDC0].intInfo.desLen   = 0;
		gSDCInfo[SDC0].intInfo.pBuf     = NULL;
		gSDCInfo[SDC0].cardFreq         = 0;
		gSDCInfo[SDC0].updateCardFreq   = FALSE;
		gSDCInfo[SDC0].event            = SDOAM_CreateEvent();
		Assert((gSDCInfo[SDC0].event != NULL), "SDC_Init:Create SDC0 event failed\n", (uint32)gSDCInfo[SDC1].event);
		if (gSDCInfo[SDC0].event == NULL)
			return;
#if (SDMMC0_BUS_WIDTH == 1)
		gSDCInfo[SDC0].busWidth = BUS_WIDTH_1_BIT;
#elif (SDMMC0_BUS_WIDTH == 4)
		gSDCInfo[SDC0].busWidth = BUS_WIDTH_4_BIT;
#elif (SDMMC0_BUS_WIDTH == 8)
		gSDCInfo[SDC0].busWidth = BUS_WIDTH_8_BIT;
#else
		#error SDMMC0 Bus Width Not Support
#endif
	} else if (CardId == 1) {
		gSDCInfo[SDC1].pReg             = (pSDC_REG_T)SDC1_ADDR;
		gSDCInfo[SDC1].pReg->SDMMC_PWREN = 0; /* emmc 1 en, sdcard 0 en */
		gSDCInfo[SDC1].intInfo.transLen = 0;
		gSDCInfo[SDC1].intInfo.desLen   = 0;
		gSDCInfo[SDC1].intInfo.pBuf     = NULL;
		gSDCInfo[SDC1].cardFreq         = 0;
		gSDCInfo[SDC1].updateCardFreq   = FALSE;
		gSDCInfo[SDC1].event            = SDOAM_CreateEvent();
		Assert((gSDCInfo[SDC1].event != NULL), "SDC_Init:Create SDC1 event failed\n", (uint32)gSDCInfo[SDC1].event);
		if (gSDCInfo[SDC1].event == NULL)
			return;
#if (SDMMC1_BUS_WIDTH == 1)
		gSDCInfo[SDC1].busWidth = BUS_WIDTH_1_BIT;
#elif (SDMMC1_BUS_WIDTH == 4)
		gSDCInfo[SDC1].busWidth = BUS_WIDTH_4_BIT;
#else
		#error SDMMC1 Bus Width Not Support
#endif
	} else {
		/* eMMC */
		gSDCInfo[SDC2].pReg             = (pSDC_REG_T)SDC2_ADDR;
		gSDCInfo[SDC2].intInfo.transLen = 0;
		gSDCInfo[SDC2].intInfo.desLen   = 0;
		gSDCInfo[SDC2].intInfo.pBuf     = NULL;
		gSDCInfo[SDC2].cardFreq         = 0;
		gSDCInfo[SDC2].updateCardFreq   = FALSE;
		gSDCInfo[SDC2].event            = SDOAM_CreateEvent();
		Assert((gSDCInfo[SDC2].event != NULL), "SDC_Init:Create SDC1 event failed\n", (uint32)gSDCInfo[SDC2].event);
		if (gSDCInfo[SDC2].event == NULL)
			return;
#if (SDMMC2_BUS_WIDTH == 1)
		gSDCInfo[SDC2].busWidth = BUS_WIDTH_1_BIT;
#elif (SDMMC2_BUS_WIDTH == 4)
		gSDCInfo[SDC2].busWidth = BUS_WIDTH_4_BIT;
#else
		gSDCInfo[SDC2].busWidth = BUS_WIDTH_8_BIT;
#endif
	}

	nSDCPort = CardId;
	/* 先保证SDMMC控制器工作cclk_in不超过52MHz，否则后面设置寄存器会跑飞掉 */
	clkvalue = SDPAM_GetAHBFreq(nSDCPort);
	clkdiv = (clkvalue + (uint32)MMCHS_52_FPP_FREQ - 1) / (uint32)MMCHS_52_FPP_FREQ;
	if (clkdiv > 0x3f) /* PLL频率太大 */
		return;
	SDPAM_SetMmcClkDiv(nSDCPort, clkdiv);
	/* enable SDMMC port */
	SDPAM_IOMUX_SetSDPort(nSDCPort, BUS_WIDTH_1_BIT);
	SDC_ResetController(nSDCPort);
	//if (SDC_IsCardPresence(nSDCPort)) {
		SDOAM_SendMsg(MSG_CARD_INSERT, nSDCPort);
	/*} else {
		_ControlClock(nSDCPort, FALSE);
		SDPAM_SDCClkEnable(nSDCPort, FALSE);
	}*/
}

/****************************************************************
* 函数名:SDC_IsCardIdValid
* 描述:检查cardId是否为有效的id
* 参数说明:cardId        输入参数  需要检查的cardId
* 返回值: TRUE       正确的cardId
*         FALSE      错误的cardId
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDC_IsCardIdValid(int32 cardId)
{
	SDMMC_PORT_E    nSDCPort = (SDMMC_PORT_E)cardId;
	if (nSDCPort >= SDC_MAX)
		return FALSE;
	else
		return TRUE;
}

/****************************************************************
* 函数名:SDC_IsCardPresence
* 描述:检查卡是否在卡槽上
* 参数说明:cardId        输入参数  需要检查的卡
* 返回值: TRUE       卡在卡槽上
*         FALSE      卡不在卡槽上
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDC_IsCardPresence(int32 cardId)
{
	return TRUE; /* 先默认，永远存在 */
}

#if (SD_CARD_Support)
/****************************************************************
* 函数名:SDC_IsCardWriteProtected
* 描述:检查某张卡的机械写保护开关是否写保护有效
* 参数说明:cardId     输入参数  需要检查的卡
* 返回值: TRUE       卡被写保护
*         FALSE      卡没有写保护
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDC_IsCardWriteProtected(int32 cardId)
{
	return FALSE;
}
#endif
/****************************************************************
* 函数名:SDC_GetHostBusWidth
* 描述:得到cardId所在接口Host所支持的线宽
* 参数说明:cardId   输入参数  需要操作的卡
*          pWidth   输出参数  得到的线宽
* 返回值:
* 相关全局变量:
* 注意:因为IOMUX和不同控制器支持的最大线宽的不同，因此上层软件
*      在改变线宽时必须先通过这个函数判断接口支持的最大线宽
****************************************************************/
int32  SDC_GetHostBusWidth(int32 cardId, HOST_BUS_WIDTH_E *pWidth)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;

	*pWidth = gSDCInfo[nSDCPort].busWidth;
	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:SDC_SetHostBusWidth
* 描述:改变SDMMC 控制器的数据线宽度
*      应该先发送完ACMD6后，紧跟调用该函数改变控制器的数据线宽度
* 参数说明:cardId   输入参数  需要操作的卡
*          value      输入参数  需要写入数据线寄存器的值
* 返回值:
* 相关全局变量:
* 注意:这个函数只改变SDMMC控制器内部的数据线宽度设置，对于卡还是
*      需要发送ACMD6来进行设置的。
*      应该先发送完ACMD6后，紧跟调用该函数改变控制器的数据线宽度
*      数据传输过程中不建议改变bus width
****************************************************************/
int32 SDC_SetHostBusWidth(int32 cardId, HOST_BUS_WIDTH_E width)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;
	uint32       value = 0;

	switch (width) {
	case BUS_WIDTH_1_BIT:
		value = BUS_1_BIT;
		break;
	case BUS_WIDTH_4_BIT:
		value = BUS_4_BIT;
		break;
	case BUS_WIDTH_8_BIT:
		value = BUS_8_BIT;
		break;
	default:
		return SDC_PARAM_ERROR;
	}
	SDPAM_IOMUX_SetSDPort(nSDCPort, width);
	pSDCReg(nSDCPort)->SDMMC_CTYPE = value;

	return SDC_SUCCESS;
}

int eMMC_Switch_ToMaskRom(void)
{
	uint32       value = 0;

	_Identify_SendCmd(2,
		(SD_GO_IDLE_STATE | SD_NODATA_OP | SD_RSP_NONE | NO_WAIT_PREV | SEND_INIT),
		0xF0F0F0F0, NULL, 0, 0, NULL);
	value = BUS_1_BIT;
	pSDCReg(2)->SDMMC_CTYPE = value;
	pSDCReg(2)->SDMMC_BLKSIZ = 0x200;
	pSDCReg(2)->SDMMC_BYTCNT = 0x200;
	pSDCReg(2)->SDMMC_INTMASK = 0;

	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:SDC_ResetController
* 描述:复位cardId所在的端口
* 参数说明:cardId   输入参数  需要操作的卡
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDC_ResetController(int32 cardId)
{
	volatile uint32  value = 0;
	pSDC_REG_T       pReg = NULL;
	SDMMC_PORT_E     nSDCPort = (SDMMC_PORT_E)cardId;
	uint32           ahbFreq = SDPAM_GetAHBFreq(nSDCPort);
	int32            timeOut = 0;
	/* reset SDMMC IP */
	SDPAM_SDCClkEnable(nSDCPort, TRUE);
	SDOAM_Delay(10);
	SDPAM_SDCReset(nSDCPort);
	/* reset */
	pReg = pSDCReg(nSDCPort);
	pReg->SDMMC_CTRL = (FIFO_RESET | SDC_RESET);
	pReg->SDMMC_PWREN = (cardId == 2) ? 1 : 0; /* emmc 1 en, sdcard 0 en */
	timeOut = 10000;
	while (((value = pReg->SDMMC_CTRL) & (FIFO_RESET | SDC_RESET)) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0) {
		SDPAM_SDCClkEnable(nSDCPort, FALSE);
		return SDC_SDC_ERROR;
	}

#if (1 == EN_SDC_INTERAL_DMA)
	pReg->SDMMC_BMOD |= BMOD_SWR;
	timeOut = 1000;
	while ((pReg->SDMMC_BMOD & BMOD_SWR) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0) {
		SDPAM_SDCClkEnable(nSDCPort, FALSE);
		return SDC_SDC_ERROR;
	}
#endif
	/* config FIFO */
	pReg->SDMMC_FIFOTH = (SD_MSIZE_16 | (RX_WMARK << RX_WMARK_SHIFT) | (TX_WMARK << TX_WMARK_SHIFT));
	pReg->SDMMC_CTYPE = BUS_1_BIT;
	pReg->SDMMC_CLKSRC = CLK_DIV_SRC_0;
	/* config debounce */
	pReg->SDMMC_DEBNCE = (DEBOUNCE_TIME*ahbFreq)&0xFFFFFF;
	pReg->SDMMC_TMOUT = 0xFFFFFF40;
	/* config interrupt */
	pReg->SDMMC_RINISTS = 0xFFFFFFFF;
	value = 0;
#if !(EN_SD_DMA)
#if (EN_SD_DATA_TRAN_INT)
	value |= (RXDR_INT | TXDR_INT);
#endif
#endif
#if EN_SD_INT
	value |= (SBE_INT | FRUN_INT | DTO_INT | CD_INT);
#else
	value |= (FRUN_INT);
#endif
	if (SDC0 == nSDCPort) {
#if (SDMMC0_DET_MODE == SD_CONTROLLER_DET)
		value |= CDT_INT;
#endif
	} else if (SDC1 == nSDCPort) {
#if (SDMMC1_DET_MODE == SD_CONTROLLER_DET)
		value |= CDT_INT;
#endif
	}

	pReg->SDMMC_INTMASK = value;

#if (EN_SD_INT || EN_SD_DATA_TRAN_INT)
#if (SD_CARD_Support)
	if (nSDCPort == SDC0)
		SDPAM_INTCRegISR(nSDCPort, _SDC0IST);
	else if (nSDCPort == SDC1)
		SDPAM_INTCRegISR(nSDCPort, _SDC1IST);
	else if (nSDCPort == SDC2)
#endif
		SDPAM_INTCRegISR(nSDCPort, _SDC2IST); /* eMMC controller */

	SDPAM_INTCEnableIRQ(nSDCPort);
	pReg->SDMMC_CTRL = ENABLE_INT;
#endif

	return SDC_SUCCESS;
}

/****************************************************************
* 函数名:SDC_UpdateCardFreq
* 描述:改变卡的工作频率，设置完以后时钟马上开始工作
* 参数说明:freqKHz  输入参数  需要设置的频率，单位KHz
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDC_UpdateCardFreq(int32 cardId, uint32 freqKHz)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;
	int32        ret = SDC_SUCCESS;

	/* ensure that there are no data or command transfers in progress */
	ret = _ChangeFreq(nSDCPort, freqKHz);
	if (ret == SDC_SUCCESS)
		gSDCInfo[nSDCPort].cardFreq = freqKHz;

	return ret;
}


/****************************************************************
* 函数名:SDC_ControlClock
* 描述:控制cardId指定的card时钟开启或关闭
* 参数说明:cardId   输入参数   需要操作的卡
*          enable   输入参数   1:开启时钟，0:关闭时钟
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDC_ControlClock(int32 cardId, uint32 enable)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;

#if (!EN_EMMC_DDR_MODE)
	uint32 clkvalue, clkdiv;
	/* ensure that there are no data or command transfers in progress */
	if (!enable) {
		gSDCInfo[nSDCPort].cardFreq = 0;
		/* 关闭时钟后，将SCU分频设置为最大，保证下次重新对SDMMC控制器操作时cclk_in不超过52M
		   先保证SDMMC控制器工作cclk_in不超过52MHz，否则后面设置寄存器会跑飞掉 */
		clkvalue = SDPAM_GetAHBFreq(nSDCPort);
		clkdiv = clkvalue / MMCHS_52_FPP_FREQ + (((clkvalue % MMCHS_52_FPP_FREQ) > 0) ? 1 : 0);
		if (clkdiv > 0x3f) /* PLL频率太大 */
			return SDC_SDC_ERROR;
		SDPAM_SetMmcClkDiv(nSDCPort, clkdiv);
	}
#endif
	return _ControlClock(nSDCPort, enable);
}

/****************************************************************
* 函数名:SDC_ControlPower
* 描述:控制cardId指定的card电源开启或关闭
* 参数说明:cardId   输入参数   需要操作的卡
*          enable   输入参数   1:开启电源，0:关闭电源
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDC_ControlPower(int32 cardId, uint32 enable)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;
	SDPAM_ControlPower(nSDCPort, enable);
	SDPAM_SDCClkEnable(nSDCPort, enable);
	return SDC_SUCCESS;
}

/*
Name:       SDC_SetDDRTuning
Desc:
Param:
Return:
Global:
Note:
Author:
Log:
*/
int32 SDC_SetDDRTuning(int32 cardId, uint32 step)
{
	uint32 DelayNum;

	DelayNum = step * 10;

	return SDPAM_SetTuning(cardId, 0, DelayNum);
}

/*
Name:       SDC_SetDDRMode
Desc:
Param:
Return:
Global:
Note:
Author:
Log:
*/
int32 SDC_SetDDRMode(int32 cardId, uint32 enable)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;
	pSDC_REG_T   pReg = pSDCReg(nSDCPort);
	int32        ret = SDC_SUCCESS;

	pReg->SDMMC_UHS_REG = pReg->SDMMC_UHS_REG | ((enable & 0x1) << 16);
	pReg->SDMMC_EMMC_DDR_REG |= (1 << 0);
	if (enable) {
		ret = _ChangeFreq(nSDCPort, gSDCInfo[nSDCPort].cardFreq);
		if (ret != SDC_SUCCESS)
			debug("SetDDRMode ERR\n");
	}

	return ret;
}

/****************************************************************
* 函数名:SDC_WaitCardBusy
* 描述:等待指定的卡busy完成
* 参数说明:cardId   输入参数   需要操作的卡
* 返回值:SDC_PARAM_ERROR      参数错误
*        SDC_BUSY_TIMEOUT     等待时间太长，时间溢出
*        SDC_SUCCESS          成功
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDC_WaitCardBusy(int32 cardId)
{
	SDMMC_PORT_E nSDCPort = (SDMMC_PORT_E)cardId;

	return _WaitCardBusy(nSDCPort);
}

#if (EN_SDC_INTERAL_DMA)
static int32 SDC_StartCmd(pSDC_REG_T pReg, uint32 cmd)
{
	int32 timeOut = 20000;

	pReg->SDMMC_CMD = cmd;
	while ((pReg->SDMMC_CMD & START_CMD) && (timeOut > 0))
		timeOut--;
	if (timeOut == 0)
		return SDC_SDC_ERROR;

	return SDC_SUCCESS;
}

static int32 SDC_SetIDMADesc(SDMMC_PORT_E SDCPort, uint32 buffer, uint32  BufSize)
{
	pSDC_INFO_T     pSDC = &gSDCInfo[SDCPort];
	pSDC_REG_T      pReg = pSDC->pReg;
	PSDMMC_DMA_DESC pDesc = (PSDMMC_DMA_DESC)&pSDC->IDMADesc[0];
	uint32 i, size;

	pReg->SDMMC_DBADDR = (uint32)(unsigned long)pDesc;
	for (i = 0; i < MAX_DESC_NUM_IDMAC; i++, pDesc++) {
		size = MIN(MAX_BUFF_SIZE_IDMAC, BufSize);
		pDesc->desc1 = ((size << DescBuf1SizeShift) & DescBuf1SizMsk);
		pDesc->desc2 =  (uint32)buffer;
		pDesc->desc0 = DescSecAddrChained | DescOwnByDma | ((i == 0) ? DescFirstDesc : 0) | DescDisInt;

		BufSize -= size;
		if (0 == BufSize)
			break;
		buffer += size;
		pDesc->desc3 = (uint32)(unsigned long)(pDesc + 1);
	}

	pDesc->desc0 |= DescLastDesc;
	pDesc->desc0 &= ~DescDisInt;

	SDPAM_FlushCache((void *)&pSDC->IDMADesc[0], (i + 1) * sizeof(SDMMC_DMA_DESC));

	return 0;
}

static int32 SDC_RequestIDMA(SDMMC_PORT_E nSDCPort,
			uint32  cmd,
			uint32  CmdArg,
			void   *pDataBuf,
			uint32  DataLen)
{
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	pSDC_INFO_T     pSDC = &gSDCInfo[nSDCPort];
	uint32          timeout  = DataLen * 500; /* 最长一个512需要等250ms */
	volatile uint32 value;

	if ((cmd & SD_OP_MASK) == SD_WRITE_OP)
		SDPAM_FlushCache(pDataBuf, DataLen);
	else
		SDPAM_InvalidateCache(pDataBuf, DataLen);

	SDC_SetIDMADesc(nSDCPort, (uint32)(unsigned long)pDataBuf, DataLen);

	pReg->SDMMC_CTRL |= CTRL_IDMAC_RESET; /* idmac reset */
	pReg->SDMMC_CTRL |= CTRL_USE_IDMAC;
	pReg->SDMMC_BMOD |= (BMOD_DE | BMOD_FB);
	//pReg->SDMMC_FIFOTH = (SD_MSIZE_1 | (RX_WMARK << RX_WMARK_SHIFT) | (TX_WMARK << TX_WMARK_SHIFT));
	pSDC->ErrorStat = SDC_SUCCESS;
	//pSDC->IDMAOn = 1;

	pReg->SDMMC_CMDARG = CmdArg;
	if (SDC_SUCCESS != SDC_StartCmd(pReg, (cmd & ~(RSP_BUSY)) | START_CMD | USE_HOLD_REG))
		return SDC_SDC_ERROR;

	do {
		SDOAM_Delay(1);
		if ((--timeout) == 0 || pSDC->ErrorStat != SDC_SUCCESS)
			break;

		value = pReg->SDMMC_RINISTS;
		/* if error happen may be no DTO_INT */
		if (value & (RE_INT | RCRC_INT | RTO_INT | SBE_INT|EBE_INT | DRTO_INT | DCRC_INT))
			break;
	} while ((value & (CD_INT | DTO_INT)) != (CD_INT | DTO_INT));
	if (timeout == 0) {
		debug("SDC ERR: data timeout 0x%x\n", value);
		pSDC->ErrorStat = SDC_RESP_TIMEOUT;
	}

	value = pReg->SDMMC_RINISTS;
	if (value & (RE_INT|RCRC_INT))
		pSDC->ErrorStat = SDC_RESP_ERROR;
	else if (value & RTO_INT)
		pSDC->ErrorStat |= SDC_RESP_TIMEOUT;
	else if (value & SBE_INT)
		pSDC->ErrorStat |= SDC_START_BIT_ERROR;
	else if (value & EBE_INT)
		pSDC->ErrorStat |= SDC_END_BIT_ERROR;
	else if (value & DRTO_INT)
		pSDC->ErrorStat |= SDC_DATA_READ_TIMEOUT;
	else if (value & DCRC_INT)
		pSDC->ErrorStat |= SDC_DATA_CRC_ERROR;

	pReg->SDMMC_RINISTS = 0xFFFFFFFF;
	pReg->SDMMC_CTRL &= ~CTRL_USE_IDMAC;
	pReg->SDMMC_BMOD &= ~BMOD_DE;
	//pSDC->IDMAOn = 0;
	if ((cmd & SD_OP_MASK) == SD_WRITE_OP && (pSDC->ErrorStat == SDC_SUCCESS))
		pSDC->ErrorStat = _WaitCardBusy(nSDCPort);

	if (pSDC->ErrorStat != SDC_SUCCESS) {
		debug("SDC ERR: 0x%x\n", pSDC->ErrorStat);
	}

	return pSDC->ErrorStat;
}
#endif

/****************************************************************
* 函数名:SDC_BusRequest
* 描述:向指定的cardId卡发起总线操作
* 参数说明:cardId        输入参数  用于指定命令要发给的卡
*          cmd           输入参数  需要发送的命令
*          cmdArg        输入参数  命令参数，当命令不需要命令参数时，不理会该值
*          isAppcmd      输入参数  指示当前命令是否是application命令
*          respType      输入参数  回复类型
*          responseBuf   输出参数  存放回复的buf
*          blockSize     输入参数  block大小,单位字节
*          datalen       输入参数  需要接收或发送的字节长度，单位字节
*          dataBuf       输入或输出参数  存放接收到的数据或者要发送出去的数据
* 返回值:SDC_PARAM_ERROR
*        SDC_BUSY_TIMEOUT
*        SDC_RESP_TIMEOUT
*        SDC_DATA_CRC_ERROR
*        SDC_END_BIT_ERROR
*        SDC_START_BIT_ERROR
*        SDC_DATA_READ_TIMEOUT
*        SDC_RESP_CRC_ERROR
*        SDC_RESP_ERROR
*        SDC_SUCCESS
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDC_BusRequest(int32 cardId,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf)
{
	volatile uint32 value = 0;
	SDMMC_PORT_E    nSDCPort = (SDMMC_PORT_E)cardId;
	pSDC_REG_T      pReg = pSDCReg(nSDCPort);
	int32           ret = SDC_SUCCESS;
	int32           timeOut = 0;
#if !(EN_SD_DMA)
	uint32          tranCount = 0;
	uint32          totleCount = 0;
	uint32          i, count;
	uint32          *pBuf;
	volatile uint32 *pFIFO = pSDCFIFOADDR(nSDCPort);
#endif
	pFunc           cb;

	//eMMC_printk(4, "SDC_BusRequest 111%s  %d\n", __FILE__,__LINE__);

	//PRINT_E( "EMMC CMD=%d arg = 0x%x len=0x%x\n",cmd&0x3f,cmdArg,dataLen);
	/* ensure the card is not busy due to any previous data transfer command */
	if (cmd & WAIT_PREV) {
		if (pReg->SDMMC_STATUS & DATA_BUSY) {
			debug("DATA_BUSY:cmd=0x%x, pReg->SDMMC_CMD=0x%x\n", (cmd & 0x3f), pReg->SDMMC_CMD);
			return SDC_BUSY_TIMEOUT;
		}
	}

	if ((cmd & STOP_CMD) || (cmd & DATA_EXPECT)) {
		/* 清除FIFO */
		if (!(pReg->SDMMC_STATUS & FIFO_EMPTY)) {
			pReg->SDMMC_CTRL |= FIFO_RESET;
			timeOut = 100000;
			while (((value = pReg->SDMMC_CTRL) & (FIFO_RESET)) && (timeOut > 0)) {
				//SDOAM_Delay(1);
				timeOut--;
			}
			if (timeOut == 0) {
				eMMC_printk(3, "SDC_BusRequest:  CMD=%d SDC_SDC_ERROR  %d\n", cmd&0x3f, __LINE__);
				return SDC_SDC_ERROR;
			}
		}
	}

	Assert(!(((value = pReg->SDMMC_STATUS) & DATA_BUSY) && (cmd & WAIT_PREV)), "SDC_BusRequest:busy error\n", value);
	Assert(!(((value = pReg->SDMMC_STATUS) & 0x3FFE0000) && (!(cmd & STOP_CMD))), "SDC_BusRequest:+FIFO not empty\n", value);
	Assert(!((value = pReg->SDMMC_CMD) & START_CMD), "SDC_BusRequest:start bit != 0\n", value);
	value = pReg->SDMMC_CMD;
	if (value & START_CMD) {
		eMMC_printk(3, "SDC_BusRequest: CMD=%d START CMD ERROR %d\n", cmd&0x3f, __LINE__);
		return SDC_SDC_ERROR;
	}

	if (gSDCInfo[nSDCPort].updateCardFreq) {
		ret = _ChangeFreq(nSDCPort, gSDCInfo[nSDCPort].cardFreq);
		if (ret != SDC_SUCCESS) {
			eMMC_printk(3, "SDC_BusRequest:  CMD=%d ChangeFreq ERROR %d\n", cmd & 0x3f, __LINE__);
			return ret;
		}
		gSDCInfo[nSDCPort].updateCardFreq = FALSE;
	}
	//eMMC_printk(2, "SDC_BusRequest 333%s  %d\n", __FILE__, __LINE__);
	if (cmd & DATA_EXPECT) {
		Assert((dataLen > 0), "SDC_BusRequest:dataLen = 0\n", dataLen);
		if (dataLen == 0)
			return SDC_PARAM_ERROR;

		pReg->SDMMC_BLKSIZ = blockSize;
		/* 这个寄存器的长度一定要设置为需要的长度，不用考虑SDMMC控制器的32bit对齐 */
		pReg->SDMMC_BYTCNT = dataLen;
#if (EN_SDC_INTERAL_DMA == 1)
		if ((dataLen <= MAX_DATA_SIZE_IDMAC) && (dataLen >= 512))
			return SDC_RequestIDMA(nSDCPort, cmd, cmdArg, pDataBuf, dataLen);
#endif
		if ((cmd & SD_OP_MASK) == SD_WRITE_OP) {
#if (SD_CARD_Support)
			if (nSDCPort == SDC0)
				cb   = _SDC0DMACallback;
			else if (nSDCPort == SDC1)
				cb   = _SDC1DMACallback;
			else
#endif
				cb   = _SDC2DMACallback;

			/* 写之前clean cache */
			SDPAM_CleanCache(pDataBuf, dataLen);
			_PrepareForWriteData(nSDCPort, pDataBuf, dataLen, cb);
		} else {
#if (SD_CARD_Support)
			if (nSDCPort == SDC0)
				cb   = _SDC0DMACallback;
			else if (nSDCPort == SDC1)
				cb   = _SDC1DMACallback;
			else
#endif
			cb   = _SDC2DMACallback;

			_PrepareForReadData(nSDCPort, pDataBuf, dataLen, cb);
		}
	}
	//eMMC_printk(3, "SDC_BusRequest 444  %s  %d\n", __FILE__, __LINE__);
	pReg->SDMMC_CMDARG = cmdArg;
	/* set start bit,CMD/CMDARG/BYTCNT/BLKSIZ/CLKDIV/CLKENA/CLKSRC/TMOUT/CTYPE were locked */
	SDC_Start(pReg, ((cmd & (~(RSP_BUSY))) | START_CMD | USE_HOLD_REG));
	/* wait until current start clear */
	timeOut = 100000;
	while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0) {
		eMMC_printk(3, "SDC_BusRequest:  CMD=%d  START_CMD timeout %d\n", cmd & 0x3f, __LINE__);
		return SDC_SDC_ERROR;
	}
#if EN_SD_INT
	SDOAM_GetEvent(gSDCInfo[nSDCPort].event, CD_EVENT);
#else
	//eMMC_printk(5, "SDC_BusRequest 444-2  ------  %s  %d\n", __FILE__, __LINE__);
	timeOut = 250000; /* 250ms */
	while (!((value = pReg->SDMMC_RINISTS) & CD_INT) && (timeOut > 0)) {
		SDOAM_Delay(1);
		timeOut--;
	}
	if (timeOut == 0) {
		eMMC_printk(5, "SDC_BusRequest 444-1  ---START_CMD timeout---  %s  %d\n", __FILE__, __LINE__);
		return SDC_SDC_ERROR;
	}
	pReg->SDMMC_RINISTS = CD_INT;
#endif
	if (cmd & STOP_CMD) {
		/* 随着STOP命令，可能还会有数据进来(如mutiple read时)，要再清除FIFO */
		value = pReg->SDMMC_STATUS;
		if (!(value & FIFO_EMPTY)) {
			value = pReg->SDMMC_CTRL;
			value |= FIFO_RESET;
			pReg->SDMMC_CTRL = value;
			timeOut = 1000;
			while (((value = pReg->SDMMC_CTRL) & (FIFO_RESET)) && (timeOut > 0)) {
				SDOAM_Delay(1);
				timeOut--;
			}
			if (timeOut == 0)
				return SDC_SDC_ERROR;
		}
	}
	//eMMC_printk(3, "SDC_BusRequest 555  %s  %d\n", __FILE__, __LINE__);
	/* check response error, or response crc error, or response timeout */
	value = pReg->SDMMC_RINISTS;
	if (value & RTO_INT) {
		//eMMC_printk(3, "SDC_BusRequest 555--1  %s  %d\n", __FILE__, __LINE__);
		pReg->SDMMC_RINISTS = 0xFFFFFFFF; /* 如果response timeout，数据传输 被中止，清除所有中断 */
#if EN_SD_DMA
		value = pReg->SDMMC_CTRL;
		if (value & ENABLE_DMA) {
			if ((cmd & SD_OP_MASK) == SD_WRITE_OP)
				SDPAM_DMAStop(nSDCPort, 1);
			else
				SDPAM_DMAStop(nSDCPort, 0);
			value &= ~(ENABLE_DMA);
			value |= FIFO_RESET;
			pReg->SDMMC_CTRL = value;
			timeOut = 1000;

			while (((value = pReg->SDMMC_CTRL) & (FIFO_RESET)) && (timeOut > 0)) {
				SDOAM_Delay(1);
				timeOut--;
			}
			if (timeOut == 0)
				return SDC_SDC_ERROR;
		}
#endif
		eMMC_printk(3, " SDC_BusRequest:  CMD=%d  SDC_RESP_TIMEOUT %d\n ", cmd & 0x3f, __LINE__);
		//PRINT_E("SDC_RESP_TIMEOUT\n","");
		//PRINT_E("sdc emmc reinit\n","");
		return SDC_RESP_TIMEOUT;
	}

	if (cmd & DATA_EXPECT) {
#if EN_SD_INT
		SDOAM_GetEvent(gSDCInfo[nSDCPort].event, DTO_EVENT);
#else
#if (EN_SD_DATA_TRAN_INT == 0)
		timeOut = 250000; /* 250ms */
		if ((cmd & SD_OP_MASK) == SD_WRITE_OP) {
			tranCount = gSDCInfo[nSDCPort].intInfo.transLen;
			totleCount = gSDCInfo[nSDCPort].intInfo.desLen;
			pBuf = gSDCInfo[nSDCPort].intInfo.pBuf;
			while ((tranCount < totleCount) && timeOut) {
				if ((pReg->SDMMC_RINISTS) & (TXDR_INT)) {
					if ((totleCount - tranCount) > (FIFO_DEPTH - TX_WMARK))
						count = (FIFO_DEPTH - TX_WMARK);
					else
						count = (totleCount - tranCount);
					for (i = 0; i < count; i++)
						 *pFIFO = pBuf[tranCount++];
					pReg->SDMMC_RINISTS = TXDR_INT;
					//timeOut = 15 * 1024 * 256;
				} else if (!(pReg->SDMMC_STATUS & FIFO_FULL)) {
					*pFIFO = pBuf[tranCount++];
					//timeOut = 15 * 1024 * 256;
				} else {
					SDOAM_Delay(1);
					timeOut--;
				}
			}
			gSDCInfo[nSDCPort].intInfo.transLen = tranCount;
		} else {
			tranCount = gSDCInfo[nSDCPort].intInfo.transLen;
			totleCount = gSDCInfo[nSDCPort].intInfo.desLen;
			pBuf = gSDCInfo[nSDCPort].intInfo.pBuf;
			while ((tranCount < totleCount) && timeOut) {
				if (pReg->SDMMC_RINISTS & (DRTO_INT | SBE_INT | EBE_INT | DCRC_INT)) {
					ret = SDC_END_BIT_ERROR;
					break;
				}
				if ((pReg->SDMMC_RINISTS) & (RXDR_INT)) {
					uint32 *pbuf32 = &pBuf[tranCount];
					for (i = 0; i < RX_WMARK + 1; i++)
						*pbuf32++ =  *pFIFO;
					pReg->SDMMC_RINISTS = RXDR_INT;
					tranCount += RX_WMARK + 1;
					timeOut = 250000;
				} else if (!(pReg->SDMMC_STATUS & FIFO_EMPTY)) {
					pBuf[tranCount++] = *pFIFO;
					timeOut =  15 * 1024 * 256;
				} else {
					SDOAM_Delay(1);
					timeOut--;
				}
			}
		}
		//PRINT_E("totleCount %d tranCount %d %d %d\n", totleCount, tranCount, timeOut, RkldTimerGetTick());
#endif
		timeOut = 100 * dataLen + 250000; /* 1ms read 20 sector, 1 sector timeout set 50ms + 250ms */
		while ((!((pReg->SDMMC_RINISTS) & (DTO_INT | SBE_INT))) &&  timeOut) {
			SDOAM_Delay(1);
			timeOut--;
		}
		if (timeOut == 0) {
			eMMC_printk(5, "SDC_BusRequest ---wait for data timeout--- %d\n", __LINE__);
			return SDC_SDC_ERROR;
		}
		pReg->SDMMC_RINISTS = DTO_INT;
#endif
		if ((cmd & SD_OP_MASK) == SD_WRITE_OP) {
#if EN_SD_DMA
			value = pReg->SDMMC_CTRL;
			if (value & ENABLE_DMA) {
				SDPAM_DMAStop(nSDCPort, 1);
				value &= ~(ENABLE_DMA);
				pReg->SDMMC_CTRL = value;
			}
#endif
			value = pReg->SDMMC_RINISTS;
			if (value & DCRC_INT) {
				ret = SDC_DATA_CRC_ERROR;
			} else if (value & EBE_INT) {
				ret = SDC_END_BIT_ERROR;
			} else {
				ret = _WaitCardBusy(nSDCPort);
				if (ret != SDC_SUCCESS)
					return ret;
				ret = SDC_SUCCESS;
			}
		} else {
#if EN_SD_DMA
			value = pReg->SDMMC_CTRL;
			if (value & ENABLE_DMA) {
				SDPAM_DMAStop(nSDCPort, 0);
				value &= ~(ENABLE_DMA);
				pReg->SDMMC_CTRL = value;
			}
#endif

			value = pReg->SDMMC_RINISTS;
			//Assert(!((value & SBE_INT) && (!(value & DRTO_INT))), "SDC_BusRequest:start bit error but not timeout\n", value);
			if (value & (SBE_INT | EBE_INT | DRTO_INT | DCRC_INT)) {
				if (value & SBE_INT) {
					uint32 stopCmd = 0;
					stopCmd = (SD_STOP_TRANSMISSION | SD_NODATA_OP | SD_RSP_R1 | STOP_CMD | NO_WAIT_PREV);
					SDC_Start(pReg, ((stopCmd & (~(RSP_BUSY))) | START_CMD));
					timeOut = 10000;
					while (((value = pReg->SDMMC_CMD) & START_CMD) && (timeOut > 0)) {
						SDOAM_Delay(1);
						timeOut--;
					}
					if (timeOut == 0) {
						eMMC_printk(3, "SDC_BusRequest:  CMD=%d START_CMD ERROR %d\n", cmd & 0x3f, __LINE__);
						return SDC_SDC_ERROR;
					}
					#if EN_SD_INT
					SDOAM_GetEvent(gSDCInfo[nSDCPort].event, CD_EVENT);
					SDOAM_GetEvent(gSDCInfo[nSDCPort].event, DTO_EVENT);
					#else
					while (((value = pReg->SDMMC_RINISTS) & (CD_INT | DTO_INT)) != (CD_INT | DTO_INT)) {
						SDOAM_Delay(1);
					}
					pReg->SDMMC_RINISTS = (CD_INT | DTO_INT);
					#endif
					pReg->SDMMC_RINISTS = SBE_INT;
					value = 0;
					#if !(EN_SD_DMA)
					#if (EN_SD_DATA_TRAN_INT)
						value |= (RXDR_INT | TXDR_INT);
					#endif
					#endif
					#if EN_SD_INT
						value |= (SBE_INT | FRUN_INT | DTO_INT | CD_INT);
					#else
						value |= (FRUN_INT);
					#endif
					if (SDC0 == nSDCPort) {
						#if (SDMMC0_DET_MODE == SD_CONTROLLER_DET)
						value |= CDT_INT;
						#endif
					} else if (SDC1 == nSDCPort) {
						#if (SDMMC1_DET_MODE == SD_CONTROLLER_DET)
						value |= CDT_INT;
						#endif
					} else {
						/* eMMC no detect */
					}

					pReg->SDMMC_INTMASK = value;
					ret = SDC_START_BIT_ERROR;
				} else if (value & EBE_INT) {
					if ((cmd & SD_CMD_MASK) == SD_CMD14)
						ret = _ReadRemainData(nSDCPort, dataLen, pDataBuf);
					else
						ret = SDC_END_BIT_ERROR;
				} else if (value & DRTO_INT) {
					ret = SDC_DATA_READ_TIMEOUT;
				} else if (value & DCRC_INT) {
					ret = SDC_DATA_CRC_ERROR;
				}
			} else {
				ret = _ReadRemainData(nSDCPort, dataLen, pDataBuf);
			}

			//eMMC_printk(3, "SDC_BusRequest 777 %s  %d\n", __FILE__, __LINE__);

#if !(EN_SD_DMA)
			Assert(!((gSDCInfo[nSDCPort].intInfo.transLen != gSDCInfo[nSDCPort].intInfo.desLen) && (ret == SDC_SUCCESS)), \
				"SDC_BusRequest:translen != deslen\n", gSDCInfo[nSDCPort].intInfo.transLen);
			gSDCInfo[nSDCPort].intInfo.transLen = 0;
			gSDCInfo[nSDCPort].intInfo.desLen   = 0;
			gSDCInfo[nSDCPort].intInfo.pBuf     = NULL;
#endif
			//eMMC_printk(3, "%s...%d....SDMMC_STATUS=%x, ret=%d \n", __FILE__, __LINE__, value = pReg->SDMMC_STATUS, ret);
			Assert(!(((value = pReg->SDMMC_STATUS) & 0x3FFE0000) && (ret == SDC_SUCCESS)), "SDC_BusRequest:-FIFO not empty\n", value);
			//eMMC_printk(3, "SDC_BusRequest 777--1  %s  %d\n", __FILE__, __LINE__);
		}
	}
	value = pReg->SDMMC_RINISTS;
	pReg->SDMMC_RINISTS = 0xFFFFFFFF;

	//eMMC_printk(3, "SDC_BusRequest 888 %s  %d\n", __FILE__, __LINE__);

	if (ret == SDC_SUCCESS) {
		if (cmd & RSP_BUSY) { /* R1b */
			ret = _WaitCardBusy(nSDCPort);
			if (ret != SDC_SUCCESS) {
				eMMC_printk(3, "SDC_BusRequest:  CMD=%d WaitCard Ready Timeout%d\n", cmd & 0x3f, __LINE__);
				return ret;
			}
		}

		/* if need, get response */
		if ((cmd & R_EXPECT) && (responseBuf != NULL)) {
			if (cmd & LONG_R) {
				responseBuf[0] = pReg->SDMMC_RESP0;
				responseBuf[1] = pReg->SDMMC_RESP1;
				responseBuf[2] = pReg->SDMMC_RESP2;
				responseBuf[3] = pReg->SDMMC_RESP3;
			} else {
				responseBuf[0] = pReg->SDMMC_RESP0;
			}
		}

		if (gSDCInfo[nSDCPort].cardFreq > 400) {
			if (value & RCRC_INT)
				return SDC_RESP_CRC_ERROR;
			if (value & RE_INT)
				return SDC_RESP_ERROR;
		}
	}
	//eMMC_printk(3, "SDC_BusRequest Success.  %s  %d\n", __FILE__, __LINE__);

	return ret;
}

#endif  /* end of #ifdef DRIVERS_SDMMC */
