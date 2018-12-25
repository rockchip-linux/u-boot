/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <resource.h>
#include <asm/arch/rkplat.h>
#include <asm/unaligned.h>
#include <linux/list.h>
#include <linux/iopoll.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_phy.h"
#include "rockchip_mipi_dsi.h"

#define IS_DSI0(dsi)	((dsi)->id == 0)
#define IS_DSI1(dsi)	((dsi)->id == 1)

#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC	1000000L

#define BIT(x) (1 << (x))

#define DSI_VERSION			0x00
#define DSI_PWR_UP			0x04
#define RESET				0
#define POWERUP				BIT(0)

#define DSI_CLKMGR_CFG			0x08
#define TO_CLK_DIVIDSION(div)		(((div) & 0xff) << 8)
#define TX_ESC_CLK_DIVIDSION(div)	(((div) & 0xff) << 0)

#define DSI_DPI_VCID			0x0c
#define DPI_VID(vid)			(((vid) & 0x3) << 0)

#define DSI_DPI_COLOR_CODING		0x10
#define EN18_LOOSELY			BIT(8)
#define DPI_COLOR_CODING_16BIT_1	0x0
#define DPI_COLOR_CODING_16BIT_2	0x1
#define DPI_COLOR_CODING_16BIT_3	0x2
#define DPI_COLOR_CODING_18BIT_1	0x3
#define DPI_COLOR_CODING_18BIT_2	0x4
#define DPI_COLOR_CODING_24BIT		0x5

#define DSI_DPI_CFG_POL			0x14
#define COLORM_ACTIVE_LOW		BIT(4)
#define SHUTD_ACTIVE_LOW		BIT(3)
#define HSYNC_ACTIVE_LOW		BIT(2)
#define VSYNC_ACTIVE_LOW		BIT(1)
#define DATAEN_ACTIVE_LOW		BIT(0)

#define DSI_DPI_LP_CMD_TIM		0x18
#define OUTVACT_LPCMD_TIME(p)		(((p) & 0xff) << 16)
#define INVACT_LPCMD_TIME(p)		((p) & 0xff)
#define DSI_DBI_VCID			0x1c
#define DBI_VCID(x)			UPDATE(x, 1, 0)
#define DSI_DBI_CFG			0x20
#define DSI_DBI_CMDSIZE			0x28
#define DSI_PCKHDL_CFG			0x2c
#define EN_CRC_RX			BIT(4)
#define EN_ECC_RX			BIT(3)
#define EN_BTA				BIT(2)
#define EN_EOTP_RX			BIT(1)
#define EN_EOTP_TX			BIT(0)
#define DSI_MODE_CFG			0x34
#define CMD_VIDEO_MODE			BIT(0)
#define COMMAND_MODE			BIT(0)
#define VIDEO_MODE			0
#define DSI_VID_MODE_CFG		0x38
#define VPG_EN				BIT(16)
#define LP_CMD_EN			BIT(15)
#define FRAME_BTA_ACK			BIT(14)
#define LP_HFP_EN			BIT(13)
#define LP_HBP_EN			BIT(12)
#define LP_VACT_EN			BIT(11)
#define LP_VFP_EN			BIT(10)
#define LP_VBP_EN			BIT(9)
#define LP_VSA_EN			BIT(8)
#define VID_MODE_TYPE_BURST_SYNC_PULSES	0x0
#define VID_MODE_TYPE_BURST_SYNC_EVENTS	0x1
#define VID_MODE_TYPE_BURST		0x2

#define DSI_VID_PKT_SIZE		0x3c
#define VID_PKT_SIZE(p)			(((p) & 0x3fff) << 0)
#define VID_PKT_MAX_SIZE		0x3fff

#define DSI_VID_NUM_CHUMKS		0x40
#define DSI_VID_NULL_PKT_SIZE		0x44
#define DSI_VID_HSA_TIME		0x48
#define DSI_VID_HBP_TIME		0x4c
#define DSI_VID_HLINE_TIME		0x50
#define DSI_VID_VSA_LINES		0x54
#define DSI_VID_VBP_LINES		0x58
#define DSI_VID_VFP_LINES		0x5c
#define DSI_VID_VACTIVE_LINES		0x60
#define DSI_EDPI_CMD_SIZE		0x64
#define DSI_CMD_MODE_CFG		0x68
#define MAX_RD_PKT_SIZE			BIT(24)
#define DCS_LW_TX			BIT(19)
#define DCS_SR_0P_TX			BIT(18)
#define DCS_SW_1P_TX			BIT(17)
#define DCS_SW_0P_TX			BIT(16)
#define GEN_LW_TX			BIT(14)
#define GEN_SR_2P_TX			BIT(13)
#define GEN_SR_1P_TX			BIT(12)
#define GEN_SR_0P_TX			BIT(11)
#define GEN_SW_2P_TX			BIT(10)
#define GEN_SW_1P_TX			BIT(9)
#define GEN_SW_0P_TX			BIT(8)
#define ACK_RQST_EN			BIT(1)
#define TEAR_FX_EN			BIT(0)

#define DSI_GEN_HDR			0x6c
#define GEN_HDATA(data)			(((data) & 0xffff) << 8)
#define GEN_HDATA_MASK			(0xffff << 8)
#define GEN_HTYPE(type)			(((type) & 0xff) << 0)
#define GEN_HTYPE_MASK			0xff

#define DSI_GEN_PLD_DATA		0x70

#define DSI_CMD_PKT_STATUS		0x74
#define GEN_CMD_EMPTY			BIT(0)
#define GEN_CMD_FULL			BIT(1)
#define GEN_PLD_W_EMPTY			BIT(2)
#define GEN_PLD_W_FULL			BIT(3)
#define GEN_PLD_R_EMPTY			BIT(4)
#define GEN_PLD_R_FULL			BIT(5)
#define GEN_RD_CMD_BUSY			BIT(6)

#define DSI_TO_CNT_CFG			0x78
#define HSTX_TO_CNT(p)			(((p) & 0xffff) << 16)
#define LPRX_TO_CNT(p)			((p) & 0xffff)

#define DSI_BTA_TO_CNT			0x8c
#define DSI_LPCLK_CTRL			0x94
#define AUTO_CLKLANE_CTRL		BIT(1)
#define PHY_TXREQUESTCLKHS		BIT(0)

#define DSI_PHY_TMR_LPCLK_CFG		0x98
#define PHY_CLKHS2LP_TIME(lbcc)		(((lbcc) & 0x3ff) << 16)
#define PHY_CLKLP2HS_TIME(lbcc)		((lbcc) & 0x3ff)

#define DSI_PHY_TMR_CFG			0x9c
#define PHY_HS2LP_TIME(lbcc)		(((lbcc) & 0xff) << 24)
#define PHY_LP2HS_TIME(lbcc)		(((lbcc) & 0xff) << 16)
#define MAX_RD_TIME(lbcc)		((lbcc) & 0x7fff)

