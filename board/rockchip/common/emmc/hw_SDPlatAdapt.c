/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "sdmmc_config.h"
#include "../config.h"

#ifdef DRIVERS_SDMMC

/****************************************************************
* 函数名:SDPAM_FlushCache
* 描述:清除cache
* 参数说明:adr      输入参数     需要清除的起始地址
*          size     输入参数     需要清除的大小，单位字节
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void SDPAM_FlushCache(void *adr, uint32 size)
{
#if (EN_SD_DMA || EN_SDC_INTERAL_DMA)
	CacheFlushDRegion((uint32)(unsigned long)adr, (uint32)size);
#endif
}

/****************************************************************
* 函数名:SDPAM_CleanCache
* 描述:清理cache
* 参数说明:adr      输入参数     需要清理的起始地址
*          size     输入参数     需要清理的大小，单位字节
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void SDPAM_CleanCache(void *adr, uint32 size)
{

}


void SDPAM_InvalidateCache(void *adr, uint32 size)
{
#if (EN_SD_DMA || EN_SDC_INTERAL_DMA)
	CacheInvalidateDRegion((uint32)(unsigned long)adr, (uint32)size);
#endif
}

/****************************************************************
* 函数名:SDPAM_GetAHBFreq
* 描述:得到当前AHB总线频率
* 参数说明:
* 返回值:返回当前AHB总线频率，单位KHz
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDPAM_GetAHBFreq(SDMMC_PORT_E nSDCPort)
{
	return GetMmcCLK(nSDCPort);
}

/****************************************************************
* 函数名:SDPAM_SDCClkEnable
* 描述:选择是否开启SDMMC控制器的工作时钟
* 参数说明:nSDCPort   输入参数   端口号
*          enable     输入参数   是否使能
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void SDPAM_SDCClkEnable(SDMMC_PORT_E nSDCPort, uint32 enable)
{

}

/****************************************************************
* 函数名:SDPAM_SDCReset
* 描述:从SCU上复位SDMMC控制器
* 参数说明:nSDCPort   输入参数   端口号
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void SDPAM_SDCReset(SDMMC_PORT_E nSDCPort)
{
	if (nSDCPort == SDC0)
		SDCReset(0);
	else if (nSDCPort == SDC1)
		SDCReset(1);
	else
		SDCReset(2);
}

/****************************************************************
* 函数名:SDPAM_SetMmcClkDiv
* 描述:设置SCU上mmc_clk_div的分频值
* 参数说明:nSDCPort   输入参数   端口号
*          div        输入参数   分频值
* 返回值:返回当前AHB总线频率，单位KHz
* 相关全局变量:
* 注意:
****************************************************************/
void SDPAM_SetMmcClkDiv(SDMMC_PORT_E nSDCPort, uint32 div)
{
	if (nSDCPort == SDC0)
		SCUSelSDClk(0, div);
	else if (nSDCPort == SDC1)
		SCUSelSDClk(1, div);
	else
		SCUSelSDClk(2, div);
}

/****************************************************************
* 函数名:SDPAM_SetTuning
* 描述:设置tuning值
* 参数说明:
*
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
int32 SDPAM_SetTuning(SDMMC_PORT_E nSDCPort, uint32 degree, uint32 DelayNum)
{
	if (nSDCPort == SDC0)
		return SDM_PARAM_ERROR;
	else if (nSDCPort == SDC1)
		return SDM_PARAM_ERROR;
	else
		return SCUSetTuning(2, degree, DelayNum);
}


/****************************************************************
* 函数名:SDPAM_SetMmcClkDiv
* 描述:设置SCU上mmc_clk_div的分频值
* 参数说明:nSDCPort   输入参数   端口号
*          div        输入参数   分频值
* 返回值:返回当前AHB总线频率，单位KHz
* 相关全局变量:
* 注意:
****************************************************************/
int32  SDPAM_SetSrcFreq(SDMMC_PORT_E nSDCPort, uint32 freqKHz)
{
	if (nSDCPort == SDC0)
		return SCUSetSDClkFreq(0, freqKHz * 1000) / 1000;
	else if (nSDCPort == SDC1)
		return SCUSetSDClkFreq(1, freqKHz * 1000) / 1000;
	else
		return SCUSetSDClkFreq(2, freqKHz * 1000) / 1000;
}

