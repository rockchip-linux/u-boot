/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * Author: Andy Yan <andy.yan@rock-chips.com>
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <lcd.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <bmp_logo.h>
#include <asm/arch/rkplat.h>

#include "rockchip_fb.h"
#if defined(CONFIG_RK1000_TVE)
#include "rk1000_tve.h"
#endif
#if defined(CONFIG_RK3036_TVE)
#include "rk3036_tve.h"
#endif
#if defined(CONFIG_GM7122_TVE)
#include "gm7122_tve.h"
#endif


DECLARE_GLOBAL_DATA_PTR;

#define COMPAT_ROCKCHIP_FB		"rockchip,rk-fb"

#define GPIO		0
#define REGULATOR	1

struct pwr_ctr {
	char name[32];
	int type;
	int is_rst;
	struct fdt_gpio_state gpio;
	int atv_val;
	char rgl_name[32];
	int volt;
	int delay;
};

struct rk_fb_pwr_ctr_list {
	struct list_head list;
	struct pwr_ctr pwr_ctr;
};

struct rockchip_fb rockchip_fb;

vidinfo_t panel_info = {
#ifdef CONFIG_OF_LIBFDT
        .vl_col         = CONFIG_LCD_MAX_WIDTH,
        .vl_row         = CONFIG_LCD_MAX_HEIGHT,
        .vl_bpix        = 4,    /* Bits per pixel, 2^5 = 32 */
#else
        .lcd_face       = OUT_D888_P666,
        .vl_freq        = 71000000,
        .vl_col         = 1280,
        .vl_row         = 800,
        .vl_width       = 1280,
        .vl_height      = 800,
        .vl_clkp        = 0,
        .vl_hsp         = 0,
        .vl_vsp         = 0,
        .vl_bpix        = 4,    /* Bits per pixel, 2^5 = 32 */
        .vl_swap_rb     = 0,
        .lvds_format    = LVDS_8BIT_2,
        .lvds_ttl_en    = 0,  // rk32 lvds ttl enable
        /* Panel infomation */
        .vl_hspw        = 10,
        .vl_hbpd        = 100,
        .vl_hfpd        = 18,

        .vl_vspw        = 2,
        .vl_vbpd        = 8,
        .vl_vfpd        = 6,

        .lcd_power_on   = NULL,
        .mipi_power     = NULL,

        .init_delay     = 0,
        .power_on_delay = 0,
        .reset_delay    = 0,
        .pixelrepeat    = 0,
        .overscan       = 100,
        .left           = 100,
        .top            = 100,
        .right          = 100,
        .bottom         = 100,
#endif
};

int rk_fb_vidinfo_to_win(struct fb_dsp_info *fb_info, struct rk_lcdc_win *win)
{
        win->area[0].format = fb_info->format;//is config format;
        win->area[0].xact   = fb_info->xact;
        win->area[0].yact   = fb_info->yact;
        win->area[0].xsize  = fb_info->xsize;
        win->area[0].ysize  = fb_info->ysize;
        win->area[0].xpos   = fb_info->xpos;
        win->area[0].ypos   = fb_info->ypos;
        debug("format=%d,xact=%d,yact=%d,xsize=%d,vsize=%d,xpos=%d,ypos=%d\n",
                fb_info->format,fb_info->xact,fb_info->yact,fb_info->xsize,
                fb_info->ysize,fb_info->xpos,fb_info->ypos);

        return 0;
}

void rk_fb_vidinfo_to_screen(vidinfo_t *vid, struct rk_screen *screen)
{
    screen->type        = vid->screen_type;
    screen->face        = vid->lcd_face;
   screen->color_mode   = vid->color_mode;
   screen->dsp_lut    = vid->dsp_lut;
   //screen->cabc_lut   = ?
   screen->mode.hsync_len   = vid->vl_hspw;
   screen->mode.left_margin = vid->vl_hbpd;
   screen->mode.xres        = vid->vl_col;
   screen->mode.right_margin= vid->vl_hfpd;
   
   screen->mode.vsync_len   = vid->vl_vspw;
   screen->mode.upper_margin= vid->vl_vbpd;
   screen->mode.yres        = vid->vl_row;
   screen->mode.lower_margin= vid->vl_vfpd;
   screen->mode.vmode       = vid->vmode;
   screen->mode.pixclock    = vid->real_freq;
   screen->pin_hsync        = vid->vl_hsp;
   screen->pin_vsync        = vid->vl_vsp;
   screen->pin_dclk         = vid->vl_clkp;
   screen->pin_den          = vid->vl_oep;
	screen->dclk_inv	    = vid->dclk_inv;
   screen->pixelrepeat      = vid->pixelrepeat;
   screen->refresh_mode	    = vid->refresh_mode;

   screen->overscan.left    = 100;
   screen->overscan.right   = 100;
   screen->overscan.top     = 100;
   screen->overscan.bottom  = 100;
   screen->swap_gb          = 0;
   screen->swap_rb          = vid->vl_swap_rb;
   screen->swap_rg          = 0;
   screen->swap_delta       = 0;
   screen->swap_dumy        = 0;
   screen->x_mirror         = !!vid->x_mirror;
   screen->y_mirror         = !!vid->y_mirror;
}

