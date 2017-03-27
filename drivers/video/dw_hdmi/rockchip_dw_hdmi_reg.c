/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/rkplat.h>
#include "rockchip_dw_hdmi_reg.h"

#define HDMI_SEL_LCDC(x, bit)  ((((x) & 1) << bit) | (1 << (16 + bit)))
#define GRF_SOC_CON20 0x6250

static const struct phy_mpll_config_tab PHY_MPLL_TABLE[] = {
	/*tmdsclk = (pixclk / ref_cntrl ) * (div2 * div1) / nctrl / tmdsmhl
	  opmode: 0:HDMI1.4	1:HDMI2.0
	*/
/*	|pixclock|	tmdsclock|pixrepet|colordepth|prepdiv|tmdsmhl|opmode|
		div2|div1|ref_cntrl|nctrl|propctrl|intctrl|gmpctrl| */
	{27000000,	27000000,	0,	8,	0,	0,	0,
		2,	3,	0,	3,	3,	0,	0},
	{27000000,	33750000,	0,	10,	1,	0,	0,
		5,	1,	0,	3,	3,	0,	0},
	{27000000,	40500000,	0,	12,	2,	0,	0,
		3,	3,	0,	3,	3,	0,	0},
	{27000000,	54000000,	0,	16,	3,	0,	0,
		2,	3,	0,	2,	5,	0,	1},
/*	{74250000,	74250000,	0,	8,	0,	0,	0,
	1,	3,	0,	2,	5,	0,	1}, */
	{74250000,      74250000,	0,      8,      0,      0,      0,
		4,      3,      3,      2,      7,      0,      3},
	{74250000,	92812500,	0,	10,	1,	0,	0,
		5,	0,	1,	1,	7,	0,	2},
	{74250000,	111375000,	0,	12,	2,	0,	0,
		1,	2,	0,	1,	7,	0,	2},
	{74250000,	148500000,	0,	16,	3,	0,	0,
		1,	3,	0,	1,	7,	0,	2},
	{148500000,	74250000,	0,	8,	0,	0,	0,
		1,	1,	1,	1,	0,	0,	3},
	{148500000,	148500000,	0,	8,	0,	0,	0,
		1,	1,	0,	1,	0,	0,	3},
	{148500000,	185625000,	0,	10,	1,	0,	0,
		5,	0,	3,	0,	7,	0,	3},
	{148500000,	222750000,	0,	12,	2,	0,	0,
		1,	2,	1,	0,	7,	0,	3},
	{148500000,	297000000,	0,	16,	3,	0,	0,
		1,	1,	0,	0,	7,	0,	3},
	{297000000,	148500000,	0,	8,	0,	0,	0,
		1,	0,	1,	0,	0,	0,	3},
	{297000000,	297000000,	0,	8,	0,	0,	0,
		1,	0,	0,	0,	0,	0,	3},
	{297000000,	371250000,	0,	10,	1,	3,	1,
		5,	1,	3,	1,	7,	0,	3},
	{297000000,	445500000,	0,	12,	2,	3,	1,
		1,	2,	0,	1,	7,	0,	3},
	{297000000,	594000000,	0,	16,	3,	3,	1,
		1,	3,	1,	0,	0,	0,	3},
/*	{594000000,	297000000,	0,	8,	0,	0,	0,
		1,	3,	3,	1,	0,	0,	3},*/
	{594000000,	297000000,	0,	8,	0,	0,	0,
		1,	0,	1,	0,	0,	0,	3},
	{594000000,	371250000,	0,	10,	1,	3,	1,
		5,	0,	3,	0,	7,	0,	3},
	{594000000,	445500000,	0,	12,	2,	3,	1,
		1,	2,	1,	1,	7,	0,	3},
	{594000000,	594000000,	0,	16,	3,	3,	1,
		1,	3,	3,	0,	0,	0,	3},
	{594000000,	594000000,	0,	8,	0,	3,	1,
		1,	3,	3,	0,	0,	0,	3},
};

static const struct ext_pll_config_tab EXT_PLL_TABLE[] = {
	{27000000,	27000000,	8,	1,	90,	3,	2,
		2,	10,	3,	3,	4,	0,	1,	40,
		8},
	{27000000,	33750000,	10,	1,	90,	1,	3,
		3,	10,	3,	3,	4,	0,	1,	40,
		8},
	{59400000,	59400000,	8,	1,	99,	3,	1,
		1,	1,	3,	3,	4,	0,	1,	40,
		8},
	{59400000,	74250000,	10,	1,	99,	0,	3,
		3,	1,	3,	3,	4,	0,	1,	40,
		8},
	{74250000,	74250000,	8,	1,	99,	1,	2,
		2,	1,	2,	3,	4,	0,	1,	40,
		8},
	{74250000,	92812500,	10,	4,	495,	1,	2,
		2,	1,	3,	3,	4,	0,	2,	40,
		4},
	{148500000,	148500000,	8,	1,	99,	1,	1,
		1,	1,	2,	2,	2,	0,	2,	40,
		4},
	{148500000,	185625000,	10,	4,	495,	0,	2,
		2,	1,	3,	2,	2,	0,	4,	40,
		2},
	{297000000,	297000000,	8,	1,	99,	0,	1,
		1,	1,	0,	2,	2,	0,	4,	40,
		2},
	{297000000,	371250000,	10,	4,	495,	1,	2,
		0,	1,	3,	1,	1,	0,	8,	40,
		1},
	{594000000,	297000000,	8,	1,	99,	0,	1,
		1,	1,	0,	2,	1,	0,	4,	40,
		2},
	{594000000,	371250000,	10,	4,	495,	1,	2,
		0,	1,	3,	1,	1,	1,	8,	40,
		1},
	{594000000,	594000000,	8,	1,	99,	0,	2,
		0,	1,	0,	1,	1,	0,	8,	40,
		1},
};

static const struct ext_pll_config_tab RK322XH_V1_PLL_TABLE[] = {
	{27000000,	27000000,	8,	1,	90,	3,	2,
		2,	10,	3,	3,	4,	0,	18,	80,
		8},
	{27000000,	33750000,	10,	1,	90,	1,	3,
		3,	10,	3,	3,	4,	0,	1,	80,
		8},
	{59400000,	59400000,	8,	1,	99,	3,	1,
		1,	1,	3,	3,	4,	0,	18,	80,
		8},
	{59400000,	74250000,	10,	1,	99,	0,	3,
		3,	1,	3,	3,	4,	0,	18,	80,
		8},
	{74250000,	74250000,	8,	1,	99,	1,	2,
		2,	1,	2,	3,	4,	0,	18,	80,
		8},
};

/* ddc i2c master reset */
static void rockchip_hdmiv2_i2cm_reset(struct hdmi_dev *hdmi_dev)
{
	hdmi_msk_reg(hdmi_dev, I2CM_SOFTRSTZ,
		     m_I2CM_SOFTRST, v_I2CM_SOFTRST(0));
	udelay(100);
}

/*set read/write offset,set read/write mode*/
static void rockchip_hdmiv2_i2cm_write_request(struct hdmi_dev *hdmi_dev,
					       u8 offset, u8 data)
{
	hdmi_writel(hdmi_dev, I2CM_ADDRESS, offset);
	hdmi_writel(hdmi_dev, I2CM_DATAO, data);
	hdmi_msk_reg(hdmi_dev, I2CM_OPERATION, m_I2CM_WR, v_I2CM_WR(1));
}

static void rockchip_hdmiv2_i2cm_read_request(struct hdmi_dev *hdmi_dev,
					      u8 offset)
{
	hdmi_writel(hdmi_dev, I2CM_ADDRESS, offset);
	hdmi_msk_reg(hdmi_dev, I2CM_OPERATION, m_I2CM_RD, v_I2CM_RD(1));
}

static void rockchip_hdmiv2_i2cm_write_data(struct hdmi_dev *hdmi_dev,
					    u8 data, u8 offset)
{
	u8 interrupt = 0;
	int trytime = 2;
	int i = 20;

	while (trytime-- > 0) {
		rockchip_hdmiv2_i2cm_write_request(hdmi_dev, offset, data);
		while (i--) {
			udelay(1000);
			interrupt = hdmi_readl(hdmi_dev, IH_I2CM_STAT0);
			if (interrupt)
				hdmi_writel(hdmi_dev,
					    IH_I2CM_STAT0, interrupt);

			if (interrupt & (m_SCDC_READREQ |
					 m_I2CM_DONE | m_I2CM_ERROR))
				break;
		}

		if (interrupt & m_I2CM_DONE) {
			HDMIDBG("[%s] write offset %02x data %02x success\n",
				__func__, offset, data);
			trytime = 0;
		} else if ((interrupt & m_I2CM_ERROR) || (i == -1)) {
			HDMIDBG("[%s] write data error\n", __func__);
			rockchip_hdmiv2_i2cm_reset(hdmi_dev);
		}
	}
}

static int rockchip_hdmiv2_i2cm_read_data(struct hdmi_dev *hdmi_dev, u8 offset)
{
	u8 interrupt = 0, val = 0;
	int trytime = 2;
	int i = 20;

	while (trytime-- > 0) {
		rockchip_hdmiv2_i2cm_read_request(hdmi_dev, offset);
		while (i--) {
			udelay(1000);
			interrupt = hdmi_readl(hdmi_dev, IH_I2CM_STAT0);
			if (interrupt)
				hdmi_writel(hdmi_dev, IH_I2CM_STAT0, interrupt);

			if (interrupt & (m_SCDC_READREQ |
				m_I2CM_DONE | m_I2CM_ERROR))
				break;
		}

		if (interrupt & m_I2CM_DONE) {
			val = hdmi_readl(hdmi_dev, I2CM_DATAI);
			trytime = 0;
		} else if ((interrupt & m_I2CM_ERROR) || (i == -1)) {
			printf("[%s] read data error\n", __func__);
			rockchip_hdmiv2_i2cm_reset(hdmi_dev);
		}
	}
	return val;
}

static void rockchip_hdmiv2_i2cm_mask_int(struct hdmi_dev *hdmi_dev, int mask)
{
	if (0 == mask) {
		hdmi_msk_reg(hdmi_dev, I2CM_INT,
			     m_I2CM_DONE_MASK, v_I2CM_DONE_MASK(0));
		hdmi_msk_reg(hdmi_dev, I2CM_CTLINT,
			     m_I2CM_NACK_MASK | m_I2CM_ARB_MASK,
			     v_I2CM_NACK_MASK(0) | v_I2CM_ARB_MASK(0));
	} else {
		hdmi_msk_reg(hdmi_dev, I2CM_INT,
			     m_I2CM_DONE_MASK, v_I2CM_DONE_MASK(1));
		hdmi_msk_reg(hdmi_dev, I2CM_CTLINT,
			     m_I2CM_NACK_MASK | m_I2CM_ARB_MASK,
			     v_I2CM_NACK_MASK(1) | v_I2CM_ARB_MASK(1));
	}
}

#define I2C_DIV_FACTOR 1000000
static u16 i2c_count(u16 sfrclock, u16 sclmintime)
{
	unsigned long tmp_scl_period = 0;

	if (((sfrclock * sclmintime) % I2C_DIV_FACTOR) != 0)
		tmp_scl_period = (unsigned long)((sfrclock * sclmintime) +
				(I2C_DIV_FACTOR - ((sfrclock * sclmintime) %
				I2C_DIV_FACTOR))) / I2C_DIV_FACTOR;
	else
		tmp_scl_period = (unsigned long)(sfrclock * sclmintime) /
				I2C_DIV_FACTOR;

	return (u16)(tmp_scl_period);
}

#define EDID_I2C_MIN_SS_SCL_HIGH_TIME	9625
#define EDID_I2C_MIN_SS_SCL_LOW_TIME	10000

