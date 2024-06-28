// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd
 *
 * Author: Wyon Bi <bivvy.bi@rock-chips.com>
 */

#include <dm.h>
#include <dm/device-internal.h>
#include <asm/unaligned.h>
#include <linux/math64.h>

#include "rk628_hdmirx.h"
#include "rk628_cru.h"

static void hdmirx_phy_write(struct rk628_hdmirx *hdmirx, u32 offset, u32 val)
{
	struct rk628 *rk628 = hdmirx->parent;

	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_ADDRESS, offset);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_DATAO, val);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_OPERATION, 1);
}

static void rk628_hdmirx_reset_control_assert(struct rk628 *rk628)
{
	/* presetn_hdmirx */
	rk628_i2c_write(rk628, CRU_SOFTRST_CON02, 0x40004);
	/* resetn_hdmirx_pon */
	rk628_i2c_write(rk628, CRU_SOFTRST_CON02, 0x10001000);
}

static void rk628_hdmirx_reset_control_deassert(struct rk628 *rk628)
{
	/* presetn_hdmirx */
	rk628_i2c_write(rk628, CRU_SOFTRST_CON02, 0x40000);
	/* resetn_hdmirx_pon */
	rk628_i2c_write(rk628, CRU_SOFTRST_CON02, 0x10000000);
}

static void rk628_hdmirx_ctrl_enable(struct rk628_hdmirx *hdmirx)
{
	struct rk628 *rk628 = hdmirx->parent;

	rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0,
	     SW_INPUT_MODE_MASK,
	     SW_INPUT_MODE(INPUT_MODE_HDMI));

	rk628_i2c_write(rk628, HDMI_RX_HDMI20_CONTROL, 0x10001f11);
	/* Support DVI mode */
	rk628_i2c_write(rk628, HDMI_RX_HDMI_TIMER_CTRL, 0xa78);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_MODE_RECOVER, 0x00000021);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_CTRL, 0xbfff8011);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_ASP_CTRL, 0x00000040);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_RESMPL_CTRL, 0x00000001);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_SYNC_CTRL, 0x00000014);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_ERR_FILTER, 0x00000008);
	rk628_i2c_write(rk628, HDMI_RX_SCDC_I2CCONFIG, 0x01000000);
	rk628_i2c_write(rk628, HDMI_RX_SCDC_CONFIG, 0x00000001);
	rk628_i2c_write(rk628, HDMI_RX_SCDC_WRDATA0, 0xabcdef01);
	rk628_i2c_write(rk628, HDMI_RX_CHLOCK_CONFIG, 0x0030c15c);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_ERROR_PROTECT, 0x000d0c98);
	rk628_i2c_write(rk628, HDMI_RX_MD_HCTRL1, 0x00000010);
	/*
	 * rk628f should set start of horizontal measurement to 3/8 of frame duration
	 * to pass hdmi 2.0 cts
	 */
	if (rk628->version == RK628D_VERSION)
		rk628_i2c_write(rk628, HDMI_RX_MD_HCTRL2, 0x00001738);
	else
		rk628_i2c_write(rk628, HDMI_RX_MD_HCTRL2, 0x0000173a);
	rk628_i2c_write(rk628, HDMI_RX_MD_VCTRL, 0x00000002);
	rk628_i2c_write(rk628, HDMI_RX_MD_VTH, 0x0000073a);
	rk628_i2c_write(rk628, HDMI_RX_MD_IL_POL, 0x00000004);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_ACRM_CTRL, 0x00000000);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_DCM_CTRL, 0x00040414);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_CKM_EVLTM, 0x00103e70);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_CKM_F, 0x0c1c0b54);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_RESMPL_CTRL, 0x00000001);

	rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_SETTINGS,
	     HDMI_RESERVED_MASK |
	     FAST_I2C_MASK |
	     ONE_DOT_ONE_MASK |
	     FAST_REAUTH_MASK,
	     HDMI_RESERVED(1) |
	     FAST_I2C(0) |
	     ONE_DOT_ONE(1) |
	     FAST_REAUTH(1));
}