#ifdef CONFIG_OF_LIBFDT
int rk_fb_find_lcdc_node_dt(struct rockchip_fb *fb,
			    const void *blob)
{
	int node;

	node = fdt_path_offset(blob, "lcdc0");
	if (PRMRY == fdtdec_get_int(blob, node, "rockchip,prop", 0)) {
		fb->lcdc_id = 0;
	} else {
		node = fdt_path_offset(blob, "lcdc1");
		if (PRMRY == fdtdec_get_int(blob, node, "rockchip,prop", 0)) {
			fb->lcdc_id = 1;
		} else {
			node = fdt_path_offset(blob, "lcdc");
			fb->lcdc_id = 0;
		}
	}
	fb->lcdc_node = node;
	debug("rockchip fb use lcdc%d\n", fb->lcdc_id);

	return node;
}

int rk_fb_pwr_ctr_parse_dt(struct rockchip_fb *rk_fb, const void *blob)
{
	int root = fdt_subnode_offset(blob, rk_fb->lcdc_node, "power_ctr");
	int child;
	struct rk_fb_pwr_ctr_list *pwr_ctr;
#if defined(DEBUG)
	struct list_head *pos;
#endif
	const char *name;
	u32 val = 0;
	int len;

	INIT_LIST_HEAD(&rk_fb->pwrlist_head);
	if (root <= 0) {
		printf("can't find power_ctr node for lcdc%d\n",
		       rk_fb->lcdc_id);
		return 0;
	}
	for (child = fdt_first_subnode(blob, root);
	     child >= 0;
	     child = fdt_next_subnode(blob, child)) {
		pwr_ctr = malloc(sizeof(*pwr_ctr));
		if (!pwr_ctr) {
			printf("ERR:%s out of memory\n", __func__);
			return -ENOMEM;
		}

		name = fdt_get_name(blob, child, &len);
		if (len < 32)
			strcpy(pwr_ctr->pwr_ctr.name, name);
		val = fdtdec_get_int(blob, child, "rockchip,power_type", -1);
		if (val == GPIO) {
			pwr_ctr->pwr_ctr.type = GPIO;
			fdtdec_decode_gpio(blob, child, "gpios",
					   &pwr_ctr->pwr_ctr.gpio);
			pwr_ctr->pwr_ctr.atv_val =  !(pwr_ctr->pwr_ctr.gpio.flags &
							OF_GPIO_ACTIVE_LOW);

		} else {
			pwr_ctr->pwr_ctr.type = REGULATOR;
		}
		pwr_ctr->pwr_ctr.delay = fdtdec_get_int(blob, child,
							"rockchip,delay", 0);
		list_add_tail(&pwr_ctr->list, &rk_fb->pwrlist_head);
	}
#if defined(DEBUG)
	list_for_each(pos, &rk_fb->pwrlist_head) {
		pwr_ctr = list_entry(pos, struct rk_fb_pwr_ctr_list,
				     list);
		printf("pwr_ctr_name:%s\n"
		       "pwr_type:%s\n"
		       "gpio:0x%08x\n"
		       "atv_val:%d\n"
		       "delay:%d\n\n",
		       pwr_ctr->pwr_ctr.name,
		       (pwr_ctr->pwr_ctr.type == GPIO) ? "gpio" : "regulator",
		       pwr_ctr->pwr_ctr.gpio.gpio,
		       pwr_ctr->pwr_ctr.atv_val,
		       pwr_ctr->pwr_ctr.delay);
	}
#endif

	return 0;
}