static void rockchip_hdmiv2_i2cm_clk_init(struct hdmi_dev *hdmi_dev)
{
	int value;

	/* Set DDC I2C CLK which devided from DDC_CLK. */
	value = i2c_count(24000, EDID_I2C_MIN_SS_SCL_HIGH_TIME);
	hdmi_writel(hdmi_dev, I2CM_SS_SCL_HCNT_0_ADDR,
		    value & 0xff);
	hdmi_writel(hdmi_dev, I2CM_SS_SCL_HCNT_1_ADDR,
		    (value >> 8) & 0xff);
	value = i2c_count(24000, EDID_I2C_MIN_SS_SCL_LOW_TIME);
	hdmi_writel(hdmi_dev, I2CM_SS_SCL_LCNT_0_ADDR,
		    value & 0xff);
	hdmi_writel(hdmi_dev, I2CM_SS_SCL_LCNT_1_ADDR,
		    (value >> 8) & 0xff);
	hdmi_msk_reg(hdmi_dev, I2CM_DIV, m_I2CM_FAST_STD_MODE,
		     v_I2CM_FAST_STD_MODE(STANDARD_MODE));
}

static int rockchip_hdmiv2_scdc_get_sink_version(struct hdmi_dev *hdmi_dev)
{
	return rockchip_hdmiv2_i2cm_read_data(hdmi_dev, SCDC_SINK_VER);
}

static void rockchip_hdmiv2_scdc_set_source_version(struct hdmi_dev *hdmi_dev,
						    u8 version)
{
	rockchip_hdmiv2_i2cm_write_data(hdmi_dev, version, SCDC_SOURCE_VER);
}


static void rockchip_hdmiv2_scdc_read_request(struct hdmi_dev *hdmi_dev,
					      int enable)
{
	hdmi_msk_reg(hdmi_dev, I2CM_SCDC_READ_UPDATE,
		     m_I2CM_READ_REQ_EN, v_I2CM_READ_REQ_EN(enable));
	rockchip_hdmiv2_i2cm_write_data(hdmi_dev, enable, SCDC_CONFIG_0);
}

#ifdef HDMI_20_SCDC
static void rockchip_hdmiv2_scdc_update_read(struct hdmi_dev *hdmi_dev)
{
	hdmi_msk_reg(hdmi_dev, I2CM_SCDC_READ_UPDATE,
		     m_I2CM_READ_UPDATE, v_I2CM_READ_UPDATE(1));
}


static int rockchip_hdmiv2_scdc_get_scambling_status(struct hdmi_dev *hdmi_dev)
{
	int val;

	val = rockchip_hdmiv2_i2cm_read_data(hdmi_dev, SCDC_SCRAMBLER_STAT);
	return val;
}

static void rockchip_hdmiv2_scdc_enable_polling(struct hdmi_dev *hdmi_dev,
						int enable)
{
	rockchip_hdmiv2_scdc_read_request(hdmi_dev, enable);
	hdmi_msk_reg(hdmi_dev, I2CM_SCDC_READ_UPDATE,
		     m_I2CM_UPRD_VSYNC_EN, v_I2CM_UPRD_VSYNC_EN(enable));
}

static int rockchip_hdmiv2_scdc_get_status_reg0(struct hdmi_dev *hdmi_dev)
{
	rockchip_hdmiv2_scdc_read_request(hdmi_dev, 1);
	rockchip_hdmiv2_scdc_update_read(hdmi_dev);
	return hdmi_readl(hdmi_dev, I2CM_SCDC_UPDATE0);
}

static int rockchip_hdmiv2_scdc_get_status_reg1(struct hdmi_dev *hdmi_dev)
{
	rockchip_hdmiv2_scdc_read_request(hdmi_dev, 1);
	rockchip_hdmiv2_scdc_update_read(hdmi_dev);
	return hdmi_readl(hdmi_dev, I2CM_SCDC_UPDATE1);
}
#endif

static void rockchip_hdmiv2_scdc_init(struct hdmi_dev *hdmi_dev)
{
	rockchip_hdmiv2_i2cm_reset(hdmi_dev);
	rockchip_hdmiv2_i2cm_mask_int(hdmi_dev, 1);
	rockchip_hdmiv2_i2cm_clk_init(hdmi_dev);
	/* set scdc i2c addr */
	hdmi_writel(hdmi_dev, I2CM_SLAVE, DDC_I2C_SCDC_ADDR);
	rockchip_hdmiv2_i2cm_mask_int(hdmi_dev, 0);/*enable interrupt*/
}


static int rockchip_hdmiv2_scrambling_enable(struct hdmi_dev *hdmi_dev,
					     int enable)
{
	HDMIDBG("%s enable %d\n", __func__, enable);
	if (1 == enable) {
		/* Write on Rx the bit Scrambling_Enable, register 0x20 */
		rockchip_hdmiv2_i2cm_write_data(hdmi_dev, 1, SCDC_TMDS_CONFIG);
		/* TMDS software reset request */
		hdmi_msk_reg(hdmi_dev, MC_SWRSTZREQ,
			     m_TMDS_SWRST, v_TMDS_SWRST(0));
		/* Enable/Disable Scrambling */
		hdmi_msk_reg(hdmi_dev, FC_SCRAMBLER_CTRL,
			     m_FC_SCRAMBLE_EN, v_FC_SCRAMBLE_EN(1));
	} else {
		/* Enable/Disable Scrambling */
		hdmi_msk_reg(hdmi_dev, FC_SCRAMBLER_CTRL,
			     m_FC_SCRAMBLE_EN, v_FC_SCRAMBLE_EN(0));
		/* TMDS software reset request */
		hdmi_msk_reg(hdmi_dev, MC_SWRSTZREQ,
			     m_TMDS_SWRST, v_TMDS_SWRST(0));
		/* Write on Rx the bit Scrambling_Enable, register 0x20 */
		rockchip_hdmiv2_i2cm_write_data(hdmi_dev, 0, SCDC_TMDS_CONFIG);
	}
	return 0;
}

static void rockchip_hdmiv2_scdc_set_tmds_rate(struct hdmi_dev *hdmi_dev)
{
	int stat;

	rockchip_hdmiv2_scdc_init(hdmi_dev);
	stat = rockchip_hdmiv2_i2cm_read_data(hdmi_dev,
					      SCDC_TMDS_CONFIG);
	if (hdmi_dev->tmdsclk > 340000000)
		stat |= 2;
	else
		stat &= 0x1;
	rockchip_hdmiv2_i2cm_write_data(hdmi_dev,
					stat, SCDC_TMDS_CONFIG);
}

static const struct ext_pll_config_tab *get_phy_ext_tab(
		struct hdmi_dev *hdmi_dev)
{
	const struct ext_pll_config_tab *ext_pll[2];
	int i, j, table, size[2];

	if (!hdmi_dev || hdmi_dev->pixelclk == 0)
		return NULL;
	HDMIDBG(2, "%s pixClock %u tmdsclk %u colorDepth %d\n",
		__func__, (u32)hdmi_dev->pixelclk, hdmi_dev->tmdsclk,
		hdmi_dev->colordepth);
	ext_pll[1] = RK322XH_V1_PLL_TABLE;
	size[1] =  ARRAY_SIZE(RK322XH_V1_PLL_TABLE);
	ext_pll[0] = EXT_PLL_TABLE;
	size[0] = ARRAY_SIZE(EXT_PLL_TABLE);

	if (hdmi_dev->soctype == HDMI_SOC_RK322XH &&
	    rk_get_cpu_version())
		table = 1;
	else
		table = 0;
	for (j = table; j >= 0; j--) {
		for (i = 0; i < size[j]; i++) {
			if ((ext_pll[j][i].pix_clock == hdmi_dev->pixelclk) &&
			    (ext_pll[j][i].tmdsclock == hdmi_dev->tmdsclk) &&
			    (ext_pll[j][i].color_depth == hdmi_dev->colordepth))
				return &ext_pll[j][i];
		}
	}
	return NULL;
}

static const struct phy_mpll_config_tab *get_phy_mpll_tab(
		unsigned int pixclock, unsigned int tmdsclk,
		char pixrepet, char colordepth)
{
	int i;

	if (pixclock == 0)
		return NULL;
	HDMIDBG("%s pixClock %u pixRepet %d colorDepth %d\n",
		__func__, pixclock, pixrepet, colordepth);
	for (i = 0; i < ARRAY_SIZE(PHY_MPLL_TABLE); i++) {
		if ((PHY_MPLL_TABLE[i].pix_clock == pixclock) &&
		    (PHY_MPLL_TABLE[i].tmdsclock == tmdsclk) &&
		    (PHY_MPLL_TABLE[i].pix_repet == pixrepet) &&
		    (PHY_MPLL_TABLE[i].color_depth == colordepth))
			return &PHY_MPLL_TABLE[i];
	}
	return NULL;
}

int hdmi_dev_read_edid(struct hdmi_dev *hdmi_dev,
			      int block, unsigned char *buff)
{
	int i = 0, n = 0, index = 0, ret = -1, trytime = 2;
	int offset = (block % 2) * 0x80;
	int interrupt = 0;

	HDMIDBG("[%s] block %d\n", __func__, block);

	rockchip_hdmiv2_i2cm_reset(hdmi_dev);

	/* Set DDC I2C CLK which devided from DDC_CLK to 100KHz. */
	rockchip_hdmiv2_i2cm_clk_init(hdmi_dev);

	/* Enable I2C interrupt for reading edid */
	rockchip_hdmiv2_i2cm_mask_int(hdmi_dev, 0);

	hdmi_writel(hdmi_dev, I2CM_SLAVE, DDC_I2C_EDID_ADDR);
	hdmi_writel(hdmi_dev, I2CM_SEGADDR, DDC_I2C_SEG_ADDR);
	hdmi_writel(hdmi_dev, I2CM_SEGPTR, block / 2);
	while (trytime--) {
		for (n = 0; n < HDMI_EDID_BLOCK_SIZE / 8; n++) {
			hdmi_writel(hdmi_dev, I2CM_ADDRESS, offset + 8 * n);
			/*enable extend sequential read operation*/
			if (block == 0)
				hdmi_msk_reg(hdmi_dev, I2CM_OPERATION,
					     m_I2CM_RD8, v_I2CM_RD8(1));
			else
				hdmi_msk_reg(hdmi_dev, I2CM_OPERATION,
					     m_I2CM_RD8_EXT, v_I2CM_RD8_EXT(1));

			i = 20;
			while (i--) {
				mdelay(1);
				interrupt = hdmi_readl(hdmi_dev,
						       IH_I2CM_STAT0);
				if (interrupt)
					hdmi_writel(hdmi_dev,
						    IH_I2CM_STAT0,
						    interrupt);

				if (interrupt &
				    (m_SCDC_READREQ |
				     m_I2CM_DONE |
				     m_I2CM_ERROR))
					break;
				mdelay(4);
			}

			if (interrupt & m_I2CM_DONE) {
				for (index = 0; index < 8; index++)
					buff[8 * n + index] =
					hdmi_readl(hdmi_dev,
						   I2CM_READ_BUFF0 + index);

				if (n == HDMI_EDID_BLOCK_SIZE / 8 - 1) {
					ret = 0;
					HDMIDBG("[%s] edid read sucess\n",
						__func__);

					#ifdef HDMIDEBUG
					for (index = 0; index < 128; index++) {
						printf("0x%02x ,", buff[index]);
						if ((index + 1) % 16 == 0)
							printf("\n");
					}
					#endif
					goto exit;
				}
				continue;
			} else if ((interrupt & m_I2CM_ERROR) || (i == -1)) {
				printf("[%s] edid read error\n", __func__);
				rockchip_hdmiv2_i2cm_reset(hdmi_dev);
				break;
			}
		}

		printf("[%s] edid try times %d\n", __func__, trytime);
		mdelay(100);
	}

exit:
	/* Disable I2C interrupt */
	rockchip_hdmiv2_i2cm_mask_int(hdmi_dev, 1);
	return ret;
}

