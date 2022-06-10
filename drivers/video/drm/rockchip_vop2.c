// SPDX-License-Identifier: GPL-2.0-only
/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <regmap.h>
#include <asm/arch/cpu.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <linux/list.h>
#include <linux/log2.h>
#include <linux/media-bus-format.h>
#include <clk.h>
#include <asm/arch/clock.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <dm/device.h>
#include <dm/read.h>
#include <fixp-arith.h>
#include <syscon.h>
#include <linux/iopoll.h>
#include <dm/uclass-internal.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"

/* System registers definition */
#define RK3568_REG_CFG_DONE			0x000
#define	CFG_DONE_EN				BIT(15)

#define RK3568_VERSION_INFO			0x004
#define EN_MASK					1

#define RK3568_AUTO_GATING_CTRL			0x008

#define RK3568_SYS_AXI_LUT_CTRL			0x024
#define LUT_DMA_EN_SHIFT			0

#define RK3568_DSP_IF_EN			0x028
#define RGB_EN_SHIFT				0
#define RK3588_DP0_EN_SHIFT			0
#define RK3588_DP1_EN_SHIFT			1
#define RK3588_RGB_EN_SHIFT			8
#define HDMI0_EN_SHIFT				1
#define EDP0_EN_SHIFT				3
#define RK3588_EDP0_EN_SHIFT			2
#define RK3588_HDMI0_EN_SHIFT			3
#define MIPI0_EN_SHIFT				4
#define RK3588_EDP1_EN_SHIFT			4
#define RK3588_HDMI1_EN_SHIFT			5
#define RK3588_MIPI0_EN_SHIFT                   6
#define MIPI1_EN_SHIFT				20
#define RK3588_MIPI1_EN_SHIFT                   7
#define LVDS0_EN_SHIFT				5
#define LVDS1_EN_SHIFT				24
#define BT1120_EN_SHIFT				6
#define BT656_EN_SHIFT				7
#define IF_MUX_MASK				3
#define RGB_MUX_SHIFT				8
#define HDMI0_MUX_SHIFT				10
#define RK3588_DP0_MUX_SHIFT			12
#define RK3588_DP1_MUX_SHIFT			14
#define EDP0_MUX_SHIFT				14
#define RK3588_HDMI_EDP0_MUX_SHIFT		16
#define RK3588_HDMI_EDP1_MUX_SHIFT		18
#define MIPI0_MUX_SHIFT				16
#define RK3588_MIPI0_MUX_SHIFT			20
#define MIPI1_MUX_SHIFT				21
#define LVDS0_MUX_SHIFT				18
#define LVDS1_MUX_SHIFT				25

#define RK3568_DSP_IF_CTRL			0x02c
#define LVDS_DUAL_EN_SHIFT			0
#define LVDS_DUAL_LEFT_RIGHT_EN_SHIFT		1
#define LVDS_DUAL_SWAP_EN_SHIFT			2
#define RK3568_MIPI_DUAL_EN_SHIFT		10
#define RK3588_MIPI_DSI0_MODE_SEL_SHIFT		11
#define RK3588_MIPI_DSI1_MODE_SEL_SHIFT		12

#define RK3568_DSP_IF_POL			0x030
#define IF_CTRL_REG_DONE_IMD_MASK		1
#define IF_CTRL_REG_DONE_IMD_SHIFT		28
#define IF_CRTL_MIPI_DCLK_POL_SHIT		19
#define IF_CRTL_EDP_DCLK_POL_SHIT		15
#define IF_CRTL_HDMI_DCLK_POL_SHIT		7
#define IF_CRTL_HDMI_PIN_POL_MASK		0x7
#define IF_CRTL_HDMI_PIN_POL_SHIT		4

#define RK3588_DP0_PIN_POL_SHIFT		8
#define RK3588_DP1_PIN_POL_SHIFT		12
#define RK3588_IF_PIN_POL_MASK			0x7

#define IF_CRTL_RGB_LVDS_DCLK_POL_SHIT		3

#define HDMI_EDP0_DCLK_DIV_SHIFT		16
#define HDMI_EDP0_PIXCLK_DIV_SHIFT		18
#define HDMI_EDP1_DCLK_DIV_SHIFT		20
#define HDMI_EDP1_PIXCLK_DIV_SHIFT		22
#define MIPI0_PIXCLK_DIV_SHIFT			24
#define MIPI1_PIXCLK_DIV_SHIFT			26

#define RK3568_SYS_OTP_WIN_EN			0x50
#define OTP_WIN_EN_SHIFT			0
#define RK3568_SYS_LUT_PORT_SEL			0x58
#define GAMMA_PORT_SEL_MASK			0x3
#define GAMMA_PORT_SEL_SHIFT			0
#define PORT_MERGE_EN_SHIFT			16

#define RK3568_SYS_PD_CTRL			0x034
#define RK3568_VP0_LINE_FLAG			0x70
#define RK3568_VP1_LINE_FLAG			0x74
#define RK3568_VP2_LINE_FLAG			0x78
#define RK3568_SYS0_INT_EN			0x80
#define RK3568_SYS0_INT_CLR			0x84
#define RK3568_SYS0_INT_STATUS			0x88
#define RK3568_SYS1_INT_EN			0x90
#define RK3568_SYS1_INT_CLR			0x94
#define RK3568_SYS1_INT_STATUS			0x98
#define RK3568_VP0_INT_EN			0xA0
#define RK3568_VP0_INT_CLR			0xA4
#define RK3568_VP0_INT_STATUS			0xA8
#define RK3568_VP1_INT_EN			0xB0
#define RK3568_VP1_INT_CLR			0xB4
#define RK3568_VP1_INT_STATUS			0xB8
#define RK3568_VP2_INT_EN			0xC0
#define RK3568_VP2_INT_CLR			0xC4
#define RK3568_VP2_INT_STATUS			0xC8
#define RK3588_CLUSTER0_PD_EN_SHIFT		0
#define RK3588_CLUSTER1_PD_EN_SHIFT		1
#define RK3588_CLUSTER2_PD_EN_SHIFT		2
#define RK3588_CLUSTER3_PD_EN_SHIFT		3
#define RK3588_DSC_8K_PD_EN_SHIFT		5
#define RK3588_DSC_4K_PD_EN_SHIFT		6
#define RK3588_ESMART_PD_EN_SHIFT		7

#define RK3568_SYS_STATUS0			0x60
#define RK3588_CLUSTER0_PD_STATUS_SHIFT		8
#define RK3588_CLUSTER1_PD_STATUS_SHIFT		9
#define RK3588_CLUSTER2_PD_STATUS_SHIFT		10
#define RK3588_CLUSTER3_PD_STATUS_SHIFT		11
#define RK3588_DSC_8K_PD_STATUS_SHIFT		13
#define RK3588_DSC_4K_PD_STATUS_SHIFT		14
#define RK3588_ESMART_PD_STATUS_SHIFT		15

#define RK3568_SYS_CTRL_LINE_FLAG0		0x70
#define LINE_FLAG_NUM_MASK			0x1fff
#define RK3568_DSP_LINE_FLAG_NUM0_SHIFT		0
#define RK3568_DSP_LINE_FLAG_NUM1_SHIFT		16

/* DSC CTRL registers definition */
#define RK3588_DSC_8K_SYS_CTRL			0x200
#define DSC_PORT_SEL_MASK			0x3
#define DSC_PORT_SEL_SHIFT			0
#define DSC_MAN_MODE_MASK			0x1
#define DSC_MAN_MODE_SHIFT			2
#define DSC_INTERFACE_MODE_MASK			0x3
#define DSC_INTERFACE_MODE_SHIFT		4
#define DSC_PIXEL_NUM_MASK			0x3
#define DSC_PIXEL_NUM_SHIFT			6
#define DSC_PXL_CLK_DIV_MASK			0x1
#define DSC_PXL_CLK_DIV_SHIFT			8
#define DSC_CDS_CLK_DIV_MASK			0x3
#define DSC_CDS_CLK_DIV_SHIFT			12
#define DSC_TXP_CLK_DIV_MASK			0x3
#define DSC_TXP_CLK_DIV_SHIFT			14
#define DSC_INIT_DLY_MODE_MASK			0x1
#define DSC_INIT_DLY_MODE_SHIFT			16
#define DSC_SCAN_EN_SHIFT			17
#define DSC_HALT_EN_SHIFT			18

#define RK3588_DSC_8K_RST			0x204
#define RST_DEASSERT_MASK			0x1
#define RST_DEASSERT_SHIFT			0

#define RK3588_DSC_8K_CFG_DONE			0x208
#define DSC_CFG_DONE_SHIFT			0

#define RK3588_DSC_8K_INIT_DLY			0x20C
#define DSC_INIT_DLY_NUM_MASK			0xffff
#define DSC_INIT_DLY_NUM_SHIFT			0
#define SCAN_TIMING_PARA_IMD_EN_SHIFT		16

#define RK3588_DSC_8K_HTOTAL_HS_END		0x210
#define DSC_HTOTAL_PW_MASK			0xffffffff
#define DSC_HTOTAL_PW_SHIFT			0

#define RK3588_DSC_8K_HACT_ST_END		0x214
#define DSC_HACT_ST_END_MASK			0xffffffff
#define DSC_HACT_ST_END_SHIFT			0

#define RK3588_DSC_8K_VTOTAL_VS_END		0x218
#define DSC_VTOTAL_PW_MASK			0xffffffff
#define DSC_VTOTAL_PW_SHIFT			0

#define RK3588_DSC_8K_VACT_ST_END		0x21C
#define DSC_VACT_ST_END_MASK			0xffffffff
#define DSC_VACT_ST_END_SHIFT			0

#define RK3588_DSC_8K_STATUS			0x220

/* Overlay registers definition    */
#define RK3568_OVL_CTRL				0x600
#define OVL_MODE_SEL_MASK			0x1
#define OVL_MODE_SEL_SHIFT			0
#define OVL_PORT_MUX_REG_DONE_IMD_SHIFT		28
#define RK3568_OVL_LAYER_SEL			0x604
#define LAYER_SEL_MASK				0xf

#define RK3568_OVL_PORT_SEL			0x608
#define PORT_MUX_MASK				0xf
#define PORT_MUX_SHIFT				0
#define LAYER_SEL_PORT_MASK			0x3
#define LAYER_SEL_PORT_SHIFT			16

#define RK3568_CLUSTER0_MIX_SRC_COLOR_CTRL	0x610
#define RK3568_CLUSTER0_MIX_DST_COLOR_CTRL	0x614
#define RK3568_CLUSTER0_MIX_SRC_ALPHA_CTRL	0x618
#define RK3568_CLUSTER0_MIX_DST_ALPHA_CTRL	0x61C
#define RK3568_MIX0_SRC_COLOR_CTRL		0x650
#define RK3568_MIX0_DST_COLOR_CTRL		0x654
#define RK3568_MIX0_SRC_ALPHA_CTRL		0x658
#define RK3568_MIX0_DST_ALPHA_CTRL		0x65C
#define RK3568_HDR0_SRC_COLOR_CTRL		0x6C0
#define RK3568_HDR0_DST_COLOR_CTRL		0x6C4
#define RK3568_HDR0_SRC_ALPHA_CTRL		0x6C8
#define RK3568_HDR0_DST_ALPHA_CTRL		0x6CC
#define RK3568_VP0_BG_MIX_CTRL			0x6E0
#define BG_MIX_CTRL_MASK			0xff
#define BG_MIX_CTRL_SHIFT			24
#define RK3568_VP1_BG_MIX_CTRL			0x6E4
#define RK3568_VP2_BG_MIX_CTRL			0x6E8
#define RK3568_CLUSTER_DLY_NUM			0x6F0
#define RK3568_SMART_DLY_NUM			0x6F8

/* Video Port registers definition */
#define RK3568_VP0_DSP_CTRL			0xC00
#define OUT_MODE_MASK				0xf
#define OUT_MODE_SHIFT				0
#define DATA_SWAP_MASK				0x1f
#define DATA_SWAP_SHIFT				8
#define DSP_BG_SWAP				0x1
#define DSP_RB_SWAP				0x2
#define DSP_RG_SWAP				0x4
#define DSP_DELTA_SWAP				0x8
#define CORE_DCLK_DIV_EN_SHIFT			4
#define P2I_EN_SHIFT				5
#define DSP_FILED_POL				6
#define INTERLACE_EN_SHIFT			7
#define POST_DSP_OUT_R2Y_SHIFT			15
#define PRE_DITHER_DOWN_EN_SHIFT		16
#define DITHER_DOWN_EN_SHIFT			17
#define DSP_LUT_EN_SHIFT			28

#define STANDBY_EN_SHIFT			31

#define RK3568_VP0_MIPI_CTRL			0xC04
#define DCLK_DIV2_SHIFT				4
#define DCLK_DIV2_MASK				0x3
#define MIPI_DUAL_EN_SHIFT			20
#define MIPI_DUAL_SWAP_EN_SHIFT			21
#define EDPI_TE_EN				28
#define EDPI_WMS_HOLD_EN			30
#define EDPI_WMS_FS				31


#define RK3568_VP0_COLOR_BAR_CTRL		0xC08
#define RK3568_VP0_3D_LUT_CTRL			0xC10
#define VP0_3D_LUT_EN_SHIFT				0
#define VP0_3D_LUT_UPDATE_SHIFT			2

#define RK3588_VP0_CLK_CTRL			0xC0C
#define DCLK_CORE_DIV_SHIFT			0
#define DCLK_OUT_DIV_SHIFT			2

#define RK3568_VP0_3D_LUT_MST			0xC20

#define RK3568_VP0_DSP_BG			0xC2C
#define RK3568_VP0_PRE_SCAN_HTIMING		0xC30
#define RK3568_VP0_POST_DSP_HACT_INFO		0xC34
#define RK3568_VP0_POST_DSP_VACT_INFO		0xC38
#define RK3568_VP0_POST_SCL_FACTOR_YRGB		0xC3C
#define RK3568_VP0_POST_SCL_CTRL		0xC40
#define RK3568_VP0_POST_DSP_VACT_INFO_F1	0xC44
#define RK3568_VP0_DSP_HTOTAL_HS_END		0xC48
#define RK3568_VP0_DSP_HACT_ST_END		0xC4C
#define RK3568_VP0_DSP_VTOTAL_VS_END		0xC50
#define RK3568_VP0_DSP_VACT_ST_END		0xC54
#define RK3568_VP0_DSP_VS_ST_END_F1		0xC58
#define RK3568_VP0_DSP_VACT_ST_END_F1		0xC5C

#define RK3568_VP0_BCSH_CTRL			0xC60
#define BCSH_CTRL_Y2R_SHIFT			0
#define BCSH_CTRL_Y2R_MASK			0x1
#define BCSH_CTRL_Y2R_CSC_MODE_SHIFT		2
#define BCSH_CTRL_Y2R_CSC_MODE_MASK		0x3
#define BCSH_CTRL_R2Y_SHIFT			4
#define BCSH_CTRL_R2Y_MASK			0x1
#define BCSH_CTRL_R2Y_CSC_MODE_SHIFT		6
#define BCSH_CTRL_R2Y_CSC_MODE_MASK		0x3

#define RK3568_VP0_BCSH_BCS			0xC64
#define BCSH_BRIGHTNESS_SHIFT			0
#define BCSH_BRIGHTNESS_MASK			0xFF
#define BCSH_CONTRAST_SHIFT			8
#define BCSH_CONTRAST_MASK			0x1FF
#define BCSH_SATURATION_SHIFT			20
#define BCSH_SATURATION_MASK			0x3FF
#define BCSH_OUT_MODE_SHIFT			30
#define BCSH_OUT_MODE_MASK			0x3

#define RK3568_VP0_BCSH_H			0xC68
#define BCSH_SIN_HUE_SHIFT			0
#define BCSH_SIN_HUE_MASK			0x1FF
#define BCSH_COS_HUE_SHIFT			16
#define BCSH_COS_HUE_MASK			0x1FF

#define RK3568_VP0_BCSH_COLOR			0xC6C
#define BCSH_EN_SHIFT				31
#define BCSH_EN_MASK				1

#define RK3568_VP1_DSP_CTRL			0xD00
#define RK3568_VP1_MIPI_CTRL			0xD04
#define RK3568_VP1_COLOR_BAR_CTRL		0xD08
#define RK3568_VP1_PRE_SCAN_HTIMING		0xD30
#define RK3568_VP1_POST_DSP_HACT_INFO		0xD34
#define RK3568_VP1_POST_DSP_VACT_INFO		0xD38
#define RK3568_VP1_POST_SCL_FACTOR_YRGB		0xD3C
#define RK3568_VP1_POST_SCL_CTRL		0xD40
#define RK3568_VP1_DSP_HACT_INFO		0xD34
#define RK3568_VP1_DSP_VACT_INFO		0xD38
#define RK3568_VP1_POST_DSP_VACT_INFO_F1	0xD44
#define RK3568_VP1_DSP_HTOTAL_HS_END		0xD48
#define RK3568_VP1_DSP_HACT_ST_END		0xD4C
#define RK3568_VP1_DSP_VTOTAL_VS_END		0xD50
#define RK3568_VP1_DSP_VACT_ST_END		0xD54
#define RK3568_VP1_DSP_VS_ST_END_F1		0xD58
#define RK3568_VP1_DSP_VACT_ST_END_F1		0xD5C

#define RK3568_VP2_DSP_CTRL			0xE00
#define RK3568_VP2_MIPI_CTRL			0xE04
#define RK3568_VP2_COLOR_BAR_CTRL		0xE08
#define RK3568_VP2_PRE_SCAN_HTIMING		0xE30
#define RK3568_VP2_POST_DSP_HACT_INFO		0xE34
#define RK3568_VP2_POST_DSP_VACT_INFO		0xE38
#define RK3568_VP2_POST_SCL_FACTOR_YRGB		0xE3C
#define RK3568_VP2_POST_SCL_CTRL		0xE40
#define RK3568_VP2_DSP_HACT_INFO		0xE34
#define RK3568_VP2_DSP_VACT_INFO		0xE38
#define RK3568_VP2_POST_DSP_VACT_INFO_F1	0xE44
#define RK3568_VP2_DSP_HTOTAL_HS_END		0xE48
#define RK3568_VP2_DSP_HACT_ST_END		0xE4C
#define RK3568_VP2_DSP_VTOTAL_VS_END		0xE50
#define RK3568_VP2_DSP_VACT_ST_END		0xE54
#define RK3568_VP2_DSP_VS_ST_END_F1		0xE58
#define RK3568_VP2_DSP_VACT_ST_END_F1		0xE5C

/* Cluster0 register definition */
#define RK3568_CLUSTER0_WIN0_CTRL0		0x1000
#define CLUSTER_YUV2RGB_EN_SHIFT		8
#define CLUSTER_RGB2YUV_EN_SHIFT		9
#define CLUSTER_CSC_MODE_SHIFT			10
#define CLUSTER_YRGB_XSCL_MODE_SHIFT		12
#define CLUSTER_YRGB_YSCL_MODE_SHIFT		14
#define RK3568_CLUSTER0_WIN0_CTRL1		0x1004
#define CLUSTER_YRGB_GT2_SHIFT			28
#define CLUSTER_YRGB_GT4_SHIFT			29
#define RK3568_CLUSTER0_WIN0_CTRL2		0x1008
#define CLUSTER_AXI_YRGB_ID_MASK		0x1f
#define CLUSTER_AXI_YRGB_ID_SHIFT		0
#define CLUSTER_AXI_UV_ID_MASK			0x1f
#define CLUSTER_AXI_UV_ID_SHIFT			5

#define RK3568_CLUSTER0_WIN0_YRGB_MST		0x1010
#define RK3568_CLUSTER0_WIN0_CBR_MST		0x1014
#define RK3568_CLUSTER0_WIN0_VIR		0x1018
#define RK3568_CLUSTER0_WIN0_ACT_INFO		0x1020
#define RK3568_CLUSTER0_WIN0_DSP_INFO		0x1024
#define RK3568_CLUSTER0_WIN0_DSP_ST		0x1028
#define RK3568_CLUSTER0_WIN0_SCL_FACTOR_YRGB	0x1030
#define RK3568_CLUSTER0_WIN0_AFBCD_ROTATE_MODE	0x1054
#define RK3568_CLUSTER0_WIN0_AFBCD_HDR_PTR	0x1058
#define RK3568_CLUSTER0_WIN0_AFBCD_VIR_WIDTH	0x105C
#define RK3568_CLUSTER0_WIN0_AFBCD_PIC_SIZE	0x1060
#define RK3568_CLUSTER0_WIN0_AFBCD_PIC_OFFSET	0x1064
#define RK3568_CLUSTER0_WIN0_AFBCD_DSP_OFFSET	0x1068
#define RK3568_CLUSTER0_WIN0_AFBCD_CTRL		0x106C

#define RK3568_CLUSTER0_WIN1_CTRL0		0x1080
#define RK3568_CLUSTER0_WIN1_CTRL1		0x1084
#define RK3568_CLUSTER0_WIN1_YRGB_MST		0x1090
#define RK3568_CLUSTER0_WIN1_CBR_MST		0x1094
#define RK3568_CLUSTER0_WIN1_VIR		0x1098
#define RK3568_CLUSTER0_WIN1_ACT_INFO		0x10A0
#define RK3568_CLUSTER0_WIN1_DSP_INFO		0x10A4
#define RK3568_CLUSTER0_WIN1_DSP_ST		0x10A8
#define RK3568_CLUSTER0_WIN1_SCL_FACTOR_YRGB	0x10B0
#define RK3568_CLUSTER0_WIN1_AFBCD_ROTATE_MODE	0x10D4
#define RK3568_CLUSTER0_WIN1_AFBCD_HDR_PTR	0x10D8
#define RK3568_CLUSTER0_WIN1_AFBCD_VIR_WIDTH	0x10DC
#define RK3568_CLUSTER0_WIN1_AFBCD_PIC_SIZE	0x10E0
#define RK3568_CLUSTER0_WIN1_AFBCD_PIC_OFFSET	0x10E4
#define RK3568_CLUSTER0_WIN1_AFBCD_DSP_OFFSET	0x10E8
#define RK3568_CLUSTER0_WIN1_AFBCD_CTRL		0x10EC