#define DSI_PHY_RSTZ			0xa0
#define PHY_DISFORCEPLL			0
#define PHY_ENFORCEPLL			BIT(3)
#define PHY_DISABLECLK			0
#define PHY_ENABLECLK			BIT(2)
#define PHY_RSTZ			0
#define PHY_UNRSTZ			BIT(1)
#define PHY_SHUTDOWNZ			0
#define PHY_UNSHUTDOWNZ			BIT(0)

#define DSI_PHY_IF_CFG			0xa4
#define N_LANES(n)			((((n) - 1) & 0x3) << 0)
#define PHY_STOP_WAIT_TIME(cycle)	(((cycle) & 0xff) << 8)

#define DSI_PHY_STATUS			0xb0
#define LOCK				BIT(0)
#define STOP_STATE_CLK_LANE		BIT(2)

#define DSI_PHY_TST_CTRL0		0xb4
#define PHY_TESTCLK			BIT(1)
#define PHY_UNTESTCLK			0
#define PHY_TESTCLR			BIT(0)
#define PHY_UNTESTCLR			0

#define DSI_PHY_TST_CTRL1		0xb8
#define PHY_TESTEN			BIT(16)
#define PHY_UNTESTEN			0
#define PHY_TESTDOUT(n)			(((n) & 0xff) << 8)
#define PHY_TESTDIN(n)			(((n) & 0xff) << 0)

#define DSI_INT_ST0			0xbc
#define DSI_INT_ST1			0xc0
#define DSI_INT_MSK0			0xc4
#define DSI_INT_MSK1			0xc8

#define PHY_STATUS_TIMEOUT_US		10000
#define CMD_PKT_STATUS_TIMEOUT_US	20000

#define BYPASS_VCO_RANGE	BIT(7)
#define VCO_RANGE_CON_SEL(val)	(((val) & 0x7) << 3)
#define VCO_IN_CAP_CON_DEFAULT	(0x0 << 1)
#define VCO_IN_CAP_CON_LOW	(0x1 << 1)
#define VCO_IN_CAP_CON_HIGH	(0x2 << 1)
#define REF_BIAS_CUR_SEL	BIT(0)

#define CP_CURRENT_3MA		BIT(3)
#define CP_PROGRAM_EN		BIT(7)
#define LPF_PROGRAM_EN		BIT(6)
#define LPF_RESISTORS_20_KOHM	0

#define HSFREQRANGE_SEL(val)	(((val) & 0x3f) << 1)

#define INPUT_DIVIDER(val)	((val - 1) & 0x7f)
#define LOW_PROGRAM_EN		0
#define HIGH_PROGRAM_EN		BIT(7)
#define LOOP_DIV_LOW_SEL(val)	((val - 1) & 0x1f)
#define LOOP_DIV_HIGH_SEL(val)	(((val - 1) >> 5) & 0x1f)
#define PLL_LOOP_DIV_EN		BIT(5)
#define PLL_INPUT_DIV_EN	BIT(4)

#define POWER_CONTROL		BIT(6)
#define INTERNAL_REG_CURRENT	BIT(3)
#define BIAS_BLOCK_ON		BIT(2)
#define BANDGAP_ON		BIT(0)

#define TER_RESISTOR_HIGH	BIT(7)
#define	TER_RESISTOR_LOW	0
#define LEVEL_SHIFTERS_ON	BIT(6)
#define TER_CAL_DONE		BIT(5)
#define SETRD_MAX		(0x7 << 2)
#define POWER_MANAGE		BIT(1)
#define TER_RESISTORS_ON	BIT(0)

#define BIASEXTR_SEL(val)	((val) & 0x7)
#define BANDGAP_SEL(val)	((val) & 0x7)
#define TLP_PROGRAM_EN		BIT(7)
#define THS_PRE_PROGRAM_EN	BIT(7)
#define THS_ZERO_PROGRAM_EN	BIT(6)

enum {
	BANDGAP_97_07,
	BANDGAP_98_05,
	BANDGAP_99_02,
	BANDGAP_100_00,
	BANDGAP_93_17,
	BANDGAP_94_15,
	BANDGAP_95_12,
	BANDGAP_96_10,
};

enum {
	BIASEXTR_87_1,
	BIASEXTR_91_5,
	BIASEXTR_95_9,
	BIASEXTR_100,
	BIASEXTR_105_94,
	BIASEXTR_111_88,
	BIASEXTR_118_8,
	BIASEXTR_127_7,
};

enum soc_type {
	RK3126,
	RK3288,
	RK3366,
	RK3368,
	RK3399,
};

#define GRF_REG_FIELD(reg, lsb, msb)	((reg << 16) | (lsb << 8) | (msb))

enum grf_reg_fields {
	DPIUPDATECFG,
	DPISHUTDN,
	DPICOLORM,
	VOPSEL,
	TURNREQUEST,
	TURNDISABLE,
	FORCETXSTOPMODE,
	FORCERXMODE,
	ENABLE_N,
	MASTERSLAVEZ,
	ENABLECLK,
	BASEDIR,
	MAX_FIELDS,
};

struct dw_mipi_dsi_plat_data {
	const u32 *dsi0_grf_reg_fields;
	const u32 *dsi1_grf_reg_fields;
	unsigned long max_bit_rate_per_lane;
	enum soc_type soc_type;
};

struct mipi_dphy {
	/* Non-SNPS PHY */
	const struct rockchip_phy *phy;

	u16 input_div;
	u16 feedback_div;
};

struct dw_mipi_dsi {
	u32 base;
	const void *blob;
	int node;
	int id;

	/* dual-channel */
	struct dw_mipi_dsi *master;
	struct dw_mipi_dsi *slave;

	unsigned int lane_mbps; /* per lane */
	u32 channel;
	u32 lanes;
	u32 format;
	u32 mode_flags;
	struct mipi_dphy dphy;
	struct drm_display_mode *mode;
	struct display_state *state;

	const struct dw_mipi_dsi_plat_data *pdata;
};

struct dphy_pll_testdin_map {
	unsigned int max_mbps;
	u8 testdin;
};

/* The table is based on 27MHz DPHY pll reference clock. */
static const struct dphy_pll_testdin_map dptdin_map[] = {
	{  90, 0x00}, { 100, 0x10}, { 110, 0x20}, { 130, 0x01},
	{ 140, 0x11}, { 150, 0x21}, { 170, 0x02}, { 180, 0x12},
	{ 200, 0x22}, { 220, 0x03}, { 240, 0x13}, { 250, 0x23},
	{ 270, 0x04}, { 300, 0x14}, { 330, 0x05}, { 360, 0x15},
	{ 400, 0x25}, { 450, 0x06}, { 500, 0x16}, { 550, 0x07},
	{ 600, 0x17}, { 650, 0x08}, { 700, 0x18}, { 750, 0x09},
	{ 800, 0x19}, { 850, 0x29}, { 900, 0x39}, { 950, 0x0a},
	{1000, 0x1a}, {1050, 0x2a}, {1100, 0x3a}, {1150, 0x0b},
	{1200, 0x1b}, {1250, 0x2b}, {1300, 0x3b}, {1350, 0x0c},
	{1400, 0x1c}, {1450, 0x2c}, {1500, 0x3c}
};

