/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: InKi Dae <inki.dae@samsung.com>
 * Author: Donghwa Lee <dh09.lee@samsung.com>
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
#include <lcd.h>
    

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
        vid->backlight_on(1);

}

void lcd_ctrl_init(void *lcdbase)
{
    printf("%s [%d]\n",__FUNCTION__,__LINE__);
    /* initialize parameters which is specific to panel. */
    init_panel_info(&panel_info);
    if (panel_info.enable_ldo)
        panel_info.enable_ldo(1);
    udelay(panel_info.init_delay);
    
    panel_width = panel_info.vl_width;
    panel_height = panel_info.vl_height;
    g_lcdbase = lcdbase;

    rk30_lcdc_init();
    rk30_load_screen(&panel_info);
}

void lcd_enable(void)
{
    printf("%s [%d]\n",__FUNCTION__,__LINE__);
    if (panel_info.logo_on) {
        rk30_lcdc_set_par(g_lcdbase, &panel_info);
    }
  
    lcd_panel_on(&panel_info);
}

/* dummy function */
void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
    return;
}

