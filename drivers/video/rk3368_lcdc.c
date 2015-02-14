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
#include "rk3368_lcdc.h"

/*******************register definition**********************/
struct lcdc_device rk33_lcdc;

extern int rk31xx_lvds_enable(vidinfo_t * vid);
//extern int rk32_edp_enable(vidinfo_t * vid);
//extern int rk32_mipi_enable(vidinfo_t * vid);
//extern int rk32_dsi_enable(void);
//extern int rk32_dsi_sync(void);

static int rk3368_lcdc_csc_mode(struct lcdc_device *lcdc_dev,
				 struct fb_dsp_info *fb_info,
				 vidinfo_t *vid)
{
    u32 csc_mode = 0;
	if (lcdc_dev->overlay_mode == VOP_YUV_DOMAIN) {
		switch (fb_info->format) {
		case ARGB888:
		case RGB888:
		case RGB565:
			if ((vid->vl_col < 1280) &&
			    (vid->vl_row < 720)) {
				csc_mode = VOP_R2Y_CSC_BT601;
			} else {
				csc_mode = VOP_R2Y_CSC_BT709;
			}
			break;
		default:
			break;
		}
	} else if (lcdc_dev->overlay_mode == VOP_RGB_DOMAIN) {
		switch (fb_info->format) {
		case YUV420:
				csc_mode = VOP_Y2R_CSC_MPEG;
			break;
		default:
			break;
		}
	}
	return csc_mode;
}

static int win0_set_par(struct lcdc_device *lcdc_dev,
			     struct fb_dsp_info *fb_info,
			     vidinfo_t *vid)
{
	u32 mask,val;
	u32 csc_mode;
	u32 off;

	off = fb_info->layer_id * 0x40;;
	csc_mode = rk3368_lcdc_csc_mode(lcdc_dev, fb_info, vid);

	mask = m_WIN0_EN | m_WIN0_DATA_FMT | m_WIN0_FMT_10 |
	    m_WIN0_LB_MODE | m_WIN0_RB_SWAP | m_WIN0_X_MIRROR |
	    m_WIN0_Y_MIRROR | m_WIN0_CSC_MODE;
	val = v_WIN0_EN(1) |
	    v_WIN0_DATA_FMT(fb_info->format) |
	    v_WIN0_FMT_10(0) |
	    v_WIN0_LB_MODE(5) |
	    v_WIN0_RB_SWAP(0) |
	    v_WIN0_X_MIRROR(0) |
	    v_WIN0_Y_MIRROR(0) |
	    v_WIN0_CSC_MODE(csc_mode);
	lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, mask, val);

	val = v_WIN0_ACT_WIDTH(fb_info->xact) |
	    v_WIN0_ACT_HEIGHT(fb_info->yact);
	lcdc_writel(lcdc_dev, WIN0_ACT_INFO + off, val);

	val = v_WIN0_DSP_WIDTH(fb_info->xsize) |
	    v_WIN0_DSP_HEIGHT(fb_info->ysize);
	lcdc_writel(lcdc_dev, WIN0_DSP_INFO + off, val);

	val = v_WIN0_DSP_XST(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) |
	    v_WIN0_DSP_YST(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
	lcdc_writel(lcdc_dev, WIN0_DSP_ST + off, val);

	val = v_WIN0_HS_FACTOR_YRGB(0x1000) |
	    v_WIN0_VS_FACTOR_YRGB(0x1000);
	lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_YRGB + off, val);

	val = v_WIN0_HS_FACTOR_CBR(0x1000) |
	    v_WIN0_VS_FACTOR_CBR(0x1000);
	lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_CBR + off, val);

	if(fb_info->xsize > 2560) {
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, m_WIN0_LB_MODE,
			     v_WIN0_LB_MODE(LB_RGB_3840X2));
	} else if(fb_info->xsize > 1920) {
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, m_WIN0_LB_MODE,
			      v_WIN0_LB_MODE(LB_RGB_2560X4));
	} else if(fb_info->xsize > 1280){
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, m_WIN0_LB_MODE,
			     v_WIN0_LB_MODE(LB_RGB_1920X5));
	} else {
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, m_WIN0_LB_MODE,
			     v_WIN0_LB_MODE(LB_RGB_1280X8));
	}
	
	switch (fb_info->format) {
	case ARGB888:
		lcdc_writel(lcdc_dev, WIN0_VIR + off, v_ARGB888_VIRWIDTH(fb_info->xvir));
		break;
	case RGB888:
		lcdc_writel(lcdc_dev, WIN0_VIR + off, v_ARGB888_VIRWIDTH(fb_info->xvir));
		break;
	case RGB565:
		lcdc_writel(lcdc_dev, WIN0_VIR + off, v_ARGB888_VIRWIDTH(fb_info->xvir));
		break;
	case YUV422:
	case YUV420:
		lcdc_writel(lcdc_dev, WIN0_VIR + off, v_ARGB888_VIRWIDTH(fb_info->xvir));
		if(fb_info->xsize > 1280) {
			lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, m_WIN0_LB_MODE,
				     v_WIN0_LB_MODE(LB_YUV_3840X5));
		} else {
			lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, m_WIN0_LB_MODE,
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
	struct lcdc_device *lcdc_dev = &rk33_lcdc;

	fb_info->layer_id = lcdc_dev->dft_win;

	if (vid->vmode) {//need check
		fb_info->ysize /= 2;
		fb_info->ypos  /= 2;
	}
	
	switch (fb_info->layer_id) {
	case WIN0:
	case WIN1:
		win0_set_par(lcdc_dev, fb_info, vid);
		break;
	default:
		printf("%s --->unknow lay_id \n", __func__);
		break;
	}
	lcdc_cfg_done(lcdc_dev);
}

