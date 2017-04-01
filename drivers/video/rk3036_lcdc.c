/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "rk3036_lcdc.h"

#if defined(CONFIG_RK32_DSI)
extern int rk32_mipi_enable(vidinfo_t * vid);
extern int rk32_dsi_disable(void);
extern int rk32_dsi_enable(void);
#endif
static struct lcdc_device rk312x_lcdc;

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)

struct rk_lvds_device {
	int    base;
	int    ctrl_reg;
	bool   sys_state;
};
struct rk_lvds_device rk31xx_lvds;

static inline int lvds_writel(struct rk_lvds_device *lvds, u32 offset, u32 val)
{
	writel(val, lvds->base + offset);
	return 0;
}

static inline int lvds_msk_reg(struct rk_lvds_device *lvds, u32 offset,
			       u32 msk, u32 val)
{
	u32 temp;

	temp = readl(lvds->base + offset) & (0xFF - (msk));
	writel(temp | ((val) & (msk)), lvds->base + offset);
	return 0;
}

static inline u32 lvds_readl(struct rk_lvds_device *lvds, u32 offset)
{
	return readl(lvds->base + offset);
}


static inline u32 lvds_phy_lock(struct rk_lvds_device *lvds)
{
	u32 val = 0;
	val = readl(lvds->ctrl_reg);
	return (val & 0x01);
}


static int rk31xx_lvds_pwr_on(vidinfo_t * vid)
{
        struct rk_lvds_device *lvds = &rk31xx_lvds;
	u32 delay_times = 20;

        if (vid->screen_type == SCREEN_LVDS) {
                /* power up lvds pll and ldo */
	        lvds_msk_reg(lvds, MIPIPHY_REG1,
	                     m_SYNC_RST | m_LDO_PWR_DOWN | m_PLL_PWR_DOWN,
	                     v_SYNC_RST(0) | v_LDO_PWR_DOWN(0) | v_PLL_PWR_DOWN(0));
		/* enable lvds lane and power on pll */
		lvds_writel(lvds, MIPIPHY_REGEB,
			    v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) |
			    v_LANE3_EN(1) | v_LANECLK_EN(1) | v_PLL_PWR_OFF(0));

	        /* enable lvds */
	        lvds_msk_reg(lvds, MIPIPHY_REGE3,
	                     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
	                     v_MIPI_EN(0) | v_LVDS_EN(1) | v_TTL_EN(0));
        } else {
                lvds_msk_reg(lvds, MIPIPHY_REGE3,
	                     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
	                     v_MIPI_EN(0) | v_LVDS_EN(0) | v_TTL_EN(1));
        }
	/* delay for waitting pll lock on */
	while (delay_times--) {
		if (lvds_phy_lock(lvds)) {
			break;
		}
		udelay(100);
	}

	if (delay_times <= 0)
		printf("wait lvds phy lock failed\n");
		
        return 0;
}

static void rk31xx_output_lvds(vidinfo_t *vid)
{
	struct rk_lvds_device *lvds = &rk31xx_lvds;
	u32 val = 0;

	/* if LVDS transmitter source from VOP, vop_dclk need get invert
	 * set iomux in dts pinctrl
	 */
	val = 0;
	val |= v_LVDSMODE_EN(1) | v_MIPIPHY_TTL_EN(0);  /* enable lvds mode */
	val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC);    /* config data source */
	val |= v_LVDS_OUTPUT_FORMAT(vid->lvds_format); /* config lvds_format */
	val |= v_LVDS_MSBSEL(LVDS_MSB_D7);      /* LSB receive mode */
	val |= v_MIPIPHY_LANE0_EN(1) | v_MIPIDPI_FORCEX_EN(1);
	grf_writel(val, GRF_LVDS_CON0);

	/* digital internal disable */
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(0));

	/* set pll prediv and fbdiv */
	lvds_writel(lvds, MIPIPHY_REG3, v_PREDIV(2) | v_FBDIV_MSB(0));
	lvds_writel(lvds, MIPIPHY_REG4, v_FBDIV_LSB(28));

	lvds_writel(lvds, MIPIPHY_REGE8, 0xfc);

	/* set lvds mode and reset phy config */
	lvds_msk_reg(lvds, MIPIPHY_REGE0,
	             m_MSB_SEL | m_DIG_INTER_RST,
	             v_MSB_SEL(1) | v_DIG_INTER_RST(1));

	rk31xx_lvds_pwr_on(vid);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));


}

