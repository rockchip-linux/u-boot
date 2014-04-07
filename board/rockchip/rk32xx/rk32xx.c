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
#include "config.h"
#include <asm/io.h>
#include <lcd.h>
#include "rkimage.h"
#include "rkloader.h"
#include "i2c.h"
#include <power/pmic.h>
#include <version.h>
#include <asm/arch/rk_i2c.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>

//#include <asm/arch/rk30_drivers.h>
DECLARE_GLOBAL_DATA_PTR;
extern char PRODUCT_NAME[20] = FASTBOOT_PRODUCT_NAME;
int wfi_status = 0;
void wait_for_interrupt()
{
	uint8 ret,i;
	u32 pllcon0[4], pllcon1[4], pllcon2[4];

	/* PLL enter slow-mode */
	g_cruReg->CRU_MODE_CON = (0x3<<((2*4) + 16)) | (0x0<<(2*4));
	g_cruReg->CRU_MODE_CON = (0x3<<((3*4) + 16)) | (0x0<<(3*4));
	g_cruReg->CRU_MODE_CON = (0x3<<((0*4) + 16)) | (0x0<<(0*4));

	printf("PLL close over! \n\n\n");
	wfi_status = 1;
	wfi();
	wfi_status = 0;
	printf("PLL open begin! \n");


	/* PLL enter normal-mode */
	g_cruReg->CRU_MODE_CON = (0x3<<((0*4) + 16)) | (0x1<<(0*4));
	g_cruReg->CRU_MODE_CON = (0x3<<((3*4) + 16)) | (0x1<<(3*4));
	g_cruReg->CRU_MODE_CON = (0x3<<((2*4) + 16)) | (0x1<<(2*4));


	printf("PLL open end! \n");
}

int get_wfi_status()
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



void startRockusb()
{
    printf("startRockusb,%d\n" , get_ticks());
//    rk_backlight_ctrl(0);
    FW_SDRAM_ExcuteAddr = 0;
    g_BootRockusb = 1;
    FWSetResetFlag = 0;
    FWLowFormatEn = 0;
    UsbBoot();
    UsbHook();
}
int board_fbt_key_pressed(void)
{
    int boot_rockusb = 0, boot_recovery = 0, boot_fastboot = 0; 
    enum fbt_reboot_type frt = FASTBOOT_REBOOT_NONE;
	int vbus = GetVbus();
    checkKey(&boot_rockusb, &boot_recovery, &boot_fastboot);
	printf("vbus = %d\n", vbus);
    if(boot_recovery && (vbus==0)) {
        printf("%s: recovery key pressed.\n",__func__);
        frt = FASTBOOT_REBOOT_RECOVERY;
    } else if (boot_rockusb && (vbus!=0)) {
        printf("%s: rockusb key pressed.\n",__func__);
        startRockusb();
    } else if(boot_fastboot && (vbus!=0)){
        printf("%s: fastboot key pressed.\n",__func__);
        frt = FASTBOOT_REBOOT_FASTBOOT;
    }

    return frt;
}

struct fbt_partition fbt_partitions[FBT_PARTITION_MAX_NUM];

