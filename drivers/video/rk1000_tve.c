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
#include "rk1000_tve.h"
#include <i2c.h>

#define MAX_I2C_RETRY  3
#define I2C_ADDRESS    0x40
#define TV_I2C_ADDRESS 0x42

#ifdef CONFIG_RKCHIP_RK3368
static int bus_num = 1;
#endif

#ifdef CONFIG_RKCHIP_RK3288
static int bus_num = 4;
#endif

int g_tve_pos = -1;
#if  defined(CONFIG_OF_LIBFDT)
struct fdt_gpio_state rst_gpios;
#endif

struct fb_videomode rk1000_cvbs_mode[MAX_TVE_COUNT] = {
	/*name	refresh	xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw			polariry				PorI		flag*/
	//{"NTSC",	60,	720,	480,	27000000,	43,	33,	19,	0,	62,	3,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	FB_VMODE_INTERLACED,	0},
	//{"PAL",		50,	720,	576,	27000000,	52,	29,	19,	2,	63,	3,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	FB_VMODE_INTERLACED,	0},
	{"NTSC", 60, 720, 480, 27000000, 116, 16, 25, 14, 6, 6, 0, 1, 0},
	{"PAL", 50, 720, 576, 27000000, 126, 12, 37, 6, 6, 6, 0, 1, 0},
};


int rk1000_read_reg(int i2c_addr, int reg)
{
	int	i;
	uchar	data;
	int	retval = -1;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_read(i2c_addr, reg, 1, &data, 1)) {
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

int rk1000_write_reg(int i2c_addr, int reg, uchar *data, uint len)
{
	int	i;
	int	retval = -1;

	for (i = 0; i < MAX_I2C_RETRY; ++i) {
		if (!i2c_write(i2c_addr, reg, 1, data, len)) {
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
void rk1000_tve_init_panel(vidinfo_t *panel)
{
	const struct fb_videomode *mode = NULL;

	if (g_tve_pos ==1)
		mode = &rk1000_cvbs_mode[g_tve_pos];
	else if (g_tve_pos == 0)
		mode = &rk1000_cvbs_mode[g_tve_pos];
	else {
		mode = &rk1000_cvbs_mode[TVOUT_DEAULT];
		g_tve_pos = TVOUT_DEAULT;
	}

	
	panel->screen_type = SCREEN_RGB;//SCREEN_TVOUT;
	panel->color_mode=0;///COLOR_RGB;
	panel->lcd_face =OUT_P888;
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
	panel->vmode= 0;

}
#endif
static int rk1000_parse_dt(const void* blob)
{
	int node;

	node = fdt_node_offset_by_compatible(blob,
					0, "rockchip,rk1000_control");
	if (node < 0) {
		printf("can't find dts node for rk1000\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,node)) {
		printf("device rk1000 is disabled\n");
		return -1;
	}
		
	fdtdec_decode_gpio(blob, node, "gpio-reset", &rst_gpios);

	printf("-----%d---%d---\n",rst_gpios.gpio,rst_gpios.flags);
	
	return 0;
}

int rk1000_tve_init(vidinfo_t *panel)
{
	//rk1000_tve_init_panel(panel);
	//PAL
	unsigned char tv_encoder_regs_pal[] = {0x06, 0x00, 0x00, 0x03, 0x00, 0x00};
	unsigned char tv_encoder_control_regs_pal[] = {0x41, 0x01};

	//NTSC
	unsigned char Tv_encoder_regs_ntsc[] = {0x00, 0x00, 0x00, 0x03, 0x00, 0x00};
        unsigned char Tv_encoder_control_regs_ntsc[] = {0x43, 0x01};

	#if  defined(CONFIG_OF_LIBFDT)
	debug("rk fb parse dt start.\n");
	int ret = rk1000_parse_dt(gd->fdt_blob);
	if (ret < 0){
		printf("parse rk1000 dts error!\n");
		return -1;
	}

	#endif

    
	//init i2c
	i2c_set_bus_num(bus_num);
	i2c_init(200*1000, 1);

	/*for rk3368*/
	#ifdef  CONFIG_RKCHIP_RK3368
	grf_writel(0xffff1500, GRF_GPIO2C_IOMUX);
	cru_writel(cru_readl(0x16c)|0x80008000,0x16c);
	#endif
	/*for rk3288*/
	#ifdef	CONFIG_RKCHIP_RK3288
	grf_writel(0xffff1a40, GRF_SOC_CON7);
	grf_writel(grf_readl(GRF_GPIO7CL_IOMUX)|(1<<4)|(1<<8)|(1<<20)|(1<<24), GRF_GPIO7CL_IOMUX);
	grf_writel(grf_readl(GRF_GPIO6B_IOMUX)|(1<<0)|(1<<16), GRF_GPIO6B_IOMUX);
	writel(0x00071f1f,0xff890008);
	#endif

	//gpio_direction_output((GPIO_BANK0 | GPIO_A1),0);
	gpio_direction_output(rst_gpios.gpio,!rst_gpios.flags);
	mdelay(100);
	
	//gpio_direction_output((GPIO_BANK0 | GPIO_A1),1);
	gpio_direction_output(rst_gpios.gpio,rst_gpios.flags);

	/* reg[0x00] = 0x88, --> ADC_CON
	   reg[0x01] = 0x0d, --> CODEC_CON
	   reg[0x02] = 0x22, --> I2C_CON
	   reg[0x03] = 0x00, --> TVE_CON
	 */
	char data[4] = {0x88, 0x00, 0x22, 0x00};
	rk1000_write_reg(0x40, 0, (uchar*)data, 4);
	
	//rk1000 power down output dac
	data[0] = 0x07;
	rk1000_write_reg(0x42, 0x03, (uchar*)data, 1);

	if (g_tve_pos ==1) 
	{
		//rk1000 tv pal init
		rk1000_write_reg(0x42, 0, tv_encoder_regs_pal, sizeof(tv_encoder_regs_pal));
		rk1000_write_reg(0x40, 3, tv_encoder_control_regs_pal, sizeof(tv_encoder_control_regs_pal));
	}
	else
	{	
		//rk1000 tv ntsc init
		rk1000_write_reg(0x42, 0, Tv_encoder_regs_ntsc, sizeof(Tv_encoder_regs_ntsc));
		rk1000_write_reg(0x40, 3, Tv_encoder_control_regs_ntsc, sizeof(Tv_encoder_control_regs_ntsc));		
	
	}

	return 0;
}


