/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __KEY_H__
#define __KEY_H__

#define LONE_PRESS_TIME		1500//ms
#define KEY_LONG_PRESS		-1
#define KEY_SHORT_PRESS		1

#if defined(CONFIG_RKCHIP_RK3126)
#define KEY_ADC_CN		2
#elif defined(CONFIG_RKCHIP_RK322XH)
#define KEY_ADC_CN		0
#else
#define KEY_ADC_CN		1
#endif

typedef enum{
	KEY_NULL,
	KEY_AD,      // AD°´¼ü
	KEY_INT,
	KEY_REMOTE,
}KEY_TYPE;

typedef struct
{
	const char *name;   /* name of the fdt property defining this */
	uint gpio;      /* GPIO number, or FDT_GPIO_NONE if none */
	u8 flags;       /* FDT_GPIO_... flags */
}gpio_conf;


typedef struct
{
	uint32  index;
	uint32  keyValueLow;
	uint32  keyValueHigh;
	uint32	data;
	uint32  stas;
	uint32	ctrl;
}adc_conf;

typedef struct
{
	const char *name;   /* name of the fdt property defining this */
	uint gpio;      /* GPIO number, or FDT_GPIO_NONE if none */
	u8 flags;       /* FDT_GPIO_... flags */
	volatile uint32  pressed_state;
	volatile uint32  press_time;
}int_conf;

typedef struct {
	KEY_TYPE type;
	union{
		adc_conf    adc;
		int_conf    ioint;
	}key;
}key_config;

int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot);
int rkkey_power_state(void);
void key_init(void);
int is_charging(void);
void powerOn(void);
void powerOff(void);

#ifdef CONFIG_OF_LIBFDT
struct fdt_gpio_state *rkkey_get_powerkey(void);
#endif

#ifdef CONFIG_RK_PWM_REMOTE
int RemotectlInit(void);
int RemotectlDeInit(void);
#endif

#endif /* __KEY_H__ */
