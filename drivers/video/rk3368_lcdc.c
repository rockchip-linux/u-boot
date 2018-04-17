/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "rk3368_lcdc.h"

/*******************register definition**********************/
static struct lcdc_device rk33_lcdc;
extern struct rockchip_fb rockchip_fb;
// LCD en status
int lcdEnstatus=-1;
#define EARLY_TIME 500 /*us*/
extern int rk31xx_lvds_enable(vidinfo_t * vid);
extern int rk32_edp_enable(vidinfo_t * vid);
extern int rk32_mipi_enable(vidinfo_t * vid);
extern int rk32_dsi_enable(void);
extern int rk32_dsi_disable(void);

#ifndef pr_info
#define pr_info(args...)  debug(args)
#endif
#ifndef pr_err
#define  pr_err(args...)  debug(args)
#endif
static int rk3368_lcdc_csc_mode(struct lcdc_device *lcdc_dev,
				 struct fb_dsp_info *fb_info,
				 vidinfo_t *vid)
{
        u32 csc_mode = 0;
        struct rk_screen *screen = lcdc_dev->screen;

	if (lcdc_dev->overlay_mode == VOP_YUV_DOMAIN) {
		switch (fb_info->format) {
		case ARGB888:
		case RGB888:
		case RGB565:
			if ((screen->mode.xres < 1280) &&
			    (screen->mode.yres < 720)) {
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

static int rk3368_lcdc_calc_scl_fac(struct rk_lcdc_win *win, struct rk_screen *screen)
{
	u16 srcW = 0;
	u16 srcH = 0;
	u16 dstW = 0;
	u16 dstH = 0;
	u16 yrgb_srcW = 0;
	u16 yrgb_srcH = 0;
	u16 yrgb_dstW = 0;
	u16 yrgb_dstH = 0;
	u32 yrgb_vscalednmult = 0;
	u32 yrgb_xscl_factor = 0;
	u32 yrgb_yscl_factor = 0;
	u8 yrgb_vsd_bil_gt2 = 0;
	u8 yrgb_vsd_bil_gt4 = 0;

	u16 cbcr_srcW = 0;
	u16 cbcr_srcH = 0;
	u16 cbcr_dstW = 0;
	u16 cbcr_dstH = 0;
	u32 cbcr_vscalednmult = 0;
	u32 cbcr_xscl_factor = 0;
	u32 cbcr_yscl_factor = 0;
	u8 cbcr_vsd_bil_gt2 = 0;
	u8 cbcr_vsd_bil_gt4 = 0;
	u8 yuv_fmt = 0;

	srcW = win->area[0].xact;
	if ((screen->mode.vmode == FB_VMODE_INTERLACED) &&
	    (win->area[0].yact == 2 * win->area[0].ysize)) {
		srcH = win->area[0].yact / 2;
		yrgb_vsd_bil_gt2 = 1;
		cbcr_vsd_bil_gt2 = 1;
	} else {
		srcH = win->area[0].yact;
	}
	dstW = win->area[0].xsize;
	dstH = win->area[0].ysize;

	/*yrgb scl mode */
	yrgb_srcW = srcW;
	yrgb_srcH = srcH;
	yrgb_dstW = dstW;
	yrgb_dstH = dstH;
	if ((yrgb_dstW * 8 <= yrgb_srcW) || (yrgb_dstH * 8 <= yrgb_srcH)) {
		pr_err("ERROR: yrgb scale exceed 8,");
		pr_err("srcW=%d,srcH=%d,dstW=%d,dstH=%d\n",
		       yrgb_srcW, yrgb_srcH, yrgb_dstW, yrgb_dstH);
	}
	if (yrgb_srcW < yrgb_dstW)
		win->yrgb_hor_scl_mode = SCALE_UP;
	else if (yrgb_srcW > yrgb_dstW)
		win->yrgb_hor_scl_mode = SCALE_DOWN;
	else
		win->yrgb_hor_scl_mode = SCALE_NONE;

	if (yrgb_srcH < yrgb_dstH)
		win->yrgb_ver_scl_mode = SCALE_UP;
	else if (yrgb_srcH > yrgb_dstH)
		win->yrgb_ver_scl_mode = SCALE_DOWN;
	else
		win->yrgb_ver_scl_mode = SCALE_NONE;

	/*cbcr scl mode */
	switch (win->area[0].format) {
	case YUV422:
		cbcr_srcW = srcW / 2;
		cbcr_dstW = dstW;
		cbcr_srcH = srcH;
		cbcr_dstH = dstH;
		yuv_fmt = 1;
		break;
	case YUV420:
		cbcr_srcW = srcW / 2;
		cbcr_dstW = dstW;
		cbcr_srcH = srcH / 2;
		cbcr_dstH = dstH;
		yuv_fmt = 1;
		break;
	case YUV444:
		cbcr_srcW = srcW;
		cbcr_dstW = dstW;
		cbcr_srcH = srcH;
		cbcr_dstH = dstH;
		yuv_fmt = 1;
		break;
	default:
		cbcr_srcW = 0;
		cbcr_dstW = 0;
		cbcr_srcH = 0;
		cbcr_dstH = 0;
		yuv_fmt = 0;
		break;
	}
	if (yuv_fmt) {
		if ((cbcr_dstW * 8 <= cbcr_srcW) ||
		    (cbcr_dstH * 8 <= cbcr_srcH)) {
			pr_err("ERROR: cbcr scale exceed 8,");
			pr_err("srcW=%d,srcH=%d,dstW=%d,dstH=%d\n", cbcr_srcW,
			       cbcr_srcH, cbcr_dstW, cbcr_dstH);
		}
	}

	if (cbcr_srcW < cbcr_dstW)
		win->cbr_hor_scl_mode = SCALE_UP;
	else if (cbcr_srcW > cbcr_dstW)
		win->cbr_hor_scl_mode = SCALE_DOWN;
	else
		win->cbr_hor_scl_mode = SCALE_NONE;

	if (cbcr_srcH < cbcr_dstH)
		win->cbr_ver_scl_mode = SCALE_UP;
	else if (cbcr_srcH > cbcr_dstH)
		win->cbr_ver_scl_mode = SCALE_DOWN;
	else
		win->cbr_ver_scl_mode = SCALE_NONE;

	/*DBG(1, "srcW:%d>>srcH:%d>>dstW:%d>>dstH:%d>>\n"
	    "yrgb:src:W=%d>>H=%d,dst:W=%d>>H=%d,H_mode=%d,V_mode=%d\n"
	    "cbcr:src:W=%d>>H=%d,dst:W=%d>>H=%d,H_mode=%d,V_mode=%d\n", srcW,
	    srcH, dstW, dstH, yrgb_srcW, yrgb_srcH, yrgb_dstW, yrgb_dstH,
	    win->yrgb_hor_scl_mode, win->yrgb_ver_scl_mode, cbcr_srcW,
	    cbcr_srcH, cbcr_dstW, cbcr_dstH, win->cbr_hor_scl_mode,
	    win->cbr_ver_scl_mode);*/

	/*line buffer mode */
	if ((win->area[0].format == YUV422) ||
	    (win->area[0].format == YUV420)) {
		if (win->cbr_hor_scl_mode == SCALE_DOWN) {
			if ((cbcr_dstW > VOP_INPUT_MAX_WIDTH / 2) ||
			    (cbcr_dstW == 0))
				pr_err("ERROR cbcr_dstW = %d,exceeds 2048\n",
				       cbcr_dstW);
			else if (cbcr_dstW > 1280)
				win->win_lb_mode = LB_YUV_3840X5;
			else
				win->win_lb_mode = LB_YUV_2560X8;
		} else {	/*SCALE_UP or SCALE_NONE */
			if ((cbcr_srcW > VOP_INPUT_MAX_WIDTH / 2) ||
			    (cbcr_srcW == 0))
				pr_err("ERROR cbcr_srcW = %d,exceeds 2048\n",
				       cbcr_srcW);
			else if (cbcr_srcW > 1280)
				win->win_lb_mode = LB_YUV_3840X5;
			else
				win->win_lb_mode = LB_YUV_2560X8;
		}
	} else {
		if (win->yrgb_hor_scl_mode == SCALE_DOWN) {
			if ((yrgb_dstW > VOP_INPUT_MAX_WIDTH) ||
			    (yrgb_dstW == 0))
				pr_err("ERROR yrgb_dstW = %d\n", yrgb_dstW);
			else if (yrgb_dstW > 2560)
				win->win_lb_mode = LB_RGB_3840X2;
			else if (yrgb_dstW > 1920)
				win->win_lb_mode = LB_RGB_2560X4;
			else if (yrgb_dstW > 1280)
				win->win_lb_mode = LB_RGB_1920X5;
			else
				win->win_lb_mode = LB_RGB_1280X8;
		} else {	/*SCALE_UP or SCALE_NONE */
			if ((yrgb_srcW > VOP_INPUT_MAX_WIDTH) ||
			    (yrgb_srcW == 0))
				pr_err("ERROR yrgb_srcW = %d\n", yrgb_srcW);
			else if (yrgb_srcW > 2560)
				win->win_lb_mode = LB_RGB_3840X2;
			else if (yrgb_srcW > 1920)
				win->win_lb_mode = LB_RGB_2560X4;
			else if (yrgb_srcW > 1280)
				win->win_lb_mode = LB_RGB_1920X5;
			else
				win->win_lb_mode = LB_RGB_1280X8;
		}
	}
	debug("win->win_lb_mode = %d;\n", win->win_lb_mode);

	/*vsd/vsu scale ALGORITHM */
	win->yrgb_hsd_mode = SCALE_DOWN_BIL;	/*not to specify */
	win->cbr_hsd_mode = SCALE_DOWN_BIL;	/*not to specify */
	win->yrgb_vsd_mode = SCALE_DOWN_BIL;	/*not to specify */
	win->cbr_vsd_mode = SCALE_DOWN_BIL;	/*not to specify */
	switch (win->win_lb_mode) {
	case LB_YUV_3840X5:
	case LB_YUV_2560X8:
	case LB_RGB_1920X5:
	case LB_RGB_1280X8:
		win->yrgb_vsu_mode = SCALE_UP_BIC;
		win->cbr_vsu_mode = SCALE_UP_BIC;
		break;
	case LB_RGB_3840X2:
		if (win->yrgb_ver_scl_mode != SCALE_NONE)
			pr_err("ERROR : not allow yrgb ver scale\n");
		if (win->cbr_ver_scl_mode != SCALE_NONE)
			pr_err("ERROR : not allow cbcr ver scale\n");
		break;
	case LB_RGB_2560X4:
		win->yrgb_vsu_mode = SCALE_UP_BIL;
		win->cbr_vsu_mode = SCALE_UP_BIL;
		break;
	default:
		pr_info("%s:un supported win_lb_mode:%d\n",
			__func__, win->win_lb_mode);
		break;
	}
	if (win->mirror_en == 1) {
		win->yrgb_vsd_mode = SCALE_DOWN_BIL;
	}
	if (screen->mode.vmode == FB_VMODE_INTERLACED) {
		/*interlace mode must bill */
		win->yrgb_vsd_mode = SCALE_DOWN_BIL;
		win->cbr_vsd_mode = SCALE_DOWN_BIL;
	}
	if ((win->yrgb_ver_scl_mode == SCALE_DOWN) &&
	    (win->area[0].fbdc_en == 1)) {
		/*in this pattern,use bil mode,not support souble scd,
		use avg mode, support double scd, but aclk should be
		bigger than dclk,aclk>>dclk */
		if (yrgb_srcH >= 2 * yrgb_dstH) {
			pr_err("ERROR : fbdc mode,not support y scale down:");
			pr_err("srcH[%d] > 2 *dstH[%d]\n",
			       yrgb_srcH, yrgb_dstH);
		}
	}
	debug("yrgb:hsd=%d,vsd=%d,vsu=%d;cbcr:hsd=%d,vsd=%d,vsu=%d\n",
	    win->yrgb_hsd_mode, win->yrgb_vsd_mode, win->yrgb_vsu_mode,
	    win->cbr_hsd_mode, win->cbr_vsd_mode, win->cbr_vsu_mode);

	/*SCALE FACTOR */

	/*(1.1)YRGB HOR SCALE FACTOR */
	switch (win->yrgb_hor_scl_mode) {
	case SCALE_NONE:
		yrgb_xscl_factor = (1 << SCALE_FACTOR_DEFAULT_FIXPOINT_SHIFT);
		break;
	case SCALE_UP:
		yrgb_xscl_factor = GET_SCALE_FACTOR_BIC(yrgb_srcW, yrgb_dstW);
		break;
	case SCALE_DOWN:
		switch (win->yrgb_hsd_mode) {
		case SCALE_DOWN_BIL:
			yrgb_xscl_factor =
			    GET_SCALE_FACTOR_BILI_DN(yrgb_srcW, yrgb_dstW);
			break;
		case SCALE_DOWN_AVG:
			yrgb_xscl_factor =
			    GET_SCALE_FACTOR_AVRG(yrgb_srcW, yrgb_dstW);
			break;
		default:
			pr_info(
				"%s:un supported yrgb_hsd_mode:%d\n", __func__,
			       win->yrgb_hsd_mode);
			break;
		}
		break;
	default:
		pr_info("%s:un supported yrgb_hor_scl_mode:%d\n",
			__func__, win->yrgb_hor_scl_mode);
		break;
	}			/*win->yrgb_hor_scl_mode */

	/*(1.2)YRGB VER SCALE FACTOR */
	switch (win->yrgb_ver_scl_mode) {
	case SCALE_NONE:
		yrgb_yscl_factor = (1 << SCALE_FACTOR_DEFAULT_FIXPOINT_SHIFT);
		break;
	case SCALE_UP:
		switch (win->yrgb_vsu_mode) {
		case SCALE_UP_BIL:
			yrgb_yscl_factor =
			    GET_SCALE_FACTOR_BILI_UP(yrgb_srcH, yrgb_dstH);
			break;
		case SCALE_UP_BIC:
			if (yrgb_srcH < 3) {
				pr_err("yrgb_srcH should be");
				pr_err(" greater than 3 !!!\n");
			}
			yrgb_yscl_factor = GET_SCALE_FACTOR_BIC(yrgb_srcH,
								yrgb_dstH);
			break;
		default:
			pr_info("%s:un support yrgb_vsu_mode:%d\n",
				__func__, win->yrgb_vsu_mode);
			break;
		}
		break;
	case SCALE_DOWN:
		switch (win->yrgb_vsd_mode) {
		case SCALE_DOWN_BIL:
			yrgb_vscalednmult =
			    rk3368_get_hard_ware_vskiplines(yrgb_srcH,
							    yrgb_dstH);
			yrgb_yscl_factor =
			    GET_SCALE_FACTOR_BILI_DN_VSKIP(yrgb_srcH, yrgb_dstH,
							   yrgb_vscalednmult);
			if (yrgb_yscl_factor >= 0x2000) {
				pr_err("yrgb_yscl_factor should be ");
				pr_err("less than 0x2000,yrgb_yscl_factor=%4x;\n",
				       yrgb_yscl_factor);
			}
			if (yrgb_vscalednmult == 4) {
				yrgb_vsd_bil_gt4 = 1;
				yrgb_vsd_bil_gt2 = 0;
			} else if (yrgb_vscalednmult == 2) {
				yrgb_vsd_bil_gt4 = 0;
				yrgb_vsd_bil_gt2 = 1;
			} else {
				yrgb_vsd_bil_gt4 = 0;
				yrgb_vsd_bil_gt2 = 0;
			}
			break;
		case SCALE_DOWN_AVG:
			yrgb_yscl_factor = GET_SCALE_FACTOR_AVRG(yrgb_srcH,
								 yrgb_dstH);
			break;
		default:
			pr_info("%s:un support yrgb_vsd_mode:%d\n",
				__func__, win->yrgb_vsd_mode);
			break;
		}		/*win->yrgb_vsd_mode */
		break;
	default:
		pr_info("%s:un supported yrgb_ver_scl_mode:%d\n",
			__func__, win->yrgb_ver_scl_mode);
		break;
	}
	win->scale_yrgb_x = yrgb_xscl_factor;
	win->scale_yrgb_y = yrgb_yscl_factor;
	win->vsd_yrgb_gt4 = yrgb_vsd_bil_gt4;
	win->vsd_yrgb_gt2 = yrgb_vsd_bil_gt2;
	debug("yrgb:h_fac=%d, v_fac=%d,gt4=%d, gt2=%d\n", yrgb_xscl_factor,
	    yrgb_yscl_factor, yrgb_vsd_bil_gt4, yrgb_vsd_bil_gt2);

	/*(2.1)CBCR HOR SCALE FACTOR */
	switch (win->cbr_hor_scl_mode) {
	case SCALE_NONE:
		cbcr_xscl_factor = (1 << SCALE_FACTOR_DEFAULT_FIXPOINT_SHIFT);
		break;
	case SCALE_UP:
		cbcr_xscl_factor = GET_SCALE_FACTOR_BIC(cbcr_srcW, cbcr_dstW);
		break;
	case SCALE_DOWN:
		switch (win->cbr_hsd_mode) {
		case SCALE_DOWN_BIL:
			cbcr_xscl_factor =
			    GET_SCALE_FACTOR_BILI_DN(cbcr_srcW, cbcr_dstW);
			break;
		case SCALE_DOWN_AVG:
			cbcr_xscl_factor =
			    GET_SCALE_FACTOR_AVRG(cbcr_srcW, cbcr_dstW);
			break;
		default:
			pr_info("%s:un support cbr_hsd_mode:%d\n",
				__func__, win->cbr_hsd_mode);
			break;
		}
		break;
	default:
		pr_info("%s:un supported cbr_hor_scl_mode:%d\n",
			__func__, win->cbr_hor_scl_mode);
		break;
	}			/*win->cbr_hor_scl_mode */

	/*(2.2)CBCR VER SCALE FACTOR */
	switch (win->cbr_ver_scl_mode) {
	case SCALE_NONE:
		cbcr_yscl_factor = (1 << SCALE_FACTOR_DEFAULT_FIXPOINT_SHIFT);
		break;
	case SCALE_UP:
		switch (win->cbr_vsu_mode) {
		case SCALE_UP_BIL:
			cbcr_yscl_factor =
			    GET_SCALE_FACTOR_BILI_UP(cbcr_srcH, cbcr_dstH);
			break;
		case SCALE_UP_BIC:
			if (cbcr_srcH < 3) {
				pr_err("cbcr_srcH should be ");
				pr_err("greater than 3 !!!\n");
			}
			cbcr_yscl_factor = GET_SCALE_FACTOR_BIC(cbcr_srcH,
								cbcr_dstH);
			break;
		default:
			pr_info("%s:un support cbr_vsu_mode:%d\n",
				__func__, win->cbr_vsu_mode);
			break;
		}
		break;
	case SCALE_DOWN:
		switch (win->cbr_vsd_mode) {
		case SCALE_DOWN_BIL:
			cbcr_vscalednmult =
			    rk3368_get_hard_ware_vskiplines(cbcr_srcH,
							    cbcr_dstH);
			cbcr_yscl_factor =
			    GET_SCALE_FACTOR_BILI_DN_VSKIP(cbcr_srcH, cbcr_dstH,
							   cbcr_vscalednmult);
			if (cbcr_yscl_factor >= 0x2000) {
				pr_err("cbcr_yscl_factor should be less ");
				pr_err("than 0x2000,cbcr_yscl_factor=%4x;\n",
				       cbcr_yscl_factor);
			}

			if (cbcr_vscalednmult == 4) {
				cbcr_vsd_bil_gt4 = 1;
				cbcr_vsd_bil_gt2 = 0;
			} else if (cbcr_vscalednmult == 2) {
				cbcr_vsd_bil_gt4 = 0;
				cbcr_vsd_bil_gt2 = 1;
			} else {
				cbcr_vsd_bil_gt4 = 0;
				cbcr_vsd_bil_gt2 = 0;
			}
			break;
		case SCALE_DOWN_AVG:
			cbcr_yscl_factor = GET_SCALE_FACTOR_AVRG(cbcr_srcH,
								 cbcr_dstH);
			break;
		default:
			pr_info("%s:un support cbr_vsd_mode:%d\n",
				__func__, win->cbr_vsd_mode);
			break;
		}
		break;
	default:
		pr_info("%s:un supported cbr_ver_scl_mode:%d\n",
			__func__, win->cbr_ver_scl_mode);
		break;
	}
	win->scale_cbcr_x = cbcr_xscl_factor;
	win->scale_cbcr_y = cbcr_yscl_factor;
	win->vsd_cbr_gt4 = cbcr_vsd_bil_gt4;
	win->vsd_cbr_gt2 = cbcr_vsd_bil_gt2;

	debug("cbcr:h_fac=%d,v_fac=%d,gt4=%d,gt2=%d\n", cbcr_xscl_factor,
	    cbcr_yscl_factor, cbcr_vsd_bil_gt4, cbcr_vsd_bil_gt2);
	return 0;
}


static int rk3368_win_0_1_reg_update(struct lcdc_device *lcdc_dev,
                                            struct rk_lcdc_win *win,
                                            int win_id)
{
	unsigned int mask, val, off;

	off = win_id * 0x40;
	if (win->state == 1) {
		mask = m_WIN0_EN | m_WIN0_DATA_FMT | m_WIN0_FMT_10 |
		    m_WIN0_LB_MODE | m_WIN0_RB_SWAP | m_WIN0_X_MIRROR |
		    m_WIN0_Y_MIRROR | m_WIN0_CSC_MODE |m_WIN0_UV_SWAP;
		val = v_WIN0_EN(win->state) |
		    v_WIN0_DATA_FMT(win->area[0].format) |
		    v_WIN0_FMT_10(win->fmt_10) |
		    v_WIN0_LB_MODE(win->win_lb_mode) |
		    v_WIN0_RB_SWAP(0) |
		    v_WIN0_X_MIRROR(win->mirror_en) |
		    v_WIN0_Y_MIRROR(win->mirror_en) |
		    v_WIN0_CSC_MODE(win->csc_mode) |
		    v_WIN0_UV_SWAP(0);
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, mask, val);

		mask = m_WIN0_BIC_COE_SEL |
		    m_WIN0_VSD_YRGB_GT4 | m_WIN0_VSD_YRGB_GT2 |
		    m_WIN0_VSD_CBR_GT4 | m_WIN0_VSD_CBR_GT2 |
		    m_WIN0_YRGB_HOR_SCL_MODE | m_WIN0_YRGB_VER_SCL_MODE |
		    m_WIN0_YRGB_HSD_MODE | m_WIN0_YRGB_VSU_MODE |
		    m_WIN0_YRGB_VSD_MODE | m_WIN0_CBR_HOR_SCL_MODE |
		    m_WIN0_CBR_VER_SCL_MODE | m_WIN0_CBR_HSD_MODE |
		    m_WIN0_CBR_VSU_MODE | m_WIN0_CBR_VSD_MODE;
		val = v_WIN0_BIC_COE_SEL(win->bic_coe_el) |
		    v_WIN0_VSD_YRGB_GT4(win->vsd_yrgb_gt4) |
		    v_WIN0_VSD_YRGB_GT2(win->vsd_yrgb_gt2) |
		    v_WIN0_VSD_CBR_GT4(win->vsd_cbr_gt4) |
		    v_WIN0_VSD_CBR_GT2(win->vsd_cbr_gt2) |
		    v_WIN0_YRGB_HOR_SCL_MODE(win->yrgb_hor_scl_mode) |
		    v_WIN0_YRGB_VER_SCL_MODE(win->yrgb_ver_scl_mode) |
		    v_WIN0_YRGB_HSD_MODE(win->yrgb_hsd_mode) |
		    v_WIN0_YRGB_VSU_MODE(win->yrgb_vsu_mode) |
		    v_WIN0_YRGB_VSD_MODE(win->yrgb_vsd_mode) |
		    v_WIN0_CBR_HOR_SCL_MODE(win->cbr_hor_scl_mode) |
		    v_WIN0_CBR_VER_SCL_MODE(win->cbr_ver_scl_mode) |
		    v_WIN0_CBR_HSD_MODE(win->cbr_hsd_mode) |
		    v_WIN0_CBR_VSU_MODE(win->cbr_vsu_mode) |
		    v_WIN0_CBR_VSD_MODE(win->cbr_vsd_mode);
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL1 + off, mask, val);
		val = v_WIN0_VIR_STRIDE(win->area[0].y_vir_stride);
		lcdc_writel(lcdc_dev, WIN0_VIR + off, val);

		val = v_WIN0_ACT_WIDTH(win->area[0].xact) |
		    v_WIN0_ACT_HEIGHT(win->area[0].yact);
		lcdc_writel(lcdc_dev, WIN0_ACT_INFO + off, val);

		val = v_WIN0_DSP_WIDTH(win->area[0].xsize) |
		    v_WIN0_DSP_HEIGHT(win->area[0].ysize);
		lcdc_writel(lcdc_dev, WIN0_DSP_INFO + off, val);

		val = v_WIN0_DSP_XST(win->area[0].dsp_stx) |
		    v_WIN0_DSP_YST(win->area[0].dsp_sty);
		lcdc_writel(lcdc_dev, WIN0_DSP_ST + off, val);

		val = v_WIN0_HS_FACTOR_YRGB(win->scale_yrgb_x) |
		    v_WIN0_VS_FACTOR_YRGB(win->scale_yrgb_y);
		lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_YRGB + off, val);

		val = v_WIN0_HS_FACTOR_CBR(win->scale_cbcr_x) |
		    v_WIN0_VS_FACTOR_CBR(win->scale_cbcr_y);
		lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_CBR + off, val);

		mask = m_WIN0_SRC_ALPHA_EN;
		val = v_WIN0_SRC_ALPHA_EN(0);
		lcdc_msk_reg(lcdc_dev, WIN0_SRC_ALPHA_CTRL + off,
			     mask, val);
	} else {
		mask = m_WIN0_EN;
		val = v_WIN0_EN(win->state);
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0 + off, mask, val);
	}
	return 0;
}

