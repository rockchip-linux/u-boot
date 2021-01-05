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
#include <asm/unaligned.h>
#include <asm/io.h>
#include <linux/list.h>
#include <linux/media-bus-format.h>
#include <clk.h>
#include <asm/arch/clock.h>
#include <linux/err.h>
#include <dm/device.h>
#include <dm/read.h>
#include <syscon.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"

/* System registers definition */
#define RK3568_REG_CFG_DONE			0x000
#define	CFG_DONE_EN				BIT(15)

#define RK3568_VERSION_INFO			0x004

#define EN_MASK					1

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
#define LVDS0_MUX_SHIFT				5
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

/* Video Port registers definition */
#define RK3568_VP0_DSP_CTRL				0xC00
#define OUT_MODE_MASK					0xf
#define OUT_MODE_SHIFT					0
#define DATA_SWAP_MASK					0x1f
#define DATA_SWAP_SHIFT					8
#define DSP_RB_SWAP					2
#define CORE_DCLK_DIV_EN_SHIFT				4
#define P2I_EN_SHIFT					5
#define INTERLACE_EN_SHIFT				7
#define POST_DSP_OUT_R2Y_SHIFT				15
#define PRE_DITHER_DOWN_EN_SHIFT			16
#define DITHER_DOWN_EN_SHIFT				17
#define STANDBY_EN_SHIFT				31

#define RK3568_VP0_MIPI_CTRL				0xC04
#define DCLK_DIV2_SHIFT					4
#define DCLK_DIV2_MASK					0x3
#define MIPI_DUAL_EN_SHIFT				20
#define MIPI_DUAL_SWAP_EN_SHIFT				21

#define RK3568_VP0_COLOR_BAR_CTRL			0xC08
#define RK3568_VP0_DSP_BG				0xC2C
#define RK3568_VP0_PRE_SCAN_HTIMING			0xC30
#define RK3568_VP0_POST_DSP_HACT_INFO			0xC34
#define RK3568_VP0_POST_DSP_VACT_INFO			0xC38
#define RK3568_VP0_POST_SCL_FACTOR_YRGB			0xC3C
#define RK3568_VP0_POST_SCL_CTRL			0xC40
#define RK3568_VP0_POST_DSP_VACT_INFO_F1		0xC44
#define RK3568_VP0_DSP_HTOTAL_HS_END			0xC48
#define RK3568_VP0_DSP_HACT_ST_END			0xC4C
#define RK3568_VP0_DSP_VTOTAL_VS_END			0xC50
#define RK3568_VP0_DSP_VACT_ST_END			0xC54
#define RK3568_VP0_DSP_VS_ST_END_F1			0xC58
#define RK3568_VP0_DSP_VACT_ST_END_F1			0xC5C

#define RK3568_VP1_DSP_CTRL				0xD00
#define RK3568_VP1_MIPI_CTRL				0xD04
#define RK3568_VP1_COLOR_BAR_CTRL			0xD08
#define RK3568_VP1_PRE_SCAN_HTIMING			0xD30
#define RK3568_VP1_POST_DSP_HACT_INFO			0xD34
#define RK3568_VP1_POST_DSP_VACT_INFO			0xD38
#define RK3568_VP1_POST_SCL_FACTOR_YRGB			0xD3C
#define RK3568_VP1_POST_SCL_CTRL			0xD40
#define RK3568_VP1_DSP_HACT_INFO			0xD34
#define RK3568_VP1_DSP_VACT_INFO			0xD38
#define RK3568_VP1_POST_DSP_VACT_INFO_F1		0xD44
#define RK3568_VP1_DSP_HTOTAL_HS_END			0xD48
#define RK3568_VP1_DSP_HACT_ST_END			0xD4C
#define RK3568_VP1_DSP_VTOTAL_VS_END			0xD50
#define RK3568_VP1_DSP_VACT_ST_END			0xD54
#define RK3568_VP1_DSP_VS_ST_END_F1			0xD58
#define RK3568_VP1_DSP_VACT_ST_END_F1			0xD5C

