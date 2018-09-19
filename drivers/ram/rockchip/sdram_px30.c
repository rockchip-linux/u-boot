// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2018 Rockchip Electronics Co., Ltd.
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <ram.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/cru_px30.h>
#include <asm/arch/grf_px30.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sdram_common.h>
#include <asm/arch/sdram_px30.h>

/*
 * Because px30 sram size is small, so need define CONFIG_TPL_TINY_FRAMEWORK
 * to reduce TPL size when build TPL firmware.
 */
#ifdef CONFIG_TPL_BUILD
#ifndef CONFIG_TPL_TINY_FRAMEWORK
#error please defined CONFIG_TPL_TINY_FRAMEWORK for px30 !!!
#endif
#endif

DECLARE_GLOBAL_DATA_PTR;
struct dram_info {
#ifdef CONFIG_TPL_BUILD
	struct px30_ddr_pctl_regs *pctl;
	struct px30_ddr_phy_regs *phy;
	struct px30_cru *cru;
	struct px30_msch_regs *msch;
	struct px30_ddr_grf_regs *ddr_grf;
	struct px30_grf *grf;
#endif
	struct ram_info info;
	struct px30_pmugrf *pmugrf;
};

#ifdef CONFIG_TPL_BUILD
#define PMUGRF_BASE_ADDR		0xFF010000
#define CRU_BASE_ADDR			0xFF2B0000
#define GRF_BASE_ADDR			0xFF140000
#define DDRC_BASE_ADDR			0xFF600000
#define DDR_PHY_BASE_ADDR		0xFF2A0000
#define SERVER_MSCH0_BASE_ADDR		0xFF530000
#define DDR_GRF_BASE_ADDR		0xff630000

struct dram_info dram_info;

struct px30_sdram_params sdram_configs[] = {
#include	"sdram-px30-lpddr3-detect-333.inc"
};

struct px30_ddr_skew skew = {
#include	"sdram-px30-ddr_skew.inc"
};

static void rkclk_ddr_reset(struct dram_info *dram,
			    u32 ctl_srstn, u32 ctl_psrstn,
			    u32 phy_srstn, u32 phy_psrstn)
{
	writel(upctl2_srstn_req(ctl_srstn) | upctl2_psrstn_req(ctl_psrstn) |
	       upctl2_asrstn_req(ctl_srstn),
	       &dram->cru->softrst_con[1]);
	writel(ddrphy_srstn_req(phy_srstn) | ddrphy_psrstn_req(phy_psrstn),
	       &dram->cru->softrst_con[2]);
}

static void rkclk_set_dpll(struct dram_info *dram, unsigned int mhz)
{
	unsigned int refdiv, postdiv1, postdiv2, fbdiv;
	int delay = 1000;

	refdiv = 1;
	if (mhz <= 300) {
		postdiv1 = 4;
		postdiv2 = 2;
	} else if (mhz <= 400) {
		postdiv1 = 6;
		postdiv2 = 1;
	} else if (mhz <= 600) {
		postdiv1 = 4;
		postdiv2 = 1;
	} else if (mhz <= 800) {
		postdiv1 = 3;
		postdiv2 = 1;
	} else if (mhz <= 1600) {
		postdiv1 = 2;
		postdiv2 = 1;
	} else {
		postdiv1 = 1;
		postdiv2 = 1;
	}
	fbdiv = (mhz * refdiv * postdiv1 * postdiv2) / 24;

	writel(DPLL_MODE(CLOCK_FROM_XIN_OSC), &dram->cru->mode);

	writel(POSTDIV1(postdiv1) | FBDIV(fbdiv), &dram->cru->pll[1].con0);
	writel(DSMPD(1) | POSTDIV2(postdiv2) | REFDIV(refdiv),
	       &dram->cru->pll[1].con1);

	while (delay > 0) {
		udelay(1);
		if (LOCK(readl(&dram->cru->pll[1].con1)))
			break;
		delay--;
	}

	writel(DPLL_MODE(CLOCK_FROM_PLL), &dram->cru->mode);
}

static void rkclk_configure_ddr(struct dram_info *dram,
				struct px30_sdram_params *sdram_params)
{
	/* for inno ddr phy need 2*freq */
	rkclk_set_dpll(dram,  sdram_params->ddr_freq * 2);
}

static void phy_soft_reset(struct dram_info *dram)
{
	void __iomem *phy_base = dram->phy;

	clrbits_le32(PHY_REG(phy_base, 0), 0x3 << 2);
	udelay(1);
	setbits_le32(PHY_REG(phy_base, 0), ANALOG_DERESET);
	udelay(5);
	setbits_le32(PHY_REG(phy_base, 0), DIGITAL_DERESET);
	udelay(1);
}

static int pctl_cfg(struct dram_info *dram,
		    struct px30_sdram_params *sdram_params)
{
	u32 i;
	void __iomem *pctl_base = dram->pctl;

	for (i = 0; sdram_params->pctl_regs.pctl[i][0] != 0xFFFFFFFF; i++) {
		writel(sdram_params->pctl_regs.pctl[i][1],
		       pctl_base + sdram_params->pctl_regs.pctl[i][0]);
	}
	clrsetbits_le32(pctl_base + DDR_PCTL2_PWRTMG,
			(0xff << 16) | 0x1f,
			((SR_IDLE & 0xff) << 16) | (PD_IDLE & 0x1f));

	clrsetbits_le32(pctl_base + DDR_PCTL2_HWLPCTL,
			0xfff << 16,
			5 << 16);
	/* disable zqcs */
	setbits_le32(pctl_base + DDR_PCTL2_ZQCTL0, 1u << 31);

	return 0;
}

/* return ddrconfig value
 *       (-1), find ddrconfig fail
 *       other, the ddrconfig value
 * only support cs0_row >= cs1_row
 */