int rk_fb_pwr_enable(struct rockchip_fb *fb)
{
	struct list_head *pos;
	struct rk_fb_pwr_ctr_list *pwr_ctr_list;
	struct pwr_ctr *pwr_ctr;

	if (list_empty(&fb->pwrlist_head))
		return 0;
	list_for_each(pos, &fb->pwrlist_head) {
		pwr_ctr_list = list_entry(pos, struct rk_fb_pwr_ctr_list,
					  list);
		pwr_ctr = &pwr_ctr_list->pwr_ctr;
		if (pwr_ctr->type == GPIO) {
			gpio_direction_output(pwr_ctr->gpio.gpio,
					      pwr_ctr->atv_val);
			mdelay(pwr_ctr->delay);
		}
	}

	return 0;
}

int rk_fb_pwr_disable(struct rockchip_fb *fb)
{
	struct list_head *pos;
	struct rk_fb_pwr_ctr_list *pwr_ctr_list;
	struct pwr_ctr *pwr_ctr;

	if (list_empty(&fb->pwrlist_head))
		return 0;
	list_for_each(pos, &fb->pwrlist_head) {
		pwr_ctr_list = list_entry(pos, struct rk_fb_pwr_ctr_list,
					  list);
		pwr_ctr = &pwr_ctr_list->pwr_ctr;
		if (pwr_ctr->type == GPIO)
			gpio_set_value(pwr_ctr->gpio.gpio, !pwr_ctr->atv_val);
	}
	return 0;
}