#define RK3568_CLUSTER0_CTRL			0x1100
#define CLUSTER_EN_SHIFT			0
#define CLUSTER_AXI_ID_MASK			0x1
#define CLUSTER_AXI_ID_SHIFT			13

#define RK3568_CLUSTER1_WIN0_CTRL0		0x1200
#define RK3568_CLUSTER1_WIN0_CTRL1		0x1204
#define RK3568_CLUSTER1_WIN0_YRGB_MST		0x1210
#define RK3568_CLUSTER1_WIN0_CBR_MST		0x1214
#define RK3568_CLUSTER1_WIN0_VIR		0x1218
#define RK3568_CLUSTER1_WIN0_ACT_INFO		0x1220
#define RK3568_CLUSTER1_WIN0_DSP_INFO		0x1224
#define RK3568_CLUSTER1_WIN0_DSP_ST		0x1228
#define RK3568_CLUSTER1_WIN0_SCL_FACTOR_YRGB	0x1230
#define RK3568_CLUSTER1_WIN0_AFBCD_ROTATE_MODE	0x1254
#define RK3568_CLUSTER1_WIN0_AFBCD_HDR_PTR	0x1258
#define RK3568_CLUSTER1_WIN0_AFBCD_VIR_WIDTH	0x125C
#define RK3568_CLUSTER1_WIN0_AFBCD_PIC_SIZE	0x1260
#define RK3568_CLUSTER1_WIN0_AFBCD_PIC_OFFSET	0x1264
#define RK3568_CLUSTER1_WIN0_AFBCD_DSP_OFFSET	0x1268
#define RK3568_CLUSTER1_WIN0_AFBCD_CTRL		0x126C

#define RK3568_CLUSTER1_WIN1_CTRL0		0x1280
#define RK3568_CLUSTER1_WIN1_CTRL1		0x1284
#define RK3568_CLUSTER1_WIN1_YRGB_MST		0x1290
#define RK3568_CLUSTER1_WIN1_CBR_MST		0x1294
#define RK3568_CLUSTER1_WIN1_VIR		0x1298
#define RK3568_CLUSTER1_WIN1_ACT_INFO		0x12A0
#define RK3568_CLUSTER1_WIN1_DSP_INFO		0x12A4
#define RK3568_CLUSTER1_WIN1_DSP_ST		0x12A8
#define RK3568_CLUSTER1_WIN1_SCL_FACTOR_YRGB	0x12B0
#define RK3568_CLUSTER1_WIN1_AFBCD_ROTATE_MODE	0x12D4
#define RK3568_CLUSTER1_WIN1_AFBCD_HDR_PTR	0x12D8
#define RK3568_CLUSTER1_WIN1_AFBCD_VIR_WIDTH	0x12DC
#define RK3568_CLUSTER1_WIN1_AFBCD_PIC_SIZE	0x12E0
#define RK3568_CLUSTER1_WIN1_AFBCD_PIC_OFFSET	0x12E4
#define RK3568_CLUSTER1_WIN1_AFBCD_DSP_OFFSET	0x12E8
#define RK3568_CLUSTER1_WIN1_AFBCD_CTRL		0x12EC

#define RK3568_CLUSTER1_CTRL			0x1300

/* Esmart register definition */
#define RK3568_ESMART0_CTRL0			0x1800
#define RGB2YUV_EN_SHIFT			1
#define CSC_MODE_SHIFT				2
#define CSC_MODE_MASK				0x3

#define RK3568_ESMART0_CTRL1			0x1804
#define ESMART_AXI_YRGB_ID_MASK			0x1f
#define ESMART_AXI_YRGB_ID_SHIFT		4
#define ESMART_AXI_UV_ID_MASK			0x1f
#define ESMART_AXI_UV_ID_SHIFT			12
#define YMIRROR_EN_SHIFT			31

#define RK3568_ESMART0_AXI_CTRL			0x1808
#define ESMART_AXI_ID_MASK			0x1
#define ESMART_AXI_ID_SHIFT			1

#define RK3568_ESMART0_REGION0_CTRL		0x1810
#define REGION0_RB_SWAP_SHIFT			14
#define WIN_EN_SHIFT				0
#define WIN_FORMAT_MASK				0x1f
#define WIN_FORMAT_SHIFT			1

#define RK3568_ESMART0_REGION0_YRGB_MST		0x1814
#define RK3568_ESMART0_REGION0_CBR_MST		0x1818
#define RK3568_ESMART0_REGION0_VIR		0x181C
#define RK3568_ESMART0_REGION0_ACT_INFO		0x1820
#define RK3568_ESMART0_REGION0_DSP_INFO		0x1824
#define RK3568_ESMART0_REGION0_DSP_ST		0x1828
#define RK3568_ESMART0_REGION0_SCL_CTRL		0x1830
#define YRGB_XSCL_MODE_MASK			0x3
#define YRGB_XSCL_MODE_SHIFT			0
#define YRGB_XSCL_FILTER_MODE_MASK		0x3
#define YRGB_XSCL_FILTER_MODE_SHIFT		2
#define YRGB_YSCL_MODE_MASK			0x3
#define YRGB_YSCL_MODE_SHIFT			4
#define YRGB_YSCL_FILTER_MODE_MASK		0x3
#define YRGB_YSCL_FILTER_MODE_SHIFT		6

#define RK3568_ESMART0_REGION0_SCL_FACTOR_YRGB	0x1834
#define RK3568_ESMART0_REGION0_SCL_FACTOR_CBR	0x1838
#define RK3568_ESMART0_REGION0_SCL_OFFSET	0x183C
#define RK3568_ESMART0_REGION1_CTRL		0x1840
#define YRGB_GT2_MASK				0x1
#define YRGB_GT2_SHIFT				8
#define YRGB_GT4_MASK				0x1
#define YRGB_GT4_SHIFT				9

#define RK3568_ESMART0_REGION1_YRGB_MST		0x1844
#define RK3568_ESMART0_REGION1_CBR_MST		0x1848
#define RK3568_ESMART0_REGION1_VIR		0x184C
#define RK3568_ESMART0_REGION1_ACT_INFO		0x1850
#define RK3568_ESMART0_REGION1_DSP_INFO		0x1854
#define RK3568_ESMART0_REGION1_DSP_ST		0x1858
#define RK3568_ESMART0_REGION1_SCL_CTRL		0x1860
#define RK3568_ESMART0_REGION1_SCL_FACTOR_YRGB	0x1864
#define RK3568_ESMART0_REGION1_SCL_FACTOR_CBR	0x1868
#define RK3568_ESMART0_REGION1_SCL_OFFSET	0x186C
#define RK3568_ESMART0_REGION2_CTRL		0x1870
#define RK3568_ESMART0_REGION2_YRGB_MST		0x1874
#define RK3568_ESMART0_REGION2_CBR_MST		0x1878
#define RK3568_ESMART0_REGION2_VIR		0x187C
#define RK3568_ESMART0_REGION2_ACT_INFO		0x1880
#define RK3568_ESMART0_REGION2_DSP_INFO		0x1884
#define RK3568_ESMART0_REGION2_DSP_ST		0x1888
#define RK3568_ESMART0_REGION2_SCL_CTRL		0x1890
#define RK3568_ESMART0_REGION2_SCL_FACTOR_YRGB	0x1894
#define RK3568_ESMART0_REGION2_SCL_FACTOR_CBR	0x1898
#define RK3568_ESMART0_REGION2_SCL_OFFSET	0x189C
#define RK3568_ESMART0_REGION3_CTRL		0x18A0
#define RK3568_ESMART0_REGION3_YRGB_MST		0x18A4
#define RK3568_ESMART0_REGION3_CBR_MST		0x18A8
#define RK3568_ESMART0_REGION3_VIR		0x18AC
#define RK3568_ESMART0_REGION3_ACT_INFO		0x18B0
#define RK3568_ESMART0_REGION3_DSP_INFO		0x18B4
#define RK3568_ESMART0_REGION3_DSP_ST		0x18B8
#define RK3568_ESMART0_REGION3_SCL_CTRL		0x18C0
#define RK3568_ESMART0_REGION3_SCL_FACTOR_YRGB	0x18C4
#define RK3568_ESMART0_REGION3_SCL_FACTOR_CBR	0x18C8
#define RK3568_ESMART0_REGION3_SCL_OFFSET	0x18CC

#define RK3568_ESMART1_CTRL0			0x1A00
#define RK3568_ESMART1_CTRL1			0x1A04
#define RK3568_ESMART1_REGION0_CTRL		0x1A10
#define RK3568_ESMART1_REGION0_YRGB_MST		0x1A14
#define RK3568_ESMART1_REGION0_CBR_MST		0x1A18
#define RK3568_ESMART1_REGION0_VIR		0x1A1C
#define RK3568_ESMART1_REGION0_ACT_INFO		0x1A20
#define RK3568_ESMART1_REGION0_DSP_INFO		0x1A24
#define RK3568_ESMART1_REGION0_DSP_ST		0x1A28
#define RK3568_ESMART1_REGION0_SCL_CTRL		0x1A30
#define RK3568_ESMART1_REGION0_SCL_FACTOR_YRGB	0x1A34
#define RK3568_ESMART1_REGION0_SCL_FACTOR_CBR	0x1A38
#define RK3568_ESMART1_REGION0_SCL_OFFSET	0x1A3C
#define RK3568_ESMART1_REGION1_CTRL		0x1A40
#define RK3568_ESMART1_REGION1_YRGB_MST		0x1A44
#define RK3568_ESMART1_REGION1_CBR_MST		0x1A48
#define RK3568_ESMART1_REGION1_VIR		0x1A4C
#define RK3568_ESMART1_REGION1_ACT_INFO		0x1A50
#define RK3568_ESMART1_REGION1_DSP_INFO		0x1A54
#define RK3568_ESMART1_REGION1_DSP_ST		0x1A58
#define RK3568_ESMART1_REGION1_SCL_CTRL		0x1A60
#define RK3568_ESMART1_REGION1_SCL_FACTOR_YRGB	0x1A64
#define RK3568_ESMART1_REGION1_SCL_FACTOR_CBR	0x1A68
#define RK3568_ESMART1_REGION1_SCL_OFFSET	0x1A6C
#define RK3568_ESMART1_REGION2_CTRL		0x1A70
#define RK3568_ESMART1_REGION2_YRGB_MST		0x1A74
#define RK3568_ESMART1_REGION2_CBR_MST		0x1A78
#define RK3568_ESMART1_REGION2_VIR		0x1A7C
#define RK3568_ESMART1_REGION2_ACT_INFO		0x1A80
#define RK3568_ESMART1_REGION2_DSP_INFO		0x1A84
#define RK3568_ESMART1_REGION2_DSP_ST		0x1A88
#define RK3568_ESMART1_REGION2_SCL_CTRL		0x1A90
#define RK3568_ESMART1_REGION2_SCL_FACTOR_YRGB	0x1A94
#define RK3568_ESMART1_REGION2_SCL_FACTOR_CBR	0x1A98
#define RK3568_ESMART1_REGION2_SCL_OFFSET	0x1A9C
#define RK3568_ESMART1_REGION3_CTRL		0x1AA0
#define RK3568_ESMART1_REGION3_YRGB_MST		0x1AA4
#define RK3568_ESMART1_REGION3_CBR_MST		0x1AA8
#define RK3568_ESMART1_REGION3_VIR		0x1AAC
#define RK3568_ESMART1_REGION3_ACT_INFO		0x1AB0
#define RK3568_ESMART1_REGION3_DSP_INFO		0x1AB4
#define RK3568_ESMART1_REGION3_DSP_ST		0x1AB8
#define RK3568_ESMART1_REGION3_SCL_CTRL		0x1AC0
#define RK3568_ESMART1_REGION3_SCL_FACTOR_YRGB	0x1AC4
#define RK3568_ESMART1_REGION3_SCL_FACTOR_CBR	0x1AC8
#define RK3568_ESMART1_REGION3_SCL_OFFSET	0x1ACC

#define RK3568_SMART0_CTRL0			0x1C00
#define RK3568_SMART0_CTRL1			0x1C04
#define RK3568_SMART0_REGION0_CTRL		0x1C10
#define RK3568_SMART0_REGION0_YRGB_MST		0x1C14
#define RK3568_SMART0_REGION0_CBR_MST		0x1C18
#define RK3568_SMART0_REGION0_VIR		0x1C1C
#define RK3568_SMART0_REGION0_ACT_INFO		0x1C20
#define RK3568_SMART0_REGION0_DSP_INFO		0x1C24
#define RK3568_SMART0_REGION0_DSP_ST		0x1C28
#define RK3568_SMART0_REGION0_SCL_CTRL		0x1C30
#define RK3568_SMART0_REGION0_SCL_FACTOR_YRGB	0x1C34
#define RK3568_SMART0_REGION0_SCL_FACTOR_CBR	0x1C38
#define RK3568_SMART0_REGION0_SCL_OFFSET	0x1C3C
#define RK3568_SMART0_REGION1_CTRL		0x1C40
#define RK3568_SMART0_REGION1_YRGB_MST		0x1C44
#define RK3568_SMART0_REGION1_CBR_MST		0x1C48
#define RK3568_SMART0_REGION1_VIR		0x1C4C
#define RK3568_SMART0_REGION1_ACT_INFO		0x1C50
#define RK3568_SMART0_REGION1_DSP_INFO		0x1C54
#define RK3568_SMART0_REGION1_DSP_ST		0x1C58
#define RK3568_SMART0_REGION1_SCL_CTRL		0x1C60
#define RK3568_SMART0_REGION1_SCL_FACTOR_YRGB	0x1C64
#define RK3568_SMART0_REGION1_SCL_FACTOR_CBR	0x1C68
#define RK3568_SMART0_REGION1_SCL_OFFSET	0x1C6C
#define RK3568_SMART0_REGION2_CTRL		0x1C70
#define RK3568_SMART0_REGION2_YRGB_MST		0x1C74
#define RK3568_SMART0_REGION2_CBR_MST		0x1C78
#define RK3568_SMART0_REGION2_VIR		0x1C7C
#define RK3568_SMART0_REGION2_ACT_INFO		0x1C80
#define RK3568_SMART0_REGION2_DSP_INFO		0x1C84
#define RK3568_SMART0_REGION2_DSP_ST		0x1C88
#define RK3568_SMART0_REGION2_SCL_CTRL		0x1C90
#define RK3568_SMART0_REGION2_SCL_FACTOR_YRGB	0x1C94
#define RK3568_SMART0_REGION2_SCL_FACTOR_CBR	0x1C98
#define RK3568_SMART0_REGION2_SCL_OFFSET	0x1C9C
#define RK3568_SMART0_REGION3_CTRL		0x1CA0
#define RK3568_SMART0_REGION3_YRGB_MST		0x1CA4
#define RK3568_SMART0_REGION3_CBR_MST		0x1CA8
#define RK3568_SMART0_REGION3_VIR		0x1CAC
#define RK3568_SMART0_REGION3_ACT_INFO		0x1CB0
#define RK3568_SMART0_REGION3_DSP_INFO		0x1CB4
#define RK3568_SMART0_REGION3_DSP_ST		0x1CB8
#define RK3568_SMART0_REGION3_SCL_CTRL		0x1CC0
#define RK3568_SMART0_REGION3_SCL_FACTOR_YRGB	0x1CC4
#define RK3568_SMART0_REGION3_SCL_FACTOR_CBR	0x1CC8
#define RK3568_SMART0_REGION3_SCL_OFFSET	0x1CCC

#define RK3568_SMART1_CTRL0			0x1E00
#define RK3568_SMART1_CTRL1			0x1E04
#define RK3568_SMART1_REGION0_CTRL		0x1E10
#define RK3568_SMART1_REGION0_YRGB_MST		0x1E14
#define RK3568_SMART1_REGION0_CBR_MST		0x1E18
#define RK3568_SMART1_REGION0_VIR		0x1E1C
#define RK3568_SMART1_REGION0_ACT_INFO		0x1E20
#define RK3568_SMART1_REGION0_DSP_INFO		0x1E24
#define RK3568_SMART1_REGION0_DSP_ST		0x1E28
#define RK3568_SMART1_REGION0_SCL_CTRL		0x1E30
#define RK3568_SMART1_REGION0_SCL_FACTOR_YRGB	0x1E34
#define RK3568_SMART1_REGION0_SCL_FACTOR_CBR	0x1E38
#define RK3568_SMART1_REGION0_SCL_OFFSET	0x1E3C
#define RK3568_SMART1_REGION1_CTRL		0x1E40
#define RK3568_SMART1_REGION1_YRGB_MST		0x1E44
#define RK3568_SMART1_REGION1_CBR_MST		0x1E48
#define RK3568_SMART1_REGION1_VIR		0x1E4C
#define RK3568_SMART1_REGION1_ACT_INFO		0x1E50
#define RK3568_SMART1_REGION1_DSP_INFO		0x1E54
#define RK3568_SMART1_REGION1_DSP_ST		0x1E58
#define RK3568_SMART1_REGION1_SCL_CTRL		0x1E60
#define RK3568_SMART1_REGION1_SCL_FACTOR_YRGB	0x1E64
#define RK3568_SMART1_REGION1_SCL_FACTOR_CBR	0x1E68
#define RK3568_SMART1_REGION1_SCL_OFFSET	0x1E6C
#define RK3568_SMART1_REGION2_CTRL		0x1E70
#define RK3568_SMART1_REGION2_YRGB_MST		0x1E74
#define RK3568_SMART1_REGION2_CBR_MST		0x1E78
#define RK3568_SMART1_REGION2_VIR		0x1E7C
#define RK3568_SMART1_REGION2_ACT_INFO		0x1E80
#define RK3568_SMART1_REGION2_DSP_INFO		0x1E84
#define RK3568_SMART1_REGION2_DSP_ST		0x1E88
#define RK3568_SMART1_REGION2_SCL_CTRL		0x1E90
#define RK3568_SMART1_REGION2_SCL_FACTOR_YRGB	0x1E94
#define RK3568_SMART1_REGION2_SCL_FACTOR_CBR	0x1E98
#define RK3568_SMART1_REGION2_SCL_OFFSET	0x1E9C
#define RK3568_SMART1_REGION3_CTRL		0x1EA0
#define RK3568_SMART1_REGION3_YRGB_MST		0x1EA4
#define RK3568_SMART1_REGION3_CBR_MST		0x1EA8
#define RK3568_SMART1_REGION3_VIR		0x1EAC
#define RK3568_SMART1_REGION3_ACT_INFO		0x1EB0
#define RK3568_SMART1_REGION3_DSP_INFO		0x1EB4
#define RK3568_SMART1_REGION3_DSP_ST		0x1EB8
#define RK3568_SMART1_REGION3_SCL_CTRL		0x1EC0
#define RK3568_SMART1_REGION3_SCL_FACTOR_YRGB	0x1EC4
#define RK3568_SMART1_REGION3_SCL_FACTOR_CBR	0x1EC8
#define RK3568_SMART1_REGION3_SCL_OFFSET	0x1ECC

/* DSC 8K/4K register definition */
#define RK3588_DSC_8K_PPS0_3			0x4000
#define RK3588_DSC_8K_CTRL0			0x40A0
#define DSC_EN_SHIFT				0
#define DSC_RBIT_SHIFT				2
#define DSC_RBYT_SHIFT				3
#define DSC_FLAL_SHIFT				4
#define DSC_MER_SHIFT				5
#define DSC_EPB_SHIFT				6
#define DSC_EPL_SHIFT				7
#define DSC_NSLC_SHIFT				16
#define DSC_SBO_SHIFT				28
#define DSC_IFEP_SHIFT				29
#define DSC_PPS_UPD_SHIFT			31

#define RK3588_DSC_8K_CTRL1			0x40A4
#define RK3588_DSC_8K_STS0			0x40A8
#define RK3588_DSC_8K_ERS			0x40C4

#define RK3588_DSC_4K_PPS0_3			0x4100
#define RK3588_DSC_4K_CTRL0			0x41A0
#define RK3588_DSC_4K_CTRL1			0x41A4
#define RK3588_DSC_4K_STS0			0x41A8
#define RK3588_DSC_4K_ERS			0x41C4

#define RK3568_MAX_REG				0x1ED0

#define RK3568_GRF_VO_CON1			0x0364
#define GRF_BT656_CLK_INV_SHIFT			1
#define GRF_BT1120_CLK_INV_SHIFT		2
#define GRF_RGB_DCLK_INV_SHIFT			3

#define RK3588_GRF_VOP_CON2			0x0008
#define RK3588_GRF_EDP0_ENABLE_SHIFT		0
#define RK3588_GRF_HDMITX0_ENABLE_SHIFT		1
#define RK3588_GRF_EDP1_ENABLE_SHIFT		3
#define RK3588_GRF_HDMITX1_ENABLE_SHIFT		4

#define RK3588_GRF_VO1_CON0			0x0000
#define HDMI_SYNC_POL_MASK			0x3
#define HDMI0_SYNC_POL_SHIFT			5
#define HDMI1_SYNC_POL_SHIFT			7

#define RK3588_PMU_BISR_CON3			0x20C
#define RK3588_PD_CLUSTER0_REPAIR_EN_SHIFT	9
#define RK3588_PD_CLUSTER1_REPAIR_EN_SHIFT	10
#define RK3588_PD_CLUSTER2_REPAIR_EN_SHIFT	11
#define RK3588_PD_CLUSTER3_REPAIR_EN_SHIFT	12
#define RK3588_PD_DSC_8K_REPAIR_EN_SHIFT	13
#define RK3588_PD_DSC_4K_REPAIR_EN_SHIFT	14
#define RK3588_PD_ESMART_REPAIR_EN_SHIFT	15

