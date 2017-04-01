/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
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
#ifndef pr_info
#define pr_info(args...)  debug(args)
#endif
#ifndef pr_err
#define  pr_err(args...)  debug(args)
#endif

static struct lcdc_device rk32_lcdc;


static int inline lvds_writel(uint32 offset, uint32 val)
{
	writel(val, lvds_regs + offset);
	writel(val, lvds_regs + offset + 0x100);

	return 0;
}

#if 0
static int rk32_lvds_disable(void)
{
	grf_writel(0x80008000, GRF_SOC_CON7);

	writel(0x00, lvds_regs + LVDS_CFG_REG_21); /*disable tx*/
	writel(0xff, lvds_regs + LVDS_CFG_REG_c); /*disable pll*/
	return 0;
}
#endif

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
		val = LVDS_TTL_EN | LVDS_CH0_EN | LVDS_CH1_EN;

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

static int rk3288_lcdc_calc_scl_fac(struct rk_lcdc_win *win, struct rk_screen *screen)
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
			    getHardWareVSkipLines(yrgb_srcH,
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
			    getHardWareVSkipLines(cbcr_srcH,
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

static int rk3288_win_0_1_reg_update(struct lcdc_device *lcdc_dev,
                                            struct rk_lcdc_win *win,
                                            int win_id)
{
	unsigned int mask, val, off;

	off = win_id * 0x40;

	if(win->win_lb_mode == 5)
		win->win_lb_mode = 4;
	
	if (win->state == 1) {
		mask =  m_WIN0_EN | m_WIN0_DATA_FMT | m_WIN0_FMT_10 |
			m_WIN0_LB_MODE | m_WIN0_RB_SWAP | m_WIN0_UV_SWAP;
		val  =  v_WIN0_EN(win->state) |
			v_WIN0_DATA_FMT(win->area[0].format) |
			v_WIN0_FMT_10(win->fmt_10) | 
			v_WIN0_LB_MODE(win->win_lb_mode) | 
			v_WIN0_RB_SWAP(win->rb_swap) |
			v_WIN0_UV_SWAP(0);
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL0+off, mask,val);	
	
		mask =	m_WIN0_BIC_COE_SEL |
			m_WIN0_VSD_YRGB_GT4 | m_WIN0_VSD_YRGB_GT2 |
			m_WIN0_VSD_CBR_GT4 | m_WIN0_VSD_CBR_GT2 |
			m_WIN0_YRGB_HOR_SCL_MODE | m_WIN0_YRGB_VER_SCL_MODE |
			m_WIN0_YRGB_HSD_MODE | m_WIN0_YRGB_VSU_MODE |
			m_WIN0_YRGB_VSD_MODE | m_WIN0_CBR_HOR_SCL_MODE |
			m_WIN0_CBR_VER_SCL_MODE | m_WIN0_CBR_HSD_MODE |
			m_WIN0_CBR_VSU_MODE | m_WIN0_CBR_VSD_MODE;
		val =	v_WIN0_BIC_COE_SEL(win->bic_coe_el) |
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
		lcdc_msk_reg(lcdc_dev, WIN0_CTRL1+off, mask,val);
	
		val =	v_WIN0_VIR_STRIDE(win->area[0].y_vir_stride);
		lcdc_writel(lcdc_dev, WIN0_VIR+off, val);	
		/*lcdc_writel(lcdc_dev, WIN0_YRGB_MST+off, win->area[0].y_addr); 
		lcdc_writel(lcdc_dev, WIN0_CBR_MST+off, win->area[0].uv_addr);*/
		val =	v_WIN0_ACT_WIDTH(win->area[0].xact) |
			v_WIN0_ACT_HEIGHT(win->area[0].yact);
		lcdc_writel(lcdc_dev, WIN0_ACT_INFO+off, val); 
	
		val =	v_WIN0_DSP_WIDTH(win->area[0].xsize) |
			v_WIN0_DSP_HEIGHT(win->area[0].ysize);
		lcdc_writel(lcdc_dev, WIN0_DSP_INFO+off, val); 
	
		val =	v_WIN0_DSP_XST(win->area[0].dsp_stx) |
			v_WIN0_DSP_YST(win->area[0].dsp_sty);
		lcdc_writel(lcdc_dev, WIN0_DSP_ST+off, val); 
	
		val =	v_WIN0_HS_FACTOR_YRGB(win->scale_yrgb_x) |
			v_WIN0_VS_FACTOR_YRGB(win->scale_yrgb_y);
		lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_YRGB+off, val); 
	
		val =	v_WIN0_HS_FACTOR_CBR(win->scale_cbcr_x) |
			v_WIN0_VS_FACTOR_CBR(win->scale_cbcr_y);
		lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_CBR+off, val); 

		mask = m_WIN0_SRC_ALPHA_EN;
		val = v_WIN0_SRC_ALPHA_EN(0);
		lcdc_msk_reg(lcdc_dev,WIN0_SRC_ALPHA_CTRL+off,mask,val);				
	
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
	u32 y_addr = fb_info->yaddr;

	memset(&win, 0, sizeof(struct rk_lcdc_win));
	rk_fb_vidinfo_to_win(fb_info, &win);
	//win.csc_mode = rk3368_lcdc_csc_mode(lcdc_dev, fb_info, vid);
	if (fb_info->yaddr)
		win.state = 1;
	else
		win.state = 0;
	win.mirror_en = 0;
	win.area[0].dsp_stx = dsp_x_pos(win.mirror_en, screen, win.area);
	win.area[0].dsp_sty = dsp_y_pos(win.mirror_en, screen, win.area);
	rk3288_lcdc_calc_scl_fac(&win, screen);

	switch (fb_info->format) {
	case ARGB888:
		win.area[0].y_vir_stride = v_ARGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB888:
		win.rb_swap = 1;
		win.area[0].y_vir_stride = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		win.area[0].y_vir_stride = v_RGB565_VIRWIDTH(fb_info->xvir);
		break;
	default:
		win.area[0].y_vir_stride = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	}
	rk3288_win_0_1_reg_update(lcdc_dev, &win, fb_info->layer_id);
	if (screen->y_mirror)
		y_addr += win.area[0].y_vir_stride * 4 * win.area[0].yact;
	lcdc_writel(lcdc_dev, WIN0_YRGB_MST, y_addr);
#if 0	
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
#endif
	return 0;
}