static int max_mbps_to_testdin(unsigned int max_mbps)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dptdin_map); i++)
		if (dptdin_map[i].max_mbps > max_mbps)
			return dptdin_map[i].testdin;

	return -EINVAL;
}

static inline void dsi_write(struct dw_mipi_dsi *dsi, u32 reg, u32 val)
{
	writel(val, dsi->base + reg);
}

static inline u32 dsi_read(struct dw_mipi_dsi *dsi, u32 reg)
{
	return readl(dsi->base + reg);
}

static inline void dsi_update_bits(struct dw_mipi_dsi *dsi,
				   u32 reg, u32 mask, u32 val)
{
	u32 orig, tmp;

	orig = dsi_read(dsi, reg);
	tmp = orig & ~mask;
	tmp |= val & mask;
	dsi_write(dsi, reg, tmp);
}

static void grf_field_write(struct dw_mipi_dsi *dsi, enum grf_reg_fields index,
			    unsigned int val)
{
	const u32 field = dsi->id ? dsi->pdata->dsi1_grf_reg_fields[index] :
				    dsi->pdata->dsi0_grf_reg_fields[index];
	u16 reg;
	u8 msb, lsb;

	if (!field)
		return;

	reg = (field >> 16) & 0xffff;
	lsb = (field >>  8) & 0xff;
	msb = (field >>  0) & 0xff;

	writel((val << lsb) | (GENMASK(msb, lsb) << 16), RKIO_GRF_PHYS + reg);
}

static inline void dpishutdn_assert(struct dw_mipi_dsi *dsi)
{
	grf_field_write(dsi, DPISHUTDN, 1);
}

static inline void dpishutdn_deassert(struct dw_mipi_dsi *dsi)
{
	grf_field_write(dsi, DPISHUTDN, 0);
}

static int genif_wait_w_pld_fifo_not_full(struct dw_mipi_dsi *dsi)
{
	u32 sts;
	int ret;

	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 sts, !(sts & GEN_PLD_W_FULL),
				 CMD_PKT_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("generic write payload fifo is full\n");
		return ret;
	}

	return 0;
}

static int genif_wait_cmd_fifo_not_full(struct dw_mipi_dsi *dsi)
{
	u32 sts;
	int ret;

	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 sts, !(sts & GEN_CMD_FULL),
				 CMD_PKT_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("generic write cmd fifo is full\n");
		return ret;
	}

	return 0;
}

static int genif_wait_write_fifo_empty(struct dw_mipi_dsi *dsi)
{
	u32 sts;
	u32 mask;
	int ret;

	mask = GEN_CMD_EMPTY | GEN_PLD_W_EMPTY;
	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 sts, (sts & mask) == mask,
				 CMD_PKT_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("generic write fifo is full\n");
		return ret;
	}

	return 0;
}

static void dw_mipi_dsi_phy_write(struct dw_mipi_dsi *dsi, u8 test_code,
				 u8 test_data)
{
	/*
	 * With the falling edge on TESTCLK, the TESTDIN[7:0] signal content
	 * is latched internally as the current test code. Test data is
	 * programmed internally by rising edge on TESTCLK.
	 */
	dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_TESTCLK | PHY_UNTESTCLR);

	dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_TESTEN | PHY_TESTDOUT(0) |
					  PHY_TESTDIN(test_code));

	dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_UNTESTCLK | PHY_UNTESTCLR);

	dsi_write(dsi, DSI_PHY_TST_CTRL1, PHY_UNTESTEN | PHY_TESTDOUT(0) |
					  PHY_TESTDIN(test_data));

	dsi_write(dsi, DSI_PHY_TST_CTRL0, PHY_TESTCLK | PHY_UNTESTCLR);
}

static int mipi_dphy_power_on(struct dw_mipi_dsi *dsi)
{
	u32 val;
	int ret;

	dsi_write(dsi, DSI_PHY_RSTZ, PHY_ENFORCEPLL | PHY_ENABLECLK |
				     PHY_UNRSTZ | PHY_UNSHUTDOWNZ);
	mdelay(2);

	if (dsi->dphy.phy)
		rockchip_phy_power_on(dsi->state);

	ret = readl_poll_timeout(dsi->base + DSI_PHY_STATUS,
				 val, val & LOCK, PHY_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("PHY is not locked\n");
		return ret;
	}

	ret = readl_poll_timeout(dsi->base + DSI_PHY_STATUS,
				 val, val & STOP_STATE_CLK_LANE,
				 PHY_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("lane module is not in stop state\n");
		return ret;
	}

	udelay(10);

	return 0;
}

static void mipi_dphy_pll_configure(struct dw_mipi_dsi *dsi)
{
	dw_mipi_dsi_phy_write(dsi, 0x17, INPUT_DIVIDER(dsi->dphy.input_div));
	dw_mipi_dsi_phy_write(dsi, 0x18,
			      LOOP_DIV_LOW_SEL(dsi->dphy.feedback_div) |
			      LOW_PROGRAM_EN);
	dw_mipi_dsi_phy_write(dsi, 0x19, PLL_LOOP_DIV_EN | PLL_INPUT_DIV_EN);
	dw_mipi_dsi_phy_write(dsi, 0x18,
			      LOOP_DIV_HIGH_SEL(dsi->dphy.feedback_div) |
			      HIGH_PROGRAM_EN);
	dw_mipi_dsi_phy_write(dsi, 0x19, PLL_LOOP_DIV_EN | PLL_INPUT_DIV_EN);
}