static int rk3368_lcdc_post_cfg(struct lcdc_device *lcdc_dev, struct rk_screen *screen)
{
	u16 x_res = screen->mode.xres;
	u16 y_res = screen->mode.yres;
	u32 mask, val;
	u16 h_total, v_total;
	u16 post_hsd_en, post_vsd_en;
	u16 post_dsp_hact_st, post_dsp_hact_end;
	u16 post_dsp_vact_st, post_dsp_vact_end;
	u16 post_dsp_vact_st_f1, post_dsp_vact_end_f1;
	u16 post_h_fac, post_v_fac;

	h_total = screen->mode.hsync_len + screen->mode.left_margin +
	    x_res + screen->mode.right_margin;
	v_total = screen->mode.vsync_len + screen->mode.upper_margin +
	    y_res + screen->mode.lower_margin;

	if (screen->post_dsp_stx + screen->post_xsize > x_res) {
		debug("post:stx[%d]+xsize[%d]>x_res[%d]\n",
			 screen->post_dsp_stx, screen->post_xsize, x_res);
		screen->post_dsp_stx = x_res - screen->post_xsize;
	}
	if (screen->x_mirror == 0) {
		post_dsp_hact_st = screen->post_dsp_stx +
		    screen->mode.hsync_len + screen->mode.left_margin;
		post_dsp_hact_end = post_dsp_hact_st + screen->post_xsize;
	} else {
		post_dsp_hact_end = h_total - screen->mode.right_margin -
		    screen->post_dsp_stx;
		post_dsp_hact_st = post_dsp_hact_end - screen->post_xsize;
	}
	if ((screen->post_xsize < x_res) && (screen->post_xsize != 0)) {
		post_hsd_en = 1;
		post_h_fac =
		    GET_SCALE_FACTOR_BILI_DN(x_res, screen->post_xsize);
	} else {
		post_hsd_en = 0;
		post_h_fac = 0x1000;
	}

	if (screen->post_dsp_sty + screen->post_ysize > y_res) {
		debug("post:sty[%d]+ysize[%d]> y_res[%d]\n",
			 screen->post_dsp_sty, screen->post_ysize, y_res);
		screen->post_dsp_sty = y_res - screen->post_ysize;
	}

	if ((screen->post_ysize < y_res) && (screen->post_ysize != 0)) {
		post_vsd_en = 1;
		post_v_fac = GET_SCALE_FACTOR_BILI_DN(y_res,
						      screen->post_ysize);
	} else {
		post_vsd_en = 0;
		post_v_fac = 0x1000;
	}

	if (screen->mode.vmode == FB_VMODE_INTERLACED) {
		post_dsp_vact_st = screen->post_dsp_sty / 2 +
					screen->mode.vsync_len +
					screen->mode.upper_margin;
		post_dsp_vact_end = post_dsp_vact_st +
					screen->post_ysize / 2;

		post_dsp_vact_st_f1 = screen->mode.vsync_len +
				      screen->mode.upper_margin +
				      y_res/2 +
				      screen->mode.lower_margin +
				      screen->mode.vsync_len +
				      screen->mode.upper_margin +
				      screen->post_dsp_sty / 2 +
				      1;
		post_dsp_vact_end_f1 = post_dsp_vact_st_f1 +
					screen->post_ysize/2;
	} else {
		if (screen->y_mirror == 0) {
			post_dsp_vact_st = screen->post_dsp_sty +
			    screen->mode.vsync_len +
			    screen->mode.upper_margin;
			post_dsp_vact_end = post_dsp_vact_st +
				screen->post_ysize;
		} else {
			post_dsp_vact_end = v_total -
				screen->mode.lower_margin -
			    screen->post_dsp_sty;
			post_dsp_vact_st = post_dsp_vact_end -
				screen->post_ysize;
		}
		post_dsp_vact_st_f1 = 0;
		post_dsp_vact_end_f1 = 0;
	}
	/*DBG(1, "post:xsize=%d,ysize=%d,xpos=%d",
	    screen->post_xsize, screen->post_ysize, screen->xpos);
	DBG(1, ",ypos=%d,hsd_en=%d,h_fac=%d,vsd_en=%d,v_fac=%d\n",
	    screen->ypos, post_hsd_en, post_h_fac, post_vsd_en, post_v_fac);*/
	mask = m_DSP_HACT_END_POST | m_DSP_HACT_ST_POST;
	val = v_DSP_HACT_END_POST(post_dsp_hact_end) |
	    v_DSP_HACT_ST_POST(post_dsp_hact_st);
	lcdc_msk_reg(lcdc_dev, POST_DSP_HACT_INFO, mask, val);

	mask = m_DSP_VACT_END_POST | m_DSP_VACT_ST_POST;
	val = v_DSP_VACT_END_POST(post_dsp_vact_end) |
	    v_DSP_VACT_ST_POST(post_dsp_vact_st);
	lcdc_msk_reg(lcdc_dev, POST_DSP_VACT_INFO, mask, val);

	mask = m_POST_HS_FACTOR_YRGB | m_POST_VS_FACTOR_YRGB;
	val = v_POST_HS_FACTOR_YRGB(post_h_fac) |
	    v_POST_VS_FACTOR_YRGB(post_v_fac);
	lcdc_msk_reg(lcdc_dev, POST_SCL_FACTOR_YRGB, mask, val);

	mask = m_DSP_VACT_END_POST_F1 | m_DSP_VACT_ST_POST_F1;
	val = v_DSP_VACT_END_POST_F1(post_dsp_vact_end_f1) |
	    v_DSP_VACT_ST_POST_F1(post_dsp_vact_st_f1);
	lcdc_msk_reg(lcdc_dev, POST_DSP_VACT_INFO_F1, mask, val);

	mask = m_POST_HOR_SD_EN | m_POST_VER_SD_EN;
	val = v_POST_HOR_SD_EN(post_hsd_en) | v_POST_VER_SD_EN(post_vsd_en);
	lcdc_msk_reg(lcdc_dev, POST_SCL_CTRL, mask, val);
	return 0;
}

