/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <linux/list.h>
#include <dm/device.h>
#include <dm/read.h>
#include <dm/of_access.h>
#include <syscon.h>
#include <asm/arch-rockchip/clock.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_phy.h"
#include "rockchip_mipi_dsi.h"

#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC	1000000L

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

#define DSI_DBI_CFG			0x20
#define DSI_DBI_CMDSIZE			0x28

#define DSI_PCKHDL_CFG			0x2c
#define EN_CRC_RX			BIT(4)
#define EN_ECC_RX			BIT(3)
#define EN_BTA				BIT(2)
#define EN_EOTP_RX			BIT(1)
#define EN_EOTP_TX			BIT(0)

#define DSI_MODE_CFG			0x34
#define ENABLE_VIDEO_MODE		0
#define ENABLE_CMD_MODE			BIT(0)

#define DSI_VID_MODE_CFG		0x38
#define VPG_EN				BIT(16)
#define FRAME_BTA_ACK			BIT(14)
#define LP_HFP_EN			BIT(13)
#define LP_HBP_EN			BIT(12)
#define ENABLE_LOW_POWER		(0xf << 8)
#define ENABLE_LOW_POWER_MASK		(0xf << 8)
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
#define DSI_CMD_MODE_CFG		0x68
#define MAX_RD_PKT_SIZE_LP		BIT(24)
#define DCS_LW_TX_LP			BIT(19)
#define DCS_SR_0P_TX_LP			BIT(18)
#define DCS_SW_1P_TX_LP			BIT(17)
#define DCS_SW_0P_TX_LP			BIT(16)
#define GEN_LW_TX_LP			BIT(14)
#define GEN_SR_2P_TX_LP			BIT(13)
#define GEN_SR_1P_TX_LP			BIT(12)
#define GEN_SR_0P_TX_LP			BIT(11)
#define GEN_SW_2P_TX_LP			BIT(10)
#define GEN_SW_1P_TX_LP			BIT(9)
#define GEN_SW_0P_TX_LP			BIT(8)
#define EN_ACK_RQST			BIT(1)
#define EN_TEAR_FX			BIT(0)

#define CMD_MODE_ALL_LP			(MAX_RD_PKT_SIZE_LP | \
					 DCS_LW_TX_LP | \
					 DCS_SR_0P_TX_LP | \
					 DCS_SW_1P_TX_LP | \
					 DCS_SW_0P_TX_LP | \
					 GEN_LW_TX_LP | \
					 GEN_SR_2P_TX_LP | \
					 GEN_SR_1P_TX_LP | \
					 GEN_SR_0P_TX_LP | \
					 GEN_SW_2P_TX_LP | \
					 GEN_SW_1P_TX_LP | \
					 GEN_SW_0P_TX_LP)

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
	PX30,
	RK3128,
	RK3288,
	RK3366,
	RK3368,
	RK3399,
	RV1108,
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
	void *base;
	void *grf;
	const void *blob;
	ofnode node;
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

	const struct dw_mipi_dsi_plat_data *pdata;
};

enum dw_mipi_dsi_mode {
	DSI_COMMAND_MODE,
	DSI_VIDEO_MODE,
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

	rk_clrsetreg(dsi->grf + reg, GENMASK(msb, lsb), val << lsb);
}

static int rockchip_wait_w_pld_fifo_not_full(struct dw_mipi_dsi *dsi)
{
	u32 sts;
	int ret;

	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 sts, !(sts & GEN_PLD_W_FULL), 10,
				 CMD_PKT_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("generic write payload fifo is full\n");
		return ret;
	}

	return 0;
}

static int rockchip_wait_cmd_fifo_not_full(struct dw_mipi_dsi *dsi)
{
	u32 sts;
	int ret;

	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 sts, !(sts & GEN_CMD_FULL), 10,
				 CMD_PKT_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("generic write cmd fifo is full\n");
		return ret;
	}

	return 0;
}

