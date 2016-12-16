/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _RKXX_CLOCK_H
#define _RKXX_CLOCK_H

#include <asm/rk-common/typedef.h>


/* config cpu and general clock in MHZ */
#define KHZ				(1000)
#define MHZ				(1000*1000)

/* define pll mode */
#define RKCLK_PLL_MODE_SLOW		0
#define RKCLK_PLL_MODE_NORMAL		1

/*
 * N.B PCIE_RESET_NOFATAL stands for:
 * PCIE_RESET_MGMT_STICKY
 * PCIE_RESET_CORE
 * PCIE_RESET_MGMT
 * PCIE_RESET_PIPE
 */
enum pcie_reset_id {
	PCIE_RESET_PHY = 1,
	PCIE_RESET_ACLK,
	PCIE_RESET_PCLK,
	PCIE_RESET_PM,
	PCIE_RESET_NOFATAL,
};

/*
 * rkplat clock set pll mode
 */
void rkclk_pll_mode(int pll_mode);

/*
 * rkplat clock set pll rate by id
 */
void rkclk_set_pll_rate_by_id(enum rk_plls_id pll_id, uint32 mHz);

/*
 * rkplat clock get pll rate by id
 */
uint32 rkclk_get_pll_rate_by_id(enum rk_plls_id pll_id);

/*
 * rkplat clock set for arm and general pll
 */
void rkclk_set_pll(void);

/*
 * rkplat clock get arm pll, general pll and so on
 */
void rkclk_get_pll(void);

/*
 * rkplat clock pll dump
 */
void rkclk_dump_pll(void);


/*
 * rkplat set nandc clock
 * nandc_id:	nandc id
 * freq:	nandc max freq request.
 */
int rkclk_set_nandc_freq_from_gpll(uint32 nandc_id, uint32 freq);


/*
 * rkplat get sdhci mmc clock
 */
uint32 rkclk_get_sdhci_emmc_clk(void);


/*
 * rkplat set mmc clock src
 * sdid:	sdmmc/sdio/emmc id
 * src:		sd clock source
 */
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src);

/*
 * rkplat get sd/sdmmc/emmc clock source freq
 * sdid:	sdmmc/sdio/emmc id
 */
unsigned int rkclk_get_mmc_clk(uint32 sdid);

/*
 * rkplat get mmc clock rate from gpll
 */
uint32 rkclk_get_mmc_freq_from_gpll(uint32 sdid);

/*
 * rkplat set sd clock div, from source input
 * sdid:	sdmmc/sdio/emmc id
 * div:		sd clock div, start from 1
 */
int rkclk_set_mmc_clk_div(uint32 sdid, uint32 div);

/*
 * rkplat set sd clock freq, from source input
 * sdid:	sdmmc/sdio/emmc id
 * freq:	sd clock freq
 */
int32 rkclk_set_mmc_clk_freq(uint32 sdid, uint32 freq);

/*
 * rkplat set sd clock div, from source input
 * sdid:	sdmmc/sdio/emmc id
 * degree:  tuning degree 0/1/2/3
 * delay_num: tuning delay_num 0~255
 */
int rkclk_set_mmc_tuning(uint32 sdid, uint32 degree, uint32 delay_num);

/*
 * rkplat disable mmc clock tuning
 */
int rkclk_disable_mmc_tuning(uint32 sdid);


/*
 * rkplat get PWM clock
 * id:		pwm id
 */
unsigned int rkclk_get_pwm_clk(uint32 id);


/*
 * rkplat get I2C clock
 * i2c_bus_id:	i2c bus id
 */
unsigned int rkclk_get_i2c_clk(uint32 i2c_bus_id);


/*
 * rkplat get spi clock
 * spi_bus:	spi bus id
 */
unsigned int rkclk_get_spi_clk(uint32 spi_bus);


/*
 * rkplat lcdc dclk and aclk parent pll source
 * lcdc_id (lcdc id select) :	start from 0, max depend chip platform.
 * dclk_hz:			dclk rate
 * return dclk rate
 */
int rkclk_lcdc_clk_set(uint32 lcdc_id, uint32 dclk_hz);


/*
 * rkclk_lcdc_dclk_pll_sel
 * lcdc_id (lcdc id select) : 0 - lcdc0, 1 - lcdc1
 * pll_sel (lcdc dclk source pll select) : 0 - vpll, 1 - cpll, 2 - gpll
 */
int rkclk_lcdc_dclk_pll_sel(uint32 lcdc_id, uint32 pll_sel);


#ifdef CONFIG_SECUREBOOT_CRYPTO
/*
 * rkplat set cryto clock
 * here no check clkgate, because chip default is enable.
 */
void rkclk_set_crypto_clk(uint32 rate);
#endif /* CONFIG_SECUREBOOT_CRYPTO */

/*
 * cpu soft reset
 */
void rkcru_cpu_soft_reset(void);


/*
 * mmc soft reset
 */
void rkcru_mmc_soft_reset(uint32 sdmmcId);


/*
 * i2c soft reset
 */
void rkcru_i2c_soft_reset(void);


#if defined(CONFIG_RKCHIP_RK322XH)
/*
 * rkplat set sar-adc clock
 * here no check clkgate, because chip default is enable.
 */
void rkclk_set_saradc_clk(void);
#endif

#if defined(CONFIG_RKCHIP_RK3399)
/*
 * PCIe soft reset
 */
void rkcru_pcie_soft_reset(enum pcie_reset_id id, u32 val);
#endif

#endif	/* _RKXX_CLOCK_H */