#if EN_SD_DMA
/****************************************************************
* 函数名:SDPAM_DMAStart
* 描述:配置一个DMA传输
* 参数说明:nSDCPort   输入参数   需要数据传输的端口号
*          dstAddr    输入参数   目标地址
*          srcAddr    输入参数   源地址
*          size       输入参数   数据长度，单位字节
*          rw         输入参数   表示数据是要从卡读出还是写到卡，1:写到卡，0:从卡读出
*          cb_f       输入参数   DMA传输完的回调函数
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
bool SDPAM_DMAStart(SDMMC_PORT_E nSDCPort, uint32 dstAddr, uint32 srcAddr, uint32 size, bool rw, pFunc cb_f)
{
	uint32 dmac_chn = 0;
	uint32 src_addr, dst_addr;
	uint32 mode;
	int ret = 0;

	debug("dstAddr = 0x%x, srcAddr = 0x%x, size = 0x%x, rw = %d\n", dstAddr, srcAddr, size, rw);

	if (nSDCPort == SDC0)
		dmac_chn = DMACH_SDMMC;
	else if (nSDCPort == SDC1)
		dmac_chn = DMACH_SDIO;
	else
		dmac_chn = DMACH_EMMC;

	if (rw) {
		mode = RK_DMASRC_MEM;
		src_addr = dstAddr;
		dst_addr = srcAddr;
	} else {
		mode = RK_DMASRC_HW;
		src_addr = srcAddr;
		dst_addr = dstAddr;
	}

	if (rk_dma_set_buffdone_fn(dmac_chn, (rk_dma_cbfn_t)cb_f) < 0) {
		PRINT_E("dma ch = %d set buffdone fail!\n", dmac_chn);
		return FALSE;
	}
	rk_dma_devconfig(dmac_chn, mode, src_addr);
	rk_dma_enqueue(dmac_chn, NULL, dst_addr, size << 2);
	ret = rk_dma_ctrl(dmac_chn, RK_DMAOP_START);
	if (ret < 0)
		return FALSE;

	return TRUE;
}

/****************************************************************
* 函数名:SDPAM_DMAStop
* 描述:停止一个已经配置过的DMA传输
* 参数说明:nSDCPort   输入参数   需要停止的端口号
*          rw         输入参数   表示停止的数据是要从卡读出的操作还是写到卡的操作，1:写到卡，0:从卡读出
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
bool SDPAM_DMAStop(SDMMC_PORT_E nSDCPort, bool rw)
{
	uint32 dmac_chn = 0;
	int ret;

	if (nSDCPort == SDC0)
		dmac_chn = DMACH_SDMMC;
	else if (nSDCPort == SDC1)
		dmac_chn = DMACH_SDIO;
	else
		dmac_chn = DMACH_EMMC;

	ret = rk_dma_ctrl(dmac_chn, RK_DMAOP_STOP);
	if (ret < 0)
		return FALSE;

	rk_dma_ctrl(dmac_chn, RK_DMAOP_FLUSH);

	return TRUE;
}


static struct rk_dma_client rk_dma_sd_client = {
	.name = "rk-dma-sd",
};
static struct rk_dma_client rk_dma_sdio_client = {
	.name = "rk-dma-sdio",
};
static struct rk_dma_client rk_dma_emmc_client = {
	.name = "rk-dma-emmc",
};

bool SDPAM_DMAInit(SDMMC_PORT_E nSDCPort)
{
	struct rk_dma_client *dma_client = NULL;
	uint32 dmac_chn = 0;

	if (nSDCPort == SDC0) {
		dma_client = &rk_dma_sd_client;
		dmac_chn = DMACH_SDMMC;
	} else if (nSDCPort == SDC1) {
		dma_client = &rk_dma_sdio_client;
		dmac_chn = DMACH_SDIO;
	} else {
		dma_client = &rk_dma_emmc_client;
		dmac_chn = DMACH_EMMC;
	}

	if (rk_dma_request(dmac_chn, dma_client, NULL) < 0) {
		PRINT_E("Dmac request ch = %d fail!\n", dmac_chn);
		return FALSE;
	}

	if (rk_dma_config(dmac_chn, 4, 16) < 0) {
		PRINT_E("Dmac ch = %d config fail!\n", dmac_chn);
		return FALSE;
	}

	return TRUE;
}

bool SDPAM_DMADeInit(SDMMC_PORT_E nSDCPort)
{
	struct rk_dma_client *dma_client = NULL;
	uint32 dmac_chn = 0;

	if (nSDCPort == SDC0) {
		dma_client = &rk_dma_sd_client;
		dmac_chn = DMACH_SDMMC;
	} else if (nSDCPort == SDC1) {
		dma_client = &rk_dma_sdio_client;
		dmac_chn = DMACH_SDIO;
	} else {
		dma_client = &rk_dma_emmc_client;
		dmac_chn = DMACH_EMMC;
	}

	if (rk_dma_free(dmac_chn, dma_client) < 0) {
		PRINT_E("Dmac free ch = %d fail!\n", dmac_chn);
		return FALSE;
	}

	return TRUE;
}
#endif /* EN_SD_DMA */