#define RK3568_VP2_DSP_CTRL				0xE00
#define RK3568_VP2_MIPI_CTRL				0xE04
#define RK3568_VP2_COLOR_BAR_CTRL			0xE08
#define RK3568_VP2_PRE_SCAN_HTIMING			0xE30
#define RK3568_VP2_POST_DSP_HACT_INFO			0xE34
#define RK3568_VP2_POST_DSP_VACT_INFO			0xE38
#define RK3568_VP2_POST_SCL_FACTOR_YRGB			0xE3C
#define RK3568_VP2_POST_SCL_CTRL			0xE40
#define RK3568_VP2_DSP_HACT_INFO			0xE34
#define RK3568_VP2_DSP_VACT_INFO			0xE38
#define RK3568_VP2_POST_DSP_VACT_INFO_F1		0xE44
#define RK3568_VP2_DSP_HTOTAL_HS_END			0xE48
#define RK3568_VP2_DSP_HACT_ST_END			0xE4C
#define RK3568_VP2_DSP_VTOTAL_VS_END			0xE50
#define RK3568_VP2_DSP_VACT_ST_END			0xE54
#define RK3568_VP2_DSP_VS_ST_END_F1			0xE58
#define RK3568_VP2_DSP_VACT_ST_END_F1			0xE5C

/* Overlay registers definition    */
#define RK3568_OVL_CTRL				0x600
#define OVL_PORT_MUX_REG_DONE_IMD_SHIFT		28
#define RK3568_OVL_LAYER_SEL			0x604
#define LAYER_SEL_MASK				0xf

#define RK3568_OVL_PORT_SEL			0x608
#define PORT_MUX_MASK				0xf
#define PORT_MUX_SHIFT				0
#define LAYER_SEL_PORT_MASK			0x3
#define LAYER_SEL_PORT_SHIFT			24

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
#define RK3568_VP1_BG_MIX_CTRL			0x6E4
#define RK3568_VP2_BG_MIX_CTRL			0x6E8
#define RK3568_CLUSTER_DLY_NUM			0x6F0
#define RK3568_SMART_DLY_NUM			0x6F8

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
#define RK3568_ESMART0_REGION0_SCL_FACTOR_YRGB	0x1834
#define RK3568_ESMART0_REGION0_SCL_FACTOR_CBR	0x1838
#define RK3568_ESMART0_REGION0_SCL_OFFSET	0x183C
#define RK3568_ESMART0_REGION1_CTRL		0x1840
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
#define VOP2_MAX_VP				4

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

struct vop2_layer {
	uint8_t id;
	/**
	 * @win_phys_id: window id of the layer selected.
	 * Every layer must make sure to select different
	 * windows of others.
	 */
	uint8_t win_phys_id;
};

struct vop2_win {
	uint8_t id;
	uint8_t layer_id;
	uint8_t phys_id;
};

struct vop2_data {
	u32 version;
	struct vop_rect max_output[VOP2_MAX_VP];
	/**
	 * win_id: id of window attach to VP0,VP1,VP2,VP3,
	 * Only support one window for one VP in u-boot.
	 *
	 */
	uint8_t win_id[VOP2_MAX_VP];
	uint8_t nr_vps;
	uint8_t nr_layers;
	/**
	 * win_sel_id: from register LAYER_SEL
	 *
	 */
	uint8_t win_sel_id[VOP2_LAYER_MAX];
};

struct vop2 {
	u32 *regsbak;
	void *regs;
	void *grf;
	u32 reg_len;
	u32 version;
	const struct vop2_data *data;
	/**
	 * @nr_wins: active wins attached to the video port
	 */
	uint8_t nr_wins[VOP2_VP_MAX];
	struct vop2_win win[VOP2_LAYER_MAX];
	struct vop2_layer layer[VOP2_LAYER_MAX];
};

static struct vop2 *rockchip_vop2;
static void vop2_setup_win_for_vp(struct display_state *state);

static inline uint16_t scl_cal_scale(int src, int dst, int shift)
{
	return ((src * 2 - 3) << (shift - 1)) / (dst - 1);
}