static int rk3368_config_timing(struct lcdc_device *lcdc_dev, struct rk_screen *screen)
{
	u16 hsync_len = screen->mode.hsync_len;
	u16 left_margin = screen->mode.left_margin;
	u16 right_margin = screen->mode.right_margin;
	u16 vsync_len = screen->mode.vsync_len;
	u16 upper_margin = screen->mode.upper_margin;
	u16 lower_margin = screen->mode.lower_margin;
	u16 x_res = screen->mode.xres;
	u16 y_res = screen->mode.yres;
	u32 mask, val;
	u16 h_total, v_total;
	u16 vact_end_f1, vact_st_f1, vs_end_f1, vs_st_f1;

	h_total = hsync_len + left_margin + x_res + right_margin;
	v_total = vsync_len + upper_margin + y_res + lower_margin;

	screen->post_dsp_stx = x_res * (100 - screen->overscan.left) / 200;
	screen->post_dsp_sty = y_res * (100 - screen->overscan.top) / 200;
	screen->post_xsize = x_res *
	    (screen->overscan.left + screen->overscan.right) / 200;
	screen->post_ysize = y_res *
	    (screen->overscan.top + screen->overscan.bottom) / 200;

	mask = m_DSP_HS_PW | m_DSP_HTOTAL;
	val = v_DSP_HS_PW(hsync_len) | v_DSP_HTOTAL(h_total);
	lcdc_msk_reg(lcdc_dev, DSP_HTOTAL_HS_END, mask, val);

	mask = m_DSP_HACT_END | m_DSP_HACT_ST;
	val = v_DSP_HACT_END(hsync_len + left_margin + x_res) |
	    v_DSP_HACT_ST(hsync_len + left_margin);
	lcdc_msk_reg(lcdc_dev, DSP_HACT_ST_END, mask, val);

	if (screen->mode.vmode == FB_VMODE_INTERLACED) {
		/* First Field Timing */
		mask = m_DSP_VS_PW | m_DSP_VTOTAL;
		val = v_DSP_VS_PW(vsync_len) |
		    v_DSP_VTOTAL(2 * (vsync_len + upper_margin +
				      lower_margin) + y_res + 1);
		lcdc_msk_reg(lcdc_dev, DSP_VTOTAL_VS_END, mask, val);

		mask = m_DSP_VACT_END | m_DSP_VACT_ST;
		val = v_DSP_VACT_END(vsync_len + upper_margin + y_res / 2) |
		    v_DSP_VACT_ST(vsync_len + upper_margin);
		lcdc_msk_reg(lcdc_dev, DSP_VACT_ST_END, mask, val);

		/* Second Field Timing */
		mask = m_DSP_VS_ST_F1 | m_DSP_VS_END_F1;
		vs_st_f1 = vsync_len + upper_margin + y_res / 2 + lower_margin;
		vs_end_f1 = 2 * vsync_len + upper_margin + y_res / 2 +
		    lower_margin;
		val = v_DSP_VS_ST_F1(vs_st_f1) | v_DSP_VS_END_F1(vs_end_f1);
		lcdc_msk_reg(lcdc_dev, DSP_VS_ST_END_F1, mask, val);

		mask = m_DSP_VACT_END_F1 | m_DSP_VAC_ST_F1;
		vact_end_f1 = 2 * (vsync_len + upper_margin) + y_res +
		    lower_margin + 1;
		vact_st_f1 = 2 * (vsync_len + upper_margin) + y_res / 2 +
		    lower_margin + 1;
		val =
		    v_DSP_VACT_END_F1(vact_end_f1) |
		    v_DSP_VAC_ST_F1(vact_st_f1);
		lcdc_msk_reg(lcdc_dev, DSP_VACT_ST_END_F1, mask, val);

		lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
			     m_DSP_INTERLACE | m_DSP_FIELD_POL,
			     v_DSP_INTERLACE(1) | v_DSP_FIELD_POL(0));
		mask =
		    m_WIN0_INTERLACE_READ | m_WIN0_YRGB_DEFLICK |
		    m_WIN0_CBR_DEFLICK;
		val =
		    v_WIN0_INTERLACE_READ(1) | v_WIN0_YRGB_DEFLICK(1) |
		    v_WIN0_CBR_DEFLICK(1);
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, mask, val);

		mask =
		    m_WIN1_INTERLACE_READ | m_WIN1_YRGB_DEFLICK |
		    m_WIN1_CBR_DEFLICK;
		val =
		    v_WIN1_INTERLACE_READ(1) | v_WIN1_YRGB_DEFLICK(1) |
		    v_WIN1_CBR_DEFLICK(1);
		lcdc_msk_reg(lcdc_dev, WIN1_CTRL0, mask, val);

		mask = m_WIN2_INTERLACE_READ;
		val = v_WIN2_INTERLACE_READ(1);
		lcdc_msk_reg(lcdc_dev, WIN2_CTRL0, mask, val);

		mask = m_WIN3_INTERLACE_READ;
		val = v_WIN3_INTERLACE_READ(1);
		lcdc_msk_reg(lcdc_dev, WIN3_CTRL0, mask, val);

		mask = m_HWC_INTERLACE_READ;
		val = v_HWC_INTERLACE_READ(1);
		lcdc_msk_reg(lcdc_dev, HWC_CTRL0, mask, val);

		mask = m_DSP_LINE_FLAG0_NUM;
		val =
		    v_DSP_LINE_FLAG0_NUM(vsync_len + upper_margin + y_res / 2);
		lcdc_msk_reg(lcdc_dev, LINE_FLAG, mask, val);
	} else {
		mask = m_DSP_VS_PW | m_DSP_VTOTAL;
		val = v_DSP_VS_PW(vsync_len) | v_DSP_VTOTAL(v_total);
		lcdc_msk_reg(lcdc_dev, DSP_VTOTAL_VS_END, mask, val);

		mask = m_DSP_VACT_END | m_DSP_VACT_ST;
		val = v_DSP_VACT_END(vsync_len + upper_margin + y_res) |
		    v_DSP_VACT_ST(vsync_len + upper_margin);
		lcdc_msk_reg(lcdc_dev, DSP_VACT_ST_END, mask, val);

		lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
			     m_DSP_INTERLACE | m_DSP_FIELD_POL,
			     v_DSP_INTERLACE(0) | v_DSP_FIELD_POL(0));

		mask =
		    m_WIN0_INTERLACE_READ | m_WIN0_YRGB_DEFLICK |
		    m_WIN0_CBR_DEFLICK;
		val =
		    v_WIN0_INTERLACE_READ(0) | v_WIN0_YRGB_DEFLICK(0) |
		    v_WIN0_CBR_DEFLICK(0);
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0, mask, val);

		mask =
		    m_WIN1_INTERLACE_READ | m_WIN1_YRGB_DEFLICK |
		    m_WIN1_CBR_DEFLICK;
		val =
		    v_WIN1_INTERLACE_READ(0) | v_WIN1_YRGB_DEFLICK(0) |
		    v_WIN1_CBR_DEFLICK(0);
		lcdc_msk_reg(lcdc_dev, WIN1_CTRL0, mask, val);

		mask = m_WIN2_INTERLACE_READ;
		val = v_WIN2_INTERLACE_READ(0);
		lcdc_msk_reg(lcdc_dev, WIN2_CTRL0, mask, val);

		mask = m_WIN3_INTERLACE_READ;
		val = v_WIN3_INTERLACE_READ(0);
		lcdc_msk_reg(lcdc_dev, WIN3_CTRL0, mask, val);

		mask = m_HWC_INTERLACE_READ;
		val = v_HWC_INTERLACE_READ(0);
		lcdc_msk_reg(lcdc_dev, HWC_CTRL0, mask, val);

		mask = m_DSP_LINE_FLAG0_NUM;
		val = v_DSP_LINE_FLAG0_NUM(vsync_len + upper_margin + y_res);
		lcdc_msk_reg(lcdc_dev, LINE_FLAG, mask, val);
	}
	rk3368_lcdc_post_cfg(lcdc_dev, screen);
	return 0;
}