static int rockchip_hdmiv2_read_phy(struct hdmi_dev *hdmi_dev,
			     int reg_addr)
{
	int trytime = 2, i = 0, op_status = 0;
	int val = 0;

	if (hdmi_dev->phybase)
		return readl(hdmi_dev->phybase + (reg_addr) * 0x04);

	while (trytime--) {
		hdmi_writel(hdmi_dev, PHY_I2CM_ADDRESS, reg_addr);
		hdmi_writel(hdmi_dev, PHY_I2CM_DATAI_1, 0x00);
		hdmi_writel(hdmi_dev, PHY_I2CM_DATAI_0, 0x00);
		hdmi_writel(hdmi_dev, PHY_I2CM_OPERATION, m_PHY_I2CM_READ);

		i = 20;
		while (i--) {
			mdelay(1);
			op_status = hdmi_readl(hdmi_dev, IH_I2CMPHY_STAT0);
			if (op_status)
				hdmi_writel(hdmi_dev, IH_I2CMPHY_STAT0,
					    op_status);

			if (op_status & (m_I2CMPHY_DONE | m_I2CMPHY_ERR))
				break;
		}

		if (op_status & m_I2CMPHY_DONE) {
			val = hdmi_readl(hdmi_dev, PHY_I2CM_DATAI_1);
			val = (val & 0xff) << 8;
			val += (hdmi_readl(hdmi_dev, PHY_I2CM_DATAI_0) & 0xff);
			return val;
		} else {
			printf("[%s] operation error,trytime=%d\n",
			       __func__, trytime);
		}
		mdelay(100);
	}

	return -EPERM;
}

static int rockchip_hdmiv2_write_phy(struct hdmi_dev *hdmi_dev,
				     int reg_addr, int val)
{
	int trytime = 2, i = 0, op_status = 0;

	if (hdmi_dev->phybase) {
		writel(val, hdmi_dev->phybase + (reg_addr) * 0x04);
		return 0;
	}
	while (trytime--) {
		hdmi_writel(hdmi_dev, PHY_I2CM_ADDRESS, reg_addr);
		hdmi_writel(hdmi_dev, PHY_I2CM_DATAO_1, (val >> 8) & 0xff);
		hdmi_writel(hdmi_dev, PHY_I2CM_DATAO_0, val & 0xff);
		hdmi_writel(hdmi_dev, PHY_I2CM_OPERATION, m_PHY_I2CM_WRITE);

		i = 20;
		while (i--) {
			mdelay(1);
			op_status = hdmi_readl(hdmi_dev, IH_I2CMPHY_STAT0);
			if (op_status)
				hdmi_writel(hdmi_dev,
					    IH_I2CMPHY_STAT0,
					    op_status);

			if (op_status & (m_I2CMPHY_DONE | m_I2CMPHY_ERR))
				break;
			mdelay(4);
		}

		if (op_status & m_I2CMPHY_DONE)
			return 0;
		else
			printf("[%s] operation error,trytime=%d\n",
			       __func__, trytime);
		mdelay(100);
	}

	return -EPERM;
}

static void rk32_hdmi_powerdown(struct hdmi_dev *hdmi_dev)
{
	if (hdmi_dev->soctype == HDMI_SOC_RK322X) {
		hdmi_msk_reg(hdmi_dev, PHY_CONF0,
			     m_TXPWRON_SIG, v_TXPWRON_SIG(0));
		/* PHY PLL VCO is 2160MHz, output pclk is 27MHz*/
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PLL_PRE_DIVIDER,
					  1);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PLL_FB_DIVIDER,
					  0x5a);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PCLK_DIVIDER1,
					  0x6a);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PCLK_DIVIDER2,
					  0x64);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_TMDSCLK_DIVIDER,
					  0x37);
	} else if (hdmi_dev->soctype == HDMI_SOC_RK322XH) {
		hdmi_msk_reg(hdmi_dev, PHY_CONF0,
			     m_TXPWRON_SIG, v_TXPWRON_SIG(0));
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PLL_PRE_DIVIDER,
					  1);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PLL_FB_DIVIDER,
					  0x5a);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PCLK_DIVIDER1,
					  0x6a);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PCLK_DIVIDER2,
					  0x64);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_TMDSCLK_DIVIDER,
					  0x3a);
	} else {
		hdmi_msk_reg(hdmi_dev, PHY_CONF0,
			     m_PDDQ_SIG | m_TXPWRON_SIG | m_ENHPD_RXSENSE_SIG,
			     v_PDDQ_SIG(1) | v_TXPWRON_SIG(0) |
			     v_ENHPD_RXSENSE_SIG(1));
	}
	hdmi_writel(hdmi_dev, MC_CLKDIS, 0x7f);
}

int hdmi_dev_detect_hotplug(struct hdmi_dev *hdmi_dev)
{
	u32 value = hdmi_readl(hdmi_dev, PHY_STAT0);

	if (value & m_PHY_HPD)
		return HDMI_HPD_ACTIVED;
	else
		return HDMI_HPD_REMOVED;
}

void hdmi_dev_init(struct hdmi_dev *hdmi_dev)
{
	int val, shift;

	/*lcdc source select*/
	/*grf_writel(HDMI_SEL_LCDC(0), GRF_SOC_CON6);*/

	/*set edid gpio to high Z mode*/
#ifdef CONFIG_RKCHIP_RK3288
	grf_writel(0xF << 22, GRF_GPIO7C_P);
	val = 0x01d4;
	shift = 9;
#endif
#ifdef CONFIG_RKCHIP_RK3368
	grf_writel(0xF << 20, GRF_GPIO3D_P);
	val = 0x031c;
	shift = 9;
#endif
#ifdef CONFIG_RKCHIP_RK322X
	grf_writel(0xf000a000, GRF_GPIO0A_IOMUX);
	grf_writel(0xF << 28, GRF_GPIO0A_P);
	grf_writel(0x40004000, GRF_GPIO0B_IOMUX);
	grf_writel(RK322X_DDC_MASK_EN, GRF_SOC_CON2);
	grf_writel(RK322X_IO_3V_DOMAIN, GRF_SOC_CON6);
	val = 0x128;
	shift = 0;
#endif
#ifdef CONFIG_RKCHIP_RK322XH
	grf_writel(0x3f001500, GRF_GPIO0A_IOMUX);
	grf_writel(0xF << 26, GRF_GPIO0A_P);
	grf_writel(RK322XH_IO_3V_DOMAIN | RK322XH_HPD_3V,
		   GRF_SOC_CON4);
	grf_writel(RK322XH_IO_CTRL_BY_HDMI, GRF_SOC_CON3);
	grf_writel(RK322XH_DDC_MASK_EN | (1 << 18), GRF_SOC_CON2);
	val = 0x320;
	shift = 15;
#endif
#ifdef CONFIG_RKCHIP_RK3399
	val = 0x440;
	shift = 4;
#endif
	/* reset hdmi */
	writel((1 << shift) | (1 << (shift + 16)), RKIO_CRU_PHYS + val);
	udelay(1);
	writel((1 << (shift + 16)), RKIO_CRU_PHYS + val);
	if (hdmi_dev->soctype == HDMI_SOC_RK322X ||
	    hdmi_dev->soctype == HDMI_SOC_RK322XH) {
		rockchip_hdmiv2_write_phy(hdmi_dev, 0x00, 0x00);
		udelay(10);
		rockchip_hdmiv2_write_phy(hdmi_dev, 0x00, 0xc0);
	}
	rk32_hdmi_powerdown(hdmi_dev);

	/*mute unnecessary interrrupt, only enable hpd */
	hdmi_writel(hdmi_dev, IH_MUTE_FC_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_FC_STAT1, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_FC_STAT2, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_AS_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_PHY_STAT0, 0xfe);
	hdmi_writel(hdmi_dev, IH_MUTE_I2CM_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_CEC_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_VP_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_I2CMPHY_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_AHBDMAAUD_STAT0, 0xff);

	hdmi_writel(hdmi_dev, PHY_MASK, 0xf1);

	/*Force output black*/
	hdmi_writel(hdmi_dev, FC_DBGTMDS2, 0x00);   /*R*/
	hdmi_writel(hdmi_dev, FC_DBGTMDS1, 0x00);   /*G*/
	hdmi_writel(hdmi_dev, FC_DBGTMDS0, 0x00);   /*B*/
}

static int rk32_hdmi_video_framecomposer(struct hdmi_dev *hdmi_dev,
					 struct hdmi_video *vpara)
{
	int value, vsync_pol, hsync_pol, de_pol;
	struct hdmi_video_timing *timing = NULL;
	struct video_mode *mode = NULL;
	int sink_version;

	vsync_pol = 0;
	hsync_pol = 0;
	de_pol = 1;

	timing = (struct hdmi_video_timing *)dw_hdmi_vic2timing(hdmi_dev,
							     vpara->vic);
	if (timing == NULL) {
		printf("[%s] not found vic %d\n", __func__, vpara->vic);
		return -EFAULT;
	}
	mode = &(timing->mode);
	if (vpara->color_input == HDMI_COLOR_YCBCR420)
		hdmi_dev->tmdsclk = mode->pixclock / 2;
	else
		hdmi_dev->tmdsclk = mode->pixclock;
	if (vpara->color_output != HDMI_COLOR_YCBCR422) {
		switch (vpara->color_output_depth) {
		case 10:
			hdmi_dev->tmdsclk += hdmi_dev->tmdsclk / 4;
			break;
		case 12:
			hdmi_dev->tmdsclk += hdmi_dev->tmdsclk / 2;
			break;
		case 16:
			hdmi_dev->tmdsclk += hdmi_dev->tmdsclk;
			break;
		case 8:
		default:
			break;
		}
	} else if (vpara->color_output_depth > 12) {
		/* YCbCr422 mode only support up to 12bit */
		vpara->color_output_depth = 12;
	}

	if ((hdmi_dev->tmdsclk > 594000000) ||
	    (hdmi_dev->tmdsclk > 340000000 &&
	     hdmi_dev->tmdsclk > hdmi_dev->driver.edid.maxtmdsclock)) {
		vpara->color_output_depth = 8;
		if (vpara->color_input == HDMI_COLOR_YCBCR420)
			hdmi_dev->tmdsclk = mode->pixclock / 2;
		else
			hdmi_dev->tmdsclk = mode->pixclock;
	}
	printf("pixel clk is %u tmds clk is %lu\n",
	       mode->pixclock, hdmi_dev->tmdsclk);
	if (hdmi_dev->tmdsclk > 340000000)
		hdmi_dev->tmdsclk_ratio_change = true;
	else
		hdmi_dev->tmdsclk_ratio_change = false;

	hdmi_dev->pixelclk = mode->pixclock;
	hdmi_dev->pixelrepeat = timing->pixelrepeat;
	/* hdmi_dev->colordepth is used for find pll config.
	 * For YCbCr422, tmdsclk is same on all color depth.
	 */
	if (vpara->color_output == HDMI_COLOR_YCBCR422)
		hdmi_dev->colordepth = 8;
	else
		hdmi_dev->colordepth = vpara->color_output_depth;