static unsigned int calculate_ddrconfig(struct px30_sdram_params *sdram_params)
{
	u32 bw, die_bw, col, bank;
	u32 i, tmp;
	u32 ddrconf = -1;

	bw = sdram_params->ch.bw;
	die_bw = sdram_params->ch.dbw;
	col = sdram_params->ch.col;
	bank = sdram_params->ch.bk;

	if (sdram_params->dramtype == DDR4) {
		if (die_bw == 0)
			ddrconf = 7 + bw;
		else
			ddrconf = 12 - bw;
		ddrconf = d4_rbc_2_d3_rbc[ddrconf - 7];
	} else {
		tmp = ((bank - 2) << 3) | (col + bw - 10);
		for (i = 0; i < 7; i++)
			if ((ddr_cfg_2_rbc[i] & 0xf) == tmp) {
				ddrconf = i;
				break;
			}
		if (i > 6)
			printascii("calculate ddrconfig error\n");
	}

	return ddrconf;
}

/* n: Unit bytes */
static void copy_to_reg(u32 *dest, u32 *src, u32 n)
{
	int i;

	for (i = 0; i < n / sizeof(u32); i++) {
		writel(*src, dest);
		src++;
		dest++;
	}
}

/*
 * calculate controller dram address map, and setting to register.
 * argument sdram_params->ch.ddrconf must be right value before
 * call this function.
 */
static void set_ctl_address_map(struct dram_info *dram,
				struct px30_sdram_params *sdram_params)
{
	void __iomem *pctl_base = dram->pctl;
	u32 cs_pst, bg, max_row, ddrconf;
	u32 i;

	if (sdram_params->dramtype == DDR4)
		/*
		 * DDR4 8bit dram BG = 2(4bank groups),
		 * 16bit dram BG = 1 (2 bank groups)
		 */
		bg = (sdram_params->ch.dbw == 0) ? 2 : 1;
	else
		bg = 0;

	cs_pst = sdram_params->ch.bw + sdram_params->ch.col +
		bg + sdram_params->ch.bk + sdram_params->ch.cs0_row;
	if (cs_pst >= 32 || sdram_params->ch.rank == 1)
		writel(0x1f, pctl_base + DDR_PCTL2_ADDRMAP0);
	else
		writel(cs_pst - 8, pctl_base + DDR_PCTL2_ADDRMAP0);

	ddrconf = sdram_params->ch.ddrconfig;
	if (sdram_params->dramtype == DDR4) {
		for (i = 0; i < ARRAY_SIZE(d4_rbc_2_d3_rbc); i++) {
			if (d4_rbc_2_d3_rbc[i] == ddrconf) {
				ddrconf = 7 + i;
				break;
			}
		}
	}

	copy_to_reg((u32 *)(pctl_base + DDR_PCTL2_ADDRMAP1),
		    &addrmap[ddrconf][0], 8 * 4);
	max_row = cs_pst - 1 - 8 - (addrmap[ddrconf][5] & 0xf);

	if (max_row < 12)
		printascii("set addrmap fail\n");
	/* need to disable row ahead of rank by set to 0xf */
	for (i = 17; i > max_row; i--)
		clrsetbits_le32(pctl_base + DDR_PCTL2_ADDRMAP6 +
			((i - 12) * 8 / 32) * 4,
			0xf << ((i - 12) * 8 % 32),
			0xf << ((i - 12) * 8 % 32));

	if ((sdram_params->dramtype == LPDDR3 ||
	     sdram_params->dramtype == LPDDR2) &&
		 sdram_params->ch.row_3_4)
		setbits_le32(pctl_base + DDR_PCTL2_ADDRMAP6, 1 << 31);
	if (sdram_params->dramtype == DDR4 && sdram_params->ch.bw != 0x2)
		setbits_le32(pctl_base + DDR_PCTL2_PCCFG, 1 << 8);
}

static void phy_dll_bypass_set(struct dram_info *dram, u32 freq)
{
	void __iomem *phy_base = dram->phy;
	u32 tmp;
	u32 i, j;

	setbits_le32(PHY_REG(phy_base, 0x13), 1 << 4);
	clrbits_le32(PHY_REG(phy_base, 0x14), 1 << 3);
	for (i = 0; i < 4; i++) {
		j = 0x26 + i * 0x10;
		setbits_le32(PHY_REG(phy_base, j), 1 << 4);
		clrbits_le32(PHY_REG(phy_base, j + 0x1), 1 << 3);
	}

	if (freq <= (400 * MHz))
		/* DLL bypass */
		setbits_le32(PHY_REG(phy_base, 0xa4), 0x1f);
	else
		clrbits_le32(PHY_REG(phy_base, 0xa4), 0x1f);

	if (freq <= (801 * MHz))
		tmp = 2;
	else
		tmp = 1;

	for (i = 0; i < 4; i++) {
		j = 0x28 + i * 0x10;
		writel(tmp, PHY_REG(phy_base, j));
	}
}

static void set_ds_odt(struct dram_info *dram,
		       struct px30_sdram_params *sdram_params)
{
	void __iomem *phy_base = dram->phy;
	u32 cmd_drv, clk_drv, dqs_drv, dqs_odt;
	u32 i, j;

	if (sdram_params->dramtype == DDR3) {
		cmd_drv = PHY_DDR3_RON_RTT_34ohm;
		clk_drv = PHY_DDR3_RON_RTT_45ohm;
		dqs_drv = PHY_DDR3_RON_RTT_34ohm;
		dqs_odt = PHY_DDR3_RON_RTT_225ohm;
	} else {
		cmd_drv = PHY_DDR4_LPDDR3_RON_RTT_34ohm;
		clk_drv = PHY_DDR4_LPDDR3_RON_RTT_43ohm;
		dqs_drv = PHY_DDR4_LPDDR3_RON_RTT_34ohm;
		if (sdram_params->dramtype == LPDDR2)
			dqs_odt = PHY_DDR4_LPDDR3_RON_RTT_DISABLE;
		else
			dqs_odt = PHY_DDR4_LPDDR3_RON_RTT_240ohm;
	}
	/* DS */
	writel(cmd_drv, PHY_REG(phy_base, 0x11));
	clrsetbits_le32(PHY_REG(phy_base, 0x12), 0x1f << 3, cmd_drv << 3);
	writel(clk_drv, PHY_REG(phy_base, 0x16));
	writel(clk_drv, PHY_REG(phy_base, 0x18));

