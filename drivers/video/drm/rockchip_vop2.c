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
#include <asm/arch/cpu.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <linux/list.h>
#include <linux/media-bus-format.h>
#include <clk.h>
#include <asm/arch/clock.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <dm/device.h>
#include <dm/read.h>
#include <fixp-arith.h>
#include <syscon.h>

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
#define HDMI0_EN_SHIFT				1
#define EDP0_EN_SHIFT				3
#define MIPI0_EN_SHIFT				4
#define MIPI1_EN_SHIFT				20
#define LVDS0_EN_SHIFT				5
#define LVDS1_EN_SHIFT				24
#define BT1120_EN_SHIFT				6
#define BT656_EN_SHIFT				7
#define IF_MUX_MASK				3
#define RGB_MUX_SHIFT				8
#define HDMI0_MUX_SHIFT				10
#define EDP0_MUX_SHIFT				14
#define MIPI0_MUX_SHIFT				16
#define MIPI1_MUX_SHIFT				21
#define LVDS0_MUX_SHIFT				18
#define LVDS1_MUX_SHIFT				25

#define RK3568_DSP_IF_CTRL			0x02c
#define LVDS_DUAL_EN_SHIFT			0
#define LVDS_DUAL_LEFT_RIGHT_EN_SHIFT		1
#define LVDS_DUAL_SWAP_EN_SHIFT			2
#define RK3568_DSP_IF_POL			0x030
#define IF_CTRL_REG_DONE_IMD_MASK		1
#define IF_CTRL_REG_DONE_IMD_SHIFT		28
#define IF_CRTL_MIPI_DCLK_POL_SHIT		19
#define IF_CRTL_EDP_DCLK_POL_SHIT		15
#define IF_CRTL_HDMI_DCLK_POL_SHIT		7
#define IF_CRTL_HDMI_PIN_POL_MASK		0x7
#define IF_CRTL_HDMI_PIN_POL_SHIT		4
#define IF_CRTL_RGB_LVDS_DCLK_POL_SHIT		3
#define RK3568_SYS_OTP_WIN_EN			0x50
#define OTP_WIN_EN_SHIFT			0
#define RK3568_SYS_LUT_PORT_SEL			0x58
#define GAMMA_PORT_SEL_MASK			0x3
#define GAMMA_PORT_SEL_SHIFT			0

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

/* Overlay registers definition    */
#define RK3568_OVL_CTRL				0x600
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
#define DSP_RB_SWAP				2
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

#define RK3568_VP0_COLOR_BAR_CTRL		0xC08
#define RK3568_VP0_3D_LUT_CTRL			0xC10
#define VP0_3D_LUT_EN_SHIFT				0
#define VP0_3D_LUT_UPDATE_SHIFT			2

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
#define RK3568_CLUSTER0_WIN0_CTRL1		0x1004
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
#define YMIRROR_EN_SHIFT			31
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

#define RK3568_MAX_REG				0x1ED0

#define RK3568_GRF_VO_CON1			0x0364
#define GRF_BT656_CLK_INV_SHIFT			1
#define GRF_BT1120_CLK_INV_SHIFT		2
#define GRF_RGB_DCLK_INV_SHIFT			3

#define VOP2_LAYER_MAX				8

#define VOP_FEATURE_OUTPUT_10BIT		BIT(0)

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

struct vop2_layer {
	u8 id;
	/**
	 * @win_phys_id: window id of the layer selected.
	 * Every layer must make sure to select different
	 * windows of others.
	 */
	u8 win_phys_id;
};

struct vop2_win_data {
	char *name;
	u8 phys_id;
	u8 win_sel_port_offset;
	u8 layer_sel_win_id;
	u32 reg_offset;
};

