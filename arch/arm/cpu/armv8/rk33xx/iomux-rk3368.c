/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
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
		grf_writel((3 << 16) | 2, GRF_GPIO3B_IOMUX);
		break;
	case RK_PWM1_IOMUX:
		pmugrf_writel((3 << 16) | 2, PMU_GRF_GPIO0B_IOMUX);
		break;
	case RK_PWM2_IOMUX:
		break;
	case RK_PWM3_IOMUX:
		grf_writel((3 << (12 + 16)) | (3 << 12), GRF_GPIO3D_IOMUX);
		break;
	case RK_VOP0_PWM_IOMUX:
		grf_writel((3 << 16) | 3, GRF_GPIO3B_IOMUX);
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
		pmugrf_writel((3 << 30) | (3 << 28) | (1 << 14) | (1 << 12), PMU_GRF_GPIO0A_IOMUX);
		break;
	case RK_I2C1_IOMUX:
		grf_writel((3 << 28) | (3 << 26) | (1 << 12) | (1 << 10), GRF_GPIO2C_IOMUX);
		break;
	case RK_I2C2_IOMUX:
		grf_writel((3 << 30) | (2 << 14), GRF_GPIO3D_IOMUX);
		pmugrf_writel((3 << 18) | (2 << 2), PMU_GRF_GPIO0B_IOMUX);
		break;
	case RK_I2C3_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (1 << 2) | 1, GRF_GPIO1C_IOMUX);
		break;
	case RK_I2C4_IOMUX:
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | 2, GRF_GPIO3D_IOMUX);
		break;
	case RK_I2C5_IOMUX:
		grf_writel((3 << 22) | (3 << 20) | (2 << 6) | (2 << 4), GRF_GPIO3D_IOMUX);
		break;
	default:
		debug("i2c id = %d iomux error!\n", i2c_id);
		break;
	}
}

static void rk_lcdc_iomux_config(int lcd_id)
{
	switch (lcd_id) {
	case RK_LCDC0_IOMUX:
		pmugrf_writel((0xff << (8 + 16)) | (0x55 << 8), PMU_GRF_GPIO0D_IOMUX);
		break;
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
		grf_writel((3 << 16) | (3 << 0), GRF_GPIO1D_IOMUX);
		/* spi0 clk rxd txd */
		grf_writel((3 << 30) | (3 << 28) | (3 << 14) | (3 << 12), GRF_GPIO1C_IOMUX);
		grf_writel((3 << 26) | (2 << 10), GRF_GPIO1D_IOMUX);
		break;
	case RK_SPI0_CS1_IOMUX:
		/* spi0 cs1 */
		grf_writel((3 << 18) | (3 << 2), GRF_GPIO1D_IOMUX);
		/* spi0 clk rxd txd */
		grf_writel((3 << 30) | (3 << 28) | (3 << 14) | (3 << 12), GRF_GPIO1C_IOMUX);
		grf_writel((3 << 26) | (2 << 10), GRF_GPIO1D_IOMUX);
		break;
	case RK_SPI1_CS0_IOMUX:
		/* spi1 cs0 */
		grf_writel((3 << 30) | (2 << 14), GRF_GPIO1B_IOMUX);
		/* spi1 clk rxd txd */
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO1C_IOMUX);
		grf_writel((3 << 28) | (2 << 12), GRF_GPIO1B_IOMUX);
		break;
	case RK_SPI1_CS1_IOMUX:
		/* spi1 cs1 */
		grf_writel((3 << 24) | (2 << 8), GRF_GPIO3D_IOMUX);
		/* spi1 clk rxd txd */
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO1C_IOMUX);
		grf_writel((3 << 28) | (2 << 12), GRF_GPIO1B_IOMUX);
		break;
	case RK_SPI2_CS0_IOMUX:
		/* spi2 cs0 */
		pmugrf_writel((3 << 26) | (2 << 10), PMU_GRF_GPIO0B_IOMUX);
		/* spi2 clk rxd txd */
		pmugrf_writel((3 << 24) | (3 << 22) | (3 << 20) | (2 << 8) | (2 << 6) | (2 << 4), PMU_GRF_GPIO0B_IOMUX);
		break;
	case RK_SPI2_CS1_IOMUX:
		/* spi2 clk rxd txd */
		pmugrf_writel((3 << 24) | (3 << 22) | (3 << 20) | (2 << 8) | (2 << 6) | (2 << 4), PMU_GRF_GPIO0B_IOMUX);
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
		grf_writel((0xff << 16) | 0x55, GRF_GPIO2D_IOMUX);
		break;
	case RK_UART_BB_IOMUX:
		pmugrf_writel((0xff00 << 16) | 0xff00, PMU_GRF_GPIO0C_IOMUX);
		break;
	case RK_UART_DBG_IOMUX:
		grf_writel((3 << 28) | (3 << 26) | (2 << 12) | (2 << 10), GRF_GPIO2A_IOMUX);
		break;
	case RK_UART_GPS_IOMUX:
		grf_writel((0xf << 16) | 0xa, GRF_GPIO3C_IOMUX);
		grf_writel((3 << 28) | (3 << 26) | (2 << 12) | (2 << 10), GRF_GPIO3D_IOMUX);
		break;
	case RK_UART_EXP_IOMUX:
		pmugrf_writel((0xff << 16) | 0xff, PMU_GRF_GPIO0D_IOMUX);
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
		grf_writel((0xfff0 << 16) | 0xaaa0, GRF_GPIO1C_IOMUX);
		/* emmc data6-7 */
		grf_writel((3 << 18) | (3 << 16) | (2 << 2) | (2 << 0), GRF_GPIO1D_IOMUX);
		/* emmc pwren, emmc cmd */
		grf_writel((3 << 22) | (3 << 20) | (2 << 6) | (2 << 4), GRF_GPIO1D_IOMUX);
		/* emmc rstn out, emmc clkout */
		grf_writel((3 << 24) | (3 << 22) | (2 << 8) | (2 << 6), GRF_GPIO2A_IOMUX);
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
		grf_writel((3 << 30) | (3 << 28) | (3 << 26) | (1 << 14) | (1 << 12) | (1 << 10), GRF_GPIO2A_IOMUX);
		grf_writel((3 << 16) | (1 << 0), GRF_GPIO2B_IOMUX);
		/* sdcard dectn, cmd, clkout */
		grf_writel((3 << 22) | (3 << 20) | (3 << 18) | (1 << 6) | (1 << 4) | (1 << 2), GRF_GPIO2B_IOMUX);
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
		/* hdmii2c_sda, hdmii2c_scl */
		grf_writel((3 << 22) | (3 << 20) | (1 << 6) | (1 << 4), GRF_GPIO3D_IOMUX);
		break;
	default:
		debug("hdmi id = %d iomux error!\n", hdmi_id);
		break;
	}
}