	for (i = 0; i < 4; i++) {
		j = 0x20 + i * 0x10;
		writel(dqs_drv, PHY_REG(phy_base, j));
		writel(dqs_drv, PHY_REG(phy_base, j + 0xf));
		/* ODT */
		writel(dqs_odt, PHY_REG(phy_base, j + 0x1));
		writel(dqs_odt, PHY_REG(phy_base, j + 0xe));
	}
}

static void phy_cfg(struct dram_info *dram,
		    struct px30_sdram_params *sdram_params)
{
	void __iomem *phy_base = dram->phy;
	u32 i;

	phy_dll_bypass_set(dram, sdram_params->ddr_freq);
	for (i = 0; sdram_params->phy_regs.phy[i][0] != 0xFFFFFFFF; i++) {
		writel(sdram_params->phy_regs.phy[i][1],
		       phy_base + sdram_params->phy_regs.phy[i][0]);
	}
	if (sdram_params->ch.bw == 2) {
		clrsetbits_le32(PHY_REG(phy_base, 0), 0xf << 4, 0xf << 4);
	} else if (sdram_params->ch.bw == 1) {
		clrsetbits_le32(PHY_REG(phy_base, 0), 0xf << 4, 3 << 4);
		/* disable DQS2,DQS3 tx dll  for saving power */
		clrbits_le32(PHY_REG(phy_base, 0x46), 1 << 3);
		clrbits_le32(PHY_REG(phy_base, 0x56), 1 << 3);
	} else {
		clrsetbits_le32(PHY_REG(phy_base, 0), 0xf << 4, 1 << 4);
		/* disable DQS2,DQS3 tx dll  for saving power */
		clrbits_le32(PHY_REG(phy_base, 0x36), 1 << 3);
		clrbits_le32(PHY_REG(phy_base, 0x46), 1 << 3);
		clrbits_le32(PHY_REG(phy_base, 0x56), 1 << 3);
	}
	set_ds_odt(dram, sdram_params);

	/* deskew */
	setbits_le32(PHY_REG(phy_base, 2), 8);
	copy_to_reg(PHY_REG(phy_base, 0xb0),
		    &sdram_params->skew->a0_a1_skew[0], 15 * 4);
	copy_to_reg(PHY_REG(phy_base, 0x70),
		    &sdram_params->skew->cs0_dm0_skew[0], 44 * 4);
	copy_to_reg(PHY_REG(phy_base, 0xc0),
		    &sdram_params->skew->cs1_dm0_skew[0], 44 * 4);
}

static int update_refresh_reg(struct dram_info *dram)
{
	void __iomem *pctl_base = dram->pctl;
	u32 ret;

	ret = readl(pctl_base + DDR_PCTL2_RFSHCTL3) ^ (1 << 1);
	writel(ret, pctl_base + DDR_PCTL2_RFSHCTL3);

	return 0;
}

/*
 * rank = 1: cs0
 * rank = 2: cs1
 */
int read_mr(struct dram_info *dram, u32 rank, u32 mr_num)
{
	void __iomem *pctl_base = dram->pctl;
	void __iomem *ddr_grf_base = dram->ddr_grf;

	writel((rank << 4) | (1 << 0), pctl_base + DDR_PCTL2_MRCTRL0);
	writel((mr_num << 8), pctl_base + DDR_PCTL2_MRCTRL1);
	setbits_le32(pctl_base + DDR_PCTL2_MRCTRL0, 1u << 31);
	while (readl(pctl_base + DDR_PCTL2_MRCTRL0) & (1u << 31))
		continue;
	while (readl(pctl_base + DDR_PCTL2_MRSTAT) & MR_WR_BUSY)
		continue;

	return (readl(ddr_grf_base + DDR_GRF_STATUS(0)) & 0xff);
}

u32 disable_zqcs_arefresh(struct dram_info *dram)
{
	void __iomem *pctl_base = dram->pctl;
	u32 dis_auto_zq = 0;

	/* disable zqcs */
	if (!(readl(pctl_base + DDR_PCTL2_ZQCTL0) &
		(1ul << 31))) {
		dis_auto_zq = 1;
		setbits_le32(pctl_base + DDR_PCTL2_ZQCTL0, 1 << 31);
	}

	/* disable auto refresh */
	setbits_le32(pctl_base + DDR_PCTL2_RFSHCTL3, 1);

	update_refresh_reg(dram);

	return dis_auto_zq;
}

void restore_zqcs_arefresh(struct dram_info *dram, u32 dis_auto_zq)
{
	void __iomem *pctl_base = dram->pctl;

	/* restore zqcs */
	if (dis_auto_zq)
		clrbits_le32(pctl_base + DDR_PCTL2_ZQCTL0, 1 << 31);

	/* restore auto refresh */
	clrbits_le32(pctl_base + DDR_PCTL2_RFSHCTL3, 1);

	update_refresh_reg(dram);
}

#define MIN(a, b)	(((a) > (b)) ? (b) : (a))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
static u32 check_rd_gate(struct dram_info *dram)
{
	void __iomem *phy_base = dram->phy;

	u32 max_val = 0;
	u32 min_val = 0xff;
	u32 gate[4];
	u32 i, bw;

	bw = (readl(PHY_REG(phy_base, 0x0)) >> 4) & 0xf;
	switch (bw) {
	case 0x1:
		bw = 1;
		break;
	case 0x3:
		bw = 2;
		break;
	case 0xf:
	default:
		bw = 4;
		break;
	}

	for (i = 0; i < bw; i++) {
		gate[i] = readl(PHY_REG(phy_base, 0xfb + i));
		max_val = MAX(max_val, gate[i]);
		min_val = MIN(min_val, gate[i]);
	}

	if (max_val > 0x80 || min_val < 0x20)
		return -1;
	else
		return 0;
}

