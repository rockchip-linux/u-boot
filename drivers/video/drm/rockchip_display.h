/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_DISPLAY_H
#define _ROCKCHIP_DISPLAY_H

#ifdef CONFIG_SPL_BUILD
#include <linux/hdmi.h>
#include <linux/media-bus-format.h>
#else
#include <bmp_layout.h>
#include <edid.h>
#endif
#include <drm_modes.h>
#include <dm/ofnode.h>
#include <drm/drm_dsc.h>
#include <reset.h>
#include <spl_display.h>
#include <clk.h>
#include <drm/drm_color_mgmt.h>

/*
 * major: IP major version, used for IP structure
 * minor: big feature change under same structure
 * build: RTL current SVN number
 */
#define VOP_VERSION(major, minor)		((major) << 8 | (minor))
#define VOP_MAJOR(version)			((version) >> 8)
#define VOP_MINOR(version)			((version) & 0xff)

#define VOP2_VERSION(major, minor, build)	((major) << 24 | (minor) << 16 | (build))
#define VOP2_MAJOR(version)			(((version) >> 24) & 0xff)
#define VOP2_MINOR(version)			(((version) >> 16) & 0xff)
#define VOP2_BUILD(version)			((version) & 0xffff)

#define VOP_VERSION_RK3066			VOP_VERSION(2, 1)
#define VOP_VERSION_RK3036			VOP_VERSION(2, 2)
#define VOP_VERSION_RK3126			VOP_VERSION(2, 4)
#define VOP_VERSION_PX30_LITE			VOP_VERSION(2, 5)
#define VOP_VERSION_PX30_BIG			VOP_VERSION(2, 6)
#define VOP_VERSION_RK3308			VOP_VERSION(2, 7)
#define VOP_VERSION_RV1126			VOP_VERSION(2, 0xb)
#define VOP_VERSION_RV1106			VOP_VERSION(2, 0xc)
#define VOP_VERSION_RK3576_LITE			VOP_VERSION(2, 0xd)
#define VOP_VERSION_RK3506			VOP_VERSION(2, 0xe)
#define VOP_VERSION_RV1126B			VOP_VERSION(2, 0xf)
#define VOP_VERSION_RK3288			VOP_VERSION(3, 0)
#define VOP_VERSION_RK3288W			VOP_VERSION(3, 1)
#define VOP_VERSION_RK3368			VOP_VERSION(3, 2)
#define VOP_VERSION_RK3366			VOP_VERSION(3, 4)
#define VOP_VERSION_RK3399_BIG			VOP_VERSION(3, 5)
#define VOP_VERSION_RK3399_LITE			VOP_VERSION(3, 6)
#define VOP_VERSION_RK3228			VOP_VERSION(3, 7)
#define VOP_VERSION_RK3328			VOP_VERSION(3, 8)

#define VOP_VERSION_RK3528			VOP2_VERSION(0x50, 0x17, 0x1263)
#define VOP_VERSION_RK3562			VOP2_VERSION(0x50, 0x17, 0x4350)
#define VOP_VERSION_RK3568			VOP2_VERSION(0x40, 0x15, 0x8023)
#define VOP_VERSION_RK3576			VOP2_VERSION(0x50, 0x19, 0x9765)
#define VOP_VERSION_RK3588			VOP2_VERSION(0x40, 0x17, 0x6786)

#define ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE	BIT(0)
#define ROCKCHIP_OUTPUT_DUAL_CHANNEL_ODD_EVEN_MODE	BIT(1)
#define ROCKCHIP_OUTPUT_DATA_SWAP			BIT(2)
#define ROCKCHIP_OUTPUT_MIPI_DS_MODE			BIT(3)

#define ROCKCHIP_DSC_PPS_SIZE_BYTE			88

#define ROCKCHIP_VOP2_SHARED_MODE_PRIMARY		1
#define ROCKCHIP_VOP2_SHARED_MODE_SECONDARY		2

