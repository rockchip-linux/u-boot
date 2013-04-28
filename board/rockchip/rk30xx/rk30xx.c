/*
 * (C) Copyright 2013-2013
 * Peter <superpeter.cai@gmail.com>
 *
 * Configuation settings for the rk30xx board.
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
#include <fastboot.h>
#include "../common/armlinux/config.h"
#include <lcd.h>

DECLARE_GLOBAL_DATA_PTR;


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

int board_fbt_key_pressed(void)
{
    int boot_rockusb = 0, boot_recovery = 0, boot_fastboot = 0; 
    enum fbt_reboot_type frt = FASTBOOT_REBOOT_NONE;
    int recovery_key = checkKey(&boot_rockusb, &boot_recovery, &boot_fastboot);

    if (recovery_key) {
        printf("%s: recovery_key=%d.\n", __func__, recovery_key);
    }

    if(boot_recovery) {
        printf("%s: recovery key pressed.\n",__func__);
        frt = FASTBOOT_REBOOT_RECOVERY;
    } else if (boot_rockusb) {
        printf("%s: rockusb key pressed.\n",__func__);
		FW_SDRAM_ExcuteAddr = 0;
		g_BootRockusb = 1;
		FWSetResetFlag = 0;
        FWLowFormatEn = 0;
		UsbBoot();
		printf("UsbHook,%d\n" , RkldTimerGetTick());
		UsbHook();
    } else if(boot_fastboot){
        printf("%s: fastboot key pressed.\n",__func__);
        frt = FASTBOOT_REBOOT_BOOTLOADER;
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
int board_fbt_check_misc()
{
    //return true if we got recovery cmd from misc.
    return checkMisc();
}
void board_fbt_set_bootloader_msg(struct bootloader_message bmsg)
{
    setBootloaderMsg(bmsg);
}
void board_fbt_boot_check(struct fastboot_boot_img_hdr *hdr)
{
    checkBoot(hdr);
}
extern char bootloader_ver[];

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
    int i = 0;
    cmdline_mtd_partition *cmd_mtd;

    printf("board_late_init\n");
	ChipTypeCheck();
	if( StorageInit() == 0)
		printf("StorageInit OK!\n");
	else
		printf("Fail!\n");

    SecureBootCheck();
	get_bootloader_ver(NULL);
	printf("##################################################\n");
	printf("\n#Boot ver: %s\n\n", bootloader_ver);
	printf("##################################################\n");

    RockusbKeyInit(&key_rockusb);
    FastbootKeyInit(&key_fastboot);
	PowerHoldKeyInit();
    if (!GetParam(0, DataBuf)) {
	    ParseParam( &gBootInfo, ((PLoaderParam)DataBuf)->parameter, \
                ((PLoaderParam)DataBuf)->length );
        cmd_mtd = &(gBootInfo.cmd_mtd);
        for(i = 0;i < cmd_mtd->num_parts;i++) {
            fbt_partitions[i].name = cmd_mtd->parts[i].name;
            fbt_partitions[i].offset = cmd_mtd->parts[i].offset;
            if (cmd_mtd->parts[i].size == SIZE_REMAINING) {
                fbt_partitions[i].size_kb = SIZE_REMAINING;
            } else {
                fbt_partitions[i].size_kb = cmd_mtd->parts[i].size >> 1;
            }
            printf("partition(%s): offset=0x%08X, size=0x%08X\n", \
                    cmd_mtd->parts[i].name, cmd_mtd->parts[i].offset, \
                    cmd_mtd->parts[i].size);
        }
    }

    //TODO:set those buffers in a better way, and use malloc?
    setup_space(gBootInfo.kernel_load_addr);

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

void rk_backlight_ctrl(unsigned int onoff)
{
    #ifdef CONFIG_T7H
    int id =2;
    int total = 0x4b0;
    int pwm = total/2;
    int *addr =0;

    if(ChipType == CHIP_RK3066)
    {
        g_grfReg->GRF_GPIO_IOMUX[0].GPIOD_IOMUX |= ((1<<12)<<16)|(1<<12);   // pwm2
    }
    //SetPortOutput(0,30,0);   //gpio0_d6 0
    write_pwm_reg(id, 0x0c, 0x80);
    write_pwm_reg(id, 0x08, total);
    write_pwm_reg(id, 0x04, pwm);
    write_pwm_reg(id, 0x00, 0);
    write_pwm_reg(id, 0x0c, 0x09);  // PWM_DIV|PWM_ENABLE|PWM_TIME_EN

    SetPortOutput(6,11,1);   //gpio6_b3 1 ,backlight enable
    #endif
}

void rk_fb_init(unsigned int onoff)
{
    #ifdef CONFIG_T7H
    SetPortOutput(4,26,1);   //gpio4_d2 1
     
    SetPortOutput(0,26,0);   //gpio0_d2 0
    SetPortOutput(6,12,0);   //gpio6_b4 0
    DRVDelayMs(25);
    SetPortOutput(4,23,1);   //gpio4_c7 1
    SetPortOutput(0,29,1);   //gpio0_d5 1
    SetPortOutput(4,30,1);   //gpio4_d6 1
    SetPortOutput(4,31,1);   //gpio4_d7 1
    #endif
}

vidinfo_t panel_info = {
    .lcd_face    = OUT_P888,
	.vl_freq	= 48,  
	.vl_col		= 1024,
	.vl_row		= 600,
	.vl_width	= 1024,
	.vl_height	= 600,
	.vl_clkp	= 0,
	.vl_hsp		= 0,
	.vl_vsp		= 0,
	.vl_bpix	= 4,	/* Bits per pixel, 2^5 = 32 */
    .vl_swap_rb = 0,

	/* Panel infomation */
	.vl_hspw	= 10,
	.vl_hbpd	= 300,
	.vl_hfpd	= 20,

	.vl_vspw	= 2,
	.vl_vbpd	= 25,
	.vl_vfpd	= 10,

	.lcd_power_on = NULL,
	.mipi_power = NULL,

	.init_delay	= 0,
	.power_on_delay = 0,
	.reset_delay	= 0,
};

void init_panel_info(vidinfo_t *vid)
{
	vid->logo_on	= 1;
    vid->enable_ldo = rk_fb_init;
    vid->backlight_on = rk_backlight_ctrl;
    vid->logo_rgb_mode = 2;
}

#endif