#define RK3588_PMU_BISR_STATUS5			0x294
#define RK3588_PD_CLUSTER0_PWR_STAT_SHIFI	9
#define RK3588_PD_CLUSTER1_PWR_STAT_SHIFI	10
#define RK3588_PD_CLUSTER2_PWR_STAT_SHIFI	11
#define RK3588_PD_CLUSTER3_PWR_STAT_SHIFI	12
#define RK3588_PD_DSC_8K_PWR_STAT_SHIFI		13
#define RK3588_PD_DSC_4K_PWR_STAT_SHIFI		14
#define RK3588_PD_ESMART_PWR_STAT_SHIFI		15

#define VOP2_LAYER_MAX				8

#define VOP2_MAX_VP_OUTPUT_WIDTH		4096

#define VOP_FEATURE_OUTPUT_10BIT		BIT(0)

/* KHz */
#define VOP2_MAX_DCLK_RATE			600000

/*
 * vop2 dsc id
 */
#define ROCKCHIP_VOP2_DSC_8K	0
#define ROCKCHIP_VOP2_DSC_4K	1

/*
 * vop2 internal power domain id,
 * should be all none zero, 0 will be
 * treat as invalid;
 */
#define VOP2_PD_CLUSTER0			BIT(0)
#define VOP2_PD_CLUSTER1			BIT(1)
#define VOP2_PD_CLUSTER2			BIT(2)
#define VOP2_PD_CLUSTER3			BIT(3)
#define VOP2_PD_DSC_8K				BIT(5)
#define VOP2_PD_DSC_4K				BIT(6)
#define VOP2_PD_ESMART				BIT(7)

enum vop2_csc_format {
	CSC_BT601L,
	CSC_BT709L,
	CSC_BT601F,
	CSC_BT2020,
};

enum vop2_pol {
	HSYNC_POSITIVE = 0,
	VSYNC_POSITIVE = 1,
	DEN_NEGATIVE   = 2,
	DCLK_INVERT    = 3
};

enum vop2_bcsh_out_mode {
	BCSH_OUT_MODE_BLACK,
	BCSH_OUT_MODE_BLUE,
	BCSH_OUT_MODE_COLOR_BAR,
	BCSH_OUT_MODE_NORMAL_VIDEO,
};

#define _VOP_REG(off, _mask, _shift, _write_mask) \
		{ \
		 .offset = off, \
		 .mask = _mask, \
		 .shift = _shift, \
		 .write_mask = _write_mask, \
		}

#define VOP_REG(off, _mask, _shift) \
		_VOP_REG(off, _mask, _shift, false)
enum dither_down_mode {
	RGB888_TO_RGB565 = 0x0,
	RGB888_TO_RGB666 = 0x1
};

enum vop2_video_ports_id {
	VOP2_VP0,
	VOP2_VP1,
	VOP2_VP2,
	VOP2_VP3,
	VOP2_VP_MAX,
};

enum vop2_layer_type {
	CLUSTER_LAYER = 0,
	ESMART_LAYER = 1,
	SMART_LAYER = 2,
};

/* This define must same with kernel win phy id */
enum vop2_layer_phy_id {
	ROCKCHIP_VOP2_CLUSTER0 = 0,
	ROCKCHIP_VOP2_CLUSTER1,
	ROCKCHIP_VOP2_ESMART0,
	ROCKCHIP_VOP2_ESMART1,
	ROCKCHIP_VOP2_SMART0,
	ROCKCHIP_VOP2_SMART1,
	ROCKCHIP_VOP2_CLUSTER2,
	ROCKCHIP_VOP2_CLUSTER3,
	ROCKCHIP_VOP2_ESMART2,
	ROCKCHIP_VOP2_ESMART3,
	ROCKCHIP_VOP2_LAYER_MAX,
};

enum vop2_scale_up_mode {
	VOP2_SCALE_UP_NRST_NBOR,
	VOP2_SCALE_UP_BIL,
	VOP2_SCALE_UP_BIC,
};

enum vop2_scale_down_mode {
	VOP2_SCALE_DOWN_NRST_NBOR,
	VOP2_SCALE_DOWN_BIL,
	VOP2_SCALE_DOWN_AVG,
};

enum scale_mode {
	SCALE_NONE = 0x0,
	SCALE_UP   = 0x1,
	SCALE_DOWN = 0x2
};

enum vop_dsc_interface_mode {
	VOP_DSC_IF_DISABLE = 0,
	VOP_DSC_IF_HDMI = 1,
	VOP_DSC_IF_MIPI_DS_MODE = 2,
	VOP_DSC_IF_MIPI_VIDEO_MODE = 3,
};

struct vop2_layer {
	u8 id;
	/**
	 * @win_phys_id: window id of the layer selected.
	 * Every layer must make sure to select different
	 * windows of others.
	 */
	u8 win_phys_id;
};

struct vop2_power_domain_data {
	u8 id;
	u8 parent_id;
	/*
	 * @module_id_mask: module id of which module this power domain is belongs to.
	 * PD_CLUSTER0,1,2,3 only belongs to CLUSTER0/1/2/3, PD_Esmart0 shared by Esmart1/2/3
	 */
	u32 module_id_mask;
};

struct vop2_win_data {
	char *name;
	u8 phys_id;
	enum vop2_layer_type type;
	u8 win_sel_port_offset;
	u8 layer_sel_win_id;
	u8 axi_id;
	u8 axi_uv_id;
	u8 axi_yrgb_id;
	u8 splice_win_id;
	u8 pd_id;
	u32 reg_offset;
	bool splice_mode_right;
};

struct vop2_vp_data {
	u32 feature;
	u8 pre_scan_max_dly;
	u8 splice_vp_id;
	struct vop_rect max_output;
	u32 max_dclk;
};

struct vop2_plane_table {
	enum vop2_layer_phy_id plane_id;
	enum vop2_layer_type plane_type;
};

struct vop2_vp_plane_mask {
	u8 primary_plane_id; /* use this win to show logo */
	u8 attached_layers_nr; /* number layers attach to this vp */
	u8 attached_layers[VOP2_LAYER_MAX]; /* the layers attached to this vp */
	u32 plane_mask;
	int cursor_plane_id;
};

struct vop2_dsc_data {
	u8 id;
	u8 pd_id;
	u8 max_slice_num;
	u8 max_linebuf_depth;	/* used to generate the bitstream */
	u8 min_bits_per_pixel;	/* bit num after encoder compress */
	const char *dsc_txp_clk_src_name;
	const char *dsc_txp_clk_name;
	const char *dsc_pxl_clk_name;
	const char *dsc_cds_clk_name;
};

struct dsc_error_info {
	u32 dsc_error_val;
	char dsc_error_info[50];
};

struct vop2_data {
	u32 version;
	struct vop2_vp_data *vp_data;
	struct vop2_win_data *win_data;
	struct vop2_vp_plane_mask *plane_mask;
	struct vop2_plane_table *plane_table;
	struct vop2_power_domain_data *pd;
	struct vop2_dsc_data *dsc;
	struct dsc_error_info *dsc_error_ecw;
	struct dsc_error_info *dsc_error_buffer_flow;
	u8 nr_vps;
	u8 nr_layers;
	u8 nr_mixers;
	u8 nr_gammas;
	u8 nr_pd;
	u8 nr_dscs;
	u8 nr_dsc_ecw;
	u8 nr_dsc_buffer_flow;
	u32 reg_len;
};

struct vop2 {
	u32 *regsbak;
	void *regs;
	void *grf;
	void *vop_grf;
	void *vo1_grf;
	void *sys_pmu;
	u32 reg_len;
	u32 version;
	bool global_init;
	const struct vop2_data *data;
	struct vop2_vp_plane_mask vp_plane_mask[VOP2_VP_MAX];
};

static struct vop2 *rockchip_vop2;
/*
 * bli_sd_factor = (src - 1) / (dst - 1) << 12;
 * avg_sd_factor:
 * bli_su_factor:
 * bic_su_factor:
 * = (src - 1) / (dst - 1) << 16;
 *
 * gt2 enable: dst get one line from two line of the src
 * gt4 enable: dst get one line from four line of the src.
 *
 */
#define VOP2_BILI_SCL_DN(src, dst)	(((src - 1) << 12) / (dst - 1))
#define VOP2_COMMON_SCL(src, dst)	(((src - 1) << 16) / (dst - 1))

#define VOP2_BILI_SCL_FAC_CHECK(src, dst, fac)	 \
				(fac * (dst - 1) >> 12 < (src - 1))
#define VOP2_COMMON_SCL_FAC_CHECK(src, dst, fac) \
				(fac * (dst - 1) >> 16 < (src - 1))

static uint16_t vop2_scale_factor(enum scale_mode mode,
				  int32_t filter_mode,
				  uint32_t src, uint32_t dst)
{
	uint32_t fac = 0;
	int i = 0;

	if (mode == SCALE_NONE)
		return 0;

	/*
	 * A workaround to avoid zero div.
	 */
	if ((dst == 1) || (src == 1)) {
		dst = dst + 1;
		src = src + 1;
	}

	if ((mode == SCALE_DOWN) && (filter_mode == VOP2_SCALE_DOWN_BIL)) {
		fac = VOP2_BILI_SCL_DN(src, dst);
		for (i = 0; i < 100; i++) {
			if (VOP2_BILI_SCL_FAC_CHECK(src, dst, fac))
				break;
			fac -= 1;
			printf("down fac cali: src:%d, dst:%d, fac:0x%x\n", src, dst, fac);
		}
	} else {
		fac = VOP2_COMMON_SCL(src, dst);
		for (i = 0; i < 100; i++) {
			if (VOP2_COMMON_SCL_FAC_CHECK(src, dst, fac))
				break;
			fac -= 1;
			printf("up fac cali:  src:%d, dst:%d, fac:0x%x\n", src, dst, fac);
		}
	}

	return fac;
}

static inline enum scale_mode scl_get_scl_mode(int src, int dst)
{
	if (src < dst)
		return SCALE_UP;
	else if (src > dst)
		return SCALE_DOWN;

	return SCALE_NONE;
}

static u8 rk3588_vop2_vp_primary_plane_order[VOP2_VP_MAX] = {
	ROCKCHIP_VOP2_ESMART0,
	ROCKCHIP_VOP2_ESMART1,
	ROCKCHIP_VOP2_ESMART2,
	ROCKCHIP_VOP2_ESMART3,
};

static u8 rk3568_vop2_vp_primary_plane_order[VOP2_VP_MAX] = {
	ROCKCHIP_VOP2_SMART0,
	ROCKCHIP_VOP2_SMART1,
	ROCKCHIP_VOP2_ESMART1,
};

static inline int interpolate(int x1, int y1, int x2, int y2, int x)
{
	return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}

static int vop2_get_primary_plane(struct vop2 *vop2, u32 plane_mask)
{
	int i = 0;
	u8 *vop2_vp_primary_plane_order;
	u8 default_primary_plane;

	if (vop2->version == VOP_VERSION_RK3588) {
		vop2_vp_primary_plane_order = rk3588_vop2_vp_primary_plane_order;
		default_primary_plane = ROCKCHIP_VOP2_ESMART0;
	} else {
		vop2_vp_primary_plane_order = rk3568_vop2_vp_primary_plane_order;
		default_primary_plane = ROCKCHIP_VOP2_SMART0;
	}

	for (i = 0; i < vop2->data->nr_vps; i++) {
		if (plane_mask & BIT(vop2_vp_primary_plane_order[i]))
			return vop2_vp_primary_plane_order[i];
	}

	return default_primary_plane;
}

static inline u16 scl_cal_scale(int src, int dst, int shift)
{
	return ((src * 2 - 3) << (shift - 1)) / (dst - 1);
}

static inline u16 scl_cal_scale2(int src, int dst)
{
	return ((src - 1) << 12) / (dst - 1);
}

static inline void vop2_writel(struct vop2 *vop2, u32 offset, u32 v)
{
	writel(v, vop2->regs + offset);
	vop2->regsbak[offset >> 2] = v;
}

static inline u32 vop2_readl(struct vop2 *vop2, u32 offset)
{
	return readl(vop2->regs + offset);
}

static inline void vop2_mask_write(struct vop2 *vop2, u32 offset,
				   u32 mask, u32 shift, u32 v,
				   bool write_mask)
{
	if (!mask)
		return;

	if (write_mask) {
		v = ((v & mask) << shift) | (mask << (shift + 16));
	} else {
		u32 cached_val = vop2->regsbak[offset >> 2];

		v = (cached_val & ~(mask << shift)) | ((v & mask) << shift);
		vop2->regsbak[offset >> 2] = v;
	}

	writel(v, vop2->regs + offset);
}

static inline void vop2_grf_writel(struct vop2 *vop, void *grf_base, u32 offset,
				   u32 mask, u32 shift, u32 v)
{
	u32 val = 0;

	val = (v << shift) | (mask << (shift + 16));
	writel(val, grf_base + offset);
}

static inline u32 vop2_grf_readl(struct vop2 *vop, void *grf_base, u32 offset,
				  u32 mask, u32 shift)
{
	return (readl(grf_base + offset) >> shift) & mask;
}

static char* get_output_if_name(u32 output_if, char *name)
{
	if (output_if & VOP_OUTPUT_IF_RGB)
		strcat(name, " RGB");
	if (output_if & VOP_OUTPUT_IF_BT1120)
		strcat(name, " BT1120");
	if (output_if & VOP_OUTPUT_IF_BT656)
		strcat(name, " BT656");
	if (output_if & VOP_OUTPUT_IF_LVDS0)
		strcat(name, " LVDS0");
	if (output_if & VOP_OUTPUT_IF_LVDS1)
		strcat(name, " LVDS1");
	if (output_if & VOP_OUTPUT_IF_MIPI0)
		strcat(name, " MIPI0");
	if (output_if & VOP_OUTPUT_IF_MIPI1)
		strcat(name, " MIPI1");
	if (output_if & VOP_OUTPUT_IF_eDP0)
		strcat(name, " eDP0");
	if (output_if & VOP_OUTPUT_IF_eDP1)
		strcat(name, " eDP1");
	if (output_if & VOP_OUTPUT_IF_DP0)
		strcat(name, " DP0");
	if (output_if & VOP_OUTPUT_IF_DP1)
		strcat(name, " DP1");
	if (output_if & VOP_OUTPUT_IF_HDMI0)
		strcat(name, " HDMI0");
	if (output_if & VOP_OUTPUT_IF_HDMI1)
		strcat(name, " HDMI1");

	return name;
}

static char *get_plane_name(int plane_id, char *name)
{
	switch (plane_id) {
	case ROCKCHIP_VOP2_CLUSTER0:
		strcat(name, "Cluster0");
		break;
	case ROCKCHIP_VOP2_CLUSTER1:
		strcat(name, "Cluster1");
		break;
	case ROCKCHIP_VOP2_ESMART0:
		strcat(name, "Esmart0");
		break;
	case ROCKCHIP_VOP2_ESMART1:
		strcat(name, "Esmart1");
		break;
	case ROCKCHIP_VOP2_SMART0:
		strcat(name, "Smart0");
		break;
	case ROCKCHIP_VOP2_SMART1:
		strcat(name, "Smart1");
		break;
	case ROCKCHIP_VOP2_CLUSTER2:
		strcat(name, "Cluster2");
		break;
	case ROCKCHIP_VOP2_CLUSTER3:
		strcat(name, "Cluster3");
		break;
	case ROCKCHIP_VOP2_ESMART2:
		strcat(name, "Esmart2");
		break;
	case ROCKCHIP_VOP2_ESMART3:
		strcat(name, "Esmart3");
		break;
	}

	return name;
}

static bool is_yuv_output(u32 bus_format)
{
	switch (bus_format) {
	case MEDIA_BUS_FMT_YUV8_1X24:
	case MEDIA_BUS_FMT_YUV10_1X30:
	case MEDIA_BUS_FMT_UYYVYY8_0_5X24:
	case MEDIA_BUS_FMT_UYYVYY10_0_5X30:
	case MEDIA_BUS_FMT_YUYV8_2X8:
	case MEDIA_BUS_FMT_YVYU8_2X8:
	case MEDIA_BUS_FMT_UYVY8_2X8:
	case MEDIA_BUS_FMT_VYUY8_2X8:
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_YVYU8_1X16:
	case MEDIA_BUS_FMT_UYVY8_1X16:
	case MEDIA_BUS_FMT_VYUY8_1X16:
		return true;
	default:
		return false;
	}
}

static int vop2_convert_csc_mode(int csc_mode)
{
	switch (csc_mode) {
	case V4L2_COLORSPACE_SMPTE170M:
	case V4L2_COLORSPACE_470_SYSTEM_M:
	case V4L2_COLORSPACE_470_SYSTEM_BG:
		return CSC_BT601L;
	case V4L2_COLORSPACE_REC709:
	case V4L2_COLORSPACE_SMPTE240M:
	case V4L2_COLORSPACE_DEFAULT:
		return CSC_BT709L;
	case V4L2_COLORSPACE_JPEG:
		return CSC_BT601F;
	case V4L2_COLORSPACE_BT2020:
		return CSC_BT2020;
	default:
		return CSC_BT709L;
	}
}

static bool is_uv_swap(u32 bus_format, u32 output_mode)
{
	/*
	 * FIXME:
	 *
	 * There is no media type for YUV444 output,
	 * so when out_mode is AAAA or P888, assume output is YUV444 on
	 * yuv format.
	 *
	 * From H/W testing, YUV444 mode need a rb swap.
	 */
	if (bus_format == MEDIA_BUS_FMT_YVYU8_1X16 ||
	    bus_format == MEDIA_BUS_FMT_VYUY8_1X16 ||
	    bus_format == MEDIA_BUS_FMT_YVYU8_2X8 ||
	    bus_format == MEDIA_BUS_FMT_VYUY8_2X8 ||
	    ((bus_format == MEDIA_BUS_FMT_YUV8_1X24 ||
	     bus_format == MEDIA_BUS_FMT_YUV10_1X30) &&
	    (output_mode == ROCKCHIP_OUT_MODE_AAAA ||
	     output_mode == ROCKCHIP_OUT_MODE_P888)))
		return true;
	else
		return false;
}

static inline bool is_hot_plug_devices(int output_type)
{
	switch (output_type) {
	case DRM_MODE_CONNECTOR_HDMIA:
	case DRM_MODE_CONNECTOR_HDMIB:
	case DRM_MODE_CONNECTOR_TV:
	case DRM_MODE_CONNECTOR_DisplayPort:
	case DRM_MODE_CONNECTOR_VGA:
	case DRM_MODE_CONNECTOR_Unknown:
		return true;
	default:
		return false;
	}
}

static struct vop2_win_data *vop2_find_win_by_phys_id(struct vop2 *vop2, int phys_id)
{
	int i = 0;

	for (i = 0; i < vop2->data->nr_layers; i++) {
		if (vop2->data->win_data[i].phys_id == phys_id)
			return &vop2->data->win_data[i];
	}

	return NULL;
}

static struct vop2_power_domain_data *vop2_find_pd_data_by_id(struct vop2 *vop2, int pd_id)
{
	int i = 0;

	for (i = 0; i < vop2->data->nr_pd; i++) {
		if (vop2->data->pd[i].id == pd_id)
			return &vop2->data->pd[i];
	}

	return NULL;
}

static int rockchip_vop2_gamma_lut_init(struct vop2 *vop2,
					struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct crtc_state *cstate = &state->crtc_state;
	struct resource gamma_res;
	fdt_size_t lut_size;
	int i, lut_len, ret = 0;
	u32 *lut_regs;
	u32 *lut_val;
	u32 r, g, b;
	u32 vp_offset = cstate->crtc_id * 0x100;
	struct base2_disp_info *disp_info = conn_state->disp_info;
	static int gamma_lut_en_num = 1;

	if (gamma_lut_en_num > vop2->data->nr_gammas) {
		printf("warn: only %d vp support gamma\n", vop2->data->nr_gammas);
		return 0;
	}

	if (!disp_info)
		return 0;

	if (!disp_info->gamma_lut_data.size)
		return 0;

	ret = ofnode_read_resource_byname(cstate->node, "gamma_lut", &gamma_res);
	if (ret)
		printf("failed to get gamma lut res\n");
	lut_regs = (u32 *)gamma_res.start;
	lut_size = gamma_res.end - gamma_res.start + 1;
	if (lut_regs == (u32 *)FDT_ADDR_T_NONE) {
		printf("failed to get gamma lut register\n");
		return 0;
	}
	lut_len = lut_size / 4;
	if (lut_len != 256 && lut_len != 1024) {
		printf("Warning: unsupport gamma lut table[%d]\n", lut_len);
		return 0;
	}
	lut_val = (u32 *)calloc(1, lut_size);
	for (i = 0; i < lut_len; i++) {
		r = disp_info->gamma_lut_data.lred[i] * (lut_len - 1) / 0xffff;
		g = disp_info->gamma_lut_data.lgreen[i] * (lut_len - 1) / 0xffff;
		b = disp_info->gamma_lut_data.lblue[i] * (lut_len - 1) / 0xffff;

		lut_val[i] = b * lut_len * lut_len + g * lut_len + r;
	}

	for (i = 0; i < lut_len; i++)
		writel(lut_val[i], lut_regs + i);

	vop2_mask_write(vop2, RK3568_SYS_LUT_PORT_SEL,
			GAMMA_PORT_SEL_MASK, GAMMA_PORT_SEL_SHIFT,
			cstate->crtc_id , false);
	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset,
			EN_MASK, DSP_LUT_EN_SHIFT, 1, false);
	gamma_lut_en_num++;

	return 0;
}

