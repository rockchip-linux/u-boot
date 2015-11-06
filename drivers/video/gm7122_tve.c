/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <lcd.h>
#include <linux/fb.h>
#include <asm/arch/rkplat.h>
#include "rockchip_fb.h"
#include <../board/rockchip/common/config.h>
#include "gm7122_tve.h"
#include <i2c.h>

#define MAX_I2C_RETRY  3
#define I2C_ADDRESS    0x44
#define PMUGRF_BASE         0xff738000
struct gm7122tve  gm7122_tve;
static int bus_num = 1;

int g_tve_pos = -1;


struct fb_videomode cvbs_mode[MAX_TVE_COUNT] = {
	/*name	refresh	xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw			polariry				PorI		flag*/	
	{"NTSC", 60, 720, 480, 27000000, 57, 19, 15, 4, 62, 3, 0, 1, 0},		
	{"PAL", 50, 720, 576, 27000000, 62, 14, 17, 2, 68, 5, 0, 1, 0},
};


int tve_readl(int reg)
{
	int	i;
	uchar	data;
	int	retval = -1;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_read(I2C_ADDRESS, reg, 1, &data, 1)) {
			retval = (int)data;
			goto exit;
		}

		udelay(100);
	}

exit:
	//printf("pmu_read %x=%x\n", reg, retval);
	if (retval < 0)
		printf("%s: failed to read register %#x: %d\n", __func__, reg,
		      retval);
	return retval;
}

int tve_writel(int reg, uchar data)
{
	int	i;
	int	retval = -1;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_write(I2C_ADDRESS, reg, 1, &data, 1)) {
			retval = 0;
			goto exit;
		}
		//printf("rk1000_write_reg:%d\n",i);

		udelay(100);
	}

exit:
	//printf("pmu_write %x=%x: ", reg, retval);
	//for (i = 0; i < len; i++)
	//	printf("%x ", data[i]);
	if (retval)
		printf("%s: failed to write register %#x\n", __func__, reg);
	return retval;
}

extern int num_tve_mode;

