/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "rk32_lcdc.h"

#define lvds_regs			RKIO_LVDS_PHYS

#define LVDS_CH0_REG_0			0x00
#define LVDS_CH0_REG_1			0x04
#define LVDS_CH0_REG_2			0x08
#define LVDS_CH0_REG_3			0x0c
#define LVDS_CH0_REG_4			0x10
#define LVDS_CH0_REG_5			0x14
#define LVDS_CH0_REG_9			0x24
#define LVDS_CFG_REG_c			0x30
#define LVDS_CH0_REG_d			0x34
#define LVDS_CH0_REG_f			0x3c
#define LVDS_CH0_REG_20			0x80
#define LVDS_CFG_REG_21			0x84

#define LVDS_SEL_VOP_LIT		(1 << 3)

#define LVDS_FMT_MASK			(0x07 << 16)
#define LVDS_MSB			(0x01 << 3)
#define LVDS_DUAL			(0x01 << 4)
#define LVDS_FMT_1			(0x01 << 5)
#define LVDS_TTL_EN			(0x01 << 6)
#define LVDS_START_PHASE_RST_1		(0x01 << 7)
#define LVDS_DCLK_INV			(0x01 << 8)
#define LVDS_CH0_EN			(0x01 << 11)
#define LVDS_CH1_EN			(0x01 << 12)
#define LVDS_PWRDN			(0x01 << 15)
extern int rk32_edp_enable(vidinfo_t * vid);
extern int rk32_mipi_enable(vidinfo_t * vid);
extern int rk32_dsi_enable(void);
extern int rk32_dsi_sync(void);
extern int rk32_dsi_disable(void);


struct lcdc_device rk32_lcdc;


static int inline lvds_writel(uint32 offset, uint32 val)
{
	writel(val, lvds_regs + offset);
	writel(val, lvds_regs + offset + 0x100);

	return 0;
}

static int rk32_lvds_disable(void)
{
	grf_writel(0x80008000, GRF_SOC_CON7);

	writel(0x00, lvds_regs + LVDS_CFG_REG_21); /*disable tx*/
	writel(0xff, lvds_regs + LVDS_CFG_REG_c); /*disable pll*/
	return 0;
}

static int rk32_lvds_en(vidinfo_t *vid)
{
	u32 h_bp = vid->vl_hspw + vid->vl_hbpd;
	u32 val ;

	if (vid->lcdc_id == 1) /*lcdc1 = vop little,lcdc0 = vop big*/
		val = LVDS_SEL_VOP_LIT | (LVDS_SEL_VOP_LIT << 16);
	else
		val = LVDS_SEL_VOP_LIT << 16; 
	grf_writel(val, GRF_SOC_CON6);

	val = vid->lvds_format;
	if (vid->screen_type == SCREEN_DUAL_LVDS)
		val |= LVDS_DUAL | LVDS_CH0_EN | LVDS_CH1_EN;
	else if(vid->screen_type == SCREEN_LVDS)
		val |= LVDS_CH0_EN;
	else if (vid->screen_type == SCREEN_RGB)
		val |= LVDS_TTL_EN | LVDS_CH0_EN | LVDS_CH1_EN;

	if (h_bp & 0x01)
		val |= LVDS_START_PHASE_RST_1;

	val |= (vid->vl_clkp << 8) | (vid->vl_hsp << 9) |
		(vid->vl_oep << 10);
	val |= 0xffff << 16;

	grf_writel(val, GRF_SOC_CON7);
	grf_writel(0x0f000f00, GRF_GPIO1H_SR);
	grf_writel(0x00ff00ff, GRF_GPIO1D_E);
	
	if(vid->screen_type == SCREEN_RGB) {
		grf_writel(0x00550055, GRF_GPIO1D_IOMUX);
	    	lvds_writel( LVDS_CH0_REG_0, 0x7f);
	    	lvds_writel( LVDS_CH0_REG_1, 0x40);
	    	lvds_writel( LVDS_CH0_REG_2, 0x00);
	    	lvds_writel( LVDS_CH0_REG_4, 0x3f);
	    	lvds_writel( LVDS_CH0_REG_5, 0x3f);
	    	lvds_writel( LVDS_CH0_REG_3, 0x46);
	    	lvds_writel( LVDS_CH0_REG_d, 0x0a);
	    	lvds_writel( LVDS_CH0_REG_20,0x44);/* 44:LSB  45:MSB*/
	    	writel(0x00, lvds_regs + LVDS_CFG_REG_c); /*eanble pll*/
	    	writel(0x92, lvds_regs + LVDS_CFG_REG_21); /*enable tx*/
	    	lvds_writel( 0x100, 0x7f);
	    	lvds_writel( 0x104, 0x40);
	    	lvds_writel( 0x108, 0x00);
	    	lvds_writel( 0x10c, 0x46);
	    	lvds_writel( 0x110, 0x3f);
	    	lvds_writel( 0x114, 0x3f);
	    	lvds_writel( 0x134, 0x0a);
	} else {
	    	lvds_writel( LVDS_CH0_REG_0, 0xbf);
	    	lvds_writel( LVDS_CH0_REG_1, 0x3f);
	    	lvds_writel( LVDS_CH0_REG_2, 0xfe);
	    	lvds_writel( LVDS_CH0_REG_3, 0x46);
	    	lvds_writel( LVDS_CH0_REG_4, 0x00);
	    	lvds_writel( LVDS_CH0_REG_d, 0x0a);//0a
	    	lvds_writel( LVDS_CH0_REG_20,0x44);/* 44:LSB  45:MSB*/
	    	writel(0x00, lvds_regs + LVDS_CFG_REG_c); /*eanble pll*/
	    	writel(0x92, lvds_regs + LVDS_CFG_REG_21); /*enable tx*/
	}

	return 0;
}