static int dsp_x_pos(int mirror_en, struct rk_screen *screen,
		     struct rk_lcdc_win_area *area)
{
	int pos;

	if (screen->x_mirror && mirror_en)
		pr_err("not support both win and global mirror\n");

	if ((!mirror_en) && (!screen->x_mirror))
		pos = area->xpos + screen->mode.left_margin +
			screen->mode.hsync_len;
	else
		pos = screen->mode.xres - area->xpos -
			area->xsize + screen->mode.left_margin +
			screen->mode.hsync_len;

	return pos;
}

static int dsp_y_pos(int mirror_en, struct rk_screen *screen,
		     struct rk_lcdc_win_area *area)
{
	int pos = 0;

	if (screen->y_mirror && mirror_en)
		pr_err("not support both win and global mirror\n");
	if (screen->mode.vmode == FB_VMODE_NONINTERLACED) {
		if ((!mirror_en) && (!screen->y_mirror))
			pos = area->ypos + screen->mode.upper_margin +
				screen->mode.vsync_len;
		else
			pos = screen->mode.yres - area->ypos -
				area->ysize + screen->mode.upper_margin +
				screen->mode.vsync_len;
	} else if (screen->mode.vmode == FB_VMODE_INTERLACED) {
		pos = area->ypos / 2 + screen->mode.upper_margin +
			screen->mode.vsync_len;
		area->ysize /= 2;
	}