struct vop2_vp_data {
	u32 feature;
	u8 pre_scan_max_dly;
	struct vop_rect max_output;
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

struct vop2_data {
	u32 version;
	struct vop2_vp_data *vp_data;
	struct vop2_win_data *win_data;
	struct vop2_vp_plane_mask *plane_mask;
	struct vop2_plane_table *plane_table;
	u8 nr_vps;
	u8 nr_layers;
	u8 nr_mixers;
	u8 nr_gammas;
};

struct vop2 {
	u32 *regsbak;
	void *regs;
	void *grf;
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

static u8 vop2_vp_primary_plane_order[VOP2_VP_MAX] = {
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

	for (i = 0; i < vop2->data->nr_vps; i++) {
		if (plane_mask & BIT(vop2_vp_primary_plane_order[i]))
			return vop2_vp_primary_plane_order[i];
	}

	return ROCKCHIP_VOP2_SMART0;
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

static inline void vop2_grf_writel(struct vop2 *vop, u32 offset,
				   u32 mask, u32 shift, u32 v)
{
	u32 val = 0;

	val = (v << shift) | (mask << (shift + 16));
	writel(val, vop->grf + offset);
}

static inline int us_to_vertical_line(struct drm_display_mode *mode, int us)
{
	return us * mode->clock / mode->htotal / 1000;
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
	if ((bus_format == MEDIA_BUS_FMT_YUV8_1X24 ||
	     bus_format == MEDIA_BUS_FMT_YUV10_1X30) &&
	    (output_mode == ROCKCHIP_OUT_MODE_AAAA ||
	     output_mode == ROCKCHIP_OUT_MODE_P888))
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

static void vop2_tv_config_update(struct display_state *state, struct vop2 *vop2)
{
	struct connector_state *conn_state = &state->conn_state;
	struct base_bcsh_info *bcsh_info;
	struct crtc_state *cstate = &state->crtc_state;
	int brightness, contrast, saturation, hue, sin_hue, cos_hue;
	bool bcsh_en = false, post_r2y_en = false, post_y2r_en = false;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	int post_csc_mode;

	if (!conn_state->disp_info)
		return;
	bcsh_info = &conn_state->disp_info->bcsh_info;
	if (!bcsh_info)
		return;

	if (bcsh_info->brightness != 50 ||
	    bcsh_info->contrast != 50 ||
	    bcsh_info->saturation != 50 || bcsh_info->hue != 50)
		bcsh_en = true;

	if (bcsh_en) {
		if (!cstate->yuv_overlay)
			post_r2y_en = 1;
		if (!is_yuv_output(conn_state->bus_format))
			post_y2r_en = 1;
	} else {
		if (!cstate->yuv_overlay && is_yuv_output(conn_state->bus_format))
			post_r2y_en = 1;
		if (cstate->yuv_overlay && !is_yuv_output(conn_state->bus_format))
			post_y2r_en = 1;
	}

	post_csc_mode = vop2_convert_csc_mode(conn_state->color_space);


	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_R2Y_MASK,
			BCSH_CTRL_R2Y_SHIFT, post_r2y_en, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_Y2R_MASK,
			BCSH_CTRL_Y2R_SHIFT, post_y2r_en, false);

	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_R2Y_CSC_MODE_MASK,
			BCSH_CTRL_R2Y_CSC_MODE_SHIFT, post_csc_mode, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_CTRL + vp_offset, BCSH_CTRL_Y2R_CSC_MODE_MASK,
			BCSH_CTRL_Y2R_CSC_MODE_SHIFT, post_csc_mode, false);
	if (!bcsh_en) {
		vop2_mask_write(vop2, RK3568_VP0_BCSH_COLOR + vp_offset,
				BCSH_EN_MASK, BCSH_EN_SHIFT, 0, false);
		return;
	}

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

	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_BRIGHTNESS_MASK, BCSH_BRIGHTNESS_SHIFT,
			brightness, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_CONTRAST_MASK, BCSH_CONTRAST_SHIFT, contrast, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			BCSH_SATURATION_MASK, BCSH_SATURATION_SHIFT,
			saturation * contrast / 0x100, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_H + vp_offset,
			BCSH_SIN_HUE_MASK, BCSH_SIN_HUE_SHIFT, sin_hue, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_H + vp_offset,
			BCSH_COS_HUE_MASK, BCSH_COS_HUE_SHIFT, cos_hue, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_BCS + vp_offset,
			 BCSH_OUT_MODE_MASK, BCSH_OUT_MODE_SHIFT,
			BCSH_OUT_MODE_NORMAL_VIDEO, false);
	vop2_mask_write(vop2, RK3568_VP0_BCSH_COLOR + vp_offset,
			BCSH_EN_MASK, BCSH_EN_SHIFT, 1, false);
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
	u32 bg_ovl_dly, bg_dly, pre_scan_dly;
	u16 hsync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;

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