enum data_format {
	ROCKCHIP_FMT_ARGB8888 = 0,
	ROCKCHIP_FMT_RGB888,
	ROCKCHIP_FMT_RGB565,
	ROCKCHIP_FMT_YUV420SP = 4,
	ROCKCHIP_FMT_YUV422SP,
	ROCKCHIP_FMT_YUV444SP,
};

enum display_mode {
	ROCKCHIP_DISPLAY_FULLSCREEN,
	ROCKCHIP_DISPLAY_CENTER,
};

enum rockchip_cmd_type {
	CMD_TYPE_DEFAULT,
	CMD_TYPE_SPI,
	CMD_TYPE_MCU
};

enum rockchip_mcu_cmd {
	MCU_WRCMD = 0,
	MCU_WRDATA,
	MCU_SETBYPASS,
};

/*
 * display output interface supported by rockchip lcdc
 */
#define ROCKCHIP_OUT_MODE_P888		0
#define ROCKCHIP_OUT_MODE_BT1120	0
#define ROCKCHIP_OUT_MODE_P666		1
#define ROCKCHIP_OUT_MODE_P565		2
#define RK3588_EDP_OUTPUT_MODE_YUV422	3
#define ROCKCHIP_OUT_MODE_BT656		5
#define ROCKCHIP_OUT_MODE_S666		9
#define ROCKCHIP_OUT_MODE_S888		8
#define ROCKCHIP_OUT_MODE_YUV422	9
#define ROCKCHIP_OUT_MODE_S565		10
#define ROCKCHIP_OUT_MODE_S888_DUMMY	12
#define RK3588_DP_OUT_MODE_YUV422	12
#define RK3576_EDP_OUT_MODE_YUV422	12
#define RK3588_DP_OUT_MODE_YUV420	13
#define RK3576_HDMI_OUT_MODE_YUV422	13
#define ROCKCHIP_OUT_MODE_YUV420	14
/* for use special outface */
#define ROCKCHIP_OUT_MODE_AAAA		15

#define VOP_OUTPUT_IF_RGB	BIT(0)
#define VOP_OUTPUT_IF_BT1120	BIT(1)
#define VOP_OUTPUT_IF_BT656	BIT(2)
#define VOP_OUTPUT_IF_LVDS0	BIT(3)
#define VOP_OUTPUT_IF_LVDS1	BIT(4)
#define VOP_OUTPUT_IF_MIPI0	BIT(5)
#define VOP_OUTPUT_IF_MIPI1	BIT(6)
#define VOP_OUTPUT_IF_eDP0	BIT(7)
#define VOP_OUTPUT_IF_eDP1	BIT(8)
#define VOP_OUTPUT_IF_DP0	BIT(9)
#define VOP_OUTPUT_IF_DP1	BIT(10)
#define VOP_OUTPUT_IF_HDMI0	BIT(11)
#define VOP_OUTPUT_IF_HDMI1	BIT(12)
#define VOP_OUTPUT_IF_DP2	BIT(13)

#define DRM_MODE_BLEND_PREMULTI		0
#define DRM_MODE_BLEND_COVERAGE		1
#define DRM_MODE_BLEND_PIXEL_NONE	2

struct rockchip_mcu_timing {
	int mcu_pix_total;
	int mcu_cs_pst;
	int mcu_cs_pend;
	int mcu_rw_pst;
	int mcu_rw_pend;
	int mcu_hold_mode;
};

struct vop_rect {
	int width;
	int height;
};

struct vop_urgency {
	u8 urgen_thl;
	u8 urgen_thh;
};