static int data_training(struct dram_info *dram, u32 cs, u32 dramtype)
{
	void __iomem *phy_base = dram->phy;
	u32 ret;
	u32 dis_auto_zq = 0;
	u32 odt_val;
	u32 i, j;

	odt_val = readl(PHY_REG(phy_base, 0x2e));

	for (i = 0; i < 4; i++) {
		j = 0x20 + i * 0x10;
		writel(PHY_DDR3_RON_RTT_225ohm, PHY_REG(phy_base, j + 0x1));
		writel(0, PHY_REG(phy_base, j + 0xe));
	}

	dis_auto_zq = disable_zqcs_arefresh(dram);

	if (dramtype == DDR4) {
		clrsetbits_le32(PHY_REG(phy_base, 0x29), 0x3, 0);
		clrsetbits_le32(PHY_REG(phy_base, 0x39), 0x3, 0);
		clrsetbits_le32(PHY_REG(phy_base, 0x49), 0x3, 0);
		clrsetbits_le32(PHY_REG(phy_base, 0x59), 0x3, 0);
	}
	/* choose training cs */
	clrsetbits_le32(PHY_REG(phy_base, 2), 0x33, (0x20 >> cs));
	/* enable gate training */
	clrsetbits_le32(PHY_REG(phy_base, 2), 0x33, (0x20 >> cs) | 1);
	udelay(50);
	ret = readl(PHY_REG(phy_base, 0xff));
	/* disable gate training */
	clrsetbits_le32(PHY_REG(phy_base, 2), 0x33, (0x20 >> cs) | 0);
	clrbits_le32(PHY_REG(phy_base, 2), 0x30);
	restore_zqcs_arefresh(dram, dis_auto_zq);

	if (dramtype == DDR4) {
		clrsetbits_le32(PHY_REG(phy_base, 0x29), 0x3, 0x2);
		clrsetbits_le32(PHY_REG(phy_base, 0x39), 0x3, 0x2);
		clrsetbits_le32(PHY_REG(phy_base, 0x49), 0x3, 0x2);
		clrsetbits_le32(PHY_REG(phy_base, 0x59), 0x3, 0x2);
	}

	if (ret & 0x10) {
		ret = -1;
	} else {
		ret = (ret & 0xf) ^ (readl(PHY_REG(phy_base, 0)) >> 4);
		ret = (ret == 0) ? 0 : -1;
	}

	for (i = 0; i < 4; i++) {
		j = 0x20 + i * 0x10;
		writel(odt_val, PHY_REG(phy_base, j + 0x1));
		writel(odt_val, PHY_REG(phy_base, j + 0xe));
	}

	return ret;
}

/* rank = 1: cs0
 * rank = 2: cs1
 * rank = 3: cs0 & cs1
 * note: be careful of keep mr original val
 */
static int write_mr(struct dram_info *dram, u32 rank, u32 mr_num, u32 arg,
		    u32 dramtype)
{
	void __iomem *pctl_base = dram->pctl;

	while (readl(pctl_base + DDR_PCTL2_MRSTAT) & MR_WR_BUSY)
		continue;
	if (dramtype == DDR3 || dramtype == DDR4) {
		writel((mr_num << 12) | (rank << 4) | (0 << 0),
		       pctl_base + DDR_PCTL2_MRCTRL0);
		writel(arg, pctl_base + DDR_PCTL2_MRCTRL1);
	} else {
		writel((rank << 4) | (0 << 0),
		       pctl_base + DDR_PCTL2_MRCTRL0);
		writel((mr_num << 8) | (arg & 0xff),
		       pctl_base + DDR_PCTL2_MRCTRL1);
	}

	setbits_le32(pctl_base + DDR_PCTL2_MRCTRL0, 1u << 31);
	while (readl(pctl_base + DDR_PCTL2_MRCTRL0) & (1u << 31))
		continue;
	while (readl(pctl_base + DDR_PCTL2_MRSTAT) & MR_WR_BUSY)
		continue;

	return 0;
}

/*
 * rank : 1:cs0, 2:cs1, 3:cs0&cs1
 * vrefrate: 4500: 45%,
 */
static int write_vrefdq(struct dram_info *dram, u32 rank, u32 vrefrate,
			u32 dramtype)
{
	void __iomem *pctl_base = dram->pctl;
	u32 tccd_l, value;
	u32 dis_auto_zq = 0;

	if (dramtype != DDR4 || vrefrate < 4500 ||
	    vrefrate > 9200)
		return (-1);

	tccd_l = (readl(pctl_base + DDR_PCTL2_DRAMTMG4) >> 16) & 0xf;
	tccd_l = (tccd_l - 4) << 10;

	if (vrefrate > 7500) {
		/* range 1 */
		value = ((vrefrate - 6000) / 65) | tccd_l;
	} else {
		/* range 2 */
		value = ((vrefrate - 4500) / 65) | tccd_l | (1 << 6);
	}

	dis_auto_zq = disable_zqcs_arefresh(dram);

	/* enable vrefdq calibratin */
	write_mr(dram, rank, 6, value | (1 << 7), dramtype);
	udelay(1);/* tvrefdqe */
	/* write vrefdq value */
	write_mr(dram, rank, 6, value | (1 << 7), dramtype);
	udelay(1);/* tvref_time */
	write_mr(dram, rank, 6, value | (0 << 7), dramtype);
	udelay(1);/* tvrefdqx */

	restore_zqcs_arefresh(dram, dis_auto_zq);

	return 0;
}

/*
 * cs: 0:cs0
 *	   1:cs1
 *     else cs0+cs1
 * note: it didn't consider about row_3_4
 */
u64 get_cs_cap(struct px30_sdram_params *sdram_params, u32 cs)
{
	u32 bg;
	u64 cap[2];

	if (sdram_params->dramtype == DDR4)
		/* DDR4 8bit dram BG = 2(4bank groups),
		 * 16bit dram BG = 1 (2 bank groups)
		 */
		bg = (sdram_params->ch.dbw == 0) ? 2 : 1;
	else
		bg = 0;
	cap[0] = 1llu << (sdram_params->ch.bw + sdram_params->ch.col +
		bg + sdram_params->ch.bk + sdram_params->ch.cs0_row);

	if (sdram_params->ch.rank == 2)
		cap[1] = 1llu << (sdram_params->ch.bw + sdram_params->ch.col +
			bg + sdram_params->ch.bk + sdram_params->ch.cs1_row);
	else
		cap[1] = 0;

	if (cs == 0)
		return cap[0];
	else if (cs == 1)
		return cap[1];
	else
		return (cap[0] + cap[1]);
}

