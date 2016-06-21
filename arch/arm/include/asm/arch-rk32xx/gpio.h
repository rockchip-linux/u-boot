/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_GPIO_H
#define __RKXX_GPIO_H

#include "rkplat.h"

#define GPIO_SWPORT_DR		0x00
#define GPIO_SWPORT_DDR		0x04
#define GPIO_INTEN		0x30
#define GPIO_INTMASK		0x34
#define GPIO_INTTYPE_LEVEL	0x38
#define GPIO_INT_POLARITY	0x3c
#define GPIO_INT_STATUS		0x40
#define GPIO_INT_RAWSTATUS	0x44
#define GPIO_DEBOUNCE		0x48
#define GPIO_PORTS_EOI		0x4c
#define GPIO_EXT_PORT		0x50
#define GPIO_LS_SYNC		0x60


/*
 * This is Linux-specific flags. By default controllers' and Linux' mapping
 * match, but GPIO controllers are free to translate their own flags to
 * Linux-specific in their .xlate callback. Though, 1:1 mapping is recommended.
 */
enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
};


/* rk gpio bank */
struct rk_gpio_bank {
	void __iomem *regbase;
	int id;
	int irq_base;
	int ngpio;
};


/*
 * rk gpio api define the gpio format as:
 * using 32 bit for rk gpio value,
 * the high 24bit of gpio is bank id, the low 8bit of gpio is pin number
 * eg: gpio = 0x00000108, it mean gpio1_b0, 0x00000100 is bank id of GPIO_BANK1, 0x00000008 is GPIO_B0
 */

/* bank and pin bit mask */
#define RK_GPIO_BANK_MASK	0xFFFFFF00
#define RK_GPIO_BANK_OFFSET	8
#define RK_GPIO_PIN_MASK	0x000000FF
#define RK_GPIO_PIN_OFFSET	0


/* gpio bank defined */
#if defined(CONFIG_RKCHIP_RK3288)
	#include "gpio-rk3288.h"
#elif defined(CONFIG_RKCHIP_RK3036)
	#include "gpio-rk3036.h"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128) || defined(CONFIG_RKCHIP_RK322X)
	#include "gpio-rk312X.h"
#else
	#error "PLS config gpio-rkxx.h!"
#endif


/* gpio pin define */
#define	GPIO_A0			0
#define	GPIO_A1			1
#define	GPIO_A2			2
#define	GPIO_A3			3
#define	GPIO_A4			4
#define	GPIO_A5			5
#define	GPIO_A6			6
#define	GPIO_A7			7
#define	GPIO_B0			8
#define	GPIO_B1			9
#define	GPIO_B2			10
#define	GPIO_B3			11
#define	GPIO_B4			12
#define	GPIO_B5			13
#define	GPIO_B6			14
#define	GPIO_B7			15
#define	GPIO_C0			16
#define	GPIO_C1			17
#define	GPIO_C2			18
#define	GPIO_C3			19
#define	GPIO_C4			20
#define	GPIO_C5			21
#define	GPIO_C6			22
#define	GPIO_C7			23
#define	GPIO_D0			24
#define	GPIO_D1			25
#define	GPIO_D2			26
#define	GPIO_D3			27
#define	GPIO_D4			28
#define	GPIO_D5			29
#define	GPIO_D6			30
#define	GPIO_D7			31


/* gpio bank and pin get */
#define RK_GPIO_BANK(gpio)		((gpio & RK_GPIO_BANK_MASK) >> RK_GPIO_BANK_OFFSET)
#define RK_GPIO_PIN(gpio)		((gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET)
#define RK_GPIO_BANK_VALID(gpio)	(RK_GPIO_BANK(gpio) < GPIO_BANKS)
#define RK_GPIO_PIN_VALID(gpio)		(RK_GPIO_PIN(gpio) < NUM_GROUP)


typedef enum eGPIOPinLevel {
	GPIO_LOW=0,
	GPIO_HIGH
} eGPIOPinLevel_t;

typedef enum eGPIOPinDirection {
	GPIO_IN=0,
	GPIO_OUT
} eGPIOPinDirection_t;

typedef enum GPIOPullType {
	PullDisable = 0,
	PullEnable = 1,
	GPIONormal = PullDisable,
	GPIOPullUp = 1,
	GPIOPullDown = 2
} eGPIOPullType_t;

typedef enum GPIODriveSlector {
	GPIODrv2mA = 0,
	GPIODrv4mA,
	GPIODrv8mA,
	GPIODrv12mA,
} eGPIODriveSlector_t;


static inline bool gpio_is_valid(int gpio)
{
	if ((gpio == INVALID_GPIO) || !RK_GPIO_BANK_VALID(gpio) || !RK_GPIO_PIN_VALID(gpio)) {
		debug("gpio = 0x%x is not valid!\n", gpio);
		return 0;
	}

	return 1;
}


#ifdef CONFIG_USE_IRQ
int rk_gpio_gpio_to_irq(unsigned gpio);
int rk_gpio_irq_to_gpio(unsigned irq);

static inline int gpio_to_irq(unsigned gpio)
{
	if (gpio_is_valid(gpio) == 0) {
		return INVALID_GPIO;
	}

	return rk_gpio_gpio_to_irq(gpio);
}

static inline int irq_to_gpio(unsigned irq)
{
	return rk_gpio_irq_to_gpio(irq);
}
#endif



static inline void rk_gpio_bit_op(void __iomem *regbase, unsigned int offset, uint32 bit, unsigned char flag)
{
	u32 val = __raw_readl(regbase + offset);

	if (flag)
		val |= bit;
	else
		val &= ~bit;

	__raw_writel(val, regbase + offset);
}

static inline unsigned pin_to_bit(unsigned pin)
{
	return 1u << pin;
}

static inline unsigned offset_to_bit(unsigned offset)
{
	return 1u << offset;
}


struct rk_gpio_bank *rk_gpio_get_bank(unsigned gpio);
struct rk_gpio_bank *rk_gpio_id_to_bank(unsigned int id);
int rk_gpio_base_to_bank(unsigned base);

#ifdef CONFIG_RK_GPIO_EXT_FUNC
int gpio_pull_updown(unsigned gpio, enum GPIOPullType type);
int gpio_drive_slector(unsigned gpio, enum GPIODriveSlector slector);
#endif /* CONFIG_RK_GPIO_EXT_FUNC */
int gpio_set_value(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_direction_input(unsigned gpio);

#endif	/* __RKXX_GPIO_H */
