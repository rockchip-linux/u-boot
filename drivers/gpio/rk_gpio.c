/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>

#define RK_GPIO_DRIVER_MODULE /* define for gpio.h */

#include <asm/arch/rkplat.h>

#define RKGPIO_VERSION		"1.2"


struct rk_gpio_bank *rk_gpio_id_to_bank(unsigned int id)
{
	int index;

	for(index = 0; index < ARRAY_SIZE(rk_gpio_banks); index++) {
		if (rk_gpio_banks[index].id == id) {
			return &(rk_gpio_banks[index]);
		}
	}

	debug("rk_gpio_id_to_bank error id = %d!\n", id);
	return NULL;
}


struct rk_gpio_bank *rk_gpio_get_bank(unsigned gpio)
{
	int id;

	if (!gpio_is_valid(gpio)) {
		return NULL;
	}

	id = (gpio & RK_GPIO_BANK_MASK) >> RK_GPIO_BANK_OFFSET;
	return rk_gpio_id_to_bank(id);
}


static int rk_gpio_base_to_id(unsigned int base)
{
	int index;

	for(index = 0; index < ARRAY_SIZE(rk_gpio_banks); index++) {
		if (rk_gpio_banks[index].regbase == base)
			return index;
	}

	debug("rk_gpio_base_to_id error base = 0x%x!\n", base);
	return -1;
}


int rk_gpio_base_to_bank(unsigned base)
{
	int bank = rk_gpio_base_to_id(base);

	if (bank == -1) {
		return -1;
	}

	return (bank << RK_GPIO_BANK_OFFSET);
}


#ifdef CONFIG_USE_IRQ
int rk_gpio_gpio_to_irq(unsigned gpio)
{
	int bank = 0, pin = 0;
	int index;

	if (!gpio_is_valid(gpio)) {
		return -1;
	}

	bank = (gpio & RK_GPIO_BANK_MASK) >> RK_GPIO_BANK_OFFSET;
	pin = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;

	for(index = 0; index < ARRAY_SIZE(rk_gpio_banks); index++) {
		if (rk_gpio_banks[index].id == bank)
			return (rk_gpio_banks[index].irq_base + pin);
	}

	debug("rk_gpio_gpio_to_irq error: gpio = %d\n", gpio);
	return -1;
}


int rk_gpio_irq_to_gpio(unsigned irq)
{
	int bank, pin;
	int index;

	bank = (irq - PIN_BASE) / NUM_GROUP;
	pin = (irq - PIN_BASE) % NUM_GROUP;

	for(index = 0; index < ARRAY_SIZE(rk_gpio_banks); index++) {
		if (rk_gpio_banks[index].id == bank) {
			return (bank << RK_GPIO_BANK_OFFSET) | (pin << RK_GPIO_PIN_OFFSET);
		}
	}

	debug("rk_gpio_irq_to_gpio error: irq = %d\n", irq);
	return -1;
}
#endif


static inline void rk_gpio_set_pin_level(void __iomem *regbase, unsigned int bit, eGPIOPinLevel_t level)
{
	rk_gpio_bit_op(regbase, GPIO_SWPORT_DR, bit, level);
}


static inline int rk_gpio_get_pin_level(void __iomem *regbase, unsigned int bit)
{
	return ((__raw_readl(regbase + GPIO_EXT_PORT) & bit) != 0);
}


static inline void rk_gpio_set_pin_direction(void __iomem *regbase, unsigned int bit, eGPIOPinDirection_t direction)
{
	rk_gpio_bit_op(regbase, GPIO_SWPORT_DDR, bit, direction);
}



/* Common GPIO API */

int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}


int gpio_free(unsigned gpio)
{
	return 0;
}


/**
 * Set gpio direction as input
 */
int gpio_direction_input(unsigned gpio)
{
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL)
		return -1;

	gpio = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;
	if (gpio >= bank->ngpio)
		return -1;

	rk_gpio_set_pin_direction(bank->regbase, offset_to_bit(gpio), GPIO_IN);

	return 0;
}


/**
 * Set gpio direction as output
 */
int gpio_direction_output(unsigned gpio, int value)
{
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL)
		return -1;

	gpio = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;
	if (gpio >= bank->ngpio)
		return -1;

	rk_gpio_set_pin_level(bank->regbase, offset_to_bit(gpio), value);
	rk_gpio_set_pin_direction(bank->regbase, offset_to_bit(gpio), GPIO_OUT);

	return 0;
}


/**
 * Get value of the specified gpio
 */
int gpio_get_value(unsigned gpio)
{
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL) {
		return -1;
	}

	gpio = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;
	if (gpio >= bank->ngpio) {
		return -1;
	}

	return rk_gpio_get_pin_level(bank->regbase, offset_to_bit(gpio));
}


/**
 * Set value of the specified gpio
 */
int gpio_set_value(unsigned gpio, int value)
{
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL) {
		return -1;
	}

	gpio = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;
	if (gpio >= bank->ngpio) {
		return -1;
	}

	rk_gpio_set_pin_level(bank->regbase, offset_to_bit(gpio), value);
	return 0;
}


/**
 * Set gpio pull up or down mode
 */
int gpio_pull_updown(unsigned gpio, enum GPIOPullType type)
{
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);
	void __iomem *base;
	u32 val;

	if (bank == NULL) {
		return -1;
	}

	gpio = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;
	if (gpio >= bank->ngpio) {
		return -1;
	}

