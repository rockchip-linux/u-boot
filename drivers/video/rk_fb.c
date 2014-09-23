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

#include <config.h>
#include <common.h>
#include <errno.h>
#include <lcd.h>
#include <fdtdec.h>
#include <bmp_logo.h>


DECLARE_GLOBAL_DATA_PTR;

static unsigned int panel_width, panel_height;
void *g_lcdbase=NULL;

static void lcd_panel_on(vidinfo_t *vid)
{
	if (vid->lcd_power_on)
		vid->lcd_power_on();
	udelay(vid->power_on_delay);

	if (vid->mipi_power)
		vid->mipi_power();

	if (vid->backlight_on)
		vid->backlight_on(50);
}


#ifdef CONFIG_OF_LIBFDT
int rk_fb_parse_dt(const void *blob)
{
	int node;
	void *handle;
	node = fdtdec_next_compatible(blob, 0, COMPAT_ROCKCHIP_FB);
	panel_info.logo_on = fdtdec_get_int(blob, node, "rockchip,uboot-logo-on", 0);
	handle = fdt_getprop_u32_default(blob, "/display-timings", "native-mode", -1);
	node = fdt_node_offset_by_phandle(blob, handle);
	if (node <= 0) {
		debug("rk_fb: Can't get device node for display-timings\n");
		return -19;
	}
	/* Panel infomation */

	panel_info.vl_bpix = 4;

	panel_info.lvds_ttl_en  = 0,  // rk32 lvds ttl enable

	panel_info.screen_type = fdtdec_get_int(blob, node,"screen-type", -1);
	panel_info.lcd_face = fdtdec_get_int(blob, node,"out-face", -1);
	if (panel_info.lcd_face == (uchar)-1) {
		debug("Can't get out-face set to OUT_D888_P666 for default\n");
		panel_info.lcd_face = OUT_D888_P666;
	}

	panel_info.vl_col = fdtdec_get_int(blob, node, "hactive", 0);
	if (panel_info.vl_col == 0) {
		debug("Can't get hactive set to 1280 for default\n");
		panel_info.vl_col = 1280;
	}

	panel_info.vl_row = fdtdec_get_int(blob, node, "vactive", 0);
	if (panel_info.vl_row == 0) {
		debug("Can't get vactive set to 800 for default\n");
		panel_info.vl_row = 800;
	}

	panel_info.vl_width = fdtdec_get_int(blob, node,
						"hactive", 0);

	panel_info.vl_height = fdtdec_get_int(blob, node,
						"vactive", 0);

	panel_info.vl_freq = fdtdec_get_int(blob, node, "clock-frequency", 0);
	if (panel_info.vl_freq == 0) {
		debug("Can't get clock-frequency set to 71Mhz for default\n");
		panel_info.vl_freq = 71000000;
	}

	panel_info.vl_oep = fdtdec_get_int(blob, node, "de-active", -1);
	if (panel_info.vl_oep == (u_char)-1) {
		debug("Can't get de-active set to 0 for default\n");
		panel_info.vl_oep = 0;
	}

	panel_info.vl_hsp = fdtdec_get_int(blob, node, "hsync-active", -1);
	if (panel_info.vl_hsp == (u_char)-1) {
		debug("Can't get hsync-active, set to 0 for default\n");
		panel_info.vl_hsp = 0;
	}

	panel_info.vl_vsp = fdtdec_get_int(blob, node, "vsync-active", -1);
	if (panel_info.vl_vsp == (u_char)-1) {
		debug("Can't get vsync-active, set to 0 for default\n");
		panel_info.vl_vsp = 0;
	}

	panel_info.lvds_format = fdtdec_get_int(blob, node, "lvds-format", -1);
	if (panel_info.lvds_format == (u_char)-1) {
		debug("Can't get lvds-format, set to LVDS_8BIT_2 for default\n");
		panel_info.lvds_format = LVDS_8BIT_2;
	}

	panel_info.vl_swap_rb = fdtdec_get_int(blob, node, "swap-rb", -1);
	if (panel_info.vl_swap_rb == (u_char)-1) {
		debug("Can't get swap-rb, set to 0 for default\n");
		panel_info.vl_swap_rb = 0;
	}

	panel_info.vl_hspw = fdtdec_get_int(blob, node, "hsync-len", 0);
	if (panel_info.vl_hspw == 0) {
		debug("Can't get hsync-len, use 10 to default\n");
		panel_info.vl_hspw = 10;
	}

	panel_info.vl_hfpd = fdtdec_get_int(blob, node, "hfront-porch", 0);
	if (panel_info.vl_hfpd == 0) {
		debug("Can't get hfront-porch, use 18 to default\n");
		panel_info.vl_hfpd = 18;
	}

	panel_info.vl_hbpd = (u_char)fdtdec_get_int(blob, node,	"hback-porch", 0);
	if (panel_info.vl_hbpd == 0) {
		debug("Can't get hback-porch use 100 to default\n");
		panel_info.vl_hbpd = 100;
	}

 	panel_info.vl_vspw = (u_char)fdtdec_get_int(blob, node, "vsync-len", 0);
	if (panel_info.vl_vspw == 0) {
		debug("Can't get vsync-len, use 2 to default\n");
		panel_info.vl_vspw = 2;
	}

	panel_info.vl_vfpd = fdtdec_get_int(blob, node, "vfront-porch", 0);
	if (panel_info.vl_vfpd == 0) {
		debug("Can't get vfront-porch, use 6 to default\n");
		panel_info.vl_vfpd = 6;
	}

	panel_info.vl_vbpd = fdtdec_get_int(blob, node, "vback-porch", 0);
	if (panel_info.vl_vbpd == 0) {
		debug("Can't get vback-porch, use 8 to default\n");
		panel_info.vl_vbpd = 8;
	}

	panel_info.lcdc_id = 0;
	node = fdt_path_offset(blob, "lcdc1");
	if(PRMRY == fdtdec_get_int(blob, node, "rockchip,prop", 0)) {
		debug("lcdc1 is the prmry lcd controller\n");
		panel_info.lcdc_id = 1;
	}

    	printf("read lcd timing from dts!\nlogo_on=%d,lcd_face=0x%x,vl_col=%d,vl_row=%d,vl_freq = %d,lvds_format=%d,lcdc_id=%d\n",
           panel_info.logo_on, panel_info.lcd_face,panel_info.vl_col, panel_info.vl_row,panel_info.vl_freq,panel_info.lvds_format,panel_info.lcdc_id);

	return 0;
}
#endif /* CONFIG_OF_LIBFDT */