static inline uint16_t scl_cal_scale2(int src, int dst)
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

static __maybe_unused bool is_uv_swap(u32 bus_format, u32 output_mode)
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

static int rockchip_vop2_init_gamma(struct vop2 *vop2,
				    struct display_state *state)
{
	return 0;
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
	u16 nr_mixers = 5, used_layer = 2, pre_scan_max_dly = 40;
	u32 bg_ovl_dly, bg_dly, pre_scan_dly;
	u16 hsync_len = mode->crtc_hsync_end - mode->crtc_hsync_start;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
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

	bg_ovl_dly = (nr_mixers - used_layer) << 1;
	bg_dly = pre_scan_max_dly - bg_ovl_dly;
	pre_scan_dly = bg_dly + (hdisplay >> 1) - 1;
	pre_scan_dly = (pre_scan_dly << 16) | hsync_len;
	vop2_writel(vop2, RK3568_VP0_PRE_SCAN_HTIMING + vp_offset, pre_scan_dly);
}

static void vop2_layer_map_initial(struct vop2 *vop2)
{
	struct vop2_layer *layer;
	struct vop2_win *win;
	u32 layer_map, sel;
	int i, j;

	layer_map = vop2_readl(vop2, RK3568_OVL_LAYER_SEL);

	for (i = 0; i < vop2->data->nr_layers; i++) {
		sel = (layer_map >> (4 * i)) & 0xf;
		layer = &vop2->layer[i];
		win = NULL;
		for (j = 0; j < vop2->data->nr_layers; j++) {
			if (sel == vop2->data->win_sel_id[j]) {
				win = &vop2->win[j];
				break;
			}
		}

		if (!win) {
			printf("invalid layer map :0x%x\n", layer_map);
			return;
		}

		layer->win_phys_id = j;
		win->layer_id = i;
		debug("layer%d select %d\n", i, j);
	}
}

static int vop2_initial(struct vop2 *vop2, struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	char dclk_name[9];
	struct clk dclk;
	uint8_t shift = 0;
	int i, ret;

	/* Process 'assigned-{clocks/clock-parents/clock-rates}' properties */
	ret = clk_set_defaults(cstate->dev);
	if (ret)
		debug("%s clk_set_defaults failed %d\n", __func__, ret);
	snprintf(dclk_name, sizeof(dclk_name), "dclk_vp%d", cstate->crtc_id);
	ret = clk_get_by_name(cstate->dev, dclk_name, &dclk);
	if (!ret)
		ret = clk_set_rate(&dclk, mode->clock * 1000);

	if (IS_ERR_VALUE(ret)) {
		printf("%s: Failed to set dclk: ret=%d\n", __func__, ret);
		return ret;
	}

	memcpy(vop2->regsbak, vop2->regs, vop2->reg_len);

	rockchip_vop2_init_gamma(vop2, state);
	vop2_mask_write(vop2, RK3568_OVL_CTRL, EN_MASK,
			OVL_PORT_MUX_REG_DONE_IMD_SHIFT, 1, false);
	vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
			IF_CTRL_REG_DONE_IMD_SHIFT, 1, false);
	for (i = 0; i < vop2->data->nr_vps - 1; i++) {
		shift = i * 4;
		vop2_mask_write(vop2, RK3568_OVL_PORT_SEL, PORT_MUX_MASK, PORT_MUX_SHIFT + shift, 8, false);	//todo
	}

	vop2_layer_map_initial(vop2);

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
		rockchip_vop2 = malloc(sizeof(struct vop2));
		if (!rockchip_vop2)
			return -ENOMEM;
		rockchip_vop2->regs = dev_read_addr_ptr(cstate->dev);
		rockchip_vop2->regsbak = malloc(RK3568_MAX_REG);
		rockchip_vop2->reg_len = RK3568_MAX_REG;
		rockchip_vop2->grf =
		    syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
		if (rockchip_vop2->grf <= 0)
			printf("%s: Get syscon grf failed (ret=%p)\n", __func__,
			       rockchip_vop2->grf);

		rockchip_vop2->version = vop2_data->version;
		rockchip_vop2->data = vop2_data;
	}

	cstate->private = rockchip_vop2;
	cstate->max_output = vop2_data->max_output[cstate->crtc_id];

	return 0;
}

