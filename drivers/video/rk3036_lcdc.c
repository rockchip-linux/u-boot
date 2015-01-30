/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <lcd.h>
#include <asm/arch/rkplat.h>
#include "rockchip_fb.h"

DECLARE_GLOBAL_DATA_PTR;
#define COMPAT_RK312X_LCDC	"rockchip,rk312x-lcdc"
#define COMPAT_RK3036_LCDC	"rockchip,rk3036-lcdc"

#define BITS(x, bit)            	((x) << (bit))
#define BITS_MASK(x, mask, bit) 	BITS((x) & (mask), bit)
#define BITS_EN(mask, bit)       	BITS(mask, bit + 16)

/*******************register definition**********************/

#define SYS_CTRL                (0x00)
#define m_WIN0_EN               BITS(1, 0)
#define m_WIN1_EN		BITS(1, 1)
#define m_HWC_EN		BITS(1, 2)
#define m_WIN0_FORMAT		BITS(7, 3)
#define m_WIN1_FORMAT		BITS(7, 6)
#define m_HWC_LUT_EN		BITS(1, 9)
#define m_HWC_SIZE		BITS(1, 10)
#define m_DIRECT_PATH_EN        BITS(1, 11)      /* rk312x */
#define m_DIRECT_PATH_LAYER     BITS(1, 12)      /* rk312x */
#define m_TVE_MODE_SEL          BITS(1, 13)      /* rk312x */
#define m_TVE_DAC_EN            BITS(1, 14)      /* rk312x */
#define m_WIN0_RB_SWAP		BITS(1, 15)
#define m_WIN0_ALPHA_SWAP	BITS(1, 16)
#define m_WIN0_Y8_SWAP		BITS(1, 17)
#define m_WIN0_UV_SWAP		BITS(1, 18)
#define m_WIN1_RB_SWAP		BITS(1, 19)
#define m_WIN1_ALPHA_SWAP	BITS(1, 20)
#define m_WIN1_ENDIAN_SWAP      BITS(1, 21)      /* rk312x */
#define m_WIN0_OTSD_DISABLE	BITS(1, 22)
#define m_WIN1_OTSD_DISABLE	BITS(1, 23)
#define m_DMA_BURST_LENGTH	BITS(3, 24)
#define m_HWC_LODAD_EN		BITS(1, 26)
#define m_WIN1_LUT_EN           BITS(1, 27)      /* rk312x */
#define m_DSP_LUT_EN            BITS(1, 28)      /* rk312x */
#define m_DMA_STOP		BITS(1, 29)
#define m_LCDC_STANDBY		BITS(1, 30)
#define m_AUTO_GATING_EN	BITS(1, 31)
	
#define v_WIN0_EN(x)		BITS_MASK(x, 1, 0)
#define v_WIN1_EN(x)		BITS_MASK(x, 1, 1)
#define v_HWC_EN(x)		BITS_MASK(x, 1, 2)
#define v_WIN0_FORMAT(x)	BITS_MASK(x, 7, 3)
#define v_WIN1_FORMAT(x)	BITS_MASK(x, 7, 6)
#define v_HWC_LUT_EN(x)		BITS_MASK(x, 1, 9)
#define v_HWC_SIZE(x)		BITS_MASK(x, 1, 10)
#define v_DIRECT_PATH_EN(x)     BITS_MASK(x, 1, 11)
#define v_DIRECT_PATH_LAYER(x)  BITS_MASK(x, 1, 12)
#define v_TVE_MODE_SEL(x)       BITS_MASK(x, 1, 13)
#define v_TVE_DAC_EN(x)         BITS_MASK(x, 1, 14)
#define v_WIN0_RB_SWAP(x)	BITS_MASK(x, 1, 15)
#define v_WIN0_ALPHA_SWAP(x)	BITS_MASK(x, 1, 16)
#define v_WIN0_Y8_SWAP(x)	BITS_MASK(x, 1, 17)
#define v_WIN0_UV_SWAP(x)	BITS_MASK(x, 1, 18)
#define v_WIN1_RB_SWAP(x)	BITS_MASK(x, 1, 19)
#define v_WIN1_ALPHA_SWAP(x)	BITS_MASK(x, 1, 20)
#define v_WIN1_ENDIAN_SWAP(x)   BITS_MASK(x, 1, 21)
#define v_WIN0_OTSD_DISABLE(x)	BITS_MASK(x, 1, 22)
#define v_WIN1_OTSD_DISABLE(x)	BITS_MASK(x, 1, 23)
#define v_DMA_BURST_LENGTH(x)	BITS_MASK(x, 3, 24)
#define v_HWC_LODAD_EN(x)	BITS_MASK(x, 1, 26)
#define v_WIN1_LUT_EN(x)	BITS_MASK(x, 1, 27)
#define v_DSP_LUT_EN(x)         BITS_MASK(x, 1, 28)
#define v_DMA_STOP(x)		BITS_MASK(x, 1, 29)
#define v_LCDC_STANDBY(x)	BITS_MASK(x, 1, 30)
#define v_AUTO_GATING_EN(x)	BITS_MASK(x, 1, 31)

#define DSP_CTRL0		(0x04)
#define m_DSP_OUT_FORMAT	BITS(0x0f, 0)
#define m_HSYNC_POL		BITS(1, 4)
#define m_VSYNC_POL		BITS(1, 5)
#define m_DEN_POL		BITS(1, 6)
#define m_DCLK_POL		BITS(1, 7)
#define m_WIN0_TOP		BITS(1, 8)
#define m_DITHER_UP_EN		BITS(1, 9)
#define m_DITHER_DOWN_MODE	BITS(1, 10)	/* use for rk312x */
#define m_DITHER_DOWN_EN	BITS(1, 11)	/* use for rk312x */
#define m_INTERLACE_DSP_EN	BITS(1, 12)
#define m_INTERLACE_FIELD_POL	BITS(1, 13)	/* use for rk312x */
#define m_WIN0_INTERLACE_EN	BITS(1, 14)	/* use for rk312x */
#define m_WIN1_INTERLACE_EN	BITS(1, 15)
#define m_WIN0_YRGB_DEFLICK_EN	BITS(1, 16)
#define m_WIN0_CBR_DEFLICK_EN	BITS(1, 17)
#define m_WIN0_ALPHA_MODE	BITS(1, 18)
#define m_WIN1_ALPHA_MODE	BITS(1, 19)
#define m_WIN0_CSC_MODE		BITS(3, 20)
#define m_WIN1_CSC_MODE		BITS(1, 22)
#define m_WIN0_YUV_CLIP		BITS(1, 23)
#define m_TVE_MODE		BITS(1, 25)
#define m_SW_UV_OFFSET_EN	BITS(1, 26)	/* use for rk312x */
#define m_DITHER_DOWN_SEL	BITS(1, 27)	/* use for rk312x */
#define m_HWC_ALPHA_MODE	BITS(1, 28)
#define m_ALPHA_MODE_SEL0       BITS(1, 29)
#define m_ALPHA_MODE_SEL1	BITS(1, 30)
#define m_WIN1_DIFF_DCLK_EN	BITS(1, 31)	/* use for rk3036 */
#define m_SW_OVERLAY_MODE	BITS(1, 31)	/* use for rk312x */

#define v_DSP_OUT_FORMAT(x)	BITS_MASK(x, 0x0f, 0)
#define v_HSYNC_POL(x)		BITS_MASK(x, 1, 4)
#define v_VSYNC_POL(x)		BITS_MASK(x, 1, 5)
#define v_DEN_POL(x)		BITS_MASK(x, 1, 6)
#define v_DCLK_POL(x)		BITS_MASK(x, 1, 7)
#define v_WIN0_TOP(x)		BITS_MASK(x, 1, 8)
#define v_DITHER_UP_EN(x)	BITS_MASK(x, 1, 9)
#define v_DITHER_DOWN_MODE(x)	BITS_MASK(x, 1, 10)	/* rk312x */
#define v_DITHER_DOWN_EN(x)	BITS_MASK(x, 1, 11)	/* rk312x */
#define v_INTERLACE_DSP_EN(x)	BITS_MASK(x, 1, 12)
#define v_INTERLACE_FIELD_POL(x)	BITS_MASK(x, 1, 13)	/* rk312x */
#define v_WIN0_INTERLACE_EN(x)		BITS_MASK(x, 1, 14)	/* rk312x */
#define v_WIN1_INTERLACE_EN(x)		BITS_MASK(x, 1, 15)
#define v_WIN0_YRGB_DEFLICK_EN(x)	BITS_MASK(x, 1, 16)
#define v_WIN0_CBR_DEFLICK_EN(x)	BITS_MASK(x, 1, 17)
#define v_WIN0_ALPHA_MODE(x)		BITS_MASK(x, 1, 18)
#define v_WIN1_ALPHA_MODE(x)		BITS_MASK(x, 1, 19)
#define v_WIN0_CSC_MODE(x)		BITS_MASK(x, 3, 20)
#define v_WIN1_CSC_MODE(x)		BITS_MASK(x, 1, 22)
#define v_WIN0_YUV_CLIP(x)		BITS_MASK(x, 1, 23)
#define v_TVE_MODE(x)			BITS_MASK(x, 1, 25)
#define v_SW_UV_OFFSET_EN(x)		BITS_MASK(x, 1, 26)      /* rk312x */
#define v_DITHER_DOWN_SEL(x)		BITS_MASK(x, 1, 27)      /* rk312x */
#define v_HWC_ALPHA_MODE(x)		BITS_MASK(x, 1, 28)
#define v_ALPHA_MODE_SEL0(x)            BITS_MASK(x, 1, 29)
#define v_ALPHA_MODE_SEL1(x)		BITS_MASK(x, 1, 30)
#define v_WIN1_DIFF_DCLK_EN(x)		BITS_MASK(x, 1, 31)	/* rk3036 */
#define v_SW_OVERLAY_MODE(x)		BITS_MASK(x, 1, 31)	/* rk312x */

