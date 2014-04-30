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
#include <asm/arch/rkplat.h>


typedef enum GPIOIntType {
	GPIOLevelLow = 0,
	GPIOLevelHigh,	 
	GPIOEdgelFalling,
	GPIOEdgelRising
} eGPIOIntType_t;


static void rk_gpio_irq_unmask(void __iomem *regbase, unsigned int bit)
{
	rk_gpio_bit_op(regbase, GPIO_INTEN, bit, 1);
}

static void rk_gpio_irq_mask(void __iomem *regbase, unsigned int bit)
{
	rk_gpio_bit_op(regbase, GPIO_INTEN, bit, 0);
}

static void rk_gpio_irq_ack(void __iomem *regbase, unsigned int bit)
{
	rk_gpio_bit_op(regbase, GPIO_PORTS_EOI, bit, 1);
}

static void rk_gpio_set_intr_type(void __iomem *regbase, unsigned int bit, eGPIOIntType_t type)
{
	switch (type) {
		case GPIOLevelLow:
			rk_gpio_bit_op(regbase, GPIO_INT_POLARITY, bit, 0);
			rk_gpio_bit_op(regbase, GPIO_INTTYPE_LEVEL, bit, 0);
			break;
		case GPIOLevelHigh:
			rk_gpio_bit_op(regbase, GPIO_INTTYPE_LEVEL, bit, 0);
			rk_gpio_bit_op(regbase, GPIO_INT_POLARITY, bit, 1);
			break;
		case GPIOEdgelFalling:
			rk_gpio_bit_op(regbase, GPIO_INTTYPE_LEVEL, bit, 1);
			rk_gpio_bit_op(regbase, GPIO_INT_POLARITY, bit, 0);
			break;
		case GPIOEdgelRising:
			rk_gpio_bit_op(regbase, GPIO_INTTYPE_LEVEL, bit, 1);
			rk_gpio_bit_op(regbase, GPIO_INT_POLARITY, bit, 1);
			break;
	}
}


/**
 * Set gpio interrupt type
 */
static int gpio_irq_set_type(int gpio_irq, unsigned int type)
{
	int gpio = rk_gpio_irq_to_gpio(gpio_irq);
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);
	eGPIOIntType_t int_type = 0;

	if (bank == NULL) {
		return -1;
	}

	gpio &= RK_GPIO_PIN_MASK;
	if (gpio >= bank->ngpio) {
		return -1;
	}

	switch (type) {
		case IRQ_TYPE_EDGE_RISING:
			int_type = GPIOEdgelRising;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			int_type = GPIOEdgelFalling;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			int_type = GPIOLevelHigh;
			break;
		case IRQ_TYPE_LEVEL_LOW:
			int_type = GPIOLevelLow;
			break;
		default:
			return -EINVAL;
	}

	/* Before set intrrupt type, gpio must set input */
	rk_gpio_bit_op(bank->regbase, GPIO_SWPORT_DDR, offset_to_bit(gpio), GPIO_IN);

	rk_gpio_set_intr_type(bank->regbase, offset_to_bit(gpio), int_type);

	return 0;
}


/**
 * ACK gpio interrupt
 */
static int gpio_irq_ack(int gpio_irq)
{
	int gpio = rk_gpio_irq_to_gpio(gpio_irq);
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL) {
		return -1;
	}

	gpio &= RK_GPIO_PIN_MASK;
	if (gpio >= bank->ngpio) {
		return -1;
	}

	rk_gpio_irq_ack(bank->regbase, offset_to_bit(gpio));

	return 0;
}

/**
 * Enable gpio interrupt
 */
static int gpio_irq_enable(int gpio_irq)
{
	int gpio = rk_gpio_irq_to_gpio(gpio_irq);
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL) {
		return -1;
	}

	gpio &= RK_GPIO_PIN_MASK;
	if (gpio >= bank->ngpio) {
		return -1;
	}

	rk_gpio_irq_unmask(bank->regbase, offset_to_bit(gpio));

	return 0;
}


/**
 * Disable gpio interrupt
 */
static int gpio_irq_disable(int gpio_irq)
{
	int gpio = rk_gpio_irq_to_gpio(gpio_irq);
	struct rk_gpio_bank *bank = rk_gpio_get_bank(gpio);

	if (bank == NULL) {
		return -1;
	}

	gpio &= RK_GPIO_PIN_MASK;
	if (gpio >= bank->ngpio) {
		return -1;
	}

	rk_gpio_irq_mask(bank->regbase, offset_to_bit(gpio));

	return 0;
}


/**
 * General gpio interrupt handler
 */
static void gpio_irq_handler(int irq)
{
	struct rk_gpio_bank *bank = rk_gpio_id_to_bank(irq - IRQ_GPIO0);
	unsigned gpio_irq, pin;
	u32 isr, ilr;
	unsigned unmasked = 0;

	isr = __raw_readl(bank->regbase + GPIO_INT_STATUS);
	ilr = __raw_readl(bank->regbase + GPIO_INTTYPE_LEVEL);

	gpio_irq = bank->irq_base;

	while (isr) {
		pin = fls(isr) - 1;

		/* first mask and ack irq */
		rk_gpio_irq_mask(bank->regbase, offset_to_bit(pin));
		rk_gpio_irq_ack(bank->regbase, offset_to_bit(pin));

		/* if gpio is edge triggered, clear condition
		 * before executing the hander so that we don't
		 * miss edges
                 */
		if (ilr & (1 << pin)) {
			unmasked = 1;
			rk_gpio_irq_unmask(bank->regbase, offset_to_bit(pin));
		}

		generic_handle_irq(gpio_irq + pin, NULL);

		isr &= ~(1 << pin);

		if (!unmasked) {
			rk_gpio_irq_unmask(bank->regbase, offset_to_bit(pin));
		}
	}
}


/**
 * General gpio interrupt init
 */
static void gpio_irq_init(void)
{
	struct rk_gpio_bank *bank = NULL;
	int i = 0;

	debug("gpio_irq_init, default enable gpio group interrupt.\n");

	for(i = 0; i < GPIO_BANKS; i++) {
		bank = rk_gpio_id_to_bank(i);
		if (bank != NULL) {
			/* disable gpio pin interrupt */
			__raw_writel(0, bank->regbase + GPIO_INTEN);
			/* register gpio group irq handler */
			irq_install_handler(IRQ_GPIO0 + bank->id, (interrupt_handler_t *)gpio_irq_handler, NULL);
			/* default enable all gpio group interrupt */
			irq_handler_enable(IRQ_GPIO0 + bank->id);
		}
	}
}


static struct irq_chip gpio_irq_chip = {
	.name			= (const char*)"gpio",

	.irq_disable		= gpio_irq_disable,
	.irq_enable		= gpio_irq_enable,

	.irq_set_type		= gpio_irq_set_type,
};