	return pos;
}

static int win0_set_par(struct lcdc_device *lcdc_dev,
			   struct fb_dsp_info *fb_info,
			   vidinfo_t *vid)
{
	struct rk_lcdc_win win;
	struct rk_screen *screen = lcdc_dev->screen;

	memset(&win, 0, sizeof(struct rk_lcdc_win));
	rk_fb_vidinfo_to_win(fb_info, &win);
	win.csc_mode = rk3368_lcdc_csc_mode(lcdc_dev, fb_info, vid);
	if (fb_info->yaddr)
		win.state = 1;
	else
		win.state = 0;
	win.mirror_en = 0;
	win.area[0].dsp_stx = dsp_x_pos(win.mirror_en, screen, win.area);
	win.area[0].dsp_sty = dsp_y_pos(win.mirror_en, screen, win.area);

	rk3368_lcdc_calc_scl_fac(&win, screen);
	
	switch (fb_info->format) {
	case ARGB888:
		win.area[0].y_vir_stride = v_ARGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB888:
		win.area[0].y_vir_stride = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		win.area[0].y_vir_stride = v_RGB565_VIRWIDTH(fb_info->xvir);
		break;
	default:
		win.area[0].y_vir_stride = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	}
	rk3368_win_0_1_reg_update(lcdc_dev, &win, fb_info->layer_id);
	lcdc_writel(lcdc_dev, WIN0_YRGB_MST, fb_info->yaddr);

	return 0;
}