#define DSP_CTRL1		(0x08)
#define m_BG_COLOR		BITS(0xffffff, 0)
#define m_BG_B			BITS(0xff, 0)
#define m_BG_G			BITS(0xff, 8)
#define m_BG_R			BITS(0xff, 16)
#define m_BLANK_EN		BITS(1, 24)
#define m_BLACK_EN		BITS(1, 25)
#define m_DSP_BG_SWAP		BITS(1, 26)
#define m_DSP_RB_SWAP		BITS(1, 27)
#define m_DSP_RG_SWAP		BITS(1, 28)
#define m_DSP_DELTA_SWAP	BITS(1, 29)              /* rk3036 */
#define m_DSP_DUMMY_SWAP	BITS(1, 30)	        /* rk3036 */
#define m_DSP_OUT_ZERO		BITS(1, 31)

#define v_BG_COLOR(x)		BITS_MASK(x, 0xffffff, 0)
#define v_BG_B(x)		BITS_MASK(x, 0xff, 0)
#define v_BG_G(x)		BITS_MASK(x, 0xff, 8)
#define v_BG_R(x)		BITS_MASK(x, 0xff, 16)
#define v_BLANK_EN(x)		BITS_MASK(x, 1, 24)
#define v_BLACK_EN(x)		BITS_MASK(x, 1, 25)
#define v_DSP_BG_SWAP(x)	BITS_MASK(x, 1, 26)
#define v_DSP_RB_SWAP(x)	BITS_MASK(x, 1, 27)
#define v_DSP_RG_SWAP(x)	BITS_MASK(x, 1, 28)
#define v_DSP_DELTA_SWAP(x)	BITS_MASK(x, 1, 29)      /* rk3036 */
#define v_DSP_DUMMY_SWAP(x)	BITS_MASK(x, 1, 30)      /* rk3036 */
#define v_DSP_OUT_ZERO(x)	BITS_MASK(x, 1, 31)

#define INT_SCALER              (0x0c)          /* only use for rk312x */
#define m_SCALER_EMPTY_INTR_EN  BITS(1, 0)
#define m_SCLAER_EMPTY_INTR_CLR BITS(1, 1)
#define m_SCLAER_EMPTY_INTR_STA BITS(1, 2)
#define m_FS_MASK_EN            BITS(1, 3)
#define m_HDMI_HSYNC_POL        BITS(1, 4)
#define m_HDMI_VSYNC_POL        BITS(1, 5)
#define m_HDMI_DEN_POL          BITS(1, 6)

#define v_SCALER_EMPTY_INTR_EN(x)       BITS_MASK(x, 1, 0)
#define v_SCLAER_EMPTY_INTR_CLR(x)      BITS_MASK(x, 1, 1)
#define v_SCLAER_EMPTY_INTR_STA(x)      BITS_MASK(x, 1, 2)
#define v_FS_MASK_EN(x)                 BITS_MASK(x, 1, 3)
#define v_HDMI_HSYNC_POL(x)             BITS_MASK(x, 1, 4)
#define v_HDMI_VSYNC_POL(x)             BITS_MASK(x, 1, 5)
#define v_HDMI_DEN_POL(x)               BITS_MASK(x, 1, 6)

#define INT_STATUS		(0x10)
#define m_HS_INT_STA		BITS(1, 0)
#define m_FS_INT_STA		BITS(1, 1)
#define m_LF_INT_STA		BITS(1, 2)
#define m_BUS_ERR_INT_STA	BITS(1, 3)
#define m_HS_INT_EN		BITS(1, 4)
#define m_FS_INT_EN          	BITS(1, 5)
#define m_LF_INT_EN         	BITS(1, 6)
#define m_BUS_ERR_INT_EN	BITS(1, 7)
#define m_HS_INT_CLEAR		BITS(1, 8)
#define m_FS_INT_CLEAR		BITS(1, 9)
#define m_LF_INT_CLEAR		BITS(1, 10)
#define m_BUS_ERR_INT_CLEAR	BITS(1, 11)
#define m_LF_INT_NUM		BITS(0xfff, 12)
#define m_WIN0_EMPTY_INT_EN	BITS(1, 24)
#define m_WIN1_EMPTY_INT_EN	BITS(1, 25)
#define m_WIN0_EMPTY_INT_CLEAR	BITS(1, 26)
#define m_WIN1_EMPTY_INT_CLEAR	BITS(1, 27)
#define m_WIN0_EMPTY_INT_STA	BITS(1, 28)
#define m_WIN1_EMPTY_INT_STA	BITS(1, 29)
#define m_FS_RAW_STA		BITS(1, 30)
#define m_LF_RAW_STA		BITS(1, 31)
	
#define v_HS_INT_EN(x)		BITS_MASK(x, 1, 4)
#define v_FS_INT_EN(x)		BITS_MASK(x, 1, 5)
#define v_LF_INT_EN(x)		BITS_MASK(x, 1, 6)
#define v_BUS_ERR_INT_EN(x)	BITS_MASK(x, 1, 7)
#define v_HS_INT_CLEAR(x)	BITS_MASK(x, 1, 8)
#define v_FS_INT_CLEAR(x)	BITS_MASK(x, 1, 9)
#define v_LF_INT_CLEAR(x)	BITS_MASK(x, 1, 10)
#define v_BUS_ERR_INT_CLEAR(x)	BITS_MASK(x, 1, 11)
#define v_LF_INT_NUM(x)		BITS_MASK(x, 0xfff, 12)
#define v_WIN0_EMPTY_INT_EN(x)	BITS_MASK(x, 1, 24)
#define v_WIN1_EMPTY_INT_EN(x)	BITS_MASK(x, 1, 25)
#define v_WIN0_EMPTY_INT_CLEAR(x)	BITS_MASK(x, 1, 26)
#define v_WIN1_EMPTY_INT_CLEAR(x)	BITS_MASK(x, 1, 27)

#define ALPHA_CTRL		(0x14)
#define m_WIN0_ALPHA_EN		BITS(1, 0)
#define m_WIN1_ALPHA_EN		BITS(1, 1)
#define m_HWC_ALPAH_EN		BITS(1, 2)
#define m_WIN1_PREMUL_SCALE	BITS(1, 3)               /* rk3036 */
#define m_WIN0_ALPHA_VAL	BITS(0xff, 4)
#define m_WIN1_ALPHA_VAL	BITS(0xff, 12)
#define m_HWC_ALPAH_VAL		BITS(0xff, 20)

#define v_WIN0_ALPHA_EN(x)	BITS_MASK(x, 1, 0)
#define v_WIN1_ALPHA_EN(x)	BITS_MASK(x, 1, 1)
#define v_HWC_ALPAH_EN(x)	BITS_MASK(x, 1, 2)
#define v_WIN1_PREMUL_SCALE(x)	BITS_MASK(x, 1, 3)       /* rk3036 */
#define v_WIN0_ALPHA_VAL(x)	BITS_MASK(x, 0xff, 4)
#define v_WIN1_ALPHA_VAL(x)	BITS_MASK(x, 0xff, 12)
#define v_HWC_ALPAH_VAL(x)	BITS_MASK(x, 0xff, 20)

#define WIN0_COLOR_KEY		(0x18)
#define WIN1_COLOR_KEY		(0x1c)
#define m_COLOR_KEY_VAL		BITS(0xffffff, 0)
#define m_COLOR_KEY_EN		BITS(1, 24)

#define v_COLOR_KEY_VAL(x)	BITS_MASK(x, 0xffffff, 0)
#define v_COLOR_KEY_EN(x)	BITS_MASK(x, 1, 24)

/* Layer Registers */
#define WIN0_YRGB_MST		(0x20)
#define WIN0_CBR_MST		(0x24)
#define WIN1_MST		(0xa0)                  /* rk3036 */
#define WIN1_MST_RK312X         (0x4c)                  /* rk312x */
#define HWC_MST			(0x58)

#define WIN1_VIR		(0x28)
#define WIN0_VIR		(0x30)
#define m_YRGB_VIR	        BITS(0x1fff, 0)
#define m_CBBR_VIR	        BITS(0x1fff, 16)   

#define v_YRGB_VIR(x)           BITS_MASK(x, 0x1fff, 0)
#define v_CBBR_VIR(x)           BITS_MASK(x, 0x1fff, 16)

#define v_ARGB888_VIRWIDTH(x)	BITS_MASK(x, 0x1fff, 0)
#define v_RGB888_VIRWIDTH(x) 	BITS_MASK(((x*3)>>2)+((x)%3), 0x1fff, 0)
#define v_RGB565_VIRWIDTH(x)	BITS_MASK(DIV_ROUND_UP(x, 2), 0x1fff, 0)
#define v_YUV_VIRWIDTH(x)	BITS_MASK(DIV_ROUND_UP(x, 4), 0x1fff, 0)
#define v_CBCR_VIR(x)		BITS_MASK(x, 0x1fff, 16)

#define WIN0_ACT_INFO		(0x34)
#define WIN1_ACT_INFO		(0xb4)          /* rk3036 */
#define m_ACT_WIDTH       	BITS(0x1fff, 0)
#define m_ACT_HEIGHT      	BITS(0x1fff, 16)

#define v_ACT_WIDTH(x)       	BITS_MASK(x - 1, 0x1fff, 0)
#define v_ACT_HEIGHT(x)      	BITS_MASK(x - 1, 0x1fff, 16)

#define WIN0_DSP_INFO		(0x38)
#define WIN1_DSP_INFO		(0xb8)          /* rk3036 */
#define WIN1_DSP_INFO_RK312X    (0x50)          /* rk312x */
#define m_DSP_WIDTH       	BITS(0x7ff, 0)
#define m_DSP_HEIGHT      	BITS(0x7ff, 16)

#define v_DSP_WIDTH(x)     	BITS_MASK(x - 1, 0x7ff, 0)
#define v_DSP_HEIGHT(x)    	BITS_MASK(x - 1, 0x7ff, 16)
	