	hdmi_msk_reg(hdmi_dev, FC_INVIDCONF,
		     m_FC_HDCP_KEEPOUT, v_FC_HDCP_KEEPOUT(1));
	if (hdmi_dev->driver.edid.scdc_present == 1) {
		if (hdmi_dev->tmdsclk > 340000000) {/* used for HDMI 2.0 TX */
			rockchip_hdmiv2_scdc_init(hdmi_dev);
			sink_version =
			rockchip_hdmiv2_scdc_get_sink_version(hdmi_dev);
			printf("sink scdc version is %d\n", sink_version);
			sink_version =
			hdmi_dev->driver.edid.hf_vsdb_version;
			rockchip_hdmiv2_scdc_set_source_version(hdmi_dev,
								sink_version);
			if (hdmi_dev->driver.edid.rr_capable == 1)
				rockchip_hdmiv2_scdc_read_request(hdmi_dev, 1);
			rockchip_hdmiv2_scrambling_enable(hdmi_dev, 1);
		} else {
			rockchip_hdmiv2_scdc_init(hdmi_dev);
			rockchip_hdmiv2_scrambling_enable(hdmi_dev, 0);
		}
	}

	if (timing->mode.sync & FB_SYNC_HOR_HIGH_ACT)
		hsync_pol = 1;
	if (timing->mode.sync & FB_SYNC_VERT_HIGH_ACT)
		vsync_pol = 1;
	printf("hsync_pol %d vsync_pol %d\n", hsync_pol, vsync_pol);
	hdmi_msk_reg(hdmi_dev, A_HDCPCFG0, m_ENCRYPT_BYPASS | m_HDMI_DVI,
		     v_ENCRYPT_BYPASS(1) | v_HDMI_DVI(vpara->sink_hdmi));
	hdmi_msk_reg(hdmi_dev, FC_INVIDCONF,
		     m_FC_VSYNC_POL | m_FC_HSYNC_POL | m_FC_DE_POL |
		     m_FC_HDMI_DVI | m_FC_INTERLACE_MODE,
		     v_FC_VSYNC_POL(vsync_pol) | v_FC_HSYNC_POL(hsync_pol) |
		     v_FC_DE_POL(de_pol) | v_FC_HDMI_DVI(vpara->sink_hdmi) |
		     v_FC_INTERLACE_MODE(mode->vmode));

	hdmi_msk_reg(hdmi_dev, FC_INVIDCONF, m_FC_VBLANK, v_FC_VBLANK(0));

	value = mode->xres;
	if (vpara->color_input == HDMI_COLOR_YCBCR420)
		value = value / 2;
	hdmi_writel(hdmi_dev, FC_INHACTIV1, v_FC_HACTIVE1(value >> 8));
	hdmi_writel(hdmi_dev, FC_INHACTIV0, (value & 0xff));

	value = mode->yres;
	hdmi_writel(hdmi_dev, FC_INVACTIV1, v_FC_VACTIVE1(value >> 8));
	hdmi_writel(hdmi_dev, FC_INVACTIV0, (value & 0xff));

	value = mode->hsync_len + mode->left_margin + mode->right_margin;
	if (vpara->color_input == HDMI_COLOR_YCBCR420)
		value = value / 2;
	hdmi_writel(hdmi_dev, FC_INHBLANK1, v_FC_HBLANK1(value >> 8));
	hdmi_writel(hdmi_dev, FC_INHBLANK0, (value & 0xff));

	value = mode->vsync_len + mode->upper_margin + mode->lower_margin;
	hdmi_writel(hdmi_dev, FC_INVBLANK, (value & 0xff));

	value = mode->right_margin;
	if (vpara->color_input == HDMI_COLOR_YCBCR420)
		value = value / 2;
	hdmi_writel(hdmi_dev, FC_HSYNCINDELAY1, v_FC_HSYNCINDEAY1(value >> 8));
	hdmi_writel(hdmi_dev, FC_HSYNCINDELAY0, (value & 0xff));

	value = mode->lower_margin;
	hdmi_writel(hdmi_dev, FC_VSYNCINDELAY, (value & 0xff));

	value = mode->hsync_len;
	if (vpara->color_input == HDMI_COLOR_YCBCR420)
		value = value / 2;
	hdmi_writel(hdmi_dev, FC_HSYNCINWIDTH1, v_FC_HSYNCWIDTH1(value >> 8));
	hdmi_writel(hdmi_dev, FC_HSYNCINWIDTH0, (value & 0xff));

	value = mode->vsync_len;
	hdmi_writel(hdmi_dev, FC_VSYNCINWIDTH, (value & 0xff));

	/*Set the control period minimum duration(min.
	  of 12 pixel clock cycles, refer to HDMI 1.4b specification)*/
	hdmi_writel(hdmi_dev, FC_CTRLDUR, 12);
	hdmi_writel(hdmi_dev, FC_EXCTRLDUR, 32);

	hdmi_writel(hdmi_dev, FC_PRCONF,
		    v_FC_PR_FACTOR(timing->pixelrepeat) |
		    v_FC_PR_FACTOR_OUT(timing->pixelrepeat - 1));

	hdmi_msk_reg(hdmi_dev, A_VIDPOLCFG,
		     m_DATAEN_POL | m_VSYNC_POL | m_HSYNC_POL,
		     v_DATAEN_POL(de_pol) |
		     v_VSYNC_POL(vsync_pol) |
		     v_HSYNC_POL(hsync_pol));
	return 0;
}

static int rk32_hdmi_video_packetizer(struct hdmi_dev *hdmi_dev,
				      struct hdmi_video *vpara)
{
	unsigned char color_depth = COLOR_DEPTH_24BIT_DEFAULT;
	unsigned char output_select = 0;
	unsigned char remap_size = 0;

	if (vpara->color_output == HDMI_COLOR_YCBCR422) {
		switch (vpara->color_output_depth) {
		case 8:
			remap_size = YCC422_16BIT;
			break;
		case 10:
			remap_size = YCC422_20BIT;
			break;
		case 12:
			remap_size = YCC422_24BIT;
			break;
		default:
			remap_size = YCC422_16BIT;
			break;
		}

		output_select = OUT_FROM_YCC422_REMAP;
		/*Config remap size for the different color Depth*/
		hdmi_msk_reg(hdmi_dev, VP_REMAP,
			     m_YCC422_SIZE, v_YCC422_SIZE(remap_size));
	} else {
		switch (vpara->color_output_depth) {
		case 10:
			color_depth = COLOR_DEPTH_30BIT;
			output_select = OUT_FROM_PIXEL_PACKING;
			break;
		case 12:
			color_depth = COLOR_DEPTH_36BIT;
			output_select = OUT_FROM_PIXEL_PACKING;
			break;
		case 16:
			color_depth = COLOR_DEPTH_48BIT;
			output_select = OUT_FROM_PIXEL_PACKING;
			break;
		case 8:
		default:
			color_depth = COLOR_DEPTH_24BIT_DEFAULT;
			output_select = OUT_FROM_8BIT_BYPASS;
			break;
		}
	}
	/*Config Color Depth*/
	hdmi_msk_reg(hdmi_dev, VP_PR_CD,
		     m_COLOR_DEPTH, v_COLOR_DEPTH(color_depth));
	/*Config pixel repettion*/
	hdmi_msk_reg(hdmi_dev, VP_PR_CD, m_DESIRED_PR_FACTOR,
		     v_DESIRED_PR_FACTOR(hdmi_dev->pixelrepeat - 1));
	if (hdmi_dev->pixelrepeat > 1)
		hdmi_msk_reg(hdmi_dev, VP_CONF,
			     m_PIXEL_REPET_EN | m_BYPASS_SEL,
			     v_PIXEL_REPET_EN(1) | v_BYPASS_SEL(0));
	else
		hdmi_msk_reg(hdmi_dev, VP_CONF,
			     m_PIXEL_REPET_EN | m_BYPASS_SEL,
			     v_PIXEL_REPET_EN(0) | v_BYPASS_SEL(1));

	/*config output select*/
	if (output_select == OUT_FROM_PIXEL_PACKING) { /* pixel packing */
		hdmi_msk_reg(hdmi_dev, VP_CONF,
			     m_BYPASS_EN | m_PIXEL_PACK_EN |
			     m_YCC422_EN | m_OUTPUT_SEL,
			     v_BYPASS_EN(0) | v_PIXEL_PACK_EN(1) |
			     v_YCC422_EN(0) | v_OUTPUT_SEL(output_select));
	} else if (output_select == OUT_FROM_YCC422_REMAP) { /* YCC422 */
		hdmi_msk_reg(hdmi_dev, VP_CONF,
			     m_BYPASS_EN | m_PIXEL_PACK_EN |
			     m_YCC422_EN | m_OUTPUT_SEL,
			     v_BYPASS_EN(0) | v_PIXEL_PACK_EN(0) |
			     v_YCC422_EN(1) | v_OUTPUT_SEL(output_select));
	} else if (output_select == OUT_FROM_8BIT_BYPASS ||
		   output_select == 3) { /* bypass */
		hdmi_msk_reg(hdmi_dev, VP_CONF,
			     m_BYPASS_EN | m_PIXEL_PACK_EN |
			     m_YCC422_EN | m_OUTPUT_SEL,
			     v_BYPASS_EN(1) | v_PIXEL_PACK_EN(0) |
			     v_YCC422_EN(0) | v_OUTPUT_SEL(output_select));
	}

#if defined(HDMI_VIDEO_STUFFING)
	/* YCC422 and pixel packing stuffing*/
	hdmi_msk_reg(hdmi_dev, VP_STUFF, m_PR_STUFFING, v_PR_STUFFING(1));
	hdmi_msk_reg(hdmi_dev, VP_STUFF,
		     m_YCC422_STUFFING | m_PP_STUFFING,
		     v_YCC422_STUFFING(1) | v_PP_STUFFING(1));
#endif
	return 0;
}

static int rk32_hdmi_video_sampler(struct hdmi_dev *hdmi_dev,
				   struct hdmi_video *vpara)
{
	int map_code = 0;

	if (vpara->color_input == HDMI_COLOR_YCBCR422) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		switch (vpara->color_output_depth) {
		case 8:
			map_code = VIDEO_YCBCR422_8BIT;
			break;
		case 10:
			map_code = VIDEO_YCBCR422_10BIT;
			break;
		case 12:
			map_code = VIDEO_YCBCR422_12BIT;
			break;
		default:
			map_code = VIDEO_YCBCR422_8BIT;
			break;
		}
	} else if (vpara->color_input == HDMI_COLOR_YCBCR420 ||
		   vpara->color_input == HDMI_COLOR_YCBCR444) {
		switch (vpara->color_output_depth) {
		case 10:
			map_code = VIDEO_YCBCR444_10BIT;
			break;
		case 12:
			map_code = VIDEO_YCBCR444_12BIT;
			break;
		case 16:
			map_code = VIDEO_YCBCR444_16BIT;
			break;
		case 8:
		default:
			map_code = VIDEO_YCBCR444_8BIT;
			break;
		}
	} else {
		switch (vpara->color_output_depth) {
		case 10:
			map_code = VIDEO_RGB444_10BIT;
			break;
		case 12:
			map_code = VIDEO_RGB444_12BIT;
			break;
		case 16:
			map_code = VIDEO_RGB444_16BIT;
			break;
		case 8:
		default:
			map_code = VIDEO_RGB444_8BIT;
			break;
		}
		map_code += (vpara->color_input == HDMI_COLOR_YCBCR444) ?
			    8 : 0;
	}

	/* Set Data enable signal from external
	   and set video sample input mapping */
	hdmi_msk_reg(hdmi_dev, TX_INVID0,
		     m_INTERNAL_DE_GEN | m_VIDEO_MAPPING,
		     v_INTERNAL_DE_GEN(0) | v_VIDEO_MAPPING(map_code));