int lcd_get_size(int *line_length)
{
	// printf("rk %s\n",__func__);
	return CONFIG_LCD_MAX_WIDTH * CONFIG_LCD_MAX_HEIGHT * 2;
}

extern int g_hdmi_noexit;

void lcd_ctrl_init(void *lcdbase)
{
	printf("%s [%d]\n",__FUNCTION__,__LINE__);
	/* initialize parameters which is specific to panel. */

#ifdef CONFIG_OF_LIBFDT
	rk_fb_parse_dt(gd->fdt_blob);
#endif

#ifdef CONFIG_RK_3288_HDMI
	rk3288_hdmi_probe(&panel_info);
#endif
#ifdef CONFIG_RK_3036_HDMI
	rk3036_hdmi_probe(&panel_info);
#endif
#ifdef CONFIG_RK3036_TVE
	if(g_hdmi_noexit == 1)
	{
		rk3036_tve_init(&panel_info);
	}
#endif
	init_panel_info(&panel_info);
	if (panel_info.enable_ldo)
		panel_info.enable_ldo(1);
	udelay(panel_info.init_delay);

	panel_width = panel_info.vl_width;
	panel_height = panel_info.vl_height;
	g_lcdbase = lcdbase;

	panel_info.real_freq = rkclk_lcdc_clk_set(panel_info.lcdc_id, panel_info.vl_freq);

	rk_lcdc_init(panel_info.lcdc_id);
	rk30_load_screen(&panel_info);
}

void lcd_enable(void)
{
	if (panel_info.logo_on) {
		rk30_lcdc_set_par(&panel_info.par[0].fb_info, &panel_info);
	}

	lcd_panel_on(&panel_info);
}

void lcd_pandispaly(struct fb_dsp_info *info)
{
	if (panel_info.logo_on) {
		rk30_lcdc_set_par(info, &panel_info);
	}
}

void lcd_standby(int enable)
{
	rk30_lcdc_standby(enable);
}

/* dummy function */
void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
	return;
}

