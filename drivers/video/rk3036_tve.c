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
#include <linux/fb.h>
#include <asm/arch/rkplat.h>
#include "rockchip_fb.h"
#include "rk3036_tve.h"
#include <../board/rockchip/common/config.h>

DECLARE_GLOBAL_DATA_PTR;

int g_tve_pos = -1;

static struct rk3036_tve tve_s;

#define tve_writel(offset, v)	writel(v, tve_s.reg_phy_base  + offset)
#define tve_readl(offset)	readl(tve_s.reg_phy_base + offset)

struct fb_videomode rk3036_cvbs_mode[MAX_TVE_COUNT] = {
	/*name	refresh	xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw			polariry				PorI		flag*/
	{"NTSC",	60,	720,	480,	27000000,	43,	33,	19,	0,	62,	3,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	FB_VMODE_INTERLACED,	0},
	{"PAL",		50,	720,	576,	27000000,	52,	29,	19,	2,	63,	3,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	FB_VMODE_INTERLACED,	0},
};


#define TVE_REG_NUM 0x300

int rk3036_tve_show_reg()
{
	int i = 0;
	u32 val = 0;

	printf("\n>>>rk3036_tve reg");

	printf("\n\n\n\n--------------------------%s------------------------------------",__FUNCTION__);

	for (i = 0; i <= TVE_REG_NUM; i++) {
		val = readl(tve_s.reg_phy_base + i * 0x04);
		if (i % 4 == 0)
			printf("\n%8x:", i*0x04 + tve_s.reg_phy_base - 0x200); 
		printf(" %08x", val);
	}   
	printf("\n-----------------------------------------------------------------\n");

	return 0;
}


static void dac_enable(int enable)
{
	u32 mask, val;
	u32 grfreg = 0;

//	printf("%s enable %d\n", __FUNCTION__, enable);
	if (enable) {
		mask = m_VBG_EN | m_DAC_EN | m_DAC_GAIN;
		#if defined(CONFIG_RKCHIP_RK3128)
			val = m_VBG_EN | m_DAC_EN | v_DAC_GAIN(0x3a);
			grfreg = GRF_TVE_CON0;
		#elif defined(CONFIG_RKCHIP_RK3036)
			val = m_VBG_EN | m_DAC_EN | v_DAC_GAIN(0x3e);
			grfreg = GRF_SOC_CON3;
		#endif
		val |= mask << 16;
	} else {
		mask = m_VBG_EN | m_DAC_EN;
		val = 0;
		#if defined(CONFIG_RKCHIP_RK3128)
			grfreg = GRF_TVE_CON0;
		#elif defined(CONFIG_RKCHIP_RK3036)
			grfreg = GRF_SOC_CON3;
		#endif
		val |= mask << 16;
	}
	if (grfreg) {
//		printf("grfreg 0x%x mask 0x%x val 0x%x\n", grfreg, mask, val);
		writel(val, RKIO_GRF_PHYS + grfreg);
//		grf_writel(grfreg, val);
	}
//	printf("%s enable %d end\n", __FUNCTION__, enable);
}

