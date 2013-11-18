/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * charge animation functions
 */
#include <common.h>
#include <command.h>
#include <asm/sizes.h>

#include <power/battery.h>
#include <bmp_image.h>
#include <bmp_image_data.h>
#include <fastboot.h>
#include <power/pmic.h>

#define IMAGES_NUM (sizeof(bmp_images) / sizeof(bmp_image_t))

static int current_index = 0;
static struct battery batt_status;

DECLARE_GLOBAL_DATA_PTR;

static void load_image(int index)
{
    if (bmp_images[index].loaded)
        return;
#ifndef CONFIG_CMD_FASTBOOT
    return;
#else
    if (((bmp_images[index].offset + bmp_images[index].size) * RK_BLK_SIZE)
            > CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE) {
        FBTDBG("size overflow!\n");
        return;
    }
    uint32_t addr = gd->arch.fastboot_buf_addr + bmp_images[index].offset * RK_BLK_SIZE;
    fbt_partition_t *ptn = fastboot_find_ptn("charge");
    if (!ptn) {
        FBTDBG("partition not found!\n");
        return;
    }
    if (StorageReadLba(ptn->offset + bmp_images[index].offset, 
                (void *)addr, bmp_images[index].size)) {
        FBTDBG("read lba error\n");
        return;
    }
    
    bmp_image_header_t* header = (bmp_image_header_t*)addr;
    if (header->tag != BMP_IMAGE_TAG) {
        FBTDBG("image tag error:%x\n", header->tag);
        return;
    }

    bmp_images[index].offset = addr + RK_BLK_SIZE;
    bmp_images[index].loaded = 1;
#endif
}

static uint32_t get_image(int index)
{
    int num = IMAGES_NUM;
    if (index >= num) {
        FBTDBG("index(%d) out of bounds(%d)\n", index, num);
        goto error;
    }

    load_image(index);
    if (bmp_images[index].loaded)
    {
        //FBTDBG("get bmp image(%d), offset:%x\n", index, bmp_images[index].offset);
        return bmp_images[index].offset;
    }
error:
    FBTDBG("failed to get bmp image(%d)\n", index);
    return 0;
}

static inline int get_index_for_level(int level) {
    int i = 0;
    for (i = 0;i < IMAGES_NUM;i++) {
        if (level <= bmp_images[i].level)
            return i;
    }
    return 0;
}

static int get_next_image_index() {
    int level = batt_status.capacity;
    int start_index = get_index_for_level(level);
    //FBTDBG("level:%d, index:%d\n", level, start_index);

    if (!start_index)
        return 0; //low power

    current_index++;

#define LOOP_FROM_LEVEL0 1
#if LOOP_FROM_LEVEL0
    if (current_index == IMAGES_NUM
            || bmp_images[start_index].level < bmp_images[current_index].level
            //level overflow
            || bmp_images[start_index].level > bmp_images[current_index].level)
            //level changed
        return start_index; //loop again
#else
    if (current_index == IMAGES_NUM
            || bmp_images[start_index].level < bmp_images[current_index].level)
            //level overflow
        return 0;//loop again.
#endif//LOOP_FROM_LEVEL0

    return current_index; //step forward
}

static uint32_t get_next_image() {
    current_index = get_next_image_index();
    uint32_t addr = get_image(current_index);
    if (!addr)
        addr = get_image(0);
    //FBTDBG("get_next_image_index:%d, %x\n", current_index, addr);
    return addr;
}

static int sleep = false;
static long long screen_on_time = 0; // 178s may overflow...

#define DELAY 50000 //us
static inline int get_delay() {
    return sleep? DELAY << 1: DELAY;
}

static inline unsigned int get_fix_duration(unsigned int base) {
    unsigned int max = 0xFFFFFFFF / 24000;
    unsigned int now = get_timer(0);
    return base > now? base - now : max + (base - now) + 1;
}

#define POWER_LONG_PRESS_TIMEOUT    1500 //ms
#define SCREEN_DIM_TIMEOUT          60000 //ms
#define SCREEN_OFF_TIMEOUT          120000 //ms
#define BRIGHT_ON                   48
#define BRIGHT_DIM                  12
#define BRIGHT_OFF                  0

static inline void set_screen_state(int brightness, int force) {
    //FBTDBG("set_screen_state:%d\n", brightness);
    int was_sleep = sleep;

    switch(brightness) {
        case BRIGHT_ON:
        case BRIGHT_DIM:
            sleep = false;
            break;
        case BRIGHT_OFF:
            sleep = true;
            break;
    }
    if (was_sleep != sleep || force) { //switch state.
        //FBTDBG("screen state changed:%d -> %d\n", was_sleep, sleep);
	if(sleep == true){
	    rk_backlight_ctrl(brightness);
	    lcd_standby(sleep);
	}else{
		lcd_standby(sleep);
		mdelay(100);
		rk_backlight_ctrl(brightness);
	}
        screen_on_time = get_timer(0);
    }else{
	    rk_backlight_ctrl(brightness);
    }
}

void do_sleep()
{
	set_screen_state(BRIGHT_OFF, false);
	wait_for_interrupt();
	set_screen_state(BRIGHT_ON, false);
}


int do_charge(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int power_pressed_state = 0;
    int count = 0;

    get_power_bat_status(&batt_status);
//init status
    if(batt_status.state_of_chrg == 2)
        pmic_charger_setting(2);
    else 
		pmic_charger_setting(1);
    set_screen_state(BRIGHT_ON, true);

    while (1) {
		get_power_bat_status(&batt_status);
        if (!is_charging())
            goto shutdown;
        if(!batt_status.state_of_chrg)
        {
            pmic_charger_setting(0);
            goto shutdown;
        }

        power_pressed_state = power_hold();
        //printf("pressed state:%x\n", power_pressed_state);
		if(power_pressed_state > 0) {
			do_sleep();
			//printf("sleep end\n");
		} else if(power_pressed_state <0){
			//long pressed key, continue bootting.
			goto boot;
		}
        udelay(get_delay());
		if (!sleep) { //screen on and device idle
			unsigned int idle_time = get_fix_duration(screen_on_time);
			//printf("idle_time:%ld\n", idle_time);
			if (idle_time > SCREEN_OFF_TIMEOUT) {
				//printf("idle time out sleep\n");
				do_sleep();
			} else if (idle_time >= SCREEN_DIM_TIMEOUT) {
				//idle dim timeout
				//FBTDBG("dim\n");
				set_screen_state(BRIGHT_DIM, false);
			}
		}
        if (!sleep)
            lcd_display_bitmap_center(get_next_image());
    }
boot:
    set_screen_state(BRIGHT_OFF, false);
    printf("booting...\n");
    return 1;
shutdown:
    printf("shutting down...\n");
    set_screen_state(BRIGHT_OFF, false);
    shut_down();
    printf("not reach here.\n");
	return 0;
}

U_BOOT_CMD(
	charge ,    2,    1,     do_charge,
	"charge animation.",
	NULL
);