void rk_lcdc_set_par(struct fb_dsp_info *fb_info, vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk33_lcdc;

	fb_info->layer_id = lcdc_dev->dft_win;
	
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
	u32 frame_time;
	h_total = hsync_len + left_margin + x_res + right_margin;
	v_total = vsync_len + upper_margin + y_res + lower_margin;
	frame_time = 1000 * v_total * h_total / (screen->mode.pixclock / 1000);
	/*need check,depend on user define*/
        screen->overscan.left = 100;
        screen->overscan.right = 100;
        screen->overscan.top = 100;
        screen->overscan.bottom = 100;
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
		    v_WIN0_INTERLACE_READ(1) | v_WIN0_YRGB_DEFLICK(0) |
		    v_WIN0_CBR_DEFLICK(0);
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

		mask = m_DSP_LINE_FLAG0_NUM | m_DSP_LINE_FLAG1_NUM;
		val =
		    v_DSP_LINE_FLAG0_NUM(vact_end_f1) |
		    v_DSP_LINE_FLAG1_NUM(vact_end_f1 -
					 EARLY_TIME * v_total / frame_time);
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

		mask = m_DSP_LINE_FLAG0_NUM | m_DSP_LINE_FLAG1_NUM;
		val = v_DSP_LINE_FLAG0_NUM(vsync_len + upper_margin + y_res) |
			v_DSP_LINE_FLAG1_NUM(vsync_len + upper_margin + y_res -
					     EARLY_TIME * v_total / frame_time);
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
	u8 dclk_inv = vid->dclk_inv;

        screen = lcdc_dev->screen;
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
		mask = m_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_YUV_420:
		/*yuv420 output prefer yuv domain overlay */
		face = OUT_YUV_420;
		dclk_ddr = 1;
		mask = m_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_S888:
		face = OUT_S888;
		mask = m_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_S888DUMY:
		face = OUT_S888DUMY;
		mask = m_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL1, mask, val);
		break;
	case OUT_CCIR656:
		if (screen->color_mode == COLOR_RGB)
			lcdc_dev->overlay_mode = VOP_RGB_DOMAIN;
		else
			lcdc_dev->overlay_mode = VOP_YUV_DOMAIN;
		face = OUT_CCIR656_MODE_0;
		mask = m_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0);
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
		grf_writel(GRF_DCLK_INV(dclk_inv), RK3366_GRF_SOC_CON6);
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
		grf_writel(GRF_DCLK_INV(dclk_inv), RK3366_GRF_SOC_CON6);
		break;
	case SCREEN_HDMI:
		/*face = OUT_RGB_AAA;*/
		if (screen->color_mode == COLOR_RGB)
                        lcdc_dev->overlay_mode = VOP_RGB_DOMAIN;
                else
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

		mask = m_EDP_HSYNC_POL | m_EDP_VSYNC_POL |
		    m_EDP_DEN_POL | m_EDP_DCLK_POL;
		val = v_EDP_HSYNC_POL(screen->pin_hsync) |
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
		rk32_edp_enable(vid);
	} else if ((vid->screen_type == SCREEN_MIPI) ||
		   (vid->screen_type == SCREEN_DUAL_MIPI)) {
#if defined(CONFIG_RK32_DSI)
		rk32_mipi_enable(vid);
#endif
	}
	return 0;
}


