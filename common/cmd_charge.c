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

#define IMAGES_NUM (sizeof(bmp_images) / sizeof(bmp_image_t))

static int current_index = 0;
static struct battery batt_status;

static void load_image(int index)
{
    if (bmp_images[index].loaded)
        return;
#ifndef CONFIG_BMP_IMAGES_BUFFER
    return;
#endif
    if (((bmp_images[index].offset + bmp_images[index].size) * RK_BLK_SIZE)
            > CONFIG_BMP_IMAGES_SIZE) {
        FBTDBG("size overflow!\n");
        return;
    }
    uint32_t addr = CONFIG_BMP_IMAGES_BUFFER + bmp_images[index].offset * RK_BLK_SIZE;
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
    //FBTDBG("load image(%d), offset:%x, start:%x\n", index, addr + RK_BLK_SIZE, CONFIG_BMP_IMAGES_BUFFER);
    bmp_images[index].loaded = 1;
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

    if (bmp_images[current_index].level < level ||
            bmp_images[start_index].level != bmp_images[current_index].level)
        return start_index; //level changed

    if (current_index == IMAGES_NUM - 1)
        return current_index; //full level

    current_index++;
    if (bmp_images[start_index].level != bmp_images[current_index].level)
        return start_index; //loop again
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

static int sleep = 0;
static long long power_hold_time = 0; // hold 178s may overflow...

#define DELAY 80000 //us
static inline int get_delay() {
    return sleep? DELAY << 1: DELAY;
}

#define POWER_LONG_PRESS_TIMEOUT 1500 //ms
int do_charge(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    uint32_t addr = 0;
    int power_pressed = 0;
    int update_lcd = 1;
    int count = 0;
#define CHECK_POWER_DELAY 1000000
    get_power_bat_status(&batt_status);
    while (1) {
        udelay(get_delay());
        if (++count > (CHECK_POWER_DELAY / get_delay())) {
            get_power_bat_status(&batt_status);
            count = 0;
        }
        if (!is_charging())
            goto shutdown;
        power_pressed = power_hold();
        //FBTDBG("pressd:%x, hold:%d\n", power_pressed, power_hold_time);
        if (power_pressed) {
            if (!power_hold_time) //pressed power key
                power_hold_time = get_timer(0);
            else if (get_timer(power_hold_time) >= POWER_LONG_PRESS_TIMEOUT)
                //long pressed key
                goto boot;
        } else if (power_hold_time) { //released power key
            power_hold_time = 0;
            sleep = !sleep;
            update_lcd = 1;
        }

        //FBTDBG("sleep:%d, update_lcd:%d\n", sleep, update_lcd);
        if (!sleep)
            lcd_display_bitmap_center(get_next_image());

        if (update_lcd) {
            rk_backlight_ctrl(!sleep);
            lcd_standby(sleep);
        }
        update_lcd = 0;
    }
boot:
    rk_backlight_ctrl(0);
    lcd_standby(1);
    printf("booting...\n");
    return 1;
shutdown:
    printf("shutting down...\n");
    shut_down();
    printf("not reach here.\n");
	return 0;
}

U_BOOT_CMD(
	charge ,    2,    1,     do_charge,
	"charge animation.",
	NULL
);