#define WIN0_DSP_ST		(0x3c)
#define WIN1_DSP_ST		(0xbc)          /* rk3036 */
#define WIN1_DSP_ST_RK312X      (0x54)          /* rk312x */
#define HWC_DSP_ST		(0x5c)
#define m_DSP_STX               BITS(0xfff, 0)
#define m_DSP_STY               BITS(0xfff, 16)

#define v_DSP_STX(x)      	BITS_MASK(x, 0xfff, 0)
#define v_DSP_STY(x)      	BITS_MASK(x, 0xfff, 16)
	
#define WIN0_SCL_FACTOR_YRGB	(0x40)
#define WIN0_SCL_FACTOR_CBR	(0x44)
#define WIN1_SCL_FACTOR_YRGB	(0xc0)          /* rk3036 */
#define m_X_SCL_FACTOR          BITS(0xffff, 0)
#define m_Y_SCL_FACTOR          BITS(0xffff, 16)

#define v_X_SCL_FACTOR(x)  	BITS_MASK(x, 0xffff, 0)
#define v_Y_SCL_FACTOR(x)  	BITS_MASK(x, 0xffff, 16)
	
#define WIN0_SCL_OFFSET		(0x48)
#define WIN1_SCL_OFFSET		(0xc8)          /* rk3036 */

/* LUT Registers */
#define WIN1_LUT_ADDR 		(0x0400)        /* rk3036 */
#define HWC_LUT_ADDR   		(0x0800)
#define DSP_LUT_ADDR            (0x0c00)        /* rk312x */

/* Display Infomation Registers */
#define DSP_HTOTAL_HS_END	(0x6c)
#define v_HSYNC(x)  		BITS_MASK(x, 0xfff, 0)   /* hsync pulse width */
#define v_HORPRD(x) 		BITS_MASK(x, 0xfff, 16)  /* horizontal period */

#define DSP_HACT_ST_END		(0x70)
#define v_HAEP(x) 		BITS_MASK(x, 0xfff, 0)  /* horizontal active end point */
#define v_HASP(x) 		BITS_MASK(x, 0xfff, 16) /* horizontal active start point */

#define DSP_VTOTAL_VS_END	(0x74)
#define v_VSYNC(x) 		BITS_MASK(x, 0xfff, 0)
#define v_VERPRD(x) 		BITS_MASK(x, 0xfff, 16)
	
#define DSP_VACT_ST_END		(0x78)
#define v_VAEP(x) 		BITS_MASK(x, 0xfff, 0)
#define v_VASP(x) 		BITS_MASK(x, 0xfff, 16)

#define DSP_VS_ST_END_F1	(0x7c)
#define v_VSYNC_END_F1(x) 	BITS_MASK(x, 0xfff, 0)
#define v_VSYNC_ST_F1(x) 	BITS_MASK(x, 0xfff, 16)
#define DSP_VACT_ST_END_F1	(0x80)
#define v_VAEP_F1(x) 		BITS_MASK(x, 0xfff, 0)
#define v_VASP_F1(x) 		BITS_MASK(x, 0xfff, 16)

/* Scaler Registers 
 * Only used for rk312x
 */
#define SCALER_CTRL             (0xa0)
#define m_SCALER_EN             BITS(1, 0)
#define m_SCALER_SYNC_INVERT    BITS(1, 2)
#define m_SCALER_DEN_INVERT     BITS(1, 3)
#define m_SCALER_OUT_ZERO       BITS(1, 4)
#define m_SCALER_OUT_EN         BITS(1, 5)
#define m_SCALER_VSYNC_MODE     BITS(3, 6)
#define m_SCALER_VSYNC_VST      BITS(0xff, 8)

#define v_SCALER_EN(x)          BITS_MASK(x, 1, 0)
#define v_SCALER_SYNC_INVERT(x) BITS_MASK(x, 1, 2)
#define v_SCALER_DEN_INVERT(x)  BITS_MASK(x, 1, 3)
#define v_SCALER_OUT_ZERO(x)    BITS_MASK(x, 1, 4)
#define v_SCALER_OUT_EN(x)      BITS_MASK(x, 1, 5)
#define v_SCALER_VSYNC_MODE(x)  BITS_MASK(x, 3, 6)
#define v_SCALER_VSYNC_VST(x)   BITS_MASK(x, 0xff, 8)

#define SCALER_FACTOR           (0xa4)
#define m_SCALER_H_FACTOR       BITS(0x3fff, 0)
#define m_SCALER_V_FACTOR       BITS(0x3fff, 16)

#define v_SCALER_H_FACTOR(x)    BITS_MASK(x, 0x3fff, 0)
#define v_SCALER_V_FACTOR(x)    BITS_MASK(x, 0x3fff, 16)

#define SCALER_FRAME_ST         (0xa8)
#define m_SCALER_FRAME_HST      BITS(0xfff, 0)
#define m_SCALER_FRAME_VST      BITS(0xfff, 16)

#define v_SCALER_FRAME_HST(x)   BITS_MASK(x, 0xfff, 0)
#define v_SCALER_FRAME_VST(x)   BITS_MASK(x, 0xfff, 16)

#define SCALER_DSP_HOR_TIMING   (0xac)
#define m_SCALER_HTOTAL         BITS(0xfff, 0)
#define m_SCALER_HS_END         BITS(0xff, 16)

#define v_SCALER_HTOTAL(x)      BITS_MASK(x, 0xfff, 0)
#define v_SCALER_HS_END(x)      BITS_MASK(x, 0xff, 16)

#define SCALER_DSP_HACT_ST_END  (0xb0)
#define m_SCALER_HAEP           BITS(0xfff, 0)
#define m_SCALER_HASP           BITS(0x3ff, 16)

#define v_SCALER_HAEP(x)        BITS_MASK(x, 0xfff, 0)
#define v_SCALER_HASP(x)        BITS_MASK(x, 0x3ff, 16)

#define SCALER_DSP_VER_TIMING   (0xb4)
#define m_SCALER_VTOTAL         BITS(0xfff, 0)
#define m_SCALER_VS_END         BITS(0xff, 16)

#define v_SCALER_VTOTAL(x)      BITS_MASK(x, 0xfff, 0)
#define v_SCALER_VS_END(x)      BITS_MASK(x, 0xff, 16)

#define SCALER_DSP_VACT_ST_END  (0xb8)
#define m_SCALER_VAEP           BITS(0xfff, 0)
#define m_SCALER_VASP           BITS(0xff, 16)

#define v_SCALER_VAEP(x)        BITS_MASK(x, 0xfff, 0)
#define v_SCALER_VASP(x)        BITS_MASK(x, 0xff, 16)

#define SCALER_DSP_HBOR_TIMING  (0xbc)
#define m_SCALER_HBOR_END       BITS(0xfff, 0)
#define m_SCALER_HBOR_ST        BITS(0x3ff, 16)

#define v_SCALER_HBOR_END(x)    BITS_MASK(x, 0xfff, 0)
#define v_SCALER_HBOR_ST(x)     BITS_MASK(x, 0x3ff, 16)

#define SCALER_DSP_VBOR_TIMING  (0xc0)
#define m_SCALER_VBOR_END       BITS(0xfff, 0)
#define m_SCALER_VBOR_ST        BITS(0xff, 16)

#define v_SCALER_VBOR_END(x)    BITS_MASK(x, 0xfff, 0)
#define v_SCALER_VBOR_ST(x)     BITS_MASK(x, 0xff, 16)        

/* BCSH Registers */
#define BCSH_CTRL		(0xd0)
#define m_BCSH_EN		BITS(1, 0)
#define m_BCSH_R2Y_CSC_MODE     BITS(1, 1)       /* rk312x */
#define m_BCSH_OUT_MODE		BITS(3, 2)
#define m_BCSH_Y2R_CSC_MODE     BITS(3, 4)
#define m_BCSH_Y2R_EN           BITS(1, 6)       /* rk312x */
#define m_BCSH_R2Y_EN           BITS(1, 7)       /* rk312x */

#define v_BCSH_EN(x)		BITS_MASK(x, 1, 0)
#define v_BCSH_R2Y_CSC_MODE(x)  BITS_MASK(x, 1, 1)       /* rk312x */
#define v_BCSH_OUT_MODE(x)	BITS_MASK(x, 3, 2)
#define v_BCSH_Y2R_CSC_MODE(x)	BITS_MASK(x, 3, 4)
#define v_BCSH_Y2R_EN(x)        BITS_MASK(x, 1, 6)       /* rk312x */
#define v_BCSH_R2Y_EN(x)        BITS_MASK(x, 1, 7)       /* rk312x */

#define BCSH_COLOR_BAR 		(0xd4)
#define m_BCSH_COLOR_BAR_Y      BITS(0xff, 0)
#define m_BCSH_COLOR_BAR_U	BITS(0xff, 8)
#define m_BCSH_COLOR_BAR_V	BITS(0xff, 16)

#define v_BCSH_COLOR_BAR_Y(x)	BITS_MASK(x, 0xff, 0)
#define v_BCSH_COLOR_BAR_U(x)   BITS_MASK(x, 0xff, 8)
#define v_BCSH_COLOR_BAR_V(x)   BITS_MASK(x, 0xff, 16)

#define BCSH_BCS 		(0xd8)	
#define m_BCSH_BRIGHTNESS	BITS(0x1f, 0)	
#define m_BCSH_CONTRAST		BITS(0xff, 8)
#define m_BCSH_SAT_CON		BITS(0x1ff, 16)

#define v_BCSH_BRIGHTNESS(x)	BITS_MASK(x, 0x1f, 0)	
#define v_BCSH_CONTRAST(x)	BITS_MASK(x, 0xff, 8)	
#define v_BCSH_SAT_CON(x)       BITS_MASK(x, 0x1ff, 16)			

#define BCSH_H 			(0xdc)	
#define m_BCSH_SIN_HUE		BITS(0xff, 0)
#define m_BCSH_COS_HUE		BITS(0xff, 8)