#if defined(CONFIG_RK_FB)
void gm7122_tve_init_panel(vidinfo_t *panel)
{
	const struct fb_videomode *mode = NULL;
	printf("gm7122_tve_init_panel %d\n",g_tve_pos);
	
	if (g_tve_pos ==1)
		mode = &cvbs_mode[g_tve_pos];
	else if (g_tve_pos == 0)
		mode = &cvbs_mode[g_tve_pos];
	else{
		mode = &cvbs_mode[TVOUT_DEAULT];
		g_tve_pos=TVOUT_DEAULT;
	}

	
	panel->screen_type = SCREEN_RGB;//SCREEN_TVOUT;
	panel->color_mode=1;///COLOR_RGB;
	panel->lcd_face =OUT_CCIR656;
	panel->vl_freq    = mode->pixclock; 
	panel->vl_col     = mode->xres ;//xres
	panel->vl_row     = mode->yres;//yres
	panel->vl_width   = mode->xres;
	panel->vl_height  = mode->yres;
	//sync polarity
	panel->vl_clkp = 0;
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

}
#endif
static int gm7122_parse_dt(const void* blob)
{
	int node;
	int err=0;

	node = fdt_node_offset_by_compatible(blob,
					0, "gm7122_tve");
	if (node < 0) {
		printf("can't find dts node for gm7122\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		printf("device gm7122 is disabled\n");
		return -1;
	}
		
	err=fdtdec_decode_gpio(blob, node, "gpio-reset", &gm7122_tve.io_reset);
	if(err>=0)
		gpio_direction_output(gm7122_tve.io_reset.gpio, !gm7122_tve.io_reset.flags);

	err=fdtdec_decode_gpio(blob, node, "gpio-sleep", &gm7122_tve.io_sleep);
	if(err>=0)
		gpio_direction_output(gm7122_tve.io_sleep.gpio, gm7122_tve.io_sleep.flags);
	
	return 0;
}

int gm7122_tve_init(vidinfo_t *panel)
{
	//IOmux lcdc data 12 13 14 15 16 17 18 19 become GPIO
	writel(0xffff0000,PMUGRF_BASE+0x08);	

	#if  defined(CONFIG_OF_LIBFDT)
	debug("rk fb parse dt start.\n");
	int ret = gm7122_parse_dt(gd->fdt_blob);
	if (ret < 0){
		printf("parse gm7122 dts error!\n");
		return -1;
	}

	#endif

	//init i2c
	i2c_set_bus_num(bus_num);
	i2c_init(50*1000, 1);

	//gpio_direction_output((GPIO_BANK0 | GPIO_C6),gm7122_tve.io_reset.flags);
	if (g_tve_pos == TVOUT_CVBS_NTSC) {			
		tve_writel(BURST_START, V_D0_BS0(1) | V_D0_BS5(1));		
		tve_writel(BURST_END, V_D0_BE0(1) | V_D0_BE2(1) |V_D0_BE3(1) | V_D0_BE4(1));
		tve_writel(INPUT_PORT_CTL, V_SYMP(1) | V_UV2C(1) | V_Y2C(1));		
		tve_writel(COLOR_DIFF_CTL, 0x00);		
		tve_writel(U_GAIN_CTL, V_GAINU0(1) | V_GAINU2(1) |V_GAINU3(3) | V_GAINU5(1) | V_GAINU6(1));
		tve_writel(V_GAIN_CTL, V_GAINV0(1) | V_GAINV1(1) |V_GAINV2(1) | V_GAINV3(1) | V_GAINV4(1) |V_GAINV7(1));
		tve_writel(UMSB_BLACK_GAIN,	V_BLACK1(1) | V_BLACK2(1) |V_BLACK3(1));
		tve_writel(VMSB_BLNNL_GAIN, V_BLNNL2(1) | V_BLNNL3(1) |V_BLNNL4(1));	
		tve_writel(STANDARD_CTL, V_PAL(0) | V_BIT0(1));		
		tve_writel(RTCEN_BURST_CTL, V_BSTA0(1) | V_BSTA1(1)|V_BSTA3(1) | V_BSTA4(1) | V_BSTA5(1));		
		tve_writel(SUBCARRIER0, V_FSC00(1) | V_FSC01(1)|	V_FSC02(1) | V_FSC03(1) | V_FSC04(1));		
		tve_writel(SUBCARRIER1, V_FSC10(1) | V_FSC11(1)|	V_FSC12(1) | V_FSC13(1) | V_FSC14(1));		
		tve_writel(SUBCARRIER2, V_FSC20(1) | V_FSC21(1) |V_FSC22(1) | V_FSC23(1));
		tve_writel(SUBCARRIER3, V_FSC29(1) | V_FSC24(1));		
		tve_writel(RCV_PORT_CTL, 0x00);		
		tve_writel(TRIG0_CTL, V_HTRIG0(1) |V_HTRIG2(1) | V_HTRIG4(1) |V_HTRIG5(1) | V_HTRIG6(1) | V_HTRIG7(1));
		tve_writel(TRIG1_CTL, V_VTRIG0(1) | V_VTRIG4(1) | V_HTRIG8(1) |V_HTRIG10(1));	
	} else if (g_tve_pos == TVOUT_CVBS_PAL) {		
		tve_writel(BURST_START, V_D0_BS0(1) | V_D0_BS5(1));		
		tve_writel(BURST_END, V_D0_BE0(1) | V_D0_BE2(1) |V_D0_BE3(1) | V_D0_BE4(1));
		tve_writel(INPUT_PORT_CTL, V_SYMP(1) | V_UV2C(1) | V_Y2C(1));		
		/*tve_writel(INPUT_PORT_CTL, 0x93);*/
		/*color bar for debug*/		
		tve_writel(COLOR_DIFF_CTL, V_CHPS0(1));		
		tve_writel(U_GAIN_CTL, V_GAINU1(1) | V_GAINU3(1) |	V_GAINU5(1) | V_GAINU6(1));		
		tve_writel(V_GAIN_CTL, V_GAINV0(1) | V_GAINV1(1) |	V_GAINV2(1) | V_GAINV3(1) | V_GAINV4(1) |V_GAINV7(1));
		tve_writel(UMSB_BLACK_GAIN,	V_BLACK1(1) | V_BLACK4(1));		
		tve_writel(VMSB_BLNNL_GAIN, V_BLNNL0(1) | V_BLNNL1(1) |V_BLNNL2(1) | V_BLNNL3(1) | V_BLNNL4(1));
		tve_writel(STANDARD_CTL, V_PAL(1) | V_SCBW(1));		
		tve_writel(RTCEN_BURST_CTL, V_BSTA0(1) | V_BSTA1(1)|V_BSTA3(1) | V_BSTA4(1) | V_BSTA5(1));		
		tve_writel(SUBCARRIER0, V_FSC00(1) | V_FSC01(1)|V_FSC03(1) | V_FSC06(1) | V_FSC07(1));
		tve_writel(SUBCARRIER1, V_FSC15(1) | V_FSC11(1)|V_FSC09(1));
		tve_writel(SUBCARRIER2, V_FSC19(1) | V_FSC16(1));		
		tve_writel(SUBCARRIER3, V_FSC29(1) | V_FSC27(1) | V_FSC25(1));
		tve_writel(RCV_PORT_CTL, 0x00);		
		tve_writel(TRIG0_CTL, V_HTRIG0(1) |V_HTRIG2(1) | V_HTRIG4(1) |	V_HTRIG5(1) | V_HTRIG6(1) | V_HTRIG7(1));
		tve_writel(TRIG1_CTL, V_VTRIG0(1) | V_VTRIG4(1) | V_HTRIG8(1) |V_HTRIG10(1));
	}else{
		printf("Don't surport mode!!!\n");
	}

	return 0;
	
}