static void rk31xx_output_lvttl(vidinfo_t *vid)
{
	u32 val = 0;
	struct rk_lvds_device *lvds = &rk31xx_lvds;

	grf_writel(0xffff5555, GRF_GPIO2B_IOMUX);
	grf_writel(0x00ff0055, GRF_GPIO2C_IOMUX);
	grf_writel(0x77771111, GRF_GPIO2C_IOMUX2);
	grf_writel(0x700c1004, GRF_GPIO2D_IOMUX);

	val |= v_LVDSMODE_EN(0) | v_MIPIPHY_TTL_EN(1);  /* enable lvds mode */
	val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC);    /* config data source */
	grf_writel(0xffff0380, GRF_LVDS_CON0);

	val = v_MIPITTL_CLK_EN(1) | v_MIPITTL_LANE0_EN(1) |
	        v_MIPITTL_LANE1_EN(1) | v_MIPITTL_LANE2_EN(1) |
	        v_MIPITTL_LANE3_EN(1);
	grf_writel(val, GRF_SOC_CON1);

	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
	        v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);

	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
	        v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);

	rk31xx_lvds_pwr_on(vid);
		
}

static int rk31xx_lvds_en(vidinfo_t *vid)
{
	rk31xx_lvds.base = 0x20038000;
	rk31xx_lvds.ctrl_reg = MIPIPHY_STATUS;

	switch (vid->screen_type) {
	case SCREEN_LVDS:
		rk31xx_output_lvds(vid);
	        break;
	case SCREEN_RGB:
		rk31xx_output_lvttl(vid);
	        break;
	default:
	        printf("unsupport screen type\n");
	        break;
	}
	return 0;
}

#endif /* RK3126 || RK3128 */


static int win0_set_par(struct lcdc_device *lcdc_dev,
			 struct fb_dsp_info *fb_info,
			 vidinfo_t *vid)
{
	int x_scale, y_scale;
 	int msk,val;
	
	x_scale = CalScale(fb_info->xact, fb_info->xsize);
	y_scale = CalScale(fb_info->yact, fb_info->ysize);
	lcdc_writel(lcdc_dev,WIN0_YRGB_MST, fb_info->yaddr);
	val = v_X_SCL_FACTOR(x_scale) | v_Y_SCL_FACTOR(y_scale);
	lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_YRGB, val);
	msk = m_WIN0_EN | m_WIN0_FORMAT;
	val = v_WIN0_EN(1) | v_WIN0_FORMAT(fb_info->format);
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, msk, val);
	val = v_ACT_WIDTH(fb_info->xact) | v_ACT_HEIGHT(fb_info->yact);
	lcdc_writel(lcdc_dev, WIN0_ACT_INFO, val);
	val = v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) |
	      v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
	lcdc_writel(lcdc_dev, WIN0_DSP_ST, val); 
	val = v_DSP_WIDTH(fb_info->xsize) | v_DSP_HEIGHT(fb_info->ysize);
	lcdc_writel(lcdc_dev, WIN0_DSP_INFO, val);
	msk = m_COLOR_KEY_EN | m_COLOR_KEY_VAL;
	val = v_COLOR_KEY_EN(0) | v_COLOR_KEY_VAL(0);
	lcdc_msk_reg(lcdc_dev, WIN0_COLOR_KEY, msk, val);
	switch(fb_info->format) 
	{
	case ARGB888:
		val = v_ARGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB888:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		val = v_RGB565_VIRWIDTH(fb_info->xvir);
		break;
	case YUV422:
	case YUV420:
		val = v_YUV_VIRWIDTH(fb_info->xvir);  
		break;
	default:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		printf("%s unknow format:%d\n", __func__,
		       fb_info->format);
		break;
	}
	lcdc_writel(lcdc_dev, WIN0_VIR, val);
	lcdc_cfg_done(lcdc_dev);
	return 0;
}