static int rockchip_wait_write_fifo_empty(struct dw_mipi_dsi *dsi)
{
	u32 sts;
	u32 mask;
	int ret;

	mask = GEN_CMD_EMPTY | GEN_PLD_W_EMPTY;
	ret = readl_poll_timeout(dsi->base + DSI_CMD_PKT_STATUS,
				 sts, (sts & mask) == mask, 10,
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

static int dw_mipi_dsi_phy_init(struct dw_mipi_dsi *dsi)
{
	int ret, testdin, vco, val;

	vco = (dsi->lane_mbps < 200) ? 0 : (dsi->lane_mbps + 100) / 200;

	testdin = max_mbps_to_testdin(dsi->lane_mbps);
	if (testdin < 0) {
		printf("failed to get testdin for %dmbps lane clock\n",
		       dsi->lane_mbps);
		return testdin;
	}

	dsi_write(dsi, DSI_PWR_UP, POWERUP);

	dw_mipi_dsi_phy_write(dsi, 0x10, BYPASS_VCO_RANGE |
					 VCO_RANGE_CON_SEL(vco) |
					 VCO_IN_CAP_CON_LOW |
					 REF_BIAS_CUR_SEL);

	dw_mipi_dsi_phy_write(dsi, 0x11, CP_CURRENT_3MA);
	dw_mipi_dsi_phy_write(dsi, 0x12, CP_PROGRAM_EN | LPF_PROGRAM_EN |
					 LPF_RESISTORS_20_KOHM);

	dw_mipi_dsi_phy_write(dsi, 0x44, HSFREQRANGE_SEL(testdin));

	dw_mipi_dsi_phy_write(dsi, 0x17, INPUT_DIVIDER(dsi->dphy.input_div));
	val = LOOP_DIV_LOW_SEL(dsi->dphy.feedback_div) | LOW_PROGRAM_EN;
	dw_mipi_dsi_phy_write(dsi, 0x18, val);
	dw_mipi_dsi_phy_write(dsi, 0x19, PLL_LOOP_DIV_EN | PLL_INPUT_DIV_EN);
	val = LOOP_DIV_HIGH_SEL(dsi->dphy.feedback_div) | HIGH_PROGRAM_EN;
	dw_mipi_dsi_phy_write(dsi, 0x18, val);
	dw_mipi_dsi_phy_write(dsi, 0x19, PLL_LOOP_DIV_EN | PLL_INPUT_DIV_EN);

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

	dsi_write(dsi, DSI_PHY_RSTZ, PHY_ENFORCEPLL | PHY_ENABLECLK |
				     PHY_UNRSTZ | PHY_UNSHUTDOWNZ);

	ret = readl_poll_timeout(dsi->base + DSI_PHY_STATUS,
				 val, val & LOCK, 1000, PHY_STATUS_TIMEOUT_US);
	if (ret < 0) {
		printf("failed to wait for phy lock state %p\n", dsi->base);
		return ret;
	}

	ret = readl_poll_timeout(dsi->base + DSI_PHY_STATUS,
				 val, val & STOP_STATE_CLK_LANE, 1000,
				 PHY_STATUS_TIMEOUT_US);
	if (ret < 0)
		printf("failed to wait for phy clk lane stop state\n");

	return ret;
}

static unsigned long rockchip_dsi_calc_bandwidth(struct dw_mipi_dsi *dsi)
{
	int bpp;
	unsigned long mpclk, tmp;
	unsigned long target_mbps = 1000;
	unsigned int max_mbps;
	int lanes;
	int rate;

	/* optional override of the desired bandwidth */
	rate = ofnode_read_u32_default(dsi->node, "rockchip,lane-rate", -1);
	if (rate > 0) {
		return rate;
	}
	max_mbps = dsi->pdata->max_bit_rate_per_lane / USEC_PER_SEC;

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	if (bpp < 0) {
		printf("failed to get bpp for pixel format %d\n",
			dsi->format);
		bpp = 24;
	}

	lanes = dsi->slave ? dsi->lanes * 2 : dsi->lanes;

	mpclk = DIV_ROUND_UP(dsi->mode->clock, MSEC_PER_SEC);
	if (mpclk) {
		/* take 1 / 0.9, since mbps must big than bandwidth of RGB */
		tmp = mpclk * (bpp / lanes) * 10 / 9;
		if (tmp < max_mbps)
			target_mbps = tmp;
		else
			printf("DPHY clock frequency is out of range\n");
	}

	return target_mbps;
}

static int dw_mipi_dsi_get_lane_bps(struct dw_mipi_dsi *dsi)
{
	unsigned int i, pre;
	unsigned long pllref, tmp;
	unsigned int m = 1, n = 1;
	unsigned long target_mbps;

	if (dsi->master)
		return 0;

	target_mbps = rockchip_dsi_calc_bandwidth(dsi);

	/* ref clk : 24MHz*/
	pllref = 24;
	tmp = pllref;

	for (i = 1; i < 6; i++) {
		pre = pllref / i;
		if ((tmp > (target_mbps % pre)) && (target_mbps / pre < 512)) {
			tmp = target_mbps % pre;
			n = i;
			m = target_mbps / pre;
		}
		if (tmp == 0)
			break;
	}

	dsi->lane_mbps = pllref / n * m;
	dsi->dphy.input_div = n;
	dsi->dphy.feedback_div = m;
	if (dsi->slave) {
		dsi->slave->lane_mbps = dsi->lane_mbps;
		dsi->slave->dphy.input_div = n;
		dsi->slave->dphy.feedback_div = m;
	}

	return 0;
}

static void rockchip_set_transfer_mode(struct dw_mipi_dsi *dsi, int flags)
{
	if (flags & MIPI_DSI_MSG_USE_LPM) {
		dsi_write(dsi, DSI_CMD_MODE_CFG, CMD_MODE_ALL_LP);
		dsi_write(dsi, DSI_LPCLK_CTRL, 0);
	} else {
		dsi_write(dsi, DSI_CMD_MODE_CFG, 0);
		dsi_write(dsi, DSI_LPCLK_CTRL, PHY_TXREQUESTCLKHS);
	}
}

static ssize_t rockchip_dsi_send_packet(struct dw_mipi_dsi *dsi,
					const struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_packet packet;
	int ret;
	int val;

	/* create a packet to the DSI protocol */
	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		printf("failed to create packet: %d\n", ret);
		return ret;
	}

	rockchip_set_transfer_mode(dsi, msg->flags);

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
		ret = rockchip_wait_w_pld_fifo_not_full(dsi);
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

	ret = rockchip_wait_cmd_fifo_not_full(dsi);
	if (ret)
		return ret;

	/* Send packet header */
	val = get_unaligned_le32(packet.header);
	dsi_write(dsi, DSI_GEN_HDR, val);

	ret = rockchip_wait_write_fifo_empty(dsi);
	if (ret)
		return ret;

	if (dsi->slave) {
		ret = rockchip_dsi_send_packet(dsi->slave, msg);
		if (ret) {
			printf("failed to send command through dsi slave, ret = %d\n", ret);
			return ret;
		}
	}
	return 0;
}

static ssize_t rockchip_dw_mipi_dsi_transfer(struct display_state *state,
					     const struct mipi_dsi_msg *msg)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	return rockchip_dsi_send_packet(dsi, msg);
}