static void rk628_hdmirx_phy_enable(struct rk628_hdmirx *hdmirx, bool is_hdmi2)
{
	hdmirx_phy_write(hdmirx, 0x02, 0x1860);
	hdmirx_phy_write(hdmirx, 0x03, 0x0060);
	hdmirx_phy_write(hdmirx, 0x27, 0x1c94);
	hdmirx_phy_write(hdmirx, 0x28, 0x3713);
	hdmirx_phy_write(hdmirx, 0x29, 0x24da);
	hdmirx_phy_write(hdmirx, 0x2a, 0x5492);
	hdmirx_phy_write(hdmirx, 0x2b, 0x4b0d);
	hdmirx_phy_write(hdmirx, 0x2d, 0x008c);
	hdmirx_phy_write(hdmirx, 0x2e, 0x0001);

	if (is_hdmi2)
		hdmirx_phy_write(hdmirx, 0x0e, 0x0108);
	else
		hdmirx_phy_write(hdmirx, 0x0e, 0x0008);
}

static void rk628f_hdmirx_phy_power_on(struct rk628_hdmirx *hdmirx)
{
	struct rk628 *rk628 = hdmirx->parent;

	/* power down phy */
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x17);
	udelay(30);
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x15);
	/* init phy i2c */
	rk628_i2c_write(rk628, HDMI_RX_SNPS_PHYG3_CTRL, 0);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_SS_CNTS, 0x018c01d2);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_FS_HCNT, 0x003c0081);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_MODE, 1);
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x11);
	/* enable rx phy */
	rk628_hdmirx_phy_enable(hdmirx, hdmirx->is_hdmi2);
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x14);
}