static void set_ddrconfig(struct dram_info *dram, u32 ddrconfig)
{
	writel(ddrconfig | (ddrconfig << 8), &dram->msch->deviceconf);
	rk_clrsetreg(&dram->grf->soc_noc_con[1], 0x3 << 14, 0 << 14);
}

static void dram_all_config(struct dram_info *dram,
			    struct px30_sdram_params *sdram_params)
{
	u32 sys_reg = 0;
	u32 sys_reg3 = 0;
	u64 cs_cap[2];

	set_ddrconfig(dram, sdram_params->ch.ddrconfig);

	sys_reg |= SYS_REG_ENC_DDRTYPE(sdram_params->dramtype);
	sys_reg |= SYS_REG_ENC_ROW_3_4(sdram_params->ch.row_3_4);
	sys_reg |= SYS_REG_ENC_RANK(sdram_params->ch.rank);
	sys_reg |= SYS_REG_ENC_COL(sdram_params->ch.col);
	sys_reg |= SYS_REG_ENC_BK(sdram_params->ch.bk);
	sys_reg |= SYS_REG_ENC_BW(sdram_params->ch.bw);
	sys_reg |= SYS_REG_ENC_DBW(sdram_params->ch.dbw);

	SYS_REG_ENC_CS0_ROW_(sdram_params->ch.cs0_row, sys_reg, sys_reg3);
	if (sdram_params->ch.cs1_row)
		SYS_REG_ENC_CS1_ROW_(sdram_params->ch.cs1_row, sys_reg,
				     sys_reg3);
	sys_reg3 |= SYS_REG_ENC_CS1_COL(sdram_params->ch.col);
	sys_reg3 |= SYS_REG_ENC_VERSION(DDR_SYS_REG_VERSION);

	writel(sys_reg, &dram->pmugrf->os_reg[2]);
	writel(sys_reg3, &dram->pmugrf->os_reg[3]);

	cs_cap[0] = get_cs_cap(sdram_params, 0);
	cs_cap[1] = get_cs_cap(sdram_params, 1);
	writel(((((cs_cap[1] >> 20) / 64) & 0xff) << 8) |
			(((cs_cap[0] >> 20) / 64) & 0xff),
			&dram->msch->devicesize);

	writel(sdram_params->ch.noc_timings.ddrtiminga0.d32,
	       &dram->msch->ddrtiminga0);
	writel(sdram_params->ch.noc_timings.ddrtimingb0.d32,
	       &dram->msch->ddrtimingb0);
	writel(sdram_params->ch.noc_timings.ddrtimingc0.d32,
	       &dram->msch->ddrtimingc0);
	writel(sdram_params->ch.noc_timings.devtodev0.d32,
	       &dram->msch->devtodev0);
	writel(sdram_params->ch.noc_timings.ddrmode.d32, &dram->msch->ddrmode);
	writel(sdram_params->ch.noc_timings.ddr4timing.d32,
	       &dram->msch->ddr4timing);
	writel(sdram_params->ch.noc_timings.agingx0, &dram->msch->agingx0);
	writel(sdram_params->ch.noc_timings.agingx0, &dram->msch->aging0);
	writel(sdram_params->ch.noc_timings.agingx0, &dram->msch->aging1);
	writel(sdram_params->ch.noc_timings.agingx0, &dram->msch->aging2);
	writel(sdram_params->ch.noc_timings.agingx0, &dram->msch->aging3);
}

static void enable_low_power(struct dram_info *dram,
			     struct px30_sdram_params *sdram_params)
{
	void __iomem *pctl_base = dram->pctl;
	void __iomem *phy_base = dram->phy;
	void __iomem *ddr_grf_base = dram->ddr_grf;
	u32 grf_lp_con;

	/*
	 * bit0: grf_upctl_axi_cg_en = 1 enable upctl2 axi clk auto gating
	 * bit1: grf_upctl_apb_cg_en = 1 ungated axi,core clk for apb access
	 * bit2: grf_upctl_core_cg_en = 1 enable upctl2 core clk auto gating
	 * bit3: grf_selfref_type2_en = 0 disable core clk gating when type2 sr
	 * bit4: grf_upctl_syscreq_cg_en = 1
	 *       ungating coreclk when c_sysreq assert
	 * bit8-11: grf_auto_sr_dly = 6
	 */
	writel(0x1f1f0617, &dram->ddr_grf->ddr_grf_con[1]);

	if (sdram_params->dramtype == DDR4)
		grf_lp_con = (0x7 << 16) | (1 << 1);
	else if (sdram_params->dramtype == DDR3)
		grf_lp_con = (0x7 << 16) | (1 << 0);
	else
		grf_lp_con = (0x7 << 16) | (1 << 2);

	/* en lpckdis_en */
	grf_lp_con = grf_lp_con | (0x1 << (9 + 16)) | (0x1 << 9);
	writel(grf_lp_con, ddr_grf_base + DDR_GRF_LP_CON);

	/* off digit module clock when enter power down */
	setbits_le32(PHY_REG(phy_base, 7), 1 << 7);

	/* enable sr, pd */
	if (PD_IDLE == 0)
		clrbits_le32(pctl_base + DDR_PCTL2_PWRCTL, (1 << 1));
	else
		setbits_le32(pctl_base + DDR_PCTL2_PWRCTL, (1 << 1));
	if (SR_IDLE == 0)
		clrbits_le32(pctl_base + DDR_PCTL2_PWRCTL, 1);
	else
		setbits_le32(pctl_base + DDR_PCTL2_PWRCTL, 1);
	setbits_le32(pctl_base + DDR_PCTL2_PWRCTL, (1 << 3));
}