#if defined(HDMI_VIDEO_STUFFING)
	hdmi_writel(hdmi_dev, TX_GYDATA0, 0x00);
	hdmi_writel(hdmi_dev, TX_GYDATA1, 0x00);
	hdmi_msk_reg(hdmi_dev, TX_INSTUFFING,
		     m_GYDATA_STUFF, v_GYDATA_STUFF(1));
	hdmi_writel(hdmi_dev, TX_RCRDATA0, 0x00);
	hdmi_writel(hdmi_dev, TX_RCRDATA1, 0x00);
	hdmi_msk_reg(hdmi_dev, TX_INSTUFFING,
		     m_RCRDATA_STUFF, v_RCRDATA_STUFF(1));
	hdmi_writel(hdmi_dev, TX_BCBDATA0, 0x00);
	hdmi_writel(hdmi_dev, TX_BCBDATA1, 0x00);
	hdmi_msk_reg(hdmi_dev, TX_INSTUFFING,
		     m_BCBDATA_STUFF, v_BCBDATA_STUFF(1));
#endif
	return 0;
}

static void hdmi_dev_config_avi(struct hdmi_dev *hdmi_dev,
				struct hdmi_video *vpara)
{
	unsigned char colorimetry, ext_colorimetry = 0, aspect_ratio, y1y0;
	unsigned char rgb_quan_range = AVI_QUANTIZATION_RANGE_DEFAULT;

	/*Set AVI infoFrame Data byte1*/
	if (vpara->color_output == HDMI_COLOR_YCBCR444)
		y1y0 = AVI_COLOR_MODE_YCBCR444;
	else if (vpara->color_output == HDMI_COLOR_YCBCR422)
		y1y0 = AVI_COLOR_MODE_YCBCR422;
	else if (vpara->color_output == HDMI_COLOR_YCBCR420)
		y1y0 = AVI_COLOR_MODE_YCBCR420;
	else
		y1y0 = AVI_COLOR_MODE_RGB;

	hdmi_msk_reg(hdmi_dev, FC_AVICONF0,
		     m_FC_ACTIV_FORMAT | m_FC_RGC_YCC,
		     v_FC_RGC_YCC(y1y0) | v_FC_ACTIV_FORMAT(1));

	/*Set AVI infoFrame Data byte2*/
	switch (vpara->vic) {
	case HDMI_720X480I_60HZ_4_3:
	case HDMI_720X576I_50HZ_4_3:
	case HDMI_720X480P_60HZ_4_3:
	case HDMI_720X576P_50HZ_4_3:
		aspect_ratio = AVI_CODED_FRAME_ASPECT_4_3;
		colorimetry = AVI_COLORIMETRY_SMPTE_170M;
		break;
	case HDMI_720X480I_60HZ_16_9:
	case HDMI_720X576I_50HZ_16_9:
	case HDMI_720X480P_60HZ_16_9:
	case HDMI_720X576P_50HZ_16_9:
		aspect_ratio = AVI_CODED_FRAME_ASPECT_16_9;
		colorimetry = AVI_COLORIMETRY_SMPTE_170M;
		break;
	default:
		aspect_ratio = AVI_CODED_FRAME_ASPECT_16_9;
		colorimetry = AVI_COLORIMETRY_ITU709;
	}

	if (vpara->colorimetry > HDMI_COLORIMETRY_ITU709) {
		colorimetry = AVI_COLORIMETRY_EXTENDED;
		ext_colorimetry = vpara->colorimetry;
	} else if (vpara->color_output == HDMI_COLOR_RGB_16_235 ||
		   vpara->color_output == HDMI_COLOR_RGB_0_255) {
		colorimetry = AVI_COLORIMETRY_NO_DATA;
		ext_colorimetry = 0;
	} else if (vpara->colorimetry != HDMI_COLORIMETRY_NO_DATA) {
		colorimetry = vpara->colorimetry;
	}

	hdmi_writel(hdmi_dev, FC_AVICONF1,
		    v_FC_COLORIMETRY(colorimetry) |
		    v_FC_PIC_ASPEC_RATIO(aspect_ratio) |
		    v_FC_ACT_ASPEC_RATIO(ACTIVE_ASPECT_RATE_SAME_AS_CODED_FRAME));

	/*Set AVI infoFrame Data byte3*/
	hdmi_msk_reg(hdmi_dev, FC_AVICONF2,
		     m_FC_EXT_COLORIMETRY | m_FC_QUAN_RANGE,
		     v_FC_EXT_COLORIMETRY(ext_colorimetry) |
		     v_FC_QUAN_RANGE(rgb_quan_range));

	/*Set AVI infoFrame Data byte4*/
	hdmi_writel(hdmi_dev, FC_AVIVID, vpara->vic & 0xff);

	/*Set AVI infoFrame Data byte5*/
	hdmi_msk_reg(hdmi_dev, FC_AVICONF3,
		     m_FC_YQ | m_FC_CN,
		     v_FC_YQ(YQ_LIMITED_RANGE) | v_FC_CN(CN_GRAPHICS));
}

static int hdmi_dev_config_vsi(struct hdmi_dev *hdmi_dev,
			       unsigned char vic_3d, unsigned char format)
{
	int i = 0, id = 0x000c03;
	unsigned char data[3] = {0};

	HDMIDBG("[%s] vic %d format %d.\n", __func__, vic_3d, format);

	hdmi_msk_reg(hdmi_dev, FC_DATAUTO0, m_VSD_AUTO, v_VSD_AUTO(0));
	hdmi_writel(hdmi_dev, FC_VSDIEEEID2, id & 0xff);
	hdmi_writel(hdmi_dev, FC_VSDIEEEID1, (id >> 8) & 0xff);
	hdmi_writel(hdmi_dev, FC_VSDIEEEID0, (id >> 16) & 0xff);

	data[0] = format << 5;	/*PB4 --HDMI_Video_Format*/
	switch (format) {
	case HDMI_VIDEO_FORMAT_4Kx2K:
		data[1] = vic_3d;	/*PB5--HDMI_VIC*/
		data[2] = 0;
		break;
	case HDMI_VIDEO_FORMAT_3D:
		data[1] = vic_3d << 4;	/*PB5--3D_Structure field*/
		data[2] = 0;		/*PB6--3D_Ext_Data field*/
		break;
	default:
		data[1] = 0;
		data[2] = 0;
		break;
	}

	for (i = 0; i < 3; i++)
		hdmi_writel(hdmi_dev, FC_VSDPAYLOAD0 + i, data[i]);

	hdmi_writel(hdmi_dev, FC_VSDSIZE, 0x6);
/*	if (auto_send) { */
	hdmi_writel(hdmi_dev, FC_DATAUTO1, 1);
	hdmi_writel(hdmi_dev, FC_DATAUTO2, 0x11);
	hdmi_msk_reg(hdmi_dev, FC_DATAUTO0, m_VSD_AUTO, v_VSD_AUTO(1));
/*	}
	else {
		hdmi_msk_reg(hdmi_dev, FC_DATMAN, m_VSD_MAN, v_VSD_MAN(1));
	}
*/

	return 0;
}

static uint8 rk_get_efuse_flag(void)
{
       uint8 value = 0;

#ifdef CONFIG_RK_EFUSE
#if defined(CONFIG_RKCHIP_RK322X)
       FtEfuseRead((void *)(unsigned long)RKIO_EFUSE_256BITS_PHYS, &value, 29, 1);
       value = value & 2;
#endif
#endif /* CONFIG_RK_EFUSE */
       return value;
}

#define PHY_TIMEOUT	10000
#define RK322X_PLL_POWER_DOWN	((1 << 12) | (1 << 28))
#define RK322X_PLL_POWER_UP	(1 << 28)
#define RK322X_PLL_PDATA_DEN	(1 << 27)
#define RK322X_PLL_PDATA_EN	((1 << 11) | (1 << 27))

static int ext_phy1_config(struct hdmi_dev *hdmi_dev)
{
	int stat = 0, i = 0, temp = 0;
	const struct ext_pll_config_tab *phy_ext = NULL;

	if (hdmi_dev->soctype == HDMI_SOC_RK1108)
		grf_writel(RK1108_PLL_POWER_DOWN |
			   RK1108_PLL_PDATA_DEN,
			   GRF_SOC_CON4);
	else if (hdmi_dev->soctype == HDMI_SOC_RK322XH)
		grf_writel(RK322XH_PLL_POWER_DOWN |
			   RK322XH_PLL_PDATA_DEN,
			   GRF_SOC_CON3);
	if (hdmi_dev->tmdsclk_ratio_change &&
	    hdmi_dev->driver.edid.scdc_present == 1)
		rockchip_hdmiv2_scdc_set_tmds_rate(hdmi_dev);
#ifdef SRC_DCK
	if (hdmi_dev->dclk_phy)
		clk_set_rate(hdmi_dev->dclk_phy, hdmi_dev->pixelclk);
	rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_CONTROL, 0xc1);
#else
	rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_CONTROL, 0xc0);
	rockchip_hdmiv2_write_phy(hdmi_dev, 0xa0, 0x04);
#endif

	/* config the required PHY I2C register */
	phy_ext = get_phy_ext_tab(hdmi_dev);
	if (phy_ext) {
#ifndef SRC_DCK
		stat = (phy_ext->vco_div_5 & 1) << 1;
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PLL_CTRL, stat);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PLL_PRE_DIVIDER,
					  phy_ext->pll_nd);
		stat = ((phy_ext->pll_nf >> 8) & 0x0f) | 0xf0;
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PLL_SPM_CTRL, stat);
		stat = phy_ext->pll_nf & 0xff;
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PLL_FB_DIVIDER, stat);
		stat = (phy_ext->pclk_divider_a & EXT_PHY_PCLK_DIVIDERA_MASK) |
		       ((phy_ext->pclk_divider_b & 3) << 5);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PCLK_DIVIDER1, stat);
		stat = (phy_ext->pclk_divider_d & EXT_PHY_PCLK_DIVIDERD_MASK) |
		       ((phy_ext->pclk_divider_c & 3) << 5);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_PCLK_DIVIDER2, stat);
		stat = ((phy_ext->tmsd_divider_a & 3) << 4) |
		       ((phy_ext->tmsd_divider_b & 3) << 2) |
		       (phy_ext->tmsd_divider_c & 3);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY1_TMDSCLK_DIVIDER, stat);