static int rockchip_vop2_cubic_lut_init(struct vop2 *vop2,
					struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct crtc_state *cstate = &state->crtc_state;
	int i, cubic_lut_len;
	u32 vp_offset = cstate->crtc_id * 0x100;
	struct base2_disp_info *disp_info = conn_state->disp_info;
	struct base2_cubic_lut_data *lut = &conn_state->disp_info->cubic_lut_data;
	u32 *cubic_lut_addr;

	if (!disp_info || CONFIG_ROCKCHIP_CUBIC_LUT_SIZE == 0)
		return 0;

	if (!disp_info->cubic_lut_data.size)
		return 0;

	cubic_lut_addr = (u32 *)get_cubic_lut_buffer(cstate->crtc_id);
	cubic_lut_len = disp_info->cubic_lut_data.size;

	for (i = 0; i < cubic_lut_len / 2; i++) {
		*cubic_lut_addr++ = ((lut->lred[2 * i]) & 0xfff) +
					((lut->lgreen[2 * i] & 0xfff) << 12) +
					((lut->lblue[2 * i] & 0xff) << 24);
		*cubic_lut_addr++ = ((lut->lblue[2 * i] & 0xf00) >> 8) +
					((lut->lred[2 * i + 1] & 0xfff) << 4) +
					((lut->lgreen[2 * i + 1] & 0xfff) << 16) +
					((lut->lblue[2 * i + 1] & 0xf) << 28);
		*cubic_lut_addr++ = (lut->lblue[2 * i + 1] & 0xff0) >> 4;
		*cubic_lut_addr++ = 0;
	}

	if (cubic_lut_len % 2) {
		*cubic_lut_addr++ = (lut->lred[2 * i] & 0xfff) +
					((lut->lgreen[2 * i] & 0xfff) << 12) +
					((lut->lblue[2 * i] & 0xff) << 24);
		*cubic_lut_addr++ = (lut->lblue[2 * i] & 0xf00) >> 8;
		*cubic_lut_addr++ = 0;
		*cubic_lut_addr = 0;
	}

	vop2_writel(vop2, RK3568_VP0_3D_LUT_MST + vp_offset,
		    get_cubic_lut_buffer(cstate->crtc_id));
	vop2_mask_write(vop2, RK3568_SYS_AXI_LUT_CTRL,
			EN_MASK, LUT_DMA_EN_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3568_VP0_3D_LUT_CTRL + vp_offset,
			EN_MASK, VP0_3D_LUT_EN_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3568_VP0_3D_LUT_CTRL + vp_offset,
			EN_MASK, VP0_3D_LUT_UPDATE_SHIFT, 1, false);

	return 0;
}

static void vop2_bcsh_reg_update(struct display_state *state, struct vop2 *vop2,
				 struct bcsh_state *bcsh_state, int crtc_id)
{
	struct crtc_state *cstate = &state->crtc_state;
	u32 vp_offset = crtc_id * 0x100;

	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_R2Y_MASK,
			BCSH_CTRL_R2Y_SHIFT, cstate->post_r2y_en, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_Y2R_MASK,
			BCSH_CTRL_Y2R_SHIFT, cstate->post_y2r_en, false);

	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_R2Y_CSC_MODE_MASK,
			BCSH_CTRL_R2Y_CSC_MODE_SHIFT, cstate->post_csc_mode, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_Y2R_CSC_MODE_MASK,
			BCSH_CTRL_Y2R_CSC_MODE_SHIFT, cstate->post_csc_mode, false);

	if (!cstate->bcsh_en) {
		vop2_mask_write(vop2, RK3568_VP0_BCSH_COLOR + vp_offset,
				BCSH_EN_MASK, BCSH_EN_SHIFT, 0, false);
		return;
	}

	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_BRIGHTNESS_MASK, BCSH_BRIGHTNESS_SHIFT,
			bcsh_state->brightness, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_CONTRAST_MASK, BCSH_CONTRAST_SHIFT, bcsh_state->contrast, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_SATURATION_MASK, BCSH_SATURATION_SHIFT,
			bcsh_state->saturation * bcsh_state->contrast / 0x100, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_H + vp_offset,
			BCSH_SIN_HUE_MASK, BCSH_SIN_HUE_SHIFT, bcsh_state->sin_hue, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_H + vp_offset,
			BCSH_COS_HUE_MASK, BCSH_COS_HUE_SHIFT, bcsh_state->cos_hue, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_OUT_MODE_MASK, BCSH_OUT_MODE_SHIFT,
			BCSH_OUT_MODE_NORMAL_VIDEO, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_COLOR + vp_offset,
			BCSH_EN_MASK, BCSH_EN_SHIFT, 1, false);
}

static void vop2_tv_config_update(struct display_state *state, struct vop2 *vop2)
{
	struct connector_state *conn_state = &state->conn_state;
	struct base_bcsh_info *bcsh_info;
	struct crtc_state *cstate = &state->crtc_state;
	struct bcsh_state bcsh_state;
	int brightness, contrast, saturation, hue, sin_hue, cos_hue;

	if (!conn_state->disp_info)
		return;
	bcsh_info = &conn_state->disp_info->bcsh_info;
	if (!bcsh_info)
		return;

	if (bcsh_info->brightness != 50 ||
	    bcsh_info->contrast != 50 ||
	    bcsh_info->saturation != 50 || bcsh_info->hue != 50)
		cstate->bcsh_en = true;

	if (cstate->bcsh_en) {
		if (!cstate->yuv_overlay)
			cstate->post_r2y_en = 1;
		if (!is_yuv_output(conn_state->bus_format))
			cstate->post_y2r_en = 1;
	} else {
		if (!cstate->yuv_overlay && is_yuv_output(conn_state->bus_format))
			cstate->post_r2y_en = 1;
		if (cstate->yuv_overlay && !is_yuv_output(conn_state->bus_format))
			cstate->post_y2r_en = 1;
	}

	cstate->post_csc_mode = vop2_convert_csc_mode(conn_state->color_space);

	if (cstate->feature & VOP_FEATURE_OUTPUT_10BIT)
		brightness = interpolate(0, -128, 100, 127,
					 bcsh_info->brightness);
	else
		brightness = interpolate(0, -32, 100, 31,
					 bcsh_info->brightness);
	contrast = interpolate(0, 0, 100, 511, bcsh_info->contrast);
	saturation = interpolate(0, 0, 100, 511, bcsh_info->saturation);
	hue = interpolate(0, -30, 100, 30, bcsh_info->hue);


	/*
	 *  a:[-30~0):
	 *    sin_hue = 0x100 - sin(a)*256;
	 *    cos_hue = cos(a)*256;
	 *  a:[0~30]
	 *    sin_hue = sin(a)*256;
	 *    cos_hue = cos(a)*256;
	 */
	sin_hue = fixp_sin32(hue) >> 23;
	cos_hue = fixp_cos32(hue) >> 23;

	bcsh_state.brightness = brightness;
	bcsh_state.contrast = contrast;
	bcsh_state.saturation = saturation;
	bcsh_state.sin_hue = sin_hue;
	bcsh_state.cos_hue = cos_hue;

	vop2_bcsh_reg_update(state, vop2, &bcsh_state, cstate->crtc_id);
	if (cstate->splice_mode)
		vop2_bcsh_reg_update(state, vop2, &bcsh_state, cstate->splice_crtc_id);
}

static void vop2_setup_dly_for_vp(struct display_state *state, struct vop2 *vop2, int crtc_id)
{
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct crtc_state *cstate = &state->crtc_state;
	u32 bg_ovl_dly, bg_dly, pre_scan_dly;
	u16 hdisplay = mode->crtc_hdisplay;
	u16 hsync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;

	bg_ovl_dly = cstate->crtc->vps[crtc_id].bg_ovl_dly;
	bg_dly =  vop2->data->vp_data[crtc_id].pre_scan_max_dly;
	bg_dly -= bg_ovl_dly;

	if (cstate->splice_mode)
		pre_scan_dly = bg_dly + (hdisplay >> 2) - 1;
	else
		pre_scan_dly = bg_dly + (hdisplay >> 1) - 1;

	if (vop2->version == VOP_VERSION_RK3588 && hsync_len < 8)
		hsync_len = 8;
	pre_scan_dly = (pre_scan_dly << 16) | hsync_len;
	vop2_mask_write(vop2, RK3568_VP0_BG_MIX_CTRL + crtc_id * 4,
			BG_MIX_CTRL_MASK, BG_MIX_CTRL_SHIFT, bg_dly, false);
	vop2_writel(vop2, RK3568_VP0_PRE_SCAN_HTIMING + (crtc_id * 0x100), pre_scan_dly);
}

static void vop2_post_config(struct display_state *state, struct vop2 *vop2)
{
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct crtc_state *cstate = &state->crtc_state;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u16 vtotal = mode->crtc_vtotal;
	u16 hact_st = mode->crtc_htotal - mode->crtc_hsync_start;
	u16 vact_st = mode->crtc_vtotal - mode->crtc_vsync_start;
	u16 hdisplay = mode->crtc_hdisplay;
	u16 vdisplay = mode->crtc_vdisplay;
	u16 hsize =
	    hdisplay * (conn_state->overscan.left_margin +
			conn_state->overscan.right_margin) / 200;
	u16 vsize =
	    vdisplay * (conn_state->overscan.top_margin +
			conn_state->overscan.bottom_margin) / 200;
	u16 hact_end, vact_end;
	u32 val;

	hsize = round_down(hsize, 2);
	vsize = round_down(vsize, 2);

	hact_st += hdisplay * (100 - conn_state->overscan.left_margin) / 200;
	hact_end = hact_st + hsize;
	val = hact_st << 16;
	val |= hact_end;

	vop2_writel(vop2, RK3568_VP0_POST_DSP_HACT_INFO + vp_offset, val);
	vact_st += vdisplay * (100 - conn_state->overscan.top_margin) / 200;
	vact_end = vact_st + vsize;
	val = vact_st << 16;
	val |= vact_end;
	vop2_writel(vop2, RK3568_VP0_POST_DSP_VACT_INFO + vp_offset, val);
	val = scl_cal_scale2(vdisplay, vsize) << 16;
	val |= scl_cal_scale2(hdisplay, hsize);
	vop2_writel(vop2, RK3568_VP0_POST_SCL_FACTOR_YRGB + vp_offset, val);
#define POST_HORIZONTAL_SCALEDOWN_EN(x)		((x) << 0)
#define POST_VERTICAL_SCALEDOWN_EN(x)		((x) << 1)
	vop2_writel(vop2, RK3568_VP0_POST_SCL_CTRL + vp_offset,
		    POST_HORIZONTAL_SCALEDOWN_EN(hdisplay != hsize) |
		    POST_VERTICAL_SCALEDOWN_EN(vdisplay != vsize));
	if (mode->flags & DRM_MODE_FLAG_INTERLACE) {
		u16 vact_st_f1 = vtotal + vact_st + 1;
		u16 vact_end_f1 = vact_st_f1 + vsize;

		val = vact_st_f1 << 16 | vact_end_f1;
		vop2_writel(vop2, RK3568_VP0_POST_DSP_VACT_INFO_F1 + vp_offset, val);
	}

	vop2_setup_dly_for_vp(state, vop2, cstate->crtc_id);
	if (cstate->splice_mode)
		vop2_setup_dly_for_vp(state, vop2, cstate->splice_crtc_id);
}

/*
 * Read VOP internal power domain on/off status.
 * We should query BISR_STS register in PMU for
 * power up/down status when memory repair is enabled.
 * Return value: 1 for power on, 0 for power off;
 */
static int vop2_wait_power_domain_on(struct vop2 *vop2, struct vop2_power_domain_data *pd_data)
{
	int val = 0;
	int shift = 0;
	int shift_factor = 0;
	bool is_bisr_en = false;

	/*
	 * The order of pd status bits in BISR_STS register
	 * is different from that in VOP SYS_STS register.
	 */
	if (pd_data->id == VOP2_PD_DSC_8K ||
	    pd_data->id == VOP2_PD_DSC_4K ||
	    pd_data->id == VOP2_PD_ESMART)
			shift_factor = 1;

	shift = RK3588_PD_CLUSTER0_REPAIR_EN_SHIFT + generic_ffs(pd_data->id) - 1 - shift_factor;
	is_bisr_en = vop2_grf_readl(vop2, vop2->sys_pmu, RK3588_PMU_BISR_CON3, EN_MASK, shift);
	if (is_bisr_en) {
		shift = RK3588_PD_CLUSTER0_PWR_STAT_SHIFI + generic_ffs(pd_data->id) - 1 - shift_factor;

		return readl_poll_timeout(vop2->sys_pmu + RK3588_PMU_BISR_STATUS5, val,
					  ((val >> shift) & 0x1), 50 * 1000);
	} else {
		shift = RK3588_CLUSTER0_PD_STATUS_SHIFT + generic_ffs(pd_data->id) - 1;

		return readl_poll_timeout(vop2->regs + RK3568_SYS_STATUS0, val,
					  !((val >> shift) & 0x1), 50 * 1000);
	}
}

static int vop2_power_domain_on(struct vop2 *vop2, int pd_id)
{
	struct vop2_power_domain_data *pd_data;
	int ret = 0;

	if (!pd_id)
		return 0;

	pd_data = vop2_find_pd_data_by_id(vop2, pd_id);
	if (!pd_data) {
		printf("can't find pd_data by id\n");
		return -EINVAL;
	}

	if (pd_data->parent_id) {
		ret = vop2_power_domain_on(vop2, pd_data->parent_id);
		if (ret) {
			printf("can't open parent power domain\n");
			return -EINVAL;
		}
	}

	vop2_mask_write(vop2, RK3568_SYS_PD_CTRL, EN_MASK,
			RK3588_CLUSTER0_PD_EN_SHIFT + generic_ffs(pd_id) - 1, 0, false);
	ret = vop2_wait_power_domain_on(vop2, pd_data);
	if (ret) {
		printf("wait vop2 power domain timeout\n");
		return ret;
	}

	return 0;
}

static void rk3588_vop2_regsbak(struct vop2 *vop2)
{
	u32 *base = vop2->regs;
	int i = 0;

	/*
	 * No need to backup HDR/DSC/GAMMA_LUT/BPP_LUT/MMU
	 */
	for (i = 0; i < (vop2->reg_len >> 2); i++)
		vop2->regsbak[i] = base[i];
}

static void vop2_global_initial(struct vop2 *vop2, struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	int i, j, port_mux = 0, total_used_layer = 0;
	u8 shift = 0;
	int layer_phy_id = 0;
	u32 layer_nr = 0;
	struct vop2_win_data *win_data;
	struct vop2_vp_plane_mask *plane_mask;

	if (vop2->global_init)
		return;

	/* OTP must enable at the first time, otherwise mirror layer register is error */
	if (soc_is_rk3566())
		vop2_mask_write(vop2, RK3568_SYS_OTP_WIN_EN, EN_MASK,
				OTP_WIN_EN_SHIFT, 1, false);

	if (cstate->crtc->assign_plane) {/* dts assign plane */
		u32 plane_mask;
		int primary_plane_id;

		for (i = 0; i < vop2->data->nr_vps; i++) {
			plane_mask = cstate->crtc->vps[i].plane_mask;
			vop2->vp_plane_mask[i].plane_mask = plane_mask;
			layer_nr = hweight32(plane_mask); /* use bitmap to store plane mask */
			vop2->vp_plane_mask[i].attached_layers_nr = layer_nr;
			primary_plane_id = vop2_get_primary_plane(vop2, plane_mask);
			vop2->vp_plane_mask[i].primary_plane_id =  primary_plane_id;
			vop2->vp_plane_mask[i].plane_mask = plane_mask;

			/* plane mask[bitmap] convert into layer phy id[enum vop2_layer_phy_id]*/
			for (j = 0; j < layer_nr; j++) {
				vop2->vp_plane_mask[i].attached_layers[j] = ffs(plane_mask) - 1;
				plane_mask &= ~BIT(vop2->vp_plane_mask[i].attached_layers[j]);
			}
		}
	} else {/* need soft assign plane mask */
		/* find the first unplug devices and set it as main display */
		int main_vp_index = -1;
		int active_vp_num = 0;

		for (i = 0; i < vop2->data->nr_vps; i++) {
			if (cstate->crtc->vps[i].enable)
				active_vp_num++;
		}
		printf("VOP have %d active VP\n", active_vp_num);

		if (soc_is_rk3566() && active_vp_num > 2)
			printf("ERROR: rk3566 only support 2 display output!!\n");
		plane_mask = vop2->data->plane_mask;
		plane_mask += (active_vp_num - 1) * VOP2_VP_MAX;

		for (i = 0; i < vop2->data->nr_vps; i++) {
			if (!is_hot_plug_devices(cstate->crtc->vps[i].output_type)) {
				vop2->vp_plane_mask[i] = plane_mask[0]; /* the first store main display plane mask*/
				main_vp_index = i;
				break;
			}
		}

		/* if no find unplug devices, use vp0 as main display */
		if (main_vp_index < 0) {
			main_vp_index = 0;
			vop2->vp_plane_mask[0] = plane_mask[0];
		}

		j = 1; /* plane_mask[0] store main display, so we from plane_mask[1] */

		/* init other display except main display */
		for (i = 0; i < vop2->data->nr_vps; i++) {
			if (i == main_vp_index || !cstate->crtc->vps[i].enable) /* main display or no connect devices */
				continue;
			vop2->vp_plane_mask[i] = plane_mask[j++];
		}

		/* store plane mask for vop2_fixup_dts */
		for (i = 0; i < vop2->data->nr_vps; i++) {
			layer_nr = vop2->vp_plane_mask[i].attached_layers_nr;
			for (j = 0; j < layer_nr; j++) {
				layer_phy_id = vop2->vp_plane_mask[i].attached_layers[j];
				vop2->vp_plane_mask[i].plane_mask |= BIT(layer_phy_id);
			}
		}
	}

	if (vop2->version == VOP_VERSION_RK3588)
		rk3588_vop2_regsbak(vop2);
	else
		memcpy(vop2->regsbak, vop2->regs, vop2->reg_len);

	vop2_mask_write(vop2, RK3568_OVL_CTRL, EN_MASK,
			OVL_PORT_MUX_REG_DONE_IMD_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
			IF_CTRL_REG_DONE_IMD_SHIFT, 1, false);

	for (i = 0; i < vop2->data->nr_vps; i++) {
		printf("vp%d have layer nr:%d[", i, vop2->vp_plane_mask[i].attached_layers_nr);
		for (j = 0; j < vop2->vp_plane_mask[i].attached_layers_nr; j++)
			printf("%d ", vop2->vp_plane_mask[i].attached_layers[j]);
		printf("], primary plane: %d\n", vop2->vp_plane_mask[i].primary_plane_id);
	}

	shift = 0;
	/* layer sel win id */
	for (i = 0; i < vop2->data->nr_vps; i++) {
		layer_nr = vop2->vp_plane_mask[i].attached_layers_nr;
		for (j = 0; j < layer_nr; j++) {
			layer_phy_id = vop2->vp_plane_mask[i].attached_layers[j];
			win_data = vop2_find_win_by_phys_id(vop2, layer_phy_id);
			vop2_mask_write(vop2, RK3568_OVL_LAYER_SEL, LAYER_SEL_MASK,
					shift, win_data->layer_sel_win_id, false);
			shift += 4;
		}
	}

	/* win sel port */
	for (i = 0; i < vop2->data->nr_vps; i++) {
		layer_nr = vop2->vp_plane_mask[i].attached_layers_nr;
		for (j = 0; j < layer_nr; j++) {
			if (!vop2->vp_plane_mask[i].attached_layers[j])
				continue;
			layer_phy_id = vop2->vp_plane_mask[i].attached_layers[j];
			win_data = vop2_find_win_by_phys_id(vop2, layer_phy_id);
			shift = win_data->win_sel_port_offset * 2;
			vop2_mask_write(vop2, RK3568_OVL_PORT_SEL, LAYER_SEL_PORT_MASK,
					LAYER_SEL_PORT_SHIFT + shift, i, false);
		}
	}

	/**
	 * port mux config
	 */
	for (i = 0; i < vop2->data->nr_vps; i++) {
		shift = i * 4;
		if (vop2->vp_plane_mask[i].attached_layers_nr) {
			total_used_layer += vop2->vp_plane_mask[i].attached_layers_nr;
			port_mux = total_used_layer - 1;
		} else {
			port_mux = 8;
		}

		if (i == vop2->data->nr_vps - 1)
			port_mux = vop2->data->nr_mixers;

		cstate->crtc->vps[i].bg_ovl_dly = (vop2->data->nr_mixers - port_mux) << 1;
		vop2_mask_write(vop2, RK3568_OVL_PORT_SEL, PORT_MUX_MASK,
				PORT_MUX_SHIFT + shift, port_mux, false);
	}

	if (vop2->version == VOP_VERSION_RK3568)
		vop2_writel(vop2, RK3568_AUTO_GATING_CTRL, 0);

	vop2->global_init = true;
}

static int vop2_initial(struct vop2 *vop2, struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	int ret;

	/* Process 'assigned-{clocks/clock-parents/clock-rates}' properties */
	ret = clk_set_defaults(cstate->dev);
	if (ret)
		debug("%s clk_set_defaults failed %d\n", __func__, ret);

	rockchip_vop2_gamma_lut_init(vop2, state);
	rockchip_vop2_cubic_lut_init(vop2, state);

	return 0;
}

/*
 * VOP2 have multi video ports.
 * video port ------- crtc
 */