	bg_ovl_dly = cstate->crtc->vps[cstate->crtc_id].bg_ovl_dly;
	bg_dly =  vop2->data->vp_data[cstate->crtc_id].pre_scan_max_dly;
	bg_dly -= bg_ovl_dly;
	pre_scan_dly = bg_dly + (hdisplay >> 1) - 1;
	pre_scan_dly = (pre_scan_dly << 16) | hsync_len;
	vop2_mask_write(vop2, RK3568_VP0_BG_MIX_CTRL + cstate->crtc_id * 4,
			BG_MIX_CTRL_MASK, BG_MIX_CTRL_SHIFT, bg_dly, false);
	vop2_writel(vop2, RK3568_VP0_PRE_SCAN_HTIMING + vp_offset, pre_scan_dly);
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

	memcpy(vop2->regsbak, vop2->regs, vop2->reg_len);
	vop2_mask_write(vop2, RK3568_OVL_CTRL, EN_MASK,
			OVL_PORT_MUX_REG_DONE_IMD_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
			IF_CTRL_REG_DONE_IMD_SHIFT, 1, false);

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
			/* rk3566 only support 3+3 policy */
			if (soc_is_rk3566() && active_vp_num == 1) {
				if (cstate->crtc->vps[i].enable) {
					for (j = 0; j < 3; j++) {
						layer_phy_id = vop2->vp_plane_mask[i].attached_layers[j];
						vop2->vp_plane_mask[i].plane_mask |= BIT(layer_phy_id);
					}
				}
			} else {
				for (j = 0; j < layer_nr; j++) {
					layer_phy_id = vop2->vp_plane_mask[i].attached_layers[j];
					vop2->vp_plane_mask[i].plane_mask |= BIT(layer_phy_id);
				}
			}
		}
	}

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
			win_data = &vop2->data->win_data[layer_phy_id];
			vop2_mask_write(vop2, RK3568_OVL_LAYER_SEL, LAYER_SEL_MASK,
					shift, win_data->layer_sel_win_id, false);
			shift += 4;
		}
	}

	/* win sel port */
	for (i = 0; i < vop2->data->nr_vps; i++) {
		layer_nr = vop2->vp_plane_mask[i].attached_layers_nr;
		for (j = 0; j < layer_nr; j++) {
			if (!cstate->crtc->vps[i].enable)
				continue;
			layer_phy_id = vop2->vp_plane_mask[i].attached_layers[j];
			win_data = &vop2->data->win_data[layer_phy_id];
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
		if (cstate->crtc->vps[i].enable) {
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

	vop2_writel(vop2, RK3568_AUTO_GATING_CTRL, 0);

	vop2->global_init = true;
}

static int vop2_initial(struct vop2 *vop2, struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	char dclk_name[9];
	struct clk dclk;
	int ret;

	/* Process 'assigned-{clocks/clock-parents/clock-rates}' properties */
	ret = clk_set_defaults(cstate->dev);
	if (ret)
		debug("%s clk_set_defaults failed %d\n", __func__, ret);
	snprintf(dclk_name, sizeof(dclk_name), "dclk_vp%d", cstate->crtc_id);
	ret = clk_get_by_name(cstate->dev, dclk_name, &dclk);
	if (!ret)
		ret = clk_set_rate(&dclk, mode->clock * 1000);
	if (IS_ERR_VALUE(ret)) {
		printf("%s: Failed to set vp%d dclk[%d khz]: ret=%d\n",
		       __func__, cstate->crtc_id, mode->clock, ret);
		return ret;
	}

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
	}

	cstate->private = rockchip_vop2;
	cstate->max_output = vop2_data->vp_data[cstate->crtc_id].max_output;
	cstate->feature = vop2_data->vp_data[cstate->crtc_id].feature;

	vop2_global_initial(rockchip_vop2, state);

	return 0;
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
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 val;
	bool dclk_inv;
	u8 dither_down_en = 0;
	u8 pre_dither_down_en = 0;
	char output_type_name[30] = {0};

	printf("VOP update mode to: %dx%d%s%d, type:%s for VP%d\n",
	       mode->hdisplay, mode->vdisplay,
	       mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "p",
	       mode->vscan,
	       get_output_if_name(conn_state->output_if, output_type_name),
	       cstate->crtc_id);

	vop2_initial(vop2, state);
	dclk_inv = (mode->flags & DRM_MODE_FLAG_PPIXDATA) ? 0 : 1;
	val = (mode->flags & DRM_MODE_FLAG_NHSYNC) ? 0 : BIT(HSYNC_POSITIVE);
	val |= (mode->flags & DRM_MODE_FLAG_NVSYNC) ? 0 : BIT(VSYNC_POSITIVE);

	if (conn_state->output_if & VOP_OUTPUT_IF_RGB) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RGB_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_grf_writel(vop2, RK3568_GRF_VO_CON1, EN_MASK,
				GRF_RGB_DCLK_INV_SHIFT, dclk_inv);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_BT1120) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RGB_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK,
				BT1120_EN_SHIFT, 1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_grf_writel(vop2, RK3568_GRF_VO_CON1, EN_MASK,
				GRF_BT1120_CLK_INV_SHIFT, !dclk_inv);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_BT656) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, BT656_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_grf_writel(vop2, RK3568_GRF_VO_CON1, EN_MASK,
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

	if (conn_state->output_mode == ROCKCHIP_OUT_MODE_AAAA &&
	    !(cstate->feature & VOP_FEATURE_OUTPUT_10BIT))
		conn_state->output_mode = ROCKCHIP_OUT_MODE_P888;

	if (is_uv_swap(conn_state->bus_format, conn_state->output_mode))
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset,
				DATA_SWAP_MASK, DATA_SWAP_SHIFT, DSP_RB_SWAP,
				false);
	else
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset,
				DATA_SWAP_MASK, DATA_SWAP_SHIFT, 0,
				false);

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
	} else {
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				INTERLACE_EN_SHIFT, 0, false);
		vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
				P2I_EN_SHIFT, 0, false);
	}
	vop2_writel(vop2, RK3568_VP0_DSP_VTOTAL_VS_END + vp_offset,
		    (vtotal << 16) | vsync_len);
	val = !!(mode->flags & DRM_MODE_FLAG_DBLCLK);
	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			CORE_DCLK_DIV_EN_SHIFT, val, false);

	if (conn_state->output_mode == ROCKCHIP_OUT_MODE_YUV420)
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
				DCLK_DIV2_MASK, DCLK_DIV2_SHIFT, 0x3, false);
	else
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset,
				DCLK_DIV2_MASK, DCLK_DIV2_SHIFT, 0, false);

	if (yuv_overlay)
		val = 0x20010200;
	else
		val = 0;
	vop2_writel(vop2, RK3568_VP0_DSP_BG + vp_offset, val);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			POST_DSP_OUT_R2Y_SHIFT, yuv_overlay, false);

	vop2_tv_config_update(state, vop2);
	vop2_post_config(state, vop2);

	return 0;
}

