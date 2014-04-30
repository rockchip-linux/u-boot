/********************************************************************************
  COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
  --  ALL RIGHTS RESERVED  --
  File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/

#include <common.h>
#include <fastboot.h>
#include <asm/io.h>
#include <lcd.h>
#include <power/pmic.h>
#include <version.h>
#include <asm/arch/rk_i2c.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <asm/arch/clock.h>
#include <errno.h>
#include <fdt_support.h>

#include "../common/config.h"
#include "../common/idblock.h"
#include "i2c.h"

//#include <asm/arch/rk30_drivers.h>
DECLARE_GLOBAL_DATA_PTR;
int wfi_status = 0;
void wait_for_interrupt()
{
	//uint8 ret,i;
	//u32 pllcon0[4], pllcon1[4], pllcon2[4];

	/* PLL enter slow-mode */
	g_cruReg->cru_mode_con = (0x3<<((2*4) + 16)) | (0x0<<(2*4));
	g_cruReg->cru_mode_con = (0x3<<((3*4) + 16)) | (0x0<<(3*4));
	g_cruReg->cru_mode_con = (0x3<<((0*4) + 16)) | (0x0<<(0*4));

	printf("PLL close over! \n\n\n");
	wfi_status = 1;
	wfi();
	wfi_status = 0;
	printf("PLL open begin! \n");


	/* PLL enter normal-mode */
	g_cruReg->cru_mode_con = (0x3<<((0*4) + 16)) | (0x1<<(0*4));
	g_cruReg->cru_mode_con = (0x3<<((3*4) + 16)) | (0x1<<(3*4));
	g_cruReg->cru_mode_con = (0x3<<((2*4) + 16)) | (0x1<<(2*4));


	printf("PLL open end! \n");
}

int get_wfi_status(void)
{
	return wfi_status;
}



#ifdef CONFIG_OF_LIBFDT
extern uint32 ddr_get_cap(void);
extern int rk_fb_parse_dt(const void *blob);
int rk_fixup_memory_banks(void *blob, u64 start[], u64 size[], int banks) {
    //TODO:auto detect size.
    if (banks > 0){
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
        size[0] = ddr_get_cap();
#else
	size[0] = 0x40000000;//1G for now
#endif
    }
    return fdt_fixup_memory_banks(blob, start, size, banks);
}
void board_lmb_reserve(struct lmb *lmb) {
    //reserve 48M for kernel & 8M for nand api.
    lmb_reserve(lmb, gd->bd->bi_dram[0].start, 56 * 1024 * 1024);
}

#endif

#ifdef CONFIG_RK_FB

#ifdef CONFIG_OF_CONTROL
struct fdt_gpio_state lcd_en_gpio,lcd_cs_gpio;
int lcd_en_delay,lcd_cs_delay;
int lcd_node = 0;

int rk_lcd_parse_dt(const void *blob)
{
	//int* blob = getenv_hex("fdtaddr", 0);
	int node;
	int lcd_en_node,lcd_cs_node;
	const struct fdt_property *prop,*prop1;
	const u32 *cell;
	const u32 *reg;
	
    lcd_node = fdt_path_offset(blob, "lcdc1");
	if (PRMRY == fdtdec_get_int(blob, lcd_node, "rockchip,prop", 0)) {
		printf("lcdc1 is the prmry lcd controller\n");
	} else {
		lcd_node = fdt_path_offset(blob, "lcdc0");
	}
	node=fdt_subnode_offset(blob,lcd_node,"power_ctr");
	lcd_en_node=fdt_subnode_offset(blob,node,"lcd_en");
	lcd_cs_node=fdt_subnode_offset(blob,node,"lcd_cs");
	fdtdec_decode_gpio(blob, lcd_en_node, "gpios", &lcd_en_gpio);
	lcd_en_gpio.gpio = rk_gpio_base_to_bank(lcd_en_gpio.gpio & RK_GPIO_BANK_MASK) | lcd_en_gpio.gpio & RK_GPIO_PIN_MASK;
	lcd_en_gpio.flags = !(lcd_en_gpio.flags  & OF_GPIO_ACTIVE_LOW);
	lcd_en_delay = fdtdec_get_int(blob, lcd_en_node, "rockchip,delay", 0);

	fdtdec_decode_gpio(blob, lcd_cs_node, "gpios", &lcd_cs_gpio);
	lcd_cs_gpio.gpio = rk_gpio_base_to_bank(lcd_cs_gpio.gpio & RK_GPIO_BANK_MASK) | lcd_cs_gpio.gpio & RK_GPIO_PIN_MASK;
	lcd_cs_gpio.flags = !(lcd_cs_gpio.flags  & OF_GPIO_ACTIVE_LOW);    
	lcd_cs_delay = fdtdec_get_int(blob, lcd_cs_node, "rockchip,delay", 0);

	return 0;
}

