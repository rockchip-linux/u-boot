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
#include <malloc.h>
#include <lcd.h>
#include <power/pmic.h>
#include <resource.h>

//#define DEBUG
#define LOGE(fmt, args...) printf(fmt "\n", ##args)
#ifdef DEBUG
#define LOGD(fmt, args...) printf(fmt "\n", ##args)
#else
#define LOGD(...)
#endif

//screen timeouts.
#define SCREEN_DIM_TIMEOUT          60000 //ms
#define SCREEN_OFF_TIMEOUT          120000 //ms

//screen brightness.
#define BRIGHT_ON                   48
#define BRIGHT_DIM                  12
#define BRIGHT_OFF                  0

//key pressed state.
#define KEY_NOT_PRESSED 			0
#define KEY_SHORT_PRESSED 			1
#define KEY_LONG_PRESSED 			2

//exit type.
#define NOT_EXIT             		0
#define EXIT_BOOT           		1
#define EXIT_SHUTDOWN            	2

#define DEF_CHARGE_DESC_PATH		"charge_anim_desc.txt"
#define DEFAULT_ANIM_DELAY			80000 //us

extern int power_hold(void);
extern int is_charging(void);
extern int check_charge(void);
extern int pmic_charger_setting(int current);
extern void rk_backlight_ctrl(int brightness);
extern void lcd_standby(int enable);

/***************board spec ops, maybe move these out of here.***************/
//define this when we dont have a worked battery.
//#define MOCK_CHARGER

#ifdef MOCK_CHARGER
#define get_power_bat_status(...)
#endif
static struct battery batt_status;
int get_battery_capacity(void) {
#ifdef MOCK_CHARGER
	return 50;
#endif
	//alreay update it before.
    //get_power_bat_status(&batt_status);
	return batt_status.capacity;
}

/**
 * check power key pressed state.
 */
int power_key_pressed(void) {
	//see: ./board/rockchip/common/key.c
	int power_pressed_state = power_hold();
	//printf("pressed state:%x\n", power_pressed_state);
	if(power_pressed_state > 0) {
		return KEY_SHORT_PRESSED;
	} else if(power_pressed_state <0){
		return KEY_LONG_PRESSED;
	}
	return KEY_NOT_PRESSED;
}

/**
 * set new brightness.
 */
void do_set_brightness(int brightness, int old_brightness) {
	if (brightness == old_brightness)
		return;
	if (brightness) {
		rk_backlight_ctrl(brightness);
		if (!old_brightness)
			lcd_standby(1);
	} else {
		if (old_brightness) {
			lcd_standby(0);
			mdelay(100);
		}
		rk_backlight_ctrl(0);
	}
}

#ifdef CONFIG_CHARGE_DEEP_SLEEP
/**
 * goto deep sleep and wait for interrupt.
 */
void wait_for_interrupt(void)
{
	/* PLL enter slow-mode */
	g_cruReg->cru_mode_con = (0x3<<((2*4) + 16)) | (0x0<<(2*4));
	g_cruReg->cru_mode_con = (0x3<<((3*4) + 16)) | (0x0<<(3*4));
	g_cruReg->cru_mode_con = (0x3<<((0*4) + 16)) | (0x0<<(0*4));

	wfi();

	/* PLL enter normal-mode */
	g_cruReg->cru_mode_con = (0x3<<((0*4) + 16)) | (0x1<<(0*4));
	g_cruReg->cru_mode_con = (0x3<<((3*4) + 16)) | (0x1<<(3*4));
	g_cruReg->cru_mode_con = (0x3<<((2*4) + 16)) | (0x1<<(2*4));

	printf("PLL open end! \n");
}
#endif

/**
 * do something before start charging.
 */
void pre_charge(void) {
    get_power_bat_status(&batt_status);
    if(batt_status.state_of_chrg == 2)
        pmic_charger_setting(2);
    else
        pmic_charger_setting(1);
}

/**
 * check if we need to continue charging
 * return exit type(EXIT_BOOT|EXIT_SHUTDOWN) when we want to exit.
 */
int check_charging(void) {
#ifdef MOCK_CHARGER
	return NOT_EXIT;
#endif
	if (!is_charging()) {
		LOGD("charger disconnceted.");
		return EXIT_SHUTDOWN;
	}
	get_power_bat_status(&batt_status);
	if(!batt_status.state_of_chrg)
	{
		LOGD("pmic not charging.");
		pmic_charger_setting(0);
		return EXIT_SHUTDOWN;
	}
	/*
	if (check cap enough)
		return EXIT_BOOT;
	*/
	return 0;
}

/**
 * cleanup before exit charge.
 * return -1 if we need to stay charging.
 */
int handle_exit_charge(void) {
	if (is_power_low()) {
		LOGE("low power, unable to boot");

		//TODO:show warning logo.
		show_resource_image("images/battery_fail.bmp");
		
		udelay(1000000);//1 sec.
		return -1;//unable to boot, so continue charging.
	}
	//do something before exit charge here.
	return 0;
}
/*****************************************************************/