static void print_ddr_info(struct px30_sdram_params *sdram_params)
{
	u64 cap;
	u32 bg;
	u32 split;

	split = readl(DDR_GRF_BASE_ADDR + DDR_GRF_SPLIT_CON);
	bg = (sdram_params->ch.dbw == 0) ? 2 : 1;
	switch (sdram_params->dramtype) {
	case LPDDR3:
		printascii("LPDDR3\n");
		break;
	case DDR3:
		printascii("DDR3\n");
		break;
	case DDR4:
		printascii("DDR4\n");
		break;
	case LPDDR2:
		printascii("LPDDR2\n");
		break;
	default:
		printascii("Unknown Device\n");
		break;
	}

	printdec(sdram_params->ddr_freq);
	printascii("MHz\n");
	printascii("BW=");
	printdec(8 << sdram_params->ch.bw);
	printascii(" Col=");
	printdec(sdram_params->ch.col);
	printascii(" Bk=");
	printdec(0x1 << sdram_params->ch.bk);
	if (sdram_params->dramtype == DDR4) {
		printascii(" BG=");
		printdec(1 << bg);
	}
	printascii(" CS0 Row=");
	printdec(sdram_params->ch.cs0_row);
	if (sdram_params->ch.cs0_high16bit_row !=
		sdram_params->ch.cs0_row) {
		printascii("/");
		printdec(sdram_params->ch.cs0_high16bit_row);
	}
	if (sdram_params->ch.rank > 1) {
		printascii(" CS1 Row=");
		printdec(sdram_params->ch.cs1_row);
		if (sdram_params->ch.cs1_high16bit_row !=
			sdram_params->ch.cs1_row) {
			printascii("/");
			printdec(sdram_params->ch.cs1_high16bit_row);
		}
	}
	printascii(" CS=");
	printdec(sdram_params->ch.rank);
	printascii(" Die BW=");
	printdec(8 << sdram_params->ch.dbw);

	cap = get_cs_cap(sdram_params, 3);
	if (sdram_params->ch.row_3_4)
		cap = cap * 3 / 4;
	else if (!(split & (1 << SPLIT_BYPASS_OFFSET)))
		cap = cap / 2 + ((split & 0xff) << 24) / 2;

	printascii(" Size=");
	printdec(cap >> 20);
	printascii("MB\n");
}

/*
 * pre_init: 0: pre init for dram cap detect
 * 1: detect correct cap(except cs1 row)info, than reinit
 * 2: after reinit, we detect cs1_row, if cs1_row not equal
 *    to cs0_row and cs is in middle on ddrconf map, we need
 *    to reinit dram, than set the correct ddrconf.
 */
static int sdram_init_(struct dram_info *dram,
		       struct px30_sdram_params *sdram_params, u32 pre_init)
{
	void __iomem *pctl_base = dram->pctl;

	rkclk_ddr_reset(dram, 1, 1, 1, 1);
	udelay(10);
	/*
	 * dereset ddr phy psrstn to config pll,
	 * if using phy pll psrstn must be dereset
	 * before config pll
	 */
	rkclk_ddr_reset(dram, 1, 1, 1, 0);
	rkclk_configure_ddr(dram, sdram_params);

	/* release phy srst to provide clk to ctrl */
	rkclk_ddr_reset(dram, 1, 1, 0, 0);
	udelay(10);
	phy_soft_reset(dram);
	/* release ctrl presetn, and config ctl registers */
	rkclk_ddr_reset(dram, 1, 0, 0, 0);
	pctl_cfg(dram, sdram_params);
	sdram_params->ch.ddrconfig = calculate_ddrconfig(sdram_params);
	set_ctl_address_map(dram, sdram_params);
	phy_cfg(dram, sdram_params);

	/* enable dfi_init_start to init phy after ctl srstn deassert */
	setbits_le32(pctl_base + DDR_PCTL2_DFIMISC, (1 << 5) | (1 << 4));

	rkclk_ddr_reset(dram, 0, 0, 0, 0);
	/* wait for dfi_init_done and dram init complete */
	while ((readl(pctl_base + DDR_PCTL2_STAT) & 0x7) == 0)
		continue;

	if (sdram_params->dramtype == LPDDR3)
		write_mr(dram, 3, 11, 3, LPDDR3);

	/* do ddr gate training */
redo_cs0_training:
	if (data_training(dram, 0, sdram_params->dramtype) != 0) {
		if (pre_init != 0)
			printascii("DTT cs0 error\n");
		return -1;
	}
	if (check_rd_gate(dram)) {
		printascii("re training cs0");
		goto redo_cs0_training;
	}

	if (sdram_params->dramtype == LPDDR3) {
		if ((read_mr(dram, 1, 8) & 0x3) != 0x3)
			return -1;
	} else if (sdram_params->dramtype == LPDDR2) {
		if ((read_mr(dram, 1, 8) & 0x3) != 0x0)
			return -1;
	}
	/* for px30: when 2cs, both 2 cs should be training */
	if (pre_init != 0 && sdram_params->ch.rank == 2) {
redo_cs1_training:
		if (data_training(dram, 1, sdram_params->dramtype) != 0) {
			printascii("DTT cs1 error\n");
			return -1;
		}
		if (check_rd_gate(dram)) {
			printascii("re training cs1");
			goto redo_cs1_training;
		}
	}

	if (sdram_params->dramtype == DDR4)
		write_vrefdq(dram, 0x3, 5670, sdram_params->dramtype);

	dram_all_config(dram, sdram_params);
	enable_low_power(dram, sdram_params);

	return 0;
}

