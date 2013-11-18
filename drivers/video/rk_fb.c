/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/

    
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
        vid->backlight_on(50);

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