void rk_lcdc_set_par(struct fb_dsp_info *fb_info, vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk32_lcdc;
	struct rk_screen *screen = lcdc_dev->screen;
	u16 post_dsp_vact_st,post_dsp_vact_end, v_total;
	u32 msk, val;

	if (fb_info->ymirror)
		screen->y_mirror = !screen->y_mirror;
	msk = m_DSP_Y_MIR_EN;
	val = v_DSP_Y_MIR_EN(screen->y_mirror);

	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);

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
	struct rk_screen *screen =  lcdc_dev->screen;
	int face = 0;
	u32 msk, val;

	rk_fb_vidinfo_to_screen(vid, screen);
	if (vid->screen_type == SCREEN_MIPI ||
	    vid->screen_type == SCREEN_DUAL_MIPI) {
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
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		      m_PRE_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(1) |
		      v_PRE_DITHER_DOWN_EN(1);
		break;
	case OUT_P101010:
		face = OUT_P101010;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		      m_PRE_DITHER_DOWN_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(1) |
		    v_PRE_DITHER_DOWN_EN(0);
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
		face = OUT_P101010;
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

	msk = m_DSP_LINE_FLAG_NUM | m_LINE_FLAG_INTR_EN;
	val = v_DSP_LINE_FLAG_NUM(vid->vl_vspw + vid->vl_vbpd + vid->vl_row) |
	      v_LINE_FLAG_INTR_EN(0);
	lcdc_msk_reg(lcdc_dev, INTR_CTRL0, msk, val);
	lcdc_cfg_done(lcdc_dev);
	if ((vid->screen_type == SCREEN_LVDS) ||
	    (vid->screen_type == SCREEN_DUAL_LVDS) ||
	    (vid->screen_type == SCREEN_RGB)) {
		rk32_lvds_en(vid);
	} else if (vid->screen_type == SCREEN_EDP) {
		rk32_edp_enable(vid);
	} else if ((vid->screen_type == SCREEN_MIPI) ||
		   (vid->screen_type == SCREEN_DUAL_MIPI)) {
		rk32_mipi_enable(vid);
		rk32_dsi_sync();
	}

	return 0;
}


/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable)
{
	struct lcdc_device *lcdc_dev = &rk32_lcdc;
#if defined(CONFIG_RK32_DSI)
	if (((panel_info.screen_type == SCREEN_MIPI) ||
			   (panel_info.screen_type == SCREEN_DUAL_MIPI))) {
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