static int mipi_dphy_configure(struct dw_mipi_dsi *dsi)
{
	int testdin, vco;

	vco = (dsi->lane_mbps < 200) ? 0 : (dsi->lane_mbps + 100) / 200;

	testdin = max_mbps_to_testdin(dsi->lane_mbps);
	if (testdin < 0) {
		printf("failed to get testdin for %dmbps lane clock\n",
		       dsi->lane_mbps);
		return testdin;
	}

	dw_mipi_dsi_phy_write(dsi, 0x10, BYPASS_VCO_RANGE |
					 VCO_RANGE_CON_SEL(vco) |
					 VCO_IN_CAP_CON_LOW |
					 REF_BIAS_CUR_SEL);
	dw_mipi_dsi_phy_write(dsi, 0x11, CP_CURRENT_3MA);
	dw_mipi_dsi_phy_write(dsi, 0x12, CP_PROGRAM_EN | LPF_PROGRAM_EN |
					 LPF_RESISTORS_20_KOHM);
	dw_mipi_dsi_phy_write(dsi, 0x44, HSFREQRANGE_SEL(testdin));

	if (IS_DSI0(dsi))
		mipi_dphy_pll_configure(dsi);

	dw_mipi_dsi_phy_write(dsi, 0x20, POWER_CONTROL | INTERNAL_REG_CURRENT |
					 BIAS_BLOCK_ON | BANDGAP_ON);
	dw_mipi_dsi_phy_write(dsi, 0x21, TER_RESISTOR_LOW | TER_CAL_DONE |
					 SETRD_MAX | TER_RESISTORS_ON);
	dw_mipi_dsi_phy_write(dsi, 0x21, TER_RESISTOR_HIGH | LEVEL_SHIFTERS_ON |
					 SETRD_MAX | POWER_MANAGE |
					 TER_RESISTORS_ON);
	dw_mipi_dsi_phy_write(dsi, 0x22, LOW_PROGRAM_EN |
					 BIASEXTR_SEL(BIASEXTR_127_7));
	dw_mipi_dsi_phy_write(dsi, 0x22, HIGH_PROGRAM_EN |
					 BANDGAP_SEL(BANDGAP_96_10));
	dw_mipi_dsi_phy_write(dsi, 0x70, TLP_PROGRAM_EN | 0xf);
	dw_mipi_dsi_phy_write(dsi, 0x71, THS_PRE_PROGRAM_EN | 0x2d);
	dw_mipi_dsi_phy_write(dsi, 0x72, THS_ZERO_PROGRAM_EN | 0xa);

	return 0;
}

static unsigned long dw_mipi_dsi_get_lane_rate(struct dw_mipi_dsi *dsi)
{
	const struct drm_display_mode *mode = dsi->mode;
	unsigned long max_lane_rate = dsi->pdata->max_bit_rate_per_lane;
	unsigned long lane_rate;
	unsigned int value;
	int bpp, lanes;
	u64 tmp;

	/* optional override of the desired bandwidth */
	value = fdt_getprop_u32_default_node(dsi->blob, dsi->node, 0,
					     "rockchip,lane-rate", 0);
	if (value > 0)
		return value * USEC_PER_SEC;

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	if (bpp < 0)
		bpp = 24;

	lanes = dsi->slave ? dsi->lanes * 2 : dsi->lanes;
	tmp = (u64)mode->clock * 1000 * bpp;
	do_div(tmp, lanes);

	/* take 1 / 0.9, since mbps must big than bandwidth of RGB */
	tmp *= 10;
	do_div(tmp, 9);

	if (tmp > max_lane_rate)
		lane_rate = max_lane_rate;
	else
		lane_rate = tmp;

	return lane_rate;
}

static void dw_mipi_dsi_set_pll(struct dw_mipi_dsi *dsi, unsigned long rate)
{
	unsigned long fin, fout;
	unsigned long fvco_min, fvco_max, best_freq = 984000000;
	u8 min_prediv, max_prediv;
	u8 _prediv, best_prediv = 2;
	u16 _fbdiv, best_fbdiv = 82;
	u32 min_delta = ~0U;

	fin = 24000000;
	fout = rate;

	/* 5Mhz < Fref / N < 40MHz, 80MHz < Fvco < 1500Mhz */
	min_prediv = DIV_ROUND_UP(fin, 40000000);
	max_prediv = fin / 5000000;
	fvco_min = 80000000;
	fvco_max = 1500000000;

	for (_prediv = min_prediv; _prediv <= max_prediv; _prediv++) {
		u64 tmp, _fout;
		u32 delta;

		/* Fvco = Fref * M / N */
		tmp = (u64)fout * _prediv;
		do_div(tmp, fin);
		_fbdiv = tmp;

		/*
		 * Due to the use of a "by 2 pre-scaler," the range of the
		 * feedback multiplication value M is limited to even division
		 * numbers, and m must be greater than 12, less than 1000.
		 */
		if (_fbdiv <= 12 || _fbdiv >= 1000)
			continue;

		if (_fbdiv % 2)
			++_fbdiv;

		_fout = (u64)_fbdiv * fin;
		do_div(_fout, _prediv);

		if (_fout < fvco_min || _fout > fvco_max)
			continue;

		delta = abs(fout - _fout);
		if (!delta) {
			best_prediv = _prediv;
			best_fbdiv = _fbdiv;
			best_freq = _fout;
			break;
		} else if (delta < min_delta) {
			best_prediv = _prediv;
			best_fbdiv = _fbdiv;
			best_freq = _fout;
			min_delta = delta;
		}
	}

	dsi->lane_mbps = best_freq / USEC_PER_SEC;
	dsi->dphy.input_div = best_prediv;
	dsi->dphy.feedback_div = best_fbdiv;
	if (dsi->slave) {
		dsi->slave->lane_mbps = dsi->lane_mbps;
		dsi->slave->dphy.input_div = dsi->dphy.input_div;
		dsi->slave->dphy.feedback_div = dsi->dphy.feedback_div;
	}
}

static int dw_mipi_dsi_read_from_fifo(struct dw_mipi_dsi *dsi,
				      const struct mipi_dsi_msg *msg)
{
	u8 *payload = msg->rx_buf;
	u16 length;
	u32 val;
	int ret;

	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 val, !(val & GEN_RD_CMD_BUSY), 5000);
	if (ret) {
		printf("entire response isn't stored in the FIFO\n");
		return ret;
	}

	/* Receive payload */
	for (length = msg->rx_len; length; length -= 4) {
		ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
					 val, !(val & GEN_PLD_R_EMPTY), 5000);
		if (ret) {
			printf("Read payload FIFO is empty\n");
			return ret;
		}

		val = dsi_read(dsi, DSI_GEN_PLD_DATA);

		switch (length) {
		case 3:
			payload[2] = (val >> 16) & 0xff;
			/* Fall through */
		case 2:
			payload[1] = (val >> 8) & 0xff;
			/* Fall through */
		case 1:
			payload[0] = val & 0xff;
			return 0;
		}

		payload[0] = (val >>  0) & 0xff;
		payload[1] = (val >>  8) & 0xff;
		payload[2] = (val >> 16) & 0xff;
		payload[3] = (val >> 24) & 0xff;
		payload += 4;
	}

	return 0;
}

static int dw_mipi_dsi_turn_on_peripheral(struct dw_mipi_dsi *dsi)
{
	dpishutdn_assert(dsi);
	udelay(20);
	dpishutdn_deassert(dsi);

	return 0;
}

static int dw_mipi_dsi_shutdown_peripheral(struct dw_mipi_dsi *dsi)
{
	dpishutdn_deassert(dsi);
	udelay(20);
	dpishutdn_assert(dsi);

	return 0;
}

