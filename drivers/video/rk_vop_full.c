/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "rk_vop_full.h"

/*******************register definition**********************/
static struct vop_device rk_vop_full;
#ifndef pr_info
#define pr_info(args...)  debug(args)
#endif
#ifndef pr_err
#define  pr_err(args...)  debug(args)
#endif

static int vop_vop_calc_scl_fac(struct rk_lcdc_win *win,
				struct rk_screen *screen)
{
	u16 srcW;
	u16 srcH;
	u16 dstW;
	u16 dstH;
	u16 yrgb_srcW;
	u16 yrgb_srcH;
	u16 yrgb_dstW;
	u16 yrgb_dstH;
	u32 yrgb_vscalednmult;
	u32 yrgb_xscl_factor = 0;
	u32 yrgb_yscl_factor = 0;
	u8 yrgb_vsd_bil_gt2 = 0;
	u8 yrgb_vsd_bil_gt4 = 0;

	u16 cbcr_srcW;
	u16 cbcr_srcH;
	u16 cbcr_dstW;
	u16 cbcr_dstH;
	u32 cbcr_vscalednmult;
	u32 cbcr_xscl_factor = 0;
	u32 cbcr_yscl_factor = 0;
	u8 cbcr_vsd_bil_gt2 = 0;
	u8 cbcr_vsd_bil_gt4 = 0;
	u8 yuv_fmt = 0;

	srcW = win->area[0].xact;
	if ((screen->mode.vmode & FB_VMODE_INTERLACED) &&
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

	/* line buffer mode */
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
		} else {	/* SCALE_UP or SCALE_NONE */
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
		} else {	/* SCALE_UP or SCALE_NONE */
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

	/* vsd/vsu scale ALGORITHM */
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

	if (win->mirror_en == 1)
		win->yrgb_vsd_mode = SCALE_DOWN_BIL;
	if (screen->mode.vmode & FB_VMODE_INTERLACED) {
		/* interlace mode must bill */
		win->yrgb_vsd_mode = SCALE_DOWN_BIL;
		win->cbr_vsd_mode = SCALE_DOWN_BIL;
	}
	if ((win->yrgb_ver_scl_mode == SCALE_DOWN) &&
	    (win->area[0].fbdc_en == 1)) {
		/* in this pattern,use bil mode,not support souble scd,
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

	/* SCALE FACTOR */

	/* (1.1)YRGB HOR SCALE FACTOR */
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
			pr_info("%s:un supported yrgb_hsd_mode:%d\n", __func__,
				win->yrgb_hsd_mode);
			break;
		}
		break;
	default:
		pr_info("%s:un supported yrgb_hor_scl_mode:%d\n",
			__func__, win->yrgb_hor_scl_mode);
		break;
	}