static int win0_set_par(struct lcdc_device *lcdc_dev,
			     struct fb_dsp_info *fb_info,
			     vidinfo_t *vid)
{
	u32 msk,val;
	lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_YRGB,
		     v_WIN0_HS_FACTOR_YRGB(0x1000) |
		     v_WIN0_VS_FACTOR_YRGB(0x1000));
	lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_CBR,
		     v_WIN0_HS_FACTOR_CBR(0x1000) |
		     v_WIN0_VS_FACTOR_CBR(0x1000));
	msk = m_WIN0_RB_SWAP | m_WIN0_ALPHA_SWAP | m_WIN0_LB_MODE |
		m_WIN0_DATA_FMT | m_WIN0_EN;
	val = v_WIN0_RB_SWAP(0) | v_WIN0_ALPHA_SWAP(0) | v_WIN0_LB_MODE(5) |
		v_WIN0_DATA_FMT(fb_info->format) | v_WIN0_EN(1);
	lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, msk, val);
	msk = m_WIN0_CBR_VSU_MODE | m_WIN0_YRGB_VSU_MODE;
	val = v_WIN0_CBR_VSU_MODE(1) | v_WIN0_YRGB_VSU_MODE(1);
	lcdc_msk_reg(lcdc_dev, WIN0_CTRL1, msk, val);
	val =  v_WIN0_ACT_WIDTH(fb_info->xact) |
		v_WIN0_ACT_HEIGHT(fb_info->yact);
	lcdc_writel(lcdc_dev, WIN0_ACT_INFO, val);
	val = v_WIN0_DSP_XST(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) |
		v_WIN0_DSP_YST(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
	lcdc_writel(lcdc_dev, WIN0_DSP_ST, val);
	val = v_WIN0_DSP_WIDTH(fb_info->xsize) | v_WIN0_DSP_HEIGHT(fb_info->ysize);
	lcdc_writel(lcdc_dev, WIN0_DSP_INFO, val);
	msk =  m_WIN0_COLOR_KEY_EN | m_WIN0_COLOR_KEY;
	val = v_WIN0_COLOR_KEY_EN(0) | v_WIN0_COLOR_KEY(0);
	lcdc_msk_reg(lcdc_dev, WIN0_COLOR_KEY, msk, val);

	if(fb_info->xsize > 2560) {
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, m_WIN0_LB_MODE,
			     v_WIN0_LB_MODE(LB_RGB_3840X2));
	} else if(fb_info->xsize > 1920) {
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, m_WIN0_LB_MODE,
			      v_WIN0_LB_MODE(LB_RGB_2560X4));
	} else if(fb_info->xsize > 1280){
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, m_WIN0_LB_MODE,
			     v_WIN0_LB_MODE(LB_RGB_1920X5));
	} else {
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, m_WIN0_LB_MODE,
			     v_WIN0_LB_MODE(LB_RGB_1280X8));
	}
	switch (fb_info->format) {
	case ARGB888:
		lcdc_writel(lcdc_dev, WIN0_VIR, v_ARGB888_VIRWIDTH(fb_info->xvir));
		break;
	case RGB888:
		lcdc_writel(lcdc_dev, WIN0_VIR, v_RGB888_VIRWIDTH(fb_info->xvir));
		break;
	case RGB565:
		lcdc_writel(lcdc_dev, WIN0_VIR, v_RGB565_VIRWIDTH(fb_info->xvir));
		break;
	case YUV422:
	case YUV420:
		lcdc_writel(lcdc_dev, WIN0_VIR, v_YUV_VIRWIDTH(fb_info->xvir));
		if(fb_info->xsize > 1280) {
			lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, m_WIN0_LB_MODE,
				     v_WIN0_LB_MODE(LB_YUV_3840X5));
		} else {
			lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, m_WIN0_LB_MODE,
				     v_WIN0_LB_MODE(LB_YUV_2560X8));
		}
		break;
	default:
		lcdc_writel(lcdc_dev, WIN0_VIR, v_RGB888_VIRWIDTH(fb_info->xvir));
		break;
	}
	lcdc_writel(lcdc_dev, WIN0_YRGB_MST, fb_info->yaddr);

	return 0;
}