static int rockchip_vop2_init(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	//const struct rockchip_crtc *crtc = cstate->crtc;
	//const struct vop2_data *vop2_data = crtc->data;
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
	//bool yuv_overlay = false, post_r2y_en = false, post_y2r_en = false;
	u32 vp_offset = (cstate->crtc_id * 0x100);
	//struct clk dclk;
	//fdt_size_t len;
	u32 val;
	//int ret;
	bool dclk_inv;
	uint8_t dither_down_en = 0;
	uint8_t pre_dither_down_en = 0;
	//uint8_t dither_down_mode = RGB888_TO_RGB666;

	vop2_initial(vop2, state);
	dclk_inv = (mode->flags & DRM_MODE_FLAG_PPIXDATA) ? 0 : 1;
	val = (mode->flags & DRM_MODE_FLAG_NHSYNC) ? 0 : BIT(HSYNC_POSITIVE);
	val |= (mode->flags & DRM_MODE_FLAG_NVSYNC) ? 0 : BIT(VSYNC_POSITIVE);

	if (conn_state->output_if & VOP_OUTPUT_IF_RGB) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, RGB_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				RGB_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_RGB_LVDS_DCLK_POL_SHIT, !!dclk_inv,
				false);
		vop2_grf_writel(vop2, RK3568_GRF_VO_CON1, EN_MASK,
				GRF_RGB_DCLK_INV_SHIFT, !dclk_inv);
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
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_LVDS1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, LVDS1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				LVDS1_MUX_SHIFT, cstate->crtc_id, false);
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
				IF_CRTL_MIPI_DCLK_POL_SHIT, ! !dclk_inv, false);
	}

	if (conn_state->output_if & VOP_OUTPUT_IF_MIPI1) {
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, EN_MASK, MIPI1_EN_SHIFT,
				1, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_EN, IF_MUX_MASK,
				MIPI1_MUX_SHIFT, cstate->crtc_id, false);
		vop2_mask_write(vop2, RK3568_DSP_IF_POL, EN_MASK,
				IF_CRTL_MIPI_DCLK_POL_SHIT, ! !dclk_inv, false);
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
	val = ! !(mode->flags & DRM_MODE_FLAG_DBLCLK);
	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			CORE_DCLK_DIV_EN_SHIFT, val, false);

	if (conn_state->output_mode == ROCKCHIP_OUT_MODE_YUV420)
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset, DCLK_DIV2_MASK,
				DCLK_DIV2_SHIFT, 0x3, false);
	else
		vop2_mask_write(vop2, RK3568_VP0_MIPI_CTRL + vp_offset, DCLK_DIV2_MASK,
				DCLK_DIV2_SHIFT, 0, false);

	if (yuv_overlay)
		val = 0x20010200;
	else
		val = 0;
	vop2_writel(vop2, RK3568_VP0_DSP_BG + vp_offset, val);

	vop2_mask_write(vop2, RK3568_VP0_DSP_CTRL + vp_offset, EN_MASK,
			POST_DSP_OUT_R2Y_SHIFT, yuv_overlay, false);

	vop2_post_config(state, vop2);
	vop2_setup_win_for_vp(state);

	return 0;
}