static void dw_mipi_dsi_video_mode_config(struct dw_mipi_dsi *dsi)
{
	u32 val;

	val = LP_HFP_EN | ENABLE_LOW_POWER;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		val |= VID_MODE_TYPE_BURST;
	else if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		val |= VID_MODE_TYPE_BURST_SYNC_PULSES;
	else
		val |= VID_MODE_TYPE_BURST_SYNC_EVENTS;

	dsi_write(dsi, DSI_VID_MODE_CFG, val);
}

static void dw_mipi_dsi_set_mode(struct dw_mipi_dsi *dsi,
				 enum dw_mipi_dsi_mode mode)
{
	if (mode == DSI_COMMAND_MODE) {
		dsi_write(dsi, DSI_MODE_CFG, ENABLE_CMD_MODE);
	} else {
		dsi_write(dsi, DSI_PWR_UP, RESET);
		dsi_write(dsi, DSI_LPCLK_CTRL, PHY_TXREQUESTCLKHS);
		dsi_write(dsi, DSI_MODE_CFG, ENABLE_VIDEO_MODE);
		dsi_write(dsi, DSI_PWR_UP, POWERUP);
	}
}

static void dw_mipi_dsi_disable(struct dw_mipi_dsi *dsi)
{
	dw_mipi_dsi_set_mode(dsi, DSI_COMMAND_MODE);

	/* host */
	dsi_write(dsi, DSI_LPCLK_CTRL, 0);
	dsi_write(dsi, DSI_PWR_UP, RESET);

	/* phy */
	dsi_write(dsi, DSI_PHY_RSTZ, PHY_RSTZ);

	if (dsi->slave)
		dw_mipi_dsi_disable(dsi->slave);
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

	if (dsi->pdata->soc_type == RK3128)
		val ^= VSYNC_ACTIVE_LOW;

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
		pkt_size = VID_PKT_SIZE(mode->hdisplay / 2 + 4);
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

static int rockchip_dsi_dual_channel_probe(struct dw_mipi_dsi *master)
{
	int phandle;
	struct device_node *np;
	struct dw_mipi_dsi *slave = NULL;

	phandle = ofnode_read_u32_default(master->node, "rockchip,dual-channel", -1);
	if (phandle < 0)
		return 0;

	np = of_find_node_by_phandle(phandle);
	if (ofnode_valid(np_to_ofnode(np))) {
		printf("failed to find dsi slave node\n");
		return -ENODEV;
	}

	if (!of_device_is_available(np)) {
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
	slave->node = np_to_ofnode(np);
	slave->base = (u32 *)ofnode_get_addr_index(slave->node, 0);
	slave->pdata = master->pdata;
	slave->id = 1;
	slave->dphy.phy = master->dphy.phy;
	slave->lanes = master->lanes;
	slave->format = master->format;
	slave->mode_flags = master->mode_flags;
	slave->channel = master->channel;

	return 0;
}

static int rockchip_dw_mipi_dsi_init(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	const struct dw_mipi_dsi_plat_data *pdata = connector->data;
	ofnode mipi_node = conn_state->node;
	struct dw_mipi_dsi *dsi;
	ofnode panel;
	int id;
	int ret;

	dsi = malloc(sizeof(*dsi));
	if (!dsi)
		return -ENOMEM;
	memset(dsi, 0, sizeof(*dsi));

	dsi->base = dev_read_addr_ptr(conn_state->dev);
	dsi->grf = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
	if (dsi->grf <= 0) {
		printf("%s: Get syscon grf failed (ret=%p)\n",
			__func__, dsi->grf);
		return -ENXIO;
	}

	id = of_alias_get_id(ofnode_to_np(mipi_node), "dsi");
	if (id < 0)
		id = 0;

	dsi->pdata = pdata;
	dsi->id = id;
	dsi->blob = state->blob;
	dsi->node = mipi_node;
	conn_state->private = dsi;
	conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;
	conn_state->color_space = V4L2_COLORSPACE_DEFAULT;

	panel = dev_read_subnode(conn_state->dev, "panel");
	if (!ofnode_valid(panel)) {
		printf("failed to find panel node\n");
		return -1;
	}

#define FDT_GET_INT(val, name) \
	val = ofnode_read_s32_default(panel, name, -1); \
	if (val < 0) { \
		printf("Can't get %s\n", name); \
		return -1; \
	}

	FDT_GET_INT(dsi->lanes, "dsi,lanes");
	FDT_GET_INT(dsi->format, "dsi,format");
	FDT_GET_INT(dsi->mode_flags, "dsi,flags");
	FDT_GET_INT(dsi->channel, "reg");

	ret = rockchip_dsi_dual_channel_probe(dsi);
	if (ret)
		return ret;

	conn_state->type = DRM_MODE_CONNECTOR_DSI;
	if (dsi->slave)
		conn_state->output_type = ROCKCHIP_OUTPUT_DSI_DUAL_CHANNEL;

	return 0;
}

static void rockchip_dw_mipi_dsi_deinit(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	if (dsi->slave)
		free(dsi->slave);
	free(dsi);
}

static void rockchip_dw_dsi_pre_init(struct display_state *state,
				     struct dw_mipi_dsi *dsi)
{
	struct connector_state *conn_state = &state->conn_state;
	unsigned long bw, rate;

	dsi->mode = &conn_state->mode;

	if (conn_state->phy) {
		bw = rockchip_dsi_calc_bandwidth(dsi);
		rate = rockchip_phy_set_pll(state, bw * USEC_PER_SEC);
		dsi->lane_mbps = rate / USEC_PER_SEC;
		rockchip_phy_power_on(state);
	} else {
		dw_mipi_dsi_get_lane_bps(dsi);
	}

	printf("final DSI-Link bandwidth: %u Mbps x %d\n",
	       dsi->lane_mbps, dsi->lanes);

	if (dsi->slave)
		rockchip_dw_dsi_pre_init(state, dsi->slave);
}

static void rockchip_dw_dsi_host_init(struct dw_mipi_dsi *dsi)
{
	dw_mipi_dsi_init(dsi);
	dw_mipi_dsi_dpi_config(dsi, dsi->mode);
	dw_mipi_dsi_packet_handler_config(dsi);
	dw_mipi_dsi_video_mode_config(dsi);
	dw_mipi_dsi_video_packet_config(dsi, dsi->mode);
	dw_mipi_dsi_command_mode_config(dsi);
	dw_mipi_dsi_set_mode(dsi, DSI_COMMAND_MODE);
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

	/* Enable Data Lane Module */
	grf_field_write(dsi, ENABLE_N, map[dsi->lanes - 1]);

	/* Enable Clock Lane Module */
	grf_field_write(dsi, ENABLECLK, 1);
}

static void rockchip_dw_dsi_controller_init(struct dw_mipi_dsi *dsi)
{
	rockchip_dw_dsi_host_init(dsi);

	mdelay(10);
	mipi_dphy_init(dsi);
	dw_mipi_dsi_phy_init(dsi);

	if (dsi->slave)
		rockchip_dw_dsi_controller_init(dsi->slave);
}

static int rockchip_dw_mipi_dsi_prepare(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct crtc_state *crtc_state = &state->crtc_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	dw_mipi_dsi_vop_routing(dsi, crtc_state->crtc_id);

	rockchip_dw_dsi_pre_init(state, dsi);

	rockchip_dw_dsi_controller_init(dsi);

	return 0;
}

static int rockchip_dw_mipi_dsi_enable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	dw_mipi_dsi_set_mode(dsi, DSI_VIDEO_MODE);
	if (dsi->slave)
		dw_mipi_dsi_set_mode(dsi->slave, DSI_VIDEO_MODE);

	return 0;
}

static int rockchip_dw_mipi_dsi_disable(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct dw_mipi_dsi *dsi = conn_state->private;

	/*
	 * This is necessary to make sure the peripheral will be driven
	 * normally when the display is enabled again later.
	 */
	mdelay(120);

	dw_mipi_dsi_disable(dsi);
	return 0;
}

static const struct rockchip_connector_funcs rockchip_dw_mipi_dsi_funcs = {
	.init = rockchip_dw_mipi_dsi_init,
	.deinit = rockchip_dw_mipi_dsi_deinit,
	.prepare = rockchip_dw_mipi_dsi_prepare,
	.enable = rockchip_dw_mipi_dsi_enable,
	.disable = rockchip_dw_mipi_dsi_disable,
	.transfer = rockchip_dw_mipi_dsi_transfer,
};

static const u32 px30_dsi_grf_reg_fields[MAX_FIELDS] = {
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x0434,  7,  7),
	[DPICOLORM]		= GRF_REG_FIELD(0x0434,  3,  3),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0434,  2,  2),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0438,  7, 10),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0438,  5,  5),
	[VOPSEL]		= GRF_REG_FIELD(0x0438,  0,  0),
};