static int rockchip_vop2_preinit(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	const struct vop2_data *vop2_data = cstate->crtc->data;

	if (!rockchip_vop2) {
		rockchip_vop2 = calloc(1, sizeof(struct vop2));
		if (!rockchip_vop2)
			return -ENOMEM;
		rockchip_vop2->regs = dev_read_addr_ptr(cstate->dev);
		rockchip_vop2->regsbak = malloc(RK3568_MAX_REG);
		rockchip_vop2->reg_len = RK3568_MAX_REG;
		rockchip_vop2->grf = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
		if (rockchip_vop2->grf <= 0)
			printf("%s: Get syscon grf failed (ret=%p)\n", __func__, rockchip_vop2->grf);
		rockchip_vop2->version = vop2_data->version;
		rockchip_vop2->data = vop2_data;
		if (rockchip_vop2->version == VOP_VERSION_RK3588) {
			struct regmap *map;

			rockchip_vop2->vop_grf = syscon_get_first_range(ROCKCHIP_SYSCON_VOP_GRF);
			if (rockchip_vop2->vop_grf <= 0)
				printf("%s: Get syscon vop_grf failed (ret=%p)\n", __func__, rockchip_vop2->vop_grf);
			map = syscon_regmap_lookup_by_phandle(cstate->dev, "rockchip,vo1-grf");
			rockchip_vop2->vo1_grf = regmap_get_range(map, 0);
			if (rockchip_vop2->vo1_grf <= 0)
				printf("%s: Get syscon vo1_grf failed (ret=%p)\n", __func__, rockchip_vop2->vo1_grf);
			rockchip_vop2->sys_pmu = syscon_get_first_range(ROCKCHIP_SYSCON_PMU);
			if (rockchip_vop2->sys_pmu <= 0)
				printf("%s: Get syscon sys_pmu failed (ret=%p)\n", __func__, rockchip_vop2->sys_pmu);
		}
	}

	cstate->private = rockchip_vop2;
	cstate->max_output = vop2_data->vp_data[cstate->crtc_id].max_output;
	cstate->feature = vop2_data->vp_data[cstate->crtc_id].feature;

	vop2_global_initial(rockchip_vop2, state);

	return 0;
}

/*
 * calc the dclk on rk3588
 * the available div of dclk is 1, 2, 4
 *
 */
static unsigned long vop2_calc_dclk(unsigned long child_clk, unsigned long max_dclk)
{
	if (child_clk * 4 <= max_dclk)
		return child_clk * 4;
	else if (child_clk * 2 <= max_dclk)
		return child_clk * 2;
	else if (child_clk <= max_dclk)
		return child_clk;
	else
		return 0;
}

/*
 * 4 pixclk/cycle on rk3588
 * RGB/eDP/HDMI: if_pixclk >= dclk_core
 * DP: dp_pixclk = dclk_out <= dclk_core
 * DSI: mipi_pixclk <= dclk_out <= dclk_core
 */
static unsigned long vop2_calc_cru_cfg(struct display_state *state,
				       int *dclk_core_div, int *dclk_out_div,
				       int *if_pixclk_div, int *if_dclk_div)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct vop2 *vop2 = cstate->private;
	unsigned long v_pixclk = mode->clock;
	unsigned long dclk_core_rate = v_pixclk >> 2;
	unsigned long dclk_rate = v_pixclk;
	unsigned long dclk_out_rate;
	u64 if_dclk_rate;
	u64 if_pixclk_rate;
	int output_type = conn_state->type;
	int output_mode = conn_state->output_mode;
	int K = 1;

	if (output_type == DRM_MODE_CONNECTOR_HDMIA) {
		/*
		 * K = 2: dclk_core = if_pixclk_rate > if_dclk_rate
		 * K = 1: dclk_core = hdmie_edp_dclk > if_pixclk_rate
		 */
		if (output_mode == ROCKCHIP_OUT_MODE_YUV420) {
			dclk_rate = dclk_rate >> 1;
			K = 2;
		}
		if (cstate->dsc_enable) {
			if_pixclk_rate = cstate->dsc_cds_clk_rate << 1;
			if_dclk_rate = cstate->dsc_cds_clk_rate;
		} else {
			if_pixclk_rate = (dclk_core_rate << 1) / K;
			if_dclk_rate = dclk_core_rate / K;
		}

		if (v_pixclk > VOP2_MAX_DCLK_RATE)
			dclk_rate = vop2_calc_dclk(dclk_core_rate, vop2->data->vp_data->max_dclk);

		if (!dclk_rate) {
			printf("DP if_pixclk_rate out of range(max_dclk: %d KHZ, dclk_core: %lld KHZ)\n",
			       vop2->data->vp_data->max_dclk, if_pixclk_rate);
			return -EINVAL;
		}
		*if_pixclk_div = dclk_rate / if_pixclk_rate;
		*if_dclk_div = dclk_rate / if_dclk_rate;
		*dclk_core_div = dclk_rate / dclk_core_rate;
		printf("dclk:%lu,if_pixclk_div;%d,if_dclk_div:%d\n",
		       dclk_rate, *if_pixclk_div, *if_dclk_div);
	} else if (output_type == DRM_MODE_CONNECTOR_eDP) {
		/* edp_pixclk = edp_dclk > dclk_core */
		if_pixclk_rate = v_pixclk / K;
		if_dclk_rate = v_pixclk / K;
		dclk_rate = if_pixclk_rate * K;
		*dclk_core_div = dclk_rate / dclk_core_rate;
		*if_pixclk_div = dclk_rate / if_pixclk_rate;
		*if_dclk_div = *if_pixclk_div;
	} else if (output_type == DRM_MODE_CONNECTOR_DisplayPort) {
		if (output_mode == ROCKCHIP_OUT_MODE_YUV420)
			dclk_out_rate = v_pixclk >> 3;
		else
			dclk_out_rate = v_pixclk >> 2;

		dclk_rate = vop2_calc_dclk(dclk_out_rate, vop2->data->vp_data->max_dclk);
		if (!dclk_rate) {
			printf("DP dclk_core out of range(max_dclk: %d KHZ, dclk_core: %ld KHZ)\n",
			       vop2->data->vp_data->max_dclk, dclk_core_rate);
			return -EINVAL;
		}
		*dclk_out_div = dclk_rate / dclk_out_rate;
		*dclk_core_div = dclk_rate / dclk_core_rate;

	} else if (output_type == DRM_MODE_CONNECTOR_DSI) {
		if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE)
			K = 2;
		if (cstate->dsc_enable)
			/* dsc output is 96bit, dsi input is 192 bit */
			if_pixclk_rate = cstate->dsc_cds_clk_rate >> 1;
		else
			if_pixclk_rate = dclk_core_rate / K;
		/* dclk_core = dclk_out * K = if_pixclk * K = v_pixclk / 4 */
		dclk_out_rate = dclk_core_rate / K;
		/* dclk_rate = N * dclk_core_rate N = (1,2,4 ), we get a little factor here */
		dclk_rate = vop2_calc_dclk(dclk_out_rate, vop2->data->vp_data->max_dclk);
		if (!dclk_rate) {
			printf("MIPI dclk out of range(max_dclk: %d KHZ, dclk_rate: %ld KHZ)\n",
			       vop2->data->vp_data->max_dclk, dclk_rate);
			return -EINVAL;
		}

		if (cstate->dsc_enable)
			dclk_rate = dclk_rate >> 1;

		*dclk_out_div = dclk_rate / dclk_out_rate;
		*dclk_core_div = dclk_rate / dclk_core_rate;
		*if_pixclk_div = 1;       /*mipi pixclk == dclk_out*/
		if (cstate->dsc_enable)
			*if_pixclk_div = dclk_out_rate / if_pixclk_rate;

	} else if (output_type == DRM_MODE_CONNECTOR_DPI) {
		dclk_rate = v_pixclk;
		*dclk_core_div = dclk_rate / dclk_core_rate;
	}

	*if_pixclk_div = ilog2(*if_pixclk_div);
	*if_dclk_div = ilog2(*if_dclk_div);
	*dclk_core_div = ilog2(*dclk_core_div);
	*dclk_out_div = ilog2(*dclk_out_div);

	return dclk_rate;
}

static int vop2_calc_dsc_clk(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct crtc_state *cstate = &state->crtc_state;
	u64 v_pixclk = mode->clock; /* video timing pixclk */
	u8 k = 1;

	if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE)
		k = 2;

	cstate->dsc_txp_clk_rate = v_pixclk;
	do_div(cstate->dsc_txp_clk_rate, (cstate->dsc_pixel_num * k));

	cstate->dsc_pxl_clk_rate = v_pixclk;
	do_div(cstate->dsc_pxl_clk_rate, (cstate->dsc_slice_num * k));

	/* dsc_cds = crtc_clock / (cds_dat_width / bits_per_pixel)
	 * cds_dat_width = 96;
	 * bits_per_pixel = [8-12];
	 * As only support 1/2/4 div, so we set dsc_cds = crtc_clock / 8;
	 */
	cstate->dsc_cds_clk_rate = v_pixclk / 8;

	return 0;
}

static unsigned long rk3588_vop2_if_cfg(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct rockchip_dsc_sink_cap *dsc_sink_cap = &cstate->dsc_sink_cap;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u16 hdisplay = mode->crtc_hdisplay;
	int output_if = conn_state->output_if;
	int dclk_core_div = 0;
	int dclk_out_div = 0;
	int if_pixclk_div = 0;
	int if_dclk_div = 0;
	unsigned long dclk_rate;
	u32 val;

	if (output_if & (VOP_OUTPUT_IF_HDMI0 | VOP_OUTPUT_IF_HDMI1)) {
		val = (mode->flags & DRM_MODE_FLAG_NHSYNC) ? BIT(HSYNC_POSITIVE) : 0;
		val |= (mode->flags & DRM_MODE_FLAG_NVSYNC) ? BIT(VSYNC_POSITIVE) : 0;
	} else {
		val = (mode->flags & DRM_MODE_FLAG_NHSYNC) ? 0 : BIT(HSYNC_POSITIVE);
		val |= (mode->flags & DRM_MODE_FLAG_NVSYNC) ? 0 : BIT(VSYNC_POSITIVE);
	}

	if (cstate->dsc_enable) {
		int k = 1;

		if (!vop2->data->nr_dscs) {
			printf("Unsupported DSC\n");
			return 0;
		}

		if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE)
			k = 2;

		cstate->dsc_id = output_if & (VOP_OUTPUT_IF_MIPI0 | VOP_OUTPUT_IF_HDMI0) ? 0 : 1;
		cstate->dsc_slice_num = hdisplay / dsc_sink_cap->slice_width / k;
		cstate->dsc_pixel_num = cstate->dsc_slice_num > 4 ? 4 : cstate->dsc_slice_num;

		vop2_calc_dsc_clk(state);
		printf("Enable DSC%d slice:%dx%d, slice num:%d\n",
		       cstate->dsc_id, dsc_sink_cap->slice_width,
		       dsc_sink_cap->slice_height, cstate->dsc_slice_num);
	}

	dclk_rate = vop2_calc_cru_cfg(state, &dclk_core_div, &dclk_out_div, &if_pixclk_div, &if_dclk_div);

	if (output_if & VOP_OUTPUT_IF_RGB) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, 0x7, RK3588_RGB_EN_SHIFT,
				4, false);
	}

	if (output_if & VOP_OUTPUT_IF_BT1120) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, 0x7, RK3588_RGB_EN_SHIFT,
				3, false);
	}

	if (output_if & VOP_OUTPUT_IF_BT656) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, 0x7, RK3588_RGB_EN_SHIFT,
				2, false);
	}

	if (output_if & VOP_OUTPUT_IF_MIPI0) {
		if (cstate->crtc_id == 2)
			val = 0;
		else
			val = 1;

		if (conn_state->output_flags & ROCKCHIP_OUTPUT_MIPI_DS_MODE)
			vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, EN_MASK,
					RK3588_MIPI_DSI0_MODE_SEL_SHIFT, 1, false);

		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_MIPI0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, 1, RK3588_MIPI0_MUX_SHIFT, val, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, MIPI0_PIXCLK_DIV_SHIFT,
				if_pixclk_div, false);

		if (conn_state->hold_mode) {
			vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
					EN_MASK, EDPI_TE_EN, 1, false);

			vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
					EN_MASK, EDPI_WMS_HOLD_EN, 1, false);
		}
	}

	if (output_if & VOP_OUTPUT_IF_MIPI1) {
		if (cstate->crtc_id == 2)
			val = 0;
		else if (cstate->crtc_id == 3)
			val = 1;
		else
			val = 3; /*VP1*/
		if (conn_state->output_flags & ROCKCHIP_OUTPUT_MIPI_DS_MODE)
			vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, EN_MASK,
					RK3588_MIPI_DSI1_MODE_SEL_SHIFT, 1, false);

		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_MIPI1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, MIPI1_MUX_SHIFT,
				val, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, MIPI1_PIXCLK_DIV_SHIFT,
				if_pixclk_div, false);

		if (conn_state->hold_mode) {
			/* UNDO: RK3588 VP1->DSC1->DSI1 only can support soft TE mode */
			if (vop2->version == VOP_VERSION_RK3588 && val == 3)
				vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
						EN_MASK, EDPI_TE_EN, 0, false);
			else
				vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
						EN_MASK, EDPI_TE_EN, 1, false);

			vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
					EN_MASK, EDPI_WMS_HOLD_EN, 1, false);
		}
	}

	if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE) {
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, EN_MASK,
				RK3568_MIPI_DUAL_EN_SHIFT, 1, false);
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset, EN_MASK,
				MIPI_DUAL_EN_SHIFT, 1, false);
		if (conn_state->output_flags & ROCKCHIP_OUTPUT_DATA_SWAP)
			vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
					EN_MASK, MIPI_DUAL_SWAP_EN_SHIFT, 1,
					false);
	}

	if (output_if & VOP_OUTPUT_IF_eDP0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_EDP0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, RK3588_HDMI_EDP0_MUX_SHIFT,
				cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP0_DCLK_DIV_SHIFT,
				if_dclk_div, false);

		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP0_PIXCLK_DIV_SHIFT,
				if_pixclk_div, false);

		vop2_grf_writel(vop2, vop2->vop_grf, RK3588_GRF_VOP_CON2, EN_MASK,
				RK3588_GRF_EDP0_ENABLE_SHIFT, 1);
	}

	if (output_if & VOP_OUTPUT_IF_eDP1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_EDP1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, RK3588_HDMI_EDP1_MUX_SHIFT,
				cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP1_DCLK_DIV_SHIFT,
				if_dclk_div, false);

		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP1_PIXCLK_DIV_SHIFT,
				if_pixclk_div, false);

		vop2_grf_writel(vop2, vop2->vop_grf, RK3588_GRF_VOP_CON2, EN_MASK,
				RK3588_GRF_EDP1_ENABLE_SHIFT, 1);
	}

	if (output_if & VOP_OUTPUT_IF_HDMI0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_HDMI0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, RK3588_HDMI_EDP0_MUX_SHIFT,
				cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP0_DCLK_DIV_SHIFT,
				if_dclk_div, false);

		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP0_PIXCLK_DIV_SHIFT,
				if_pixclk_div, false);

		vop2_grf_writel(vop2, vop2->vop_grf, RK3588_GRF_VOP_CON2, EN_MASK,
				RK3588_GRF_HDMITX0_ENABLE_SHIFT, 1);
		vop2_grf_writel(vop2, vop2->vo1_grf, RK3588_GRF_VO1_CON0,
				HDMI_SYNC_POL_MASK,
				HDMI0_SYNC_POL_SHIFT, val);
	}

	if (output_if & VOP_OUTPUT_IF_HDMI1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_HDMI1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, RK3588_HDMI_EDP1_MUX_SHIFT,
				cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP1_DCLK_DIV_SHIFT,
				if_dclk_div, false);

		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, 3, HDMI_EDP1_PIXCLK_DIV_SHIFT,
				if_pixclk_div, false);

		vop2_grf_writel(vop2, vop2->vop_grf, RK3588_GRF_VOP_CON2, EN_MASK,
				RK3588_GRF_HDMITX1_ENABLE_SHIFT, 1);
		vop2_grf_writel(vop2, vop2->vo1_grf, RK3588_GRF_VO1_CON0,
				HDMI_SYNC_POL_MASK,
				HDMI1_SYNC_POL_SHIFT, val);
	}

	if (output_if & VOP_OUTPUT_IF_DP0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_DP0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, RK3588_DP0_MUX_SHIFT,
				cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, RK3588_IF_PIN_POL_MASK,
				RK3588_DP0_PIN_POL_SHIFT, val, false);
	}

	if (output_if & VOP_OUTPUT_IF_DP1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RK3588_DP1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK, RK3588_DP1_MUX_SHIFT,
				cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, RK3588_IF_PIN_POL_MASK,
				RK3588_DP1_PIN_POL_SHIFT, val, false);
	}

	vop2_mask_write(vop2, RK3588_VP0_CLK_CTRL + vp_offset, 0x3,
			DCLK_CORE_DIV_SHIFT, dclk_core_div, false);
	vop2_mask_write(vop2, RK3588_VP0_CLK_CTRL + vp_offset, 0x3,
			DCLK_OUT_DIV_SHIFT, dclk_out_div, false);

	return dclk_rate;
}

static unsigned long rk3568_vop2_if_cfg(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	bool dclk_inv;
	u32 val;

	dclk_inv = (mode->flags & DRM_MODE_FLAG_PPIXDATA) ? 0 : 1;
	val = (mode->flags & DRM_MODE_FLAG_NHSYNC) ? 0 : BIT(HSYNC_POSITIVE);
	val |= (mode->flags & DRM_MODE_FLAG_NVSYNC) ? 0 : BIT(VSYNC_POSITIVE);

	if (conn_state->output_if & VOP_OUTPUT_IF_RGB) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RGB_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_grf_writel(vop2, vop2->grf, RK3568_GRF_VO_CON1, EN_MASK,
				GRF_RGB_DCLK_INV_SHIFT, dclk_inv);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_BT1120) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RGB_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK,
				BT1120_EN_SHIFT, 1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_grf_writel(vop2, vop2->grf, RK3568_GRF_VO_CON1, EN_MASK,
				GRF_BT1120_CLK_INV_SHIFT, !dclk_inv);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_BT656) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, BT656_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_grf_writel(vop2, vop2->grf, RK3568_GRF_VO_CON1, EN_MASK,
				GRF_BT656_CLK_INV_SHIFT, !dclk_inv);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_LVDS0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, LVDS0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				LVDS0_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_RGB_LVDS_DCLK_POL_SHIT, dclk_inv, false);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_LVDS1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, LVDS1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				LVDS1_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_RGB_LVDS_DCLK_POL_SHIT, dclk_inv, false);
	}

	if (conn_state->output_flags &
	    (ROCKCHIP_OUTPUT_DUAL_CHANNEL_ODD_EVEN_MODE |
	     ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE)) {
		vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, EN_MASK,
				LVDS_DUAL_EN_SHIFT, 1, false);
		if (conn_state->output_flags &
		    ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE)
			vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, EN_MASK,
					LVDS_DUAL_LEFT_RIGHT_EN_SHIFT, 1,
					false);
		if (conn_state->output_flags & ROCKCHIP_OUTPUT_DATA_SWAP)
			vop2_mask_write(vop2, RK3568_DSP_IF_CTRL, EN_MASK,
					LVDS_DUAL_SWAP_EN_SHIFT, 1, false);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_MIPI0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, MIPI0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				MIPI0_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_MIPI_DCLK_POL_SHIT, dclk_inv, false);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_MIPI1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, MIPI1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				MIPI1_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_MIPI_DCLK_POL_SHIT, dclk_inv, false);
	}

	if (conn_state->output_flags &
	    ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE) {
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset, EN_MASK,
				MIPI_DUAL_EN_SHIFT, 1, false);
		if (conn_state->output_flags & ROCKCHIP_OUTPUT_DATA_SWAP)
			vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
					EN_MASK, MIPI_DUAL_SWAP_EN_SHIFT, 1,
					false);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_eDP0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, EDP0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				EDP0_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_EDP_DCLK_POL_SHIT, dclk_inv, false);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_HDMI0) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, HDMI0_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				HDMI0_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_HDMI_DCLK_POL_SHIT, 1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL,
				IF_CRTL_HDMI_PIN_POL_MASK,
				IF_CRTL_HDMI_PIN_POL_SHIT, val, false);
	}

	return mode->clock;
}

static void vop2_post_color_swap(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 output_type = conn_state->type;
	u32 data_swap = 0;

	if (is_uv_swap(conn_state->bus_format, conn_state->output_mode))
		data_swap = DSP_RB_SWAP;

	if (vop2->version == VOP_VERSION_RK3588 &&
	    (output_type == DRM_MODE_CONNECTOR_HDMIA ||
	     output_type == DRM_MODE_CONNECTOR_eDP) &&
	    (conn_state->bus_format == MEDIA_BUS_FMT_YUV8_1X24 ||
	     conn_state->bus_format == MEDIA_BUS_FMT_YUV10_1X30))
		data_swap |= DSP_RG_SWAP;

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset,
			DATA_SWAP_MASK, DATA_SWAP_SHIFT, data_swap, false);
}

static void vop2_clk_set_parent(struct clk *clk, struct clk *parent)
{
	int ret = 0;

	if (parent->dev)
		ret = clk_set_parent(clk, parent);
	if (ret < 0)
		debug("failed to set %s as parent for %s\n",
		      parent->dev->name, clk->dev->name);
}

static ulong vop2_clk_set_rate(struct clk *clk, ulong rate)
{
	int ret = 0;

	if (clk->dev)
		ret = clk_set_rate(clk, rate);
	if (ret < 0)
		debug("failed to set %s rate %lu \n", clk->dev->name, rate);

	return ret;
}

static void vop2_calc_dsc_cru_cfg(struct display_state *state,
				  int *dsc_txp_clk_div, int *dsc_pxl_clk_div,
				  int *dsc_cds_clk_div, u64 dclk_rate)
{
	struct crtc_state *cstate = &state->crtc_state;

	*dsc_txp_clk_div = dclk_rate / cstate->dsc_txp_clk_rate;
	*dsc_pxl_clk_div = dclk_rate / cstate->dsc_pxl_clk_rate;
	*dsc_cds_clk_div = dclk_rate / cstate->dsc_cds_clk_rate;

	*dsc_txp_clk_div = ilog2(*dsc_txp_clk_div);
	*dsc_pxl_clk_div = ilog2(*dsc_pxl_clk_div);
	*dsc_cds_clk_div = ilog2(*dsc_cds_clk_div);
}