static void vop2_setup_win_for_vp(struct display_state *state)
{
	struct crtc_state *cstate = &state->crtc_state;
	const struct rockchip_crtc *crtc = cstate->crtc;
	const struct vop2_data *vop2_data = crtc->data;
	struct vop2 *vop2 = cstate->private;
	u8 port_id = cstate->crtc_id;
	struct vop2_win *win;
	struct vop2_layer *layer;
	u8 used_layers = 0;
	u8 layer_id, win_phys_id, win_sel_id;
	u8 shift;
	int i;

	for (i = 0; i < port_id; i++)
		used_layers += vop2->nr_wins[i];

	vop2->nr_wins[port_id]++;
	/*
	 * Win and layer must map one by one, if a win is selected
	 * by two layers, unexpected error may happen.
	 * So when we attach a new win to a layer, we also move the
	 * old win of the layer to the layer where the new win comes from.
	 *
	 */
	layer = &vop2->layer[used_layers];
	win = &vop2->win[port_id];
	shift = port_id * 2;
	vop2_mask_write(vop2, RK3568_OVL_PORT_SEL, LAYER_SEL_PORT_MASK,
			LAYER_SEL_PORT_SHIFT + shift, port_id, false);
	shift = used_layers * 4;
	win_phys_id = vop2->data->win_id[port_id];
	win_sel_id = vop2->data->win_sel_id[win_phys_id];
	vop2_mask_write(vop2, RK3568_OVL_LAYER_SEL, LAYER_SEL_PORT_MASK, shift,
			win_sel_id, false);
	layer_id = win->layer_id;
	win_phys_id = layer->win_phys_id;
	win->layer_id = layer->id;
	layer->win_phys_id = win->phys_id;
	layer = &vop2->layer[layer_id];
	win = &vop2->win[win_phys_id];
	shift = layer_id * 4;
	win_sel_id = vop2->data->win_sel_id[win_phys_id];
	vop2_mask_write(vop2, RK3568_OVL_LAYER_SEL, LAYER_SEL_PORT_MASK, shift,
			win_sel_id, false);
	win->layer_id = layer_id;
	layer->win_phys_id = win_phys_id;

	if (port_id == (vop2_data->nr_vps - 1))
		used_layers = vop2_data->nr_layers;
	shift = port_id * 4;
	vop2_mask_write(vop2, RK3568_OVL_PORT_SEL, PORT_MUX_MASK, shift,
			used_layers, false);

	if (port_id == 0) {
		vop2_writel(vop2, 0x604, 0x54760312);
		vop2_writel(vop2, 0x608, 0x84000781);
		vop2_writel(vop2, 0x6e0, 0x22000000);
	} else {
		vop2_writel(vop2, 0x604, 0x54720316);
		vop2_writel(vop2, 0x608, 0x84000708);
		vop2_writel(vop2, 0x6e4, 0x1e000000);
	}
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
	u32 win_offset = cstate->crtc_id * 0x200;
	u32 cfg_done = CFG_DONE_EN | BIT(cstate->crtc_id);

	if (crtc_w > cstate->max_output.width) {
		printf("ERROR: output w[%d] exceeded max width[%d]\n",
		       crtc_w, cstate->max_output.width);
		return -EINVAL;
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

const struct vop2_data rk3568_vop = {
	.nr_vps = 3,
	.max_output = {
		       [VOP2_VP0] = {4096, 2304},
		       [VOP2_VP1] = {2048, 1536},
		       [VOP2_VP2] = {1920, 1080},
		       },

	/*
	 * Cluster0-Win0: 0
	 * Cluster1-Win0: 1
	 * Esmart0-Win0:  2
	 * Esmart1-Win0:  3
	 * Smart0-Win0:   4
	 * Smart1-Win0:   5
	 */
	.win_id = {
		   [VOP2_VP0] = 2,
		   [VOP2_VP1] = 3,
		   [VOP2_VP2] = 4,
		   },

	/**
	 * Win select id: from register LAYER_SEL
	 *
	 * Cluster0-Win0: 0
	 * Cluster1-Win0: 1
	 * Esmart0-Win0:  2
	 * Esmart1-Win0:  6
	 * Smart0-Win0:   3
	 * Smart1-Win0:   7
	 */
	.win_sel_id = {0, 1, 2, 6, 3, 7},
	.nr_layers = 6,
};

const struct rockchip_crtc_funcs rockchip_vop2_funcs = {
	.preinit = rockchip_vop2_preinit,
	.prepare = rockchip_vop2_prepare,
	.init = rockchip_vop2_init,
	.set_plane = rockchip_vop2_set_plane,
	.enable = rockchip_vop2_enable,
	.disable = rockchip_vop2_disable,
};