static int win1_set_par(struct lcdc_device *lcdc_dev,
			 struct fb_dsp_info *fb_info,
			 vidinfo_t *vid)
{
	int x_scale, y_scale;
 	int msk,val;
	
	x_scale = CalScale(fb_info->xact, fb_info->xsize);
	y_scale = CalScale(fb_info->yact, fb_info->ysize);
	if (lcdc_dev->soc_type == CONFIG_RK3036) {
		val = v_X_SCL_FACTOR(x_scale) | v_Y_SCL_FACTOR(y_scale);
		lcdc_writel(lcdc_dev, WIN1_SCL_FACTOR_YRGB, val);
	} else {
		if ((fb_info->xact != fb_info->xsize) ||
		      (fb_info->yact != fb_info->ysize))
		      printf("win1 don't support scale!\n");
	}
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_WIN1_EN | m_WIN1_FORMAT, 
		     v_WIN1_EN(1) | v_WIN1_FORMAT(fb_info->format));

	if (lcdc_dev->soc_type == CONFIG_RK3036) {
		val = v_ACT_WIDTH(fb_info->xact) | v_ACT_HEIGHT(fb_info->yact);
		lcdc_writel(lcdc_dev, WIN1_ACT_INFO, val);
		val = v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) | 
		      v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev, WIN1_DSP_ST, val);
		val = v_DSP_WIDTH(fb_info->xsize)| v_DSP_HEIGHT(fb_info->ysize);
		lcdc_writel(lcdc_dev, WIN1_DSP_INFO, val);
	} else {
		val = v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) | 
		      v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev, WIN1_DSP_ST_RK312X, val);
		val = v_DSP_WIDTH(fb_info->xsize)| v_DSP_HEIGHT(fb_info->ysize);
		lcdc_writel(lcdc_dev, WIN1_DSP_INFO_RK312X, val);
	}

	msk = m_COLOR_KEY_EN | m_COLOR_KEY_VAL;
	val = v_COLOR_KEY_EN(0) | v_COLOR_KEY_VAL(0);
	lcdc_msk_reg(lcdc_dev, WIN1_COLOR_KEY, msk, val);
	switch(fb_info->format) 
	{
	case ARGB888:
	case RGB888:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		val = v_RGB565_VIRWIDTH(fb_info->xvir);
		break;
	case YUV422:
	case YUV420:   
		val = v_YUV_VIRWIDTH(fb_info->xvir);         
		break;
	default:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		printf("unknown format %d\n",fb_info->format);
		break;
	}
	lcdc_writel(lcdc_dev, WIN1_VIR, val);
	if (lcdc_dev->soc_type == CONFIG_RK3036)
		lcdc_writel(lcdc_dev, WIN1_MST, fb_info->yaddr);
	else
		lcdc_writel(lcdc_dev, WIN1_MST_RK312X, fb_info->yaddr);
	lcdc_cfg_done(lcdc_dev);
	return 0;
}


