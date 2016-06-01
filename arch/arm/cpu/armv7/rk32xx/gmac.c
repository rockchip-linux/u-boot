/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <netdev.h>
#include <miiphy.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define GRF_BIT(nr)		(BIT(nr) | BIT(nr + 16))
#define GRF_CLR_BIT(nr)		(BIT(nr + 16))

#define HIWORD_UPDATE(val, mask, shift) \
		((val) << (shift) | (mask) << ((shift) + 16))

/* RK3288_GRF_SOC_CON1 */
/* RK3128_GRF_MAC_CON1 */
#define GMAC_PHY_INTF_SEL_RGMII ((0x01C0 << 16) | (0x0040))
#define GMAC_PHY_INTF_SEL_RMII  ((0x01C0 << 16) | (0x0100))
#define GMAC_FLOW_CTRL          ((0x0200 << 16) | (0x0200))
#define GMAC_FLOW_CTRL_CLR      ((0x0200 << 16) | (0x0000))
#define GMAC_SPEED_10M          ((0x0400 << 16) | (0x0000))
#define GMAC_SPEED_100M         ((0x0400 << 16) | (0x0400))
#define GMAC_RMII_CLK_25M       ((0x0800 << 16) | (0x0800))
#define GMAC_RMII_CLK_2_5M      ((0x0800 << 16) | (0x0000))
#define GMAC_CLK_125M           ((0x3000 << 16) | (0x0000))
#define GMAC_CLK_25M            ((0x3000 << 16) | (0x3000))
#define GMAC_CLK_2_5M           ((0x3000 << 16) | (0x2000))
#define GMAC_RMII_MODE          ((0x4000 << 16) | (0x4000))
#define GMAC_RMII_MODE_CLR      ((0x4000 << 16) | (0x0000))

/* RK3288_GRF_SOC_CON3 */
/* RK3128_GRF_MAC_CON0 */
#define GMAC_TXCLK_DLY_ENABLE   ((0x4000 << 16) | (0x4000))
#define GMAC_TXCLK_DLY_DISABLE  ((0x4000 << 16) | (0x0000))
#define GMAC_RXCLK_DLY_ENABLE   ((0x8000 << 16) | (0x8000))
#define GMAC_RXCLK_DLY_DISABLE  ((0x8000 << 16) | (0x0000))
#define GMAC_CLK_RX_DL_CFG(val) ((0x3F80 << 16) | (val<<7))
#define GMAC_CLK_TX_DL_CFG(val) ((0x007F << 16) | (val))

/* GMAC PHY mode */
#define GMAC_PHY_MODE_RMII	0
#define GMAC_PHY_MODE_RGMII	1

/* GMAC PHY speed */
#define GMAC_PHY_SPEED_10M	10
#define GMAC_PHY_SPEED_100M	100
#define GMAC_PHY_SPEED_1000M	1000

#ifdef CONFIG_RGMII
static void rk_gmac_set_rgmii_mode(int tx_delay, int rx_delay)
{
	debug("%s: tx delay=0x%x rx delay=0x%x\n", __func__, tx_delay, rx_delay);

#if defined(CONFIG_RKCHIP_RK3288)
	grf_writel(GMAC_PHY_INTF_SEL_RGMII, GRF_SOC_CON1);
	grf_writel(GMAC_RMII_MODE_CLR, GRF_SOC_CON1);
	grf_writel(GMAC_RXCLK_DLY_ENABLE, GRF_SOC_CON3);
	grf_writel(GMAC_TXCLK_DLY_ENABLE, GRF_SOC_CON3);
	grf_writel(GMAC_CLK_RX_DL_CFG(rx_delay), GRF_SOC_CON3);
	grf_writel(GMAC_CLK_TX_DL_CFG(tx_delay), GRF_SOC_CON3);
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	grf_writel(GMAC_PHY_INTF_SEL_RGMII, GRF_MAC_CON1);
	grf_writel(GMAC_RMII_MODE_CLR, GRF_MAC_CON1);
	grf_writel(GMAC_RXCLK_DLY_ENABLE, GRF_MAC_CON0);
	grf_writel(GMAC_TXCLK_DLY_ENABLE, GRF_MAC_CON0);
	grf_writel(GMAC_CLK_RX_DL_CFG(rx_delay), GRF_MAC_CON0);
	grf_writel(GMAC_CLK_TX_DL_CFG(tx_delay), GRF_MAC_CON0);
#endif
}

static void rk_gmac_set_rgmii_speed(uint32_t speed)
{
#if defined(CONFIG_RKCHIP_RK3288)
	switch (speed) {
	case GMAC_PHY_SPEED_10M:
		grf_writel(GMAC_CLK_2_5M, GRF_SOC_CON1);
		break;
	case GMAC_PHY_SPEED_100M:
		grf_writel(GMAC_CLK_25M, GRF_SOC_CON1);
		break;
	case GMAC_PHY_SPEED_1000M:
		grf_writel(GMAC_CLK_125M, GRF_SOC_CON1);
		break;
	default:
		printf("%s: ERROR: speed %d is not defined!\n", __func__, speed);
	}
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	switch (speed) {
	case GMAC_PHY_SPEED_10M:
		grf_writel(GMAC_CLK_2_5M, GRF_MAC_CON1);
		break;
	case GMAC_PHY_SPEED_100M:
		grf_writel(GMAC_CLK_2_5M, GRF_MAC_CON1);
		break;
	case GMAC_PHY_SPEED_1000M:
		grf_writel(GMAC_CLK_125M, GRF_MAC_CON1);
		break;
	default:
		printf("%s: ERROR: speed %d is not defined!\n", __func__, speed);
	}
#endif
}
#endif /* CONFIG_RGMII */