static void rk628_hdmirx_get_timing(struct rk628_hdmirx *hdmirx)
{
	u32 hact, vact, htotal, vtotal, fps, status;
	u32 val = 0;
	u32 modetclk_cnt_hs, modetclk_cnt_vs, hs, vs;
	u32 hofs_pix, hbp, hfp, vbp, vfp;
	u32 tmds_clk, tmdsclk_cnt, modetclk_hz;
	u64 tmp_data;
	u32 interlaced;
	u32 hfrontporch, hsync, hbackporch, vfrontporch, vsync, vbackporch;
	struct rk628 *rk628 = hdmirx->parent;
	unsigned long long pixelclock, clock;
	unsigned long flags = 0;

	rk628_i2c_read(rk628, HDMI_RX_SCDC_REGS1, &val);
	status = val;

	rk628_i2c_read(rk628, HDMI_RX_MD_STS, &val);
	interlaced = val & ILACE_STS ? 1 : 0;

	rk628_i2c_read(rk628, HDMI_RX_MD_HACT_PX, &val);
	hact = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VAL, &val);
	vact = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_HT1, &val);
	htotal = (val >> 16) & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VTL, &val);
	vtotal = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_HT1, &val);
	hofs_pix = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VOL, &val);
	vbp = (val & 0xffff) + 1;

	modetclk_hz = rk628_cru_clk_get_rate(rk628, CGU_CLK_CPLL) / 24;
	rk628_i2c_read(rk628, HDMI_RX_HDMI_CKM_RESULT, &val);
	tmdsclk_cnt = val & 0xffff;
	tmp_data = tmdsclk_cnt;
	/* rk628d modet clk is always 49.5m, rk628f's freq changes with ref clock */
	if (hdmirx->parent->version != RK628D_VERSION)
		tmp_data = ((tmp_data * modetclk_hz) + MODETCLK_CNT_NUM / 2);
	else
		tmp_data = ((tmp_data * MODETCLK_HZ) + MODETCLK_CNT_NUM / 2);
	do_div(tmp_data, MODETCLK_CNT_NUM);
	tmds_clk = tmp_data;
	if (!(htotal && vtotal)) {
		printf("rk628_hdmirx timing err, htotal:%d, vtotal:%d\n", htotal, vtotal);
		return;
	}
	/* rk628f should get exact frame rate frequency to pass hdmi2.0 cts */
	if (hdmirx->parent->version != RK628D_VERSION)
		fps = tmds_clk  / (htotal * vtotal);
	else
		fps = (tmds_clk + (htotal * vtotal) / 2) / (htotal * vtotal);

	rk628_i2c_read(rk628, HDMI_RX_MD_HT0, &val);
	modetclk_cnt_hs = val & 0xffff;
	hs = (tmdsclk_cnt * modetclk_cnt_hs + MODETCLK_CNT_NUM / 2) /
		MODETCLK_CNT_NUM;

	rk628_i2c_read(rk628, HDMI_RX_MD_VSC, &val);
	modetclk_cnt_vs = val & 0xffff;
	vs = (tmdsclk_cnt * modetclk_cnt_vs + MODETCLK_CNT_NUM / 2) /
		MODETCLK_CNT_NUM;
	vs = (vs + htotal / 2) / htotal;

	rk628_i2c_read(rk628, HDMI_RX_HDMI_STS, &val);
	if (val & BIT(8))
		flags |= DRM_MODE_FLAG_PHSYNC;
	else
		flags |= DRM_MODE_FLAG_NHSYNC;
	if (val & BIT(9))
		flags |= DRM_MODE_FLAG_PVSYNC;
	else
		flags |= DRM_MODE_FLAG_NVSYNC;

	if ((hofs_pix < hs) || (htotal < (hact + hofs_pix)) ||
	    (vtotal < (vact + vs + vbp))) {
		printf("rk628_hdmi timing err, total:%dx%d, act:%dx%d, hofs:%d, hs:%d, vs:%d, vbp:%d\n",
		       htotal, vtotal, hact, vact, hofs_pix, hs, vs, vbp);
		return;
	}

	hbp = hofs_pix - hs;
	hfp = htotal - hact - hofs_pix;
	vfp = vtotal - vact - vs - vbp;

	printf("rk628_hdmirx cnt_num:%d, tmds_cnt:%d, hs_cnt:%d, vs_cnt:%d, hofs:%d\n",
	       MODETCLK_CNT_NUM, tmdsclk_cnt, modetclk_cnt_hs, modetclk_cnt_vs, hofs_pix);

	hfrontporch = hfp;
	hsync = hs;
	hbackporch = hbp;
	vfrontporch = vfp;
	vsync = vs;
	vbackporch = vbp;
	/* rk628f should get exact pixel clk frequency to pass hdmi2.0 cts */
	if (hdmirx->parent->version != RK628D_VERSION)
		pixelclock = tmds_clk;
	else
		pixelclock = htotal * vtotal * fps;

	if (interlaced == 1) {
		vsync = vsync + 1;
		pixelclock /= 2;
	}

	clock = pixelclock;
	do_div(clock, 1000);

	hdmirx->mode.clock = clock;
	hdmirx->mode.hdisplay = hact;
	hdmirx->mode.hstart = hdmirx->mode.hdisplay + hfrontporch;
	hdmirx->mode.hend = hdmirx->mode.hstart + hsync;
	hdmirx->mode.htotal = hdmirx->mode.hend + hbackporch;

	hdmirx->mode.vdisplay = vact;
	hdmirx->mode.vstart = hdmirx->mode.vdisplay + vfrontporch;
	hdmirx->mode.vend = hdmirx->mode.vstart + vsync;
	hdmirx->mode.vtotal = hdmirx->mode.vend + vbackporch;
	hdmirx->mode.flags = flags;

	printf("rk628_hdmirx SCDC_REGS1:%#x, act:%dx%d, total:%dx%d, fps:%d, pixclk:%llu\n",
		 status, hact, vact, htotal, vtotal, fps, pixelclock);
	printf("rk628_hdmirx hfp:%d, hs:%d, hbp:%d, vfp:%d, vs:%d, vbp:%d, interlace:%d\n",
		 hfrontporch, hsync, hbackporch, vfrontporch, vsync, vbackporch, interlaced);
}

