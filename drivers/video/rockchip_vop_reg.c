/*
 * Copyright (C) Fuzhou Rockchip Electronics Co.Ltd
 * Author:Mark Yao <mark.yao@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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

#include "rockchip_vop.h"
#include "rockchip_vop_reg.h"

#define VOP_REG_VER_MASK(off, _mask, s, _write_mask, _major, \
		         _begin_minor, _end_minor) \
		{.offset = off, \
		 .mask = _mask, \
		 .shift = s, \
		 .write_mask = _write_mask, \
		 .major = _major, \
		 .begin_minor = _begin_minor, \
		 .end_minor = _end_minor,}

#define VOP_REG(off, _mask, s) \
		VOP_REG_VER_MASK(off, _mask, s, false, 0, 0, -1)

#define VOP_REG_MASK(off, _mask, s) \
		VOP_REG_VER_MASK(off, _mask, s, true, 0, 0, -1)

#define VOP_REG_VER(off, _mask, s, _major, _begin_minor, _end_minor) \
		VOP_REG_VER_MASK(off, _mask, s, false, \
				 _major, _begin_minor, _end_minor)

static const struct vop_scl_extension rk3288_win_full_scl_ext = {
	.cbcr_vsd_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 31),
	.cbcr_vsu_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 30),
	.cbcr_hsd_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 28),
	.cbcr_ver_scl_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 26),
	.cbcr_hor_scl_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 24),
	.yrgb_vsd_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 23),
	.yrgb_vsu_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 22),
	.yrgb_hsd_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 20),
	.yrgb_ver_scl_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 18),
	.yrgb_hor_scl_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 16),
	.line_load_mode = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 15),
	.cbcr_axi_gather_num = VOP_REG(RK3288_WIN0_CTRL1, 0x7, 12),
	.yrgb_axi_gather_num = VOP_REG(RK3288_WIN0_CTRL1, 0xf, 8),
	.vsd_cbcr_gt2 = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 7),
	.vsd_cbcr_gt4 = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 6),
	.vsd_yrgb_gt2 = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 5),
	.vsd_yrgb_gt4 = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 4),
	.bic_coe_sel = VOP_REG(RK3288_WIN0_CTRL1, 0x3, 2),
	.cbcr_axi_gather_en = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 1),
	.yrgb_axi_gather_en = VOP_REG(RK3288_WIN0_CTRL1, 0x1, 0),
	.lb_mode = VOP_REG(RK3288_WIN0_CTRL0, 0x7, 5),
};

static const struct vop_scl_regs rk3288_win_full_scl = {
	.ext = &rk3288_win_full_scl_ext,
	.scale_yrgb_x = VOP_REG(RK3288_WIN0_SCL_FACTOR_YRGB, 0xffff, 0x0),
	.scale_yrgb_y = VOP_REG(RK3288_WIN0_SCL_FACTOR_YRGB, 0xffff, 16),
	.scale_cbcr_x = VOP_REG(RK3288_WIN0_SCL_FACTOR_CBR, 0xffff, 0x0),
	.scale_cbcr_y = VOP_REG(RK3288_WIN0_SCL_FACTOR_CBR, 0xffff, 16),
};

static const struct vop_win rk3288_win01_data = {
	.scl = &rk3288_win_full_scl,
	.enable = VOP_REG(RK3288_WIN0_CTRL0, 0x1, 0),
	.format = VOP_REG(RK3288_WIN0_CTRL0, 0x7, 1),
	.rb_swap = VOP_REG(RK3288_WIN0_CTRL0, 0x1, 12),
	.ymirror = VOP_REG_VER(RK3368_WIN0_CTRL0, 0x1, 22, 3, 2, -1),
	.act_info = VOP_REG(RK3288_WIN0_ACT_INFO, 0x1fff1fff, 0),
	.dsp_info = VOP_REG(RK3288_WIN0_DSP_INFO, 0x0fff0fff, 0),
	.dsp_st = VOP_REG(RK3288_WIN0_DSP_ST, 0x1fff1fff, 0),
	.yrgb_mst = VOP_REG(RK3288_WIN0_YRGB_MST, 0xffffffff, 0),
	.uv_mst = VOP_REG(RK3288_WIN0_CBR_MST, 0xffffffff, 0),
	.yrgb_vir = VOP_REG(RK3288_WIN0_VIR, 0x3fff, 0),
	.uv_vir = VOP_REG(RK3288_WIN0_VIR, 0x3fff, 16),
	.src_alpha_ctl = VOP_REG(RK3288_WIN0_SRC_ALPHA_CTRL, 0xffffffff, 0),
	.dst_alpha_ctl = VOP_REG(RK3288_WIN0_DST_ALPHA_CTRL, 0xffffffff, 0),
};

static const struct vop_ctrl rk3288_ctrl_data = {
	.standby = VOP_REG(RK3288_SYS_CTRL, 0x1, 22),
	.axi_outstanding_max_num = VOP_REG(RK3288_SYS_CTRL1, 0x1f, 13),
	.axi_max_outstanding_en = VOP_REG(RK3288_SYS_CTRL1, 0x1, 12),
	.reg_done_frm = VOP_REG_VER(RK3288_SYS_CTRL1, 0x1, 24, 3, 7, -1),
	.htotal_pw = VOP_REG(RK3288_DSP_HTOTAL_HS_END, 0x1fff1fff, 0),
	.hact_st_end = VOP_REG(RK3288_DSP_HACT_ST_END, 0x1fff1fff, 0),
	.vtotal_pw = VOP_REG(RK3288_DSP_VTOTAL_VS_END, 0x1fff1fff, 0),
	.vact_st_end = VOP_REG(RK3288_DSP_VACT_ST_END, 0x1fff1fff, 0),
	.vact_st_end_f1 = VOP_REG(RK3288_DSP_VACT_ST_END_F1, 0x1fff1fff, 0),
	.vs_st_end_f1 = VOP_REG(RK3288_DSP_VS_ST_END_F1, 0x1fff1fff, 0),
	.hpost_st_end = VOP_REG(RK3288_POST_DSP_HACT_INFO, 0x1fff1fff, 0),
	.vpost_st_end = VOP_REG(RK3288_POST_DSP_VACT_INFO, 0x1fff1fff, 0),
	.vpost_st_end_f1 = VOP_REG(RK3288_POST_DSP_VACT_INFO_F1, 0x1fff1fff, 0),
	.post_scl_factor = VOP_REG(RK3288_POST_SCL_FACTOR_YRGB, 0xffffffff, 0),
	.post_scl_ctrl = VOP_REG(RK3288_POST_SCL_CTRL, 0x3, 0),

	.dsp_interlace = VOP_REG(RK3288_DSP_CTRL0, 0x1, 10),
	.auto_gate_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 23),
	.dsp_layer_sel = VOP_REG(RK3288_DSP_CTRL1, 0xff, 8),
	.post_lb_mode = VOP_REG_VER(RK3288_SYS_CTRL, 0x1, 18, 3, 2, -1),
	.global_regdone_en = VOP_REG_VER(RK3288_SYS_CTRL, 0x1, 11, 3, 2, -1),
	.overlay_mode = VOP_REG_VER(RK3288_SYS_CTRL, 0x1, 16, 3, 2, -1),
	.core_dclk_div = VOP_REG_VER(RK3399_DSP_CTRL0, 0x1, 4, 3, 4, -1),
	.p2i_en = VOP_REG_VER(RK3399_DSP_CTRL0, 0x1, 5, 3, 4, -1),
	.dclk_ddr = VOP_REG_VER(RK3399_DSP_CTRL0, 0x1, 8, 3, 4, -1),
	.dp_en = VOP_REG(RK3399_SYS_CTRL, 0x1, 11),
	.rgb_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 12),
	.hdmi_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 13),
	.edp_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 14),
	.mipi_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 15),
	.mipi_dual_channel_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 3),
	.data01_swap = VOP_REG_VER(RK3288_SYS_CTRL, 0x1, 17, 3, 5, -1),
	.pin_pol = VOP_REG_VER(RK3288_DSP_CTRL0, 0xf, 4, 3, 0, 1),
	.dp_pin_pol = VOP_REG_VER(RK3368_DSP_CTRL1, 0xf, 16, 3, 2, -1),
	.rgb_pin_pol = VOP_REG_VER(RK3368_DSP_CTRL1, 0xf, 16, 3, 2, -1),
	.tve_dclk_en = VOP_REG(RK3288_SYS_CTRL, 0x1, 24),
	.tve_dclk_pol = VOP_REG(RK3288_SYS_CTRL, 0x1, 25),
	.tve_sw_mode = VOP_REG(RK3288_SYS_CTRL, 0x1, 26),
	.sw_uv_offset_en  = VOP_REG(RK3288_SYS_CTRL, 0x1, 27),
	.sw_genlock   = VOP_REG(RK3288_SYS_CTRL, 0x1, 28),
	.hdmi_pin_pol = VOP_REG_VER(RK3368_DSP_CTRL1, 0xf, 20, 3, 2, -1),
	.edp_pin_pol = VOP_REG_VER(RK3368_DSP_CTRL1, 0xf, 24, 3, 2, -1),
	.mipi_pin_pol = VOP_REG_VER(RK3368_DSP_CTRL1, 0xf, 28, 3, 2, -1),

	.dither_down = VOP_REG(RK3288_DSP_CTRL1, 0xf, 1),
	.dither_up = VOP_REG(RK3288_DSP_CTRL1, 0x1, 6),

	.dsp_out_yuv = VOP_REG_VER(RK3399_POST_SCL_CTRL, 0x1, 2, 3, 5, -1),
	.dsp_data_swap = VOP_REG(RK3288_DSP_CTRL0, 0x1f, 12),
	.dsp_ccir656_avg = VOP_REG(RK3288_DSP_CTRL0, 0x1, 20),
	.dsp_blank = VOP_REG(RK3288_DSP_CTRL0, 0x3, 18),
	.dsp_lut_en = VOP_REG(RK3288_DSP_CTRL1, 0x1, 0),
	.update_gamma_lut = VOP_REG_VER(RK3288_DSP_CTRL1, 0x1, 7, 3, 5, -1),
	.out_mode = VOP_REG(RK3288_DSP_CTRL0, 0xf, 0),

	.bcsh_brightness = VOP_REG(RK3288_BCSH_BCS, 0xff, 0),
	.bcsh_contrast = VOP_REG(RK3288_BCSH_BCS, 0x1ff, 8),
	.bcsh_sat_con = VOP_REG(RK3288_BCSH_BCS, 0x3ff, 20),
	.bcsh_out_mode = VOP_REG(RK3288_BCSH_BCS, 0x3, 0),
	.bcsh_sin_hue = VOP_REG(RK3288_BCSH_H, 0x1ff, 0),
	.bcsh_cos_hue = VOP_REG(RK3288_BCSH_H, 0x1ff, 16),
	.bcsh_r2y_csc_mode = VOP_REG_VER(RK3368_BCSH_CTRL, 0x1, 6, 3, 1, -1),
	.bcsh_r2y_en = VOP_REG_VER(RK3368_BCSH_CTRL, 0x1, 4, 3, 1, -1),
	.bcsh_y2r_csc_mode = VOP_REG_VER(RK3368_BCSH_CTRL, 0x3, 2, 3, 1, -1),
	.bcsh_y2r_en = VOP_REG_VER(RK3368_BCSH_CTRL, 0x1, 0, 3, 1, -1),
	.bcsh_color_bar = VOP_REG(RK3288_BCSH_COLOR_BAR, 0xffffff, 8),
	.bcsh_en = VOP_REG(RK3288_BCSH_COLOR_BAR, 0x1, 0),

	.xmirror = VOP_REG(RK3288_DSP_CTRL0, 0x1, 22),
	.ymirror = VOP_REG(RK3288_DSP_CTRL0, 0x1, 23),

	.dsp_background = VOP_REG(RK3288_DSP_BG, 0xffffffff, 0),

	.cfg_done = VOP_REG(RK3288_REG_CFG_DONE, 0x1, 0),
	.win_gate[0] = VOP_REG(RK3288_WIN2_CTRL0, 0x1, 0),
	.win_gate[1] = VOP_REG(RK3288_WIN3_CTRL0, 0x1, 0),
};

static const struct vop_line_flag rk3288_vop_line_flag = {
	.line_flag_num[0] = VOP_REG(RK3288_INTR_CTRL0, 0x1fff, 12),
};

const struct vop_data rk3288_vop_big = {
	.version = VOP_VERSION(3, 1),
	.max_output = {3840, 2160},
	.feature = VOP_FEATURE_OUTPUT_10BIT,
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3288_vop_line_flag,
	.reg_len = RK3288_DSP_VACT_ST_END_F1 * 4,
};

const struct vop_data rk3288_vop_lit = {
	.version = VOP_VERSION(3, 1),
	.max_output = {2560, 1600},
	.feature = VOP_FEATURE_OUTPUT_10BIT,
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3288_vop_line_flag,
	.reg_len = RK3288_DSP_VACT_ST_END_F1 * 4,
};

static const struct vop_line_flag rk3368_vop_line_flag = {
	.line_flag_num[0] = VOP_REG(RK3368_LINE_FLAG, 0xffff, 0),
	.line_flag_num[1] = VOP_REG(RK3368_LINE_FLAG, 0xffff, 16),
};

const struct vop_data rk3368_vop = {
	.version = VOP_VERSION(3, 2),
	.max_output = {4096, 2160},
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3368_vop_line_flag,
	.reg_len = RK3368_DSP_VACT_ST_END_F1 * 4,
};

static const struct vop_line_flag rk3366_vop_line_flag = {
	.line_flag_num[0] = VOP_REG(RK3366_LINE_FLAG, 0xffff, 0),
	.line_flag_num[1] = VOP_REG(RK3366_LINE_FLAG, 0xffff, 16),
};

const struct vop_data rk3366_vop = {
	.version = VOP_VERSION(3, 4),
	.max_output = {4096, 2160},
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3366_vop_line_flag,
	.reg_len = RK3366_DSP_VACT_ST_END_F1 * 4,
};

static const uint32_t vop_csc_r2y_bt601[] = {
	0x02590132, 0xff530075, 0x0200fead, 0xfe530200,
	0x0000ffad, 0x00000200, 0x00080200, 0x00080200,
};

static const uint32_t vop_csc_r2y_bt601_12_235[] = {
	0x02040107, 0xff680064, 0x01c2fed6, 0xffb7fe87,
	0x0000ffb7, 0x00010200, 0x00080200, 0x00080200,
};

static const uint32_t vop_csc_r2y_bt709[] = {
	0x027500bb, 0xff99003f, 0x01c2fea5, 0xfe6801c2,
	0x0000ffd7, 0x00010200, 0x00080200, 0x00080200,
};

static const uint32_t vop_csc_r2y_bt2020[] = {
	0x025300e6, 0xff830034, 0x01c1febd, 0xfe6401c1,
	0x0000ffdc, 0x00010200, 0x00080200, 0x00080200,
};

static const struct vop_csc_table rk3399_csc_table = {
	.r2y_bt601		= vop_csc_r2y_bt601,
	.r2y_bt601_12_235	= vop_csc_r2y_bt601_12_235,
	.r2y_bt709		= vop_csc_r2y_bt709,
	.r2y_bt2020		= vop_csc_r2y_bt2020,
};

static const struct vop_csc rk3399_win0_csc = {
	.r2r_en = VOP_REG(RK3399_YUV2YUV_WIN, 0x1, 0),
	.y2r_en = VOP_REG(RK3399_YUV2YUV_WIN, 0x1, 1),
	.r2y_en = VOP_REG(RK3399_YUV2YUV_WIN, 0x1, 2),
	.y2r_offset = RK3399_WIN0_YUV2YUV_Y2R,
	.r2r_offset = RK3399_WIN0_YUV2YUV_3X3,
	.r2y_offset = RK3399_WIN0_YUV2YUV_R2Y,
};

const struct vop_data rk3399_vop_big = {
	.version = VOP_VERSION(3, 5),
	.max_output = {4096, 2160},
	.feature = VOP_FEATURE_OUTPUT_10BIT,
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3366_vop_line_flag,
	.csc_table = &rk3399_csc_table,
	.win_csc = &rk3399_win0_csc,
	.reg_len = RK3399_DSP_VACT_ST_END_F1 * 4,
};

const struct vop_data rk3399_vop_lit = {
	.version = VOP_VERSION(3, 6),
	.max_output = {2560, 1600},
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3366_vop_line_flag,
	.csc_table = &rk3399_csc_table,
	.win_csc = &rk3399_win0_csc,
	.reg_len = RK3399_DSP_VACT_ST_END_F1 * 4,
};

const struct vop_data rk322x_vop = {
	.version = VOP_VERSION(3, 7),
	.max_output = {4096, 2160},
	.feature = VOP_FEATURE_OUTPUT_10BIT,
	.ctrl = &rk3288_ctrl_data,
	.win = &rk3288_win01_data,
	.line_flag = &rk3366_vop_line_flag,
	.reg_len = RK3399_DSP_VACT_ST_END_F1 * 4,
};

static const struct vop_ctrl rk3328_ctrl_data = {
	.standby = VOP_REG(RK3328_SYS_CTRL, 0x1, 22),
	.axi_outstanding_max_num = VOP_REG(RK3328_SYS_CTRL1, 0x1f, 13),
	.axi_max_outstanding_en = VOP_REG(RK3328_SYS_CTRL1, 0x1, 12),
	.reg_done_frm = VOP_REG(RK3328_SYS_CTRL1, 0x1, 24),
	.auto_gate_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 23),
	.htotal_pw = VOP_REG(RK3328_DSP_HTOTAL_HS_END, 0x1fff1fff, 0),
	.hact_st_end = VOP_REG(RK3328_DSP_HACT_ST_END, 0x1fff1fff, 0),
	.vtotal_pw = VOP_REG(RK3328_DSP_VTOTAL_VS_END, 0x1fff1fff, 0),
	.vact_st_end = VOP_REG(RK3328_DSP_VACT_ST_END, 0x1fff1fff, 0),
	.vact_st_end_f1 = VOP_REG(RK3328_DSP_VACT_ST_END_F1, 0x1fff1fff, 0),
	.vs_st_end_f1 = VOP_REG(RK3328_DSP_VS_ST_END_F1, 0x1fff1fff, 0),
	.hpost_st_end = VOP_REG(RK3328_POST_DSP_HACT_INFO, 0x1fff1fff, 0),
	.vpost_st_end = VOP_REG(RK3328_POST_DSP_VACT_INFO, 0x1fff1fff, 0),
	.vpost_st_end_f1 = VOP_REG(RK3328_POST_DSP_VACT_INFO_F1, 0x1fff1fff, 0),
	.post_scl_factor = VOP_REG(RK3328_POST_SCL_FACTOR_YRGB, 0xffffffff, 0),
	.post_scl_ctrl = VOP_REG(RK3328_POST_SCL_CTRL, 0x3, 0),
	.dsp_out_yuv = VOP_REG(RK3328_POST_SCL_CTRL, 0x1, 2),
	.dsp_interlace = VOP_REG(RK3328_DSP_CTRL0, 0x1, 10),
	.dsp_layer_sel = VOP_REG(RK3328_DSP_CTRL1, 0xff, 8),
	.post_lb_mode = VOP_REG(RK3328_SYS_CTRL, 0x1, 18),
	.global_regdone_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 11),
	.overlay_mode = VOP_REG(RK3328_SYS_CTRL, 0x1, 16),
	.core_dclk_div = VOP_REG(RK3328_DSP_CTRL0, 0x1, 4),
	.dclk_ddr = VOP_REG(RK3328_DSP_CTRL0, 0x1, 8),
	.p2i_en = VOP_REG(RK3328_DSP_CTRL0, 0x1, 5),
	.rgb_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 12),
	.hdmi_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 13),
	.edp_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 14),
	.mipi_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 15),
	.tve_dclk_en = VOP_REG(RK3328_SYS_CTRL, 0x1, 24),
	.tve_dclk_pol = VOP_REG(RK3328_SYS_CTRL, 0x1, 25),
	.tve_sw_mode = VOP_REG(RK3328_SYS_CTRL, 0x1, 26),
	.sw_uv_offset_en  = VOP_REG(RK3328_SYS_CTRL, 0x1, 27),
	.sw_genlock   = VOP_REG(RK3328_SYS_CTRL, 0x1, 28),
	.sw_dac_sel = VOP_REG(RK3328_SYS_CTRL, 0x1, 29),
	.rgb_pin_pol = VOP_REG(RK3328_DSP_CTRL1, 0xf, 16),
	.hdmi_pin_pol = VOP_REG(RK3328_DSP_CTRL1, 0xf, 20),
	.edp_pin_pol = VOP_REG(RK3328_DSP_CTRL1, 0xf, 24),
	.mipi_pin_pol = VOP_REG(RK3328_DSP_CTRL1, 0xf, 28),

	.dither_down = VOP_REG(RK3328_DSP_CTRL1, 0xf, 1),
	.dither_up = VOP_REG(RK3328_DSP_CTRL1, 0x1, 6),

	.dsp_data_swap = VOP_REG(RK3328_DSP_CTRL0, 0x1f, 12),
	.dsp_ccir656_avg = VOP_REG(RK3328_DSP_CTRL0, 0x1, 20),
	.dsp_blank = VOP_REG(RK3328_DSP_CTRL0, 0x3, 18),
	.dsp_lut_en = VOP_REG(RK3328_DSP_CTRL1, 0x1, 0),
	.out_mode = VOP_REG(RK3328_DSP_CTRL0, 0xf, 0),

	.xmirror = VOP_REG(RK3328_DSP_CTRL0, 0x1, 22),
	.ymirror = VOP_REG(RK3328_DSP_CTRL0, 0x1, 23),

	.dsp_background = VOP_REG(RK3328_DSP_BG, 0xffffffff, 0),

	.bcsh_brightness = VOP_REG(RK3328_BCSH_BCS, 0xff, 0),
	.bcsh_contrast = VOP_REG(RK3328_BCSH_BCS, 0x1ff, 8),
	.bcsh_sat_con = VOP_REG(RK3328_BCSH_BCS, 0x3ff, 20),
	.bcsh_out_mode = VOP_REG(RK3328_BCSH_BCS, 0x3, 30),
	.bcsh_sin_hue = VOP_REG(RK3328_BCSH_H, 0x1ff, 0),
	.bcsh_cos_hue = VOP_REG(RK3328_BCSH_H, 0x1ff, 16),
	.bcsh_r2y_csc_mode = VOP_REG(RK3328_BCSH_CTRL, 0x3, 6),
	.bcsh_r2y_en = VOP_REG(RK3328_BCSH_CTRL, 0x1, 4),
	.bcsh_y2r_csc_mode = VOP_REG(RK3328_BCSH_CTRL, 0x3, 2),
	.bcsh_y2r_en = VOP_REG(RK3328_BCSH_CTRL, 0x1, 0),
	.bcsh_color_bar = VOP_REG(RK3328_BCSH_COLOR_BAR, 0xffffff, 8),
	.bcsh_en = VOP_REG(RK3328_BCSH_COLOR_BAR, 0x1, 0),
	.win_channel[0] = VOP_REG_VER(RK3328_WIN0_CTRL2, 0xff, 0, 3, 8, 8),
	.win_channel[1] = VOP_REG_VER(RK3328_WIN1_CTRL2, 0xff, 0, 3, 8, 8),
	.win_channel[2] = VOP_REG_VER(RK3328_WIN2_CTRL2, 0xff, 0, 3, 8, 8),

	.cfg_done = VOP_REG(RK3328_REG_CFG_DONE, 0x1, 0),
};


static const struct vop_line_flag rk3328_vop_line_flag = {
	.line_flag_num[0] = VOP_REG(RK3328_LINE_FLAG, 0xffff, 0),
	.line_flag_num[1] = VOP_REG(RK3328_LINE_FLAG, 0xffff, 16),
};

const struct vop_data rk3328_vop = {
	.version = VOP_VERSION(3, 8),
	.max_output = {4096, 2160},
	.feature = VOP_FEATURE_OUTPUT_10BIT,
	.ctrl = &rk3328_ctrl_data,
	.win = &rk3288_win01_data,
	.win_offset = 0xd0,
	.line_flag = &rk3328_vop_line_flag,
	.reg_len = RK3328_DSP_VACT_ST_END_F1 * 4,
};

static const struct vop_scl_regs rk3036_win_scl = {
	.scale_yrgb_x = VOP_REG(RK3036_WIN0_SCL_FACTOR_YRGB, 0xffff, 0x0),
	.scale_yrgb_y = VOP_REG(RK3036_WIN0_SCL_FACTOR_YRGB, 0xffff, 16),
	.scale_cbcr_x = VOP_REG(RK3036_WIN0_SCL_FACTOR_CBR, 0xffff, 0x0),
	.scale_cbcr_y = VOP_REG(RK3036_WIN0_SCL_FACTOR_CBR, 0xffff, 16),
};

static const struct vop_win rk3036_win0_data = {
	.scl = &rk3036_win_scl,
	.enable = VOP_REG(RK3036_SYS_CTRL, 0x1, 0),
	.format = VOP_REG(RK3036_SYS_CTRL, 0x7, 3),
	.rb_swap = VOP_REG(RK3036_SYS_CTRL, 0x1, 15),
	.act_info = VOP_REG(RK3036_WIN0_ACT_INFO, 0x1fff1fff, 0),
	.dsp_info = VOP_REG(RK3036_WIN0_DSP_INFO, 0x0fff0fff, 0),
	.dsp_st = VOP_REG(RK3036_WIN0_DSP_ST, 0x1fff1fff, 0),
	.yrgb_mst = VOP_REG(RK3036_WIN0_YRGB_MST, 0xffffffff, 0),
	.uv_mst = VOP_REG(RK3036_WIN0_CBR_MST, 0xffffffff, 0),
	.yrgb_vir = VOP_REG(RK3036_WIN0_VIR, 0xffff, 0),
	.uv_vir = VOP_REG(RK3036_WIN0_VIR, 0x1fff, 16),
	.alpha_mode = VOP_REG(RK3036_DSP_CTRL0, 0x1, 18),
	.alpha_en = VOP_REG(RK3036_ALPHA_CTRL, 0x1, 0)
};

static const struct vop_ctrl rk3036_ctrl_data = {
	.standby = VOP_REG(RK3036_SYS_CTRL, 0x1, 30),
	.out_mode = VOP_REG(RK3036_DSP_CTRL0, 0xf, 0),
	.dsp_blank = VOP_REG(RK3036_DSP_CTRL1, 0x1, 24),
	.pin_pol = VOP_REG(RK3036_DSP_CTRL0, 0xf, 4),
	.dsp_layer_sel = VOP_REG(RK3036_DSP_CTRL0, 0x1, 8),
	.htotal_pw = VOP_REG(RK3036_DSP_HTOTAL_HS_END, 0x1fff1fff, 0),
	.hact_st_end = VOP_REG(RK3036_DSP_HACT_ST_END, 0x1fff1fff, 0),
	.vtotal_pw = VOP_REG(RK3036_DSP_VTOTAL_VS_END, 0x1fff1fff, 0),
	.vact_st_end = VOP_REG(RK3036_DSP_VACT_ST_END, 0x1fff1fff, 0),
	.cfg_done = VOP_REG(RK3036_REG_CFG_DONE, 0x1, 0),
};

static const struct vop_line_flag rk3036_vop_line_flag = {
	.line_flag_num[0] = VOP_REG(RK3036_INT_STATUS, 0xfff, 12),
};

const struct vop_data rk3036_vop = {
	.version = VOP_VERSION(2, 2),
	.max_output = {1920, 1080},
	.ctrl = &rk3036_ctrl_data,
	.win = &rk3036_win0_data,
	.line_flag = &rk3036_vop_line_flag,
	.reg_len = RK3036_DSP_VACT_ST_END_F1 * 4,
};
