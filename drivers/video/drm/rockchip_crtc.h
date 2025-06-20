/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROCKCHIP_CRTC_H_
#define _ROCKCHIP_CRTC_H_

#define VOP2_MAX_VP				4
#define VOP2_MAX_LAYER				8

/*
 * FBD: Fast Boot Display
 *
 * ROCKCHIP_DRM_FBD_FROM_UBOOT:
 *     show logo.bmp from uboot and show logo_kernel.bmp after enter kernel;
 * ROCKCHIP_DRM_FBD_FROM_UBOOT_TO_RTOS:
 *     crtc/connector/panel will be init at uboot, and update plane at rtos;
 * ROCKCHIP_DRM_FBD_FROM_RTOS:
 *     crtc/connector/panel will be init at rtos, uboot no need to do any hardware
 *     config, but need to pass the logic state to kernel to ensure pd/clk/drm
 *     state is continuous.
 */
#define ROCKCHIP_DRM_FBD_FROM_UBOOT		0
#define ROCKCHIP_DRM_FBD_FROM_UBOOT_TO_RTOS	1
#define ROCKCHIP_DRM_FBD_FROM_RTOS		2

struct vop2_zpos {
	u8 plane_id;
	u8 zpos;
	int global_alpha;
	int blend_mode;
};

struct rockchip_vp {
	bool enable;
	bool xmirror_en;
	bool sharp_en;
	u8 bg_ovl_dly;
	u8 primary_plane_id;
	u8 cursor_plane_id;
	u8 fbd_mode;	/* fast boot display mode */
	u8 reserved_plane_id;
	u8 dclk_div;
	u8 active_layers;
	int output_type;
	u32 plane_mask;
	struct vop2_zpos vop2_zpos[VOP2_MAX_LAYER];
};

struct rockchip_crtc {
	const struct rockchip_crtc_funcs *funcs;
	const void *data;
	struct drm_display_mode active_mode;
	struct rockchip_vp vps[4];
	bool hdmi_hpd : 1;
	bool active : 1;
	bool assign_plane : 1;
	bool splice_mode : 1;
	u8 splice_crtc_id;
};

struct rockchip_crtc_funcs {
	int (*reset) (struct udevice *dev, u32 axi, u32 vp_mask, u32 plane_mask);
	int (*preinit)(struct display_state *state);
	int (*init)(struct display_state *state);
	void (*deinit)(struct display_state *state);
	int (*set_plane)(struct display_state *state, bool reserved_plane);
	int (*prepare)(struct display_state *state);
	int (*enable)(struct display_state *state);
	int (*post_enable)(struct display_state *state);
	int (*disable)(struct display_state *state);
	void (*unprepare)(struct display_state *state);
	int (*fixup_dts)(struct display_state *state, void *blob);
	int (*send_mcu_cmd)(struct display_state *state, u32 type, u32 value);
	int (*check)(struct display_state *state);
	int (*mode_valid)(struct display_state *state);
	int (*mode_fixup)(struct display_state *state);
	int (*plane_check)(struct display_state *state);
	int (*regs_dump)(struct display_state *state);
	int (*active_regs_dump)(struct display_state *state);
	int (*apply_soft_te)(struct display_state *state);
};

struct vop_data;
struct vop2_data;
extern const struct rockchip_crtc_funcs rockchip_vop_funcs;
extern const struct rockchip_crtc_funcs rockchip_vop2_funcs;
extern const struct vop_data rk3036_vop;
extern const struct vop_data px30_vop_lit;
extern const struct vop_data px30_vop_big;
extern const struct vop_data rk3308_vop;
extern const struct vop_data rk1808_vop;
extern const struct vop_data rk3288_vop_big;
extern const struct vop_data rk3288_vop_lit;
extern const struct vop_data rk3368_vop;
extern const struct vop_data rk3366_vop;
extern const struct vop_data rk3399_vop_big;
extern const struct vop_data rk3399_vop_lit;
extern const struct vop_data rk322x_vop;
extern const struct vop_data rk3328_vop;
extern const struct vop_data rk3506_vop;
extern const struct vop_data rv1106_vop;
extern const struct vop_data rv1108_vop;
extern const struct vop_data rv1126_vop;
extern const struct vop_data rv1126b_vop;
extern const struct vop2_data rk3528_vop;
extern const struct vop2_data rk3562_vop;
extern const struct vop2_data rk3568_vop;
extern const struct vop2_data rk3576_vop;
extern const struct vop2_data rk3576_vop_lit;
extern const struct vop2_data rk3588_vop;
#endif