static ssize_t dw_mipi_dsi_transfer(struct dw_mipi_dsi *dsi,
				    const struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_packet packet;
	int ret;
	int val;

	if (dsi->mode_flags & MIPI_DSI_MODE_LPM) {
		dsi_update_bits(dsi, DSI_VID_MODE_CFG, LP_CMD_EN, LP_CMD_EN);
		dsi_update_bits(dsi, DSI_LPCLK_CTRL, PHY_TXREQUESTCLKHS, 0);
	} else {
		dsi_update_bits(dsi, DSI_VID_MODE_CFG, LP_CMD_EN, 0);
		dsi_update_bits(dsi, DSI_LPCLK_CTRL,
				PHY_TXREQUESTCLKHS, PHY_TXREQUESTCLKHS);
	}

	switch (msg->type) {
	case MIPI_DSI_SHUTDOWN_PERIPHERAL:
		return dw_mipi_dsi_shutdown_peripheral(dsi);
	case MIPI_DSI_TURN_ON_PERIPHERAL:
		return dw_mipi_dsi_turn_on_peripheral(dsi);
	case MIPI_DSI_DCS_SHORT_WRITE:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, DCS_SW_0P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				DCS_SW_0P_TX : 0);
		break;
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, DCS_SW_1P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				DCS_SW_1P_TX : 0);
		break;
	case MIPI_DSI_DCS_LONG_WRITE:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, DCS_LW_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				DCS_LW_TX : 0);
		break;
	case MIPI_DSI_DCS_READ:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, DCS_SR_0P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				DCS_SR_0P_TX : 0);
		break;
	case MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, MAX_RD_PKT_SIZE,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				MAX_RD_PKT_SIZE : 0);
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_SW_0P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_SW_0P_TX : 0);
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_SW_1P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_SW_1P_TX : 0);
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_SW_2P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_SW_2P_TX : 0);
		break;
	case MIPI_DSI_GENERIC_LONG_WRITE:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_LW_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_LW_TX : 0);
		break;
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_SR_0P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_SR_0P_TX : 0);
		break;
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_SR_1P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_SR_1P_TX : 0);
		break;
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, GEN_SR_2P_TX,
				dsi->mode_flags & MIPI_DSI_MODE_LPM ?
				GEN_SR_2P_TX : 0);
		break;
	default:
		return -EINVAL;
	}

	if (msg->flags & MIPI_DSI_MSG_REQ_ACK)
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG,
				ACK_RQST_EN, ACK_RQST_EN);

	/* create a packet to the DSI protocol */
	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		printf("failed to create packet: %d\n", ret);
		return ret;
	}

	/* Send payload,  */
	while (DIV_ROUND_UP(packet.payload_length, 4)) {
		/*
		 * Alternatively, you can always keep the FIFO
		 * nearly full by monitoring the FIFO state until
		 * it is not full, and then writea single word of data.
		 * This solution is more resource consuming
		 * but it simultaneously avoids FIFO starvation,
		 * making it possible to use FIFO sizes smaller than
		 * the amount of data of the longest packet to be written.
		 */
		ret = genif_wait_w_pld_fifo_not_full(dsi);
		if (ret)
			return ret;

		if (packet.payload_length < 4) {
			/* send residu payload */
			val = 0;
			memcpy(&val, packet.payload, packet.payload_length);
			dsi_write(dsi, DSI_GEN_PLD_DATA, val);
			packet.payload_length = 0;
		} else {
			val = get_unaligned_le32(packet.payload);
			dsi_write(dsi, DSI_GEN_PLD_DATA, val);
			packet.payload += 4;
			packet.payload_length -= 4;
		}
	}

	ret = genif_wait_cmd_fifo_not_full(dsi);
	if (ret)
		return ret;

	/* Send packet header */
	val = get_unaligned_le32(packet.header);
	dsi_write(dsi, DSI_GEN_HDR, val);

	ret = genif_wait_write_fifo_empty(dsi);
	if (ret)
		return ret;

	if (msg->rx_len) {
		ret = dw_mipi_dsi_read_from_fifo(dsi, msg);
		if (ret < 0)
			return ret;
	}

	if (dsi->slave) {
		ret = dw_mipi_dsi_transfer(dsi->slave, msg);
		if (ret) {
			printf("failed to send command through dsi slave, ret = %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static ssize_t dw_mipi_dsi_connector_transfer(struct display_state *state,
					      const struct mipi_dsi_msg *msg)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	return dw_mipi_dsi_transfer(dsi, msg);
}

static void dw_mipi_dsi_video_mode_config(struct dw_mipi_dsi *dsi)
{
	u32 val;

	val = LP_VACT_EN | LP_VFP_EN | LP_VBP_EN | LP_VSA_EN;

	if (!(dsi->mode_flags & MIPI_DSI_MODE_VIDEO_HFP))
		val |= LP_HFP_EN;

	if (!(dsi->mode_flags & MIPI_DSI_MODE_VIDEO_HBP))
		val |= LP_HBP_EN;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		val |= VID_MODE_TYPE_BURST;
	else if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		val |= VID_MODE_TYPE_BURST_SYNC_PULSES;
	else
		val |= VID_MODE_TYPE_BURST_SYNC_EVENTS;

	dsi_write(dsi, DSI_VID_MODE_CFG, val);

	if (dsi->mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS)
		dsi_update_bits(dsi, DSI_LPCLK_CTRL,
				AUTO_CLKLANE_CTRL, AUTO_CLKLANE_CTRL);
}

static void dw_mipi_dsi_enable(struct dw_mipi_dsi *dsi)
{
	const struct drm_display_mode *mode = dsi->mode;

	dsi_update_bits(dsi, DSI_LPCLK_CTRL,
			PHY_TXREQUESTCLKHS, PHY_TXREQUESTCLKHS);

	dsi_write(dsi, DSI_PWR_UP, RESET);

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO) {
		dsi_update_bits(dsi, DSI_MODE_CFG, CMD_VIDEO_MODE, VIDEO_MODE);
	} else {
		dsi_write(dsi, DSI_DBI_VCID, DBI_VCID(dsi->channel));
		dsi_update_bits(dsi, DSI_CMD_MODE_CFG, DCS_LW_TX, 0);
		dsi_write(dsi, DSI_EDPI_CMD_SIZE, mode->hdisplay);
		dsi_update_bits(dsi, DSI_MODE_CFG,
				CMD_VIDEO_MODE, COMMAND_MODE);
	}

	dsi_write(dsi, DSI_PWR_UP, POWERUP);

	if (dsi->slave)
		dw_mipi_dsi_enable(dsi->slave);
}

