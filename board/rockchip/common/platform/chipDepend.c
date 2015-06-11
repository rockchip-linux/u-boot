/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <asm/io.h>
#include "../config.h"


/***************************************************************************
函数描述:延时
入口参数:us数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayUs(uint32 count)
{
	udelay(count);
}


/***************************************************************************
函数描述:延时
入口参数:ms数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayMs(uint32 count)
{
	DRVDelayUs(1000*count);
}



void CacheFlushDRegion(uint32 adr, uint32 size)
{
#ifndef CONFIG_SYS_DCACHE_OFF
	flush_cache(adr, size);
#endif
}


void CacheInvalidateDRegion(uint32 adr, uint32 size)
{
#ifndef CONFIG_SYS_DCACHE_OFF
	invalidate_dcache_range((unsigned long)adr, (unsigned long)adr + size);
#endif
}


//定义Loader启动异常类型
//系统中设置指定的sdram值为该标志，重启即可进入rockusb
//系统启动失败标志
uint32 IReadLoaderFlag(void)
{
#if defined(CONFIG_RKCHIP_RK3288) || defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	return readl(RKIO_PMU_PHYS + PMU_SYS_REG0);
#elif defined(CONFIG_RKCHIP_RK3036)
	return readl(RKIO_GRF_PHYS + GRF_OS_REG4);
#elif defined(CONFIG_RKCHIP_RK3368)
	return readl(RKIO_PMU_GRF_PHYS + PMU_GRF_OS_REG0);
#else
	#error "PLS config rk chip for loader flag."
#endif
}

void ISetLoaderFlag(uint32 flag)
{
#if defined(CONFIG_RKCHIP_RK3288)
	writel(flag, RKIO_PMU_PHYS + PMU_SYS_REG0);
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	writel(flag, RKIO_PMU_PHYS + PMU_SYS_REG0);
	// if set maskrom flag, also set GRF REG0 for ddr driver.
	if (flag == 0xEF08A53C) {
		writel(flag, RKIO_GRF_PHYS + GRF_OS_REG0);
	}
#elif defined(CONFIG_RKCHIP_RK3036)
	writel(flag, RKIO_GRF_PHYS + GRF_OS_REG4);
#elif defined(CONFIG_RKCHIP_RK3368)
	writel(flag, RKIO_PMU_GRF_PHYS + PMU_GRF_OS_REG0);
#else
	#error "PLS config rk chip for loader flag."
#endif
}


uint32 GetMmcCLK(uint32 nSDCPort)
{
	uint32 src_clk;

#if defined(CONFIG_RKCHIP_RK3288) || defined(CONFIG_RKCHIP_RK3368)
	// set general pll
	rkclk_set_mmc_clk_src(nSDCPort, 1);
	//rk32 emmc src generall pll, emmc automic divide setting freq to 1/2, for get the right freq, we divide this freq to 1/2
	src_clk = rkclk_get_mmc_clk(nSDCPort) / 2;
#elif defined(CONFIG_RKCHIP_RK3036)
	// set general pll
	rkclk_set_mmc_clk_src(nSDCPort, 2);
	src_clk = rkclk_get_mmc_clk(nSDCPort);
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	// set general pll
	rkclk_set_mmc_clk_src(nSDCPort, 1);
	src_clk = rkclk_get_mmc_clk(nSDCPort);
#else
	#error "PLS config platform for emmc clock get!"
#endif
	src_clk = src_clk / KHZ;
	debug("GetMmcCLK: sd %d, clk = %d\n", nSDCPort,src_clk);
	return src_clk;
}


#define RK_EMMC_PWREN		0X04
#define RK_EMMC_RST_N		0x78
void EmmcPowerEn(char En)
{
	if (En) {
		writel(1, RKIO_EMMC_PHYS + RK_EMMC_PWREN);
		writel(1, RKIO_EMMC_PHYS + RK_EMMC_RST_N);
	} else {
		writel(0, RKIO_EMMC_PHYS + RK_EMMC_PWREN);
		writel(0, RKIO_EMMC_PHYS + RK_EMMC_RST_N);
	}
}

void SDCReset(uint32 sdmmcId)
{
	uint32 con = 0;

#if defined(CONFIG_RKCHIP_RK3288) || defined(CONFIG_RKCHIP_RK3368)
	if (sdmmcId == 2) {
		con = (0x01 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	} else {
		con = (0x01 << sdmmcId) | (0x01 << (sdmmcId + 16));
	}
	cru_writel(con, CRU_SOFTRSTS_CON(8));
	udelay(100);
	if (sdmmcId == 2) {
		con = (0x00 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	} else {
		con = (0x00 << sdmmcId) | (0x01 << (sdmmcId + 16));
	}
	cru_writel(con, CRU_SOFTRSTS_CON(8));
	udelay(200);
#elif defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	con = (0x01 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	cru_writel(con, CRU_SOFTRSTS_CON(5));
	udelay(100);
	con = (0x00 << (sdmmcId + 1)) | (0x01 << (sdmmcId + 1 + 16));
	cru_writel(con, CRU_SOFTRSTS_CON(5));
	udelay(200);
#else
	#error "PLS config platform for emmc reset!"
#endif
	if (sdmmcId == 2) {
		EmmcPowerEn(1);
	}
}


int SCUSelSDClk(uint32 sdmmcId, uint32 div)
{
	debug("SCUSelSDClk: sd id = %d, div = %d\n", sdmmcId, div);
	return rkclk_set_mmc_clk_div(sdmmcId, div);
}


int32 SCUSetSDClkFreq(uint32 sdmmcId, uint32 freq)
{
#if defined(CONFIG_RK_MMC_DDR_MODE)
	debug("SCUSetSDClkFreq: sd id = %d, freq = %d\n", sdmmcId, freq);
	return rkclk_set_mmc_clk_freq(sdmmcId, freq);
#else
    return 0;
#endif
}

int32 SCUSetTuning(uint32 sdmmcId, uint32 degree, uint32 DelayNum)
{
#if defined(CONFIG_RK_MMC_DDR_MODE)
    debug("SCUSetTuning: degree = %d, DelayNum = %d\n", degree, DelayNum);
    return rkclk_set_mmc_tuning(sdmmcId, degree, DelayNum);
#else
        return -1;
#endif
}

void sdmmcGpioInit(uint32 ChipSel)
{
#ifdef RK_SDCARD_BOOT_EN
	if (ChipSel == 0) {
		rk_iomux_config(RK_SDCARD_IOMUX);
	}
#endif

#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
	if (ChipSel == 2) {
		rk_iomux_config(RK_EMMC_IOMUX);
	}
#endif
}

void FW_NandDeInit(void)
{
#ifdef RK_FLASH_BOOT_EN
	if(gpMemFun->flag == BOOT_FROM_FLASH) {
		FtlDeInit();
		FlashDeInit();

		return;
	}
#endif

#if defined(RK_SDMMC_BOOT_EN)
	if(gpMemFun->flag == BOOT_FROM_EMMC) {
		SdmmcDeInit(2);

		return;
	}
#endif

#if defined(RK_SDCARD_BOOT_EN)
	if (gpMemFun->flag == BOOT_FROM_SD0) {
		SdmmcDeInit(0);

		return;
	}
#endif
}


#if defined(CONFIG_RKCHIP_RK3368)
static void rk3368_uart2usb(uint32 en)
{
	if (en) {
		grf_writel(0x34000000, GRF_UOC1_CON4); // usbphy bypass disable and otg enable.

		/* if define force enable usb to uart, maybe usb function will be affected */
