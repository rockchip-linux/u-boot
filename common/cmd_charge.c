/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * charge animation functions
 */
#include <common.h>
#include <command.h>
#include <linux/sizes.h>

#include <power/battery.h>
#include <fastboot.h>
#include <malloc.h>
#include <lcd.h>
#include <power/pmic.h>
#include <resource.h>
#include <asm/arch/rkplat.h>

/*#define DEBUG*/
#define LOGE(fmt, args...) printf(fmt "\n", ##args)
#ifdef DEBUG
#define LOGD(fmt, args...) printf(fmt "\n", ##args)
#else
#define LOGD(...)
#endif

//screen timeouts.
#define SCREEN_DIM_TIMEOUT          15000 //ms
#define SCREEN_OFF_TIMEOUT          5000 //ms

//screen brightness state.
#define SCREEN_BRIGHT               0
#define SCREEN_DIM                  1
#define SCREEN_OFF                  2

#define IS_BRIGHT(brightness) 		(brightness != SCREEN_OFF)

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

#define BRIGHT_MAXLOW_BATTERY_CAPACITY 20

extern int rkkey_power_state(void);
extern int is_charging(void);
extern int pmic_charger_setting(int current);
#ifdef CONFIG_RK_PWM_BL
extern int rk_pwm_bl_config(int brightness);
#endif
extern void lcd_standby(int enable);
extern uint32 rk_timer1_get_curr_count(void);
extern int rk818_regulator_enable(int num_regulator);
extern int rk818_regulator_disable(int num_regulator);
extern void power_pmic_init(void);
extern void power_on_pmic(void);
extern void power_off_pmic(void);
extern bool board_fbt_exit_uboot_charge(void);

extern int uboot_brightness;

#ifdef CONFIG_POWER_FG_ADC
u8 g_increment = 0;
#define BIG_CHARGE_INCRE_TIME          100000
#define LITTLE_CHARGE_INCRE_TIME       300000
#endif

int timer_interrupt_wakeup = 0;


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

static int wfi_pwrkey_pressed = 0;

void rkkey_set_wfi_pwrkey_state(int pressed)
{
	wfi_pwrkey_pressed = pressed;
}

int rkkey_get_wfi_pwrkey_state(void)
{
	return wfi_pwrkey_pressed;
}

/**
 * check power key pressed state.
 */
int power_key_pressed(void) {
#ifdef CONFIG_INTERRUPT_KEY_DETECT
	//see: ./board/rockchip/common/key.c
	int power_pressed_state = rkkey_power_state();
	//printf("pressed state:%x\n", power_pressed_state);
	if(power_pressed_state > 0) {
		return KEY_SHORT_PRESSED;
	} else if(power_pressed_state <0){
		return KEY_LONG_PRESSED;
	}
	return KEY_NOT_PRESSED;
#else
	static unsigned int power_pressed_time = 0;
#ifdef CONFIG_CHARGE_DEEP_SLEEP
	int power_pressed = rkkey_power_state() | rkkey_get_wfi_pwrkey_state();
#else
	int power_pressed = rkkey_power_state();
#endif
	int power_pressed_state = KEY_NOT_PRESSED;

	if (!power_pressed_time) {
		//haven't pressed before.
		if (power_pressed) {
			//record press time.
			power_pressed_time = (get_timer(0) >> 1 << 1) + 1;
		}
		return KEY_NOT_PRESSED;
	} else {
		//power pressed before.
		if (power_pressed) {
			//still pressing
#define LONG_PRESSED_TIME 2000 //2s
			if (get_timer(power_pressed_time) >= LONG_PRESSED_TIME) {
				//long pressed.
				power_pressed_state = KEY_LONG_PRESSED;
			}
		} else {
			//power key released now.
			power_pressed_state = KEY_SHORT_PRESSED;
			power_pressed_time = 0;
		}
	}
	return power_pressed_state;
#endif
}