static u64 dram_detect_cap(struct dram_info *dram,
			   struct px30_sdram_params *sdram_params,
			   unsigned char channel)
{
	void __iomem *pctl_base = dram->pctl;
	void __iomem *phy_base = dram->phy;

	/*
	 * for ddr3: ddrconf = 3
	 * for ddr4: ddrconf = 12
	 * for lpddr3: ddrconf = 3
	 * default bw = 1
	 */
	u32 bk, bktmp;
	u32 col, coltmp;
	u32 row, rowtmp, row_3_4;
	void __iomem *test_addr, *test_addr1;
	u32 dbw;
	u32 cs;
	u32 bw = 1;
	u64 cap = 0;
	u32 dram_type = sdram_params->dramtype;
	u32 pwrctl;

	if (dram_type != DDR4) {
		/* detect col and bk for ddr3/lpddr3 */
		coltmp = 12;
		bktmp = 3;
		rowtmp = 16;

		for (col = coltmp; col >= 9; col -= 1) {
			writel(0, CONFIG_SYS_SDRAM_BASE);
			test_addr = (void __iomem *)(CONFIG_SYS_SDRAM_BASE +
					(1ul << (col + bw - 1ul)));
			writel(PATTERN, test_addr);
			if ((readl(test_addr) == PATTERN) &&
			    (readl(CONFIG_SYS_SDRAM_BASE) == 0))
				break;
		}
		if (col == 8) {
			printascii("col error\n");
			goto cap_err;
		}

		test_addr = (void __iomem *)(CONFIG_SYS_SDRAM_BASE +
				(1ul << (coltmp + bktmp + bw - 1ul)));
		writel(0, CONFIG_SYS_SDRAM_BASE);
		writel(PATTERN, test_addr);
		if ((readl(test_addr) == PATTERN) &&
		    (readl(CONFIG_SYS_SDRAM_BASE) == 0))
			bk = 3;
		else
			bk = 2;
		if (dram_type == DDR3)
			dbw = 1;
		else
			dbw = 2;
	} else {
		/* detect bg for ddr4 */
		coltmp = 10;
		bktmp = 4;
		rowtmp = 17;

		col = 10;
		bk = 2;
		test_addr = (void __iomem *)(CONFIG_SYS_SDRAM_BASE +
				(1ul << (coltmp + bw + 1ul)));
		writel(0, CONFIG_SYS_SDRAM_BASE);
		writel(PATTERN, test_addr);
		if ((readl(test_addr) == PATTERN) &&
		    (readl(CONFIG_SYS_SDRAM_BASE) == 0))
			dbw = 0;
		else
			dbw = 1;
	}
	/* detect row */
	for (row = rowtmp; row > 12; row--) {
		writel(0, CONFIG_SYS_SDRAM_BASE);
		test_addr = (void __iomem *)(CONFIG_SYS_SDRAM_BASE +
				(1ul << (row + bktmp + coltmp + bw - 1ul)));
		writel(PATTERN, test_addr);
		if ((readl(test_addr) == PATTERN) &&
		    (readl(CONFIG_SYS_SDRAM_BASE) == 0))
			break;
	}
	if (row == 12) {
		printascii("row error");
		goto cap_err;
	}
	/* detect row_3_4 */
	test_addr = CONFIG_SYS_SDRAM_BASE;
	test_addr1 = (void __iomem *)(CONFIG_SYS_SDRAM_BASE +
			(0x3ul << (row + bktmp + coltmp + bw - 1ul - 1ul)));

	writel(0, test_addr);
	writel(PATTERN, test_addr1);
	if ((readl(test_addr) == 0) &&
	    (readl(test_addr1) == PATTERN))
		row_3_4 = 0;
	else
		row_3_4 = 1;

	/* disable auto low-power */
	pwrctl = readl(pctl_base + DDR_PCTL2_PWRCTL);
	writel(0, pctl_base + DDR_PCTL2_PWRCTL);

	/* bw and cs detect using phy read gate training */
	if (data_training(dram, 1, dram_type) == 0)
		cs = 1;
	else
		cs = 0;

	clrsetbits_le32(PHY_REG(phy_base, 0), 0xf << 4, 0xf << 4);
	setbits_le32(PHY_REG(phy_base, 0x46), 1 << 3);
	setbits_le32(PHY_REG(phy_base, 0x56), 1 << 3);

	phy_soft_reset(dram);

	if (data_training(dram, 0, dram_type) == 0)
		bw = 2;
	else
		bw = 1;

	/* restore auto low-power */
	writel(pwrctl, pctl_base + DDR_PCTL2_PWRCTL);

	sdram_params->ch.rank = cs + 1;
	sdram_params->ch.col = col;
	sdram_params->ch.bk = bk;
	sdram_params->ch.dbw = dbw;
	sdram_params->ch.bw = bw;
	sdram_params->ch.cs0_row = row;
	sdram_params->ch.cs0_high16bit_row = row;
	if (cs) {
		sdram_params->ch.cs1_row = row;
		sdram_params->ch.cs1_high16bit_row = row;
	} else {
		sdram_params->ch.cs1_row = 0;
		sdram_params->ch.cs1_high16bit_row = 0;
	}
	sdram_params->ch.row_3_4 = row_3_4;

	if (dram_type == DDR4)
		cap = 1llu << (cs + row + bk + col + ((dbw == 0) ? 2 : 1) + bw);
	else
		cap = 1llu << (cs + row + bk + col + bw);

	return cap;

cap_err:
	return 0;
}

static u32 remodify_sdram_params(struct px30_sdram_params *sdram_params)
{
	u32 tmp = 0, tmp_adr = 0, i;

	for (i = 0; sdram_params->pctl_regs.pctl[i][0] != 0xFFFFFFFF; i++) {
		if (sdram_params->pctl_regs.pctl[i][0] == 0) {
			tmp = sdram_params->pctl_regs.pctl[i][1];/* MSTR */
			tmp_adr = i;
		}
	}

	tmp &= ~((3ul << 30) | (3ul << 24) | (3ul << 12));

	switch (sdram_params->ch.dbw) {
	case 2:
		tmp |= (3ul << 30);
		break;
	case 1:
		tmp |= (2ul << 30);
		break;
	case 0:
	default:
		tmp |= (1ul << 30);
		break;
	}

	/*
	 * If DDR3 or DDR4 MSTR.active_ranks=1,
	 * it will gate memory clock when enter power down.
	 * Force set active_ranks to 3 to workaround it.
	 */
	if (sdram_params->ch.rank == 2 || sdram_params->dramtype == DDR3 ||
	    sdram_params->dramtype == DDR4)
		tmp |= 3 << 24;
	else
		tmp |= 1 << 24;

	tmp |= (2 - sdram_params->ch.bw) << 12;

	sdram_params->pctl_regs.pctl[tmp_adr][1] = tmp;

	return 0;
}