static void rk312x_lcdc_select_bcsh(struct lcdc_device *lcdc_dev)
{
	int msk, val;
	int bcsh_open;
	if (lcdc_dev->overlay_mode == VOP_YUV_DOMAIN) {
		if (lcdc_dev->output_color == COLOR_YCBCR) {/* bypass */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_Y2R_EN | m_BCSH_R2Y_EN,
				     v_BCSH_Y2R_EN(0) | v_BCSH_R2Y_EN(0));
			bcsh_open = 0;
		} else 	{/* YUV2RGB */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
			     m_BCSH_Y2R_EN | m_BCSH_Y2R_CSC_MODE |
			     m_BCSH_R2Y_EN,
			     v_BCSH_Y2R_EN(1) |
			     v_BCSH_Y2R_CSC_MODE(VOP_Y2R_CSC_MPEG) |
			     v_BCSH_R2Y_EN(0));
			bcsh_open = 1;
		}
	} else {	/* overlay_mode=VOP_RGB_DOMAIN */
		if (lcdc_dev->output_color == COLOR_RGB) {	/* bypass */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_R2Y_EN | m_BCSH_Y2R_EN,
				     v_BCSH_R2Y_EN(1) | v_BCSH_Y2R_EN(1));
			bcsh_open = 0;
		} else {/* RGB2YUV */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_R2Y_EN |
					m_BCSH_R2Y_CSC_MODE | m_BCSH_Y2R_EN,
					v_BCSH_R2Y_EN(1) |
					v_BCSH_R2Y_CSC_MODE(VOP_Y2R_CSC_MPEG) |
					v_BCSH_Y2R_EN(0));
			bcsh_open = 1;
		}
	}
	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, m_SW_OVERLAY_MODE,
		     v_SW_OVERLAY_MODE(lcdc_dev->overlay_mode));
	if (bcsh_open) {
		lcdc_msk_reg(lcdc_dev,
			     BCSH_CTRL, m_BCSH_EN | m_BCSH_OUT_MODE,
			     v_BCSH_EN(1) | v_BCSH_OUT_MODE(3));
		lcdc_writel(lcdc_dev, BCSH_BCS,
			    v_BCSH_BRIGHTNESS(0x00) |
			    v_BCSH_CONTRAST(0x80) |
			    v_BCSH_SAT_CON(0x80));
		lcdc_writel(lcdc_dev, BCSH_H, v_BCSH_COS_HUE(0x80));
	} else {
		msk = m_BCSH_EN;
		val = v_BCSH_EN(0);
		lcdc_msk_reg(lcdc_dev, BCSH_CTRL, msk, val);
	}
}


/* Configure VENC for a given Mode (NTSC / PAL) */
void rk_lcdc_set_par(struct fb_dsp_info *fb_info,
			     vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;
	if (vid->vmode) {
		fb_info->ysize /= 2;
		fb_info->ypos  /= 2;
	}

	fb_info->layer_id = lcdc_dev->dft_win;
	if(fb_info->layer_id == WIN0)
		win0_set_par(lcdc_dev, fb_info, vid);
	else if(fb_info->layer_id == WIN1) 
		win1_set_par(lcdc_dev, fb_info, vid);
	else 
		printf("%s --->unknow lay_id \n", __func__);

	/*setenv("bootdelay", "3");*/
}

