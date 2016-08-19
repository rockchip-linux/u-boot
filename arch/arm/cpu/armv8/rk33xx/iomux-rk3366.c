/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
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
		pmugrf_writel((3 << 16) | 1, PMU_GRF_GPIO0B_IOMUX);
		break;
	case RK_PWM1_IOMUX:
		pmugrf_writel((3 << (12 + 16)) | (2 << 12), PMU_GRF_GPIO1A_IOMUX);
		break;
	case RK_PWM2_IOMUX:
		break;
	case RK_PWM3_IOMUX:
		pmugrf_writel((3 << 16) | 2, PMU_GRF_GPIO1A_IOMUX);
		break;
	case RK_VOP0_PWM_IOMUX:
		pmugrf_writel((3 << 16) | 2, PMU_GRF_GPIO0B_IOMUX);
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
		pmugrf_writel((3 << 24) | (3 << 22) | (1 << 8) | (1 << 6), PMU_GRF_GPIO0A_IOMUX);
		break;

	case RK_I2C1_IOMUX:
		grf_writel((3 << 20) | (3 << 18) | (1 << 4) | (1 << 2), GRF_GPIO4D_IOMUX);
		break;
	case RK_I2C2_IOMUX:
		grf_writel((3 << 30) | (2 << 14), GRF_GPIO5B_IOMUX);
		grf_writel((3 << 16) | 2, GRF_GPIO5C_IOMUX);
		break;
	case RK_I2C3_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | 2, GRF_GPIO2C_IOMUX);
		break;
	case RK_I2C4_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (1 << 2) | 1, GRF_GPIO5B_IOMUX);
		break;
	case RK_I2C5_IOMUX:
		grf_writel((3 << 28) | (3 << 26) | (1 << 12) | (1 << 10), GRF_GPIO5B_IOMUX);
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
	case RK_SPI0_CS0_IOMUX:
		/* spi0 cs0 */
		grf_writel((3 << 16) | (3 << 0), GRF_GPIO2D_IOMUX);
		/* spi0 clk rxd txd */
		grf_writel((3 << 30) | (3 << 28) | (3 << 14) | (3 << 12), GRF_GPIO2C_IOMUX);
		grf_writel((3 << 26) | (2 << 10), GRF_GPIO2D_IOMUX);
		break;
	case RK_SPI0_CS1_IOMUX:
		/* spi0 cs1 */
		grf_writel((3 << 18) | (3 << 2), GRF_GPIO2D_IOMUX);
		/* spi0 clk rxd txd */
		grf_writel((3 << 30) | (3 << 28) | (3 << 14) | (3 << 12), GRF_GPIO2C_IOMUX);
		grf_writel((3 << 26) | (2 << 10), GRF_GPIO2D_IOMUX);
		break;
	case RK_SPI1_CS0_IOMUX:
		/* spi1 cs0 */
		grf_writel((3 << 26) | (3 << 10), GRF_GPIO2A_IOMUX);
		/* spi1 clk rxd txd */
		grf_writel((3 << 30) | (3 << 28) | (3 << 14) | (3 << 12), GRF_GPIO2A_IOMUX);
		grf_writel((3 << 24) | (3 << 8), GRF_GPIO2A_IOMUX);
		break;
	default:
		debug("spi id = %d iomux error!\n", spi_id);
		break;
	}
}

static void rk_uart_iomux_config(int uart_id)
{
	switch (uart_id) {
	case RK_UART_BT_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (1 << 2) | (1 << 0), GRF_GPIO3B_IOMUX);
		break;
	case RK_UART_DBG_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO5A_IOMUX);
		break;
	case RK_UART_GPS_IOMUX:
		grf_writel((3 << 30) | (1 << 14), GRF_GPIO5B_IOMUX);
		grf_writel((3 << 16) | (1 << 0), GRF_GPIO5C_IOMUX);
		break;
	default:
		debug("uart id = %d iomux error!\n", uart_id);
		break;
	}
}

static void rk_emmc_iomux_config(int emmc_id)
{
	switch (emmc_id) {
	case RK_EMMC_IOMUX:
		/* emmc data0-5 */
		grf_writel((0xFFF0 << 16) | 0xAAA0, GRF_GPIO2C_IOMUX);
		/* emmc data6-7 */
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO2D_IOMUX);
		/* emmc pwren, emmc cmd */
		grf_writel((3 << 22) | (3 << 20) | (2 << 6) | (2 << 4), GRF_GPIO2D_IOMUX);
		/* emmc rstn out, emmc clkout */
		grf_writel((3 << 24) | (3 << 22) | (2 << 8) | (2 << 6), GRF_GPIO3A_IOMUX);
		break;
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
		grf_writel((3 << 22) | (3 << 20) | (3 << 18) | (3 << 16) | (1 << 6) | (1 << 4) | (1 << 2) | (1 << 0), GRF_GPIO5A_IOMUX);
		/* sdcard dectn, cmd, clkout */
		grf_writel((3 << 26) | (3 << 24) | (1 << 10) | (1 << 8), GRF_GPIO5A_IOMUX);
		break;
	default:
		debug("sdcard id = %d iomux error!\n", sdcard_id);
		break;
	}
}

static void rk_hdmi_iomux_config(int hdmi_id)
{
	switch (hdmi_id) {
	default:
		debug("hdmi id = %d iomux error!\n", hdmi_id);
		break;
	}
}


#ifdef CONFIG_RK_SDCARD_BOOT_EN
#define RK_FORCE_SELECT_JTAG	(grf_readl(GRF_SOC_CON15) & (1 << 13))
static uint32 grf_gpio5a_iomux;

__maybe_unused
void rk_iomux_sdcard_save(void)
{
	debug("rk save sdcard iomux config.\n");
	grf_gpio5a_iomux = grf_readl(GRF_GPIO5A_IOMUX);
	grf_gpio5a_iomux &= ((3 << 10) | (3 << 8) | (3 << 6) | (3 << 4) | (3 << 2) | (3 << 0));
	debug("grf gpio5a iomux = 0x%08x\n", grf_gpio5a_iomux);

	if (RK_FORCE_SELECT_JTAG) {
		debug("Force select jtag from sdcard io.\n");
	}
}


__maybe_unused
void rk_iomux_sdcard_restore(void)
{
	debug("rk restore sdcard iomux config.\n");
	grf_writel((3 << 26) | (3 << 24) | (3 << 22) | (3 << 20) | (3 << 18) | (3 << 16) | grf_gpio5a_iomux, GRF_GPIO5A_IOMUX);
	debug("grf gpio5a iomux = 0x%08x\n", grf_readl(GRF_GPIO5A_IOMUX) & ((3 << 10) | (3 << 8) | (3 << 6) | (3 << 4) | (3 << 2) | (3 << 0)));
}
#endif /* CONFIG_RK_SDCARD_BOOT_EN */