static void vop2_load_pps(struct display_state *state, struct vop2 *vop2, u8 dsc_id)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct drm_dsc_picture_parameter_set *pps = &cstate->pps;
	struct drm_dsc_picture_parameter_set config_pps;
	const struct vop2_data *vop2_data = vop2->data;
	const struct vop2_dsc_data *dsc_data = &vop2_data->dsc[dsc_id];
	u32 *pps_val = (u32 *)&config_pps;
	u32 decoder_regs_offset = (dsc_id * 0x100);
	int i = 0;

	memcpy(&config_pps, pps, sizeof(config_pps));

	if ((config_pps.pps_3 & 0xf) > dsc_data->max_linebuf_depth) {
		config_pps.pps_3 &= 0xf0;
		config_pps.pps_3 |= dsc_data->max_linebuf_depth;
		printf("DSC%d max_linebuf_depth is: %d, current set value is: %d\n",
		       dsc_id, dsc_data->max_linebuf_depth, config_pps.pps_3 & 0xf);
	}

	for (i = 0; i < DSC_NUM_BUF_RANGES; i++) {
		config_pps.rc_range_parameters[i] =
			(pps->rc_range_parameters[i] >> 3 & 0x1f) |
			((pps->rc_range_parameters[i] >> 14 & 0x3) << 5) |
			((pps->rc_range_parameters[i] >> 0 & 0x7) << 7) |
			((pps->rc_range_parameters[i] >> 8 & 0x3f) << 10);
	}

	for (i = 0; i < ROCKCHIP_DSC_PPS_SIZE_BYTE / 4; i++)
		vop2_writel(vop2, RK3588_DSC_8K_PPS0_3 + decoder_regs_offset + i * 4, *pps_val++);
}

static void vop2_dsc_enable(struct display_state *state, struct vop2 *vop2, u8 dsc_id, u64 dclk_rate)
{
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct crtc_state *cstate = &state->crtc_state;
	struct rockchip_dsc_sink_cap *dsc_sink_cap = &cstate->dsc_sink_cap;
	const struct vop2_data *vop2_data = vop2->data;
	const struct vop2_dsc_data *dsc_data = &vop2_data->dsc[dsc_id];
	bool mipi_ds_mode = false;
	u8 dsc_interface_mode = 0;
	u16 hsync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;
	u16 hdisplay = mode->crtc_hdisplay;
	u16 htotal = mode->crtc_htotal;
	u16 hact_st = mode->crtc_htotal - mode->crtc_hsync_start;
	u16 vdisplay = mode->crtc_vdisplay;
	u16 vtotal = mode->crtc_vtotal;
	u16 vsync_len = mode->crtc_vsync_end - mode->crtc_vsync_start;
	u16 vact_st = mode->crtc_vtotal - mode->crtc_vsync_start;
	u16 vact_end = vact_st + vdisplay;
	u32 ctrl_regs_offset = (dsc_id * 0x30);
	u32 decoder_regs_offset = (dsc_id * 0x100);
	u32 backup_regs_offset = 0;
	int dsc_txp_clk_div = 0;
	int dsc_pxl_clk_div = 0;
	int dsc_cds_clk_div = 0;

	if (!vop2->data->nr_dscs) {
		printf("Unsupported DSC\n");
		return;
	}

	if (cstate->dsc_slice_num > dsc_data->max_slice_num)
		printf("DSC%d supported max slice is: %d, current is: %d\n",
		       dsc_data->id, dsc_data->max_slice_num, cstate->dsc_slice_num);

	if (dsc_data->pd_id) {
		if (vop2_power_domain_on(vop2, dsc_data->pd_id))
			printf("open dsc%d pd fail\n", dsc_id);
	}

	vop2_mask_write(vop2, RK3588_DSC_8K_INIT_DLY + ctrl_regs_offset, EN_MASK,
			SCAN_TIMING_PARA_IMD_EN_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_PORT_SEL_MASK,
			DSC_PORT_SEL_SHIFT, cstate->crtc_id, false);
	if (conn_state->output_if & (VOP_OUTPUT_IF_HDMI0 | VOP_OUTPUT_IF_HDMI1)) {
		dsc_interface_mode = VOP_DSC_IF_HDMI;
	} else {
		mipi_ds_mode = !!(conn_state->output_flags & ROCKCHIP_OUTPUT_MIPI_DS_MODE);
		if (mipi_ds_mode)
			dsc_interface_mode = VOP_DSC_IF_MIPI_DS_MODE;
		else
			dsc_interface_mode = VOP_DSC_IF_MIPI_VIDEO_MODE;
	}

	if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE)
		vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_MAN_MODE_MASK,
				DSC_MAN_MODE_SHIFT, 0, false);
	else
		vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_MAN_MODE_MASK,
				DSC_MAN_MODE_SHIFT, 1, false);

	vop2_calc_dsc_cru_cfg(state, &dsc_txp_clk_div, &dsc_pxl_clk_div, &dsc_cds_clk_div, dclk_rate);

	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_INTERFACE_MODE_MASK,
			DSC_INTERFACE_MODE_SHIFT, dsc_interface_mode, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_PIXEL_NUM_MASK,
			DSC_PIXEL_NUM_SHIFT, cstate->dsc_pixel_num >> 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_TXP_CLK_DIV_MASK,
			DSC_TXP_CLK_DIV_SHIFT, dsc_txp_clk_div, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_PXL_CLK_DIV_MASK,
			DSC_PXL_CLK_DIV_SHIFT, dsc_pxl_clk_div, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_CDS_CLK_DIV_MASK,
			DSC_CDS_CLK_DIV_SHIFT, dsc_cds_clk_div, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, EN_MASK,
			DSC_SCAN_EN_SHIFT, !mipi_ds_mode, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_SYS_CTRL + ctrl_regs_offset, DSC_CDS_CLK_DIV_MASK,
			DSC_HALT_EN_SHIFT, mipi_ds_mode, false);

	if (!mipi_ds_mode) {
		u16 dsc_hsync, dsc_htotal, dsc_hact_st, dsc_hact_end;
		u32 target_bpp = dsc_sink_cap->target_bits_per_pixel_x16;
		u64 dsc_cds_rate = cstate->dsc_cds_clk_rate;
		u32 v_pixclk_mhz = mode->crtc_clock / 1000; /* video timing pixclk */
		u32 dly_num, dsc_cds_rate_mhz, val = 0;

		if (target_bpp >> 4 < dsc_data->min_bits_per_pixel)
			printf("Unsupported bpp less than: %d\n", dsc_data->min_bits_per_pixel);

		/*
		 * dly_num = delay_line_num * T(one-line) / T (dsc_cds)
		 * T (one-line) = 1/v_pixclk_mhz * htotal = htotal/v_pixclk_mhz
		 * T (dsc_cds) = 1 / dsc_cds_rate_mhz
		 * delay_line_num: according the pps initial_xmit_delay to adjust vop dsc delay
		 *                 delay_line_num = 4 - BPP / 8
		 *                                = (64 - target_bpp / 8) / 16
		 *
		 * dly_num = htotal * dsc_cds_rate_mhz / v_pixclk_mhz * (64 - target_bpp / 8) / 16;
		 */
		do_div(dsc_cds_rate, 1000000); /* hz to Mhz */
		dsc_cds_rate_mhz = dsc_cds_rate;
		dly_num = htotal * dsc_cds_rate_mhz / v_pixclk_mhz * (64 - target_bpp / 8) / 16;
		vop2_mask_write(vop2, RK3588_DSC_8K_INIT_DLY + ctrl_regs_offset, DSC_INIT_DLY_MODE_MASK,
				DSC_INIT_DLY_MODE_SHIFT, 0, false);
		vop2_mask_write(vop2, RK3588_DSC_8K_INIT_DLY + ctrl_regs_offset, DSC_INIT_DLY_NUM_MASK,
				DSC_INIT_DLY_NUM_SHIFT, dly_num, false);

		dsc_hsync = hsync_len / 2;
		dsc_htotal = htotal / (1 << dsc_cds_clk_div);
		val = dsc_htotal << 16 | dsc_hsync;
		vop2_mask_write(vop2, RK3588_DSC_8K_HTOTAL_HS_END + ctrl_regs_offset, DSC_HTOTAL_PW_MASK,
				DSC_HTOTAL_PW_SHIFT, val, false);

		dsc_hact_st = hact_st / 2;
		dsc_hact_end = (hdisplay * target_bpp >> 4) / 24 + dsc_hact_st;
		val = dsc_hact_end << 16 | dsc_hact_st;
		vop2_mask_write(vop2, RK3588_DSC_8K_HACT_ST_END + ctrl_regs_offset, DSC_HACT_ST_END_MASK,
				DSC_HACT_ST_END_SHIFT, val, false);

		vop2_mask_write(vop2, RK3588_DSC_8K_VTOTAL_VS_END + ctrl_regs_offset, DSC_VTOTAL_PW_MASK,
				DSC_VTOTAL_PW_SHIFT, vtotal << 16 | vsync_len, false);
		vop2_mask_write(vop2, RK3588_DSC_8K_VACT_ST_END + ctrl_regs_offset, DSC_VACT_ST_END_MASK,
				DSC_VACT_ST_END_SHIFT, vact_end << 16 | vact_st, false);
	}

	vop2_mask_write(vop2, RK3588_DSC_8K_RST + ctrl_regs_offset, RST_DEASSERT_MASK,
			RST_DEASSERT_SHIFT, 1, false);
	udelay(10);
	/* read current dsc core register and backup to regsbak */
	backup_regs_offset = RK3588_DSC_8K_CTRL0;
	vop2->regsbak[backup_regs_offset >> 2] = vop2_readl(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset);

	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_EN_SHIFT, 1, false);
	vop2_load_pps(state, vop2, dsc_id);

	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_RBIT_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_RBYT_SHIFT, 0, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_FLAL_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_MER_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_EPB_SHIFT, 0, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_EPL_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_NSLC_SHIFT, ilog2(cstate->dsc_slice_num), false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_SBO_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_IFEP_SHIFT, dsc_sink_cap->version_minor == 2 ? 1 : 0, false);
	vop2_mask_write(vop2, RK3588_DSC_8K_CTRL0 + decoder_regs_offset, EN_MASK,
			DSC_PPS_UPD_SHIFT, 1, false);

	printf("DSC%d: txp:%lld div:%d, pxl:%lld div:%d, dsc:%lld div:%d\n",
	       dsc_id,
	       cstate->dsc_txp_clk_rate, dsc_txp_clk_div,
	       cstate->dsc_pxl_clk_rate, dsc_pxl_clk_div,
	       cstate->dsc_cds_clk_rate, dsc_cds_clk_div);
}

static bool is_extend_pll(struct display_state *state, struct udevice **clk_dev)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	struct udevice *vp_dev, *dev;
	struct ofnode_phandle_args args;
	char vp_name[10];
	int ret;

	if (vop2->version != VOP_VERSION_RK3588)
		return false;

	sprintf(vp_name, "port@%d", cstate->crtc_id);
	if (uclass_find_device_by_name(UCLASS_VIDEO_CRTC, vp_name, &vp_dev)) {
		printf("warn: can't get vp device\n");
		return false;
	}

	ret = dev_read_phandle_with_args(vp_dev, "assigned-clock-parents", "#clock-cells", 0,
					 0, &args);
	if (ret) {
		printf("warn: can't get assigned-clock-parents's node\n");
		return false;
	}

	if (uclass_find_device_by_ofnode(UCLASS_CLK, args.node, &dev)) {
		printf("warn: can't get clk device\n");
		return false;
	}

	if (!strcmp(dev->name, "hdmiphypll_clk0") || !strcmp(dev->name, "hdmiphypll_clk1")) {
		printf("%s: clk dev :%s: vp port:%s\n", __func__, dev->name, vp_dev->name);
		if (clk_dev)
			*clk_dev = dev;
		return true;
	}

	return false;
}

static int rockchip_vop2_init(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct vop2 *vop2 = cstate->private;
	u16 hsync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;
	u16 hdisplay = mode->crtc_hdisplay;
	u16 htotal = mode->crtc_htotal;
	u16 hact_st = mode->crtc_htotal - mode->crtc_hsync_start;
	u16 hact_end = hact_st + hdisplay;
	u16 vdisplay = mode->crtc_vdisplay;
	u16 vtotal = mode->crtc_vtotal;
	u16 vsync_len = mode->crtc_vsync_end - mode->crtc_vsync_start;
	u16 vact_st = mode->crtc_vtotal - mode->crtc_vsync_start;
	u16 vact_end = vact_st + vdisplay;
	bool yuv_overlay = false;
	bool splice_en = false;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 line_flag_offset = (cstate->crtc_id * 4);
	u32 val, act_end;
	u8 dither_down_en = 0;
	u8 pre_dither_down_en = 0;
	char output_type_name[30] = {0};
	char dclk_name[9];
	struct clk dclk;
	struct clk hdmi0_phy_pll;
	struct clk hdmi1_phy_pll;
	struct clk hdmi_phy_pll;
	struct udevice *disp_dev;
	unsigned long dclk_rate;
	int ret;

	printf("VOP update mode to: %dx%d%s%d, type:%s for VP%d\n",
	       mode->hdisplay, mode->vdisplay,
	       mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "p",
	       mode->vscan,
	       get_output_if_name(conn_state->output_if, output_type_name),
	       cstate->crtc_id);

	if (mode->hdisplay > VOP2_MAX_VP_OUTPUT_WIDTH) {
		cstate->splice_mode = true;
		splice_en = true;
		cstate->splice_crtc_id = vop2->data->vp_data[cstate->crtc_id].splice_vp_id;
		if (!cstate->splice_crtc_id) {
			printf("%s: Splice mode is unsupported by vp%d\n",
			       __func__, cstate->crtc_id);
			return -EINVAL;
		}
	}

	vop2_initial(vop2, state);
	if (vop2->version == VOP_VERSION_RK3588)
		dclk_rate = rk3588_vop2_if_cfg(state);
	else
		dclk_rate = rk3568_vop2_if_cfg(state);

	if (conn_state->output_mode == ROCKCHIP_OUT_MODE_AAAA &&
	    !(cstate->feature & VOP_FEATURE_OUTPUT_10BIT))
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;

	vop2_post_color_swap(state);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, OUT_MODE_MASK,
			OUT_MODE_SHIFT, conn_state->output_mode, false);

	switch (conn_state->bus_format) {
	case MEDIA_BUS_FMT_RGB565_1X16:
		dither_down_en = 1;
		break;
	case MEDIA_BUS_FMT_RGB666_1X18:
	case MEDIA_BUS_FMT_RGB666_1X24_CPADHI:
	case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG:
	case MEDIA_BUS_FMT_RGB666_1X7X3_JEIDA:
		dither_down_en = 1;
		break;
	case MEDIA_BUS_FMT_YUV8_1X24:
	case MEDIA_BUS_FMT_UYYVYY8_0_5X24:
		dither_down_en = 0;
		pre_dither_down_en = 1;
		break;
	case MEDIA_BUS_FMT_YUV10_1X30:
	case MEDIA_BUS_FMT_UYYVYY10_0_5X30:
	case MEDIA_BUS_FMT_RGB888_1X24:
	case MEDIA_BUS_FMT_RGB888_1X7X4_SPWG:
	case MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA:
	default:
		dither_down_en = 0;
		pre_dither_down_en = 0;
		break;
	}

	if (conn_state->output_mode == ROCKCHIP_OUT_MODE_AAAA)
		pre_dither_down_en = 0;
	else
		pre_dither_down_en = 1;
	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			DITHER_DOWN_EN_SHIFT, dither_down_en, false);
	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			PRE_DITHER_DOWN_EN_SHIFT, pre_dither_down_en, false);

	yuv_overlay = is_yuv_output(conn_state->bus_format) ? 1 : 0;
	vop2_mask_write(vop2, RK3568_OVL_CTRL, EN_MASK, cstate->crtc_id,
			yuv_overlay, false);

	cstate->yuv_overlay = yuv_overlay;

	vop2_mask_write(vop2, RK3568_SYS_LUT_PORT_SEL, EN_MASK,
			PORT_MERGE_EN_SHIFT, splice_en, false);

	vop2_writel(vop2, RK3568_VP0_DSP_HTOTAL_HS_END + vp_offset,
		    (htotal << 16) | hsync_len);
	val = hact_st << 16;
	val |= hact_end;
	vop2_writel(vop2, RK3568_VP0_DSP_HACT_ST_END + vp_offset, val);
	val = vact_st << 16;
	val |= vact_end;
	vop2_writel(vop2, RK3568_VP0_DSP_VACT_ST_END + vp_offset, val);
	if (mode->flags & DRM_MODE_FLAG_INTERLACE) {
		u16 vact_st_f1 = vtotal + vact_st + 1;
		u16 vact_end_f1 = vact_st_f1 + vdisplay;

		val = vact_st_f1 << 16 | vact_end_f1;
		vop2_writel(vop2, RK3568_VP0_DSP_VACT_ST_END_F1 + vp_offset,
			    val);

		val = vtotal << 16 | (vtotal + vsync_len);
		vop2_writel(vop2, RK3568_VP0_DSP_VS_ST_END_F1 + vp_offset, val);
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				INTERLACE_EN_SHIFT, 1, false);
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				DSP_FILED_POL, 1, false);
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				P2I_EN_SHIFT, 1, false);
		vtotal += vtotal + 1;
		act_end = vact_end_f1;
	} else {
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				INTERLACE_EN_SHIFT, 0, false);
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				P2I_EN_SHIFT, 0, false);
		act_end = vact_end;
	}
	vop2_writel(vop2, RK3568_VP0_DSP_VTOTAL_VS_END + vp_offset,
		    (vtotal << 16) | vsync_len);

	if (vop2->version == VOP_VERSION_RK3568) {
		if (mode->flags & DRM_MODE_FLAG_DBLCLK ||
		    conn_state->output_if & VOP_OUTPUT_IF_BT656)
			vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
					CORE_DCLK_DIV_EN_SHIFT, 1, false);
		else
			vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
					CORE_DCLK_DIV_EN_SHIFT, 0, false);
	}

	if (conn_state->output_mode == ROCKCHIP_OUT_MODE_YUV420)
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
				DCLK_DIV2_MASK, DCLK_DIV2_SHIFT, 0x3, false);
	else
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
				DCLK_DIV2_MASK, DCLK_DIV2_SHIFT, 0, false);

	vop2_mask_write(vop2, RK3568_OVL_CTRL, OVL_MODE_SEL_MASK,
			OVL_MODE_SEL_SHIFT + cstate->crtc_id, yuv_overlay, false);

	if (yuv_overlay)
		val = 0x20010200;
	else
		val = 0;
	vop2_writel(vop2, RK3568_VP0_DSP_BG + vp_offset, val);
	if (splice_en) {
		vop2_mask_write(vop2, RK3568_OVL_CTRL, OVL_MODE_SEL_MASK,
				OVL_MODE_SEL_SHIFT + cstate->splice_crtc_id,
				yuv_overlay, false);
		vop2_writel(vop2, RK3568_VP0_DSP_BG + (cstate->splice_crtc_id * 0x100), val);
	}

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			POST_DSP_OUT_R2Y_SHIFT, yuv_overlay, false);

	vop2_tv_config_update(state, vop2);
	vop2_post_config(state, vop2);

	if (cstate->dsc_enable) {
		if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE) {
			vop2_dsc_enable(state, vop2, 0, dclk_rate);
			vop2_dsc_enable(state, vop2, 1, dclk_rate);
		} else {
			vop2_dsc_enable(state, vop2, cstate->dsc_id, dclk_rate);
		}
	}

	snprintf(dclk_name, sizeof(dclk_name), "dclk_vp%d", cstate->crtc_id);
	ret = clk_get_by_name(cstate->dev, dclk_name, &dclk);
	if (ret) {
		printf("%s: Failed to get dclk ret=%d\n", __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_name(UCLASS_VIDEO, "display-subsystem", &disp_dev);
	if (!ret) {
		ret = clk_get_by_name(disp_dev, "hdmi0_phy_pll", &hdmi0_phy_pll);
		if (ret)
			printf("%s: Failed to get hdmi0_phy_pll ret=%d\n", __func__, ret);
		ret = clk_get_by_name(disp_dev, "hdmi1_phy_pll", &hdmi1_phy_pll);
		if (ret)
			printf("%s: Failed to get hdmi1_phy_pll ret=%d\n", __func__, ret);
	} else {
		hdmi0_phy_pll.dev = NULL;
		hdmi1_phy_pll.dev = NULL;
		printf("%s: Faile to find display-subsystem node\n", __func__);
	}

	if (mode->clock < VOP2_MAX_DCLK_RATE) {
		if (conn_state->output_if & VOP_OUTPUT_IF_HDMI0)
			vop2_clk_set_parent(&dclk, &hdmi0_phy_pll);
		else if (conn_state->output_if & VOP_OUTPUT_IF_HDMI1)
			vop2_clk_set_parent(&dclk, &hdmi1_phy_pll);

		/*
		 * uboot clk driver won't set dclk parent's rate when use
		 * hdmi phypll as dclk source.
		 * So set dclk rate is meaningless. Set hdmi phypll rate
		 * directly.
		 */
		if ((conn_state->output_if & VOP_OUTPUT_IF_HDMI0) && hdmi0_phy_pll.dev) {
			ret = vop2_clk_set_rate(&hdmi0_phy_pll, dclk_rate * 1000);
		} else if ((conn_state->output_if & VOP_OUTPUT_IF_HDMI1) && hdmi1_phy_pll.dev) {
			ret = vop2_clk_set_rate(&hdmi1_phy_pll, dclk_rate * 1000);
		} else {
			if (is_extend_pll(state, &hdmi_phy_pll.dev))
				ret = vop2_clk_set_rate(&hdmi_phy_pll, dclk_rate * 1000);
			else
				ret = vop2_clk_set_rate(&dclk, dclk_rate * 1000);
		}

		if (IS_ERR_VALUE(ret)) {
			printf("%s: Failed to set vp%d dclk[%ld KHZ] ret=%d\n",
			       __func__, cstate->crtc_id, dclk_rate, ret);
			return ret;
		} else {
			if (mode->flags & DRM_MODE_FLAG_DBLCLK)
				mode->crtc_clock = ret * 2 / 1000;
			else
				mode->crtc_clock = ret / 1000;
		}
	} else {
		if (is_extend_pll(state, &hdmi_phy_pll.dev))
			ret = vop2_clk_set_rate(&hdmi_phy_pll, dclk_rate * 1000);
		else
			ret = vop2_clk_set_rate(&dclk, dclk_rate * 1000);

		if (IS_ERR_VALUE(ret)) {
			printf("%s: Failed to set vp%d dclk[%ld KHZ] ret=%d\n",
			       __func__, cstate->crtc_id, dclk_rate, ret);
			return ret;
		} else {
			if (mode->flags & DRM_MODE_FLAG_DBLCLK)
				mode->crtc_clock = ret * 2 / 1000;
			else
				mode->crtc_clock = ret / 1000;
		}
	}

	vop2_mask_write(vop2, RK3568_SYS_CTRL_LINE_FLAG0 + line_flag_offset, LINE_FLAG_NUM_MASK,
			RK3568_DSP_LINE_FLAG_NUM0_SHIFT, act_end, false);
	vop2_mask_write(vop2, RK3568_SYS_CTRL_LINE_FLAG0 + line_flag_offset, LINE_FLAG_NUM_MASK,
			RK3568_DSP_LINE_FLAG_NUM1_SHIFT, act_end, false);

	return 0;
}

static void vop2_setup_scale(struct vop2 *vop2, struct vop2_win_data *win,
			     uint32_t src_w, uint32_t src_h, uint32_t dst_w,
			     uint32_t dst_h)
{
	uint16_t yrgb_hor_scl_mode, yrgb_ver_scl_mode;
	uint16_t hscl_filter_mode, vscl_filter_mode;
	uint8_t gt2 = 0, gt4 = 0;
	uint32_t xfac = 0, yfac = 0;
	uint16_t hsu_filter_mode = VOP2_SCALE_UP_BIC;
	uint16_t hsd_filter_mode = VOP2_SCALE_DOWN_BIL;
	uint16_t vsu_filter_mode = VOP2_SCALE_UP_BIL;
	uint16_t vsd_filter_mode = VOP2_SCALE_DOWN_BIL;
	u32 win_offset = win->reg_offset;

	if (src_h >= (4 * dst_h))
		gt4 = 1;
	else if (src_h >= (2 * dst_h))
		gt2 = 1;

	if (gt4)
		src_h >>= 2;
	else if (gt2)
		src_h >>= 1;

	yrgb_hor_scl_mode = scl_get_scl_mode(src_w, dst_w);
	yrgb_ver_scl_mode = scl_get_scl_mode(src_h, dst_h);

	if (yrgb_hor_scl_mode == SCALE_UP)
		hscl_filter_mode = hsu_filter_mode;
	else
		hscl_filter_mode = hsd_filter_mode;

	if (yrgb_ver_scl_mode == SCALE_UP)
		vscl_filter_mode = vsu_filter_mode;
	else
		vscl_filter_mode = vsd_filter_mode;

	/*
	 * RK3568 VOP Esmart/Smart dsp_w should be even pixel
	 * at scale down mode
	 */
	if ((yrgb_hor_scl_mode == SCALE_DOWN) && (dst_w & 0x1)) {
		printf("win dst_w[%d] should align as 2 pixel\n", dst_w);
		dst_w += 1;
	}

	xfac = vop2_scale_factor(yrgb_hor_scl_mode, hscl_filter_mode, src_w, dst_w);
	yfac = vop2_scale_factor(yrgb_ver_scl_mode, vscl_filter_mode, src_h, dst_h);

	if (win->type == CLUSTER_LAYER) {
		vop2_writel(vop2, RK3568_CLUSTER0_WIN0_SCL_FACTOR_YRGB + win_offset,
			    yfac << 16 | xfac);

		vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL1 + win_offset,
				YRGB_GT2_MASK, CLUSTER_YRGB_GT2_SHIFT, gt2, false);
		vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL1 + win_offset,
				YRGB_GT4_MASK, CLUSTER_YRGB_GT4_SHIFT, gt4, false);

		vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL1 + win_offset,
				YRGB_XSCL_MODE_MASK, CLUSTER_YRGB_XSCL_MODE_SHIFT, yrgb_hor_scl_mode, false);
		vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL1 + win_offset,
				YRGB_YSCL_MODE_MASK, CLUSTER_YRGB_YSCL_MODE_SHIFT, yrgb_ver_scl_mode, false);

	} else {
		vop2_writel(vop2, RK3568_ESMART0_REGION0_SCL_FACTOR_YRGB + win_offset,
			    yfac << 16 | xfac);

		vop2_mask_write(vop2, RK3568_ESMART0_REGION0_CTRL + win_offset,
				YRGB_GT2_MASK, YRGB_GT2_SHIFT, gt2, false);
		vop2_mask_write(vop2, RK3568_ESMART0_REGION0_CTRL + win_offset,
				YRGB_GT4_MASK, YRGB_GT4_SHIFT, gt4, false);

		vop2_mask_write(vop2, RK3568_ESMART0_REGION0_SCL_CTRL + win_offset,
				YRGB_XSCL_MODE_MASK, YRGB_XSCL_MODE_SHIFT, yrgb_hor_scl_mode, false);
		vop2_mask_write(vop2, RK3568_ESMART0_REGION0_SCL_CTRL + win_offset,
				YRGB_YSCL_MODE_MASK, YRGB_YSCL_MODE_SHIFT, yrgb_ver_scl_mode, false);

		vop2_mask_write(vop2, RK3568_ESMART0_REGION0_SCL_CTRL + win_offset,
				YRGB_XSCL_FILTER_MODE_MASK, YRGB_XSCL_FILTER_MODE_SHIFT,
				hscl_filter_mode, false);
		vop2_mask_write(vop2, RK3568_ESMART0_REGION0_SCL_CTRL + win_offset,
				YRGB_YSCL_FILTER_MODE_MASK, YRGB_YSCL_FILTER_MODE_SHIFT,
				vscl_filter_mode, false);
	}
}