void rk_lcdc_set_par(struct fb_dsp_info *fb_info, vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk32_lcdc;

	fb_info->layer_id = lcdc_dev->dft_win;
	switch (fb_info->layer_id) {
	case WIN0:
		win0_set_par(lcdc_dev, fb_info, vid);
		break;
	case WIN1:
		printf("%s --->WIN1 not support\n", __func__);
		break;
	default:
		printf("%s --->unknow lay_id \n", __func__);
		break;
	}
	lcdc_writel(lcdc_dev, BCSH_BCS, 0xd0010000);
	lcdc_writel(lcdc_dev, BCSH_H, 0x01000000);
	lcdc_writel(lcdc_dev, BCSH_COLOR_BAR, 1);
	lcdc_cfg_done(lcdc_dev);
}


int rk_lcdc_load_screen(vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk32_lcdc;
	int face = 0;
	u32 msk, val;
	
	if (vid->screen_type == SCREEN_MIPI ||
	    vid->screen_type == SCREEN_DUAL_MIPI) {
		rk32_mipi_enable(vid);
		if (vid->screen_type == SCREEN_MIPI) {
			msk = m_MIPI_OUT_EN | m_EDP_OUT_EN |
				m_HDMI_OUT_EN | m_RGB_OUT_EN;
			val = v_MIPI_OUT_EN(1);
		} else {
			msk = m_MIPI_OUT_EN | m_EDP_OUT_EN |
				m_HDMI_OUT_EN | m_RGB_OUT_EN |
				m_DOUB_CHANNEL_EN;
			val = v_MIPI_OUT_EN(1) | v_DOUB_CHANNEL_EN(1);
		}
	} else if (vid->screen_type == SCREEN_EDP) {
		msk = m_MIPI_OUT_EN | m_EDP_OUT_EN |
			m_HDMI_OUT_EN | m_RGB_OUT_EN;
		val = v_EDP_OUT_EN(1);
	} else if (vid->screen_type == SCREEN_HDMI) {
		msk = m_MIPI_OUT_EN | m_EDP_OUT_EN |
			m_HDMI_OUT_EN | m_RGB_OUT_EN;
		val = v_HDMI_OUT_EN(1);
	} else if (vid->screen_type == SCREEN_RGB ||
		   vid->screen_type == SCREEN_LVDS ||
		   vid->screen_type == SCREEN_DUAL_LVDS) {
		msk = m_MIPI_OUT_EN | m_EDP_OUT_EN |
			m_HDMI_OUT_EN | m_RGB_OUT_EN;
		val = v_RGB_OUT_EN(1);
	} else {
		msk = m_MIPI_OUT_EN | m_EDP_OUT_EN |
			m_HDMI_OUT_EN | m_RGB_OUT_EN;
		val = v_HDMI_OUT_EN(1);
	}
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, msk, val);

	msk = m_DSP_BLACK_EN | m_DSP_BLANK_EN | m_DSP_OUT_ZERO |
		m_DSP_DCLK_POL | m_DSP_DEN_POL | m_DSP_VSYNC_POL |
		m_DSP_HSYNC_POL;
	val = v_DSP_BLACK_EN(0) | v_DSP_BLANK_EN(0) | v_DSP_OUT_ZERO(0) |
		v_DSP_DCLK_POL(vid->vl_clkp) | v_DSP_DEN_POL(vid->vl_oep) |
		v_DSP_VSYNC_POL(vid->vl_vsp) | v_DSP_HSYNC_POL(vid->vl_hsp);
	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);
	switch (vid->lcd_face) {
	case OUT_P565:
		face = OUT_P565;
		msk = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(0);
		break;
	case OUT_P666:
		face = OUT_P666;
		msk = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
			m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(1) |
			v_DITHER_DOWN_SEL(1);
		break;
	case OUT_D888_P565:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(0);
		break;
	case OUT_D888_P666:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
			m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(1) |
			v_DITHER_DOWN_SEL(1);
		break;
	case OUT_P888:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
			m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_DOWN_MODE(0) |
			v_DITHER_UP_EN(1); /*we display rgb565 ,so dither up*/
		break;
	default:
		face = vid->lcd_face;
		msk = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
			m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_DOWN_MODE(0) |
			v_DITHER_UP_EN(1);
		break;
	}
	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, msk, val);
	if (vid->screen_type == SCREEN_EDP || vid->screen_type == SCREEN_HDMI)
		face = OUT_RGB_AAA;
	msk = m_DSP_RG_SWAP | m_DSP_RB_SWAP | m_DSP_DELTA_SWAP |
		m_DSP_FIELD_POL | m_DSP_DUMMY_SWAP | m_DSP_BG_SWAP |
		m_DSP_OUT_MODE;
	val = v_DSP_RG_SWAP(0) | v_DSP_RB_SWAP(vid->vl_swap_rb) |
		v_DSP_DELTA_SWAP(0) | v_DSP_DUMMY_SWAP(0) |
		v_DSP_FIELD_POL(0) | v_DSP_BG_SWAP(0) |
		v_DSP_OUT_MODE(face);
	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);
	lcdc_writel(lcdc_dev, DSP_BG, 0);
	val = v_DSP_HS_PW(vid->vl_hspw) | v_DSP_HTOTAL(vid->vl_hspw +
		vid->vl_hbpd + vid->vl_col + vid->vl_hfpd);
	lcdc_writel(lcdc_dev, DSP_HTOTAL_HS_END, val);
	val = v_DSP_HACT_END(vid->vl_hspw + vid->vl_hbpd + vid->vl_col) |
		v_DSP_HACT_ST(vid->vl_hspw + vid->vl_hbpd);
	lcdc_writel(lcdc_dev, DSP_HACT_ST_END, val);
	val = v_DSP_VTOTAL(vid->vl_vspw + vid->vl_vbpd +
		vid->vl_row + vid->vl_vfpd) | v_DSP_VS_PW(vid->vl_vspw);
	lcdc_writel(lcdc_dev, DSP_VTOTAL_VS_END, val);
	val = v_DSP_VACT_END(vid->vl_vspw + vid->vl_vbpd + vid->vl_row)|
		v_DSP_VACT_ST(vid->vl_vspw + vid->vl_vbpd);
	lcdc_writel(lcdc_dev, DSP_VACT_ST_END, val);
	val = v_DSP_HACT_END_POST(vid->vl_hspw + vid->vl_hbpd + vid->vl_col) |
		v_DSP_HACT_ST_POST(vid->vl_hspw + vid->vl_hbpd);
	lcdc_writel(lcdc_dev, POST_DSP_HACT_INFO, val);
	val = v_DSP_HACT_END_POST(vid->vl_vspw + vid->vl_vbpd + vid->vl_row)|
	      v_DSP_HACT_ST_POST(vid->vl_vspw + vid->vl_vbpd);
	lcdc_writel(lcdc_dev, POST_DSP_VACT_INFO, val);
	lcdc_writel(lcdc_dev, POST_DSP_VACT_INFO_F1, 0);
	lcdc_writel(lcdc_dev, POST_RESERVED, 0x10001000);
	lcdc_writel(lcdc_dev, MCU_CTRL, 0);
	lcdc_cfg_done(lcdc_dev);
	if ((vid->screen_type == SCREEN_LVDS) ||
	    (vid->screen_type == SCREEN_DUAL_LVDS) ||
	    (vid->screen_type == SCREEN_RGB)) {
		rk32_lvds_en(vid);
	} else if (vid->screen_type == SCREEN_EDP) {
		rk32_edp_enable(vid);
	} else if ((vid->screen_type == SCREEN_MIPI) ||
		   (vid->screen_type == SCREEN_DUAL_MIPI)) {
		rk32_dsi_sync();
	}

	return 0;
}