/****************************************************************
* 函数名:SDPAM_INTCRegISR
* 描述:向中断控制器注册某个端口的中断服务线程
* 参数说明:nSDCPort   输入参数   需要注册的端口号
*          Routine    输入参数   服务线程
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDPAM_INTCRegISR(SDMMC_PORT_E nSDCPort, pFunc Routine)
{
	if (nSDCPort == SDC0)
		irq_install_handler(RKPLAT_IRQ_SDMMC, (interrupt_handler_t *)Routine, NULL);
	else if (nSDCPort == SDC1)
		irq_install_handler(RKPLAT_IRQ_SDIO, (interrupt_handler_t *)Routine, NULL);
	else
		irq_install_handler(RKPLAT_IRQ_EMMC, (interrupt_handler_t *)Routine, NULL);

	return TRUE;
}

/****************************************************************
* 函数名:SDPAM_INTCEnableIRQ
* 描述:使能中断控制器上某端口的中断
* 参数说明:nSDCPort   输入参数   需要使能的端口号
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDPAM_INTCEnableIRQ(SDMMC_PORT_E nSDCPort)
{
	uint32 ret = 0;
	if (nSDCPort == SDC0)
		ret = irq_handler_enable(RKPLAT_IRQ_SDMMC);
	else if (nSDCPort == SDC1)
		ret = irq_handler_enable(RKPLAT_IRQ_SDIO);
	else
		ret = irq_handler_enable(RKPLAT_IRQ_EMMC);

	if (ret == 0)
		return TRUE;

	return FALSE;
}

/****************************************************************
* 函数名:SDPAM_IOMUX_SetSDPort
* 描述:将IO复用到某个端口，并且该端口的数据线宽度由width指定
* 参数说明:nSDCPort   输入参数   端口号
*          width      输入参数   数据线宽度
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
uint32 SDPAM_IOMUX_SetSDPort(SDMMC_PORT_E nSDCPort, HOST_BUS_WIDTH_E width)
{
	return TRUE;
}


/****************************************************************
* 函数名:SDPAM_ControlPower
* 描述:控制指定端口的card电源开启或关闭
* 参数说明:nSDCPort 输入参数   端口号
*          enable   输入参数   1:开启电源，0:关闭电源
* 返回值:
* 相关全局变量:
* 注意:
****************************************************************/
void SDPAM_ControlPower(SDMMC_PORT_E nSDCPort, uint32 enable)
{

}

#endif /* end of #ifdef DRIVERS_SDMMC */
