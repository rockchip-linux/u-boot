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
#include <fastboot.h>
#include <power/pmic.h>
#include <resource.h>

#define LOGE(fmt, args...) FBTERR(fmt "\n", ##args)
#define LOGD(fmt, args...) FBTDBG(fmt "\n", ##args)

typedef struct {
	int max_level;
	int delay;
	char prefix[MAX_INDEX_ENTRY_PATH_LEN];
	int num;
	resource_content *images;
} anim_level_conf;

static anim_level_conf *level_confs = NULL;
static int level_conf_num = 0;
static int only_current_level = false;

#define DEF_CHARGE_DESC_PATH        "charge_anim_desc.txt"

#define OPT_CHARGE_ANIM_DELAY       "delay="
#define OPT_CHARGE_ANIM_LOOP_CUR    "only_current_level="
#define OPT_CHARGE_ANIM_LEVELS      "levels="
#define OPT_CHARGE_ANIM_LEVEL_CONF  "max_level="
#define OPT_CHARGE_ANIM_LEVEL_NUM   "num="
#define OPT_CHARGE_ANIM_LEVEL_PFX   "prefix="

/* we use this so that we can do without the ctype library */
#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int atoi(const char *s)
{
	int i = 0;
	int index = 0;
	while (is_digit(s[index]))
		i = i * 10 + s[index++] - '0';

	return i;
}

static bool parse_level_conf(const char *arg, anim_level_conf * level_conf)
{
	memset(level_conf, 0, sizeof(anim_level_conf));
	char *buf = NULL;
	buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_CONF);
	if (buf) {
		level_conf->max_level =
		    atoi(buf + strlen(OPT_CHARGE_ANIM_LEVEL_CONF));
	} else {
		LOGE("Not found:%s", OPT_CHARGE_ANIM_LEVEL_CONF);
		return false;
	}
	buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_NUM);
	if (buf) {
		level_conf->num = atoi(buf + strlen(OPT_CHARGE_ANIM_LEVEL_NUM));
		if (level_conf->num <= 0) {
			return false;
		}
	} else {
		LOGE("Not found:%s", OPT_CHARGE_ANIM_LEVEL_NUM);
		return false;
	}
	buf = strstr(arg, OPT_CHARGE_ANIM_DELAY);
	if (buf) {
		level_conf->delay = atoi(buf + strlen(OPT_CHARGE_ANIM_DELAY));
	}
	buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_PFX);
	if (buf) {
		snprintf(level_conf->prefix, sizeof(level_conf->prefix),
			 "%s", buf + strlen(OPT_CHARGE_ANIM_LEVEL_PFX));
	} else {
		LOGE("Not found:%s", OPT_CHARGE_ANIM_LEVEL_PFX);
		return false;
	}

	LOGD("Found conf:\nmax_level:%d, num:%d, delay:%d, prefix:%s",
	     level_conf->max_level, level_conf->num,
	     level_conf->delay, level_conf->prefix);
	return true;
}

static void free_level_confs()
{
	if (!level_confs)
		return;
	int i = 0, j = 0;
	for (i = 0; i < level_conf_num; i++) {
		if (level_confs[i].images) {
			for (j = 0; j < level_confs[i].num; j++) {
				free_content(level_confs[i].images[j]);
			}
			free(level_confs[i].images);
		}
	}
	free(level_confs);
	level_confs = NULL;
	level_conf_num = 0;
}