/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable)
{
	struct lcdc_device *lcdc_dev = &rk33_lcdc;

#if defined(CONFIG_RK32_DSI)
	if (((panel_info.screen_type == SCREEN_MIPI) ||
		(panel_info.screen_type == SCREEN_DUAL_MIPI))) {
		if (enable == 0) {
			//the lcd power enable if lcd power seted disable 
			if(lcdEnstatus)
				rk_fb_pwr_enable(&rockchip_fb);
			rk32_dsi_enable();
			
		} else if (enable == 1) {
			rk32_dsi_disable();
			//when lcd standby power off lcd 
			rk_fb_pwr_disable(&rockchip_fb);
			lcdEnstatus=1;
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

	lcdc_dev->node  = fdt_path_offset(blob, "lcdc");
	if (lcdc_dev->node < 0) {
		debug("rk33 lcdc node is not found\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, lcdc_dev->node)) {
		debug("device lcdc is disabled\n");
		return -EPERM;
	}

	lcdc_dev->regs = fdtdec_get_addr_size_auto_noparent(blob,
							    lcdc_dev->node,
							    "reg", 0, NULL);
	order = fdtdec_get_int(blob, lcdc_dev->node,
			       "rockchip,fb-win-map", order);
	lcdc_dev->dft_win = order % 10;

	return 0;
}
#endif

int rk3368_lcdc_read_def_cfg(struct lcdc_device *lcdc_dev)
{
        int reg = 0;

        for (reg = 0; reg < REG_LEN; reg += 4)
                lcdc_readl_backup(lcdc_dev, reg);

        return 0;
}


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
        lcdc_dev->screen = kzalloc(sizeof(struct rk_screen), GFP_KERNEL);
	if (lcdc_dev->node <= 0) {
	    if (lcdc_dev->soc_type == CONFIG_RK3368) {
            lcdc_dev->regs = RKIO_VOP_PHYS;
	    }
	}
	lcdc_dev->regs = RKIO_VOP_PHYS;

        rk3368_lcdc_read_def_cfg(lcdc_dev);

	//grf_writel(1<<16, GRF_IO_VSEL); /*LCDCIOdomain 3.3 Vvoltageselectio*/
        writel(0x20 << 16, VOP_PMU_GRF_BASE + VOP_PMUGRF_SOC_CON0);

	msk = m_AUTO_GATING_EN | m_STANDBY_EN |
		m_DMA_STOP | m_MMU_EN;
	val =  v_AUTO_GATING_EN(0) | v_STANDBY_EN(0) |
		v_DMA_STOP(0) | v_MMU_EN(0);
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, msk, val);
	msk = m_DSP_LAYER3_SEL | m_DSP_LAYER2_SEL |
		m_DSP_LAYER1_SEL | m_DSP_LAYER0_SEL |
		m_DITHER_UP_EN;
	val = v_DSP_LAYER3_SEL(3) | v_DSP_LAYER2_SEL(2) |
		v_DSP_LAYER1_SEL(1) | v_DSP_LAYER0_SEL(0) |
		v_DITHER_UP_EN(1);
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