typedef struct {
	int                 max_level;
	int                 delay;
	char                prefix[MAX_INDEX_ENTRY_PATH_LEN];
	int                 num;
} anim_level_conf;

static anim_level_conf* level_confs = NULL;
static int level_conf_num = 0;
static int only_current_level = false;
static char bat_err_path[MAX_INDEX_ENTRY_PATH_LEN];

#define OPT_CHARGE_ANIM_DELAY       "delay="
#define OPT_CHARGE_ANIM_LOOP_CUR    "only_current_level="
#define OPT_CHARGE_ANIM_LEVELS      "levels="
#define OPT_CHARGE_ANIM_BAT_ERROR   "bat_error="
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

static bool parse_level_conf(const char* arg, anim_level_conf* level_conf) {
	memset(level_conf, 0, sizeof(anim_level_conf));
	char* buf = NULL;
	buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_CONF);
	if (buf) {
		level_conf->max_level = atoi(buf + strlen(OPT_CHARGE_ANIM_LEVEL_CONF));
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

static void free_level_confs(void) {
	if (!level_confs)
		return;
	free(level_confs);
	level_confs = NULL;
	level_conf_num = 0;
}

static bool load_anim_desc(const char* desc_path, bool dump) {
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

	char* buf = (char*)content.load_addr;
	char* end = buf + content.content_size - 1;
	*end = '\0';
	LOGD("desc:\n%s", buf);

	//splite lines.
	int pos = 0;
	while (1) {
		char* line = (char*) memchr(buf + pos, '\n', strlen(buf + pos));
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
		const char* arg = buf;
		buf += (strlen(buf) + 1);

		LOGD("parse arg:%s", arg);
		if (arg[0] == '#' || !arg[0])
			continue;
		if (!memcmp(arg, OPT_CHARGE_ANIM_LEVEL_CONF,
					strlen(OPT_CHARGE_ANIM_LEVEL_CONF))) {
			if (!level_confs) {
				LOGE("Found level conf before levels!");
				goto end;
			}
			if (level_conf_pos >= level_conf_num) {
				LOGE("Too many level confs!(%d >= %d)", level_conf_pos, level_conf_num);
				goto end;
			}
			if (!parse_level_conf(arg, level_confs + level_conf_pos)) {
				LOGE("Failed to parse level conf:%s", arg);
				goto end;
			}
			level_conf_pos ++;
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_DELAY,
					strlen(OPT_CHARGE_ANIM_DELAY))) {
			delay = atoi(arg + strlen(OPT_CHARGE_ANIM_DELAY));
			LOGD("Found delay:%d", delay);
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_LOOP_CUR,
					strlen(OPT_CHARGE_ANIM_LOOP_CUR))) {
			only_current_level =
				!memcmp(arg + strlen(OPT_CHARGE_ANIM_LOOP_CUR), "true", 4);
			LOGD("Found only_current_level:%d", only_current_level);
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_LEVELS,
					strlen(OPT_CHARGE_ANIM_LEVELS))) {
			if (level_conf_num) {
				goto end;
			}
			level_conf_num = atoi(arg + strlen(OPT_CHARGE_ANIM_LEVELS));
			if (!level_conf_num) {
				goto end;
			}
			level_confs =
				(anim_level_conf*) malloc(level_conf_num * sizeof(anim_level_conf));
			memset(level_confs, 0, level_conf_num * sizeof(anim_level_conf));
			LOGD("Found levels:%d", level_conf_num);
		} else if (!memcmp(arg, OPT_CHARGE_ANIM_BAT_ERROR,
					strlen(OPT_CHARGE_ANIM_BAT_ERROR))) {
			snprintf(bat_err_path, sizeof(bat_err_path), "%s",
					arg + strlen(OPT_CHARGE_ANIM_BAT_ERROR));
			LOGD("Found battery error image:%s", bat_err_path);
		} else {
			LOGE("Unknown arg:%s", arg);
			continue;
		}
	}

	if (level_conf_pos != level_conf_num || !level_conf_num) {
		LOGE("Something wrong with level confs!");
		goto end;
	}

	//sort level confs.
	int i = 0, j = 0;
	for (i = 0; i < level_conf_num; i++) {
		if (!level_confs[i].delay) {
			level_confs[i].delay = delay;
		}
		if (!level_confs[i].delay) {
			LOGE("Missing delay in level conf:%d", i);
			goto end;
		}
		for (j = 0; j < i; j++) {
			if (level_confs[j].max_level == level_confs[i].max_level) {
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
void test_anim_desc(void) {
	load_anim_desc(DEF_CHARGE_DESC_PATH, true);
}
#endif

static int current_conf = 0;
static int current_index = 0;

static bool show_image(void) {
	if (!level_confs
			|| current_conf >= level_conf_num || current_conf < 0
			|| current_index >= level_confs[current_conf].num
			|| current_index < 0) {
		LOGE("Inval params!");
		return false;
	}

	//generate image path.
	anim_level_conf* conf = level_confs + current_conf;
	char path[MAX_INDEX_ENTRY_PATH_LEN];
	if (conf->num == 1) {
		snprintf(path, sizeof(path), "%s.bmp",
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
		snprintf(path, sizeof(path), buf,
				conf->prefix, current_index);
	}

	LOGD("show image:%s", path);
	return show_resource_image(path);
}

static inline int get_index_for_level(int level) {
	int i = 0;
	for (i = 0;i < level_conf_num;i++) {
		if (level <= level_confs[i].max_level)
			return i;
	}
	return 0;
}

static void update_image(void) {
	int level = get_battery_capacity();
	int actual_conf = get_index_for_level(level);
	LOGD("level:%d, index:%d", level, actual_conf);

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
		int start_conf = get_index_for_level(0);

		if (current_index >= level_confs[current_conf].num) {
			//index overflow, goto next level
			current_conf++;
			current_index = 0;
		}
		if (actual_conf < current_conf
				|| current_conf < start_conf) {
			//level changed(down) or level overflow
			//loop from first level.
			current_conf = start_conf;
			current_index = 0;
		}
	}
	show_image();
}

static int get_anim_delay(void) {
	if (!level_confs
			|| current_conf >= level_conf_num || current_conf < 0)
		return DEFAULT_ANIM_DELAY;
	return level_confs[current_conf].delay * 1000;
}

typedef struct _screen_state {
	int brightness;
	long long screen_on_time; // 178s may overflow...
} screen_state;

static inline int get_delay(const screen_state* state) {
	return state->brightness? get_anim_delay()

		/* check duration while screen off */
		: DEFAULT_ANIM_DELAY << 1;
}

static inline unsigned int get_fix_duration(unsigned int base) {
	unsigned int max = 0xFFFFFFFF / 24000;
	unsigned int now = get_timer(0);
	return base > now? base - now : max + (base - now) + 1;
}

static inline void set_brightness(int brightness, screen_state* state) {
	LOGD("set_brightness: %d -> %d", state->brightness, brightness);
	if (state->brightness && !brightness) {
		LOGD("screen off!");
		state->screen_on_time = 0;
	} else if (!state->brightness && brightness) {
		LOGD("screen on!");
		state->screen_on_time = get_timer(0);
	}
	do_set_brightness(brightness, state->brightness);
	state->brightness = brightness;
}

int do_charge(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	pre_charge();

	load_anim_desc(DEF_CHARGE_DESC_PATH, true);

	//init status
	screen_state g_state;
	memset(&g_state, 0, sizeof(g_state));

	int load_delay = 0;
	int brightness = BRIGHT_ON;
	int key_state = KEY_NOT_PRESSED;
	int exit_type = NOT_EXIT;
	while (1) {
		//step 1: check charger state.
		exit_type = check_charging();
		if (exit_type != NOT_EXIT) {
			LOGD("should quit charge");
			goto exit;
		}

		//step 2: handle timeouts.
		if (g_state.brightness) {
			unsigned int idle_time = get_fix_duration(g_state.screen_on_time);
			//printf("idle_time:%ld\n", idle_time);
			if (idle_time > SCREEN_OFF_TIMEOUT) {
				LOGD("screen off");
				brightness = BRIGHT_OFF;
			} else if (idle_time >= SCREEN_DIM_TIMEOUT) {
				LOGD("screen dim");
				brightness = BRIGHT_DIM;
			}
		}

		//step 3: check power key pressed state.
		key_state = power_key_pressed();
		LOGD("key pressed state:%d", key_state);
		if (key_state == KEY_SHORT_PRESSED) {
			brightness = g_state.brightness? BRIGHT_OFF : BRIGHT_ON;
#ifdef CONFIG_CHARGE_DEEP_SLEEP
			if (brightness) {
				//should not reach here!
				LOGE("screen state error!");
				brightness = BRIGHT_OFF;
			}
#endif
		} else if(key_state == KEY_LONG_PRESSED){
			//long pressed key, continue bootting.
			if (handle_exit_charge() < 0) {
				continue;
			}
			exit_type = EXIT_BOOT;
			goto exit;
		}

		//step 4: update anim & set brightness.
		if (brightness) {
			//do anim when screen is on.
			load_delay = get_timer(0);
			update_image();
			load_delay = get_fix_duration(load_delay);
		} else {
			//screen off.
#ifdef CONFIG_CHARGE_DEEP_SLEEP
			//goto sleep, and wait for wakeup by power-key.
			set_brightness(BRIGHT_OFF, &g_state);
			wait_for_interrupt();
			brightness = BRIGHT_ON;
#endif
		}
		set_brightness(brightness, &g_state);

		udelay(get_delay(&g_state) - load_delay);
		load_delay = 0;
	}
exit:
	set_brightness(BRIGHT_OFF, &g_state);
	if (exit_type == EXIT_BOOT) {
		printf("booting...\n");
		return 1;
	} else if (exit_type == EXIT_SHUTDOWN) {
		printf("shutting down...\n");
		shut_down();
		LOGE("not reach here.\n");
		return 0;
	}
	LOGE("not reach here.\n");
	return 0;
}

U_BOOT_CMD(
		charge ,    2,    1,     do_charge,
		"charge animation.",
		NULL
		);