static bool load_anim_desc(const char *desc_path, bool dump)
{
	bool ret = false;

	//load content
	resource_content content;
	snprintf(content.path, sizeof(content.path), "%s", desc_path);
	content.load_addr = 0;
	if (!get_content(&content)) {
		goto end;
	}
	if (!load_content(&content)) {
		goto end;
	}

	char *buf = (char *)content.load_addr;
	char *end = buf + content.content_size - 1;
	*end = '\0';
	LOGD("desc:\n%s", buf);

	//splite lines.
	int pos = 0;
	while (1) {
		char *line = (char *)memchr(buf + pos, '\n', strlen(buf + pos));
		if (!line)
			break;
		*line = '\0';
		LOGD("splite:%s", buf + pos);
		pos += (strlen(buf + pos) + 1);
	}

	int delay = 900;
	int level_conf_pos = 0;

	free_level_confs();

	while (true) {
		if (buf >= end)
			break;
		const char *arg = buf;
		buf += (strlen(buf) + 1);

		LOGD("parse arg:%s", arg);
		if (!memcmp(arg, OPT_CHARGE_ANIM_LEVEL_CONF,
			    strlen(OPT_CHARGE_ANIM_LEVEL_CONF))) {
			if (!level_confs) {
				LOGE("Found level conf before levels!");
				goto end;
			}
			if (level_conf_pos >= level_conf_num) {
				LOGE("Too many level confs!(%d >= %d)",
				     level_conf_pos, level_conf_num);
				goto end;
			}
			if (!parse_level_conf
			    (arg, level_confs + level_conf_pos)) {
				LOGE("Failed to parse level conf:%s", arg);
				goto end;
			}
			level_conf_pos++;
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_DELAY,
				   strlen(OPT_CHARGE_ANIM_DELAY))) {
			delay = atoi(arg + strlen(OPT_CHARGE_ANIM_DELAY));
			LOGD("Found delay:%d", delay);
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_LOOP_CUR,
				   strlen(OPT_CHARGE_ANIM_LOOP_CUR))) {
			only_current_level =
			    !memcmp(arg + strlen(OPT_CHARGE_ANIM_LOOP_CUR),
				    "true", 4);
			LOGD("Found only_current_level:%d", only_current_level);
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_LEVELS,
				   strlen(OPT_CHARGE_ANIM_LEVELS))) {
			if (level_conf_num) {
				goto end;
			}
			level_conf_num =
			    atoi(arg + strlen(OPT_CHARGE_ANIM_LEVELS));
			if (!level_conf_num) {
				goto end;
			}
			level_confs =
			    (anim_level_conf *) malloc(level_conf_num *
						       sizeof(anim_level_conf));
			memset(level_confs, 0,
			       level_conf_num * sizeof(anim_level_conf));
			LOGD("Found levels:%d", level_conf_num);
		} else {
			LOGE("Unknown arg:%s", arg);
			goto end;
		}
	}

	if (level_conf_pos != level_conf_num || !level_conf_num) {
		LOGE("Something wrong with level confs!");
		goto end;
	}
	//sort level confs.
	int i = 0, j = 0, k = 0;
	for (i = 0; i < level_conf_num; i++) {
		if (!level_confs[i].delay) {
			level_confs[i].delay = delay;
		}
		if (!level_confs[i].delay) {
			LOGE("Missing delay in level conf:%d", i);
			goto end;
		}
		for (j = 0; j < i; j++) {
			if (level_confs[j].max_level ==
			    level_confs[i].max_level) {
				LOGE("Dup level conf:%d", i);
				goto end;
			}
			if (level_confs[j].max_level > level_confs[i].max_level) {
				anim_level_conf conf = level_confs[i];
				memmove(level_confs + j + 1, level_confs + j,
					(i - j) * sizeof(anim_level_conf));
				level_confs[j] = conf;
			}
		}
	}

	if (dump) {
		printf("Parse anim desc(%s):\n", desc_path);
		printf("only_current_level=%d\n", only_current_level);
		printf("level conf:\n");
		for (i = 0; i < level_conf_num; i++) {
			printf("\tmax=%d, delay=%d, num=%d, prefix=%s\n",
			       level_confs[i].max_level, level_confs[i].delay,
			       level_confs[i].num, level_confs[i].prefix);
		}
	}

	ret = true;
end:
	free_content(&content);
	if (!ret) {
		free_level_confs();
	}
	return ret;
}

#if 0
void test_anim_desc()
{
	load_anim_desc(DEF_CHARGE_DESC_PATH, true);
}
#endif

static int current_conf = 0;
static int current_index = 0;

static struct battery batt_status;

static resource_content *get_image_content()
{
	if (!level_confs
	    || current_conf >= level_conf_num || current_conf < 0
	    || current_index >= level_confs[current_conf].num
	    || current_index < 0) {
		LOGE("Inval params!");
		return NULL;
	}
	//alloc images.
	anim_level_conf *conf = level_confs + current_conf;
	if (!conf->images) {
		conf->images =
		    (resource_content *) malloc(conf->num *
						sizeof(resource_content));
		memset(conf->images, 0, conf->num * sizeof(resource_content));
	}
	if (!conf->images) {
		LOGE("Alloc images failed!");
		return NULL;
	}
	//load image.
	resource_content *image = conf->images + current_index;
	if (!image->load_addr) {
		if (conf->num == 1) {
			snprintf(image->path, sizeof(image->path), "%s.bmp",
				 conf->prefix);
		} else {
			int num = conf->num;
			int n = 0;
			while (num > 0) {
				num /= 10;
				n++;
			}
			char buf[30];
			snprintf(buf, sizeof(buf), "%%s%%0%dd.bmp", n);
			snprintf(image->path, sizeof(image->path), buf,
				 conf->prefix, current_index);
		}
		if (!get_content(image)) {
			LOGE("Failed to get content:%s", image->path);
			return NULL;
		}
		if (!load_content(image)) {
			LOGE("Failed to load content:%s", image->path);
			return NULL;
		}
	}
	if (!image->load_addr) {
		return NULL;
	}
	return image;
}

static uint32_t get_image()
{
	resource_content *image = get_image_content();
	if (!image) {
		//TODO:use a default error logo.
		FBTDBG("failed to get bmp image\n");
		return NULL;
	}
	return image->load_addr;
}