#endif
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  0xac,
					  (phy_ext->ppll_nf & 0xff));
		if (phy_ext->ppll_no == 1) {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  0xaa,
						  2);
			stat = (phy_ext->ppll_nf >> 8) | phy_ext->ppll_nd;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  0xab,
						  stat);
		} else {
			stat = ((phy_ext->ppll_no / 2) - 1);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  0xad,
						  stat);
			stat = (phy_ext->ppll_nf >> 8) | phy_ext->ppll_nd;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  0xab,
						  stat);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  0xaa,
						  0x0e);
		}
	} else {
		printf("%s no supported phy configuration.\n", __func__);
		return -EPERM;
	}

	if (hdmi_dev->phy_table) {
		for (i = 0; i < hdmi_dev->phy_table_size; i++) {
			temp = hdmi_dev->phy_table[i].maxfreq;
			if (hdmi_dev->tmdsclk <= temp)
				break;
		}
	}

	if (i != hdmi_dev->phy_table_size) {
		if (hdmi_dev->phy_table[i].pre_emphasis) {
			if (hdmi_dev->phy_table[i].pre_emphasis > 0x3f) {
				stat = 0;
				if (hdmi_dev->phy_table[i].pre_emphasis > 0x5f)
					temp = 3;
				else
					temp = 1;
			} else {
				stat = 7;
				if (hdmi_dev->phy_table[i].pre_emphasis > 0x1f)
					temp = 3;
				else
					temp = 1;
			}
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_PREEMPHASIS_MODE,
						  stat);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_PREEMPHASIS_CTRL,
						  (temp << 4) |
						  (temp << 2) |
						  temp);
			stat = hdmi_dev->phy_table[i].pre_emphasis % 0x20;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_PREEMPHASIS_D2,
						  stat);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_PREEMPHASIS_D1,
						  stat);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_PREEMPHASIS_D0,
						  stat);
		} else {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_PREEMPHASIS_CTRL,
						  0);
		}
		if (hdmi_dev->phy_table[i].slopeboost) {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_DELAY_PATH_EN,
						  0x3f);
			stat = hdmi_dev->phy_table[i].slopeboost % 0x10;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_DELAY_TIME1,
						  stat | 0xc0);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_DELAY_TIME2,
						  (stat << 4) | stat);
		} else {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY1_DELAY_PATH_EN,
						  0x00);
		}
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TMDS_CLK_LEVEL,
					  hdmi_dev->phy_table[i].clk_level);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TMDS_D2_LEVEL,
					  hdmi_dev->phy_table[i].data2_level);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TMDS_D1_LEVEL,
					  hdmi_dev->phy_table[i].data1_level);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TMDS_D0_LEVEL,
					  hdmi_dev->phy_table[i].data0_level);
	}
	/* bit[7:6] of reg c8/c9/ca/c8 is ESD detect threshold:
	 * 00 - 340mV
	 * 01 - 280mV
	 * 10 - 260mV
	 * 11 - 240mV
	 * default is 240mV, now we set it to 340mV
	 */
	rockchip_hdmiv2_write_phy(hdmi_dev, 0xc8, 0);
	rockchip_hdmiv2_write_phy(hdmi_dev, 0xc9, 0);
	rockchip_hdmiv2_write_phy(hdmi_dev, 0xca, 0);
	rockchip_hdmiv2_write_phy(hdmi_dev, 0xcb, 0);
	if (hdmi_dev->tmdsclk > 340000000) {
		/* Set termination resistor to 100ohm */
		stat = 75000000 / 100000;
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TERM_CAL_CTRL1,
					  ((stat >> 8) & 0xff) | 0x80);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TERM_CAL_CTRL2,
					  stat & 0xff);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TERM_RESIS_AUTO,
					  3 << 1);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TERM_CAL_CTRL1,
					  ((stat >> 8) & 0xff));
	} else if (hdmi_dev->tmdsclk > 165000000) {
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TERM_CAL_CTRL1,
					  0x81);
		/* clk termination resistor is 50ohm
		 * data termination resistor is 150ohm
		 */
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xc8, 0x30);
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xc9, 0x10);
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xca, 0x10);
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xcb, 0x10);
	} else {
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY1_TERM_CAL_CTRL1,
					  0x81);
	}
	if (hdmi_dev->soctype == HDMI_SOC_RK1108)
		grf_writel(RK1108_PLL_POWER_UP, GRF_SOC_CON4);
	else if (hdmi_dev->soctype == HDMI_SOC_RK322XH)
		grf_writel(RK322XH_PLL_POWER_UP, GRF_SOC_CON3);
	if (hdmi_dev->tmdsclk_ratio_change)
		mdelay(100);
	else
		udelay(1000);
	hdmi_msk_reg(hdmi_dev, PHY_CONF0,
		     m_TXPWRON_SIG, v_TXPWRON_SIG(1));
	i = 0;
	while (i++ < PHY_TIMEOUT) {
		if ((i % 10) == 0) {
			stat = rockchip_hdmiv2_read_phy(hdmi_dev, 0xa9);
			temp = rockchip_hdmiv2_read_phy(hdmi_dev, 0xaf);
			if ((stat & 0x01) && (temp & 0x01))
				break;
			mdelay(1);
		}
	}
	if (!(stat & 0x01) || !(temp & 0x01)) {
		stat = hdmi_readl(hdmi_dev, MC_LOCKONCLOCK);
		printf("PHY PLL not locked: PCLK_ON=%d,TMDSCLK_ON=%d\n",
		       (stat & m_PCLK_ON) >> 6, (stat & m_TMDSCLK_ON) >> 5);
		return -EPERM;
	}
	if (hdmi_dev->soctype == HDMI_SOC_RK1108)
		grf_writel(RK1108_PLL_PDATA_EN, GRF_SOC_CON4);
	else if (hdmi_dev->soctype == HDMI_SOC_RK322XH)
		grf_writel(RK322XH_PLL_PDATA_EN, GRF_SOC_CON3);
	return 0;
}

static int ext_phy_config(struct hdmi_dev *hdmi_dev)
{
	int stat = 0, i = 0, temp;
	const struct ext_pll_config_tab *phy_ext = NULL;

	grf_writel(RK322X_PLL_POWER_DOWN |
		   RK322X_PLL_PDATA_DEN,
		   GRF_SOC_CON2);

	if (hdmi_dev->tmdsclk_ratio_change &&
	    hdmi_dev->driver.edid.scdc_present == 1)
		rockchip_hdmiv2_scdc_set_tmds_rate(hdmi_dev);

	/* config the required PHY I2C register */
	phy_ext = get_phy_ext_tab(hdmi_dev);
	if (phy_ext) {
		stat = ((phy_ext->pll_nf >> 1) & EXT_PHY_PLL_FB_BIT8_MASK) |
		       ((phy_ext->vco_div_5 & 1) << 5) |
		       (phy_ext->pll_nd & EXT_PHY_PLL_PRE_DIVIDER_MASK);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PLL_PRE_DIVIDER, stat);
		stat = phy_ext->pll_nf & 0xff;
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PLL_FB_DIVIDER, stat);
		stat = (phy_ext->pclk_divider_a & EXT_PHY_PCLK_DIVIDERA_MASK) |
		       ((phy_ext->pclk_divider_b & 3) << 5);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PCLK_DIVIDER1, stat);
		stat = (phy_ext->pclk_divider_d & EXT_PHY_PCLK_DIVIDERD_MASK) |
		       ((phy_ext->pclk_divider_c & 3) << 5);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PCLK_DIVIDER2, stat);
		stat = ((phy_ext->tmsd_divider_c & 3) << 4) |
		       ((phy_ext->tmsd_divider_a & 3) << 2) |
		       (phy_ext->tmsd_divider_b & 3);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_TMDSCLK_DIVIDER, stat);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PPLL_FB_DIVIDER,
					  phy_ext->ppll_nf);

		if (phy_ext->ppll_no == 1) {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_PPLL_POST_DIVIDER,
						  0);
			stat = 0x20 | phy_ext->ppll_nd;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_PPLL_PRE_DIVIDER,
						  stat);
               } else if (hdmi_dev->tmdsclk == 27000000 &&
                          rk_get_efuse_flag()) {
                       rockchip_hdmiv2_write_phy(hdmi_dev,
                                                 EXT_PHY_PPLL_POST_DIVIDER,
                                                 0x00);
                       rockchip_hdmiv2_write_phy(hdmi_dev,
                                                 EXT_PHY_PPLL_PRE_DIVIDER,
                                                 0xe1);
                       rockchip_hdmiv2_write_phy(hdmi_dev,
                                                 EXT_PHY_PPLL_FB_DIVIDER,
                                                 10);
		} else {
			stat = ((phy_ext->ppll_no / 2) - 1) << 4;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_PPLL_POST_DIVIDER,
						  stat);
			stat = 0xe0 | phy_ext->ppll_nd;
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_PPLL_PRE_DIVIDER,
						  stat);
		}
	} else {
		printf("%s no supported phy configuration.\n", __func__);
		return -EPERM;
	}

	if (hdmi_dev->phy_table) {
		for (i = 0; i < hdmi_dev->phy_table_size; i++) {
			temp = hdmi_dev->phy_table[i].maxfreq;
			if (hdmi_dev->tmdsclk <= temp)
				break;
		}
	}

	if (i != hdmi_dev->phy_table_size) {
		if (hdmi_dev->phy_table[i].slopeboost) {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_SIGNAL_CTRL, 0xff);
			temp = hdmi_dev->phy_table[i].slopeboost - 1;
			stat = ((temp & 3) << 6) | ((temp & 3) << 4) |
			       ((temp & 3) << 2) | (temp & 3);
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_SLOPEBOOST, stat);
		} else {
			rockchip_hdmiv2_write_phy(hdmi_dev,
						  EXT_PHY_SIGNAL_CTRL, 0x0f);
		}
		stat = ((hdmi_dev->phy_table[i].pre_emphasis & 3) << 4) |
		       ((hdmi_dev->phy_table[i].pre_emphasis & 3) << 2) |
		       (hdmi_dev->phy_table[i].pre_emphasis & 3);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_PREEMPHASIS, stat);
		stat = ((hdmi_dev->phy_table[i].clk_level & 0xf) << 4) |
		       (hdmi_dev->phy_table[i].data2_level & 0xf);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_LEVEL1, stat);
		stat = ((hdmi_dev->phy_table[i].data1_level & 0xf) << 4) |
		       (hdmi_dev->phy_table[i].data0_level & 0xf);
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_LEVEL2, stat);
	} else {
		rockchip_hdmiv2_write_phy(hdmi_dev,
					  EXT_PHY_SIGNAL_CTRL, 0x0f);
	}
	rockchip_hdmiv2_write_phy(hdmi_dev, 0xf3, 0x22);

	stat = 62500000 / 100000;
	rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_TERM_CAL,
				  ((stat >> 8) & 0xff) | 0x80);
	rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_TERM_CAL_DIV_L,
				  stat & 0xff);
	if (hdmi_dev->tmdsclk > 340000000) {
		stat = EXT_PHY_AUTO_R100_OHMS;
	} else if (hdmi_dev->tmdsclk > 200000000) {
		if (hdmi_dev->io_pullup > 0)
			stat = EXT_PHY_AUTO_R150_OHMS;
		else
			stat = EXT_PHY_AUTO_R50_OHMS;
	} else {
		stat = EXT_PHY_AUTO_ROPEN_CIRCUIT;
	}


	if (stat & EXT_PHY_TERM_CAL_EN_MASK) {
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xfc,
					  stat & 0x7f);
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xfd,
					  stat & 0x7f);
		rockchip_hdmiv2_write_phy(hdmi_dev, 0xfe,
					  stat & 0x7f);
	} else {
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_TERM_RESIS_AUTO,
					  stat | 0x20);
		rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_TERM_CAL,
					  (stat >> 8) & 0xff);
	}

	if (hdmi_dev->tmdsclk > 200000000)
		stat = 0;
	else
		stat = 0x11;
	rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_PLL_BW, stat);
	rockchip_hdmiv2_write_phy(hdmi_dev, EXT_PHY_PPLL_BW, 0x27);
	grf_writel(RK322X_PLL_POWER_UP, GRF_SOC_CON2);
	if (hdmi_dev->tmdsclk_ratio_change)
		mdelay(100);
	else
		udelay(1000);
	hdmi_msk_reg(hdmi_dev, PHY_CONF0,
		     m_TXPWRON_SIG, v_TXPWRON_SIG(1));
	i = 0;
	while (i++ < PHY_TIMEOUT) {
		if ((i % 10) == 0) {
			temp = EXT_PHY_PPLL_POST_DIVIDER;
			stat = rockchip_hdmiv2_read_phy(hdmi_dev, temp);
			if (stat & EXT_PHY_PPLL_LOCK_STATUS_MASK)
				break;
			mdelay(1);
		}
	}
	if ((stat & EXT_PHY_PPLL_LOCK_STATUS_MASK) == 0) {
		stat = hdmi_readl(hdmi_dev, MC_LOCKONCLOCK);
		printf("PHY PLL not locked: PCLK_ON=%d,TMDSCLK_ON=%d\n",
		       (stat & m_PCLK_ON) >> 6, (stat & m_TMDSCLK_ON) >> 5);
		return -EPERM;
	}

	grf_writel(RK322X_PLL_PDATA_EN, GRF_SOC_CON2);
	return 0;
}