static const struct dw_mipi_dsi_plat_data px30_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = px30_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = PX30,
};

static const struct rockchip_connector px30_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &px30_mipi_dsi_drv_data,
};

static const u32 rk3128_dsi_grf_reg_fields[MAX_FIELDS] = {
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0150, 10, 13),
	[FORCERXMODE]		= GRF_REG_FIELD(0x0150,  9,  9),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0150,  8,  8),
	[DPICOLORM]		= GRF_REG_FIELD(0x0150,  5,  5),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0150,  4,  4),
};

static const struct dw_mipi_dsi_plat_data rk3128_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3128_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RK3128,
};

static const struct rockchip_connector rk3128_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3128_mipi_dsi_drv_data,
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

static const struct dw_mipi_dsi_plat_data rk3288_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3288_dsi0_grf_reg_fields,
	.dsi1_grf_reg_fields = rk3288_dsi1_grf_reg_fields,
	.max_bit_rate_per_lane = 1500000000UL,
	.soc_type = RK3288,
};

static const struct rockchip_connector rk3288_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3288_mipi_dsi_drv_data,
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

static const struct dw_mipi_dsi_plat_data rk3366_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3366_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RK3366,
};

static const struct rockchip_connector rk3366_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3366_mipi_dsi_drv_data,
};