static void vop2_axi_config(struct vop2 *vop2, struct vop2_win_data *win)
{
	u32 win_offset = win->reg_offset;

	if (win->type == CLUSTER_LAYER) {
		vop2_mask_write(vop2, RK3568_CLUSTER0_CTRL + win_offset, CLUSTER_AXI_ID_MASK,
				CLUSTER_AXI_ID_SHIFT, win->axi_id, false);
		vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL2 + win_offset, CLUSTER_AXI_YRGB_ID_MASK,
				CLUSTER_AXI_YRGB_ID_SHIFT, win->axi_yrgb_id, false);
		vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL2 + win_offset, CLUSTER_AXI_UV_ID_MASK,
				CLUSTER_AXI_UV_ID_SHIFT, win->axi_uv_id, false);
	} else {
		vop2_mask_write(vop2, RK3568_ESMART0_AXI_CTRL + win_offset, ESMART_AXI_ID_MASK,
				ESMART_AXI_ID_SHIFT, win->axi_id, false);
		vop2_mask_write(vop2, RK3568_ESMART0_CTRL1 + win_offset, ESMART_AXI_YRGB_ID_MASK,
				ESMART_AXI_YRGB_ID_SHIFT, win->axi_yrgb_id, false);
		vop2_mask_write(vop2, RK3568_ESMART0_CTRL1 + win_offset, ESMART_AXI_UV_ID_MASK,
				ESMART_AXI_UV_ID_SHIFT, win->axi_uv_id, false);
	}
}

static void vop2_set_cluster_win(struct display_state *state, struct vop2_win_data *win)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct vop2 *vop2 = cstate->private;
	int src_w = cstate->src_rect.w;
	int src_h = cstate->src_rect.h;
	int crtc_x = cstate->crtc_rect.x;
	int crtc_y = cstate->crtc_rect.y;
	int crtc_w = cstate->crtc_rect.w;
	int crtc_h = cstate->crtc_rect.h;
	int xvir = cstate->xvir;
	int y_mirror = 0;
	int csc_mode;
	u32 act_info, dsp_info, dsp_st, dsp_stx, dsp_sty;
	/* offset of the right window in splice mode */
	u32 splice_pixel_offset = 0;
	u32 splice_yrgb_offset = 0;
	u32 win_offset = win->reg_offset;
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id) | (BIT(cstate->crtc_id) << 16);

	if (win->splice_mode_right) {
		src_w = cstate->right_src_rect.w;
		src_h = cstate->right_src_rect.h;
		crtc_x = cstate->right_crtc_rect.x;
		crtc_y = cstate->right_crtc_rect.y;
		crtc_w = cstate->right_crtc_rect.w;
		crtc_h = cstate->right_crtc_rect.h;
		splice_pixel_offset = cstate->right_src_rect.x - cstate->src_rect.x;
		splice_yrgb_offset = splice_pixel_offset * (state->logo.bpp >> 3);
		cfg_done = CFG_DONE_EN | BIT(cstate->splice_crtc_id) | (BIT(cstate->splice_crtc_id) << 16);
	}

	act_info = (src_h - 1) << 16;
	act_info |= (src_w - 1) & 0xffff;

	dsp_info = (crtc_h - 1) << 16;
	dsp_info |= (crtc_w - 1) & 0xffff;

	dsp_stx = crtc_x;
	dsp_sty = crtc_y;
	dsp_st = dsp_sty << 16 | (dsp_stx & 0xffff);

	if (mode->flags & DRM_MODE_FLAG_YMIRROR)
		y_mirror = 1;
	else
		y_mirror = 0;

	vop2_setup_scale(vop2, win, src_w, src_h, crtc_w, crtc_h);

	if (vop2->version == VOP_VERSION_RK3588)
		vop2_axi_config(vop2, win);

	if (y_mirror)
		printf("WARN: y mirror is unsupported by cluster window\n");

	vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL0 + win_offset,
			WIN_FORMAT_MASK, WIN_FORMAT_SHIFT, cstate->format,
			false);
	vop2_writel(vop2, RK3568_CLUSTER0_WIN0_VIR + win_offset, xvir);
	vop2_writel(vop2, RK3568_CLUSTER0_WIN0_YRGB_MST + win_offset,
		    cstate->dma_addr + splice_yrgb_offset);

	vop2_writel(vop2, RK3568_CLUSTER0_WIN0_ACT_INFO + win_offset, act_info);
	vop2_writel(vop2, RK3568_CLUSTER0_WIN0_DSP_INFO + win_offset, dsp_info);
	vop2_writel(vop2, RK3568_CLUSTER0_WIN0_DSP_ST + win_offset, dsp_st);

	vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL0 + win_offset, EN_MASK, WIN_EN_SHIFT, 1, false);

	csc_mode = vop2_convert_csc_mode(conn_state->color_space);
	vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL0 + win_offset, EN_MASK,
			CLUSTER_RGB2YUV_EN_SHIFT,
			is_yuv_output(conn_state->bus_format), false);
	vop2_mask_write(vop2, RK3568_CLUSTER0_WIN0_CTRL0 + win_offset, CSC_MODE_MASK,
			CLUSTER_CSC_MODE_SHIFT, csc_mode, false);
	vop2_mask_write(vop2, RK3568_CLUSTER0_CTRL + win_offset, EN_MASK, CLUSTER_EN_SHIFT, 1, false);

	vop2_writel(vop2, RK3568_REG_CFG_DONE, cfg_done);
}

static void vop2_set_smart_win(struct display_state *state, struct vop2_win_data *win)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct vop2 *vop2 = cstate->private;
	int src_w = cstate->src_rect.w;
	int src_h = cstate->src_rect.h;
	int crtc_x = cstate->crtc_rect.x;
	int crtc_y = cstate->crtc_rect.y;
	int crtc_w = cstate->crtc_rect.w;
	int crtc_h = cstate->crtc_rect.h;
	int xvir = cstate->xvir;
	int y_mirror = 0;
	int csc_mode;
	u32 act_info, dsp_info, dsp_st, dsp_stx, dsp_sty;
	/* offset of the right window in splice mode */
	u32 splice_pixel_offset = 0;
	u32 splice_yrgb_offset = 0;
	u32 win_offset = win->reg_offset;
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id) | (BIT(cstate->crtc_id) << 16);

	if (win->splice_mode_right) {
		src_w = cstate->right_src_rect.w;
		src_h = cstate->right_src_rect.h;
		crtc_x = cstate->right_crtc_rect.x;
		crtc_y = cstate->right_crtc_rect.y;
		crtc_w = cstate->right_crtc_rect.w;
		crtc_h = cstate->right_crtc_rect.h;
		splice_pixel_offset = cstate->right_src_rect.x - cstate->src_rect.x;
		splice_yrgb_offset = splice_pixel_offset * (state->logo.bpp >> 3);
		cfg_done = CFG_DONE_EN | BIT(cstate->splice_crtc_id) | (BIT(cstate->splice_crtc_id) << 16);
	}

	/*
	 * This is workaround solution for IC design:
	 * esmart can't support scale down when actual_w % 16 == 1.
	 */
	if (src_w > crtc_w && (src_w & 0xf) == 1) {
		printf("WARN: vp%d unsupported act_w[%d] mode 16 = 1 when scale down\n", cstate->crtc_id, src_w);
		src_w -= 1;
	}

	act_info = (src_h - 1) << 16;
	act_info |= (src_w - 1) & 0xffff;

	dsp_info = (crtc_h - 1) << 16;
	dsp_info |= (crtc_w - 1) & 0xffff;

	dsp_stx = crtc_x;
	dsp_sty = crtc_y;
	dsp_st = dsp_sty << 16 | (dsp_stx & 0xffff);

	if (mode->flags & DRM_MODE_FLAG_YMIRROR)
		y_mirror = 1;
	else
		y_mirror = 0;

	vop2_setup_scale(vop2, win, src_w, src_h, crtc_w, crtc_h);

	if (vop2->version == VOP_VERSION_RK3588)
		vop2_axi_config(vop2, win);

	if (y_mirror)
		cstate->dma_addr += (src_h - 1) * xvir * 4;
	vop2_mask_write(vop2, RK3568_ESMART0_CTRL1 + win_offset, EN_MASK,
			YMIRROR_EN_SHIFT, y_mirror, false);

	vop2_mask_write(vop2, RK3568_ESMART0_REGION0_CTRL + win_offset,
			WIN_FORMAT_MASK, WIN_FORMAT_SHIFT, cstate->format,
			false);
	vop2_writel(vop2, RK3568_ESMART0_REGION0_VIR + win_offset, xvir);
	vop2_writel(vop2, RK3568_ESMART0_REGION0_YRGB_MST + win_offset,
		    cstate->dma_addr + splice_yrgb_offset);

	vop2_writel(vop2, RK3568_ESMART0_REGION0_ACT_INFO + win_offset,
		    act_info);
	vop2_writel(vop2, RK3568_ESMART0_REGION0_DSP_INFO + win_offset,
		    dsp_info);
	vop2_writel(vop2, RK3568_ESMART0_REGION0_DSP_ST + win_offset, dsp_st);

	vop2_mask_write(vop2, RK3568_ESMART0_REGION0_CTRL + win_offset, EN_MASK,
			WIN_EN_SHIFT, 1, false);

	csc_mode = vop2_convert_csc_mode(conn_state->color_space);
	vop2_mask_write(vop2, RK3568_ESMART0_CTRL0 + win_offset, EN_MASK,
			RGB2YUV_EN_SHIFT,
			is_yuv_output(conn_state->bus_format), false);
	vop2_mask_write(vop2, RK3568_ESMART0_CTRL0 + win_offset, CSC_MODE_MASK,
			CSC_MODE_SHIFT, csc_mode, false);

	vop2_writel(vop2, RK3568_REG_CFG_DONE, cfg_done);
}

static int display_rect_calc_scale(int src, int dst)
{
	int scale = 0;

	if (WARN_ON(src < 0 || dst < 0))
		return -EINVAL;

	if (dst == 0)
		return 0;

	if (src > (dst << 16))
		return DIV_ROUND_UP(src, dst);

	scale = src / dst;

	return scale;
}

static int display_rect_calc_hscale(const struct display_rect *src,
				    const struct display_rect *dst,
				    int min_hscale, int max_hscale)
{
	int src_w = src->w;
	int dst_w = dst->w;
	int hscale = display_rect_calc_scale(src_w, dst_w);

	if (hscale < 0 || dst_w == 0)
		return hscale;

	if (hscale < min_hscale || hscale > max_hscale)
		return -ERANGE;

	return hscale;
}

static void vop2_calc_display_rect_for_splice(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	struct display_rect *src_rect = &cstate->src_rect;
	struct display_rect *dst_rect = &cstate->crtc_rect;
	struct display_rect left_src, left_dst, right_src, right_dst;
	u16 half_hdisplay = mode->crtc_hdisplay >> 1;
	int hscale = display_rect_calc_hscale(src_rect, dst_rect, 0, INT_MAX);
	int left_src_w, left_dst_w, right_dst_w;

	left_dst_w = min_t(u16, half_hdisplay, dst_rect->x + dst_rect->w) - dst_rect->x;
	if (left_dst_w < 0)
		left_dst_w = 0;
	right_dst_w = dst_rect->w - left_dst_w;

	if (!right_dst_w)
		left_src_w = src_rect->w;
	else
		left_src_w = left_dst_w * hscale;

	left_src.x = src_rect->x;
	left_src.w = left_src_w;
	left_dst.x = dst_rect->x;
	left_dst.w = left_dst_w;
	right_src.x = left_src.x + left_src.w;
	right_src.w = src_rect->x + src_rect->w - left_src.x - left_src.w;
	right_dst.x = dst_rect->x + left_dst_w - half_hdisplay;
	right_dst.w = right_dst_w;

	left_src.y = src_rect->y;
	left_src.h = src_rect->h;
	left_dst.y = dst_rect->y;
	left_dst.h = dst_rect->h;
	right_src.y = src_rect->y;
	right_src.h = src_rect->h;
	right_dst.y = dst_rect->y;
	right_dst.h = dst_rect->h;

	memcpy(&cstate->src_rect, &left_src, sizeof(struct display_rect));
	memcpy(&cstate->crtc_rect, &left_dst, sizeof(struct display_rect));
	memcpy(&cstate->right_src_rect, &right_src, sizeof(struct display_rect));
	memcpy(&cstate->right_crtc_rect, &right_dst, sizeof(struct display_rect));
}

static int rockchip_vop2_set_plane(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	struct vop2_win_data *win_data;
	struct vop2_win_data *splice_win_data;
	u8 primary_plane_id = vop2->vp_plane_mask[cstate->crtc_id].primary_plane_id;
	char plane_name[10] = {0};

	if (cstate->crtc_rect.w > cstate->max_output.width) {
		printf("ERROR: output w[%d] exceeded max width[%d]\n",
		       cstate->crtc_rect.w, cstate->max_output.width);
		return -EINVAL;
	}

	win_data = vop2_find_win_by_phys_id(vop2, primary_plane_id);
	if (!win_data) {
		printf("invalid win id %d\n", primary_plane_id);
		return -ENODEV;
	}

	if (vop2->version == VOP_VERSION_RK3588) {
		if (vop2_power_domain_on(vop2, win_data->pd_id))
			printf("open vp%d plane pd fail\n", cstate->crtc_id);
	}

	if (cstate->splice_mode) {
		if (win_data->splice_win_id) {
			splice_win_data = vop2_find_win_by_phys_id(vop2, win_data->splice_win_id);
			splice_win_data->splice_mode_right = true;

			if (vop2_power_domain_on(vop2, splice_win_data->pd_id))
				printf("splice mode: open vp%d plane pd fail\n", cstate->splice_crtc_id);

			vop2_calc_display_rect_for_splice(state);
			if (win_data->type == CLUSTER_LAYER)
				vop2_set_cluster_win(state, splice_win_data);
			else
				vop2_set_smart_win(state, splice_win_data);
		} else {
			printf("ERROR: splice mode is unsupported by plane %s\n",
			       get_plane_name(primary_plane_id, plane_name));
			return -EINVAL;
		}
	}

	if (win_data->type == CLUSTER_LAYER)
		vop2_set_cluster_win(state, win_data);
	else
		vop2_set_smart_win(state, win_data);

	printf("VOP VP%d enable %s[%dx%d->%dx%d@%dx%d] fmt[%d] addr[0x%x]\n",
		cstate->crtc_id, get_plane_name(primary_plane_id, plane_name),
		cstate->src_rect.w, cstate->src_rect.h, cstate->crtc_rect.w, cstate->crtc_rect.h,
		cstate->crtc_rect.x, cstate->crtc_rect.y, cstate->format,
		cstate->dma_addr);

	return 0;
}

static int rockchip_vop2_prepare(struct display_state *state)
{
	return 0;
}

static void vop2_dsc_cfg_done(struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	u8 dsc_id = cstate->dsc_id;
	u32 ctrl_regs_offset = (dsc_id * 0x30);

	if (conn_state->output_flags & ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE) {
		vop2_mask_write(vop2, RK3588_DSC_8K_CFG_DONE, EN_MASK,
				DSC_CFG_DONE_SHIFT, 1, false);
		vop2_mask_write(vop2, RK3588_DSC_8K_CFG_DONE + 0x30, EN_MASK,
				DSC_CFG_DONE_SHIFT, 1, false);
	} else {
		vop2_mask_write(vop2, RK3588_DSC_8K_CFG_DONE + ctrl_regs_offset, EN_MASK,
				DSC_CFG_DONE_SHIFT, 1, false);
	}
}

static int rockchip_vop2_enable(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id) | (BIT(cstate->crtc_id) << 16);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			STANDBY_EN_SHIFT, 0, false);

	if (cstate->splice_mode)
		cfg_done |= BIT(cstate->splice_crtc_id) | (BIT(cstate->splice_crtc_id) << 16);

	vop2_writel(vop2, RK3568_REG_CFG_DONE, cfg_done);

	if (cstate->dsc_enable)
		vop2_dsc_cfg_done(state);

	return 0;
}