static int rk32_hdmi_config_phy(struct hdmi_dev *hdmi_dev)
{
	int stat = 0, i = 0;
	const struct phy_mpll_config_tab *phy_mpll = NULL;

	if (hdmi_dev->soctype == HDMI_SOC_RK322X)
		return ext_phy_config(hdmi_dev);
	else if (hdmi_dev->soctype == HDMI_SOC_RK1108 ||
		 hdmi_dev->soctype == HDMI_SOC_RK322XH)
		return ext_phy1_config(hdmi_dev);

	hdmi_msk_reg(hdmi_dev, PHY_I2CM_DIV,
		     m_PHY_I2CM_FAST_STD, v_PHY_I2CM_FAST_STD(0));

	/* power off PHY */
	hdmi_msk_reg(hdmi_dev, PHY_CONF0,
		     m_PDDQ_SIG | m_TXPWRON_SIG,
		     v_PDDQ_SIG(1) | v_TXPWRON_SIG(0));

	if (hdmi_dev->tmdsclk_ratio_change &&
	    hdmi_dev->driver.edid.scdc_present == 1)
		rockchip_hdmiv2_scdc_set_tmds_rate(hdmi_dev);

	/* reset PHY */
	hdmi_writel(hdmi_dev, MC_PHYRSTZ, v_PHY_RSTZ(1));
	mdelay(5);
	hdmi_writel(hdmi_dev, MC_PHYRSTZ, v_PHY_RSTZ(0));

	/* Set slave address as PHY GEN2 address */
	hdmi_writel(hdmi_dev, PHY_I2CM_SLAVE, PHY_GEN2_ADDR);

	/* config the required PHY I2C register */
	phy_mpll = get_phy_mpll_tab(hdmi_dev->pixelclk,
				    hdmi_dev->tmdsclk,
				    hdmi_dev->pixelrepeat - 1,
				    hdmi_dev->colordepth);
	if (phy_mpll) {
		stat = v_PREP_DIV(phy_mpll->prep_div) |
		       v_TMDS_CNTRL(phy_mpll->tmdsmhl_cntrl) |
		       v_OPMODE(phy_mpll->opmode) |
		       v_FBDIV2_CNTRL(phy_mpll->div2_cntrl) |
		       v_FBDIV1_CNTRL(phy_mpll->div1_cntrl) |
		       v_REF_CNTRL(phy_mpll->ref_cntrl) |
		       v_MPLL_N_CNTRL(phy_mpll->n_cntrl);
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_OPMODE_PLLCFG,
					  stat);
		stat = v_MPLL_PROP_CNTRL(phy_mpll->prop_cntrl) |
		       v_MPLL_INT_CNTRL(phy_mpll->int_cntrl);
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_PLLCURRCTRL,
					  stat);
		stat = v_MPLL_GMP_CNTRL(phy_mpll->gmp_cntrl);
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_PLLGMPCTRL,
					  stat);
	}

	if (hdmi_dev->phy_table) {
		for (i = 0; i < hdmi_dev->phy_table_size; i++)
			if (hdmi_dev->tmdsclk <= hdmi_dev->phy_table[i].maxfreq)
				break;
	}
	if (i == hdmi_dev->phy_table_size) {
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_CLKSYMCTRL,
					  v_OVERRIDE(1) | v_SLOPEBOOST(0) |
					  v_TX_SYMON(1) | v_CLK_SYMON(1) |
					  v_PREEMPHASIS(0));
		if (hdmi_dev->tmdsclk > 340000000)
			rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_VLEVCTRL,
						  v_SUP_TXLVL(9) |
						  v_SUP_CLKLVL(17));
		else if (hdmi_dev->tmdsclk > 165000000)
			rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_VLEVCTRL,
						  v_SUP_TXLVL(14) |
						  v_SUP_CLKLVL(17));
		else
			rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_VLEVCTRL,
						  v_SUP_TXLVL(18) |
						  v_SUP_CLKLVL(17));
	} else {
		stat = v_OVERRIDE(1) | v_TX_SYMON(1) | v_CLK_SYMON(1) |
		       v_PREEMPHASIS(hdmi_dev->phy_table[i].pre_emphasis) |
		       v_SLOPEBOOST(hdmi_dev->phy_table[i].slopeboost);
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_CLKSYMCTRL, stat);

		stat = v_SUP_CLKLVL(hdmi_dev->phy_table[i].clk_level) |
		       v_SUP_TXLVL(hdmi_dev->phy_table[i].data0_level);
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_VLEVCTRL, stat);
	}
	if (hdmi_dev->tmdsclk > 340000000)
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_TERM_RESIS,
					  v_TX_TERM(R50_OHMS));
	else
		rockchip_hdmiv2_write_phy(hdmi_dev, PHYTX_TERM_RESIS,
					  v_TX_TERM(R100_OHMS));
	/* rockchip_hdmiv2_write_phy(hdmi_dev, 0x05, 0x8000); */

	if (hdmi_dev->tmdsclk_ratio_change)
		mdelay(100);
	/* power on PHY */
	hdmi_writel(hdmi_dev, PHY_CONF0, 0x2e);

	/* check if the PHY PLL is locked */
	i = 0;
	while (i++ < PHY_TIMEOUT) {
		if ((i % 100) == 0) {
			stat = hdmi_readl(hdmi_dev, PHY_STAT0);
			if (stat & m_PHY_LOCK)
				break;
		}
	}
	if ((stat & m_PHY_LOCK) == 0) {
		stat = hdmi_readl(hdmi_dev, MC_LOCKONCLOCK);
		printf("PHY PLL not locked: PCLK_ON=%d,TMDSCLK_ON=%d\n",
		       (stat & m_PCLK_ON) >> 6, (stat & m_TMDSCLK_ON) >> 5);
		return -EPERM;
	}

	return 0;
}


static const char coeff_csc[][24] = {
		/*   G		R	    B		Bias
		     A1    |	A2     |    A3     |	A4    |
		     B1    |    B2     |    B3     |    B4    |
		     C1    |    C2     |    C3     |    C4    | */
	{	/* CSC_RGB_0_255_TO_RGB_16_235_8BIT */
		0x36, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,		/*G*/
		0x00, 0x00, 0x36, 0xf7, 0x00, 0x00, 0x00, 0x40,		/*R*/
		0x00, 0x00, 0x00, 0x00, 0x36, 0xf7, 0x00, 0x40,		/*B*/
	},
	{	/* CSC_RGB_0_255_TO_RGB_16_235_10BIT */
		0x36, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,		/*G*/
		0x00, 0x00, 0x36, 0xf7, 0x00, 0x00, 0x01, 0x00,		/*R*/
		0x00, 0x00, 0x00, 0x00, 0x36, 0xf7, 0x01, 0x00,		/*B*/
	},
	{	/* CSC_RGB_0_255_TO_ITU601_16_235_8BIT */
		0x20, 0x40, 0x10, 0x80, 0x06, 0x40, 0x00, 0x40,		/*Y*/
		0xe8, 0x80, 0x1c, 0x00, 0xfb, 0x80, 0x02, 0x00,		/*Cr*/
		0xed, 0x80, 0xf6, 0x80, 0x1c, 0x00, 0x02, 0x00,		/*Cb*/
	},
	{	/* CSC_RGB_0_255_TO_ITU601_16_235_10BIT */
		0x20, 0x40, 0x10, 0x80, 0x06, 0x40, 0x01, 0x00,		/*Y*/
		0xe8, 0x80, 0x1c, 0x00, 0xfb, 0x80, 0x08, 0x00,		/*Cr*/
		0xed, 0x80, 0xf6, 0x80, 0x1c, 0x00, 0x08, 0x00,		/*Cb*/
	},
	{	/* CSC_RGB_0_255_TO_ITU709_16_235_8BIT */
		0x27, 0x40, 0x0b, 0xc0, 0x04, 0x00, 0x00, 0x40,		/*Y*/
		0xe6, 0x80, 0x1c, 0x00, 0xfd, 0x80, 0x02, 0x00,		/*Cr*/
		0xea, 0x40, 0xf9, 0x80, 0x1c, 0x00, 0x02, 0x00,		/*Cb*/
	},
	{	/* CSC_RGB_0_255_TO_ITU709_16_235_10BIT */
		0x27, 0x40, 0x0b, 0xc0, 0x04, 0x00, 0x01, 0x00,		/*Y*/
		0xe6, 0x80, 0x1c, 0x00, 0xfd, 0x80, 0x08, 0x00,		/*Cr*/
		0xea, 0x40, 0xf9, 0x80, 0x1c, 0x00, 0x08, 0x00,		/*Cb*/
	},
		/* Y		Cr	    Cb		Bias */
	{	/* CSC_ITU601_16_235_TO_RGB_0_255_8BIT */
		0x20, 0x00, 0x69, 0x26, 0x74, 0xfd, 0x01, 0x0e,		/*G*/
		0x20, 0x00, 0x2c, 0xdd, 0x00, 0x00, 0x7e, 0x9a,		/*R*/
		0x20, 0x00, 0x00, 0x00, 0x38, 0xb4, 0x7e, 0x3b,		/*B*/
	},
	{	/* CSC_ITU709_16_235_TO_RGB_0_255_8BIT */
		0x20, 0x00, 0x71, 0x06, 0x7a, 0x02, 0x00, 0xa7,		/*G*/
		0x20, 0x00, 0x32, 0x64, 0x00, 0x00, 0x7e, 0x6d,		/*R*/
		0x20, 0x00, 0x00, 0x00, 0x3b, 0x61, 0x7e, 0x25,		/*B*/
	},
};

static int rk32_hdmi_video_csc(struct hdmi_dev *hdmi_dev,
			       struct hdmi_video *vpara)
{
	int i, mode = 0, interpolation, decimation, csc_scale = 0;
	const char *coeff = NULL;
	unsigned char color_depth = 0;

	if (vpara->color_input == vpara->color_output) {
		hdmi_msk_reg(hdmi_dev, MC_FLOWCTRL,
			     m_FEED_THROUGH_OFF, v_FEED_THROUGH_OFF(0));
		return 0;
	}

	if (vpara->color_input == HDMI_COLOR_YCBCR422 &&
	    vpara->color_output != HDMI_COLOR_YCBCR422 &&
	    vpara->color_output != HDMI_COLOR_YCBCR420) {
		interpolation = 1;
		hdmi_msk_reg(hdmi_dev, CSC_CFG,
			     m_CSC_INTPMODE, v_CSC_INTPMODE(interpolation));
	}

	if ((vpara->color_input == HDMI_COLOR_RGB_0_255 ||
	     vpara->color_input == HDMI_COLOR_YCBCR444) &&
	     vpara->color_output == HDMI_COLOR_YCBCR422) {
		decimation = 1;
		hdmi_msk_reg(hdmi_dev, CSC_CFG,
			     m_CSC_DECIMODE, v_CSC_DECIMODE(decimation));
	}