	/* (1.2)YRGB VER SCALE FACTOR */
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
			    vop_get_hard_ware_vskiplines(yrgb_srcH, yrgb_dstH);
			yrgb_yscl_factor =
			    GET_SCALE_FACTOR_BILI_DN_VSKIP(yrgb_srcH, yrgb_dstH,
							   yrgb_vscalednmult);
			if (yrgb_yscl_factor >= 0x2000) {
				pr_err("yrgb_yscl_factor should less 0x2000");
				pr_err("yrgb_yscl_factor=%4x;\n",
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
	debug("yrgb:h_fac=%d, V_fac=%d,gt4=%d, gt2=%d\n", yrgb_xscl_factor,
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

	/* (2.2)CBCR VER SCALE FACTOR */
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
			    vop_get_hard_ware_vskiplines(cbcr_srcH, cbcr_dstH);
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

static int vop_axi_gather_cfg(struct vop_device *vop_dev,
			      struct rk_lcdc_win *win, int win_id)
{
	u64 val;
	u16 yrgb_gather_num = 3;
	u16 cbcr_gather_num = 1;

	switch (win->area[0].format) {
	case ARGB888:
		yrgb_gather_num = 3;
		break;
	case RGB888:
	case RGB565:
		yrgb_gather_num = 2;
		break;
	case YUV444:
	case YUV422:
	case YUV420:
		yrgb_gather_num = 1;
		cbcr_gather_num = 2;
		break;
	default:
		dev_err(vop_dev->driver.dev, "%s:un supported format!\n",
			__func__);
		return -EINVAL;
	}

	if ((win_id == 0) || (win_id == 1)) {
		val = V_WIN0_YRGB_AXI_GATHER_EN(1) |
			V_WIN0_CBR_AXI_GATHER_EN(1) |
			V_WIN0_YRGB_AXI_GATHER_NUM(yrgb_gather_num) |
			V_WIN0_CBR_AXI_GATHER_NUM(cbcr_gather_num);
		vop_msk_reg(vop_dev, WIN0_CTRL1 + (win_id * 0x40), val);
	} else if (win_id == 2) {
		val = V_HWC_AXI_GATHER_EN(1) |
			V_HWC_AXI_GATHER_NUM(yrgb_gather_num);
		vop_msk_reg(vop_dev, HWC_CTRL1, val);
	}
	return 0;
}

static int vop_win_full_reg_update(struct vop_device *vop_dev,
				   struct rk_lcdc_win *win, int win_id)
{
	unsigned int off;
	uint64_t val;
	off = win_id * 0x100;

	if (win->state == 1) {
		vop_axi_gather_cfg(vop_dev, win, win_id);
		val = V_WIN0_EN(win->state) |
			V_WIN0_DATA_FMT(win->area[0].format) |
			V_WIN0_FMT_10(win->fmt_10) |
			V_WIN0_LB_MODE(win->win_lb_mode) |
			V_WIN0_RB_SWAP(win->rb_swap) |
			V_WIN0_X_MIR_EN(win->xmirror) |
			V_WIN0_Y_MIR_EN(win->ymirror) |
			V_WIN0_UV_SWAP(0);
		vop_msk_reg(vop_dev, WIN0_CTRL0 + off, val);
		val = V_WIN0_BIC_COE_SEL(win->bic_coe_el) |
		    V_WIN0_VSD_YRGB_GT4(win->vsd_yrgb_gt4) |
		    V_WIN0_VSD_YRGB_GT2(win->vsd_yrgb_gt2) |
		    V_WIN0_VSD_CBR_GT4(win->vsd_cbr_gt4) |
		    V_WIN0_VSD_CBR_GT2(win->vsd_cbr_gt2) |
		    V_WIN0_YRGB_HOR_SCL_MODE(win->yrgb_hor_scl_mode) |
		    V_WIN0_YRGB_VER_SCL_MODE(win->yrgb_ver_scl_mode) |
		    V_WIN0_YRGB_HSD_MODE(win->yrgb_hsd_mode) |
		    V_WIN0_YRGB_VSU_MODE(win->yrgb_vsu_mode) |
		    V_WIN0_YRGB_VSD_MODE(win->yrgb_vsd_mode) |
		    V_WIN0_CBR_HOR_SCL_MODE(win->cbr_hor_scl_mode) |
		    V_WIN0_CBR_VER_SCL_MODE(win->cbr_ver_scl_mode) |
		    V_WIN0_CBR_HSD_MODE(win->cbr_hsd_mode) |
		    V_WIN0_CBR_VSU_MODE(win->cbr_vsu_mode) |
		    V_WIN0_CBR_VSD_MODE(win->cbr_vsd_mode);
		vop_msk_reg(vop_dev, WIN0_CTRL1 + off, val);
		val = V_WIN0_VIR_STRIDE(win->area[0].y_vir_stride) |
		    V_WIN0_VIR_STRIDE_UV(0);
		vop_writel(vop_dev, WIN0_VIR + off, val);
		val = V_WIN0_ACT_WIDTH(win->area[0].xact - 1) |
		    V_WIN0_ACT_HEIGHT(win->area[0].yact - 1);
		vop_writel(vop_dev, WIN0_ACT_INFO + off, val);

		val = V_WIN0_DSP_WIDTH(win->area[0].xsize - 1) |
		    V_WIN0_DSP_HEIGHT(win->area[0].ysize - 1);
		vop_writel(vop_dev, WIN0_DSP_INFO + off, val);

		val = V_WIN0_DSP_XST(win->area[0].dsp_stx) |
		    V_WIN0_DSP_YST(win->area[0].dsp_sty);
		vop_writel(vop_dev, WIN0_DSP_ST + off, val);

		val = V_WIN0_HS_FACTOR_YRGB(win->scale_yrgb_x) |
		    V_WIN0_VS_FACTOR_YRGB(win->scale_yrgb_y);
		vop_writel(vop_dev, WIN0_SCL_FACTOR_YRGB + off, val);

		val = V_WIN0_HS_FACTOR_CBR(win->scale_cbcr_x) |
		    V_WIN0_VS_FACTOR_CBR(win->scale_cbcr_y);
		vop_writel(vop_dev, WIN0_SCL_FACTOR_CBR + off, val);
		val = V_WIN0_SRC_ALPHA_EN(0);
		vop_msk_reg(vop_dev, WIN0_SRC_ALPHA_CTRL + off, val);
	} else {
		val = V_WIN0_EN(win->state);
		vop_msk_reg(vop_dev, WIN0_CTRL0 + off, val);
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
	if ((!mirror_en) && (!screen->y_mirror))
		pos = area->ypos + screen->mode.upper_margin +
			screen->mode.vsync_len;
	else
		pos = screen->mode.yres - area->ypos -
			area->ysize + screen->mode.upper_margin +
			screen->mode.vsync_len;

	return pos;
}

static void vop_win_csc_mode(struct vop_device *vop_dev,
				     struct fb_dsp_info *fb_info,
				     int overlay_mode,
				     int output_color)
{
	u64 val;
	u32 shift;
	int win_csc_mode = 0;
	int r2y_en = 0, y2r_en = 0;

	if (overlay_mode == VOP_YUV_DOMAIN) {
		r2y_en = 1;
		y2r_en = 0;
		/* r2y csc mode depend on output color mode */
		if (output_color == COLOR_YCBCR_BT2020)
			win_csc_mode = VOP_CSC_BT2020;
		else if (output_color == COLOR_YCBCR_BT709)
			win_csc_mode = VOP_CSC_BT709L;
		else
			win_csc_mode = VOP_CSC_BT601L;
	} else {
		r2y_en = 0;
		y2r_en = 0;
	}

	shift = fb_info->layer_id * 2;
	val = (V_WIN0_R2Y_EN(r2y_en) | V_WIN0_Y2R_EN(y2r_en)) << shift;
	vop_msk_reg(vop_dev, SDR2HDR_CTRL, val);
	if (r2y_en | y2r_en) {
		shift = fb_info->layer_id * 0x100;
		val = V_WIN0_CSC_MODE(win_csc_mode);
		vop_msk_reg(vop_dev, WIN0_CTRL0 + shift, val);
	}
}

static int win_full_set_par(struct vop_device *vop_dev,
			    struct fb_dsp_info *fb_info, vidinfo_t *vid)
{
	struct rk_lcdc_win win;
	struct rk_screen *screen = vop_dev->screen;
	u32 y_addr = fb_info->yaddr;
	u32 top, bottom, left, right;

	memset(&win, 0, sizeof(struct rk_lcdc_win));
	rk_fb_vidinfo_to_win(fb_info, &win);
	if (fb_info->yaddr)
		win.state = 1;
	else
		win.state = 0;
	win.mirror_en = 0;
	left = screen->mode.xres * (vid->overscan - vid->left) / (vid->overscan * 2);
	right = screen->mode.xres * (vid->overscan - vid->right) / (vid->overscan * 2);
	top = screen->mode.yres * (vid->overscan - vid->top) / (vid->overscan * 2);
	bottom = screen->mode.yres * (vid->overscan - vid->bottom) / (vid->overscan * 2);

	win.area[0].xpos += left;
	win.area[0].ypos += top;
	win.area[0].xsize -= left + right;
	win.area[0].ysize -= top + bottom;
	debug("xpos = %d , ypos = %d, xsize = %d, ysize = %d\n",
	      win.area[0].xpos, win.area[0].ypos,
	      win.area[0].xsize, win.area[0].ysize);

	win.area[0].dsp_stx = dsp_x_pos(win.mirror_en, screen, win.area);
	win.area[0].dsp_sty = dsp_y_pos(win.mirror_en, screen, win.area);
	vop_vop_calc_scl_fac(&win, screen);

	switch (fb_info->format) {
	case ARGB888:
		win.area[0].y_vir_stride = ARGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB888:
		win.rb_swap = 1;
		win.area[0].y_vir_stride = RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		win.area[0].y_vir_stride = RGB565_VIRWIDTH(fb_info->xvir);
		break;
	default:
		win.area[0].y_vir_stride = RGB888_VIRWIDTH(fb_info->xvir);
		break;
	}
	if (screen->y_mirror && fb_info->ymirror) {
		printf("unspoort enable screen and win ymirror\n");
		return -EINVAL;
	}

	if (screen->y_mirror || fb_info->ymirror) {
		y_addr += win.area[0].y_vir_stride * 4 * (win.area[0].yact - 1);
		win.ymirror = fb_info->ymirror;
	}
	vop_win_csc_mode(vop_dev, fb_info, vop_dev->overlay_mode,
			 vop_dev->output_color);
	vop_win_full_reg_update(vop_dev, &win, fb_info->layer_id);
	vop_writel(vop_dev, WIN0_YRGB_MST, y_addr);

	return 0;
}

void rk_lcdc_set_par(struct fb_dsp_info *fb_info, vidinfo_t *vid)
{
	struct vop_device *vop_dev = &rk_vop_full;

	fb_info->layer_id = vop_dev->dft_win;

	switch (fb_info->layer_id) {
	case WIN0:
	case WIN1:
		win_full_set_par(vop_dev, fb_info, vid);
		break;
	default:
		printf("%s unknow lay_id %d\n", __func__, fb_info->layer_id);
		break;
	}
	vop_cfg_done(vop_dev);
}

static int vop_vop_post_cfg(struct vop_device *vop_dev,
			    struct rk_screen *screen)
{
	u16 x_res = screen->mode.xres;
	u16 y_res = screen->mode.yres;
	uint64_t val;
	u16 h_total, v_total;
	u16 post_hsd_en, post_vsd_en;
	u16 post_dsp_hact_st, post_dsp_hact_end;
	u16 post_dsp_vact_st, post_dsp_vact_end;
	u16 post_dsp_vact_st_f1, post_dsp_vact_end_f1;
	u16 post_h_fac, post_v_fac;

	screen->post_dsp_stx = x_res * (100 - screen->overscan.left) / 200;
	screen->post_dsp_sty = y_res * (100 - screen->overscan.top) / 200;
	screen->post_xsize = x_res *
		(screen->overscan.left + screen->overscan.right) / 200;
	screen->post_ysize = y_res *
		(screen->overscan.top + screen->overscan.bottom) / 200;

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

	debug("post:xsize=%d,ysize=%d,xpos=%d",
	      screen->post_xsize, screen->post_ysize, screen->xpos);
	debug(",ypos=%d,hsd_en=%d,h_fac=%d,vsd_en=%d,v_fac=%d\n",
	      screen->ypos, post_hsd_en, post_h_fac, post_vsd_en, post_v_fac);
	val = V_DSP_HACT_END_POST(post_dsp_hact_end) |
	    V_DSP_HACT_ST_POST(post_dsp_hact_st);
	vop_msk_reg(vop_dev, POST_DSP_HACT_INFO, val);

	val = V_DSP_VACT_END_POST(post_dsp_vact_end) |
	    V_DSP_VACT_ST_POST(post_dsp_vact_st);
	vop_msk_reg(vop_dev, POST_DSP_VACT_INFO, val);

	val = V_POST_HS_FACTOR_YRGB(post_h_fac) |
	    V_POST_VS_FACTOR_YRGB(post_v_fac);
	vop_msk_reg(vop_dev, POST_SCL_FACTOR_YRGB, val);
	val = V_DSP_VACT_END_POST(post_dsp_vact_end_f1) |
	    V_DSP_VACT_ST_POST(post_dsp_vact_st_f1);
	vop_msk_reg(vop_dev, POST_DSP_VACT_INFO_F1, val);
	val = V_POST_HOR_SD_EN(post_hsd_en) | V_POST_VER_SD_EN(post_vsd_en);
	vop_msk_reg(vop_dev, POST_SCL_CTRL, val);

	return 0;
}

static int vop_config_timing(struct vop_device *vop_dev,
			     struct rk_screen *screen)
{
	u16 hsync_len = screen->mode.hsync_len;
	u16 left_margin = screen->mode.left_margin;
	u16 right_margin = screen->mode.right_margin;
	u16 vsync_len = screen->mode.vsync_len;
	u16 upper_margin = screen->mode.upper_margin;
	u16 lower_margin = screen->mode.lower_margin;
	u16 x_res = screen->mode.xres;
	u16 y_res = screen->mode.yres;
	u64 val;
	u16 h_total, v_total;
	u16 vact_end_f1, vact_st_f1, vs_end_f1, vs_st_f1;

	h_total = hsync_len + left_margin + x_res + right_margin;
	v_total = vsync_len + upper_margin + y_res + lower_margin;

	val = V_DSP_HS_END(hsync_len) | V_DSP_HTOTAL(h_total);
	vop_msk_reg(vop_dev, DSP_HTOTAL_HS_END, val);

	val = V_DSP_HACT_END(hsync_len + left_margin + x_res) |
	    V_DSP_HACT_ST(hsync_len + left_margin);
	vop_msk_reg(vop_dev, DSP_HACT_ST_END, val);

	if (screen->mode.vmode & FB_VMODE_INTERLACED) {
		/* First Field Timing */
		val = V_DSP_VS_END(vsync_len) |
		    V_DSP_VTOTAL(2 * (vsync_len + upper_margin +
				      lower_margin) + y_res + 1);
		vop_msk_reg(vop_dev, DSP_VTOTAL_VS_END, val);

		val = V_DSP_VACT_END(vsync_len + upper_margin + y_res / 2) |
		    V_DSP_VACT_ST(vsync_len + upper_margin);
		vop_msk_reg(vop_dev, DSP_VACT_ST_END, val);

		/* Second Field Timing */
		vs_st_f1 = vsync_len + upper_margin + y_res / 2 + lower_margin;
		vs_end_f1 = 2 * vsync_len + upper_margin + y_res / 2 +
		    lower_margin;
		val = V_DSP_VS_ST_F1(vs_st_f1) | V_DSP_VS_END_F1(vs_end_f1);
		vop_msk_reg(vop_dev, DSP_VS_ST_END_F1, val);

		vact_end_f1 = 2 * (vsync_len + upper_margin) + y_res +
		    lower_margin + 1;
		vact_st_f1 = 2 * (vsync_len + upper_margin) + y_res / 2 +
		    lower_margin + 1;
		val = V_DSP_VACT_END_F1(vact_end_f1) |
			V_DSP_VACT_ST_F1(vact_st_f1);
		vop_msk_reg(vop_dev, DSP_VACT_ST_END_F1, val);
		vop_msk_reg(vop_dev, DSP_CTRL0,
			    V_DSP_INTERLACE(1) | V_DSP_FIELD_POL(0));
		val = V_DSP_LINE_FLAG_NUM_0(lower_margin ?
					    vact_end_f1 : vact_end_f1 - 1);
		val |= V_DSP_LINE_FLAG_NUM_1(lower_margin ?
					     vact_end_f1 : vact_end_f1 - 1);
		vop_msk_reg(vop_dev, LINE_FLAG, val);
	} else {
		val = V_DSP_VS_END(vsync_len) | V_DSP_VTOTAL(v_total);
		vop_msk_reg(vop_dev, DSP_VTOTAL_VS_END, val);

		val = V_DSP_VACT_END(vsync_len + upper_margin + y_res) |
		    V_DSP_VACT_ST(vsync_len + upper_margin);
		vop_msk_reg(vop_dev, DSP_VACT_ST_END, val);

		vop_msk_reg(vop_dev, DSP_CTRL0, V_DSP_INTERLACE(0) |
			    V_DSP_FIELD_POL(0));
		val = V_DSP_LINE_FLAG_NUM_0(vsync_len + upper_margin + y_res) |
			V_DSP_LINE_FLAG_NUM_1(vsync_len + upper_margin + y_res);
		vop_msk_reg(vop_dev, LINE_FLAG, val);
	}
	vop_vop_post_cfg(vop_dev, screen);
	return 0;
}

static void vop_bcsh_path_sel(struct vop_device *vop_dev)
{
	u32 bcsh_ctrl;
	u32 r2y_mode = 0, y2r_mode = 0;

	vop_msk_reg(vop_dev, SYS_CTRL, V_OVERLAY_MODE(vop_dev->overlay_mode));
	vop_msk_reg(vop_dev, SYS_CTRL1,
		    V_LEVEL2_OVERLAY_EN(vop_dev->pre_overlay));
	if (vop_dev->overlay_mode == VOP_YUV_DOMAIN) {
		if (IS_YUV_COLOR(vop_dev->output_color)) {	/* bypass */
			vop_msk_reg(vop_dev, BCSH_CTRL,
				    V_BCSH_Y2R_EN(0) | V_BCSH_R2Y_EN(0));
		} else {		/* YUV2RGB */
			y2r_mode = VOP_CSC_BT709L;
			vop_msk_reg(vop_dev, BCSH_CTRL, V_BCSH_Y2R_EN(1) |
				    V_BCSH_Y2R_CSC_MODE(y2r_mode) |
				    V_BCSH_R2Y_EN(0));
		}
	} else {
		/* overlay_mode=VOP_RGB_DOMAIN */
		/* bypass  --need check,if bcsh close */
		if (!IS_YUV_COLOR(vop_dev->output_color)) {
			bcsh_ctrl = vop_readl(vop_dev, BCSH_CTRL);
			if ((bcsh_ctrl & MASK(BCSH_EN)) == 1)/*bcsh enabled */
				vop_msk_reg(vop_dev, BCSH_CTRL,
					    V_BCSH_R2Y_EN(1) |
					    V_BCSH_Y2R_EN(1));
			else
				vop_msk_reg(vop_dev, BCSH_CTRL,
					    V_BCSH_R2Y_EN(0) |
					    V_BCSH_Y2R_EN(0));
		} else {
			/* RGB2YUV */
			if (vop_dev->output_color == COLOR_YCBCR_BT2020)
				r2y_mode = VOP_CSC_BT2020;
			else if (vop_dev->output_color == COLOR_YCBCR_BT709)
				r2y_mode = VOP_CSC_BT709L;
			else if (vop_dev->output_color == COLOR_YCBCR)
				r2y_mode = VOP_CSC_BT601L;
			else
				r2y_mode = VOP_CSC_BT601F;
			vop_msk_reg(vop_dev, BCSH_CTRL,
				    V_BCSH_R2Y_EN(1) |
				    V_BCSH_R2Y_CSC_MODE(r2y_mode) |
				    V_BCSH_Y2R_EN(0));
		}
	}
}

int rk_lcdc_load_screen(vidinfo_t *vid)
{
	struct vop_device *vop_dev = &rk_vop_full;
	struct rk_screen *screen;
	uint64_t val = 0;
	int face = 0;
	u16 dclk_ddr = 0;

	screen = vop_dev->screen;
	rk_fb_vidinfo_to_screen(vid, screen);
	vop_dev->overlay_mode = VOP_RGB_DOMAIN;

	switch (screen->face) {
	case OUT_P565:
		face = OUT_P565;
		val = V_DITHER_DOWN_EN(1) | V_DITHER_UP_EN(1) |
			V_PRE_DITHER_DOWN_EN(1) |
			V_DITHER_DOWN_SEL(1) | V_DITHER_DOWN_MODE(0);
		break;
	case OUT_P666:
		face = OUT_P666;
		val = V_DITHER_DOWN_EN(1) | V_DITHER_UP_EN(1) |
			V_PRE_DITHER_DOWN_EN(1) |
			V_DITHER_DOWN_SEL(1) | V_DITHER_DOWN_MODE(1);
		break;
	case OUT_D888_P565:
		face = OUT_P888;
		val = V_DITHER_DOWN_EN(1) | V_DITHER_UP_EN(1) |
			V_PRE_DITHER_DOWN_EN(1) |
			V_DITHER_DOWN_SEL(1) | V_DITHER_DOWN_MODE(0);
		break;
	case OUT_D888_P666:
		face = OUT_P888;
		val = V_DITHER_DOWN_EN(1) | V_DITHER_UP_EN(1) |
			V_PRE_DITHER_DOWN_EN(1) |
			V_DITHER_DOWN_SEL(1) | V_DITHER_DOWN_MODE(1);
		break;
	case OUT_P888:
		face = OUT_P888;
		val = V_DITHER_DOWN_EN(0) | V_DITHER_UP_EN(1)
			| V_PRE_DITHER_DOWN_EN(1);
		break;
	case OUT_YUV_420:
		face = OUT_YUV_420;
		dclk_ddr = 1;
		val = V_DITHER_DOWN_EN(0) | V_DITHER_UP_EN(1)
			| V_PRE_DITHER_DOWN_EN(1);
		break;
	case OUT_YUV_420_10BIT:
		face = OUT_YUV_420;
		dclk_ddr = 1;
		val = V_DITHER_DOWN_EN(0) | V_DITHER_UP_EN(1)
			| V_PRE_DITHER_DOWN_EN(0);
			break;
		break;
	case OUT_P101010:
		face = OUT_P101010;
		val = V_DITHER_DOWN_EN(0) | V_DITHER_UP_EN(1)
			| V_PRE_DITHER_DOWN_EN(0);
		break;
	default:
		dev_err(vop_dev->dev, "un supported screen face[%d]!\n",
			screen->face);
		break;
	}

	vop_msk_reg(vop_dev, DSP_CTRL1, val);
	switch (screen->type) {
	case SCREEN_TVOUT:
		val = V_SW_UV_OFFSET_EN(1) | V_SW_IMD_TVE_DCLK_EN(1) |
			V_SW_IMD_TVE_DCLK_EN(1) |
			V_SW_IMD_TVE_DCLK_POL(1) |
			V_SW_GENLOCK(1) | V_SW_DAC_SEL(1);
		if (screen->mode.xres == 720 &&
		    screen->mode.yres == 576)
			val |= V_SW_TVE_MODE(1);
		else
			val |= V_SW_TVE_MODE(0);
		vop_msk_reg(vop_dev, SYS_CTRL, val);
		val = V_HDMI_HSYNC_POL(screen->pin_hsync) |
			V_HDMI_VSYNC_POL(screen->pin_vsync) |
			V_HDMI_DEN_POL(screen->pin_den) |
			V_HDMI_DCLK_POL(screen->pin_dclk);
		/* hsync vsync den dclk polo,dither */
		vop_msk_reg(vop_dev, DSP_CTRL1, val);
		break;
	case SCREEN_HDMI:
		val = V_HDMI_OUT_EN(1) | V_SW_UV_OFFSET_EN(0);
		vop_msk_reg(vop_dev, SYS_CTRL, val);
		if ((screen->face == OUT_P888) ||
		    (screen->face == OUT_P101010)) {
			face = OUT_P101010;
			val = V_PRE_DITHER_DOWN_EN(0);
			vop_msk_reg(vop_dev, DSP_CTRL1, val);
		}
		val = V_HDMI_HSYNC_POL(screen->pin_hsync) |
			V_HDMI_VSYNC_POL(screen->pin_vsync) |
			V_HDMI_DEN_POL(screen->pin_den) |
			V_HDMI_DCLK_POL(screen->pin_dclk);
		/* hsync vsync den dclk polo,dither */
		vop_msk_reg(vop_dev, DSP_CTRL1, val);
		break;
	case SCREEN_RGB:
	case SCREEN_LVDS:
		val = V_RGB_OUT_EN(1) | V_HDMI_OUT_EN(1);
		vop_msk_reg(vop_dev, SYS_CTRL, val);
		break;
	default:
		dev_err(vop_dev->dev, "un supported interface[%d]!\n",
			screen->type);
		break;
	}

	if (screen->color_mode == COLOR_RGB)
		vop_dev->overlay_mode = VOP_RGB_DOMAIN;
	else
		vop_dev->overlay_mode = VOP_YUV_DOMAIN;

	val = V_DSP_OUT_MODE(face) | V_DSP_DCLK_DDR(dclk_ddr) |
		V_DSP_BG_SWAP(screen->swap_gb) |
		V_DSP_RB_SWAP(screen->swap_rb) |
		V_DSP_RG_SWAP(screen->swap_rg) |
		V_DSP_DELTA_SWAP(screen->swap_delta) |
		V_DSP_DUMMY_SWAP(screen->swap_dumy) | V_DSP_OUT_ZERO(0) |
		V_DSP_BLANK_EN(0) | V_DSP_BLACK_EN(0) |
		V_DSP_X_MIR_EN(screen->x_mirror) |
		V_DSP_Y_MIR_EN(screen->y_mirror);
		val |= V_SW_CORE_DCLK_SEL(!!screen->pixelrepeat);
	if (screen->mode.vmode & FB_VMODE_INTERLACED)
		val |= V_SW_P2I_EN(1);
	else
		val |= V_SW_P2I_EN(0);
	vop_msk_reg(vop_dev, DSP_CTRL0, val);
	vop_msk_reg(vop_dev, SYS_CTRL1, V_REG_DONE_FRM(0));
	/* BG color */
	if (vop_dev->overlay_mode == VOP_YUV_DOMAIN)
		val = V_DSP_BG_BLUE(0x200) | V_DSP_BG_GREEN(0x40) |
			V_DSP_BG_RED(0x200);
	else
		val = V_DSP_BG_BLUE(0) | V_DSP_BG_GREEN(0) |
			V_DSP_BG_RED(0);
	vop_msk_reg(vop_dev, DSP_BG, val);
	vop_dev->output_color = screen->color_mode;
	vop_bcsh_path_sel(vop_dev);
	vop_config_timing(vop_dev, screen);
	vop_cfg_done(vop_dev);
	return 0;
}

/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable)
{
	struct vop_device *vop_dev = &rk_vop_full;
	vop_msk_reg(vop_dev, SYS_CTRL, V_VOP_STANDBY_EN(!!enable));
	vop_cfg_done(vop_dev);
}

#if defined(CONFIG_OF_LIBFDT)
static int vop_parse_dt(struct vop_device *vop_dev,
			const void *blob, int vop_id)
{
	int order = FB0_WIN0_FB1_WIN1_FB2_WIN2;

	if (vop_id == 0)
		vop_dev->node  = fdt_path_offset(blob, "lcdc0");
	else
		vop_dev->node  = fdt_path_offset(blob, "lcdc1");
	if (vop_dev->node < 0) {
		debug("rk vop full node is not found\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, vop_dev->node)) {
		debug("device lcdc is disabled\n");
		return -EPERM;
	}

	vop_dev->regs = fdtdec_get_addr(blob, vop_dev->node, "reg");
	if (vop_dev->regs == FDT_ADDR_T_NONE) {
		debug("%s: Could not find vop regs\n", __func__);
		return -1;
	}
	order = fdtdec_get_int(blob, vop_dev->node,
			       "rockchip,fb-win-map", order);
	vop_dev->dft_win = order % 10;
	vop_dev->cabc_mode = fdtdec_get_int(blob, vop_dev->node,
				"rockchip,cabc_mode", 0);

	return 0;
}
#endif

int vop_read_def_cfg(struct vop_device *vop_dev)
{
	int reg = 0;

	for (reg = 0; reg < REG_LEN; reg += 4)
		vop_readl_backup(vop_dev, reg);

	return 0;
}


int rk_lcdc_init(int vop_id)
{
	struct vop_device *vop_dev = &rk_vop_full;
	uint64_t val;

	vop_dev->soc_type = gd->arch.chiptype;
	vop_dev->id = vop_id;
#ifdef CONFIG_OF_LIBFDT
	if (vop_parse_dt(vop_dev, gd->fdt_blob, vop_id)) {
		debug("%s: vop_parse_dt failed\n", __func__);
		return -EINVAL;
	}
#else
	debug("%s: Not support libfdt\n", __func__);
	return -EINVAL;
#endif
	vop_dev->screen = kzalloc(sizeof(*vop_dev->screen), GFP_KERNEL);
	vop_read_def_cfg(vop_dev);

	val =  V_AUTO_GATING_EN(0) | V_VOP_STANDBY_EN(0) |
		V_VOP_DMA_STOP(0) | V_VOP_MMU_EN(0);
	vop_msk_reg(vop_dev, SYS_CTRL, val);
	val = V_DSP_LAYER3_SEL(3) | V_DSP_LAYER2_SEL(2) |
		V_DSP_LAYER1_SEL(1) | V_DSP_LAYER0_SEL(0) |
		V_DITHER_UP_EN(1);
	vop_msk_reg(vop_dev, DSP_CTRL1, val);

	vop_writel(vop_dev, FRC_LOWER01_0, 0x12844821);
	vop_writel(vop_dev, FRC_LOWER01_1, 0x21488412);
	vop_writel(vop_dev, FRC_LOWER10_0, 0xa55a9696);
	vop_writel(vop_dev, FRC_LOWER10_1, 0x5aa56969);
	vop_writel(vop_dev, FRC_LOWER11_0, 0xdeb77deb);
	vop_writel(vop_dev, FRC_LOWER11_1, 0xed7bb7de);

	vop_writel(vop_dev, WIN0_CTRL2, 0x21);
	vop_writel(vop_dev, WIN1_CTRL2, 0x43);
	vop_writel(vop_dev, WIN2_CTRL2, 0x65);

	vop_msk_reg(vop_dev, SYS_CTRL, V_AUTO_GATING_EN(0));
	vop_msk_reg(vop_dev, DSP_CTRL1, V_DITHER_UP_EN(1));

	vop_msk_reg(vop_dev, SDR2HDR_CTRL, V_WIN_CSC_MODE_SEL(1));
	val = V_SRC_MAX(12642) | V_SRC_MIN(494);
	vop_msk_reg(vop_dev, HDR2SDR_SRC_RANGE, val);
	val = V_NORMFACEETF(1327);
	vop_msk_reg(vop_dev, HDR2SDR_NORMFACEETF, val);
	val = V_SRC_MAX(4636) | V_SRC_MIN(0);
	vop_msk_reg(vop_dev, HDR2SDR_DST_RANGE, val);
	val = V_NORMFACCGAMMA(10240);
	vop_msk_reg(vop_dev, HDR2SDR_NORMFACCGAMMA, val);

	vop_dev->pre_overlay = 0;
	vop_cfg_done(vop_dev);

	return 0;
}
