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
#include <version.h>
#include <errno.h>
#include <lcd.h>
#include <fastboot.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <power/pmic.h>

#include <asm/io.h>
#include <asm/arch/rkplat.h>

#include "../common/config.h"

DECLARE_GLOBAL_DATA_PTR;


#ifdef CONFIG_OF_LIBFDT
extern int rk_fb_parse_dt(const void *blob);

static ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}

void board_lmb_reserve(struct lmb *lmb) {
	ulong sp;
	sp = get_sp();
	debug("## Current stack ends at 0x%08lx ", sp);

	/* adjust sp by 64K to be safe */
	sp -= 64<<10;
	lmb_reserve(lmb, sp,
			gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size - sp);

	//reserve 48M for kernel & 8M for nand api.
	lmb_reserve(lmb, gd->bd->bi_dram[0].start, CONFIG_LMB_RESERVE_SIZE);
}
#endif /* CONFIG_OF_LIBFDT */


#ifdef CONFIG_RK_FB

#ifdef CONFIG_OF_LIBFDT
static struct fdt_gpio_state lcd_en_gpio, lcd_cs_gpio;
static int lcd_en_delay, lcd_cs_delay;
static int lcd_node = 0;

int rk_lcd_parse_dt(const void *blob)
{
	int node;
	int lcd_en_node, lcd_cs_node;

	lcd_node = fdt_path_offset(blob, "lcdc1");
	if (PRMRY == fdtdec_get_int(blob, lcd_node, "rockchip,prop", 0)) {
		printf("lcdc1 is the prmry lcd controller\n");
	} else {
		lcd_node = fdt_path_offset(blob, "lcdc0");
	}
	node = fdt_subnode_offset(blob, lcd_node, "power_ctr");
	lcd_en_node = fdt_subnode_offset(blob, node, "lcd_en");
	lcd_cs_node = fdt_subnode_offset(blob, node, "lcd_cs");
	fdtdec_decode_gpio(blob, lcd_en_node, "gpios", &lcd_en_gpio);
	lcd_en_gpio.gpio = rk_gpio_base_to_bank(lcd_en_gpio.gpio & RK_GPIO_BANK_MASK) | (lcd_en_gpio.gpio & RK_GPIO_PIN_MASK);
	lcd_en_gpio.flags = !(lcd_en_gpio.flags  & OF_GPIO_ACTIVE_LOW);
	lcd_en_delay = fdtdec_get_int(blob, lcd_en_node, "rockchip,delay", 0);

	fdtdec_decode_gpio(blob, lcd_cs_node, "gpios", &lcd_cs_gpio);
	lcd_cs_gpio.gpio = rk_gpio_base_to_bank(lcd_cs_gpio.gpio & RK_GPIO_BANK_MASK) | (lcd_cs_gpio.gpio & RK_GPIO_PIN_MASK);
	lcd_cs_gpio.flags = !(lcd_cs_gpio.flags & OF_GPIO_ACTIVE_LOW);
	lcd_cs_delay = fdtdec_get_int(blob, lcd_cs_node, "rockchip,delay", 0);

	return 0;
}
#endif /* CONFIG_OF_LIBFDT */


void rk_backlight_ctrl(int brightness)
{
#ifdef CONFIG_OF_LIBFDT
	if (!lcd_node)
		rk_lcd_parse_dt(gd->fdt_blob);
#endif

	rk_pwm_config(brightness);
}