static void vop2_setup_scale(struct vop2 *vop2, uint32_t win_offset,
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

static int rockchip_vop2_set_plane(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	u32 act_info, dsp_info, dsp_st, dsp_stx, dsp_sty;
	struct vop2 *vop2 = cstate->private;
	int src_w = cstate->src_w;
	int src_h = cstate->src_h;
	int crtc_x = cstate->crtc_x;
	int crtc_y = cstate->crtc_y;
	int crtc_w = cstate->crtc_w;
	int crtc_h = cstate->crtc_h;
	int xvir = cstate->xvir;
	int y_mirror = 0;
	int csc_mode;
	u32 win_offset;
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id);
	u8 primary_plane_id = vop2->vp_plane_mask[cstate->crtc_id].primary_plane_id;
	char plane_name[10] = {0};

	win_offset = vop2->data->win_data[primary_plane_id].reg_offset;
	if (crtc_w > cstate->max_output.width) {
		printf("ERROR: output w[%d] exceeded max width[%d]\n",
		       crtc_w, cstate->max_output.width);
		return -EINVAL;
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

	vop2_setup_scale(vop2, win_offset, src_w, src_h, crtc_w, crtc_h);

	if (y_mirror)
		cstate->dma_addr += (src_h - 1) * xvir * 4;
	vop2_mask_write(vop2, RK3568_ESMART0_CTRL1 + win_offset, EN_MASK,
			YMIRROR_EN_SHIFT, y_mirror, false);

	vop2_mask_write(vop2, RK3568_ESMART0_REGION0_CTRL + win_offset,
			WIN_FORMAT_MASK, WIN_FORMAT_SHIFT, cstate->format,
			false);
	vop2_writel(vop2, RK3568_ESMART0_REGION0_VIR + win_offset, xvir);
	vop2_writel(vop2, RK3568_ESMART0_REGION0_YRGB_MST + win_offset,
		    cstate->dma_addr);

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

	printf("VOP VP%d enable %s[%dx%d->%dx%d@%dx%d] fmt[%d] addr[0x%x]\n",
		cstate->crtc_id, get_plane_name(primary_plane_id, plane_name),
		src_w, src_h, crtc_w, crtc_h, crtc_x, crtc_y, cstate->format,
		cstate->dma_addr);

	return 0;
}

static int rockchip_vop2_prepare(struct display_state *state)
{
	return 0;
}

static int rockchip_vop2_enable(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			STANDBY_EN_SHIFT, 0, false);
	vop2_writel(vop2, RK3568_REG_CFG_DONE, cfg_done);

	return 0;
}