#define v_BCSH_SIN_HUE(x)	BITS_MASK(x, 0xff, 0)
#define v_BCSH_COS_HUE(x)	BITS_MASK(x, 0xff, 8)

#define FRC_LOWER01_0           (0xe0)
#define FRC_LOWER01_1           (0xe4)
#define FRC_LOWER10_0           (0xe8)
#define FRC_LOWER10_1           (0xec)
#define FRC_LOWER11_0           (0xf0)
#define FRC_LOWER11_1           (0xf4)

/* Bus Register */
#define AXI_BUS_CTRL		(0x2c)
#define m_IO_PAD_CLK			BITS(1, 31)
#define m_CORE_CLK_DIV_EN		BITS(1, 30)
#define m_MIPI_DCLK_INVERT              BITS(1, 29)      /* rk312x */
#define m_MIPI_DCLK_EN                  BITS(1, 28)      /* rk312x */
#define m_LVDS_DCLK_INVERT              BITS(1, 27)      /* rk312x */
#define m_LVDS_DCLK_EN                  BITS(1, 26)      /* rk312x */
#define m_RGB_DCLK_INVERT               BITS(1, 25)      /* rk312x */
#define m_RGB_DCLK_EN                   BITS(1, 24)      /* rk312x */
#define m_HDMI_DCLK_INVERT		BITS(1, 23)
#define m_HDMI_DCLK_EN			BITS(1, 22)
#define m_TVE_DAC_DCLK_INVERT		BITS(1, 21)
#define m_TVE_DAC_DCLK_EN		BITS(1, 20)
#define m_HDMI_DCLK_DIV_EN		BITS(1, 19)
#define m_AXI_OUTSTANDING_MAX_NUM	BITS(0x1f, 12)
#define m_AXI_MAX_OUTSTANDING_EN	BITS(1, 11)
#define m_MMU_EN			BITS(1, 10)
#define m_NOC_HURRY_THRESHOLD		BITS(0xf, 6)
#define m_NOC_HURRY_VALUE		BITS(3, 4)
#define m_NOC_HURRY_EN			BITS(1, 3)
#define m_NOC_QOS_VALUE			BITS(3, 1)
#define m_NOC_QOS_EN			BITS(1, 0)

#define v_IO_PAD_CLK(x)			BITS_MASK(x, 1, 31)
#define v_CORE_CLK_DIV_EN(x)		BITS_MASK(x, 1, 30)
#define v_MIPI_DCLK_INVERT(x)           BITS_MASK(x, 1, 29)
#define v_MIPI_DCLK_EN(x)               BITS_MASK(x, 1, 28)
#define v_LVDS_DCLK_INVERT(x)           BITS_MASK(x, 1, 27)
#define v_LVDS_DCLK_EN(x)               BITS_MASK(x, 1, 26)
#define v_RGB_DCLK_INVERT(x)            BITS_MASK(x, 1, 25)
#define v_RGB_DCLK_EN(x)                BITS_MASK(x, 1, 24)
#define v_HDMI_DCLK_INVERT(x)		BITS_MASK(x, 1, 23)
#define v_HDMI_DCLK_EN(x)		BITS_MASK(x, 1, 22)
#define v_TVE_DAC_DCLK_INVERT(x)	BITS_MASK(x, 1, 21)
#define v_TVE_DAC_DCLK_EN(x)		BITS_MASK(x, 1, 20)
#define v_HDMI_DCLK_DIV_EN(x)		BITS_MASK(x, 1, 19)
#define v_AXI_OUTSTANDING_MAX_NUM(x)	BITS_MASK(x, 0x1f, 12)
#define v_AXI_MAX_OUTSTANDING_EN(x)	BITS_MASK(x, 1, 11)
#define v_MMU_EN(x)			BITS_MASK(x, 1, 10)
#define v_NOC_HURRY_THRESHOLD(x)	BITS_MASK(x, 0xf, 6)
#define v_NOC_HURRY_VALUE(x)		BITS_MASK(x, 3, 4)
#define v_NOC_HURRY_EN(x)		BITS_MASK(x, 1, 3)
#define v_NOC_QOS_VALUE(x)		BITS_MASK(x, 3, 1)
#define v_NOC_QOS_EN(x)			BITS_MASK(x, 1, 0)
	
#define GATHER_TRANSFER		(0x84)
#define m_WIN1_AXI_GATHER_NUM		BITS(0xf, 12)
#define m_WIN0_CBCR_AXI_GATHER_NUM	BITS(0x7, 8)
#define m_WIN0_YRGB_AXI_GATHER_NUM	BITS(0xf, 4)
#define m_WIN1_AXI_GAHTER_EN		BITS(1, 2)
#define m_WIN0_CBCR_AXI_GATHER_EN	BITS(1, 1)
#define m_WIN0_YRGB_AXI_GATHER_EN	BITS(1, 0)

#define v_WIN1_AXI_GATHER_NUM(x)	BITS_MASK(x, 0xf, 12)
#define v_WIN0_CBCR_AXI_GATHER_NUM(x)	BITS_MASK(x, 0x7, 8)
#define v_WIN0_YRGB_AXI_GATHER_NUM(x)	BITS_MASK(x, 0xf, 4)
#define v_WIN1_AXI_GAHTER_EN(x)		BITS_MASK(x, 1, 2)
#define v_WIN0_CBCR_AXI_GATHER_EN(x)	BITS_MASK(x, 1, 1)
#define v_WIN0_YRGB_AXI_GATHER_EN(x)	BITS_MASK(x, 1, 0)
	
#define VERSION_INFO		(0x94)
#define m_MAJOR		                BITS(0xff, 24)
#define m_MINOR		                BITS(0xff, 16)
#define m_BUILD		                BITS(0xffff)
		
#define REG_CFG_DONE		(0x90)

/* TV Control Registers */
#define TV_CTRL			(0x200)	
#define TV_SYNC_TIMING		(0x204)
#define TV_ACT_TIMING		(0x208)
#define TV_ADJ_TIMING		(0x20c)
#define TV_FREQ_SC		(0x210)
#define TV_FILTER0		(0x214)
#define TV_FILTER1		(0x218)
#define TV_FILTER2		(0x21C)
#define TV_ACT_ST		(0x234)
#define TV_ROUTING		(0x238)
#define TV_SYNC_ADJUST		(0x250)
#define TV_STATUS		(0x254)
#define TV_RESET		(0x268)
#define TV_SATURATION		(0x278)
#define TV_BW_CTRL		(0x28C)
#define TV_BRIGHTNESS_CONTRAST	(0x290)


/* MMU registers */
#define MMU_DTE_ADDR		(0x0300)
#define m_MMU_DTE_ADDR			BITS(0xffffffff, 0)
#define v_MMU_DTE_ADDR(x)		BITS_MASK(x, 0xffffffff, 0)

#define MMU_STATUS		(0x0304)
#define m_PAGING_ENABLED		BITS(1, 0)
#define m_PAGE_FAULT_ACTIVE		BITS(1, 1)
#define m_STAIL_ACTIVE			BITS(1, 2)
#define m_MMU_IDLE			BITS(1, 3)
#define m_REPLAY_BUFFER_EMPTY		BITS(1, 4)
#define m_PAGE_FAULT_IS_WRITE		BITS(1, 5)
#define m_PAGE_FAULT_BUS_ID		BITS(0x1f, 6)

#define v_PAGING_ENABLED(x)		BITS_MASK(x, 1, 0)
#define v_PAGE_FAULT_ACTIVE(x)		BITS_MASK(x, 1, 1)
#define v_STAIL_ACTIVE(x)		BITS_MASK(x, 1, 2)
#define v_MMU_IDLE(x)			BITS_MASK(x, 1, 3)
#define v_REPLAY_BUFFER_EMPTY(x)	BITS_MASK(x, 1, 4)
#define v_PAGE_FAULT_IS_WRITE(x)	BITS_MASK(x, 1, 5)
#define v_PAGE_FAULT_BUS_ID(x)		BITS_MASK(x, 0x1f, 6)
	
#define MMU_COMMAND		(0x0308)
#define m_MMU_CMD			BITS(0x7, 0)
#define v_MMU_CMD(x)			BITS_MASK(x, 0x7, 0)	

#define MMU_PAGE_FAULT_ADDR	(0x030c)
#define m_PAGE_FAULT_ADDR		BITS(0xffffffff, 0)
#define v_PAGE_FAULT_ADDR(x)		BITS_MASK(x, 0xffffffff, 0)
	
#define MMU_ZAP_ONE_LINE	(0x0310)
#define m_MMU_ZAP_ONE_LINE		BITS(0xffffffff, 0)
#define v_MMU_ZAP_ONE_LINE(x)		BITS_MASK(x, 0xffffffff, 0)

#define MMU_INT_RAWSTAT		(0x0314)
#define m_PAGE_FAULT_RAWSTAT		BITS(1, 0)
#define m_READ_BUS_ERROR_RAWSTAT	BITS(1, 1)

#define v_PAGE_FAULT_RAWSTAT(x)		BITS(x, 1, 0)
#define v_READ_BUS_ERROR_RAWSTAT(x)	BITS(x, 1, 1)
	
#define MMU_INT_CLEAR		(0x0318)
#define m_PAGE_FAULT_CLEAR		BITS(1, 0)
#define m_READ_BUS_ERROR_CLEAR		BITS(1, 1)

#define v_PAGE_FAULT_CLEAR(x)		BITS(x, 1, 0)
#define v_READ_BUS_ERROR_CLEAR(x)	BITS(x, 1, 1)
	
#define MMU_INT_MASK		(0x031c)
#define m_PAGE_FAULT_MASK		BITS(1, 0)
#define m_READ_BUS_ERROR_MASK		BITS(1, 1)

#define v_PAGE_FAULT_MASK(x)		BITS(x, 1, 0)
#define v_READ_BUS_ERROR_MASK(x)	BITS(x, 1, 1)
	