int dram_detect_high_row(struct dram_info *dram,
			 struct px30_sdram_params *sdram_params,
			 unsigned char channel)
{
	sdram_params->ch.cs0_high16bit_row = sdram_params->ch.cs0_row;
	sdram_params->ch.cs1_high16bit_row = sdram_params->ch.cs1_row;

	return 0;
}

static int dram_detect_cs1_row(struct px30_sdram_params *sdram_params,
			       unsigned char channel)
{
	u32 ret = 0;
	void __iomem *test_addr;
	u32 row, bktmp, coltmp, bw;
	u64 cs0_cap;
	u32 byte_mask;

	if (sdram_params->ch.rank == 2) {
		cs0_cap = get_cs_cap(sdram_params, 0);

		if (sdram_params->dramtype == DDR4) {
			if (sdram_params->ch.dbw == 0)
				bktmp = sdram_params->ch.bk + 2;
			else
				bktmp = sdram_params->ch.bk + 1;
		} else {
			bktmp = sdram_params->ch.bk;
		}
		bw = sdram_params->ch.bw;
		coltmp = sdram_params->ch.col;

		/*
		 * because px30 support axi split,min bandwidth
		 * is 8bit. if cs0 is 32bit, cs1 may 32bit or 16bit
		 * so we check low 16bit data when detect cs1 row.
		 * if cs0 is 16bit/8bit, we check low 8bit data.
		 */
		if (bw == 2)
			byte_mask = 0xFFFF;
		else
			byte_mask = 0xFF;

		/* detect cs1 row */
		for (row = sdram_params->ch.cs0_row; row > 12; row--) {
			test_addr = (void __iomem *)(CONFIG_SYS_SDRAM_BASE +
				    cs0_cap +
				    (1ul << (row + bktmp + coltmp + bw - 1ul)));
			writel(0, CONFIG_SYS_SDRAM_BASE + cs0_cap);
			writel(PATTERN, test_addr);

			if (((readl(test_addr) & byte_mask) ==
			     (PATTERN & byte_mask)) &&
			    ((readl(CONFIG_SYS_SDRAM_BASE + cs0_cap) &
			      byte_mask) == 0)) {
				ret = row;
				break;
			}
		}
	}

	return ret;
}

/* return: 0 = success, other = fail */
static int sdram_init_detect(struct dram_info *dram,
			     struct px30_sdram_params *sdram_params)
{
	u32 ret;
	u32 sys_reg = 0;
	u32 sys_reg3 = 0;

	if (sdram_init_(dram, sdram_params, 0) != 0)
		return -1;

	if (dram_detect_cap(dram, sdram_params, 0) == 0)
		return -1;

	/* modify bw, cs related timing */
	remodify_sdram_params(sdram_params);
	/* reinit sdram by real dram cap */
	ret = sdram_init_(dram, sdram_params, 1);
	if (ret != 0)
		goto out;

	/* redetect cs1 row */
	sdram_params->ch.cs1_row =
		dram_detect_cs1_row(sdram_params, 0);
	if (sdram_params->ch.cs1_row) {
		sys_reg = readl(&dram->pmugrf->os_reg[2]);
		sys_reg3 = readl(&dram->pmugrf->os_reg[3]);
		SYS_REG_ENC_CS1_ROW_(sdram_params->ch.cs1_row,
				     sys_reg, sys_reg3);
		writel(sys_reg, &dram->pmugrf->os_reg[2]);
		writel(sys_reg3, &dram->pmugrf->os_reg[3]);
	}

	ret = dram_detect_high_row(dram, sdram_params, 0);

out:
	return ret;
}

struct px30_sdram_params
		*get_default_sdram_config(void)
{
	sdram_configs[0].skew = &skew;

	return &sdram_configs[0];
}

/* return: 0 = success, other = fail */
int sdram_init(void)
{
	struct px30_sdram_params *sdram_params;
	int ret = 0;

	dram_info.phy = (void *)DDR_PHY_BASE_ADDR;
	dram_info.pctl = (void *)DDRC_BASE_ADDR;
	dram_info.grf = (void *)GRF_BASE_ADDR;
	dram_info.cru = (void *)CRU_BASE_ADDR;
	dram_info.msch = (void *)SERVER_MSCH0_BASE_ADDR;
	dram_info.ddr_grf = (void *)DDR_GRF_BASE_ADDR;
	dram_info.pmugrf = (void *)PMUGRF_BASE_ADDR;

	sdram_params = get_default_sdram_config();
	ret = sdram_init_detect(&dram_info, sdram_params);

	if (ret)
		goto error;

	print_ddr_info(sdram_params);

	printascii("out\n");
	return ret;
error:
	return (-1);
}

#else /* CONFIG_TPL_BUILD */

static int px30_dmc_probe(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);

	priv->pmugrf = syscon_get_first_range(ROCKCHIP_SYSCON_PMUGRF);
	debug("%s: pmugrf=%p\n", __func__, priv->pmugrf);
	priv->info.base = CONFIG_SYS_SDRAM_BASE;
	priv->info.size =
		rockchip_sdram_size((phys_addr_t)&priv->pmugrf->os_reg[2]);

	return 0;
}

static int px30_dmc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops px30_dmc_ops = {
	.get_info = px30_dmc_get_info,
};

static const struct udevice_id px30_dmc_ids[] = {
	{ .compatible = "rockchip,px30-dmc" },
	{ }
};

U_BOOT_DRIVER(dmc_px30) = {
	.name = "rockchip_px30_dmc",
	.id = UCLASS_RAM,
	.of_match = px30_dmc_ids,
	.ops = &px30_dmc_ops,
	.probe = px30_dmc_probe,
	.priv_auto_alloc_size = sizeof(struct dram_info),
};
#endif /* CONFIG_TPL_BUILD */