static void tve_set_mode (int mode)
{
//	printf("%s mode %d\n", __FUNCTION__, mode);
	
	tve_writel(TV_RESET, v_RESET(1));
	udelay(100);
	tve_writel(TV_RESET, v_RESET(0));

	if (tve_s.soctype == SOC_RK3036)
		tve_writel(TV_CTRL, v_CVBS_MODE(mode) | v_CLK_UPSTREAM_EN(2) |
			   v_TIMING_EN(2) | v_LUMA_FILTER_GAIN(0) |
			   v_LUMA_FILTER_UPSAMPLE(1) | v_CSC_PATH(0));
	else
		tve_writel(TV_CTRL, v_CVBS_MODE(mode) | v_CLK_UPSTREAM_EN(2) |
			   v_TIMING_EN(2) | v_LUMA_FILTER_GAIN(0) |
			   v_LUMA_FILTER_UPSAMPLE(1) | v_CSC_PATH(3));
	tve_writel(TV_LUMA_FILTER0, 0x02ff0000);
	tve_writel(TV_LUMA_FILTER1, 0xF40202fd);
	tve_writel(TV_LUMA_FILTER2, 0xF332d919);
	
	if(mode == TVOUT_CVBS_NTSC) {
		tve_writel(TV_ROUTING, v_DAC_SENSE_EN(0) | v_Y_IRE_7_5(1) | 
			v_Y_AGC_PULSE_ON(1) | v_Y_VIDEO_ON(1) | 
			v_Y_SYNC_ON(1) | v_PIC_MODE(mode));
		tve_writel(TV_BW_CTRL, v_CHROMA_BW(BP_FILTER_NTSC) | v_COLOR_DIFF_BW(COLOR_DIFF_FILTER_BW_1_3));
		tve_writel(TV_SATURATION, 0x0052543C);
		if(tve_s.test_mode)
		tve_writel(TV_BRIGHTNESS_CONTRAST, 0x00008300);
		else
		tve_writel(TV_BRIGHTNESS_CONTRAST, 0x00007900);
		
		tve_writel(TV_FREQ_SC,	0x21F07BD7);
		tve_writel(TV_SYNC_TIMING, 0x00C07a81);
		tve_writel(TV_ADJ_TIMING, 0x96B40000);
		tve_writel(TV_ACT_ST,	0x001500D6);
		tve_writel(TV_ACT_TIMING, 0x169800FC);

	} else if (mode == TVOUT_CVBS_PAL) {
		tve_writel(TV_ROUTING, v_DAC_SENSE_EN(0) | v_Y_IRE_7_5(0) |
			v_Y_AGC_PULSE_ON(0) | v_Y_VIDEO_ON(1) |
			v_YPP_MODE(1) | v_Y_SYNC_ON(1) | v_PIC_MODE(mode));
		tve_writel(TV_BW_CTRL, v_CHROMA_BW(BP_FILTER_PAL) |
			v_COLOR_DIFF_BW(COLOR_DIFF_FILTER_BW_1_3));
		if (tve_s.soctype == SOC_RK312X) {
			if(tve_s.saturation != 0)
			tve_writel(TV_SATURATION, tve_s.saturation);
			else
			tve_writel(TV_SATURATION, /*0x00325c40*/ 0x002b4d3c);
			if(tve_s.test_mode)
			tve_writel(TV_BRIGHTNESS_CONTRAST, 0x00008a0a);
			else
			tve_writel(TV_BRIGHTNESS_CONTRAST, 0x0000800a);
		} else {
			tve_writel(TV_SATURATION, /*0x00325c40*/ 0x00386346);
			tve_writel(TV_BRIGHTNESS_CONTRAST, 0x00008b00);
		}

		tve_writel(TV_FREQ_SC,	0x2A098ACB);
		tve_writel(TV_SYNC_TIMING, 0x00C28381);
		tve_writel(TV_ADJ_TIMING, (0xc << 28) | 0x06c00800 | 0x80);
		tve_writel(TV_ACT_ST,	0x001500F6);
		tve_writel(TV_ACT_TIMING, 0x0694011D | (1 << 12) | (2 << 28));
		if (tve_s.soctype == SOC_RK312X) {
			tve_writel(TV_ADJ_TIMING, (0xa << 28) | 0x06c00800 | 0x80);
			udelay(100);
			tve_writel(TV_ADJ_TIMING, (0xa << 28) | 0x06c00800 | 0x80);
			tve_writel(TV_ACT_TIMING, 0x0694011D | (1 << 12) | (2 << 28));
		} else {
			tve_writel(TV_ADJ_TIMING, (0xa << 28) | 0x06c00800 | 0x80);
		}
	}
}