static void dw_mipi_dsi_disable(struct dw_mipi_dsi *dsi)
{
	dsi_write(dsi, DSI_PWR_UP, RESET);
	dsi_write(dsi, DSI_LPCLK_CTRL, 0);
	dsi_write(dsi, DSI_EDPI_CMD_SIZE, 0);
	dsi_update_bits(dsi, DSI_MODE_CFG, CMD_VIDEO_MODE, COMMAND_MODE);
	dsi_write(dsi, DSI_PWR_UP, POWERUP);

	if (dsi->slave)
		dw_mipi_dsi_disable(dsi->slave);
}

static void dw_mipi_dsi_post_disable(struct dw_mipi_dsi *dsi)
{
	dsi_write(dsi, DSI_PWR_UP, RESET);
	dsi_write(dsi, DSI_PHY_RSTZ, 0);

	if (dsi->slave)
		dw_mipi_dsi_post_disable(dsi->slave);
}

static void dw_mipi_dsi_init(struct dw_mipi_dsi *dsi)
{
	u32 esc_clk_div;

	dsi_write(dsi, DSI_PWR_UP, RESET);
	dsi_write(dsi, DSI_PHY_RSTZ, PHY_DISFORCEPLL | PHY_DISABLECLK
		  | PHY_RSTZ | PHY_SHUTDOWNZ);

	/* The maximum value of the escape clock frequency is 20MHz */
	esc_clk_div = DIV_ROUND_UP(dsi->lane_mbps >> 3, 20);
	dsi_write(dsi, DSI_CLKMGR_CFG, TO_CLK_DIVIDSION(10) |
		  TX_ESC_CLK_DIVIDSION(esc_clk_div));
}

static void dw_mipi_dsi_dpi_config(struct dw_mipi_dsi *dsi,
				   struct drm_display_mode *mode)
{
	u32 val = 0, color = 0;

	switch (dsi->format) {
	case MIPI_DSI_FMT_RGB888:
		color = DPI_COLOR_CODING_24BIT;
		break;
	case MIPI_DSI_FMT_RGB666:
		color = DPI_COLOR_CODING_18BIT_2 | EN18_LOOSELY;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		color = DPI_COLOR_CODING_18BIT_1;
		break;
	case MIPI_DSI_FMT_RGB565:
		color = DPI_COLOR_CODING_16BIT_1;
		break;
	}

	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		val |= VSYNC_ACTIVE_LOW;
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		val |= HSYNC_ACTIVE_LOW;

	dsi_write(dsi, DSI_DPI_VCID, DPI_VID(dsi->channel));
	dsi_write(dsi, DSI_DPI_COLOR_CODING, color);
	dsi_write(dsi, DSI_DPI_CFG_POL, val);
	dsi_write(dsi, DSI_DPI_LP_CMD_TIM, OUTVACT_LPCMD_TIME(4)
		  | INVACT_LPCMD_TIME(4));
}

static void dw_mipi_dsi_packet_handler_config(struct dw_mipi_dsi *dsi)
{
	dsi_write(dsi, DSI_PCKHDL_CFG, EN_CRC_RX | EN_ECC_RX | EN_BTA);
}

static void dw_mipi_dsi_video_packet_config(struct dw_mipi_dsi *dsi,
					    struct drm_display_mode *mode)
{
	int pkt_size;

	if (dsi->slave || dsi->master)
		pkt_size = VID_PKT_SIZE(mode->hdisplay / 2);
	else
		pkt_size = VID_PKT_SIZE(mode->hdisplay);

	dsi_write(dsi, DSI_VID_PKT_SIZE, pkt_size);
}

static void dw_mipi_dsi_command_mode_config(struct dw_mipi_dsi *dsi)
{
	dsi_write(dsi, DSI_TO_CNT_CFG, HSTX_TO_CNT(1000) | LPRX_TO_CNT(1000));
	dsi_write(dsi, DSI_BTA_TO_CNT, 0xd00);
}

/* Get lane byte clock cycles. */
static int dw_mipi_dsi_get_hcomponent_lbcc(struct dw_mipi_dsi *dsi,
					   u32 hcomponent)
{
	u32 lbcc;

	lbcc = hcomponent * dsi->lane_mbps * MSEC_PER_SEC / 8;

	if (dsi->mode->clock == 0) {
		printf("dsi mode clock is 0!\n");
		return 0;
	}

	return DIV_ROUND_CLOSEST(lbcc, dsi->mode->clock);
}

static void dw_mipi_dsi_line_timer_config(struct dw_mipi_dsi *dsi)
{
	int htotal, hsa, hbp, lbcc;
	struct drm_display_mode *mode = dsi->mode;

	htotal = mode->htotal;
	hsa = mode->hsync_end - mode->hsync_start;
	hbp = mode->htotal - mode->hsync_end;

	lbcc = dw_mipi_dsi_get_hcomponent_lbcc(dsi, htotal);
	dsi_write(dsi, DSI_VID_HLINE_TIME, lbcc);

	lbcc = dw_mipi_dsi_get_hcomponent_lbcc(dsi, hsa);
	dsi_write(dsi, DSI_VID_HSA_TIME, lbcc);

	lbcc = dw_mipi_dsi_get_hcomponent_lbcc(dsi, hbp);
	dsi_write(dsi, DSI_VID_HBP_TIME, lbcc);
}

static void dw_mipi_dsi_vertical_timing_config(struct dw_mipi_dsi *dsi)
{
	u32 vactive, vsa, vfp, vbp;
	struct drm_display_mode *mode = dsi->mode;

	vactive = mode->vdisplay;
	vsa = mode->vsync_end - mode->vsync_start;
	vfp = mode->vsync_start - mode->vdisplay;
	vbp = mode->vtotal - mode->vsync_end;

	dsi_write(dsi, DSI_VID_VACTIVE_LINES, vactive);
	dsi_write(dsi, DSI_VID_VSA_LINES, vsa);
	dsi_write(dsi, DSI_VID_VFP_LINES, vfp);
	dsi_write(dsi, DSI_VID_VBP_LINES, vbp);
}

static void dw_mipi_dsi_dphy_timing_config(struct dw_mipi_dsi *dsi)
{
	dsi_write(dsi, DSI_PHY_TMR_CFG, PHY_HS2LP_TIME(0x14)
		  | PHY_LP2HS_TIME(0x10) | MAX_RD_TIME(10000));

	dsi_write(dsi, DSI_PHY_TMR_LPCLK_CFG, PHY_CLKHS2LP_TIME(0x40)
		  | PHY_CLKLP2HS_TIME(0x40));
}

static void dw_mipi_dsi_dphy_interface_config(struct dw_mipi_dsi *dsi)
{
	dsi_write(dsi, DSI_PHY_IF_CFG, PHY_STOP_WAIT_TIME(0x20) |
		  N_LANES(dsi->lanes));
}

static void dw_mipi_dsi_clear_err(struct dw_mipi_dsi *dsi)
{
	dsi_read(dsi, DSI_INT_ST0);
	dsi_read(dsi, DSI_INT_ST1);
	dsi_write(dsi, DSI_INT_MSK0, 0);
	dsi_write(dsi, DSI_INT_MSK1, 0);
}