struct rockchip_dsc_sink_cap {
	/**
	 * @slice_width: the number of pixel columns that comprise the slice width
	 * @slice_height: the number of pixel rows that comprise the slice height
	 * @block_pred: Does block prediction
	 * @native_420: Does sink support DSC with 4:2:0 compression
	 * @bpc_supported: compressed bpc supported by sink : 10, 12 or 16 bpc
	 * @version_major: DSC major version
	 * @version_minor: DSC minor version
	 * @target_bits_per_pixel_x16: bits num after compress and multiply 16
	 */
	u16 slice_width;
	u16 slice_height;
	bool block_pred;
	bool native_420;
	u8 bpc_supported;
	u8 version_major;
	u8 version_minor;
	u16 target_bits_per_pixel_x16;
};

struct display_rect {
	int x;
	int y;
	int w;
	int h;
};

struct bcsh_state {
	int brightness;
	int contrast;
	int saturation;
	int sin_hue;
	int cos_hue;
};

struct post_csc_convert_mode {
	enum drm_color_encoding intput_color_encoding;
	enum drm_color_encoding output_color_encoding;
	bool is_input_yuv;
	bool is_output_yuv;
	bool is_input_full_range;
	bool is_output_full_range;
	u8 swap_channels;	/* For now, only rg swap in DCI mode is required */
	u32 plat;		/* To distinguish platform */
	u8 pixel_depth;		/* {8, 10} */
	u8 coef_precision;	/* {8, 10, 13}, NOTE: coef_precision should be >= pixel_depth */
};

struct post_csc_coef {
	s32 csc_coef00;
	s32 csc_coef01;
	s32 csc_coef02;
	s32 csc_coef10;
	s32 csc_coef11;
	s32 csc_coef12;
	s32 csc_coef20;
	s32 csc_coef21;
	s32 csc_coef22;

	s32 csc_dc0;
	s32 csc_dc1;
	s32 csc_dc2;

	u32 range_type;
};

struct crtc_state {
	struct udevice *dev;
	struct rockchip_crtc *crtc;
	void *private;
	ofnode node;
	struct device_node *ports_node; /* if (ports_node) it's vop2; */
	struct device_node *port_node;
	struct reset_ctl dclk_rst;
	struct clk dclk;
	int crtc_id;

	int format;
	u32 dma_addr;
	int ymirror;
	int rb_swap;
	int xvir;
	int post_csc_mode;
	int dclk_core_div;
	int dclk_out_div;
	struct display_rect src_rect;
	struct display_rect crtc_rect;
	struct display_rect right_src_rect;
	struct display_rect right_crtc_rect;
	bool yuv_overlay;
	bool post_r2y_en;
	bool post_y2r_en;
	bool bcsh_en;
	bool splice_mode;
	bool soft_te;
	bool overscan_by_win_scale;
	u8 splice_crtc_id;
	u8 dsc_id;
	u8 dsc_enable;
	u8 dsc_slice_num;
	u8 dsc_pixel_num;
	bool reserved_plane_en;
	struct rockchip_mcu_timing mcu_timing;
	u32 dual_channel_swap;
	u32 feature;
	struct vop_rect max_output;

	u64 dsc_txp_clk_rate;
	u64 dsc_pxl_clk_rate;
	u64 dsc_cds_clk_rate;
	struct drm_dsc_picture_parameter_set pps;
	struct rockchip_dsc_sink_cap dsc_sink_cap;

	u32 *lut_val;
};

struct panel_state {
	struct rockchip_panel *panel;

	ofnode dsp_lut_node;
};

struct overscan {
	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
};

struct connector_state {
	struct rockchip_connector *connector;
	struct rockchip_connector *secondary;

	struct drm_display_mode mode;
	struct overscan overscan;
	u8 *edid;
	int bus_format;
	u32 bus_flags;
	int output_mode;
	int type;
	int output_if;
	int output_flags;
	int data_map_mode;
	enum drm_color_encoding color_encoding;
	enum drm_color_range color_range;
	unsigned int bpc;

	/**
	 * @hold_mode: enabled when it's:
	 * (1) mcu hold mode
	 * (2) mipi dsi cmd mode
	 * (3) edp psr mode
	 */
	bool hold_mode;