#ifdef CONFIG_ROCKCHIP_DISPLAY
extern int g_is_new_display;
extern bool rockchip_show_bmp(const char *bmp);
#else
static int g_is_new_display;
static bool rockchip_show_bmp(const char *bmp) {};
#endif

/**
 * set new brightness.
 */
void do_set_brightness(int brightness, int old_brightness) {
	if (brightness == old_brightness)
		return;
	LOGD("set_brightness: %d -> %d", old_brightness, brightness);
	if (IS_BRIGHT(brightness)) {
		if (!IS_BRIGHT(old_brightness) && !g_is_new_display) {
			lcd_standby(0);
			mdelay(100);
		}

#ifdef CONFIG_RK_PWM_BL
		rk_pwm_bl_config(brightness == SCREEN_BRIGHT ? -1 : CONFIG_BRIGHTNESS_DIM);
#endif
	} else {
		if (IS_BRIGHT(old_brightness)) {
			if (g_is_new_display) {
				rockchip_show_bmp(NULL);
			} else {
				#ifdef CONFIG_RK_PWM_BL
				rk_pwm_bl_config(0);
				#endif
				lcd_standby(1);
			}
		}
	}
}


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
	int ret;
	struct power_chrg chrg;
	/*if (!is_charging()) {
		LOGD("charger disconnceted.");
		return EXIT_SHUTDOWN;
	}*/
	get_power_bat_status(&batt_status);
	ret = get_power_charger_status(&chrg);
	// if no exist bat but charging
	if ((batt_status.state_of_chrg || (!ret && chrg.state_of_charger)) &&
	    (!batt_status.isexistbat))
	{
		printf("charging but no exist batterry!.");
		return EXIT_BOOT;
	}

	if (!batt_status.state_of_chrg &&
	    (ret == -ENODEV || !chrg.state_of_charger))
	{
		printf("pmic not charging.");
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
	int                 min_level;
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
#define OPT_CHARGE_ANIM_LEVEL_CONF  "min_level="
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
		level_conf->min_level = atoi(buf + strlen(OPT_CHARGE_ANIM_LEVEL_CONF));
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

	LOGD("Found conf:\nmin_level:%d, num:%d, delay:%d, prefix:%s",
			level_conf->min_level, level_conf->num,
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
	if (!get_content(0, &content)) {
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
			if (level_confs[j].min_level == level_confs[i].min_level) {
				LOGE("Dup level conf:%d", i);
				goto end;
			}
			if (level_confs[j].min_level > level_confs[i].min_level) {
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
			printf("\tmin=%d, delay=%d, num=%d, prefix=%s\n",
					level_confs[i].min_level, level_confs[i].delay,
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
	for (i = level_conf_num - 1;i >= 0;i--) {
		if (level >= level_confs[i].min_level)
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
		int end_conf = get_index_for_level(100);

		if (current_index >= level_confs[current_conf].num) {
			//index overflow, goto next level
			current_conf++;
			current_index = 0;
		}
		if (current_conf < actual_conf
				|| current_conf > end_conf) {
			//level changed(up) or level overflow
			//loop from actual level.
			current_conf = actual_conf;
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
	return IS_BRIGHT(state->brightness)? get_anim_delay()

		/* check duration while screen off */
		: get_anim_delay() << 1;
}


static inline void set_brightness(int brightness, screen_state* state) {
	if (IS_BRIGHT(state->brightness) && !IS_BRIGHT(brightness)) {
		LOGD("screen off!");
		state->screen_on_time = 0;
	} else if (!IS_BRIGHT(state->brightness) && IS_BRIGHT(brightness)) {
		LOGD("screen on!");
		state->screen_on_time = get_timer(0);
	}
	do_set_brightness(brightness, state->brightness);
	state->brightness = brightness;
}

#ifdef CONFIG_CHARGE_TIMER_WAKEUP

#define RK_CHARGE_TIMER_BASE		(RKIO_TIMER_BASE + 0x20)
#define RK_CHARGE_TIMER_IRQ		RKIRQ_TIMER1

int timer1_init(uint64_t count)
{
	//1 s interrupt
	writel(1*CONFIG_SYS_CLK_FREQ, RK_CHARGE_TIMER_BASE);
	/* auto reload & enable the timer */
	writel(0x05, RK_CHARGE_TIMER_BASE+0x10);

	writel(0x01, RK_CHARGE_TIMER_BASE+0x18);

	return 0;
}

void timer1_irq_init(void(*function)(void))
{
	irq_install_handler(RK_CHARGE_TIMER_IRQ, (interrupt_handler_t *)function, NULL);
			/* default enable all gpio group interrupt */
	irq_handler_enable(RK_CHARGE_TIMER_IRQ);
}

void timer1_irq_deinit(void)
{
	irq_handler_disable(RK_CHARGE_TIMER_IRQ);
	irq_uninstall_handler(RK_CHARGE_TIMER_IRQ);
}

uint32 rk_timer1_get_curr_count(void)
{
	return readl(RK_CHARGE_TIMER_BASE+0x08);
}

void rk_timer1_isr(void){
	//printf("rk_timer1_isr########\n");
	timer_interrupt_wakeup = 1;
	writel(0x01, RK_CHARGE_TIMER_BASE+0x18);
}
#endif /* CONFIG_CHARGE_TIMER_WAKEUP */


int do_charge(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	pre_charge();

	load_anim_desc(DEF_CHARGE_DESC_PATH, true);

	//init status
	screen_state g_state;
	memset(&g_state, 0, sizeof(g_state));
	g_state.brightness = SCREEN_OFF;

	unsigned int anim_time = 0;
	int brightness = SCREEN_DIM;
	int key_state = KEY_NOT_PRESSED;
	int exit_type = NOT_EXIT;

	#ifdef CONFIG_POWER_FG_ADC
	unsigned int charge_start_time = 0;
	unsigned int charge_last_time = 0;
	charge_start_time = get_timer(0);
	#endif
	printf("do_charge!!!!\n");

#ifdef CONFIG_CHARGE_DEEP_SLEEP
#ifdef CONFIG_CHARGE_TIMER_WAKEUP
	//timer 1s wakeup cpu
	timer1_init(CONFIG_SYS_CLK_FREQ);
#endif
	//close no use power
	power_pmic_init();
#endif /* CONFIG_CHARGE_DEEP_SLEEP */

	/* enbale fb buffer flip */
	lcd_enable_flip(true);

	while (1) {
		//step 1: check charger state.
		exit_type = check_charging();
		if (exit_type != NOT_EXIT) {
			LOGD("should quit charge");
			goto exit;
		}
	
		#ifdef CONFIG_POWER_FG_ADC
		charge_last_time = get_timer(charge_start_time);
		if(adc_charge_status()==2){
			if(charge_last_time > BIG_CHARGE_INCRE_TIME && g_increment<100){
				g_increment++;
				charge_start_time = get_timer(0);
				debug("increment is %d\n", g_increment);
			}
		}
		else if(adc_charge_status()==1){
			if(charge_last_time > LITTLE_CHARGE_INCRE_TIME && g_increment<100){
				g_increment++;
				charge_start_time = get_timer(0);
				debug("usb pc increment is %d\n", g_increment);
			}
		}
		#endif
		//step 2: handle timeouts.
		if (IS_BRIGHT(g_state.brightness)) {
			unsigned int idle_time = get_timer(g_state.screen_on_time);
			//printf("idle_time:%ld\n", idle_time);
			if (idle_time > SCREEN_OFF_TIMEOUT) {
				LOGD("screen off");
				brightness = SCREEN_OFF;
			}
	#if 0
			if (idle_time > SCREEN_OFF_TIMEOUT) {
				LOGD("screen off");
				brightness = SCREEN_OFF;
			} else if (idle_time >= SCREEN_DIM_TIMEOUT) {
				LOGD("screen dim");
				brightness = SCREEN_DIM;
			}
	#endif
		}

		//step 3: check power key pressed state.
		key_state = power_key_pressed();
		if (key_state) {
			LOGD("key pressed state:%d", key_state);
		}
		if (key_state == KEY_SHORT_PRESSED) {
			if (batt_status.capacity > BRIGHT_MAXLOW_BATTERY_CAPACITY)
				brightness = IS_BRIGHT(brightness) ?
						SCREEN_OFF : SCREEN_BRIGHT;
			else
				brightness = IS_BRIGHT(brightness) ?
						SCREEN_OFF : SCREEN_DIM;
		} else if(key_state == KEY_LONG_PRESSED){
			//long pressed key, continue bootting.
			if (handle_exit_charge() < 0) {
				continue;
			}
			exit_type = EXIT_BOOT;
			goto exit;
		} else if (board_fbt_exit_uboot_charge()) {
			if (handle_exit_charge() < 0) {
				continue;
			}
			exit_type = EXIT_BOOT;
			goto exit;
		}

#ifdef CONFIG_CHARGE_DEEP_SLEEP
		//step 4:try to do deep sleep.
		if (!IS_BRIGHT(brightness)) {
			//goto sleep, and wait for wakeup by power-key.
			set_brightness(SCREEN_OFF, &g_state);
			//printf("wakeup gpio init and sleep.\n");
			timer_interrupt_wakeup = 0;
			//close some ldo
			power_off_pmic();
#ifdef CONFIG_CHARGE_TIMER_WAKEUP
			//timer enable
			timer1_irq_init(rk_timer1_isr);
#endif
#ifdef CONFIG_PM_SUBSYSTEM
			rk_pm_wakeup_gpio_init();
			rk_pm_enter(NULL);
			rk_pm_wakeup_gpio_deinit();
#endif
#ifdef CONFIG_CHARGE_TIMER_WAKEUP
			timer1_irq_deinit();
#endif
			//close some ldo
  			power_on_pmic();
			
			mdelay(10);
			if(!timer_interrupt_wakeup){
				g_state.screen_on_time = 0;			
			}
		}
#endif

		//step 5:step anim when screen is on.
		if (uboot_brightness && IS_BRIGHT(brightness)) {
			//do anim when screen is on.
			unsigned int duration = get_timer(anim_time) * 1000;
			if (!IS_BRIGHT(g_state.brightness)
					|| duration >= get_delay(&g_state)) {
				anim_time = get_timer(0);
				update_image();
			}
		}

		//step 6:set brightness state.
		if (uboot_brightness)
			set_brightness(brightness, &g_state);
		else
			set_brightness(SCREEN_OFF, &g_state);

		//udelay(100);// 50ms.
	}
exit:
	/* disable fb buffer flip */
	lcd_enable_flip(false);
	set_brightness(SCREEN_OFF, &g_state);
	if (exit_type == EXIT_BOOT) {
		#ifdef CONFIG_POWER_FG_ADC
		if(fg_adc_storage_flag_load()==0)
		{
			debug("the increment is %d\n", g_increment);
			g_increment = g_increment + fg_adc_storage_load();
			debug("the increment 11111111111 is %d\n", g_increment);
		}
		#endif

		printf("booting...\n");
		return 1;
	} else if (exit_type == EXIT_SHUTDOWN) {
		#ifdef CONFIG_POWER_FG_ADC
		g_increment = g_increment + fg_adc_storage_load();
		fg_adc_storage_store(g_increment);
		fg_adc_storage_load();
		fg_adc_storage_flag_store(1);
		fg_adc_storage_flag_load();
		#endif
		printf("%s :shutting down...\n", __func__);
		shut_down();
		LOGE("%s : not reach here: %d \n", __func__, __LINE__);
		return 0;
	}
	LOGE("%s : not reach here :%d\n", __func__, __LINE__);
	return 0;
}

U_BOOT_CMD(
		charge ,    2,    1,     do_charge,
		"charge animation.",
		NULL
		);