static int dw_mipi_dsi_dual_channel_probe(struct dw_mipi_dsi *master)
{
	int node0, node1;
	struct dw_mipi_dsi *slave = NULL;

	node0 = fdt_getprop_u32_default_node(master->blob, master->node, 0,
					       "rockchip,dual-channel", -1);
	if (node0 < 0)
		return 0;

	node1 = fdt_node_offset_by_phandle(master->blob, node0);
	if (node1 < 0) {
		printf("failed to find dsi slave node\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(master->blob, node1)) {
		printf("dsi slave node is not available\n");
		return -ENODEV;
	}

	slave = malloc(sizeof(*slave));
	if (!slave)
		return -ENOMEM;

	memset(slave, 0, sizeof(*slave));

	master->lanes /= 2;
	master->slave = slave;
	slave->master = master;

	slave->blob = master->blob;
	slave->node = node1;
	slave->base = fdtdec_get_addr_size_auto_noparent(slave->blob, node1,
							 "reg", 0, NULL);
	slave->pdata = master->pdata;
	slave->id = 1;
	slave->dphy.phy = master->dphy.phy;
	slave->lanes = master->lanes;
	slave->format = master->format;
	slave->mode_flags = master->mode_flags;
	slave->channel = master->channel;

	return 0;
}

static int dw_mipi_dsi_connector_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	const struct dw_mipi_dsi_plat_data *pdata = connector->data;
	int mipi_node = conn_state->node;
	struct dw_mipi_dsi *dsi;
	int panel;
	static int id = 0;
	int ret;

	dsi = malloc(sizeof(*dsi));
	if (!dsi)
		return -ENOMEM;
	memset(dsi, 0, sizeof(*dsi));

	dsi->base = fdtdec_get_addr_size_auto_noparent(state->blob, mipi_node,
						       "reg", 0, NULL);
	dsi->pdata = pdata;
	dsi->id = id++;
	dsi->blob = state->blob;
	dsi->node = mipi_node;
	dsi->dphy.phy = conn_state->phy;
	dsi->state = state;

	conn_state->private = dsi;
	conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;
	conn_state->color_space = V4L2_COLORSPACE_DEFAULT;
	conn_state->type = DRM_MODE_CONNECTOR_DSI;

	panel = fdt_subnode_offset(state->blob, mipi_node, "panel");
	if (panel < 0) {
		printf("failed to find panel node\n");
		return -1;
	}

#define FDT_GET_INT(val, name) \
	val = fdtdec_get_int(state->blob, panel, name, -1); \
	if (val < 0) { \
		printf("Can't get %s\n", name); \
		return -1; \
	}

	FDT_GET_INT(dsi->lanes, "dsi,lanes");
	FDT_GET_INT(dsi->format, "dsi,format");
	FDT_GET_INT(dsi->mode_flags, "dsi,flags");
	FDT_GET_INT(dsi->channel, "reg");

	ret = dw_mipi_dsi_dual_channel_probe(dsi);
	if (ret)
		return ret;

	if (dsi->slave)
		conn_state->output_type |= ROCKCHIP_OUTPUT_DSI_DUAL_CHANNEL;

	if (IS_DSI1(dsi))
		conn_state->output_type |= ROCKCHIP_OUTPUT_DSI_DUAL_LINK;

	return 0;
}

static void dw_mipi_dsi_connector_deinit(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	free(dsi);

	if (dsi->slave)
		free(dsi->slave);
}

static void dw_mipi_dsi_set_hs_clk(struct dw_mipi_dsi *dsi,
				   unsigned long rate)
{
	rate = rockchip_phy_set_pll(dsi->state, rate);
	dsi->lane_mbps = rate / USEC_PER_SEC;
}

static void dw_mipi_dsi_host_init(struct dw_mipi_dsi *dsi)
{
	dw_mipi_dsi_init(dsi);
	dw_mipi_dsi_dpi_config(dsi, dsi->mode);
	dw_mipi_dsi_packet_handler_config(dsi);
	dw_mipi_dsi_video_mode_config(dsi);
	dw_mipi_dsi_video_packet_config(dsi, dsi->mode);
	dw_mipi_dsi_command_mode_config(dsi);
	dsi_update_bits(dsi, DSI_MODE_CFG, CMD_VIDEO_MODE, COMMAND_MODE);
	dw_mipi_dsi_line_timer_config(dsi);
	dw_mipi_dsi_vertical_timing_config(dsi);
	dw_mipi_dsi_dphy_timing_config(dsi);
	dw_mipi_dsi_dphy_interface_config(dsi);
	dw_mipi_dsi_clear_err(dsi);
}

static void dw_mipi_dsi_vop_routing(struct dw_mipi_dsi *dsi, int vop_id)
{
	grf_field_write(dsi, VOPSEL, vop_id);
	if (dsi->slave)
		grf_field_write(dsi->slave, VOPSEL, vop_id);
}

static void mipi_dphy_init(struct dw_mipi_dsi *dsi)
{
	u32 map[] = {0x1, 0x3, 0x7, 0xf};

	/* Configures DPHY to work as a Master */
	grf_field_write(dsi, MASTERSLAVEZ, 1);

	/* Configures lane as TX */
	grf_field_write(dsi, BASEDIR, 0);

	/* Set all REQUEST inputs to zero */
	grf_field_write(dsi, TURNREQUEST, 0);
	grf_field_write(dsi, TURNDISABLE, 0);
	grf_field_write(dsi, FORCETXSTOPMODE, 0);
	grf_field_write(dsi, FORCERXMODE, 0);
	udelay(1);

	if (!dsi->dphy.phy)
		mipi_dphy_configure(dsi);

	/* Enable Data Lane Module */
	grf_field_write(dsi, ENABLE_N, map[dsi->lanes - 1]);

	/* Enable Clock Lane Module */
	grf_field_write(dsi, ENABLECLK, 1);
}

static void dw_mipi_dsi_pre_enable(struct dw_mipi_dsi *dsi)
{
	dw_mipi_dsi_host_init(dsi);
	mipi_dphy_init(dsi);
	mipi_dphy_power_on(dsi);
	dsi_write(dsi, DSI_PWR_UP, POWERUP);

	if (dsi->slave)
		dw_mipi_dsi_pre_enable(dsi->slave);
}

static int dw_mipi_dsi_connector_prepare(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct crtc_state *crtc_state = &state->crtc_state;
	struct dw_mipi_dsi *dsi = conn_state->private;
	unsigned long lane_rate;

	dsi->mode = &conn_state->mode;
	if (dsi->slave)
		dsi->slave->mode = dsi->mode;

	lane_rate = dw_mipi_dsi_get_lane_rate(dsi);

	if (dsi->dphy.phy)
		dw_mipi_dsi_set_hs_clk(dsi, lane_rate);
	else
		dw_mipi_dsi_set_pll(dsi, lane_rate);

	printf("final DSI-Link bandwidth: %u Mbps x %d\n",
	       dsi->lane_mbps, dsi->slave ? dsi->lanes * 2 : dsi->lanes);

	dw_mipi_dsi_vop_routing(dsi, crtc_state->crtc_id);
	dw_mipi_dsi_pre_enable(dsi);

	return 0;
}