void rk_fb_init(unsigned int onoff)
{

#ifdef CONFIG_OF_LIBFDT
	if (lcd_node == 0) rk_lcd_parse_dt(gd->fdt_blob);

	if(onoff) {
		if (lcd_en_gpio.name!=NULL) gpio_direction_output(lcd_en_gpio.gpio, lcd_en_gpio.flags);
		mdelay(lcd_en_delay);
		if (lcd_cs_gpio.name!=NULL) gpio_direction_output(lcd_cs_gpio.gpio, lcd_cs_gpio.flags);
		mdelay(lcd_cs_delay);
	} else {
		if (lcd_cs_gpio.name!=NULL) gpio_direction_output(lcd_cs_gpio.gpio, !lcd_cs_gpio.flags);
		mdelay(lcd_cs_delay);
		if (lcd_en_gpio.name!=NULL) gpio_direction_output(lcd_en_gpio.gpio, !lcd_en_gpio.flags);
		mdelay(lcd_en_delay);
	}
#else
	gpio_direction_output(GPIO_BANK7 | GPIO_A4, 1);
#endif
}


vidinfo_t panel_info = {
#ifndef CONFIG_OF_LIBFDT
	.lcd_face	= OUT_D888_P666,
	.vl_freq	= 71000000,
	.vl_col		= 1280,
	.vl_row		= 800,
	.vl_width	= 1280,
	.vl_height	= 800,
	.vl_clkp	= 0,
	.vl_hsp		= 0,
	.vl_vsp		= 0,
	.vl_bpix	= 4,	/* Bits per pixel, 2^5 = 32 */
	.vl_swap_rb	= 0,
	.lvds_format	= LVDS_8BIT_2,
	.lvds_ttl_en	= 0,  // rk32 lvds ttl enable
	/* Panel infomation */
	.vl_hspw	= 10,
	.vl_hbpd	= 100,
	.vl_hfpd	= 18,

	.vl_vspw	= 2,
	.vl_vbpd	= 8,
	.vl_vfpd	= 6,

	.lcd_power_on	= NULL,
	.mipi_power	= NULL,

	.init_delay	= 0,
	.power_on_delay = 0,
	.reset_delay	= 0,
#endif
};


void init_panel_info(vidinfo_t *vid)
{
	vid->logo_on	= 1;
	vid->enable_ldo = rk_fb_init;
	vid->backlight_on = NULL;//rk_backlight_ctrl;   //move backlight enable to fbt_preboot, for don't show logo in rockusb
	vid->logo_rgb_mode = RGB565;
}


#ifdef CONFIG_RK616
int rk616_power_on(void)
{
	return 0;
}
#endif

#endif /* CONFIG_RK_FB */


extern int rk_mmc_init(void);
int board_mmc_init(bd_t *bis)
{
	rk_mmc_init();

	if (StorageInit() == 0) {
		printf("storage init OK!\n");
	} else {
		printf("storage init fail!\n");
	}

	return 0;
}


/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	/* Set Initial global variables */

	gd->bd->bi_arch_number = MACH_TYPE_RK30XX;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x88000;

	return 0;
}


#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\tRockchip platform Board\n");
	return 0;
}
#endif


#ifdef CONFIG_ARCH_EARLY_INIT_R
int arch_early_init_r(void)
{
	debug("arch_early_init_r\n");

	/* rk pl330 dmac init */
#ifdef CONFIG_RK_DMAC
#ifdef CONFIG_RK_DMAC_0
	rk_pl330_dmac_init(0);
#endif
#ifdef CONFIG_RK_DMAC_1
	rk_pl330_dmac_init(1);
#endif
#endif /* CONFIG_RK_DMAC*/

	return 0;
}
#endif


#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	debug("board_late_init\n");
	load_disk_partitions();
	prepare_fdt();
	key_init();
	pmic_init(0);
	fg_init(0); /*fuel gauge init*/

	SecureBootCheck();

	//TODO:set those buffers in a better way, and use malloc?
	setup_space(gd->arch.rk_extra_buf_addr);

	char tmp_buf[30];
	if (getSn(tmp_buf)) {
		tmp_buf[sizeof(tmp_buf)-1] = 0;
		setenv("fbt_sn#", tmp_buf);
	}
#ifdef CONFIG_CMD_FASTBOOT
	fbt_preboot();
#else
	rk_preboot();
#endif
	return 0;
}
#endif