static const u32 rk3368_dsi_grf_reg_fields[MAX_FIELDS] = {
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x0418,  7,  7),
	[DPICOLORM]		= GRF_REG_FIELD(0x0418,  3,  3),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0418,  2,  2),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x041c,  7, 10),
	[FORCERXMODE]		= GRF_REG_FIELD(0x041c,  6,  6),
	[TURNDISABLE]		= GRF_REG_FIELD(0x041c,  5,  5),
};

static const struct dw_mipi_dsi_plat_data rk3368_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3368_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RK3368,
};

static const struct rockchip_connector rk3368_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3368_mipi_dsi_drv_data,
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
	[ENABLE_N]		= GRF_REG_FIELD(0x625c,  0,  3),
	[MASTERSLAVEZ]		= GRF_REG_FIELD(0x6260,  7,  7),
	[ENABLECLK]		= GRF_REG_FIELD(0x6260,  6,  6),
	[BASEDIR]		= GRF_REG_FIELD(0x6260,  5,  5),
	[TURNREQUEST]		= GRF_REG_FIELD(0x6260,  0,  3),
};

static const struct dw_mipi_dsi_plat_data rk3399_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rk3399_dsi0_grf_reg_fields,
	.dsi1_grf_reg_fields = rk3399_dsi1_grf_reg_fields,
	.max_bit_rate_per_lane = 1500000000UL,
	.soc_type = RK3399,
};