static void dw_mipi_dsi_connector_unprepare(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	dw_mipi_dsi_post_disable(dsi);
}

static int dw_mipi_dsi_connector_enable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	dw_mipi_dsi_enable(dsi);

	return 0;
}

static int dw_mipi_dsi_connector_disable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	dw_mipi_dsi_disable(dsi);

	return 0;
}

static const u32 rk3128_dsi_grf_reg_fields[MAX_FIELDS] = {
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0150, 10, 13),
	[FORCERXMODE]		= GRF_REG_FIELD(0x0150,  9,  9),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0150,  8,  8),
	[DPICOLORM]		= GRF_REG_FIELD(0x0150,  5,  5),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0150,  4,  4),
};

const struct dw_mipi_dsi_plat_data rk3128_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3128_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RK3126,
};

static const u32 rk3288_dsi0_grf_reg_fields[MAX_FIELDS] = {
	[DPICOLORM]		= GRF_REG_FIELD(0x025c,  8,  8),
	[DPISHUTDN]		= GRF_REG_FIELD(0x025c,  7,  7),
	[VOPSEL]		= GRF_REG_FIELD(0x025c,  6,  6),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0264,  8, 11),
	[FORCERXMODE]		= GRF_REG_FIELD(0x0264,  4,  7),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0264,  0,  3),
	[TURNREQUEST]		= GRF_REG_FIELD(0x03a4,  8, 10),
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x03a8,  0,  0),
};

static const u32 rk3288_dsi1_grf_reg_fields[MAX_FIELDS] = {
	[DPICOLORM]		= GRF_REG_FIELD(0x025c, 11, 11),
	[DPISHUTDN]		= GRF_REG_FIELD(0x025c, 10, 10),
	[VOPSEL]		= GRF_REG_FIELD(0x025c,  9,  9),
	[ENABLE_N]		= GRF_REG_FIELD(0x0268, 12, 15),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0268,  8, 11),
	[FORCERXMODE]		= GRF_REG_FIELD(0x0268,  4,  7),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0268,  0,  3),
	[BASEDIR]		= GRF_REG_FIELD(0x027c, 15, 15),
	[MASTERSLAVEZ]		= GRF_REG_FIELD(0x027c, 14, 14),
	[ENABLECLK]		= GRF_REG_FIELD(0x027c, 12, 12),
	[TURNREQUEST]		= GRF_REG_FIELD(0x03a4,  4,  7),
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x03a8,  1,  1),
};

const struct dw_mipi_dsi_plat_data rk3288_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3288_dsi0_grf_reg_fields,
	.dsi1_grf_reg_fields = rk3288_dsi1_grf_reg_fields,
	.max_bit_rate_per_lane = 1500000000UL,
	.soc_type = RK3288,
};

static const u32 rk3366_dsi_grf_reg_fields[MAX_FIELDS] = {
	[VOPSEL]		= GRF_REG_FIELD(0x0400,  2,  2),
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x0410,  9,  9),
	[DPICOLORM]		= GRF_REG_FIELD(0x0410,  3,  3),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0410,  2,  2),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0414,  7, 10),
	[FORCERXMODE]		= GRF_REG_FIELD(0x0414,  6,  6),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0414,  5,  5),
};

const struct dw_mipi_dsi_plat_data rk3366_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3366_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RK3366,
};

static const u32 rk3368_dsi_grf_reg_fields[MAX_FIELDS] = {
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x0418,  7,  7),
	[DPICOLORM]		= GRF_REG_FIELD(0x0418,  3,  3),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0418,  2,  2),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x041c,  7, 10),
	[FORCERXMODE]		= GRF_REG_FIELD(0x041c,  6,  6),
	[TURNDISABLE]		= GRF_REG_FIELD(0x041c,  5,  5),
};

const struct dw_mipi_dsi_plat_data rk3368_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3368_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RK3368,
};

static const u32 rk3399_dsi0_grf_reg_fields[MAX_FIELDS] = {
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x6224, 15, 15),
	[DPISHUTDN]		= GRF_REG_FIELD(0x6224, 14, 14),
	[DPICOLORM]		= GRF_REG_FIELD(0x6224, 13, 13),
	[VOPSEL]		= GRF_REG_FIELD(0x6250,  0,  0),
	[TURNREQUEST]		= GRF_REG_FIELD(0x6258, 12, 15),
	[TURNDISABLE]		= GRF_REG_FIELD(0x6258,  8, 11),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x6258,  4,  7),
	[FORCERXMODE]		= GRF_REG_FIELD(0x6258,  0,  3),
};

static const u32 rk3399_dsi1_grf_reg_fields[MAX_FIELDS] = {
	[VOPSEL]		= GRF_REG_FIELD(0x6250,  4,  4),
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x6250,  3,  3),
	[DPISHUTDN]		= GRF_REG_FIELD(0x6250,  2,  2),
	[DPICOLORM]		= GRF_REG_FIELD(0x6250,  1,  1),
	[TURNDISABLE]		= GRF_REG_FIELD(0x625c, 12, 15),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x625c,  8, 11),
	[FORCERXMODE]		= GRF_REG_FIELD(0x625c,  4,  7),
	[ENABLE_N]		= GRF_REG_FIELD(0x625c,  0,  3),
	[MASTERSLAVEZ]		= GRF_REG_FIELD(0x6260,  7,  7),
	[ENABLECLK]		= GRF_REG_FIELD(0x6260,  6,  6),
	[BASEDIR]		= GRF_REG_FIELD(0x6260,  5,  5),
	[TURNREQUEST]		= GRF_REG_FIELD(0x6260,  0,  3),
};

const struct dw_mipi_dsi_plat_data rk3399_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3399_dsi0_grf_reg_fields,
	.dsi1_grf_reg_fields = rk3399_dsi1_grf_reg_fields,
	.max_bit_rate_per_lane = 1500000000UL,
	.soc_type = RK3399,
};

const struct rockchip_connector_funcs rockchip_dw_mipi_dsi_funcs = {
	.init = dw_mipi_dsi_connector_init,
	.deinit = dw_mipi_dsi_connector_deinit,
	.prepare = dw_mipi_dsi_connector_prepare,
	.unprepare = dw_mipi_dsi_connector_unprepare,
	.enable = dw_mipi_dsi_connector_enable,
	.disable = dw_mipi_dsi_connector_disable,
	.transfer = dw_mipi_dsi_connector_transfer,
};
