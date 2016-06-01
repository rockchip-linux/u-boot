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
		grf_writel((1 << 20) | (1 << 4), GRF_GPIO0D_IOMUX);
		break;
	case RK_PWM1_IOMUX:
		grf_writel((1 << 22) | (1 << 6), GRF_GPIO0D_IOMUX);
		break;
	case RK_PWM2_IOMUX:
		grf_writel((1 << 24) | (1 << 8), GRF_GPIO0D_IOMUX);
		break;
	case RK_PWM3_IOMUX:
		grf_writel((1 << 20) | (1 << 4), GRF_GPIO3D_IOMUX);
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
		grf_writel((1 << 18) | (1 << 16) | (1 << 2) | (1 << 0), GRF_GPIO0A_IOMUX);
		break;
	case RK_I2C1_IOMUX:
		grf_writel((3 << 22) | (1 << 20) | (1 << 6) | (1 << 4), GRF_GPIO0A_IOMUX);
		break;
	case RK_I2C2_IOMUX:
		grf_writel((7 << 20) | (7 << 16) | (3 << 4) | (3 << 0), GRF_GPIO2C_IOMUX2);
		break;
	case RK_I2C3_IOMUX:
		grf_writel((3 << 30) | (3 << 28) | (1 << 14) | (1 << 12), GRF_GPIO0A_IOMUX);
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
		grf_writel(0x00550055, GRF_GPIO2B_IOMUX);
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
		grf_writel((3 << 28) | (3 << 26) | (3 << 22) | (3 << 18) | (2 << 12) | (2 << 10) | (2 << 6) | (2 << 2), GRF_GPIO0B_IOMUX);
		break;
	default:
		debug("spi id = %d iomux error!\n", spi_id);
		break;
	}
}

static void rk_uart_iomux_config(int uart_id)
{
	switch (uart_id) {
	case RK_UART0_IOMUX:
		grf_writel((3 << 22) | (3 << 20) | (2 << 6) | (2 << 4), GRF_GPIO2D_IOMUX);
		break;
	case RK_UART1_IOMUX:
		grf_writel((3 << 20) | (3 << 18) | (2 << 4) | (2 << 2), GRF_GPIO1B_IOMUX);
		break;
	case RK_UART2_IOMUX:
		grf_writel((3 << 22) | (3 << 20) | (2 << 6) | (2 << 4), GRF_GPIO1C_IOMUX);
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
		/* emmc data0-7 */
		grf_writel((0xFFFF << 16) | 0xAAAA, GRF_GPIO1D_IOMUX);
		/* emmc emmc rstn out, pwren, emmc clkout */
		/* note: here no iomux emmc cmd for maskrom has do it for rk3126 or rk3128 */
		grf_writel((3 << 18) | (3 << 26) | (3 << 30) | (2 << 2) | (2 << 10) | (2 << 14), GRF_GPIO2A_IOMUX);
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
		/* iomux sdcard pwren, cmd */
		grf_writel((3 << 28) | (3 << 30) | (1 << 12) | (1 << 14), GRF_GPIO1B_IOMUX);
		/* iomux sdcard d0 - d3, detn, clkout */
		grf_writel((0xFFF << 16) | 0x555, GRF_GPIO1C_IOMUX);
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
		/* iomux scl/ada/ */
		grf_writel((0xa000 | (0xa000 << 16)), GRF_GPIO0A_IOMUX);
		/* iomux hpd */
		grf_writel((0x4000 | (0x4000 << 16)), GRF_GPIO0B_IOMUX);
		/* iomux cec */
		grf_writel((0x0100 | (0x0100 << 16)), GRF_GPIO0C_IOMUX);
		break;
	default:
		debug("hdmi id = %d iomux error!\n", hdmi_id);
		break;
	}
}

#ifdef CONFIG_RK_GMAC
static void rk_gmac_iomux_config(int gmac_id)
{
	switch (gmac_id) {
	case RK_GMAC_IOMUX:
		/* txd0: gpio2c3, txd1: gpio2c2, txd2: gpio2c6, txd3: gpio2c7 */
		/* rxd0: gpio2c1, rxd1: gpio2c0, rxd2: gpio2c5, rxd3: gpio2c4 */
		/* mdc: gpio2d1, rxdv: gpio2b0, rxer:  gpio2b7, clk: gpio2b6 */
		/* txen: gpio2b5, mdio: gpio2b4, rxclk:  gpio2b3, crs: gpio2b2 */
		/* col: gpio2d0, txclk: gpio2b1 */

		/* gmac txd0 - txd3/rxd0 - rxd3 */
		grf_writel((0xFF << 16) | (0xFF << 0), GRF_GPIO2C_IOMUX);
		grf_writel((0x7777 << 16) | (0x4444 << 0), GRF_GPIO2C_IOMUX2);
		/* gmac rxer/clk/txen/mdio/rxclk/crs/txclk/rxdv/mdc, col not set */
		grf_writel((0xFFFF << 16) | (0xFFFF << 0), GRF_GPIO2B_IOMUX);
		grf_writel((0x3 << 18) | (0x3 << 2), GRF_GPIO2D_IOMUX);
		break;
	default:
		debug("gmac id = %d iomux error!\n", gmac_id);
		break;
	}
}
#endif /* CONFIG_RK_GMAC */

#ifdef CONFIG_RK_SDCARD_BOOT_EN
#define RK_FORCE_SELECT_JTAG	(grf_readl(GRF_SOC_CON0) & (1 << 8))
static uint32 grf_gpio1b_iomux = 0, grf_gpio1c_iomux = 0;

__maybe_unused
void rk_iomux_sdcard_save(void)
{
	debug("rk save sdcard iomux config.\n");
	grf_gpio1b_iomux = grf_readl(GRF_GPIO1B_IOMUX) & ((3 << 12) | (3 << 14));
	grf_gpio1c_iomux = grf_readl(GRF_GPIO1C_IOMUX) & 0xFFF;
	debug("grf gpio1b iomux = 0x%08x\n", grf_gpio1b_iomux);
	debug("grf gpio1c iomux = 0x%08x\n", grf_gpio1c_iomux);

	if (RK_FORCE_SELECT_JTAG) {
		debug("Force select jtag from sdcard io.\n");
	}
}


__maybe_unused
void rk_iomux_sdcard_restore(void)
{
	debug("rk restore sdcard iomux config.\n");
	grf_writel((3 << 28) | (3 << 30) | grf_gpio1b_iomux, GRF_GPIO1B_IOMUX);
	grf_writel((0xFFF << 16) | grf_gpio1c_iomux, GRF_GPIO1C_IOMUX);
	debug("grf gpio1b iomux = 0x%08x\n", grf_readl(GRF_GPIO1B_IOMUX) & ((3<<12) | (3<<14)));
	debug("grf gpio1c iomux = 0x%08x\n", grf_readl(GRF_GPIO1C_IOMUX) & 0xFFF);
}
#endif /* CONFIG_RK_SDCARD_BOOT_EN */