/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable, int mode)
{
	struct lcdc_device *lcdc_dev = &rk32_lcdc;
#if defined(CONFIG_RK32_DSI)
	if (((panel_info.screen_type == SCREEN_MIPI) ||
			   (panel_info.screen_type == SCREEN_DUAL_MIPI)) && (mode == 1)) {
		if (enable == 0) {
			rk32_dsi_enable();
			rk32_dsi_sync();
		} else if (enable == 1) {
			rk32_dsi_disable();
		}
	}
#endif
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_STANDBY_EN,
		     v_STANDBY_EN(enable ? 1 : 0));
	lcdc_cfg_done(lcdc_dev);
}

#if defined(CONFIG_OF_LIBFDT)
static int rk32_lcdc_parse_dt(struct lcdc_device *lcdc_dev,
				     const void *blob)
{
	int order = FB0_WIN0_FB1_WIN1_FB2_WIN2;

	if (lcdc_dev->id == 0)
		lcdc_dev->node  = fdt_path_offset(blob, "lcdc0");
	else
		lcdc_dev->node  = fdt_path_offset(blob, "lcdc1");
	if (lcdc_dev->node < 0) {
		debug("rk32 lcdc node is not found\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, lcdc_dev->node)) {
		debug("device lcdc is disabled\n");
		return -EPERM;
	}

	lcdc_dev->regs = fdtdec_get_addr(blob, lcdc_dev->node, "reg");
	order = fdtdec_get_int(blob, lcdc_dev->node,
			       "rockchip,fb-win-map", order);
	lcdc_dev->dft_win = order % 10;

	return 0;
}
#endif


int rk_lcdc_init(int lcdc_id)
{
	struct lcdc_device *lcdc_dev = &rk32_lcdc;
	u32 msk, val;

	lcdc_dev->soc_type = gd->arch.chiptype;
	lcdc_dev->id = lcdc_id;
#ifdef CONFIG_OF_LIBFDT
	if (!lcdc_dev->node)
		rk32_lcdc_parse_dt(lcdc_dev, gd->fdt_blob);
#endif
	if (lcdc_dev->node <= 0) {
		if (lcdc_dev->id == 0)
			lcdc_dev->regs = RKIO_VOP_BIG_PHYS;
		else
			lcdc_dev->regs = RKIO_VOP_LIT_PHYS;
	}

	grf_writel(1<<16, GRF_IO_VSEL); /*LCDCIOdomain 3.3 Vvoltageselectio*/

	msk = m_AUTO_GATING_EN | m_STANDBY_EN |
		m_DMA_STOP | m_MMU_EN;
	val =  v_AUTO_GATING_EN(1) | v_STANDBY_EN(0) |
		v_DMA_STOP(0) | v_MMU_EN(0);
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, msk, val);
	msk = m_DSP_LAYER3_SEL | m_DSP_LAYER2_SEL|
		m_DSP_LAYER1_SEL | m_DSP_LAYER0_SEL;
	val = v_DSP_LAYER3_SEL(3) | v_DSP_LAYER2_SEL(2) |
		v_DSP_LAYER1_SEL(1) | v_DSP_LAYER0_SEL(0);
	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, msk, val);
	lcdc_cfg_done(lcdc_dev);

	return 0;
}