static const struct rockchip_connector rk3399_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3399_mipi_dsi_drv_data,
};

static const u32 rv1108_dsi_grf_reg_fields[MAX_FIELDS] = {
	[DPICOLORM]		= GRF_REG_FIELD(0x0410,  7,  7),
	[DPISHUTDN]		= GRF_REG_FIELD(0x0410,  6,  6),
	[DPIUPDATECFG]		= GRF_REG_FIELD(0x0410,  8,  8),
	[FORCERXMODE]		= GRF_REG_FIELD(0x0414,  5,  5),
	[FORCETXSTOPMODE]	= GRF_REG_FIELD(0x0414,  6,  9),
	[TURNDISABLE]		= GRF_REG_FIELD(0x0414,  4,  4),
};

static const struct dw_mipi_dsi_plat_data rv1108_mipi_dsi_drv_data = {
	.dsi0_grf_reg_fields = rv1108_dsi_grf_reg_fields,
	.max_bit_rate_per_lane = 1000000000UL,
	.soc_type = RV1108,
};

static const struct rockchip_connector rv1108_mipi_dsi_data = {
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rv1108_mipi_dsi_drv_data,
};

static const struct udevice_id rockchip_mipi_dsi_ids[] = {
	{
		.compatible = "rockchip,px30-mipi-dsi",
		.data = (ulong)&px30_mipi_dsi_data,
	},
	{
		.compatible = "rockchip,rk3128-mipi-dsi",
		.data = (ulong)&rk3128_mipi_dsi_data,
	},
	{
		.compatible = "rockchip,rk3288-mipi-dsi",
		.data = (ulong)&rk3288_mipi_dsi_data,
	},
	{
		.compatible = "rockchip,rk3366-mipi-dsi",
		.data = (ulong)&rk3366_mipi_dsi_data,
	},
	{
		.compatible = "rockchip,rk3368-mipi-dsi",
		.data = (ulong)&rk3368_mipi_dsi_data,
	},
	{
		.compatible = "rockchip,rk3399-mipi-dsi",
		.data = (ulong)&rk3399_mipi_dsi_data,
	},
	{
		.compatible = "rockchip,rv1108-mipi-dsi",
		.data = (ulong)&rv1108_mipi_dsi_data,
	},
	{}
};

U_BOOT_DRIVER(rockchip_mipi_dsi) = {
	.name = "rockchip_mipi_dsi",
	.id = UCLASS_DISPLAY,
	.of_match = rockchip_mipi_dsi_ids,
};