#if defined(CONFIG_RK_FB)
static void rk3036_tve_init_panel(vidinfo_t *panel)
{
	const struct fb_videomode *mode = NULL;

	if (g_tve_pos ==1)
		mode = &rk3036_cvbs_mode[g_tve_pos];
	else if (g_tve_pos == 0)
		mode = &rk3036_cvbs_mode[g_tve_pos];
	else
		mode = &rk3036_cvbs_mode[TVOUT_DEAULT];

	if (tve_s.test_mode) {
		panel->screen_type = SCREEN_TVOUT_TEST;
		printf("SCREEN_TVOUT_TEST\n");
	} else {
		panel->screen_type = SCREEN_TVOUT;
		printf("SCREEN_TVOUT\n");
	}

	panel->vl_freq    = mode->pixclock; 
	panel->vl_col     = mode->xres ;//xres
	panel->vl_row     = mode->yres;//yres
	panel->vl_width   = mode->xres;
	panel->vl_height  = mode->yres;
	//sync polarity
	panel->vl_clkp = 1;
	if(FB_SYNC_HOR_HIGH_ACT & mode->sync)
		panel->vl_hsp  = 1;
	else
		panel->vl_hsp  = 0;
	if(FB_SYNC_VERT_HIGH_ACT & mode->sync)
		panel->vl_vsp  = 1;
	else
		panel->vl_vsp  = 0;

	//h
	panel->vl_hspw = mode->hsync_len;
	panel->vl_hbpd = mode->left_margin;
	panel->vl_hfpd = mode->right_margin;
	//v
	panel->vl_vspw = mode->vsync_len;
	panel->vl_vbpd = mode->upper_margin;
	panel->vl_vfpd = mode->lower_margin;
	panel->pixelrepeat = 1;
	panel->interface_mode = 1;
	panel->vmode= 1;
/*
	printf("%s:panel->lcd_face=%d\n panel->vl_freq=%d\n panel->vl_col=%d\n panel->vl_row=%d\n panel->vl_width=%d\n panel->vl_height=%d\n panel->vl_clkp=%d\n panel->vl_hsp=%d\n panel->vl_vsp=%d\n panel->vl_bpix=%d\n panel->vl_swap_rb=%d\n panel->vl_hspw=%d\n panel->vl_hbpd=%d\n panel->vl_hfpd=%d\n panel->vl_vspw=%d\n panel->vl_vbpd=%d\n panel->vl_vfpd=%d\n",

	__func__,
	
	panel->lcd_face,
	panel->vl_freq,
	panel->vl_col,
	panel->vl_row,
	panel->vl_width,
	panel->vl_height,
	panel->vl_clkp,
	panel->vl_hsp,
	panel->vl_vsp,
	panel->vl_bpix,
       panel->vl_swap_rb,
	
	panel->vl_hspw,//Panel infomation 
	panel->vl_hbpd,
	panel->vl_hfpd,

	panel->vl_vspw,
	panel->vl_vbpd,
	panel->vl_vfpd);
*/
	//printf("panel->vmode = %d \n", panel->vmode);
}
#endif


int rk3036_tve_init(vidinfo_t *panel)
{
	int val = 0;
	int node = 0;

#if defined(CONFIG_RKCHIP_RK3036)
	tve_s.reg_phy_base = 0x10118000 + 0x200;
	tve_s.soctype = SOC_RK3036;
//	printf("%s start soc is 3036\n", __func__);
#elif defined(CONFIG_RKCHIP_RK3128)
	tve_s.reg_phy_base = 0x1010e000 + 0x200;
	tve_s.soctype = SOC_RK312X;
	tve_s.saturation = 0;

	if (gd->fdt_blob)
	{
		node = fdt_node_offset_by_compatible(gd->fdt_blob,
			0, "rockchip,rk312x-tve");
		if (node < 0) {
			printf("can't find dts node for rk312x-tve\n");
			return -ENODEV;
		}

		if (!fdt_device_is_available(gd->fdt_blob, node)) {
			printf("rk312x-tve is disabled\n");
			return -EPERM;
		}

		tve_s.test_mode = fdtdec_get_int(gd->fdt_blob, node, "test_mode", 0);
		tve_s.saturation = fdtdec_get_int(gd->fdt_blob, node, "saturation", 0);

	}
	printf("test_mode=%d,saturation=0x%x\n", tve_s.test_mode, tve_s.saturation);
//	printf("%s start soc is 3128\n", __func__);
#endif

	rk3036_tve_init_panel(panel);

	if(g_tve_pos < 0)
	{
		g_tve_pos = TVOUT_DEAULT;
		printf("%s:use default config g_tve_pos = %d \n", __func__,g_tve_pos);
	}
	else
	{
		printf("%s:use baseparamer config g_tve_pos = %d \n", __func__,g_tve_pos);
	}

	if(g_tve_pos < 0)
	{
		g_tve_pos = TVOUT_DEAULT;
		printf("%s:use default config g_tve_pos = %d \n", __func__,g_tve_pos);
	}
	else
	{
		printf("%s:use baseparamer config g_tve_pos = %d \n", __func__,g_tve_pos);
	}

	dac_enable(0);
	tve_set_mode(g_tve_pos);
	dac_enable(1);


//	rk3036_tve_show_reg();
}