static int rk628_hdmirx_phy_setup(struct rk628_hdmirx *hdmirx)
{
	u32 i, cnt, val = 0, status, vs;
	u32 width, height, frame_width, frame_height, tmdsclk_cnt, modetclk_cnt_hs, modetclk_cnt_vs;
	struct drm_display_mode *src_mode;
	struct rk628 *rk628 = hdmirx->parent;
	u32 f, tmds_rate;
	bool timing_detected;

	src_mode = &rk628->src_mode;
	f = src_mode->clock;
	/*
	 * force 594m mode to yuv420 format.
	 * bit30 is used to indicate whether it is yuv420 format.
	 */
	if (hdmirx->src_mode_4K_yuv420) {
		f /= 2;
		f |= BIT(30);
	}

	tmds_rate = src_mode->clock;

	if (hdmirx->src_mode_4K_yuv420)
		tmds_rate /= 2;
	if (hdmirx->src_depth_10bit)
		tmds_rate = tmds_rate * 10 / 8;

	if (tmds_rate >= 340000)
		hdmirx->is_hdmi2 = true;
	else
		hdmirx->is_hdmi2 = false;

	/* enable scramble in hdmi2.0 mode */
	if (hdmirx->is_hdmi2)
		rk628_i2c_write(rk628, HDMI_RX_HDMI20_CONTROL, 0x10001f13);

	for (i = 0; i < RXPHY_CFG_MAX_TIMES; i++) {
		rk628f_hdmirx_phy_power_on(hdmirx);
		cnt = 0;

		do {
			cnt++;

			mdelay(100);
			rk628_i2c_read(rk628, HDMI_RX_MD_HACT_PX, &val);
			width = val & 0xffff;
			rk628_i2c_read(rk628, HDMI_RX_MD_HT1, &val);
			frame_width = (val >> 16) & 0xffff;

			rk628_i2c_read(rk628, HDMI_RX_MD_VAL, &val);
			height = val & 0xffff;
			rk628_i2c_read(rk628, HDMI_RX_MD_VTL, &val);
			frame_height = val & 0xffff;

			rk628_i2c_read(rk628, HDMI_RX_HDMI_CKM_RESULT, &val);
			tmdsclk_cnt = val & 0xffff;
			rk628_i2c_read(rk628, HDMI_RX_MD_HT0, &val);
			modetclk_cnt_hs = val & 0xffff;
			rk628_i2c_read(rk628, HDMI_RX_MD_VSC, &val);
			modetclk_cnt_vs = val & 0xffff;

			if (frame_width) {
				vs = (tmdsclk_cnt * modetclk_cnt_vs + MODETCLK_CNT_NUM / 2) /
					MODETCLK_CNT_NUM;
				vs = (vs + frame_width / 2) / frame_width;
			} else {
				vs = 0;
			}

			if (width && height && frame_width && frame_height && tmdsclk_cnt &&
			    modetclk_cnt_hs && modetclk_cnt_vs && vs)
				timing_detected = true;
			else
				timing_detected = false;

			rk628_i2c_read(rk628, HDMI_RX_SCDC_REGS1, &val);
			status = val;

			printf("rk628_hdmirx tmdsclk_cnt:%d, modetclk_cnt_hs:%d, modetclk_cnt_vs:%d,vs:%d\n",
			       tmdsclk_cnt, modetclk_cnt_hs, modetclk_cnt_vs, vs);
			printf("rk628_hdmirx read wxh:%dx%d, total:%dx%d, SCDC_REGS1:%#x, cnt:%d\n",
			       width, height, frame_width, frame_height, status, cnt);

			if (cnt >= 15)
				break;
		} while (((status & 0xfff) != 0xf00) || !timing_detected);

		if (((status & 0xfff) != 0xf00) || (((status >> 16) > 0xc000) &&
		    hdmirx->parent->version != RK628D_VERSION)) {
			printf("rk628_hdmirx %s hdmi rxphy lock failed, retry:%d, status:0x%x\n",
			       __func__, i, status);
			if (((status >> 16) > 0xc000))
				printf("rk628_hdmirx ((status >> 16) > 0xc000)\n");
			continue;
		} else {
			rk628_hdmirx_get_timing(hdmirx);

			if (hdmirx->mode.flags & DRM_MODE_FLAG_INTERLACE) {
				printf("rk628_hdmirx interlace mode is unsupported\n");
				continue;
			}

			if (hdmirx->mode.clock == 0)
				return -EINVAL;

			src_mode->clock = hdmirx->mode.clock;
			src_mode->hdisplay = hdmirx->mode.hdisplay;
			src_mode->hsync_start = hdmirx->mode.hstart;
			src_mode->hsync_end = hdmirx->mode.hend;
			src_mode->htotal = hdmirx->mode.htotal;

			src_mode->vdisplay = hdmirx->mode.vdisplay;
			src_mode->vsync_start = hdmirx->mode.vstart;
			src_mode->vsync_end = hdmirx->mode.vend;
			src_mode->vtotal = hdmirx->mode.vtotal;
			src_mode->flags = hdmirx->mode.flags;

			break;
		}
	}

	if (i == RXPHY_CFG_MAX_TIMES) {
		hdmirx->phy_lock = false;
		return -1;
	}

	hdmirx->phy_lock = true;

	return 0;
}