#if defined(CONFIG_RKCHIP_RK3188)
	/*
	 * pull setting
	 * 2'b00: Z(Noraml operaton)
	 * 2'b01: weak 1(pull-up)
	 * 2'b10: weak 0(pull-down)
	 * 2'b11: Repeater(Bus keeper)
	 */
	switch (type) {
		case PullDisable:
			val = 0;
			break;
		case GPIOPullUp:
			val = 1;
			break;
		case GPIOPullDown:
			val = 2;
			break;
		default:
			return -EINVAL;
	}

	if (bank->id == 0 && gpio < 12) {
		base = (void __iomem *)(RKIO_PMU_PHYS + PMU_GPIO0A_PULL + ((gpio / 8) * 4));
		gpio = (gpio % 8) * 2;
		__raw_writel((0x3 << (16 + gpio)) | (val << gpio), base);
	} else {
		base = (void __iomem *)(RKIO_GRF_PHYS + GRF_GPIO0B_PULL - 4 + bank->id * 16 + ((gpio / 8) * 4));
		gpio = (7 - (gpio % 8)) * 2;
		__raw_writel((0x3 << (16 + gpio)) | (val << gpio), base);
	}
#elif defined(CONFIG_RKCHIP_RK3288)
	/*
	 * pull setting
	 * 2'b00: Z(Noraml operaton)
	 * 2'b01: weak 1(pull-up)
	 * 2'b10: weak 0(pull-down)
	 * 2'b11: Repeater(Bus keeper)
	 */
	switch (type) {
		case PullDisable:
			val = 0;
			break;
		case GPIOPullUp:
			val = 1;
			break;
		case GPIOPullDown:
			val = 2;
			break;
		default:
			return -EINVAL;
	}

	if (bank->id == 0) { /* gpio0, pmu control */
		if (gpio < (8+8+3)) { /* gpio0_a0-a8, gpio0_b0-b8, gpio0_c0-c2 */
			base = (void __iomem *)(RKIO_PMU_PHYS + PMU_GPIO0A_PULL + ((gpio / 8) * 4));
			gpio = (gpio % 8) * 2;
			__raw_writel((0x3 << (16 + gpio)) | (val << gpio), base);
		}
	} else { /* gpio1-gpio8, grf control */
		base = (void __iomem *)((RKIO_GRF_PHYS + (GRF_GPIO1D_P - 0xC) + (bank->id - 1) * 16) + ((gpio / 8) * 4));
		gpio = (7 - (gpio % 8)) * 2;
		__raw_writel((0x3 << (16 + gpio)) | (val << gpio), base);
	}
#elif defined(CONFIG_RKCHIP_RK3168)
	/* rk3168 do nothing */

#elif defined(CONFIG_RKCHIP_RK3066) || defined(CONFIG_RKCHIP_RK3036) \
	|| defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	/* RK30XX && RK292X */
	/*
	 * Values written to this register independently
	 * control Pullup/Pulldown or not for the
	 * corresponding data bit in GPIO.
	 * 0: pull up/down enable, PAD type will decide
	 * to be up or down, not related with this value
	 * 1: pull up/down disable
	*/
	val = (type == PullDisable) ? 1 : 0;
	base = RKIO_GRF_PHYS + GRF_GPIO0L_PULL + bank->id * 8 + ((gpio / 16) * 4);
	gpio = gpio % 16;
	__raw_writel((1 << (16 + gpio)) | (val << gpio), base);
#else
	#error "PLS config platform for gpio driver."
#endif

	return 0;
}


/**
 * gpio drive strength slector
 */
int gpio_drive_slector(unsigned gpio, enum GPIODriveSlector slector)
{
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);
	void __iomem *base;
	u32 val;

	if (bank == NULL) {
		return -1;
	}

	gpio = (gpio & RK_GPIO_PIN_MASK) >> RK_GPIO_PIN_OFFSET;
	if (gpio >= bank->ngpio) {
		return -1;
	}

#if defined(CONFIG_RKCHIP_RK3288)
	/*
	 * drive slector
	 * 2'b00: 2mA
	 * 2'b01: 4mA
	 * 2'b10: 8mA
	 * 2'b11: 12mA
	 */
	switch (slector) {
		case GPIODrv2mA:
			val = 0;
			break;
		case GPIODrv4mA:
			val = 1;
			break;
		case GPIODrv8mA:
			val = 2;
			break;
		case GPIODrv12mA:
			val = 3;
			break;
		default:
			return -EINVAL;
	}

	if (bank->id == 0) { /* gpio0, pmu control */
		if (gpio < (8+8+3)) { /* gpio0_a0-a8, gpio0_b0-b8, gpio0_c0-c2 */
			base = (void __iomem *)(RKIO_PMU_PHYS + PMU_GPIO0A_DRV + ((gpio / 8) * 4));
			gpio = (gpio % 8) * 2;
			__raw_writel((0x3 << (16 + gpio)) | (val << gpio), base);
		}
	} else { /* gpio1-gpio8, grf control */
		base = (void __iomem *)((RKIO_GRF_PHYS + (GRF_GPIO1D_E - 0xC) + (bank->id - 1) * 16) + ((gpio / 8) * 4));
		gpio = (7 - (gpio % 8)) * 2;
		__raw_writel((0x3 << (16 + gpio)) | (val << gpio), base);
	}
#elif defined(CONFIG_RKCHIP_RK3066) || defined(CONFIG_RKCHIP_RK3168) || defined(CONFIG_RKCHIP_RK3036) \
	|| defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	/* no drive config */
#else
	/* check chip if support gpio drive slector */
	#error "PLS config platform for gpio driver."
#endif

	return 0;
}
