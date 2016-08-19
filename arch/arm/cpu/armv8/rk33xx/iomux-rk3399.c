/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/rkplat.h>


static void rk_pwm_iomux_config(int pwm_id)
{
	switch (pwm_id) {
	case RK_PWM0_IOMUX:
		grf_writel((3 << 20) | (1 << 4) , GRF_GPIO4C_IOMUX);
		break;
	case RK_PWM1_IOMUX:
		grf_writel((3 << 28) | (1 << 12) , GRF_GPIO4C_IOMUX);
		break;
	case RK_PWM2_IOMUX:
		pmugrf_writel((3 << 22) | (1 << 6), PMU_GRF_GPIO1C_IOMUX);
		break;
	case RK_PWM3_IOMUX:
		if (((pmugrf_readl(PMU_GRF_SOC_CON0) >> 5) & 1) == 0)
			pmugrf_writel((3 << 28) | (1 << 12), PMU_GRF_GPIO0A_IOMUX);
		else
			pmugrf_writel((3 << 28) | (1 << 12), PMU_GRF_GPIO1B_IOMUX);
		break;
	case RK_VOP0_PWM_IOMUX:
		grf_writel((3 << 20) | (2 << 4) , GRF_GPIO4C_IOMUX);
		break;
	case RK_VOP1_PWM_IOMUX:
		grf_writel((3 << 20) | (3 << 4) , GRF_GPIO4C_IOMUX);
		break;
	default:
		debug("pwm id = %d iomux error!\n", pwm_id);
		break;
	}
}

static void rk_i2c_iomux_config(int i2c_id)
{
	switch (i2c_id) {
	case RK_I2C0_IOMUX:
		pmugrf_writel((3 << 30) | (2 << 14), PMU_GRF_GPIO1B_IOMUX);
		pmugrf_writel((3 << 16) | (2 << 0), PMU_GRF_GPIO1C_IOMUX);
		break;
	case RK_I2C1_IOMUX:
		grf_writel((3 << 20) | (3 << 18) | (1 << 4) | (1 << 2), GRF_GPIO4A_IOMUX);
		break;
	case RK_I2C2_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO2A_IOMUX);
		break;
	case RK_I2C3_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (1 << 2) | (1 << 0), GRF_GPIO4C_IOMUX);
		break;
	case RK_I2C4_IOMUX:
		pmugrf_writel((3 << 24) | (3 << 22) | (1 << 8) | (1 << 6), PMU_GRF_GPIO1B_IOMUX);
		break;
	case RK_I2C5_IOMUX:
		grf_writel((3 << 22) | (3 << 20) | (2 << 6) | (2 << 4), GRF_GPIO3B_IOMUX);
		break;
	case RK_I2C6_IOMUX:
		grf_writel((3 << 20) | (3 << 18) | (2 << 4) | (2 << 2), GRF_GPIO2B_IOMUX);
		break;
	case RK_I2C7_IOMUX:
		grf_writel((3 << 30) | (2 << 14), GRF_GPIO2A_IOMUX);
		grf_writel((3 << 16) | (2 << 0), GRF_GPIO2B_IOMUX);
		break;
	case RK_I2C8_IOMUX:
		pmugrf_writel((3 << 26) | (3 << 24) | (1 << 10) | (1 << 8), PMU_GRF_GPIO1B_IOMUX);
		break;
	default:
		debug("i2c id = %d iomux error!\n", i2c_id);
		break;
	}
}

static void rk_lcdc_iomux_config(int lcd_id)
{
	switch (lcd_id) {
	default:
		debug("lcdc id = %d iomux error!\n", lcd_id);
		break;
	}
}

static void rk_spi_iomux_config(int spi_id)
{
	switch (spi_id) {
	default:
		debug("spi id = %d iomux error!\n", spi_id);
		break;
	}
}

static void rk_uart_iomux_config(int uart_id)
{
	switch (uart_id) {
	case RK_UART_BT_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (1 << 2) | (1 << 0), GRF_GPIO2C_IOMUX);
		break;
	case RK_UART_BB_IOMUX:
		grf_writel((3 << 26) | (3 << 24) | (2 << 10) | (2 << 8), GRF_GPIO3B_IOMUX);
		break;
	case RK_UART_DBG_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO4B_IOMUX);
		break;
	case RK_UART_GPS_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO3C_IOMUX);
		break;
	case RK_UART_EXP_IOMUX:
		pmugrf_writel((3 << 30) | (1 << 14), PMU_GRF_GPIO1A_IOMUX);
		pmugrf_writel((3 << 16) | (1 << 0), PMU_GRF_GPIO1B_IOMUX);
		break;
	default:
		debug("uart id = %d iomux error!\n", uart_id);
		break;
	}
}

static void rk_emmc_iomux_config(int emmc_id)
{
	switch (emmc_id) {
	default:
		debug("emmc id = %d iomux error!\n", emmc_id);
		break;
	}
}

static void rk_sdcard_iomux_config(int sdcard_id)
{
	switch (sdcard_id) {
	case RK_SDCARD_IOMUX:
		/* sdcard d0 - d3 */
		grf_writel((3 << 22) | (3 << 20) | (3 << 18) | (3 << 16) | (1 << 6) | (1 << 4) | (1 << 2) | (1 << 0), GRF_GPIO4B_IOMUX);
		/* sdcard cmd, clkout */
		grf_writel((3 << 26) | (3 << 24) | (1 << 10) | (1 << 8), GRF_GPIO4B_IOMUX);
		break;
	default:
		debug("sdcard id = %d iomux error!\n", sdcard_id);
		break;
	}
}

static void rk_hdmi_iomux_config(int hdmi_id)
{
	switch (hdmi_id) {
	case RK_HDMI_IOMUX:
		/* hdmii2c_sda, hdmii2c_scl hdmi_cecinout */
		grf_writel((3 << 30) | (3 << 18) | (3 << 16) | (1 << 14) | (3 << 2) | (3 << 0), GRF_GPIO4C_IOMUX);
		break;
	default:
		debug("hdmi id = %d iomux error!\n", hdmi_id);
		break;
	}
}


#ifdef CONFIG_RK_SDCARD_BOOT_EN
#define RK_FORCE_SELECT_JTAG	(grf_readl(GRF_SOC_CON7) & (1 << 12))
static uint32 grf_gpio4b_iomux;

__maybe_unused
void rk_iomux_sdcard_save(void)
{
	debug("rk save sdcard iomux config.\n");
	grf_gpio4b_iomux = grf_readl(GRF_GPIO4B_IOMUX);
	grf_gpio4b_iomux &= ((3 << 10) | (3 << 8) | (3 << 6) | (3 << 4) | (3 << 2) | (3 << 0));
	debug("grf gpio4b iomux = 0x%08x\n", grf_gpio4b_iomux);

	if (RK_FORCE_SELECT_JTAG) {
		debug("Force select jtag from sdcard io.\n");
	}
}


__maybe_unused
void rk_iomux_sdcard_restore(void)
{
	debug("rk restore sdcard iomux config.\n");
	grf_writel((3 << 26) | (3 << 24) | (3 << 22) | (3 << 20) | (3 << 18) | (3 << 16) | grf_gpio4b_iomux, GRF_GPIO4B_IOMUX);
	debug("grf gpio4b iomux = 0x%08x\n", grf_readl(GRF_GPIO4B_IOMUX) & ((3 << 10) | (3 << 8) | (3 << 6) | (3 << 4) | (3 << 2) | (3 << 0)));
}
#endif /* CONFIG_RK_SDCARD_BOOT_EN */