#define MMU_INT_STATUS		(0x0320)
#define m_PAGE_FAULT_STATUS		BITS(1, 0)
#define m_READ_BUS_ERROR_STATUS		BITS(1, 1)

#define v_PAGE_FAULT_STATUS(x)		BITS(x, 1, 0)
#define v_READ_BUS_ERROR_STATUS(x)	BITS(x, 1, 1)

#define MMU_AUTO_GATING		(0x0324)
#define m_MMU_AUTO_GATING		BITS(1, 0)
#define v_MMU_AUTO_GATING(x)		BITS(x, 1, 0)

#define REG_LEN		(MMU_AUTO_GATING + 4)
enum _vop_dma_burst {
	DMA_BURST_16 = 0,
	DMA_BURST_8,
	DMA_BURST_4
};

enum _vop_format_e {
	VOP_FORMAT_ARGB888 = 0,
	VOP_FORMAT_RGB888,
	VOP_FORMAT_RGB565,
	VOP_FORMAT_YCBCR420 = 4,
	VOP_FORMAT_YCBCR422,
	VOP_FORMAT_YCBCR444
};

enum _vop_tv_mode {
	TV_NTSC,
	TV_PAL,
};

enum _vop_r2y_csc_mode {
	VOP_R2Y_CSC_BT601 = 0,
	VOP_R2Y_CSC_BT709
};

enum _vop_y2r_csc_mode {
	VOP_Y2R_CSC_MPEG = 0,
	VOP_Y2R_CSC_JPEG,
	VOP_Y2R_CSC_HD,
	VOP_Y2R_CSC_BYPASS
};

enum _vop_hwc_size {
	VOP_HWC_SIZE_32,
	VOP_HWC_SIZE_64
};

enum _vop_overlay_mode {
	VOP_RGB_DOMAIN,
	VOP_YUV_DOMAIN
};

enum _vop_output_mode {
	COLOR_RGB,
	COLOR_YCBCR
};

#define CalScale(x, y)	             ((((u32)(x - 1)) * 0x1000) / (y - 1))


struct lcdc_device {
	int node;
        int soc_type;
	int dft_win; /*default win for display*/
	int regsbak[REG_LEN];
	int regs;
	int output_color;
	int overlay_mode;
};

struct lcdc_device rk312x_lcdc;
static inline void lcdc_writel(struct lcdc_device *lcdc_dev, u32 offset, u32 v)
{
	u32 *_pv = (u32*)lcdc_dev->regsbak;	
	_pv += (offset >> 2);	
	*_pv = v;
	writel(v, lcdc_dev->regs + offset);	
}

static inline u32 lcdc_readl(struct lcdc_device *lcdc_dev, u32 offset)
{
	u32 v;
	u32 *_pv = (u32*)lcdc_dev->regsbak;
	_pv += (offset >> 2);
	v = readl(lcdc_dev->regs + offset);
	*_pv = v;
	return v;
}

static inline u32 lcdc_read_bit(struct lcdc_device *lcdc_dev, u32 offset,
                                u32 msk) 
{
       u32 _v = readl(lcdc_dev->regs + offset); 
       _v &= msk;
       return (_v? 1 : 0);   
}

static inline void  lcdc_set_bit(struct lcdc_device *lcdc_dev, u32 offset,
                                 u32 msk) 
{
	u32* _pv = (u32*)lcdc_dev->regsbak;	
	_pv += (offset >> 2);				
	(*_pv) |= msk;				
	writel(*_pv, lcdc_dev->regs + offset); 
} 

static inline void lcdc_clr_bit(struct lcdc_device *lcdc_dev, u32 offset,
                                u32 msk)
{
	u32* _pv = (u32*)lcdc_dev->regsbak;	
	_pv += (offset >> 2);				
	(*_pv) &= (~msk);				
	writel(*_pv, lcdc_dev->regs + offset); 
} 

static inline void  lcdc_msk_reg(struct lcdc_device *lcdc_dev, u32 offset,
                                 u32 msk, u32 v)
{
	u32 *_pv = (u32*)lcdc_dev->regsbak;	
	_pv += (offset >> 2);			
	(*_pv) &= (~msk);				
	(*_pv) |= v;				
	writel(*_pv, lcdc_dev->regs + offset);	
}

static inline void lcdc_cfg_done(struct lcdc_device *lcdc_dev) 
{
	writel(0x01, lcdc_dev->regs + REG_CFG_DONE); 
} 


/* RK312X_GRF_LVDS_CON0 */
#define v_LVDS_DATA_SEL(x)      (BITS_MASK(x, 1, 0) | BITS_EN(1, 0))
#define v_LVDS_OUTPUT_FORMAT(x) (BITS_MASK(x, 3, 1) | BITS_EN(3, 1))
#define v_LVDS_MSBSEL(x)        (BITS_MASK(x, 1, 3) | BITS_EN(1, 3))
#define v_LVDSMODE_EN(x)        (BITS_MASK(x, 1, 6) | BITS_EN(1, 6))
#define v_MIPIPHY_TTL_EN(x)     (BITS_MASK(x, 1, 7) | BITS_EN(1, 7))
#define v_MIPIPHY_LANE0_EN(x)   (BITS_MASK(x, 1, 8) | BITS_EN(1, 8))
#define v_MIPIDPI_FORCEX_EN(x)  (BITS_MASK(x, 1, 9) | BITS_EN(1, 9))

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
enum {
        LVDS_DATA_FROM_LCDC = 0,
        LVDS_DATA_FORM_EBC,
};

enum {
        LVDS_MSB_D0 = 0,
        LVDS_MSB_D7,
};


/* RK312X_GRF_SOC_CON1 */
#define v_MIPITTL_CLK_EN(x)     (BITS_MASK(x, 1, 7) | BITS_EN(1, 7))
#define v_MIPITTL_LANE0_EN(x)   (BITS_MASK(x, 1, 11) | BITS_EN(1, 11))
#define v_MIPITTL_LANE1_EN(x)   (BITS_MASK(x, 1, 12) | BITS_EN(1, 12))
#define v_MIPITTL_LANE2_EN(x)   (BITS_MASK(x, 1, 13) | BITS_EN(1, 13))
#define v_MIPITTL_LANE3_EN(x)   (BITS_MASK(x, 1, 14) | BITS_EN(1, 14))


#define MIPIPHY_REG0            0x0000
#define m_LANE_EN_0             BITS(1, 2)
#define m_LANE_EN_1             BITS(1, 3)
#define m_LANE_EN_2             BITS(1, 4)
#define m_LANE_EN_3             BITS(1, 5)
#define m_LANE_EN_CLK           BITS(1, 5)
#define v_LANE_EN_0(x)          BITS(1, 2)
#define v_LANE_EN_1(x)          BITS(1, 3)
#define v_LANE_EN_2(x)          BITS(1, 4)
#define v_LANE_EN_3(x)          BITS(1, 5)
#define v_LANE_EN_CLK(x)        BITS(1, 5)

#define MIPIPHY_REG1            0x0004
#define m_SYNC_RST              BITS(1, 0)
#define m_LDO_PWR_DOWN          BITS(1, 1)
#define m_PLL_PWR_DOWN          BITS(1, 2)
#define v_SYNC_RST(x)           BITS_MASK(x, 1, 0)
#define v_LDO_PWR_DOWN(x)       BITS_MASK(x, 1, 1)
#define v_PLL_PWR_DOWN(x)       BITS_MASK(x, 1, 2)

#define MIPIPHY_REG3		0x000c
#define m_PREDIV                BITS(0x1f, 0)
#define m_FBDIV_MSB             BITS(1, 5)
#define v_PREDIV(x)             BITS_MASK(x, 0x1f, 0)
#define v_FBDIV_MSB(x)          BITS_MASK(x, 1, 5)

#define MIPIPHY_REG4		0x0010
#define v_FBDIV_LSB(x)          BITS_MASK(x, 0xff, 0)

#define MIPIPHY_REGE0		0x0380
#define m_MSB_SEL               BITS(1, 0)
#define m_DIG_INTER_RST         BITS(1, 2)
#define m_LVDS_MODE_EN          BITS(1, 5)
#define m_TTL_MODE_EN           BITS(1, 6)
#define m_MIPI_MODE_EN          BITS(1, 7)
#define v_MSB_SEL(x)            BITS_MASK(x, 1, 0)
#define v_DIG_INTER_RST(x)      BITS_MASK(x, 1, 2)
#define v_LVDS_MODE_EN(x)       BITS_MASK(x, 1, 5)
#define v_TTL_MODE_EN(x)        BITS_MASK(x, 1, 6)
#define v_MIPI_MODE_EN(x)       BITS_MASK(x, 1, 7)

#define MIPIPHY_REGE1           0x0384
#define m_DIG_INTER_EN          BITS(1, 7)
#define v_DIG_INTER_EN(x)       BITS_MASK(x, 1, 7)

#define MIPIPHY_REGE3           0x038c
#define m_MIPI_EN               BITS(1, 0)
#define m_LVDS_EN               BITS(1, 1)
#define m_TTL_EN                BITS(1, 2)
#define v_MIPI_EN(x)            BITS_MASK(x, 1, 0)
#define v_LVDS_EN(x)            BITS_MASK(x, 1, 1)
#define v_TTL_EN(x)             BITS_MASK(x, 1, 2)

#define MIPIPHY_REGE8           0x03a0

#define MIPIPHY_REGEB           0x03ac
#define v_PLL_PWR_OFF(x)        BITS_MASK(x, 1, 2)
#define v_LANECLK_EN(x)         BITS_MASK(x, 1, 3)
#define v_LANE3_EN(x)           BITS_MASK(x, 1, 4)
#define v_LANE2_EN(x)           BITS_MASK(x, 1, 5)
#define v_LANE1_EN(x)           BITS_MASK(x, 1, 6)
#define v_LANE0_EN(x)           BITS_MASK(x, 1, 7)