static void rk3368_lcdc_bcsh_path_sel(struct lcdc_device *lcdc_dev)
{
	u32 bcsh_ctrl;

	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_OVERLAY_MODE,
		     v_OVERLAY_MODE(lcdc_dev->overlay_mode));
	if (lcdc_dev->overlay_mode == VOP_YUV_DOMAIN) {
		if (lcdc_dev->output_color == COLOR_YCBCR)	/* bypass */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_Y2R_EN | m_BCSH_R2Y_EN,
				     v_BCSH_Y2R_EN(0) | v_BCSH_R2Y_EN(0));
		else		/* YUV2RGB */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_Y2R_EN | m_BCSH_Y2R_CSC_MODE |
				     m_BCSH_R2Y_EN,
				     v_BCSH_Y2R_EN(1) |
				     v_BCSH_Y2R_CSC_MODE(VOP_Y2R_CSC_MPEG) |
				     v_BCSH_R2Y_EN(0));
	} else {		/* overlay_mode=VOP_RGB_DOMAIN */
		/* bypass  --need check,if bcsh close? */
		if (lcdc_dev->output_color == COLOR_RGB) {
			bcsh_ctrl = lcdc_readl(lcdc_dev, BCSH_CTRL);
			if (((bcsh_ctrl & m_BCSH_EN) == 1)/* ||
			    (lcdc_dev->bcsh.enable == 1)*/)/*bcsh enabled */
				lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
					     m_BCSH_R2Y_EN |
					     m_BCSH_Y2R_EN,
					     v_BCSH_R2Y_EN(1) |
					     v_BCSH_Y2R_EN(1));
			else
				lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
					     m_BCSH_R2Y_EN | m_BCSH_Y2R_EN,
					     v_BCSH_R2Y_EN(0) |
					     v_BCSH_Y2R_EN(0));
		} else		/* RGB2YUV */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_R2Y_EN |
				     m_BCSH_R2Y_CSC_MODE | m_BCSH_Y2R_EN,
				     v_BCSH_R2Y_EN(1) |
				     v_BCSH_R2Y_CSC_MODE(VOP_Y2R_CSC_MPEG) |
				     v_BCSH_Y2R_EN(0));
	}
}