static void rk628_hdmirx_phy_set_clrdpt(struct rk628_hdmirx *hdmirx, bool is_8bit)
{
	if (is_8bit)
		hdmirx_phy_write(hdmirx, 0x03, 0x0000);
	else
		hdmirx_phy_write(hdmirx, 0x03, 0x0060);
}

static void rk628_hdmirx_phy_prepclk_cfg(struct rk628_hdmirx *hdmirx)
{
	struct rk628 *rk628 = hdmirx->parent;
	u32 format = 0;
	bool is_clrdpt_8bit = false;

	rk628_i2c_read(rk628, HDMI_RX_PDEC_AVI_PB, &format);
	format = (format & VIDEO_FORMAT) >> 5;

	/* yuv420 should set phy color depth 8bit */
	if (format == 3)
		is_clrdpt_8bit = true;

	rk628_i2c_read(rk628, HDMI_RX_PDEC_GCP_AVMUTE, &format);
	format = (format & PKTDEC_GCP_CD_MASK) >> 4;

	/* 10bit color depth should set phy color depth 8bit */
	if (format == 5)
		is_clrdpt_8bit = true;

	rk628_hdmirx_phy_set_clrdpt(hdmirx, is_clrdpt_8bit);
}

static bool rk628_check_signal_get(struct rk628_hdmirx *hdmirx)
{
	struct rk628 *rk628 = hdmirx->parent;
	u32 hact, vact, val = 0;

	rk628_i2c_read(rk628, HDMI_RX_MD_HACT_PX, &val);
	hact = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VAL, &val);
	vact = val & 0xffff;

	if (!hact || !vact) {
		printf("rk628_hdmirx no signal\n");
		return false;
	}

	return true;
}

static void rk628_hdmirx_video_unmute(struct rk628_hdmirx *hdmirx, u8 unmute)
{
	struct rk628 *rk628 = hdmirx->parent;

	rk628_i2c_update_bits(rk628, HDMI_RX_DMI_DISABLE_IF, VID_ENABLE_MASK, VID_ENABLE(unmute));
}

