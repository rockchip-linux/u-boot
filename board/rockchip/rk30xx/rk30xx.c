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
#include "../common/armlinux/config.h"
#include <asm/io.h>
#include <lcd.h>
#include "rkimage.h"
#include "rkloader.h"
#include "i2c.h"
#include <power/pmic.h>
#include <version.h>
#include <asm/arch/rk_i2c.h>

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

int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot)
{
    int i;
    int recovery_key = 0;
	*boot_rockusb = 0;
	*boot_recovery = 0;
	*boot_fastboot = 0;
	printf("checkKey\n");
	if(GetPortState(&key_rockusb))
	{
        *boot_rockusb = 1;
	    //printf("rockusb key is pressed\n");
	}
	if(GetPortState(&key_recovery))
	{
        *boot_recovery = 1;
	    //printf("recovery key is pressed\n");
	}
	if(GetPortState(&key_fastboot))
	{
		*boot_fastboot = 1;
		//printf("fastboot key is pressed\n");
	}
	return 0;
}

void RockusbKeyInit(key_config *key)
{
    key->type = KEY_AD;
    if(ChipType == CONFIG_RK3026)
    	key->key.adc.index = 3;
    else
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 0;
    key->key.adc.keyValueHigh= 30;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}

void RecoveryKeyInit(key_config *key)
{
    key->type = KEY_AD;
    if(ChipType == CONFIG_RK3026)
    	key->key.adc.index = 3;
    else
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 0;
    key->key.adc.keyValueHigh= 30;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}


void FastbootKeyInit(key_config *key)
{
    key->type = KEY_AD;
    if(ChipType == CONFIG_RK3026)
    	key->key.adc.index = 3;
    else
	key->key.adc.index = 1;	
    key->key.adc.keyValueLow = 950;
    key->key.adc.keyValueHigh= 960;
    key->key.adc.data = SARADC_BASE;
    key->key.adc.stas = SARADC_BASE+4;
    key->key.adc.ctrl = SARADC_BASE+8;
}

void PowerHoldPinInit()
{
   // pin_powerHold.type = KEY_GPIO;
   // pin_powerHold.key.gpio.valid = 1; 

  //  pin_powerHold.key.gpio.group = 0;
  //  pin_powerHold.key.gpio.index = 0; // gpio0A0
  //  setup_gpio(&pin_powerHold.key.gpio);
  //  if(pin_powerHold.key.gpio.valid)
  //      powerOn();
}
void PowerKeyInit()
{
#if 0
    key_power.type = KEY_GPIO;
    key_power.key.gpio.valid = 0; 
    if(ChipType == CONFIG_RK3066)
    {
        key_power.key.gpio.group = 6;
        key_power.key.gpio.index = 8; // gpio6B0
    }
    else
    {
        key_power.key.gpio.group = 0;
        key_power.key.gpio.index = 4; // gpio0A4
        //rknand_print_hex("grf:", g_3188_grfReg,1,512);
    }

    setup_gpio(&key_power.key.gpio);
    if(key_power.key.gpio.valid)
        powerOn();
#else
    key_power.type = KEY_INT;
    key_power.key.ioint.valid = 0; 
    if(ChipType == CONFIG_RK3066)
    {
        key_power.key.ioint.group = 6;
        key_power.key.ioint.index = 8; // gpio6B0
    }
    else
    {
        key_power.key.ioint.group = 0;
        key_power.key.ioint.index = 4; // gpio0A4
    }
	printf("setup gpio int\n");
	clr_all_gpio_int();
    setup_int(&key_power.key.ioint);
	IRQEnable(INT_GPIO0);
#endif

}

int power_hold() {
    return GetPortState(&key_power);
}

void reset_cpu(ulong ignored)
{
	SoftReset();
}

#ifdef CONFIG_USE_RK30IRQ
static int rk30_interrupt_inited = 0;
void do_irq (struct pt_regs *pt_regs)
{
	//printf("do_irq\n");
	IrqHandler();
}