int rk_lcdc_load_screen(vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk33_lcdc;
	struct rk_screen *screen;
	u32 mask = 0, val = 0;
	int face = 0;
	u16 dclk_ddr = 0;

    screen = kzalloc(sizeof(struct rk_screen), GFP_KERNEL);
    rk_fb_vidinfo_to_screen(vid, screen);
	lcdc_dev->overlay_mode = VOP_RGB_DOMAIN;
	switch (screen->face) {
	case OUT_P565:
		face = OUT_P565;
		mask = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
		    m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(0) |
		    v_DITHER_DOWN_SEL(1);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_P666:
		face = OUT_P666;
		mask = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
		    m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(1) |
		    v_DITHER_DOWN_SEL(1);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_D888_P565:
		face = OUT_P888;
		mask = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
		    m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(0) |
		    v_DITHER_DOWN_SEL(1);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_D888_P666:
		face = OUT_P888;
		mask = m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE |
		    m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(1) |
		    v_DITHER_DOWN_SEL(1);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_P888:
		face = OUT_P888;
		mask = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_YUV_420:
		/*yuv420 output prefer yuv domain overlay */
		face = OUT_YUV_420;
		dclk_ddr = 1;
		mask = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	default:
		dev_err(lcdc_dev->dev, "un supported interface!\n");
		break;
	}
	switch (screen->type) {
	case SCREEN_RGB:
		mask = m_RGB_OUT_EN;
		val = v_RGB_OUT_EN(1);
		lcdc_msk_reg(lcdc_dev, SYS_CTRL, mask, val);
		mask = m_RGB_LVDS_HSYNC_POL | m_RGB_LVDS_VSYNC_POL |
		    m_RGB_LVDS_DEN_POL | m_RGB_LVDS_DCLK_POL;
		val = v_RGB_LVDS_HSYNC_POL(screen->pin_hsync) |
		    v_RGB_LVDS_VSYNC_POL(screen->pin_vsync) |
		    v_RGB_LVDS_DEN_POL(screen->pin_den) |
		    v_RGB_LVDS_DCLK_POL(screen->pin_dclk);
		break;
	case SCREEN_LVDS:
		mask = m_RGB_OUT_EN;
		val = v_RGB_OUT_EN(1);
		lcdc_msk_reg(lcdc_dev, SYS_CTRL, mask, val);
		mask = m_RGB_LVDS_HSYNC_POL | m_RGB_LVDS_VSYNC_POL |
		    m_RGB_LVDS_DEN_POL | m_RGB_LVDS_DCLK_POL;
		val = v_RGB_LVDS_HSYNC_POL(screen->pin_hsync) |
		    v_RGB_LVDS_VSYNC_POL(screen->pin_vsync) |
		    v_RGB_LVDS_DEN_POL(screen->pin_den) |
		    v_RGB_LVDS_DCLK_POL(screen->pin_dclk);
		break;
	case SCREEN_HDMI:
		/*face = OUT_RGB_AAA;*/
		lcdc_dev->overlay_mode = VOP_YUV_DOMAIN;
		mask = m_HDMI_OUT_EN  | m_RGB_OUT_EN;
		val = v_HDMI_OUT_EN(1) | v_RGB_OUT_EN(0);
		lcdc_msk_reg(lcdc_dev, SYS_CTRL, mask, val);
		mask = m_HDMI_HSYNC_POL | m_HDMI_VSYNC_POL |
		    m_HDMI_DEN_POL | m_HDMI_DCLK_POL;
		val = v_HDMI_HSYNC_POL(screen->pin_hsync) |
		    v_HDMI_VSYNC_POL(screen->pin_vsync) |
		    v_HDMI_DEN_POL(screen->pin_den) |
		    v_HDMI_DCLK_POL(screen->pin_dclk);
		break;
	case SCREEN_MIPI:
		mask = m_MIPI_OUT_EN  | m_RGB_OUT_EN;
		val = v_MIPI_OUT_EN(1) | v_RGB_OUT_EN(0);
		lcdc_msk_reg(lcdc_dev, SYS_CTRL, mask, val);
		mask = m_MIPI_HSYNC_POL | m_MIPI_VSYNC_POL |
		    m_MIPI_DEN_POL | m_MIPI_DCLK_POL;
		val = v_MIPI_HSYNC_POL(screen->pin_hsync) |
		    v_MIPI_VSYNC_POL(screen->pin_vsync) |
		    v_MIPI_DEN_POL(screen->pin_den) |
		    v_MIPI_DCLK_POL(screen->pin_dclk);
		break;
	case SCREEN_DUAL_MIPI:
		mask = m_MIPI_OUT_EN | m_DOUB_CHANNEL_EN  |
			m_RGB_OUT_EN;
		val = v_MIPI_OUT_EN(1) | v_DOUB_CHANNEL_EN(1) |
			v_RGB_OUT_EN(0);
		lcdc_msk_reg(lcdc_dev, SYS_CTRL, mask, val);
		mask = m_MIPI_HSYNC_POL | m_MIPI_VSYNC_POL |
		    m_MIPI_DEN_POL | m_MIPI_DCLK_POL;
		val = v_MIPI_HSYNC_POL(screen->pin_hsync) |
		    v_MIPI_VSYNC_POL(screen->pin_vsync) |
		    v_MIPI_DEN_POL(screen->pin_den) |
		    v_MIPI_DCLK_POL(screen->pin_dclk);
		break;
	case SCREEN_EDP:
		face = OUT_P888;	/*RGB 888 output */

		mask = m_EDP_OUT_EN | m_RGB_OUT_EN;
		val = v_EDP_OUT_EN(1) | v_RGB_OUT_EN(0);
		lcdc_msk_reg(lcdc_dev, SYS_CTRL, mask, val);
		/*because edp have to sent aaa fmt */
		mask = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);

		mask |= m_EDP_HSYNC_POL | m_EDP_VSYNC_POL |
		    m_EDP_DEN_POL | m_EDP_DCLK_POL;
		val |= v_EDP_HSYNC_POL(screen->pin_hsync) |
		    v_EDP_VSYNC_POL(screen->pin_vsync) |
		    v_EDP_DEN_POL(screen->pin_den) |
		    v_EDP_DCLK_POL(screen->pin_dclk);
		break;
	}
	/*hsync vsync den dclk polo,dither */
	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