	switch (vpara->vic) {
	case HDMI_720X480I_60HZ_4_3:
	case HDMI_720X576I_50HZ_4_3:
	case HDMI_720X480P_60HZ_4_3:
	case HDMI_720X576P_50HZ_4_3:
	case HDMI_720X480I_60HZ_16_9:
	case HDMI_720X576I_50HZ_16_9:
	case HDMI_720X480P_60HZ_16_9:
	case HDMI_720X576P_50HZ_16_9:
		if (vpara->color_input == HDMI_COLOR_RGB_0_255 &&
		    vpara->color_output >= HDMI_COLOR_YCBCR444) {
			mode = CSC_RGB_0_255_TO_ITU601_16_235_8BIT;
			csc_scale = 0;
		} else if (vpara->color_input >= HDMI_COLOR_YCBCR444 &&
			   vpara->color_output == HDMI_COLOR_RGB_0_255) {
			mode = CSC_ITU601_16_235_TO_RGB_0_255_8BIT;
			csc_scale = 1;
		}
		break;
	default:
		if (vpara->color_input == HDMI_COLOR_RGB_0_255 &&
		    vpara->color_output >= HDMI_COLOR_YCBCR444) {
			mode = CSC_RGB_0_255_TO_ITU709_16_235_8BIT;
			csc_scale = 0;
		} else if (vpara->color_input >= HDMI_COLOR_YCBCR444 &&
			   vpara->color_output == HDMI_COLOR_RGB_0_255) {
			mode = CSC_ITU709_16_235_TO_RGB_0_255_8BIT;
			csc_scale = 1;
		}
		break;
	}

	if ((vpara->color_input == HDMI_COLOR_RGB_0_255) &&
	    (vpara->color_output == HDMI_COLOR_RGB_16_235)) {
		mode = CSC_RGB_0_255_TO_RGB_16_235_8BIT;
		csc_scale = 0;
	}

	switch (vpara->color_output_depth) {
	case 10:
		color_depth = COLOR_DEPTH_30BIT;
		mode += 1;
		break;
	case 12:
		color_depth = COLOR_DEPTH_36BIT;
		mode += 2;
		break;
	case 16:
		color_depth = COLOR_DEPTH_48BIT;
		mode += 3;
		break;
	case 8:
	default:
		color_depth = COLOR_DEPTH_24BIT;
		break;
	}

	coeff = coeff_csc[mode];
	for (i = 0; i < 24; i++)
		hdmi_writel(hdmi_dev, CSC_COEF_A1_MSB + i, coeff[i]);

	hdmi_msk_reg(hdmi_dev, CSC_SCALE,
		     m_CSC_SCALE, v_CSC_SCALE(csc_scale));
	/*config CSC_COLOR_DEPTH*/
	hdmi_msk_reg(hdmi_dev, CSC_SCALE,
		     m_CSC_COLOR_DEPTH, v_CSC_COLOR_DEPTH(color_depth));

	/* enable CSC */
	hdmi_msk_reg(hdmi_dev, MC_FLOWCTRL,
		     m_FEED_THROUGH_OFF, v_FEED_THROUGH_OFF(1));

	return 0;
}

int hdmi_dev_config_video(struct hdmi_dev *hdmi_dev,
				 struct hdmi_video *vpara)
{
	int vic;

	printf("%s vic %d color_output %d color_output_depth %d\n",
	       __func__, vpara->vic, vpara->color_output,
	       vpara->color_output_depth);

	/* force output black */
	if (vpara->color_output == HDMI_COLOR_RGB_0_255) {
		hdmi_writel(hdmi_dev, FC_DBGTMDS2, 0x00);	/*R*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS1, 0x00);	/*G*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS0, 0x00);	/*B*/
	} else if (vpara->color_output == HDMI_COLOR_RGB_16_235) {
		hdmi_writel(hdmi_dev, FC_DBGTMDS2, 0x10);	/*R*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS1, 0x10);	/*G*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS0, 0x10);	/*B*/
	} else if (vpara->color_output == HDMI_COLOR_YCBCR420) {
		hdmi_writel(hdmi_dev, FC_DBGTMDS2, 0x00);	/*Cb*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS1, 0x10);	/*Y*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS0, 0x80);	/*Cr*/
	} else {
		hdmi_writel(hdmi_dev, FC_DBGTMDS2, 0x80);	/*Cb*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS1, 0x10);	/*Y*/
		hdmi_writel(hdmi_dev, FC_DBGTMDS0, 0x80);	/*Cr*/
	}

	hdmi_msk_reg(hdmi_dev, FC_DBGFORCE,
		     m_FC_FORCEVIDEO, v_FC_FORCEVIDEO(1));

	if (rk32_hdmi_video_framecomposer(hdmi_dev, vpara) < 0)
		return -EPERM;
	if (rk32_hdmi_video_packetizer(hdmi_dev, vpara) < 0)
		return -EPERM;
	/*Color space convert*/
	if (rk32_hdmi_video_csc(hdmi_dev, vpara) < 0)
		return -EPERM;
	if (rk32_hdmi_video_sampler(hdmi_dev, vpara) < 0)
		return -EPERM;

	if (vpara->sink_hdmi == OUTPUT_HDMI) {
		hdmi_dev_config_avi(hdmi_dev, vpara);
		if (vpara->format_3d != HDMI_3D_NONE)
			hdmi_dev_config_vsi(hdmi_dev,
					    vpara->format_3d,
					    HDMI_VIDEO_FORMAT_3D);
		#ifndef HDMI_VERSION_2
		else if ((vpara->vic > 92 && vpara->vic < 96) ||
			 (vpara->vic == 98)) {
			vic = (vpara->vic == 98) ? 4 : (96 - vpara->vic);
			hdmi_dev_config_vsi(hdmi_dev, vic,
					    HDMI_VIDEO_FORMAT_4Kx2K);
		}
		#endif
		else
			hdmi_dev_config_vsi(hdmi_dev, vpara->vic,
					    HDMI_VIDEO_FORMAT_NORMAL);
		printf("[HDMI] sucess output HDMI.\n");
	} else {
		printf("[HDMI] sucess output DVI.\n");
	}

	rk32_hdmi_config_phy(hdmi_dev);
	return 0;
}

static void hdcp_load_key(struct hdmi_dev *hdmi_dev)
{
	struct hdcp_keys *key = hdmi_dev->keys;
	int i, value;

	/* Disable decryption logic */
	hdmi_writel(hdmi_dev, HDCPREG_RMCTL, 0);
	/* Poll untile DPK write is allowed */
	do {
		value = hdmi_readl(hdmi_dev, HDCPREG_RMSTS);
	} while ((value & m_DPK_WR_OK_STS) == 0);

	/* write unencryped AKSV */
	hdmi_writel(hdmi_dev, HDCPREG_DPK6, 0);
	hdmi_writel(hdmi_dev, HDCPREG_DPK5, 0);
	hdmi_writel(hdmi_dev, HDCPREG_DPK4, key->KSV[4]);
	hdmi_writel(hdmi_dev, HDCPREG_DPK3, key->KSV[3]);
	hdmi_writel(hdmi_dev, HDCPREG_DPK2, key->KSV[2]);
	hdmi_writel(hdmi_dev, HDCPREG_DPK1, key->KSV[1]);
	hdmi_writel(hdmi_dev, HDCPREG_DPK0, key->KSV[0]);
	/* Poll untile DPK write is allowed */
	do {
		value = hdmi_readl(hdmi_dev, HDCPREG_RMSTS);
	} while ((value & m_DPK_WR_OK_STS) == 0);

	hdmi_writel(hdmi_dev, HDCPREG_RMCTL, 1);
	hdmi_writel(hdmi_dev, HDCPREG_SEED1, key->seeds[0]);
	hdmi_writel(hdmi_dev, HDCPREG_SEED0, key->seeds[1]);

	/* write private key */
	for (i = 0; i < HDCP_PRIVATE_KEY_SIZE; i += 7) {
		hdmi_writel(hdmi_dev, HDCPREG_DPK6, key->devicekey[i + 6]);
		hdmi_writel(hdmi_dev, HDCPREG_DPK5, key->devicekey[i + 5]);
		hdmi_writel(hdmi_dev, HDCPREG_DPK4, key->devicekey[i + 4]);
		hdmi_writel(hdmi_dev, HDCPREG_DPK3, key->devicekey[i + 3]);
		hdmi_writel(hdmi_dev, HDCPREG_DPK2, key->devicekey[i + 2]);
		hdmi_writel(hdmi_dev, HDCPREG_DPK1, key->devicekey[i + 1]);
		hdmi_writel(hdmi_dev, HDCPREG_DPK0, key->devicekey[i]);

		do {
			value = hdmi_readl(hdmi_dev, HDCPREG_RMSTS);
		} while ((value & m_DPK_WR_OK_STS) == 0);
	}
}

void hdmi_dev_hdcp_start(struct hdmi_dev *hdmi_dev)
{
	if (!hdmi_dev->hdcp_enable || hdmi_dev->keys == NULL)
		return;
	#ifdef CONFIG_RKCHIP_RK3368
	hdmi_msk_reg(hdmi_dev, HDCP2REG_CTRL,
		     m_HDCP2_OVR_EN | m_HDCP2_FORCE,
		     v_HDCP2_OVR_EN(1) | v_HDCP2_FORCE(0));
	hdmi_writel(hdmi_dev, HDCP2REG_MASK, 0xff);
	hdmi_writel(hdmi_dev, HDCP2REG_MUTE, 0xff);
	#endif

	hdmi_msk_reg(hdmi_dev, FC_INVIDCONF,
		     m_FC_HDCP_KEEPOUT, v_FC_HDCP_KEEPOUT(1));
	hdmi_msk_reg(hdmi_dev, A_HDCPCFG0,
		     m_HDMI_DVI, v_HDMI_DVI(hdmi_dev->video.sink_hdmi));
	hdmi_writel(hdmi_dev, A_OESSWCFG, 0x40);
	hdmi_msk_reg(hdmi_dev, A_HDCPCFG0,
		     m_ENCRYPT_BYPASS | m_FEATURE11_EN | m_SYNC_RI_CHECK,
		     v_ENCRYPT_BYPASS(0) | v_FEATURE11_EN(0) |
		     v_SYNC_RI_CHECK(1));
	hdmi_msk_reg(hdmi_dev, A_HDCPCFG1,
		     m_ENCRYPT_DISBALE | m_PH2UPSHFTENC,
		     v_ENCRYPT_DISBALE(0) | v_PH2UPSHFTENC(1));
	/* Reset HDCP Engine */
	hdmi_msk_reg(hdmi_dev, A_HDCPCFG1,
		     m_HDCP_SW_RST, v_HDCP_SW_RST(0));
	hdcp_load_key(hdmi_dev);
	/* hdmi_writel(hdmi_dev, A_APIINTMSK, 0x00); */
	hdmi_msk_reg(hdmi_dev, A_HDCPCFG0, m_RX_DETECT, v_RX_DETECT(1));

	hdmi_msk_reg(hdmi_dev, MC_CLKDIS,
		     m_HDCPCLK_DISABLE, v_HDCPCLK_DISABLE(0));

	printf("%s success\n", __func__);
}

int hdmi_dev_control_output(struct hdmi_dev *hdmi_dev, int enable)
{
	if (enable == HDMI_AV_UNMUTE) {
		hdmi_writel(hdmi_dev, FC_DBGFORCE, 0x00);
		hdmi_msk_reg(hdmi_dev, FC_GCP,
			     m_FC_SET_AVMUTE | m_FC_CLR_AVMUTE,
			     v_FC_SET_AVMUTE(0) | v_FC_CLR_AVMUTE(1));
	} else {
		if (enable & HDMI_VIDEO_MUTE)
			hdmi_msk_reg(hdmi_dev, FC_DBGFORCE,
				     m_FC_FORCEVIDEO, v_FC_FORCEVIDEO(1));
	}

	return 0;
}

int hdmi_dev_insert(struct hdmi_dev *hdmi_dev)
{
	HDMIDBG("%s\n", __func__);

	hdmi_writel(hdmi_dev, MC_CLKDIS, m_HDCPCLK_DISABLE);
	if (hdmi_dev->soctype == HDMI_SOC_RK322XH)
		grf_writel(RK322XH_IO_5V_DOMAIN, GRF_SOC_CON4);
	return HDMI_ERROR_SUCESS;
}