int arch_interrupt_init (void)
{
	if(!rk30_interrupt_inited)
	{
		printf("arch_interrupt_init\n");
		InterruptInit();
		//rk30_reg_irq(irq_init_reg);
		rk30_interrupt_inited = 1;
	}
	return 0;
}
#endif /* CONFIG_USE_IRQ */


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	ChipTypeCheck();
	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
	printf("CPU:\tRK3066\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3066B)
     printf("CPU:\tRK3066B\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
     printf("CPU:\tRK3168\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)
     printf("CPU:\tRK3188\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188B)
     printf("CPU:\tRK3188B\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188T)
     printf("CPU:\tRK3188T\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
     printf("CPU:\tRK3026\n");
#endif
     return 0;
}
#endif

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


/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
	gd->ram_size = get_ram_size(
			(void *)CONFIG_SYS_SDRAM_BASE,
			CONFIG_SYS_SDRAM_SIZE);

	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\tRK30xx platform Board\n");
	return 0;
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

void startRockusb()
{
    printf("startRockusb,%d\n" , RkldTimerGetTick());
    rk_backlight_ctrl(0);
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

void board_fbt_finalize_bootargs(char* args, size_t buf_sz,
        size_t ramdisk_sz, int recovery)
{
    char recv_cmd[2]={0};
    ReSizeRamdisk(&gBootInfo, ramdisk_sz);
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

extern char bootloader_ver[];

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
    printf("board_late_init\n");

#ifdef CONFIG_RK_I2C 
	rk_i2c_init();
#endif

#ifdef CONFIG_POWER_ACT8846
	pmic_init(0);
#endif

    SecureBootCheck();
	get_bootloader_ver(NULL);
	printf("##################################################\n");
	printf("uboot version: %s\n",U_BOOT_VERSION_STRING);
	printf("\n#Boot ver: %s\n\n", bootloader_ver);
	printf("##################################################\n");

    RockusbKeyInit(&key_rockusb);
    FastbootKeyInit(&key_fastboot);
    RecoveryKeyInit(&key_recovery);
	PowerKeyInit();
    ChargerStateInit();

    getParameter();

    //TODO:set those buffers in a better way, and use malloc?
    setup_space(gd->arch.rk_extra_buf_addr);

    char tmp_buf[30];
    if (getSn(tmp_buf)) {
        tmp_buf[sizeof(tmp_buf)-1] = 0;
        setenv("fbt_sn#", tmp_buf);
    }
    fbt_preboot();
	return 0;
}
#endif

#ifdef CONFIG_RK_FB
#define write_pwm_reg(id, addr, val)        (*(unsigned long *)(addr+(PWM01_BASE_ADDR+(id>>1)*0x20000)+id*0x10)=val)

void rk_backlight_ctrl(int brightness)
{
    #ifdef CONFIG_RK3066SDK
    int id =0;
    int total = 0x4b0;
    int pwm = total * (100 - brightness) / 100;
    int *addr =0;
    printf("backlight --- brightness:%d\n", brightness);

 
        g_grfReg->GRF_GPIO_IOMUX[0].GPIOA_IOMUX |= ((1<<6)<<16)|(1<<6);   // pwm0, gpio0_a3
    
    //SetPortOutput(0,30,0);   //gpio0_d6 0
    write_pwm_reg(id, 0x0c, 0x80);
    write_pwm_reg(id, 0x08, total);
    write_pwm_reg(id, 0x04, pwm);
    write_pwm_reg(id, 0x00, 0);
    write_pwm_reg(id, 0x0c, 0x09);  // PWM_DIV|PWM_ENABLE|PWM_TIME_EN
    g_grfReg->GRF_GPIO_IOMUX[3].GPIOD_IOMUX |= ((1<<12)<<16)|(1<<12);   // pwm3, gpio3_d6

    SetPortOutput(6,11, pwm != total);   //gpio6_b3 1 ,backlight enable
    #endif
    #ifdef CONFIG_RK3188SDK
    int id =3;
    int total = 0x4b0;
    int pwm = total * (100 - brightness) / 100;
    int *addr =0;


    g_3188_grfReg->GRF_GPIO_IOMUX[3].GPIOD_IOMUX |= ((1<<12)<<16)|(1<<12);   // pwm3, gpio3_d6

    //SetPortOutput(3,30,0);   //gpio3_d6 0
    write_pwm_reg(id, 0x0c, 0x80);
    write_pwm_reg(id, 0x08, total);
    write_pwm_reg(id, 0x04, pwm);
    write_pwm_reg(id, 0x00, 0);
    write_pwm_reg(id, 0x0c, 0x09);  // PWM_DIV|PWM_ENABLE|PWM_TIME_EN   
    g_3188_grfReg->GRF_GPIO_IOMUX[3].GPIOD_IOMUX |= ((1<<12)<<16)|(1<<12);   // pwm3, gpio3_d6

    SetPortOutput(0,2, pwm != total);   //gpio0_a2 1 ,backlight enable
    
    #endif
}

void rk_fb_init(unsigned int onoff)
{
    printf("i2c init OVER in board! \n");
    pmic_init(0);  //enable lcdc power
    #ifdef CONFIG_RK3066SDK
    SetPortOutput(4,23,1);   //gpio4_c7 1 cs 1
    SetPortOutput(6,12,0);   //gpio6_b4 0 en 0
    #endif
    #ifdef CONFIG_RK3188SDK
    SetPortOutput(0,8,1);   //gpio0_b0 cs 1
    #endif
}

vidinfo_t panel_info = {
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

#ifdef CONFIG_RK616
	.screen_type  = SCREEN_LVDS,
#ifdef CONFIG_RK616_LVDS
	.lvds_format  = LVDS_8BIT_2,
	.lvds_ch_nr = 1,
#endif
#endif
};

void init_panel_info(vidinfo_t *vid)
{
	vid->logo_on	= 1;
    vid->enable_ldo = rk_fb_init;
    vid->backlight_on = rk_backlight_ctrl;   //move backlight enable to fbt_preboot, for don't show logo in rockusb
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

static key_config charger_state;
void ChargerStateInit()
{
    charger_state.type = KEY_GPIO;
    charger_state.key.gpio.valid = 1;
    charger_state.key.gpio.group = 0;
    charger_state.key.gpio.index = 10;

    setup_gpio(&charger_state.key.gpio);
}

/*
return 0: no charger
return 1: charging
*/
int is_charging()
{
    return !GetPortState(&charger_state);  //gpio0_b2, charger in status
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