#ifndef CONFIG_RK_FPGA
	/*writel_relaxed(v, RK_GRF_VIRT + rk3368_GRF_SOC_CON7);
	move to  lvds driver*/
	/*GRF_SOC_CON7 bit[15]:0->dsi/lvds mode,1->ttl mode */
#endif
	mask = m_DSP_OUT_MODE | m_DSP_DCLK_DDR | m_DSP_BG_SWAP |
	    m_DSP_RB_SWAP | m_DSP_RG_SWAP | m_DSP_DELTA_SWAP |
	    m_DSP_DUMMY_SWAP | m_DSP_OUT_ZERO | m_DSP_BLANK_EN |
	    m_DSP_BLACK_EN | m_DSP_X_MIR_EN | m_DSP_Y_MIR_EN;
	val = v_DSP_OUT_MODE(face) | v_DSP_DCLK_DDR(dclk_ddr) |
	    v_DSP_BG_SWAP(screen->swap_gb) |
	    v_DSP_RB_SWAP(screen->swap_rb) |
	    v_DSP_RG_SWAP(screen->swap_rg) |
	    v_DSP_DELTA_SWAP(screen->swap_delta) |
	    v_DSP_DUMMY_SWAP(screen->swap_dumy) | v_DSP_OUT_ZERO(0) |
	    v_DSP_BLANK_EN(0) | v_DSP_BLACK_EN(0) |
	    v_DSP_X_MIR_EN(screen->x_mirror) |
	    v_DSP_Y_MIR_EN(screen->y_mirror);
	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, mask, val);
	/*BG color */
	mask = m_DSP_BG_BLUE | m_DSP_BG_GREEN | m_DSP_BG_RED;
	if (lcdc_dev->overlay_mode == VOP_YUV_DOMAIN)
		val = v_DSP_BG_BLUE(0x80) | v_DSP_BG_GREEN(0x10) |
			v_DSP_BG_RED(0x80);
	else
		val = v_DSP_BG_BLUE(0) | v_DSP_BG_GREEN(0) |
			v_DSP_BG_RED(0);
	lcdc_msk_reg(lcdc_dev, DSP_BG, mask, val);
	lcdc_dev->output_color = screen->color_mode;
	if (screen->dsp_lut == NULL)
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, m_DSP_LUT_EN,
			     v_DSP_LUT_EN(0));
	else
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, m_DSP_LUT_EN,
			     v_DSP_LUT_EN(1));
	if (screen->cabc_lut == NULL) {
		lcdc_msk_reg(lcdc_dev, CABC_CTRL0, m_CABC_EN,
			     v_CABC_EN(0));
	} else {
		lcdc_msk_reg(lcdc_dev, CABC_CTRL1, m_CABC_LUT_EN,
			     v_CABC_LUT_EN(1));
	}
	rk3368_lcdc_bcsh_path_sel(lcdc_dev);
	rk3368_config_timing(lcdc_dev, screen);

	if ((vid->screen_type == SCREEN_LVDS) ||
	    (vid->screen_type == SCREEN_DUAL_LVDS) ||
	    (vid->screen_type == SCREEN_RGB)) {
		rk31xx_lvds_enable(vid);
	} else if (vid->screen_type == SCREEN_EDP) {
		/*rk32_edp_enable(vid);*/
	} else if ((vid->screen_type == SCREEN_MIPI) ||
		   (vid->screen_type == SCREEN_DUAL_MIPI)) {
		/*rk32_dsi_sync();*/
	}

	return 0;
}