	struct base2_disp_info *disp_info; /* disp_info from baseparameter 2.0 */

	u8 dsc_id;
	u8 dsc_slice_num;
	u8 dsc_pixel_num;
	u64 dsc_txp_clk;
	u64 dsc_pxl_clk;
	u64 dsc_cds_clk;
	struct rockchip_dsc_sink_cap dsc_sink_cap;
	struct drm_dsc_picture_parameter_set pps;

	struct gpio_desc *te_gpio;

	struct {
		u32 *lut;
		int size;
	} gamma;
};

struct logo_info {
	int mode;
	int rotate;
	char *mem;
	bool ymirror;
	u32 offset;
	u32 width;
	int height;
	u32 bpp;
};

struct rockchip_logo_cache {
	struct list_head head;
	char name[20];
	struct logo_info logo;
	int logo_rotate;
};

struct display_state {
	struct list_head head;

	const void *blob;
	ofnode node;

	struct crtc_state crtc_state;
	struct connector_state conn_state;
	struct panel_state panel_state;

	char ulogo_name[30];
	char klogo_name[30];

	struct logo_info logo;
	int logo_mode;
	int charge_logo_mode;
	int logo_rotate;
	void *mem_base;
	int mem_size;

	int enable;
	int is_init;
	int is_enable;
	bool is_klogo_valid;
	bool force_output;
	bool enabled_at_spl;
	struct drm_display_mode force_mode;
	u32 force_bus_format;

	ulong vidcon_fb_addr;
};

int drm_mode_vrefresh(const struct drm_display_mode *mode);
int display_send_mcu_cmd(struct display_state *state, u32 type, u32 val);
bool drm_mode_is_420_only(const struct drm_display_info *display,
			  struct drm_display_mode *mode);
bool drm_mode_is_420_also(const struct drm_display_info *display,
			  struct drm_display_mode *mode);
bool drm_mode_is_420(const struct drm_display_info *display,
		     struct drm_display_mode *mode);
bool drm_mode_is_420_only(const struct drm_display_info *display,
		     struct drm_display_mode *mode);
struct base2_disp_info *rockchip_get_disp_info(int type, int id);

void drm_mode_max_resolution_filter(struct hdmi_edid_data *edid_data,
				    struct vop_rect *max_output);
unsigned long get_cubic_lut_buffer(int crtc_id);
int rockchip_ofnode_get_display_mode(ofnode node, struct drm_display_mode *mode,
				     u32 *bus_flags);
void drm_mode_set_crtcinfo(struct drm_display_mode *p, int adjust_flags);
void drm_mode_convert_to_origin_mode(struct drm_display_mode *mode);
void drm_mode_convert_to_split_mode(struct drm_display_mode *mode);

int display_rect_calc_hscale(struct display_rect *src, struct display_rect *dst,
			     int min_hscale, int max_hscale);
int display_rect_calc_vscale(struct display_rect *src, struct display_rect *dst,
			     int min_vscale, int max_vscale);
const struct device_node *
rockchip_of_graph_get_endpoint_by_regs(ofnode node, int port, int endpoint);
const struct device_node *
rockchip_of_graph_get_port_by_id(ofnode node, int id);
uint32_t rockchip_drm_get_cycles_per_pixel(uint32_t bus_format);
char* rockchip_get_output_if_name(u32 output_if, char *name);
int rockchip_calc_post_csc(struct csc_info *csc_cfg, struct post_csc_coef *csc_simple_coef,
			   struct post_csc_convert_mode *convert_mode);

#ifdef CONFIG_SPL_BUILD
int rockchip_spl_vop_probe(struct crtc_state *crtc_state);
int rockchip_spl_dw_hdmi_probe(struct connector_state *conn_state);
int inno_spl_hdmi_phy_probe(struct display_state *state);
#endif
#endif