void board_fbt_finalize_bootargs(char* args, int buf_sz,
        int ramdisk_addr, int ramdisk_sz, int recovery)
{
    char recv_cmd[2]={0};
    fixInitrd(&gBootInfo, ramdisk_addr, ramdisk_sz);
    if (recovery) {
        change_cmd_for_recovery(&gBootInfo, recv_cmd);
    }
    snprintf(args, buf_sz, "%s", gBootInfo.cmd_line);
//TODO:setup serial_no/device_id/mac here?
}
int board_fbt_handle_flash(char *name,
        struct cmd_fastboot_interface *priv)
{
    return handleRkFlash(name, priv);
}
int board_fbt_handle_download(unsigned char *buffer,
        int length, struct cmd_fastboot_interface *priv)
{
    return handleDownload(buffer, length, priv);
}
int board_fbt_check_misc()
{
    //return true if we got recovery cmd from misc.
    return checkMisc();
}
int board_fbt_set_bootloader_msg(struct bootloader_message* bmsg)
{
    return setBootloaderMsg(bmsg);
}
int board_fbt_boot_check(struct fastboot_boot_img_hdr *hdr, int unlocked)
{
    return secureCheck(hdr, unlocked);
}
void board_fbt_boot_failed(const char* boot)
{
    printf("Unable to boot:%s\n", boot);

    if (!memcmp(BOOT_NAME, boot, sizeof(BOOT_NAME))) {
        printf("try to start recovery\n");
        char *const boot_cmd[] = {"booti", RECOVERY_NAME};
        do_booti(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
    } else if (!memcmp(RECOVERY_NAME, boot, sizeof(RECOVERY_NAME))) {
        printf("try to start backup\n");
        char *const boot_cmd[] = {"booti", BACKUP_NAME};
        do_booti(NULL, 0, ARRAY_SIZE(boot_cmd), boot_cmd);
    }  
    printf("try to start rockusb\n");
    startRockusb();
}


#ifdef CONFIG_RK_FB
#ifdef CONFIG_RK3288SDK
#define write_pwm_reg(id, addr, val)        (*(unsigned long *)(addr+RK3288_PWM0123_BASE_ADDR+id*0x10)=val)
#else
#define write_pwm_reg(id, addr, val)        (*(unsigned long *)(addr+(PWM01_BASE_ADDR+(id>>1)*0x20000)+id*0x10)=val)
#endif

#ifdef CONFIG_OF_CONTROL
struct fdt_gpio_state lcd_en_gpio,lcd_cs_gpio,bl_en_gpio;
int lcd_en_delay,lcd_cs_delay;
int lcd_node = 0,bl_node=0;
int pwm_addr = 0;
int rk_lcd_parse_dt(const void *blob)
{
    //int* blob = getenv_hex("fdtaddr", 0);
    int node;
    lcd_node = fdt_path_offset(blob, "lcdc1");
    int lcd_en_node,lcd_cs_node;
    const struct fdt_property *prop,*prop1;
    const u32 *cell;
    const u32 *reg;
    
    if(PRMRY == fdtdec_get_int(blob, lcd_node, "rockchip,prop", 0))
    {
        printf("lcdc1 is the prmry lcd controller\n");
    }else{
        lcd_node = fdt_path_offset(blob, "lcdc0");
    }
    node=fdt_subnode_offset(blob,lcd_node,"power_ctr");
    lcd_en_node=fdt_subnode_offset(blob,node,"lcd_en");
    lcd_cs_node=fdt_subnode_offset(blob,node,"lcd_cs");
    fdtdec_decode_gpio(blob, lcd_en_node, "gpios", &lcd_en_gpio);
    lcd_en_gpio.flags = !(lcd_en_gpio.flags  & OF_GPIO_ACTIVE_LOW);
    lcd_en_delay = fdtdec_get_int(blob, lcd_en_node, "rockchip,delay", 0);

    fdtdec_decode_gpio(blob, lcd_cs_node, "gpios", &lcd_cs_gpio);
    lcd_cs_gpio.flags = !(lcd_cs_gpio.flags  & OF_GPIO_ACTIVE_LOW);    
    lcd_cs_delay = fdtdec_get_int(blob, lcd_cs_node, "rockchip,delay", 0);

    bl_node = fdt_path_offset(blob, "/backlight");
    fdtdec_decode_gpio(blob, bl_node, "enable-gpios", &bl_en_gpio);
    bl_en_gpio.flags = !(bl_en_gpio.flags  & OF_GPIO_ACTIVE_LOW); 

    prop=fdt_get_property(blob, bl_node, "pwms", 0);
    cell = (u32 *)prop->data;
    prop1 = fdt_get_property(blob, 
                             fdt_node_offset_by_phandle(blob, fdt32_to_cpu(cell[0])), 
                             "reg", 
                             0);
    reg = (u32 *)prop1->data;
    pwm_addr = fdt32_to_cpu(reg[0]);
    printf("read lcd power ctrl from dts! \nlcd_en_gpio %s gpio=0x%x, flag=%d\n",lcd_en_gpio.name,lcd_en_gpio.gpio,lcd_en_gpio.flags);
    printf("lcd_cs_gpio %s gpio=0x%x, flag=%d\n",lcd_cs_gpio.name,lcd_cs_gpio.gpio,lcd_cs_gpio.flags);
    printf("bl_en_gpio %s gpio=0x%x, flag=%d, pwm_addr=0x%x\n",bl_en_gpio.name,bl_en_gpio.gpio,bl_en_gpio.flags,pwm_addr);
    return 0;
}

#endif

void rk_backlight_ctrl(int brightness)
{
    int id =0;
    int total = 0x4b0;
    int pwm = total * (100 - brightness) / 100;
    
    printf("backlight --- brightness:%d\n", brightness);
    
    #ifdef CONFIG_OF_CONTROL
    if(bl_node == 0)rk_lcd_parse_dt(getenv_hex("fdtaddr", 0));
    if(pwm_addr)id = (pwm_addr-RK3288_PWM0123_BASE_ADDR)/0x10;   
    #endif
    if(id == 0)g_grfReg->GRF_GPIO7A_IOMUX = (3<<16)|1;   // 1: PWM0/ 0: GPIO7_A0

    //gpio_direction_output(RK3288_GPIO7_PHYS|GPIO_A0, 1);   // PWM0 /GPIO7_A0 /high
    write_pwm_reg(id, 0x0c, 0x80);
    write_pwm_reg(id, 0x08, total);
    write_pwm_reg(id, 0x04, pwm);
    write_pwm_reg(id, 0x00, 0);
    write_pwm_reg(id, 0x0c, 0x09);  // PWM_DIV|PWM_ENABLE|PWM_TIME_EN
    #ifdef CONFIG_OF_CONTROL
    if(bl_en_gpio.name!=NULL)gpio_direction_output(bl_en_gpio.gpio, (pwm != total)?bl_en_gpio.flags:!bl_en_gpio.flags);  
    #else
    gpio_direction_output(RK3288_GPIO7_PHYS|GPIO_A2, (pwm != total)); // BL_EN  GPIO7_A2, high
    #endif
}

void rk_fb_init(unsigned int onoff)
{
    pmic_init(0);  //enable lcdc power
#ifdef CONFIG_OF_CONTROL    
    if(lcd_node == 0)rk_lcd_parse_dt(getenv_hex("fdtaddr", 0));

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
#else
    .vl_col     = 4096,
    .vl_row     = 4096,
    .vl_bpix    = 4,
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
void rk_i2c_init()
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