int rk_lcdc_load_screen(vidinfo_t *vid)
{
	int face = 0;
	int msk,val;
	int bg_val = 0;
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;
	lcdc_dev->output_color = COLOR_RGB;
	lcdc_dev->overlay_mode = VOP_RGB_DOMAIN;
	switch (vid->screen_type) {
	case SCREEN_HDMI:
        	lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
			      m_HDMI_DCLK_EN, v_HDMI_DCLK_EN(1));
		if (vid->pixelrepeat) {
			lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
				      m_CORE_CLK_DIV_EN, v_CORE_CLK_DIV_EN(1));
		}
		if (gd->arch.chiptype == CONFIG_RK3128) {
			 lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
				      m_SW_UV_OFFSET_EN,
				      v_SW_UV_OFFSET_EN(0));
			 msk = m_HDMI_HSYNC_POL | m_HDMI_VSYNC_POL |
				m_HDMI_DEN_POL;
			 val = v_HDMI_HSYNC_POL(vid->vl_hsp) |
			       v_HDMI_VSYNC_POL(vid->vl_vsp) |
			       v_HDMI_DEN_POL(vid->vl_oep);
			 lcdc_msk_reg(lcdc_dev, INT_SCALER, msk, val);
		 } else {
			 msk = (1 << 4) | (1 << 5) | (1 << 6);
			 val = (vid->vl_hsp << 4) |
				 (vid->vl_vsp << 5) |
				 (vid->vl_oep << 6);
			 grf_writel((msk << 16)|val,GRF_SOC_CON2);
		 }
		 bg_val = 0x801080;
		break;
	case SCREEN_TVOUT:
	case SCREEN_TVOUT_TEST:
		lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
			      m_TVE_DAC_DCLK_EN | m_HDMI_DCLK_EN,
			      v_TVE_DAC_DCLK_EN(1) |
			      v_HDMI_DCLK_EN(1));
		if (vid->pixelrepeat) {
			lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
				      m_CORE_CLK_DIV_EN, v_CORE_CLK_DIV_EN(1));
		}
		if(vid->vl_col == 720 && vid->vl_row== 576) {
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0,m_TVE_MODE, v_TVE_MODE(TV_PAL));
		} else if(vid->vl_col == 720 && vid->vl_row== 480) {
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0, m_TVE_MODE, v_TVE_MODE(TV_NTSC));
		} else {
			printf("unsupported video timing!\n");
			return -EINVAL;
		}
		if (vid->screen_type == SCREEN_TVOUT_TEST) {/*for TVE index test,vop must ovarlay at yuv domain*/
			lcdc_dev->overlay_mode = VOP_YUV_DOMAIN;
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0, m_SW_UV_OFFSET_EN,
				     v_SW_UV_OFFSET_EN(1));
		} else {
			bg_val = 0x801080;
		}
		if (lcdc_dev->soc_type == CONFIG_RK3128) {
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
				     m_SW_UV_OFFSET_EN,
				     v_SW_UV_OFFSET_EN(1));
		}

		break;
	case SCREEN_LVDS:
		msk = m_LVDS_DCLK_INVERT | m_LVDS_DCLK_EN;
		val = v_LVDS_DCLK_INVERT(1) | v_LVDS_DCLK_EN(1);
		lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, msk, val);	
		break;
	case SCREEN_RGB:
		lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, m_RGB_DCLK_EN |
			      m_LVDS_DCLK_EN, v_RGB_DCLK_EN(1) | v_LVDS_DCLK_EN(1));
		break;
	case SCREEN_MIPI:
		lcdc_msk_reg(lcdc_dev,AXI_BUS_CTRL, m_MIPI_DCLK_EN, v_MIPI_DCLK_EN(1));
		break;
	default:
		printf("unsupport screen_type %d\n",vid->screen_type);
		break;
	}
 	
    	lcdc_msk_reg(lcdc_dev,DSP_CTRL1, m_BLACK_EN | m_BLANK_EN | m_DSP_OUT_ZERO|
		      m_DSP_BG_SWAP | m_DSP_RG_SWAP | m_DSP_RB_SWAP | m_BG_COLOR |
		      m_DSP_DELTA_SWAP | m_DSP_DUMMY_SWAP, v_BLACK_EN(0) |
		      v_BLANK_EN(0) | v_DSP_OUT_ZERO(0) | v_DSP_BG_SWAP(0) |
		      v_DSP_RG_SWAP(0) | v_DSP_RB_SWAP(0) | v_BG_COLOR(0) |
		      v_DSP_DELTA_SWAP(0) | v_DSP_DUMMY_SWAP(0));

	if (gd->arch.chiptype == CONFIG_RK3036)
	    bg_val = 0;

	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, m_BG_COLOR,
		      v_BG_COLOR(bg_val));

	switch (vid->lcd_face)
	{
    	case OUT_P565:
        	face = OUT_P565;
		msk =  m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		       m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val =  v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		       v_DITHER_DOWN_MODE(0) | v_DITHER_DOWN_SEL(1);
		break;
    	case OUT_P666:
		face = OUT_P666;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		      m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		      v_DITHER_DOWN_MODE(1) | v_DITHER_DOWN_SEL(1);
		break;
    	case OUT_D888_P565:
		msk =  m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		       m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val =  v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		       v_DITHER_DOWN_MODE(0) | v_DITHER_DOWN_SEL(1);
		break;
    	case OUT_D888_P666:
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		      m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		      v_DITHER_DOWN_MODE(1) | v_DITHER_DOWN_SEL(1);
    	case OUT_P888:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);
        	break;
    	default:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);
		printf("unknown interface:%d\n", face);
        	break;
	}

	msk |= m_HSYNC_POL | m_HSYNC_POL | m_DEN_POL |
	       m_DCLK_POL | m_DSP_OUT_FORMAT;
	val |=  v_HSYNC_POL(vid->vl_hsp) | v_VSYNC_POL(vid->vl_vsp) |
		v_DEN_POL(vid->vl_oep)| v_DCLK_POL(vid->vl_clkp) |
		v_DSP_OUT_FORMAT(face);
    	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);

	val = v_HORPRD(vid->vl_hspw + vid->vl_hbpd + vid->vl_col + vid->vl_hfpd) |
	      v_HSYNC(vid->vl_hspw);
	lcdc_writel(lcdc_dev, DSP_HTOTAL_HS_END, val);

	val = v_HAEP(vid->vl_hspw + vid->vl_hbpd + vid->vl_col) |
	      v_HASP(vid->vl_hspw + vid->vl_hbpd);
	lcdc_writel(lcdc_dev, DSP_HACT_ST_END, val);

	if(vid->vmode) {
		//First Field Timing
		val = v_VERPRD(2 * (vid->vl_vspw + vid->vl_vbpd + vid->vl_vfpd) +
		      vid->vl_row + 1) | v_VSYNC(vid->vl_vspw);
		lcdc_writel(lcdc_dev, DSP_VTOTAL_VS_END, val);
		val = v_VAEP(vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2)|
		      v_VASP(vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev,DSP_VACT_ST_END, val);
		//Second Field Timing
		val = v_VSYNC_ST_F1(vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2 + vid->vl_vfpd) |
		      v_VSYNC_END_F1(2 * vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2 + vid->vl_vfpd);
		lcdc_writel(lcdc_dev, DSP_VS_ST_END_F1, val);
		val = v_VAEP(2 * (vid->vl_vspw + vid->vl_vbpd) + vid->vl_row + vid->vl_vfpd + 1)|
		      v_VASP(2 * (vid->vl_vspw + vid->vl_vbpd) + vid->vl_row/2 + vid->vl_vfpd + 1);
		lcdc_writel(lcdc_dev, DSP_VACT_ST_END_F1, val);
		msk = m_INTERLACE_DSP_EN | m_WIN1_INTERLACE_EN |
		      m_WIN0_YRGB_DEFLICK_EN | m_WIN0_CBR_DEFLICK_EN |
		      m_WIN0_INTERLACE_EN;
		val = v_INTERLACE_DSP_EN(1) | v_WIN1_INTERLACE_EN(1) |
		      v_WIN0_YRGB_DEFLICK_EN(1) | v_WIN0_CBR_DEFLICK_EN(1) |
		      v_WIN0_INTERLACE_EN(1);
		lcdc_msk_reg(lcdc_dev,DSP_CTRL0, msk, val);

		lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
			     m_INTERLACE_DSP_EN |
			     m_WIN0_YRGB_DEFLICK_EN |
			     m_WIN0_CBR_DEFLICK_EN |
			     m_INTERLACE_FIELD_POL |
			     m_WIN0_INTERLACE_EN |
			     m_WIN1_INTERLACE_EN,
			     v_INTERLACE_DSP_EN(1) |
			     v_WIN0_YRGB_DEFLICK_EN(1) |
			     v_WIN0_CBR_DEFLICK_EN(1) |
			     v_INTERLACE_FIELD_POL(0) |
			     v_WIN0_INTERLACE_EN(1) |
			     v_WIN1_INTERLACE_EN(1));

		msk = m_LF_INT_NUM;
		val = v_LF_INT_NUM(vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2);
		lcdc_msk_reg(lcdc_dev, INT_STATUS, msk, val);

	} else {
		val = v_VERPRD(vid->vl_vspw + vid->vl_vbpd + vid->vl_row +
		      vid->vl_vfpd) | v_VSYNC(vid->vl_vspw);
		lcdc_writel(lcdc_dev, DSP_VTOTAL_VS_END, val);
		val = v_VAEP(vid->vl_vspw + vid->vl_vbpd + vid->vl_row) |
		      v_VASP(vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev, DSP_VACT_ST_END, val);
		msk = m_INTERLACE_DSP_EN | m_WIN1_INTERLACE_EN |
		      m_WIN0_YRGB_DEFLICK_EN | m_WIN0_CBR_DEFLICK_EN |
		      m_WIN0_INTERLACE_EN;;
		val = v_INTERLACE_DSP_EN(0) | v_WIN1_INTERLACE_EN(0) |
		      v_WIN0_YRGB_DEFLICK_EN(0) | v_WIN0_CBR_DEFLICK_EN(0) |
		      v_WIN0_INTERLACE_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);

		lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
			     m_INTERLACE_DSP_EN |
			     m_WIN0_YRGB_DEFLICK_EN |
			     m_WIN0_CBR_DEFLICK_EN |
			     m_INTERLACE_FIELD_POL |
			     m_WIN0_INTERLACE_EN |
			     m_WIN1_INTERLACE_EN,
			     v_INTERLACE_DSP_EN(0) |
			     v_WIN0_YRGB_DEFLICK_EN(0) |
			     v_WIN0_CBR_DEFLICK_EN(0) |
			     v_INTERLACE_FIELD_POL(0) |
			     v_WIN0_INTERLACE_EN(0) |
			     v_WIN1_INTERLACE_EN(0));

		msk = m_LF_INT_NUM;
		val = v_LF_INT_NUM(vid->vl_vspw + vid->vl_vbpd + vid->vl_row);
		lcdc_msk_reg(lcdc_dev, INT_STATUS, msk, val);
	}
	if ((lcdc_dev->soc_type == CONFIG_RK3128) &&
	    ((vid->screen_type == SCREEN_HDMI) ||
	     (vid->screen_type == SCREEN_TVOUT) ||
	     (vid->screen_type == SCREEN_TVOUT_TEST))) {
		lcdc_dev->output_color = vid->color_mode;
		rk312x_lcdc_select_bcsh(lcdc_dev);
	}
	
	lcdc_cfg_done(lcdc_dev);
