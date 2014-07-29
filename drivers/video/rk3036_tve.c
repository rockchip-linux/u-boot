#include <common.h>
#include <asm/io.h>
#include <asm/arch-rk32xx/gpio-rk3036.h>
#include <asm/arch-rk32xx/grf-rk3036.h>
#include <asm/arch-rk32xx/io-rk3036.h>

#include <linux/fb.h>
#include <i2c.h>
#include"rk3036_tve.h"


#define tve_writel(offset, v)	writel(v, RK30_TVE_REGBASE  + offset)
#define tve_readl(offset)	readl(RK30_TVE_REGBASE + offset)

static const struct fb_videomode rk3036_cvbs_mode [] = {
	/*name	refresh	xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw			polariry				PorI		flag*/
	{"NTSC",60,	720,	480,	27000000,	57,	19,	19,	0,	62,	3,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	FB_VMODE_INTERLACED,	0},
	{"PAL",	50,	720,	576,	27000000,	69,	12,	19,	2,	63,	3,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	FB_VMODE_INTERLACED,	0},
};


#define TVE_REG_NUM 0x300

int rk3036_tve_show_reg()
{
	int i = 0;
	u32 val = 0;

	printf("\n>>>rk3036_tve reg");

	printf("\n\n\n\n--------------------------%s------------------------------------",__FUNCTION__);

	for (i = 0; i <= TVE_REG_NUM; i++) {
		val = readl(0x10118000 + i * 0x04);
		if (i % 4 == 0)
			printf("\n%8x:", i*0x04 + 0x10118000); 
		printf(" %08x", val);
	}   
	printf("\n-----------------------------------------------------------------\n");

	return 0;
}


static void dac_enable(int enable)
{
	u32 mask, val;
	
//	printf("%s enable %d\n", __FUNCTION__, enable);
	
	if(enable) {
		mask = m_VBG_EN | m_DAC_EN;
		val = mask;
	} else {
		mask = m_VBG_EN | m_DAC_EN;
		val = 0;
	}
	grf_writel((mask << 16) | val,GRF_SOC_CON3);

}

static void tve_set_mode (int mode)
{
//	printf("%s mode %d\n", __FUNCTION__, mode);
	
	tve_writel(TV_RESET, v_RESET(1));
	udelay(100);
	tve_writel(TV_RESET, v_RESET(0));
	
	tve_writel(TV_CTRL, v_CVBS_MODE(mode) | v_CLK_UPSTREAM_EN(2) | 
			v_TIMING_EN(2) | v_LUMA_FILTER_GAIN(0) | 
			v_LUMA_FILTER_UPSAMPLE(0) | v_CSC_PATH(0) );
	tve_writel(TV_LUMA_FILTER0, 0x02ff0000);
	tve_writel(TV_LUMA_FILTER1, 0xF40202fd);
	tve_writel(TV_LUMA_FILTER2, 0xF332d919);
	
	if(mode == TVOUT_CVBS_NTSC) {
		tve_writel(TV_ROUTING, v_DAC_SENSE_EN(0) | v_Y_IRE_7_5(1) | 
			v_Y_AGC_PULSE_ON(1) | v_Y_VIDEO_ON(1) | 
			v_Y_SYNC_ON(1) | v_PIC_MODE(mode));
		tve_writel(TV_BW_CTRL, v_CHROMA_BW(BP_FILTER_NTSC) | v_COLOR_DIFF_BW(COLOR_DIFF_FILTER_BW_1_3));
		tve_writel(TV_SATURATION, 0x0052543C);
		tve_writel(TV_BRIGHTNESS_CONTRAST, 0x00008300);
		
		tve_writel(TV_FREQ_SC,	0x21F07BD7);
		tve_writel(TV_SYNC_TIMING, 0x00C07a81);
		tve_writel(TV_ADJ_TIMING, 0x96B40000);
		tve_writel(TV_ACT_ST,	0x001500D6);
		tve_writel(TV_ACT_TIMING, 0x169800FC);

	} else if (mode == TVOUT_CVBS_PAL) {
		tve_writel(TV_ROUTING, v_DAC_SENSE_EN(0) | v_Y_IRE_7_5(0) | 
			v_Y_AGC_PULSE_ON(1) | v_Y_VIDEO_ON(1) | 
			v_Y_SYNC_ON(1) | v_CVBS_MODE(mode));
		tve_writel(TV_BW_CTRL, v_CHROMA_BW(BP_FILTER_PAL) | v_COLOR_DIFF_BW(COLOR_DIFF_FILTER_BW_1_3));
		tve_writel(TV_SATURATION, 0x002e553c);
		tve_writel(TV_BRIGHTNESS_CONTRAST, 0x00007579);
		
		tve_writel(TV_FREQ_SC,	0x2A098ACB);
		tve_writel(TV_SYNC_TIMING, 0x00C28381);
		tve_writel(TV_ADJ_TIMING, 0xB6C00880);
		tve_writel(TV_ACT_ST,	0x001500F6);
		tve_writel(TV_ACT_TIMING, 0x2694011D);
	}
}

#if defined(CONFIG_RK_FB)
static void rk3036_tve_init_panel(vidinfo_t *panel)
{
	const struct fb_videomode *mode = NULL;
	
	mode = &rk3036_cvbs_mode[TVOUT_DEAULT];

	panel->screen_type = SCREEN_TVOUT; 
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
	printf("panel->vmode = %d \n", panel->vmode);
}
#endif


int rk3036_tve_init(vidinfo_t *panel)
{
	rk3036_tve_init_panel(panel);

	dac_enable(0);
	tve_set_mode(TVOUT_DEAULT);
	dac_enable(1);

//	rk3036_tve_show_reg();
}
