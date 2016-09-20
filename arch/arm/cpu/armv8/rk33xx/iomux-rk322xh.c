/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 * William Zhang, SoftWare Engineering, <william.zhang@rock-chips.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/rkplat.h>


static void rk_pwm_iomux_config(int pwm_id)
{
	u8 shift;

	switch (pwm_id) {
	case RK_PWM0_IOMUX:
	case RK_PWM1_IOMUX:
	case RK_PWM2_IOMUX:
		shift = (pwm_id - RK_PWM0_IOMUX) * 2 + 8;
		grf_writel((3 << (shift + 16)) | (1 << shift), GRF_GPIO2A_IOMUX);
		break;
	case RK_PWM3_IOMUX:
		grf_writel((3 << (4 + 16)) | (1 << 4), GRF_GPIO2A_IOMUX);
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
		grf_writel((0xf << (0 + 16)) | (0x5 << 0), GRF_GPIO2D_IOMUX);
		break;
	case RK_I2C1_IOMUX:
		grf_writel((0xf << (8 + 16)) | (0xa << 8), GRF_GPIO2A_IOMUX);
		break;
	case RK_I2C2_IOMUX:
		grf_writel((0xf << (10 + 16)) | (0x5 << 10), GRF_GPIO2BL_IOMUX);
		break;
	case RK_I2C3_IOMUX:
		grf_writel((0xf << (10 + 16)) | (0xa << 10), GRF_GPIO0A_IOMUX);
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
	u8 cs_shift;
	u32 mux_solution = (grf_readl(GRF_COM_IOMUX) >> 4) & 0x3;

	switch (spi_id) {
	case RK_SPI0_CS0_IOMUX:
	case RK_SPI0_CS1_IOMUX:
		if (0 == mux_solution) {
			/* spi0 cs0 or cs1 */
			cs_shift = ((RK_SPI0_CS0_IOMUX == spi_id) ? 6 : 8);
			grf_writel((0x3 << (cs_shift + 16)) | (0x1 << cs_shift), GRF_GPIO2BL_IOMUX);
			/* spi0 clk rxd txd */
			grf_writel((0x3f << (0 + 16)) | (0x15 << 0), GRF_GPIO2BL_IOMUX);
		} else if (1 == mux_solution) {
			/* spi0 cs0 or cs1 */
			cs_shift = ((RK_SPI0_CS0_IOMUX == spi_id) ? 4 : 6);
			grf_writel((0x3 << (cs_shift + 16)) | (0x2 << cs_shift), GRF_GPIO3D_IOMUX);
			/* spi0 clk rxd txd */
			grf_writel((0x3 << (14 + 16)) | (0x2 << 14), GRF_GPIO3C_IOMUX);
			grf_writel((0xf << (0 + 16)) | (0xa << 0), GRF_GPIO3D_IOMUX);
		} else if (2 == mux_solution) {
			if (RK_SPI0_CS0_IOMUX == spi_id) {
				/* spi0 cs0 */
				grf_writel((0x7 << (0 + 16)) | (0x3 << 0), GRF_GPIO3BL_IOMUX);
				/* spi0 clk rxd txd */
				grf_writel((0x1ff << (0 + 16)) | (0x124 << 0), GRF_GPIO3AL_IOMUX);
			}
		} else {

		}

		break;
	default:
		debug("spi id = %d iomux error!\n", spi_id);
		break;
	}
}

static void rk_uart_iomux_config(int uart_id)
{
	u32 dbg_sel = (grf_readl(GRF_COM_IOMUX) >> 0) & 0x3;

	switch (uart_id) {
	case RK_UART_BT_IOMUX:
		grf_writel((0xff << 16) | 0x55, GRF_GPIO1B_IOMUX);
		break;
	case RK_UART_BB_IOMUX:
		grf_writel((0x7 << (12 + 16)) | (0x4 << 12), GRF_GPIO3AL_IOMUX);
		grf_writel((0x1f << (0 + 16)) | (0x124 << 0), GRF_GPIO3AH_IOMUX);
		break;
	case RK_UART_DBG_IOMUX:
		if (0 == dbg_sel) {
			/* uart2 m0 */
			grf_writel((0xf << 16) | 0xA, GRF_GPIO1A_IOMUX);
		} else if (1 == dbg_sel) {
			/* uart2 m1 */
			grf_writel((0xf << 16) | 0x5, GRF_GPIO2A_IOMUX);
		} else {

		}
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
		/* emmc data0 */
		grf_writel((0x3 << (14 + 16)) | (0x2 << 14), GRF_GPIO0A_IOMUX);
		/* emmc data[1~4] */
		grf_writel((0xff << (8 + 16)) | (0xaa << 8), GRF_GPIO2D_IOMUX);
		/* emmc data[5~7], pwr_en, clkout, rstn, cmd */
		grf_writel((0x3fff << (0 + 16)) | (0x2aaa << 0), GRF_GPIO3C_IOMUX);
		break;
	default:
		debug("emmc id = %d iomux error!\n", emmc_id);
		break;
	}
}

static void rk_sdcard_iomux_config(int sdcard_id)
{
	u32 pwr_en_sel = (grf_readl(GRF_COM_IOMUX) >> 7) & 0x1;

	switch (sdcard_id) {
	case RK_SDCARD_IOMUX:
		/* sdcard data[0~3], clkout, detn, cmd */
		grf_writel((0x3fff << (0 + 16)) | (0x1555 << 0), GRF_GPIO1A_IOMUX);
		if (0 == pwr_en_sel) {
			/* sdcard pwr_en m0 */
			grf_writel((0x3 << (14 + 16)) | (0x1 << 14), GRF_GPIO2A_IOMUX);
		} else {
			/* sdcard pwr_en m1 */
			grf_writel((0x3 << (12 + 16)) | (0x3 << 12), GRF_GPIO0D_IOMUX);
		}
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
		/* hdmi sda, scl, hdp, cec */
		grf_writel((0xff << (6 + 16)) | (0x55 << 6), GRF_GPIO0A_IOMUX);
		break;
	default:
		debug("hdmi id = %d iomux error!\n", hdmi_id);
		break;
	}
}


#ifdef CONFIG_RK_SDCARD_BOOT_EN
#define RK_FORCE_SELECT_JTAG	(grf_readl(GRF_SOC_CON4) & (1 << 12))
static uint32 grf_gpio1a_iomux;

__maybe_unused
void rk_iomux_sdcard_save(void)
{
	debug("rk save sdcard iomux config.\n");
	/* jtag_tclk/tms iomuxed with sdmmc0_d2/d3*/
	grf_gpio1a_iomux = grf_readl(GRF_GPIO1A_IOMUX) & ((3 << 6) | (3 << 4));
	debug("grf gpio1a iomux = 0x%08x\n", grf_gpio1a_iomux);

	if (RK_FORCE_SELECT_JTAG) {
		debug("Force select jtag from sdcard io.\n");
	}
}


__maybe_unused
void rk_iomux_sdcard_restore(void)
{
	debug("rk restore sdcard iomux config.\n");
	grf_writel((3 << (6 + 16)) | (3 << (4 + 16)) | grf_gpio1a_iomux, GRF_GPIO1A_IOMUX);
	debug("grf gpio1a iomux = 0x%08x\n", grf_readl(GRF_GPIO1A_IOMUX) & ((3 << 6) | (3 << 4)));
}
#endif /* CONFIG_RK_SDCARD_BOOT_EN */