#ifdef CONFIG_RMII
static void rk_gmac_set_rmii_mode(void)
{
#if defined(CONFIG_RKCHIP_RK3288)
	grf_writel(GMAC_PHY_INTF_SEL_RMII, GRF_SOC_CON1);
	grf_writel(GMAC_RMII_MODE, GRF_SOC_CON1);
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	grf_writel(GMAC_PHY_INTF_SEL_RMII, GRF_MAC_CON1);
	grf_writel(GMAC_RMII_MODE, GRF_MAC_CON1);
#endif
}

static void rk_gmac_set_rmii_speed(uint32_t speed)
{
#if defined(CONFIG_RKCHIP_RK3288)
	switch (speed) {
	case GMAC_PHY_SPEED_10M:
		grf_writel(GMAC_RMII_CLK_2_5M, GRF_SOC_CON1);
		grf_writel(GMAC_SPEED_10M, GRF_SOC_CON1);
		break;
	case GMAC_PHY_SPEED_100M:
		grf_writel(GMAC_RMII_CLK_25M, GRF_SOC_CON1);
		grf_writel(GMAC_SPEED_100M, GRF_SOC_CON1);
		break;
	default:
		printf("%s: ERROR: speed %d is not defined!\n", __func__, speed);
	}
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	switch (speed) {
	case GMAC_PHY_SPEED_10M:
		grf_writel(GMAC_RMII_CLK_2_5M, GRF_MAC_CON1);
		grf_writel(GMAC_SPEED_10M, GRF_MAC_CON1);
		break;
	case GMAC_PHY_SPEED_100M:
		grf_writel(GMAC_RMII_CLK_25M, GRF_MAC_CON1);
		grf_writel(GMAC_SPEED_100M, GRF_MAC_CON1);
		break;
	default:
		printf("%s: ERROR: speed %d is not defined!\n", __func__, speed);
	}
#endif
}
#endif /* CONFIG_RMII */


#if defined(CONFIG_RKCHIP_RK3288)
	#define COMPAT_ROCKCHIP_GMAC  "rockchip,rk3288-gmac"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#define COMPAT_ROCKCHIP_GMAC  "rockchip,rk312x-gmac"
#elif defined(CONFIG_RKCHIP_RK322X)
	#define COMPAT_ROCKCHIP_GMAC  "rockchip,rk322x-gmac"
#else
	#error "PLS define gmac compatitle first!"
#endif

int rk_gmac_initialize(bd_t *bis)
{
	int tx_delay = 0x30, rx_delay = 0x10;

	debug("rk_gmac_initialize\n");

#ifdef CONFIG_OF_LIBFDT
	if (gd->fdt_blob) {
		int node;
		const void* blob = gd->fdt_blob;
		struct fdt_gpio_state reset_gpio;

		node = fdt_node_offset_by_compatible(blob, 0, COMPAT_ROCKCHIP_GMAC);
		if (node < 0) {
			printf("can't find dts node for %s\n", COMPAT_ROCKCHIP_GMAC);
			return -ENODEV;
		}

		tx_delay = fdtdec_get_int(blob, node, "tx_delay", -1);
		if (tx_delay == -1)
			tx_delay = 0x30;
		rx_delay = fdtdec_get_int(blob, node, "rx_delay", -1);
		if (tx_delay == -1)
			rx_delay = 0x10;
		printf("gmac tx_delay = 0x%x, rx_delay = 0x%x\n", tx_delay, rx_delay);

		if (fdtdec_decode_gpio(blob, node, "reset-gpio", &reset_gpio) == 0) {
			reset_gpio.flags = !(reset_gpio.flags & OF_GPIO_ACTIVE_LOW);
			printf("gmac reset gpio: 0x%08x, active level = %d\n", reset_gpio.gpio, reset_gpio.flags);

			gpio_direction_output(reset_gpio.gpio, reset_gpio.flags);
			mdelay(5);
			gpio_direction_output(reset_gpio.gpio, !reset_gpio.flags);
		}
	}
#endif

	rk_iomux_config(RK_GMAC_IOMUX);
#ifdef CONFIG_RGMII
	rkclk_set_gmac_clk(GMAC_PHY_MODE_RGMII);
	rk_gmac_set_rgmii_mode(tx_delay, rx_delay);
	rk_gmac_set_rgmii_speed(GMAC_PHY_SPEED_1000M);

	return designware_initialize(RKIO_GMAC_BASE, PHY_INTERFACE_MODE_RGMII);
#else
	rkclk_set_gmac_clk(GMAC_PHY_MODE_RMII);
	rk_gmac_set_rmii_mode();
	rk_gmac_set_rmii_speed(GMAC_PHY_SPEED_100M);

	return designware_initialize(RKIO_GMAC_BASE, PHY_INTERFACE_MODE_MII);
#endif
}