#define MIPIPHY_STATUS		0x101100b0		

struct rk_lvds_device {
	int    base;
	int    ctrl_reg;
	bool   sys_state;
};

struct rk_lvds_device rk31xx_lvds;

static inline int lvds_writel(struct rk_lvds_device *lvds, u32 offset, u32 val)
{
	writel(val, lvds->base + offset);
	return 0;
}

static inline int lvds_msk_reg(struct rk_lvds_device *lvds, u32 offset,
			       u32 msk, u32 val)
{
	u32 temp;

	temp = readl(lvds->base + offset) & (0xFF - (msk));
	writel(temp | ((val) & (msk)), lvds->base + offset);
	return 0;
}

static inline u32 lvds_readl(struct rk_lvds_device *lvds, u32 offset)
{
	return readl(lvds->base + offset);
}


static inline u32 lvds_phy_lock(struct rk_lvds_device *lvds)
{
	u32 val = 0;
	val = readl(lvds->ctrl_reg);
	return (val & 0x01);
}


static int rk31xx_lvds_pwr_on(vidinfo_t * vid)
{
        struct rk_lvds_device *lvds = &rk31xx_lvds;
	u32 delay_times = 20;

        if (vid->screen_type == SCREEN_LVDS) {
                /* power up lvds pll and ldo */
	        lvds_msk_reg(lvds, MIPIPHY_REG1,
	                     m_SYNC_RST | m_LDO_PWR_DOWN | m_PLL_PWR_DOWN,
	                     v_SYNC_RST(0) | v_LDO_PWR_DOWN(0) | v_PLL_PWR_DOWN(0));
		/* enable lvds lane and power on pll */
		lvds_writel(lvds, MIPIPHY_REGEB,
			    v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) |
			    v_LANE3_EN(1) | v_LANECLK_EN(1) | v_PLL_PWR_OFF(0));

	        /* enable lvds */
	        lvds_msk_reg(lvds, MIPIPHY_REGE3,
	                     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
	                     v_MIPI_EN(0) | v_LVDS_EN(1) | v_TTL_EN(0));
        } else {
                lvds_msk_reg(lvds, MIPIPHY_REGE3,
	                     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
	                     v_MIPI_EN(0) | v_LVDS_EN(0) | v_TTL_EN(1));
        }
	/* delay for waitting pll lock on */
	while (delay_times--) {
		if (lvds_phy_lock(lvds)) {
			break;
		}
		udelay(100);
	}

	if (delay_times <= 0)
		printf("wait lvds phy lock failed\n");
		
        return 0;
}

static void rk31xx_output_lvds(vidinfo_t *vid)
{
	struct rk_lvds_device *lvds = &rk31xx_lvds;
	u32 val = 0;

	/* if LVDS transmitter source from VOP, vop_dclk need get invert
	 * set iomux in dts pinctrl
	 */
	val = 0;
	val |= v_LVDSMODE_EN(1) | v_MIPIPHY_TTL_EN(0);  /* enable lvds mode */
	val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC);    /* config data source */
	val |= v_LVDS_OUTPUT_FORMAT(vid->lvds_format); /* config lvds_format */
	val |= v_LVDS_MSBSEL(LVDS_MSB_D7);      /* LSB receive mode */
	val |= v_MIPIPHY_LANE0_EN(1) | v_MIPIDPI_FORCEX_EN(1);
	grf_writel(val, GRF_LVDS_CON0);

	/* digital internal disable */
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(0));

	/* set pll prediv and fbdiv */
	lvds_writel(lvds, MIPIPHY_REG3, v_PREDIV(2) | v_FBDIV_MSB(0));
	lvds_writel(lvds, MIPIPHY_REG4, v_FBDIV_LSB(28));

	lvds_writel(lvds, MIPIPHY_REGE8, 0xfc);

	/* set lvds mode and reset phy config */
	lvds_msk_reg(lvds, MIPIPHY_REGE0,
	             m_MSB_SEL | m_DIG_INTER_RST,
	             v_MSB_SEL(1) | v_DIG_INTER_RST(1));

	rk31xx_lvds_pwr_on(vid);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));


}

static void rk31xx_output_lvttl(vidinfo_t *vid)
{
	u32 val = 0;
	struct rk_lvds_device *lvds = &rk31xx_lvds;

	grf_writel(0xfff35555, GRF_GPIO2B_IOMUX);
	grf_writel(0x00ff0055, GRF_GPIO2C_IOMUX);
	grf_writel(0x77771111, GRF_GPIO2C_IOMUX2);
	grf_writel(0x700c1004, GRF_GPIO2D_IOMUX);

	val |= v_LVDSMODE_EN(0) | v_MIPIPHY_TTL_EN(1);  /* enable lvds mode */
	val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC);    /* config data source */
	grf_writel(0xffff0380, GRF_LVDS_CON0);

	val = v_MIPITTL_CLK_EN(1) | v_MIPITTL_LANE0_EN(1) |
	        v_MIPITTL_LANE1_EN(1) | v_MIPITTL_LANE2_EN(1) |
	        v_MIPITTL_LANE3_EN(1);
	grf_writel(val, GRF_SOC_CON1);

	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
	        v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);

	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
	        v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);

	rk31xx_lvds_pwr_on(vid);
		
}

static int rk31xx_lvds_en(vidinfo_t *vid)
{
	rk31xx_lvds.base = 0x20038000;
	rk31xx_lvds.ctrl_reg = MIPIPHY_STATUS;

	switch (vid->screen_type) {
	case SCREEN_LVDS:
		rk31xx_output_lvds(vid);
	        break;
	case SCREEN_RGB:
		rk31xx_output_lvttl(vid);
	        break;
	default:
	        printf("unsupport screen type\n");
	        break;
	}
	return 0;
}

#endif


static int win0_set_par(struct lcdc_device *lcdc_dev,
			 struct fb_dsp_info *fb_info,
			 vidinfo_t *vid)
{
	int x_scale, y_scale;
 	int msk,val;
	
	x_scale = CalScale(fb_info->xact, fb_info->xsize);
	y_scale = CalScale(fb_info->yact, fb_info->ysize);
	lcdc_writel(lcdc_dev,WIN0_YRGB_MST, fb_info->yaddr);
	val = v_X_SCL_FACTOR(x_scale) | v_Y_SCL_FACTOR(y_scale);
	lcdc_writel(lcdc_dev, WIN0_SCL_FACTOR_YRGB, val);
	msk = m_WIN0_EN | m_WIN0_FORMAT;
	val = v_WIN0_EN(1) | v_WIN0_FORMAT(fb_info->format);
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, msk, val);
	val = v_ACT_WIDTH(fb_info->xact) | v_ACT_HEIGHT(fb_info->yact);
	lcdc_writel(lcdc_dev, WIN0_ACT_INFO, val);
	val = v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) |
	      v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
	lcdc_writel(lcdc_dev, WIN0_DSP_ST, val); 
	val = v_DSP_WIDTH(fb_info->xsize) | v_DSP_HEIGHT(fb_info->ysize);
	lcdc_writel(lcdc_dev, WIN0_DSP_INFO, val);
	msk = m_COLOR_KEY_EN | m_COLOR_KEY_VAL;
	val = v_COLOR_KEY_EN(0) | v_COLOR_KEY_VAL(0);
	lcdc_msk_reg(lcdc_dev, WIN0_COLOR_KEY, msk, val);
	switch(fb_info->format) 
	{
	case ARGB888:
		val = v_ARGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB888:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		val = v_RGB565_VIRWIDTH(fb_info->xvir);
		break;
	case YUV422:
	case YUV420:
		val = v_YUV_VIRWIDTH(fb_info->xvir);  
		break;
	default:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		printf("%s unknow format:%d\n", __func__,
		       fb_info->format);
		break;
	}
	lcdc_writel(lcdc_dev, WIN0_VIR, val);
	lcdc_cfg_done(lcdc_dev);
	return 0;
}

static int win1_set_par(struct lcdc_device *lcdc_dev,
			 struct fb_dsp_info *fb_info,
			 vidinfo_t *vid)
{
	int x_scale, y_scale;
 	int msk,val;
	
	x_scale = CalScale(fb_info->xact, fb_info->xsize);
	y_scale = CalScale(fb_info->yact, fb_info->ysize);
	if (lcdc_dev->soc_type == CONFIG_RK3036) {
		val = v_X_SCL_FACTOR(x_scale) | v_Y_SCL_FACTOR(y_scale);
		lcdc_writel(lcdc_dev, WIN1_SCL_FACTOR_YRGB, val);
	} else {
		if ((fb_info->xact != fb_info->xsize) ||
		      (fb_info->yact != fb_info->ysize))
		      printf("win1 don't support scale!\n");
	}
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_WIN1_EN | m_WIN1_FORMAT, 
		     v_WIN1_EN(1) | v_WIN1_FORMAT(fb_info->format));

	if (lcdc_dev->soc_type == CONFIG_RK3036) {
		val = v_ACT_WIDTH(fb_info->xact) | v_ACT_HEIGHT(fb_info->yact);
		lcdc_writel(lcdc_dev, WIN1_ACT_INFO, val);
		val = v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) | 
		      v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev, WIN1_DSP_ST, val);
		val = v_DSP_WIDTH(fb_info->xsize)| v_DSP_HEIGHT(fb_info->ysize);
		lcdc_writel(lcdc_dev, WIN1_DSP_INFO, val);
	} else {
		val = v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) | 
		      v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev, WIN1_DSP_ST_RK312X, val);
		val = v_DSP_WIDTH(fb_info->xsize)| v_DSP_HEIGHT(fb_info->ysize);
		lcdc_writel(lcdc_dev, WIN1_DSP_INFO_RK312X, val);
	}

	msk = m_COLOR_KEY_EN | m_COLOR_KEY_VAL;
	val = v_COLOR_KEY_EN(0) | v_COLOR_KEY_VAL(0);
	lcdc_msk_reg(lcdc_dev, WIN1_COLOR_KEY, msk, val);
	switch(fb_info->format) 
	{
	case ARGB888:
	case RGB888:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		break;
	case RGB565:
		val = v_RGB565_VIRWIDTH(fb_info->xvir);
		break;
	case YUV422:
	case YUV420:   
		val = v_YUV_VIRWIDTH(fb_info->xvir);         
		break;
	default:
		val = v_RGB888_VIRWIDTH(fb_info->xvir);
		printf("unknown format %d\n",fb_info->format);
		break;
	}
	lcdc_writel(lcdc_dev, WIN1_VIR, val);
	if (lcdc_dev->soc_type == CONFIG_RK3036)
		lcdc_writel(lcdc_dev, WIN1_MST, fb_info->yaddr);
	else
		lcdc_writel(lcdc_dev, WIN1_MST_RK312X, fb_info->yaddr);
	lcdc_cfg_done(lcdc_dev);
	return 0;
}