static u32 rk628_hdmirx_get_input_format(struct rk628_hdmirx *hdmirx)
{
	struct rk628 *rk628 = hdmirx->parent;
	u32 val = 0, format = 0, avi_pb = 0;
	u8 i;
	u8 cnt = 0, max_cnt = 2;

	rk628_i2c_read(rk628, HDMI_RX_PDEC_ISTS, &val);
	if (val & AVI_RCV_ISTS) {
		for (i = 0; i < 100; i++) {
			rk628_i2c_read(rk628, HDMI_RX_PDEC_AVI_PB, &format);
			dev_dbg(hdmirx->dev, "%s PDEC_AVI_PB:%#x\n", __func__, format);
			if (format && format == avi_pb) {
				if (++cnt >= max_cnt)
					break;
			} else {
				cnt = 0;
				avi_pb = format;
			}
			mdelay(30);
		}
		format  = (avi_pb & VIDEO_FORMAT) >> 5;
		switch (format) {
		case 0:
			hdmirx->input_format = BUS_FMT_RGB;
			break;
		case 1:
			hdmirx->input_format = BUS_FMT_YUV422;
			break;
		case 2:
			hdmirx->input_format = BUS_FMT_YUV444;
			break;
		case 3:
			hdmirx->input_format = BUS_FMT_YUV420;
			break;
		default:
			hdmirx->input_format = BUS_FMT_RGB;
			break;
		}

		rk628_i2c_write(rk628, HDMI_RX_PDEC_ICLR, AVI_RCV_ISTS);
	}

	return hdmirx->input_format;
}

static int rk628_hdmirx_init(struct rk628 *rk628)
{
	struct rk628_hdmirx *hdmirx = &rk628->hdmirx;

	hdmirx->parent = rk628;

	hdmirx->src_mode_4K_yuv420 = dev_read_bool(rk628->dev, "src-mode-4k-yuv420");

	hdmirx->src_depth_10bit = dev_read_bool(rk628->dev, "src-depth-10bit");

	/* HDMIRX IOMUX */
	rk628_i2c_write(rk628, GRF_GPIO1AB_SEL_CON, HIWORD_UPDATE(0x7, 10, 8));
	//i2s pinctrl
	rk628_i2c_write(rk628, GRF_GPIO0AB_SEL_CON, 0x155c155c);

	/* enable */
	rk628_i2c_write(rk628, GRF_GPIO1AB_SEL_CON, HIWORD_UPDATE(0, 0, 0));
	rk628_i2c_write(rk628, GRF_GPIO3AB_SEL_CON, HIWORD_UPDATE(1, 14, 14));

	/* if GVI and HDMITX OUT, HDMIRX missing signal */
	rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0,
			      SW_OUTPUT_RGB_MODE_MASK,
			      SW_OUTPUT_RGB_MODE(OUTPUT_MODE_RGB >> 3));

	rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0,
			      SW_INPUT_MODE_MASK, SW_INPUT_MODE(INPUT_MODE_HDMI));
	return 0;
}

int rk628_hdmirx_enable(struct rk628 *rk628)
{
	int ret;

	rk628_hdmirx_init(rk628);

	rk628_hdmirx_reset_control_assert(rk628);
	udelay(40);
	rk628_hdmirx_reset_control_deassert(rk628);
	udelay(40);

	rk628_hdmirx_ctrl_enable(&rk628->hdmirx);
	ret = rk628_hdmirx_phy_setup(&rk628->hdmirx);
	if (ret < 0) {
		dev_err(hdmirx->dev, "hdmirx channel can't lock!\n");
		return -EINVAL;
	}

	rk628_hdmirx_get_input_format(&rk628->hdmirx);
	if (rk628->version != RK628D_VERSION)
		rk628_hdmirx_phy_prepclk_cfg(&rk628->hdmirx);

	if (!rk628_check_signal_get(&rk628->hdmirx) || !rk628->hdmirx.phy_lock)
		return -EINVAL;

	printf("rk628_hdmirx success\n");

	rk628_hdmirx_video_unmute(&rk628->hdmirx, 1);

	return 0;
}