/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable)
{
	struct lcdc_device *lcdc_dev = &rk33_lcdc;
	if(enable == 0) {
		//rk32_dsi_enable();
		//rk32_dsi_sync();
	}
	else {
		//rk32_dsi_disable();
	}
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_STANDBY_EN,
		     v_STANDBY_EN(enable ? 1 : 0));
	lcdc_cfg_done(lcdc_dev);
}

#if defined(CONFIG_OF_LIBFDT)
static int rk32_lcdc_parse_dt(struct lcdc_device *lcdc_dev,
				     const void *blob)
{
	int order = FB0_WIN0_FB1_WIN1_FB2_WIN2;

	lcdc_dev->node  = fdt_path_offset(blob, "lcdc");
	if (lcdc_dev->node < 0) {
		debug("rk33 lcdc node is not found\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, lcdc_dev->node)) {
		debug("device lcdc is disabled\n");
		return -EPERM;
	}

	lcdc_dev->regs = fdtdec_get_reg(blob, lcdc_dev->node);
	order = fdtdec_get_int(blob, lcdc_dev->node,
			       "rockchip,fb-win-map", order);
	lcdc_dev->dft_win = order % 10;

	return 0;
}
#endif

#define CPU_AXI_QOS_PRIORITY_LEVEL(h, l)        ((((h) & 3) << 2) | ((l) & 3))

int rk_lcdc_init(int lcdc_id)
{
	struct lcdc_device *lcdc_dev = &rk33_lcdc;
	u32 msk, val;

	lcdc_dev->soc_type = gd->arch.chiptype;
	lcdc_dev->id = lcdc_id;
#ifdef CONFIG_OF_LIBFDT
	if (!lcdc_dev->node)
		rk32_lcdc_parse_dt(lcdc_dev, gd->fdt_blob);
#endif
	if (lcdc_dev->node <= 0) {
	    if (lcdc_dev->soc_type == CONFIG_RK3368) {
            lcdc_dev->regs = RKIO_VOP_PHYS;
	    }
	}
	lcdc_dev->regs = RKIO_VOP_PHYS;
	// set vop qos to highest priority
	/*
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(2, 2), 0xffad0408);//need check
	writel(CPU_AXI_QOS_PRIORITY_LEVEL(2, 2), 0xffad0008);
    */
    /*pmu grf need check*/
	//grf_writel(1<<16, GRF_IO_VSEL); /*LCDCIOdomain 3.3 Vvoltageselectio*/

	msk = m_AUTO_GATING_EN | m_STANDBY_EN |
		m_DMA_STOP | m_MMU_EN;
	val =  v_AUTO_GATING_EN(0) | v_STANDBY_EN(0) |
		v_DMA_STOP(0) | v_MMU_EN(0);
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, msk, val);
	msk = m_DSP_LAYER3_SEL | m_DSP_LAYER2_SEL|
		m_DSP_LAYER1_SEL | m_DSP_LAYER0_SEL;
	val = v_DSP_LAYER3_SEL(3) | v_DSP_LAYER2_SEL(2) |
		v_DSP_LAYER1_SEL(1) | v_DSP_LAYER0_SEL(0);
	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, msk, val);

	lcdc_writel(lcdc_dev, CABC_GAUSS_LINE0_0, 0x15110903);
	lcdc_writel(lcdc_dev, CABC_GAUSS_LINE0_1, 0x00030911);
	lcdc_writel(lcdc_dev, CABC_GAUSS_LINE1_0, 0x1a150b04);
	lcdc_writel(lcdc_dev, CABC_GAUSS_LINE1_1, 0x00040b15);
	lcdc_writel(lcdc_dev, CABC_GAUSS_LINE2_0, 0x15110903);
	lcdc_writel(lcdc_dev, CABC_GAUSS_LINE2_1, 0x00030911);

	lcdc_writel(lcdc_dev, FRC_LOWER01_0, 0x12844821);
	lcdc_writel(lcdc_dev, FRC_LOWER01_1, 0x21488412);
	lcdc_writel(lcdc_dev, FRC_LOWER10_0, 0xa55a9696);
	lcdc_writel(lcdc_dev, FRC_LOWER10_1, 0x5aa56969);
	lcdc_writel(lcdc_dev, FRC_LOWER11_0, 0xdeb77deb);
	lcdc_writel(lcdc_dev, FRC_LOWER11_1, 0xed7bb7de);
	lcdc_cfg_done(lcdc_dev);
    /*main_loop();*/
	return 0;
}