int rk_fb_parse_dt(struct rockchip_fb *rk_fb, const void *blob)
{
	int node;
	int phandle;
	int rotate_mode, len, lut_num;

	node = fdt_node_offset_by_compatible(blob, 0, COMPAT_ROCKCHIP_FB);
	panel_info.dual_lcd_enabled =
		fdtdec_get_int(blob, node, "rockchip,disp-mode", 0);

	/* 0: for fdt running time save */
#if 0
	phandle = fdt_getprop_u32_default(blob, "/display-timings",
					  "native-mode", -1);
	node = fdt_node_offset_by_phandle(blob, phandle);
#else
#if defined(CONFIG_RKCHIP_RK3399)
	int screen_node;
	screen_node = fdt_node_offset_by_compatible(blob, 0, "rockchip,screen");
	if (screen_node < 0) {
		debug("Can't find rk_screen device node");
		return screen_node;
	}
	node = fdt_subnode_offset(blob, screen_node, "display-timings");
#else
	node = fdt_path_offset(blob, "/display-timings");
#endif
	if (node < 0) {
		printf("rk fb dt: can't find node '/display-timings'\n");
		return -ENODEV;
	}
	phandle = fdt_getprop_u32_default_node(blob, node, 0, "native-mode", -1);
	node = fdt_node_offset_by_phandle_node(blob, node, phandle);
#endif
	if (node <= 0) {
		printf("rk fb dt: can't get device node for display-timings\n");
		return -ENODEV;
	}

	/* Panel infomation */
	panel_info.vl_bpix = 5;
	panel_info.lvds_ttl_en  = 0;
	panel_info.screen_type = fdtdec_get_int(blob, node, "screen-type", -1);
	panel_info.refresh_mode = fdtdec_get_int(blob, node, "refresh-mode", 0);
	panel_info.color_mode = fdtdec_get_int(blob, node, "color-mode", 0);
	panel_info.lcd_face = fdtdec_get_int(blob, node, "out-face", -1);
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

	panel_info.vl_width = fdtdec_get_int(blob, node, "hactive", 0);

	panel_info.vl_height = fdtdec_get_int(blob, node, "vactive", 0);

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

	panel_info.dclk_inv = fdtdec_get_int(blob, node, "dclk-inv", -1);
	if (panel_info.dclk_inv == (u_char)-1) {
		debug("Can't get dclk-inv, set to 0 for default\n");
		panel_info.dclk_inv = 0;
	}

	panel_info.lvds_format = fdtdec_get_int(blob, node, "lvds-format", -1);
	if (panel_info.lvds_format == (u_char)-1) {
		debug("Can't get lvds-format, set to LVDS_8BIT_2\n");
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

	panel_info.vl_hbpd = fdtdec_get_int(blob, node,	"hback-porch", 0);
	if (panel_info.vl_hbpd == 0) {
		debug("Can't get hback-porch use 100 to default\n");
		panel_info.vl_hbpd = 100;
	}

	panel_info.vl_vspw = fdtdec_get_int(blob, node, "vsync-len", 0);
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

	fdt_getprop(blob, node, "dsp-lut", &len);
	if (len > 0) {
		lut_num  = len / sizeof(u32);
		panel_info.dsp_lut = malloc(len);
		if (!panel_info.dsp_lut) {
			printf("malloc dsp lut failed\n");
			return -ENOMEM;
		}
		if (fdtdec_get_int_array(blob, node, "dsp-lut",
			(u32 *)panel_info.dsp_lut, lut_num)) {
			printf("Cannot decode dsp_lut\n");
			return -EINVAL;
		}
	} else {
		panel_info.dsp_lut = NULL;
	}

	node = rk_fb_find_lcdc_node_dt(rk_fb, blob);
	if (node <= 0) {
		printf("no lcdc found\n");
		return node;
	}
	rotate_mode = fdtdec_get_int(blob, node, "rockchip,mirror", 0);
	panel_info.x_mirror = rotate_mode & X_MIRROR;
	panel_info.y_mirror = rotate_mode & Y_MIRROR;
	panel_info.lcdc_id = rk_fb->lcdc_id;
	rk_fb_pwr_ctr_parse_dt(rk_fb, blob);
	debug("lcd timing:\n"
	      "screen_type:%d\n"
	      "lcd_face=0x%x\n"
	      "vl_col=%d\n"
	      "vl_row=%d\n"
	      "vl_freq = %d\n"
	      "lvds_format=%d\n"
	      "lcdc_id=%d\n",
	      panel_info.screen_type,
	      panel_info.lcd_face,
	      panel_info.vl_col,
	      panel_info.vl_row,
	      panel_info.vl_freq,
	      panel_info.lvds_format,
	      panel_info.lcdc_id);

	return 0;
}
#endif /* CONFIG_OF_LIBFDT */


#if defined(CONFIG_RK_HDMI)
extern int g_hdmi_noexit;
#endif

void lcd_ctrl_init(void *lcdbase)
{
	struct rockchip_fb *fb = &rockchip_fb;
#if  defined(CONFIG_OF_LIBFDT)
	debug("rk fb parse dt start.\n");
	if (!gd->fdt_blob)
		return;
	int ret = rk_fb_parse_dt(fb, gd->fdt_blob);
	debug("rk fb parse dt end.\n");
	if (ret < 0)
		return;

#endif

#if defined(CONFIG_RK_HDMI)
	rk_hdmi_probe(&panel_info);
#endif

#if defined(CONFIG_RK3036_TVE)
#if defined(CONFIG_RK_HDMI)
	if (g_hdmi_noexit == 1)
#endif
		rk3036_tve_init(&panel_info);
#endif

#if defined(CONFIG_RK1000_TVE)
#if defined(CONFIG_RK_HDMI)
	if (g_hdmi_noexit == 1)
#endif
		rk1000_tve_init_panel(&panel_info);
#endif

#if defined(CONFIG_GM7122_TVE)
#if defined(CONFIG_RK_HDMI)
	if (g_hdmi_noexit == 1)
#endif
		gm7122_tve_init_panel(&panel_info);
		
#endif

	panel_info.logo_rgb_mode = RGB565;
	rk_fb_pwr_enable(fb);
#if defined(CONFIG_RKCHIP_RK3399)
	if ((panel_info.screen_type == SCREEN_HDMI) ||
	    (panel_info.dual_lcd_enabled == NO_DUAL) ||
	    (panel_info.dual_lcd_enabled == ONE_DUAL))
		rkclk_lcdc_dclk_pll_sel(panel_info.lcdc_id, 0);
	else
		rkclk_lcdc_dclk_pll_sel(panel_info.lcdc_id, 1);
#endif
	panel_info.real_freq = rkclk_lcdc_clk_set(panel_info.lcdc_id,
						  panel_info.vl_freq);
	rk_lcdc_init(panel_info.lcdc_id);

	rk_lcdc_load_screen(&panel_info);

#if defined(CONFIG_RK1000_TVE)
#if defined(CONFIG_RK_HDMI)
	if (g_hdmi_noexit == 1)
#endif
		rk1000_tve_init(&panel_info);

#endif

#if defined(CONFIG_GM7122_TVE)
#if defined(CONFIG_RK_HDMI)
	if (g_hdmi_noexit == 1)
#endif
		gm7122_tve_init(&panel_info);

#endif

}

void lcd_enable(void)
{
	//rk_lcdc_set_par(&panel_info.par[0].fb_info, &panel_info);
}

void lcd_pandispaly(struct fb_dsp_info *info)
{
	rk_lcdc_set_par(info, &panel_info);
}

void lcd_standby(int enable)
{
	rk_lcdc_standby(enable);
}

/* dummy function */
void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
}