#ifdef CONFIG_RK_SDCARD_BOOT_EN
#define RK_FORCE_SELECT_JTAG	(grf_readl(GRF_SOC_CON15) & (1 << 13))
static uint32 grf_gpio2a_iomux, grf_gpio2b_iomux;

__maybe_unused
void rk_iomux_sdcard_save(void)
{
	debug("rk save sdcard iomux config.\n");
	grf_gpio2a_iomux = grf_readl(GRF_GPIO2A_IOMUX) & ((3 << 14) | (3 << 12) | (3 << 10));
	grf_gpio2b_iomux = grf_readl(GRF_GPIO2B_IOMUX) & ((3 << 6) | (3 << 4) | (3 << 2));
	debug("grf gpio2a iomux = 0x%08x\n", grf_gpio2a_iomux);
	debug("grf gpio2b iomux = 0x%08x\n", grf_gpio2b_iomux);

	if (RK_FORCE_SELECT_JTAG) {
		debug("Force select jtag from sdcard io.\n");
	}
}


__maybe_unused
void rk_iomux_sdcard_restore(void)
{
	debug("rk restore sdcard iomux config.\n");
	grf_writel((3 << 30) | (3 << 28) | (3 << 26) | grf_gpio2a_iomux, GRF_GPIO2A_IOMUX);
	grf_writel((3 << 22) | (3 << 20) | (3 << 18) | grf_gpio2b_iomux, GRF_GPIO2B_IOMUX);
	debug("grf gpio2a iomux = 0x%08x\n", grf_readl(GRF_GPIO2A_IOMUX) & ((3 << 14) | (3 << 12) | (3 << 10)));
	debug("grf gpio2b iomux = 0x%08x\n", grf_readl(GRF_GPIO2B_IOMUX) & ((3 << 6) | (3 << 4) | (3 << 2)));
}
#endif /* CONFIG_RK_SDCARD_BOOT_EN */