static void rk312x_lcdc_select_bcsh(struct lcdc_device *lcdc_dev)
{
	int msk, val;
	int bcsh_open;
	if (lcdc_dev->overlay_mode == VOP_YUV_DOMAIN) {
		if (lcdc_dev->output_color == COLOR_YCBCR) {/* bypass */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_Y2R_EN | m_BCSH_R2Y_EN,
				     v_BCSH_Y2R_EN(0) | v_BCSH_R2Y_EN(0));
			bcsh_open = 0;
		} else 	{/* YUV2RGB */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
			     m_BCSH_Y2R_EN | m_BCSH_Y2R_CSC_MODE |
			     m_BCSH_R2Y_EN,
			     v_BCSH_Y2R_EN(1) |
			     v_BCSH_Y2R_CSC_MODE(VOP_Y2R_CSC_MPEG) |
			     v_BCSH_R2Y_EN(0));
			bcsh_open = 1;
		}
	} else {	/* overlay_mode=VOP_RGB_DOMAIN */
		if (lcdc_dev->output_color == COLOR_RGB) {	/* bypass */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_R2Y_EN | m_BCSH_Y2R_EN,
				     v_BCSH_R2Y_EN(1) | v_BCSH_Y2R_EN(1));
			bcsh_open = 0;
		} else {/* RGB2YUV */
			lcdc_msk_reg(lcdc_dev, BCSH_CTRL,
				     m_BCSH_R2Y_EN |
					m_BCSH_R2Y_CSC_MODE | m_BCSH_Y2R_EN,
					v_BCSH_R2Y_EN(1) |
					v_BCSH_R2Y_CSC_MODE(VOP_Y2R_CSC_MPEG) |
					v_BCSH_Y2R_EN(0));
			bcsh_open = 1;
		}
	}
	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, m_SW_OVERLAY_MODE,
		     v_SW_OVERLAY_MODE(lcdc_dev->overlay_mode));
	if (bcsh_open) {
		lcdc_msk_reg(lcdc_dev,
			     BCSH_CTRL, m_BCSH_EN | m_BCSH_OUT_MODE,
			     v_BCSH_EN(1) | v_BCSH_OUT_MODE(3));
		lcdc_writel(lcdc_dev, BCSH_BCS,
			    v_BCSH_BRIGHTNESS(0x00) |
			    v_BCSH_CONTRAST(0x80) |
			    v_BCSH_SAT_CON(0x80));
		lcdc_writel(lcdc_dev, BCSH_H, v_BCSH_COS_HUE(0x80));
	} else {
		msk = m_BCSH_EN;
		val = v_BCSH_EN(0);
		lcdc_msk_reg(lcdc_dev, BCSH_CTRL, msk, val);
	}
}


/* Configure VENC for a given Mode (NTSC / PAL) */
void rk_lcdc_set_par(struct fb_dsp_info *fb_info,
			     vidinfo_t *vid)
{
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;
	if (vid->vmode) {
		fb_info->ysize /= 2;
		fb_info->ypos  /= 2;
	}

	fb_info->layer_id = lcdc_dev->dft_win;
	if(fb_info->layer_id == WIN0)
		win0_set_par(lcdc_dev, fb_info, vid);
	else if(fb_info->layer_id == WIN1) 
		win1_set_par(lcdc_dev, fb_info, vid);
	else 
		printf("%s --->unknow lay_id \n", __func__);

	/*setenv("bootdelay", "3");*/
}