#ifdef CONFIG_RKUART2USB_FORCE
		grf_writel(0x007f0055, GRF_UOC0_CON0); // usb phy enter suspend
		grf_writel(0x34003000, GRF_UOC1_CON4); // usb uart enable.
#else
		con = grf_readl(GRF_SOC_STATUS15);
		if (!(con & (1<<23)) && (con & (1<<26))) { // detect id and bus
			grf_writel(0x007f0055, GRF_UOC0_CON0); // usb phy enter suspend
			grf_writel(0x34003000, GRF_UOC1_CON4); // usb uart enable.
		}
#endif /* CONFIG_RKUART2USB_FORCE */
	} else {
		grf_writel(0x34000000, GRF_UOC1_CON4); // usb uart disable
	}
}
#endif

#if defined(CONFIG_RKCHIP_RK3288)
static void rk3288_uart2usb(uint32 en)
{
	if (en) {
		grf_writel((0x0000 | (0x00C0 << 16)), GRF_UOC0_CON3); // usbphy0 bypass disable and otg enable.

		/* if define force enable usb to uart, maybe usb function will be affected */
#ifdef CONFIG_RKUART2USB_FORCE
		grf_writel((0x0004 | (0x0004 << 16)), GRF_UOC0_CON2); // software control usb phy enable
		grf_writel((0x002A | (0x003F << 16)), GRF_UOC0_CON3); // usb phy enter suspend
		grf_writel((0x00C0 | (0x00C0 << 16)), GRF_UOC0_CON3); // usb uart enable.
#else
		con = grf_readl(GRF_SOC_STATUS2);
		if (!(con & (1<<14)) && (con & (1<<17))) { // check IO domain voltage select.
			grf_writel((0x0004 | (0x0004 << 16)), GRF_UOC0_CON2); // software control usb phy enable
			grf_writel((0x002A | (0x003F << 16)), GRF_UOC0_CON3); // usb phy enter suspend
			grf_writel((0x00C0 | (0x00C0 << 16)), GRF_UOC0_CON3); // uart enable
		}
#endif /* CONFIG_RKUART2USB_FORCE */
	} else {
		grf_writel((0x0000 | (0x00C0 << 16)), GRF_UOC0_CON3); // usb uart disable
		grf_writel((0x0000 | (0x0004 << 16)), GRF_UOC0_CON2); // software control usb phy disable
	}
}
#endif