#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	if ((vid->screen_type == SCREEN_LVDS) ||
	    (vid->screen_type == SCREEN_RGB)) {
		rk31xx_lvds_en(vid);
	} else if ((vid->screen_type == SCREEN_MIPI)||
		   (vid->screen_type == SCREEN_DUAL_MIPI)) {
#if defined(CONFIG_RK32_DSI)
		rk32_mipi_enable(vid);
#endif
	}
#endif
	return 0;
}

/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable)
{
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;
#if defined(CONFIG_RK32_DSI)
	if (((panel_info.screen_type == SCREEN_MIPI) ||
			   (panel_info.screen_type == SCREEN_DUAL_MIPI))) {
		if (enable == 0) {
			rk32_dsi_enable();
		} else if (enable == 1) {
			rk32_dsi_disable();
		}
	}
#endif
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_LCDC_STANDBY,
		     v_LCDC_STANDBY(enable ? 1 : 0));
	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, m_DSP_OUT_ZERO | m_BLACK_EN,
			v_DSP_OUT_ZERO(enable ? 1 : 0) | v_BLACK_EN(enable ? 1 : 0));
	lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, m_IO_PAD_CLK, v_IO_PAD_CLK(enable ? 1 : 0));
	lcdc_cfg_done(lcdc_dev);
}