#endif





void rk_backlight_ctrl(int brightness)
{
#ifdef CONFIG_OF_CONTROL
	if (!lcd_node)
		rk_lcd_parse_dt(gd->fdt_blob);   
#endif

	rk_pwm_config(brightness);
}

void rk_fb_init(unsigned int onoff)
{
	pmic_init(0);  //enable lcdc power
#ifdef CONFIG_OF_CONTROL    
    if(lcd_node == 0)rk_lcd_parse_dt(gd->fdt_blob);

	if(onoff)
	{
		if(lcd_en_gpio.name!=NULL)gpio_direction_output(lcd_en_gpio.gpio, lcd_en_gpio.flags);
		mdelay(lcd_en_delay);     
		if(lcd_cs_gpio.name!=NULL)gpio_direction_output(lcd_cs_gpio.gpio, lcd_cs_gpio.flags);
		mdelay(lcd_cs_delay);
	}else{
		if(lcd_cs_gpio.name!=NULL)gpio_direction_output(lcd_cs_gpio.gpio, !lcd_cs_gpio.flags);
		mdelay(lcd_cs_delay);
		if(lcd_en_gpio.name!=NULL)gpio_direction_output(lcd_en_gpio.gpio, !lcd_en_gpio.flags);
		mdelay(lcd_en_delay);
	}
#else
	gpio_direction_output(RK3288_GPIO7_PHYS|GPIO_A4, 1);   // LCD_CS /GPIO7_A4 /NC ,high
#endif
}

vidinfo_t panel_info = {
#ifndef CONFIG_OF_CONTROL     
	.lcd_face    = OUT_D888_P666,
	.vl_freq	= 71,  
	.vl_col		= 1280,
	.vl_row		= 800,
	.vl_width	= 1280,
	.vl_height	= 800,
	.vl_clkp	= 0,
	.vl_hsp		= 0,
	.vl_vsp		= 0,
	.vl_bpix	= 4,	/* Bits per pixel, 2^5 = 32 */
	.vl_swap_rb = 0,
	.lvds_format  = LVDS_8BIT_2,
	.lvds_ttl_en  = 0,  // rk32 lvds ttl enable
	/* Panel infomation */
	.vl_hspw	= 10,
	.vl_hbpd	= 100,
	.vl_hfpd	= 18,

	.vl_vspw	= 2,
	.vl_vbpd	= 8,
	.vl_vfpd	= 6,

	.lcd_power_on = NULL,
	.mipi_power = NULL,

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
int rk616_power_on()
{	
	return 0;
}
#endif

#endif

#ifdef CONFIG_RK_I2C 
void rk_i2c_init(void)
{

#ifdef CONFIG_POWER_ACT8846
	i2c_set_bus_num(I2C_BUS_CH1);
	i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif

#ifdef CONFIG_RK616
	i2c_set_bus_num(I2C_BUS_CH4);
	i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif 
}
#endif

extern int rk_mmc_init(void);
int board_mmc_init(bd_t *bis)
{
	rk_mmc_init();
	return 0;
}

#ifdef CONFIG_POWER_ACT8846
struct pmic_voltage pmic_vol[] = {
	{"act_dcdc1",1200000},
	{"vdd_core" ,1000000},
	{"vdd_cpu"  ,1000000},
	{"act_dcdc4",3000000},
	{"act_ldo1" ,1000000},
	{"act_ldo2" ,1200000},
	{"act_ldo3" ,1800000},
	{"act_ldo4" ,3300000},
	{"act_ldo5" ,3300000}, 
	{"act_ldo6" ,3300000},
	{"act_ldo7" ,1800000},
	{"act_ldo8" ,2800000},
};

int pmic_get_vol(char *name)
{
	int i =0 ,vol = 0;
	for(i=0;i<ARRAY_SIZE(pmic_vol);i++){
		if(strcmp(pmic_vol[i].name,name)==0){
			vol = pmic_vol[i].vol;
			break;
		}
	}
	return vol;
}
#endif