static int rockchip_vop2_disable(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct vop2 *vop2 = cstate->private;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			STANDBY_EN_SHIFT, 1, false);
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
		.win_sel_port_offset = 0,
		.layer_sel_win_id = 0,
		.reg_offset = 0,
	},

	{
		.name = "Cluster1",
		.phys_id = ROCKCHIP_VOP2_CLUSTER1,
		.win_sel_port_offset = 1,
		.layer_sel_win_id = 1,
		.reg_offset = 0x200,
	},

	{
		.name = "Esmart0",
		.phys_id = ROCKCHIP_VOP2_ESMART0,
		.win_sel_port_offset = 4,
		.layer_sel_win_id = 2,
		.reg_offset = 0,
	},

	{
		.name = "Esmart1",
		.phys_id = ROCKCHIP_VOP2_ESMART1,
		.win_sel_port_offset = 5,
		.layer_sel_win_id = 6,
		.reg_offset = 0x200,
	},

	{
		.name = "Smart0",
		.phys_id = ROCKCHIP_VOP2_SMART0,
		.win_sel_port_offset = 6,
		.layer_sel_win_id = 3,
		.reg_offset = 0x400,
	},

	{
		.name = "Smart1",
		.phys_id = ROCKCHIP_VOP2_SMART1,
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
	.nr_vps = 3,
	.vp_data = rk3568_vp_data,
	.win_data = rk3568_win_data,
	.plane_mask = rk356x_vp_plane_mask[0],
	.plane_table = rk356x_plane_table,
	.nr_layers = 6,
	.nr_mixers = 5,
	.nr_gammas = 1,
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