int rk_lcdc_load_screen(vidinfo_t *vid)
{
	int face = 0;
	int msk,val;
	int bg_val = 0;
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;
	lcdc_dev->output_color = COLOR_RGB;
	lcdc_dev->overlay_mode = VOP_RGB_DOMAIN;
	switch (vid->screen_type) {
	case SCREEN_HDMI:
        	lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
			      m_HDMI_DCLK_EN, v_HDMI_DCLK_EN(1));
		if (vid->pixelrepeat) {
			lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
				      m_CORE_CLK_DIV_EN, v_CORE_CLK_DIV_EN(1));
		}
		if (gd->arch.chiptype == CONFIG_RK3128) {
			 lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
				      m_SW_UV_OFFSET_EN,
				      v_SW_UV_OFFSET_EN(0));
			 msk = m_HDMI_HSYNC_POL | m_HDMI_VSYNC_POL |
				m_HDMI_DEN_POL;
			 val = v_HDMI_HSYNC_POL(vid->vl_hsp) |
			       v_HDMI_VSYNC_POL(vid->vl_vsp) |
			       v_HDMI_DEN_POL(vid->vl_oep);
			 lcdc_msk_reg(lcdc_dev, INT_SCALER, msk, val);
		 } else {
			 msk = (1 << 4) | (1 << 5) | (1 << 6);
			 val = (vid->vl_hsp << 4) |
				 (vid->vl_vsp << 5) |
				 (vid->vl_oep << 6);
			 grf_writel((msk << 16)|val,GRF_SOC_CON2);
		 }
		 bg_val = 0x801080;
		break;
	case SCREEN_TVOUT:
	case SCREEN_TVOUT_TEST:
		lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
			      m_TVE_DAC_DCLK_EN | m_HDMI_DCLK_EN,
			      v_TVE_DAC_DCLK_EN(1) |
			      v_HDMI_DCLK_EN(1));
		if (vid->pixelrepeat) {
			lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL,
				      m_CORE_CLK_DIV_EN, v_CORE_CLK_DIV_EN(1));
		}
		if(vid->vl_col == 720 && vid->vl_row== 576) {
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0,m_TVE_MODE, v_TVE_MODE(TV_PAL));
		} else if(vid->vl_col == 720 && vid->vl_row== 480) {
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0, m_TVE_MODE, v_TVE_MODE(TV_NTSC));
		} else {
			printf("unsupported video timing!\n");
			return -EINVAL;
		}
		if (vid->screen_type == SCREEN_TVOUT_TEST) {/*for TVE index test,vop must ovarlay at yuv domain*/
			lcdc_dev->overlay_mode = VOP_YUV_DOMAIN;
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0, m_SW_UV_OFFSET_EN,
				     v_SW_UV_OFFSET_EN(1));
		} else {
			bg_val = 0x801080;
		}
		if (lcdc_dev->soc_type == CONFIG_RK3128) {
			lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
				     m_SW_UV_OFFSET_EN,
				     v_SW_UV_OFFSET_EN(1));
		}

		break;
	case SCREEN_LVDS:
		msk = m_LVDS_DCLK_INVERT | m_LVDS_DCLK_EN;
		val = v_LVDS_DCLK_INVERT(1) | v_LVDS_DCLK_EN(1);
		lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, msk, val);	
		break;
	case SCREEN_RGB:
		lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, m_RGB_DCLK_EN |
			      m_LVDS_DCLK_EN, v_RGB_DCLK_EN(1) | v_LVDS_DCLK_EN(1));
		break;
	case SCREEN_MIPI:
		lcdc_msk_reg(lcdc_dev,AXI_BUS_CTRL, m_MIPI_DCLK_EN, v_MIPI_DCLK_EN(1));
		break;
	default:
		printf("unsupport screen_type %d\n",vid->screen_type);
		break;
	}
 	
    	lcdc_msk_reg(lcdc_dev,DSP_CTRL1, m_BLACK_EN | m_BLANK_EN | m_DSP_OUT_ZERO|
		      m_DSP_BG_SWAP | m_DSP_RG_SWAP | m_DSP_RB_SWAP | m_BG_COLOR |
		      m_DSP_DELTA_SWAP | m_DSP_DUMMY_SWAP, v_BLACK_EN(0) |
		      v_BLANK_EN(0) | v_DSP_OUT_ZERO(0) | v_DSP_BG_SWAP(0) |
		      v_DSP_RG_SWAP(0) | v_DSP_RB_SWAP(0) | v_BG_COLOR(0) |
		      v_DSP_DELTA_SWAP(0) | v_DSP_DUMMY_SWAP(0));

	if (gd->arch.chiptype == CONFIG_RK3036)
	    bg_val = 0;

	lcdc_msk_reg(lcdc_dev, DSP_CTRL1, m_BG_COLOR,
		      v_BG_COLOR(bg_val));

	switch (vid->lcd_face)
	{
    	case OUT_P565:
        	face = OUT_P565;
		msk =  m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		       m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val =  v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		       v_DITHER_DOWN_MODE(0) | v_DITHER_DOWN_SEL(1);
		break;
    	case OUT_P666:
		face = OUT_P666;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		      m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		      v_DITHER_DOWN_MODE(1) | v_DITHER_DOWN_SEL(1);
		break;
    	case OUT_D888_P565:
		msk =  m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		       m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val =  v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		       v_DITHER_DOWN_MODE(0) | v_DITHER_DOWN_SEL(1);
		break;
    	case OUT_D888_P666:
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN |
		      m_DITHER_DOWN_MODE | m_DITHER_DOWN_SEL;
		val = v_DITHER_DOWN_EN(1) | v_DITHER_UP_EN(0) |
		      v_DITHER_DOWN_MODE(1) | v_DITHER_DOWN_SEL(1);
    	case OUT_P888:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);
        	break;
    	default:
		face = OUT_P888;
		msk = m_DITHER_DOWN_EN | m_DITHER_UP_EN;
		val = v_DITHER_DOWN_EN(0) | v_DITHER_UP_EN(0);
		printf("unknown interface:%d\n", face);
        	break;
	}

	msk |= m_HSYNC_POL | m_HSYNC_POL | m_DEN_POL |
	       m_DCLK_POL | m_DSP_OUT_FORMAT;
	val |=  v_HSYNC_POL(vid->vl_hsp) | v_VSYNC_POL(vid->vl_vsp) |
		v_DEN_POL(vid->vl_oep)| v_DCLK_POL(vid->vl_clkp) |
		v_DSP_OUT_FORMAT(face);
    	lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);

	val = v_HORPRD(vid->vl_hspw + vid->vl_hbpd + vid->vl_col + vid->vl_hfpd) |
	      v_HSYNC(vid->vl_hspw);
	lcdc_writel(lcdc_dev, DSP_HTOTAL_HS_END, val);

	val = v_HAEP(vid->vl_hspw + vid->vl_hbpd + vid->vl_col) |
	      v_HASP(vid->vl_hspw + vid->vl_hbpd);
	lcdc_writel(lcdc_dev, DSP_HACT_ST_END, val);

	if(vid->vmode) {
		//First Field Timing
		val = v_VERPRD(2 * (vid->vl_vspw + vid->vl_vbpd + vid->vl_vfpd) +
		      vid->vl_row + 1) | v_VSYNC(vid->vl_vspw);
		lcdc_writel(lcdc_dev, DSP_VTOTAL_VS_END, val);
		val = v_VAEP(vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2)|
		      v_VASP(vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev,DSP_VACT_ST_END, val);
		//Second Field Timing
		val = v_VSYNC_ST_F1(vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2 + vid->vl_vfpd) |
		      v_VSYNC_END_F1(2 * vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2 + vid->vl_vfpd);
		lcdc_writel(lcdc_dev, DSP_VS_ST_END_F1, val);
		val = v_VAEP(2 * (vid->vl_vspw + vid->vl_vbpd) + vid->vl_row + vid->vl_vfpd + 1)|
		      v_VASP(2 * (vid->vl_vspw + vid->vl_vbpd) + vid->vl_row/2 + vid->vl_vfpd + 1);
		lcdc_writel(lcdc_dev, DSP_VACT_ST_END_F1, val);
		msk = m_INTERLACE_DSP_EN | m_WIN1_INTERLACE_EN |
		      m_WIN0_YRGB_DEFLICK_EN | m_WIN0_CBR_DEFLICK_EN |
		      m_WIN0_INTERLACE_EN;
		val = v_INTERLACE_DSP_EN(1) | v_WIN1_INTERLACE_EN(1) |
		      v_WIN0_YRGB_DEFLICK_EN(1) | v_WIN0_CBR_DEFLICK_EN(1) |
		      v_WIN0_INTERLACE_EN(1);
		lcdc_msk_reg(lcdc_dev,DSP_CTRL0, msk, val);

		lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
			     m_INTERLACE_DSP_EN |
			     m_WIN0_YRGB_DEFLICK_EN |
			     m_WIN0_CBR_DEFLICK_EN |
			     m_INTERLACE_FIELD_POL |
			     m_WIN0_INTERLACE_EN |
			     m_WIN1_INTERLACE_EN,
			     v_INTERLACE_DSP_EN(1) |
			     v_WIN0_YRGB_DEFLICK_EN(1) |
			     v_WIN0_CBR_DEFLICK_EN(1) |
			     v_INTERLACE_FIELD_POL(0) |
			     v_WIN0_INTERLACE_EN(1) |
			     v_WIN1_INTERLACE_EN(1));

		msk = m_LF_INT_NUM;
		val = v_LF_INT_NUM(vid->vl_vspw + vid->vl_vbpd + vid->vl_row/2);
		lcdc_msk_reg(lcdc_dev, INT_STATUS, msk, val);

	} else {
		val = v_VERPRD(vid->vl_vspw + vid->vl_vbpd + vid->vl_row +
		      vid->vl_vfpd) | v_VSYNC(vid->vl_vspw);
		lcdc_writel(lcdc_dev, DSP_VTOTAL_VS_END, val);
		val = v_VAEP(vid->vl_vspw + vid->vl_vbpd + vid->vl_row) |
		      v_VASP(vid->vl_vspw + vid->vl_vbpd);
		lcdc_writel(lcdc_dev, DSP_VACT_ST_END, val);
		msk = m_INTERLACE_DSP_EN | m_WIN1_INTERLACE_EN |
		      m_WIN0_YRGB_DEFLICK_EN | m_WIN0_CBR_DEFLICK_EN |
		      m_WIN0_INTERLACE_EN;;
		val = v_INTERLACE_DSP_EN(0) | v_WIN1_INTERLACE_EN(0) |
		      v_WIN0_YRGB_DEFLICK_EN(0) | v_WIN0_CBR_DEFLICK_EN(0) |
		      v_WIN0_INTERLACE_EN(0);
		lcdc_msk_reg(lcdc_dev, DSP_CTRL0, msk, val);

		lcdc_msk_reg(lcdc_dev, DSP_CTRL0,
			     m_INTERLACE_DSP_EN |
			     m_WIN0_YRGB_DEFLICK_EN |
			     m_WIN0_CBR_DEFLICK_EN |
			     m_INTERLACE_FIELD_POL |
			     m_WIN0_INTERLACE_EN |
			     m_WIN1_INTERLACE_EN,
			     v_INTERLACE_DSP_EN(0) |
			     v_WIN0_YRGB_DEFLICK_EN(0) |
			     v_WIN0_CBR_DEFLICK_EN(0) |
			     v_INTERLACE_FIELD_POL(0) |
			     v_WIN0_INTERLACE_EN(0) |
			     v_WIN1_INTERLACE_EN(0));

		msk = m_LF_INT_NUM;
		val = v_LF_INT_NUM(vid->vl_vspw + vid->vl_vbpd + vid->vl_row);
		lcdc_msk_reg(lcdc_dev, INT_STATUS, msk, val);
	}
	if ((lcdc_dev->soc_type == CONFIG_RK3128) &&
	    ((vid->screen_type == SCREEN_HDMI) ||
	     (vid->screen_type == SCREEN_TVOUT) ||
	     (vid->screen_type == SCREEN_TVOUT_TEST))) {
		lcdc_dev->output_color = COLOR_YCBCR;
		rk312x_lcdc_select_bcsh(lcdc_dev);
	}
	
	lcdc_cfg_done(lcdc_dev);
#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	if ((vid->screen_type == SCREEN_LVDS) ||
	    (vid->screen_type == SCREEN_RGB)) {
		rk31xx_lvds_en(vid);
	} else if ((vid->screen_type == SCREEN_MIPI)||
		   (vid->screen_type == SCREEN_DUAL_MIPI)) {
#if defined(CONFIG_RK32_DSI)
		rk32_mipi_enable(vid);
#endif
	}
#endif
	return 0;
}

/* Enable LCD and DIGITAL OUT in DSS */
void rk_lcdc_standby(int enable)
{
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;
#if defined(CONFIG_RK32_DSI)
	if ((panel_info.screen_type == SCREEN_MIPI)||
			   (panel_info.screen_type == SCREEN_DUAL_MIPI)) {
		if (enable == 0) {
			rk32_dsi_enable();
		} else if (enable == 1) {
			rk32_dsi_disable();
		}
	}
#endif
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_LCDC_STANDBY,
		     v_LCDC_STANDBY(enable ? 1 : 0));
	lcdc_cfg_done(lcdc_dev);
}


#if defined(CONFIG_OF_LIBFDT)
static int rk312x_lcdc_parse_dt(struct lcdc_device *lcdc_dev,
					const void *blob)
{
	int order = FB0_WIN0_FB1_WIN1_FB2_WIN2;

	lcdc_dev->node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_RK312X_LCDC);
	if (lcdc_dev->node < 0) {
		lcdc_dev->node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_RK3036_LCDC);
		if (lcdc_dev->node < 0) {
			debug("can't find dts node for lcdc\n");
			return -ENODEV;
		}
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

#define CPU_AXI_QOS_PRIORITY_LEVEL(h, l)        ((((h) & 3) << 2) | ((l) & 3))

int rk_lcdc_init(int lcdc_id)
{
	struct lcdc_device *lcdc_dev = &rk312x_lcdc;

	lcdc_dev->soc_type = gd->arch.chiptype;
#ifdef CONFIG_OF_LIBFDT
	if (!lcdc_dev->node)
		rk312x_lcdc_parse_dt(lcdc_dev, gd->fdt_blob);
#endif
	if (lcdc_dev->node <= 0) {
		if (lcdc_dev->soc_type == CONFIG_RK3036) {
			lcdc_dev->regs = 0x10118000;
		} else {
			lcdc_dev->regs = 0x1010e000;
		}
	}
	if (lcdc_dev->soc_type == CONFIG_RK3036) {
		writel(CPU_AXI_QOS_PRIORITY_LEVEL(3, 3), 0x1012f008);
	} else {
		writel(CPU_AXI_QOS_PRIORITY_LEVEL(3, 3), 0x1012f188);
	}
	lcdc_msk_reg(lcdc_dev, SYS_CTRL, m_AUTO_GATING_EN |
		      m_LCDC_STANDBY | m_DMA_STOP, v_AUTO_GATING_EN(0)|
		      v_LCDC_STANDBY(0)|v_DMA_STOP(0));
	lcdc_msk_reg(lcdc_dev, AXI_BUS_CTRL, m_MMU_EN, v_MMU_EN(0));
	lcdc_cfg_done(lcdc_dev);

	return 0;
}