static int rockchip_vop2_disable(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id) | (BIT(cstate->crtc_id) << 16);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			STANDBY_EN_SHIFT, 1, false);

	if (cstate->splice_mode)
		cfg_done |= BIT(cstate->splice_crtc_id) | (BIT(cstate->splice_crtc_id) << 16);

	vop2_writel(vop2, RK3568_REG_CFG_DONE, cfg_done);

	return 0;
}

static int rockchip_vop2_get_cursor_plane(struct display_state *state, u32 plane_mask, int cursor_plane)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	int i = 0;
	int correct_cursor_plane = -1;
	int plane_type = -1;

	if (cursor_plane < 0)
		return -1;

	if (plane_mask & (1 << cursor_plane))
		return cursor_plane;

	/* Get current cursor plane type */
	for (i = 0; i < vop2->data->nr_layers; i++) {
		if (vop2->data->plane_table[i].plane_id == cursor_plane) {
			plane_type = vop2->data->plane_table[i].plane_type;
			break;
		}
	}

	/* Get the other same plane type plane id */
	for (i = 0; i < vop2->data->nr_layers; i++) {
		if (vop2->data->plane_table[i].plane_type == plane_type &&
		    vop2->data->plane_table[i].plane_id != cursor_plane) {
			correct_cursor_plane = vop2->data->plane_table[i].plane_id;
			break;
		}
	}

	/* To check whether the new correct_cursor_plane is attach to current vp */
	if (correct_cursor_plane < 0 || !(plane_mask & (1 << correct_cursor_plane))) {
		printf("error: faild to find correct plane as cursor plane\n");
		return -1;
	}

	printf("vp%d adjust cursor plane from %d to %d\n",
	       cstate->crtc_id, cursor_plane, correct_cursor_plane);

	return correct_cursor_plane;
}

static int rockchip_vop2_fixup_dts(struct display_state *state, void *blob)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	ofnode vp_node;
	struct device_node *port_parent_node = cstate->ports_node;
	static bool vop_fix_dts;
	const char *path;
	u32 plane_mask = 0;
	int vp_id = 0;
	int cursor_plane_id = -1;

	if (vop_fix_dts)
		return 0;

	ofnode_for_each_subnode(vp_node, np_to_ofnode(port_parent_node)) {
		path = vp_node.np->full_name;
		plane_mask = vop2->vp_plane_mask[vp_id].plane_mask;

		if (cstate->crtc->assign_plane)
			continue;
		cursor_plane_id = rockchip_vop2_get_cursor_plane(state, plane_mask,
								 cstate->crtc->vps[vp_id].cursor_plane);
		printf("vp%d, plane_mask:0x%x, primary-id:%d, curser-id:%d\n",
		       vp_id, plane_mask,
		       vop2->vp_plane_mask[vp_id].primary_plane_id,
		       cursor_plane_id);

		do_fixup_by_path_u32(blob, path, "rockchip,plane-mask",
				     plane_mask, 1);
		do_fixup_by_path_u32(blob, path, "rockchip,primary-plane",
				     vop2->vp_plane_mask[vp_id].primary_plane_id, 1);
		if (cursor_plane_id >= 0)
			do_fixup_by_path_u32(blob, path, "cursor-win-id",
					     cursor_plane_id, 1);
		vp_id++;
	}

	vop_fix_dts = true;

	return 0;
}

static struct vop2_plane_table rk356x_plane_table[ROCKCHIP_VOP2_LAYER_MAX] = {
	{ROCKCHIP_VOP2_CLUSTER0, CLUSTER_LAYER},
	{ROCKCHIP_VOP2_CLUSTER1, CLUSTER_LAYER},
	{ROCKCHIP_VOP2_ESMART0, ESMART_LAYER},
	{ROCKCHIP_VOP2_ESMART1, ESMART_LAYER},
	{ROCKCHIP_VOP2_SMART0, SMART_LAYER},
	{ROCKCHIP_VOP2_SMART0, SMART_LAYER},
};

static struct vop2_vp_plane_mask rk356x_vp_plane_mask[VOP2_VP_MAX][VOP2_VP_MAX] = {
	{ /* one display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_SMART0,
			.attached_layers_nr = 6,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_ESMART0, ROCKCHIP_VOP2_SMART0,
				  ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_ESMART1, ROCKCHIP_VOP2_SMART1
				},
		},
		{/* second display */},
		{/* third  display */},
		{/* fourth display */},
	},

	{ /* two display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_SMART0,
			.attached_layers_nr = 3,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_ESMART0, ROCKCHIP_VOP2_SMART0
				},
		},

		{/* second display */
			.primary_plane_id = ROCKCHIP_VOP2_SMART1,
			.attached_layers_nr = 3,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_ESMART1, ROCKCHIP_VOP2_SMART1
				},
		},
		{/* third  display */},
		{/* fourth display */},
	},

	{ /* three display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_SMART0,
			.attached_layers_nr = 3,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_ESMART0, ROCKCHIP_VOP2_SMART0
				},
		},

		{/* second display */
			.primary_plane_id = ROCKCHIP_VOP2_SMART1,
			.attached_layers_nr = 2,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_SMART1
				},
		},

		{/* third  display */
			.primary_plane_id = ROCKCHIP_VOP2_ESMART1,
			.attached_layers_nr = 1,
			.attached_layers = { ROCKCHIP_VOP2_ESMART1 },
		},

		{/* fourth display */},
	},

	{/* reserved for four display policy */},
};

static struct vop2_win_data rk3568_win_data[6] = {
	{
		.name = "Cluster0",
		.phys_id = ROCKCHIP_VOP2_CLUSTER0,
		.type = CLUSTER_LAYER,
		.win_sel_port_offset = 0,
		.layer_sel_win_id = 0,
		.reg_offset = 0,
	},

	{
		.name = "Cluster1",
		.phys_id = ROCKCHIP_VOP2_CLUSTER1,
		.type = CLUSTER_LAYER,
		.win_sel_port_offset = 1,
		.layer_sel_win_id = 1,
		.reg_offset = 0x200,
	},

	{
		.name = "Esmart0",
		.phys_id = ROCKCHIP_VOP2_ESMART0,
		.type = ESMART_LAYER,
		.win_sel_port_offset = 4,
		.layer_sel_win_id = 2,
		.reg_offset = 0,
	},

	{
		.name = "Esmart1",
		.phys_id = ROCKCHIP_VOP2_ESMART1,
		.type = ESMART_LAYER,
		.win_sel_port_offset = 5,
		.layer_sel_win_id = 6,
		.reg_offset = 0x200,
	},

	{
		.name = "Smart0",
		.phys_id = ROCKCHIP_VOP2_SMART0,
		.type = SMART_LAYER,
		.win_sel_port_offset = 6,
		.layer_sel_win_id = 3,
		.reg_offset = 0x400,
	},

	{
		.name = "Smart1",
		.phys_id = ROCKCHIP_VOP2_SMART1,
		.type = SMART_LAYER,
		.win_sel_port_offset = 7,
		.layer_sel_win_id = 7,
		.reg_offset = 0x600,
	},
};

static struct vop2_vp_data rk3568_vp_data[3] = {
	{
		.feature = VOP_FEATURE_OUTPUT_10BIT,
		.pre_scan_max_dly = 42,
		.max_output = {4096, 2304},
	},
	{
		.feature = 0,
		.pre_scan_max_dly = 40,
		.max_output = {2048, 1536},
	},
	{
		.feature = 0,
		.pre_scan_max_dly = 40,
		.max_output = {1920, 1080},
	},
};

const struct vop2_data rk3568_vop = {
	.version = VOP_VERSION_RK3568,
	.nr_vps = 3,
	.vp_data = rk3568_vp_data,
	.win_data = rk3568_win_data,
	.plane_mask = rk356x_vp_plane_mask[0],
	.plane_table = rk356x_plane_table,
	.nr_layers = 6,
	.nr_mixers = 5,
	.nr_gammas = 1,
};

static struct vop2_plane_table rk3588_plane_table[ROCKCHIP_VOP2_LAYER_MAX] = {
	{ROCKCHIP_VOP2_CLUSTER0, CLUSTER_LAYER},
	{ROCKCHIP_VOP2_CLUSTER1, CLUSTER_LAYER},
	{ROCKCHIP_VOP2_CLUSTER2, CLUSTER_LAYER},
	{ROCKCHIP_VOP2_CLUSTER3, CLUSTER_LAYER},
	{ROCKCHIP_VOP2_ESMART0, ESMART_LAYER},
	{ROCKCHIP_VOP2_ESMART1, ESMART_LAYER},
	{ROCKCHIP_VOP2_ESMART2, ESMART_LAYER},
	{ROCKCHIP_VOP2_ESMART3, ESMART_LAYER},
};

static struct vop2_vp_plane_mask rk3588_vp_plane_mask[VOP2_VP_MAX][VOP2_VP_MAX] = {
	{ /* one display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER0,
			.attached_layers_nr = 8,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_ESMART0, ROCKCHIP_VOP2_ESMART2,
				  ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_ESMART1, ROCKCHIP_VOP2_ESMART3,
				  ROCKCHIP_VOP2_CLUSTER2, ROCKCHIP_VOP2_CLUSTER3
			},
		},
		{/* second display */},
		{/* third  display */},
		{/* fourth display */},
	},

	{ /* two display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER0,
			.attached_layers_nr = 4,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_ESMART0,
				  ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_ESMART1
			},
		},

		{/* second display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER2,
			.attached_layers_nr = 4,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER2, ROCKCHIP_VOP2_ESMART2,
				  ROCKCHIP_VOP2_CLUSTER3, ROCKCHIP_VOP2_ESMART3
			},
		},
		{/* third  display */},
		{/* fourth display */},
	},

	{ /* three display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER0,
			.attached_layers_nr = 3,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_ESMART0
			},
		},

		{/* second display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER2,
			.attached_layers_nr = 3,
			.attached_layers = {
				  ROCKCHIP_VOP2_CLUSTER2, ROCKCHIP_VOP2_CLUSTER3, ROCKCHIP_VOP2_ESMART1
			},
		},

		{/* third  display */
			.primary_plane_id = ROCKCHIP_VOP2_ESMART2,
			.attached_layers_nr = 2,
			.attached_layers = { ROCKCHIP_VOP2_ESMART2, ROCKCHIP_VOP2_ESMART3 },
		},

		{/* fourth display */},
	},

	{ /* four display policy */
		{/* main display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER0,
			.attached_layers_nr = 2,
			.attached_layers = { ROCKCHIP_VOP2_CLUSTER0, ROCKCHIP_VOP2_ESMART0 },
		},

		{/* second display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER1,
			.attached_layers_nr = 2,
			.attached_layers = { ROCKCHIP_VOP2_CLUSTER1, ROCKCHIP_VOP2_ESMART1 },
		},

		{/* third  display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER2,
			.attached_layers_nr = 2,
			.attached_layers = { ROCKCHIP_VOP2_CLUSTER2, ROCKCHIP_VOP2_ESMART2 },
		},

		{/* fourth display */
			.primary_plane_id = ROCKCHIP_VOP2_CLUSTER3,
			.attached_layers_nr = 2,
			.attached_layers = { ROCKCHIP_VOP2_CLUSTER3, ROCKCHIP_VOP2_ESMART3 },
		},
	},

};

static struct vop2_win_data rk3588_win_data[8] = {
	{
		.name = "Cluster0",
		.phys_id = ROCKCHIP_VOP2_CLUSTER0,
		.splice_win_id = ROCKCHIP_VOP2_CLUSTER1,
		.type = CLUSTER_LAYER,
		.win_sel_port_offset = 0,
		.layer_sel_win_id = 0,
		.reg_offset = 0,
		.axi_id = 0,
		.axi_yrgb_id = 2,
		.axi_uv_id = 3,
		.pd_id = VOP2_PD_CLUSTER0,
	},

	{
		.name = "Cluster1",
		.phys_id = ROCKCHIP_VOP2_CLUSTER1,
		.type = CLUSTER_LAYER,
		.win_sel_port_offset = 1,
		.layer_sel_win_id = 1,
		.reg_offset = 0x200,
		.axi_id = 0,
		.axi_yrgb_id = 6,
		.axi_uv_id = 7,
		.pd_id = VOP2_PD_CLUSTER1,
	},

	{
		.name = "Cluster2",
		.phys_id = ROCKCHIP_VOP2_CLUSTER2,
		.splice_win_id = ROCKCHIP_VOP2_CLUSTER3,
		.type = CLUSTER_LAYER,
		.win_sel_port_offset = 2,
		.layer_sel_win_id = 4,
		.reg_offset = 0x400,
		.axi_id = 1,
		.axi_yrgb_id = 2,
		.axi_uv_id = 3,
		.pd_id = VOP2_PD_CLUSTER2,
	},

	{
		.name = "Cluster3",
		.phys_id = ROCKCHIP_VOP2_CLUSTER3,
		.type = CLUSTER_LAYER,
		.win_sel_port_offset = 3,
		.layer_sel_win_id = 5,
		.reg_offset = 0x600,
		.axi_id = 1,
		.axi_yrgb_id = 6,
		.axi_uv_id = 7,
		.pd_id = VOP2_PD_CLUSTER3,
	},

	{
		.name = "Esmart0",
		.phys_id = ROCKCHIP_VOP2_ESMART0,
		.splice_win_id = ROCKCHIP_VOP2_ESMART1,
		.type = ESMART_LAYER,
		.win_sel_port_offset = 4,
		.layer_sel_win_id = 2,
		.reg_offset = 0,
		.axi_id = 0,
		.axi_yrgb_id = 0x0a,
		.axi_uv_id = 0x0b,
	},

	{
		.name = "Esmart1",
		.phys_id = ROCKCHIP_VOP2_ESMART1,
		.type = ESMART_LAYER,
		.win_sel_port_offset = 5,
		.layer_sel_win_id = 3,
		.reg_offset = 0x200,
		.axi_id = 0,
		.axi_yrgb_id = 0x0c,
		.axi_uv_id = 0x0d,
		.pd_id = VOP2_PD_ESMART,
	},

	{
		.name = "Esmart2",
		.phys_id = ROCKCHIP_VOP2_ESMART2,
		.splice_win_id = ROCKCHIP_VOP2_ESMART3,
		.type = ESMART_LAYER,
		.win_sel_port_offset = 6,
		.layer_sel_win_id = 6,
		.reg_offset = 0x400,
		.axi_id = 1,
		.axi_yrgb_id = 0x0a,
		.axi_uv_id = 0x0b,
		.pd_id = VOP2_PD_ESMART,
	},

	{
		.name = "Esmart3",
		.phys_id = ROCKCHIP_VOP2_ESMART3,
		.type = ESMART_LAYER,
		.win_sel_port_offset = 7,
		.layer_sel_win_id = 7,
		.reg_offset = 0x600,
		.axi_id = 1,
		.axi_yrgb_id = 0x0c,
		.axi_uv_id = 0x0d,
		.pd_id = VOP2_PD_ESMART,
	},
};

static struct dsc_error_info dsc_ecw[] = {
	{0x00000000, "no error detected by DSC encoder"},
	{0x0030ffff, "bits per component error"},
	{0x0040ffff, "multiple mode error"},
	{0x0050ffff, "line buffer depth error"},
	{0x0060ffff, "minor version error"},
	{0x0070ffff, "picture height error"},
	{0x0080ffff, "picture width error"},
	{0x0090ffff, "number of slices error"},
	{0x00c0ffff, "slice height Error "},
	{0x00d0ffff, "slice width error"},
	{0x00e0ffff, "second line BPG offset error"},
	{0x00f0ffff, "non second line BPG offset error"},
	{0x0100ffff, "PPS ID error"},
	{0x0110ffff, "bits per pixel (BPP) Error"},
	{0x0120ffff, "buffer flow error"},  /* dsc_buffer_flow */

	{0x01510001, "slice 0 RC buffer model overflow error"},
	{0x01510002, "slice 1 RC buffer model overflow error"},
	{0x01510004, "slice 2 RC buffer model overflow error"},
	{0x01510008, "slice 3 RC buffer model overflow error"},
	{0x01510010, "slice 4 RC buffer model overflow error"},
	{0x01510020, "slice 5 RC buffer model overflow error"},
	{0x01510040, "slice 6 RC buffer model overflow error"},
	{0x01510080, "slice 7 RC buffer model overflow error"},

	{0x01610001, "slice 0 RC buffer model underflow error"},
	{0x01610002, "slice 1 RC buffer model underflow error"},
	{0x01610004, "slice 2 RC buffer model underflow error"},
	{0x01610008, "slice 3 RC buffer model underflow error"},
	{0x01610010, "slice 4 RC buffer model underflow error"},
	{0x01610020, "slice 5 RC buffer model underflow error"},
	{0x01610040, "slice 6 RC buffer model underflow error"},
	{0x01610080, "slice 7 RC buffer model underflow error"},

	{0xffffffff, "unsuccessful RESET cycle status"},
	{0x00a0ffff, "ICH full error precision settings error"},
	{0x0020ffff, "native mode"},
};

static struct dsc_error_info dsc_buffer_flow[] = {
	{0x00000000, "rate buffer status"},
	{0x00000001, "line buffer status"},
	{0x00000002, "decoder model status"},
	{0x00000003, "pixel buffer status"},
	{0x00000004, "balance fifo buffer status"},
	{0x00000005, "syntax element fifo status"},
};

static struct vop2_dsc_data rk3588_dsc_data[] = {
	{
		.id = ROCKCHIP_VOP2_DSC_8K,
		.pd_id = VOP2_PD_DSC_8K,
		.max_slice_num = 8,
		.max_linebuf_depth = 11,
		.min_bits_per_pixel = 9,
		.dsc_txp_clk_src_name = "dsc_8k_txp_clk_src",
		.dsc_txp_clk_name = "dsc_8k_txp_clk",
		.dsc_pxl_clk_name = "dsc_8k_pxl_clk",
		.dsc_cds_clk_name = "dsc_8k_cds_clk",
	},

	{
		.id = ROCKCHIP_VOP2_DSC_4K,
		.pd_id = VOP2_PD_DSC_4K,
		.max_slice_num = 2,
		.max_linebuf_depth = 11,
		.min_bits_per_pixel = 9,
		.dsc_txp_clk_src_name = "dsc_4k_txp_clk_src",
		.dsc_txp_clk_name = "dsc_4k_txp_clk",
		.dsc_pxl_clk_name = "dsc_4k_pxl_clk",
		.dsc_cds_clk_name = "dsc_4k_cds_clk",
	},
};

static struct vop2_vp_data rk3588_vp_data[4] = {
	{
		.splice_vp_id = 1,
		.feature = VOP_FEATURE_OUTPUT_10BIT,
		.pre_scan_max_dly = 54,
		.max_dclk = 600000,
		.max_output = {7680, 4320},
	},
	{
		.feature = VOP_FEATURE_OUTPUT_10BIT,
		.pre_scan_max_dly = 54,
		.max_dclk = 600000,
		.max_output = {4096, 2304},
	},
	{
		.feature = VOP_FEATURE_OUTPUT_10BIT,
		.pre_scan_max_dly = 52,
		.max_dclk = 600000,
		.max_output = {4096, 2304},
	},
	{
		.feature = 0,
		.pre_scan_max_dly = 52,
		.max_dclk = 200000,
		.max_output = {1920, 1080},
	},
};

static struct vop2_power_domain_data rk3588_vop_pd_data[] = {
	{
	  .id = VOP2_PD_CLUSTER0,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_CLUSTER0),
	},
	{
	  .id = VOP2_PD_CLUSTER1,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_CLUSTER1),
	  .parent_id = VOP2_PD_CLUSTER0,
	},
	{
	  .id = VOP2_PD_CLUSTER2,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_CLUSTER2),
	  .parent_id = VOP2_PD_CLUSTER0,
	},
	{
	  .id = VOP2_PD_CLUSTER3,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_CLUSTER3),
	  .parent_id = VOP2_PD_CLUSTER0,
	},
	{
	  .id = VOP2_PD_ESMART,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_ESMART1) |
			    BIT(ROCKCHIP_VOP2_ESMART2) |
			    BIT(ROCKCHIP_VOP2_ESMART3),
	},
	{
	  .id = VOP2_PD_DSC_8K,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_DSC_8K),
	},
	{
	  .id = VOP2_PD_DSC_4K,
	  .module_id_mask = BIT(ROCKCHIP_VOP2_DSC_4K),
	},
};

const struct vop2_data rk3588_vop = {
	.version = VOP_VERSION_RK3588,
	.nr_vps = 4,
	.vp_data = rk3588_vp_data,
	.win_data = rk3588_win_data,
	.plane_mask = rk3588_vp_plane_mask[0],
	.plane_table = rk3588_plane_table,
	.pd = rk3588_vop_pd_data,
	.dsc = rk3588_dsc_data,
	.dsc_error_ecw = dsc_ecw,
	.dsc_error_buffer_flow = dsc_buffer_flow,
	.nr_layers = 8,
	.nr_mixers = 7,
	.nr_gammas = 4,
	.nr_pd = ARRAY_SIZE(rk3588_vop_pd_data),
	.nr_dscs = 2,
	.nr_dsc_ecw = ARRAY_SIZE(dsc_ecw),
	.nr_dsc_buffer_flow = ARRAY_SIZE(dsc_buffer_flow),
};

const struct rockchip_crtc_funcs rockchip_vop2_funcs = {
	.preinit = rockchip_vop2_preinit,
	.prepare = rockchip_vop2_prepare,
	.init = rockchip_vop2_init,
	.set_plane = rockchip_vop2_set_plane,
	.enable = rockchip_vop2_enable,
	.disable = rockchip_vop2_disable,
	.fixup_dts = rockchip_vop2_fixup_dts,
};