static void free_image()
{
	resource_content *image = get_image_content();
	if (image) {
		free_content(image);
	}
}

static inline int get_index_for_level(int level)
{
	int i = 0;
	for (i = 0; i < level_conf_num; i++) {
		if (level <= level_confs[i].max_level)
			return i;
	}
	return 0;
}

static uint32_t get_next_image()
{
	int level = batt_status.capacity;
	int actual_conf = get_index_for_level(level);
	//FBTDBG("level:%d, index:%d\n", level, next_conf);

	//step forward.
	current_index++;

	if (only_current_level) {
		if (actual_conf != current_conf) {
			//level changed
			//loop actual level.
			current_conf = actual_conf;
			current_index = 0;
		}
		if (current_index >= level_confs[current_conf].num) {
			//index overflow
			//loop current level
			current_index = 0;
		}
	} else {
		//mayby there is level 0 bmp for err.
		int start_conf = get_index_for_level(1);

		if (current_index >= level_confs[current_conf].num) {
			//index overflow, goto next level
			current_conf++;
			current_index = 0;
		}
		if (actual_conf < current_conf || current_conf < start_conf) {
			//level changed(down) or level overflow
			//loop from first level.
			current_conf = start_conf;
			current_index = 0;
		}
	}
	uint32_t addr = get_image();
	if (!addr) {
		//TODO:use default image?
		return 0;
	}
	return addr;
}

static void show_next_image()
{
	lcd_display_bitmap_center(get_next_image());
	free_image();
}

#define DELAY 80000		//us
static int get_anim_delay()
{
	if (!level_confs || current_conf >= level_conf_num || current_conf < 0)
		return DELAY;
	return level_confs[current_conf].delay * 1000;
}

static int sleep = false;
static long long power_hold_time = 0;	// hold 178s may overflow...
static long long screen_on_time = 0;	// 178s may overflow...

static inline int get_delay()
{
	return sleep ? DELAY << 1 : get_anim_delay();
}

static inline unsigned int get_fix_duration(unsigned int base)
{
	unsigned int max = 0xFFFFFFFF / 24000;
	unsigned int now = get_timer(0);
	return base > now ? base - now : max + (base - now) + 1;
}

#define POWER_LONG_PRESS_TIMEOUT    1500	//ms
#define SCREEN_DIM_TIMEOUT          60000	//ms
#define SCREEN_OFF_TIMEOUT          120000	//ms
#define BRIGHT_ON                   48
#define BRIGHT_DIM                  12
#define BRIGHT_OFF                  0

static inline void set_screen_state(int brightness, int force)
{
	//FBTDBG("set_screen_state:%d\n", brightness);
	int was_sleep = sleep;

	switch (brightness) {
	case BRIGHT_ON:
	case BRIGHT_DIM:
		sleep = false;
		break;
	case BRIGHT_OFF:
		sleep = true;
		break;
	}
	if (was_sleep != sleep || force) {	//switch state.
		//FBTDBG("screen state changed:%d -> %d\n", was_sleep, sleep);
		if (sleep == true) {
			rk_backlight_ctrl(brightness);
			lcd_standby(sleep);
		} else {
			lcd_standby(sleep);
			mdelay(100);
			rk_backlight_ctrl(brightness);
		}
		screen_on_time = get_timer(0);
	} else {
		rk_backlight_ctrl(brightness);
	}
}

void do_sleep()
{
	set_screen_state(BRIGHT_OFF, false);
	wait_for_interrupt();
	set_screen_state(BRIGHT_ON, false);
}

int do_charge(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	load_anim_desc(DEF_CHARGE_DESC_PATH, true);
	int power_pressed_state = 0;
	int count = 0;

	get_power_bat_status(&batt_status);
//init status
	if (batt_status.state_of_chrg == 2)
		pmic_charger_setting(2);
	else
		pmic_charger_setting(1);
	set_screen_state(BRIGHT_ON, true);

	int load_delay = 0;

	while (1) {
		get_power_bat_status(&batt_status);
		if (!is_charging())
			goto shutdown;
		if (!batt_status.state_of_chrg) {
			pmic_charger_setting(0);
			goto shutdown;
		}

		power_pressed_state = power_hold();
		//printf("pressed state:%x\n", power_pressed_state);
		if (power_pressed_state > 0) {
			do_sleep();
			//printf("sleep end\n");
		} else if (power_pressed_state < 0) {
			//long pressed key, continue bootting.
			goto boot;
		}
		udelay(get_delay() - load_delay);
		if (!sleep) {	//screen on and device idle
			unsigned int idle_time =
			    get_fix_duration(screen_on_time);
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
		if (!sleep) {
			load_delay = get_timer(0);
			show_next_image();
			load_delay = get_fix_duration(load_delay);
		}
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

U_BOOT_CMD(charge, 2, 1, do_charge, "charge animation.", NULL);
