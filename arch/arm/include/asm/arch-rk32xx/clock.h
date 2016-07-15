/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
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
 * rkplat clock set pll mode
 */
void rkclk_pll_mode(int pll_id, int pll_mode);

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
 * rkplat set mmc clock src
 * sdid:	sdmmc/sdio/emmc id
 * src:		sd clock source
 */
void rkclk_set_mmc_clk_src(uint32 sdid, uint32 src);

/*
 * rkplat get sd/sdmmc/emmc clock source freq
 * sdid:	sdmmc/sdio/emmc id
 */
uint32 rkclk_get_mmc_clk(uint32 sdid);

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


#ifdef CONFIG_SECUREBOOT_CRYPTO
/*
 * rkplat set cryto clock
 * here no check clkgate, because chip default is enable.
 */
void rkclk_set_crypto_clk(uint32 rate);
#endif /* CONFIG_SECUREBOOT_CRYPTO */


#ifdef CONFIG_RK_GMAC
/*
 * rkplat set gmac clock
 * mode: 0 - rmii, 1 - rgmii
 * rmii gmac clock 50MHZ from rk pll, rgmii gmac clock 125MHZ from PHY
 */
void rkclk_set_gmac_clk(uint32_t mode);
#endif /* CONFIG_RK_GMAC */
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

#endif	/* _RKXX_CLOCK_H */