#if defined(CONFIG_OF_LIBFDT)
static int rk312x_lcdc_parse_dt(struct lcdc_device *lcdc_dev,
					const void *blob)
{
	int order = FB0_WIN0_FB1_WIN1_FB2_WIN2;

	lcdc_dev->node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_RK312X_LCDC);
	if (lcdc_dev->node < 0) {
		lcdc_dev->node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_RK3036_LCDC);
		if (lcdc_dev->node < 0) {
			debug("can't find dts node for lcdc\n");
			return -ENODEV;
		}
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
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;

	lcdc_dev->soc_type = gd->arch.chiptype;
#ifdef CONFIG_OF_LIBFDT
	if (!lcdc_dev->node)
		rk312x_lcdc_parse_dt(lcdc_dev, gd->fdt_blob);
#endif
	if (lcdc_dev->node <= 0) {
		if (lcdc_dev->soc_type == CONFIG_RK3036) {
			lcdc_dev->regs = 0x10118000;
		} else {
			lcdc_dev->regs = 0x1010e000;
		}
	}
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_AUTO_GATING_EN |
		      m_LCDC_STANDBY | m_DMA_STOP, v_AUTO_GATING_EN(0)|
		      v_LCDC_STANDBY(0)|v_DMA_STOP(0));
	lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, m_MMU_EN, v_MMU_EN(0));
	lcdc_cfg_done(lcdc_dev);

	return 0;
}