#if defined(CONFIG_RKCHIP_RK3036)
static void rk3036_uart2usb(uint32 en)
{
	if (en) {
		grf_writel(0x34000000, GRF_UOC0_CON5); // usbphy0 bypass disable and otg enable.

		/* if define force enable usb to uart, maybe usb function will be affected */
#ifdef CONFIG_RKUART2USB_FORCE
		grf_writel(0x007f0055, GRF_UOC0_CON5); // usb phy enter suspend
		grf_writel(0x34003000, GRF_UOC1_CON4); // usb uart enable.
#else
		con = grf_readl(GRF_SOC_STATUS0);
		if (!(con & (1<<7)) && (con & (1<<10))) { // detect id and bus
			grf_writel(0x007f0055, GRF_UOC0_CON5); // usb phy enter suspend
			grf_writel(0x34003000, GRF_UOC1_CON4); // usb uart enable.
		}
#endif /* CONFIG_RKUART2USB_FORCE */
	} else {
		grf_writel(0x34000000, GRF_UOC0_CON5); // usb uart disable
	}
}
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
static void rk312X_uart2usb(uint32 en)
{
	if (en) {
		grf_writel(0x34000000, GRF_UOC1_CON4); // usbphy bypass disable and otg enable.

		/* if define force enable usb to uart, maybe usb function will be affected */
#ifdef CONFIG_RKUART2USB_FORCE
		grf_writel(0x007f0055, GRF_UOC0_CON0); // usb phy enter suspend
		grf_writel(0x34003000, GRF_UOC1_CON4); // usb uart enable.
#else
		con = grf_readl(GRF_SOC_STATUS0);
		if (!(con & (1<<5)) && (con & (1<<8))) { // detect id and bus
			grf_writel(0x007f0055, GRF_UOC0_CON0); // usb phy enter suspend
			grf_writel(0x34003000, GRF_UOC1_CON4); // usb uart enable.
		}
#endif /* CONFIG_RKUART2USB_FORCE */
	} else {
		grf_writel(0x34000000, GRF_UOC1_CON4); // usb uart disable
	}
}
#endif

void rkplat_uart2UsbEn(uint32 en)
{
#if defined(CONFIG_RKCHIP_RK3288)
	rk3288_uart2usb(en);
#elif defined(CONFIG_RKCHIP_RK3036)
	rk3036_uart2usb(en);
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	rk312X_uart2usb(en);
#elif defined(CONFIG_RKCHIP_RK3368)
	rk3368_uart2usb(en);
#else
	#error "PLS config rk chip if support uart2usb."
#endif /* CONFIG_RKPLATFORM */
}

