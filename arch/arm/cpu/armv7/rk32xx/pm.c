/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKPM_VERSION		"1.2"

#ifdef CONFIG_OF_LIBFDT
extern struct fdt_gpio_state *rkkey_get_powerkey(void);
#endif

extern void rkkey_set_wfi_pwrkey_state(int pressed);
extern int rkkey_get_wfi_pwrkey_state(void);
extern int rkkey_power_state(void);

/*
 * rkpm wakeup gpio init
 */
void rk_pm_wakeup_gpio_init(void)
{
	int irq = INVALID_GPIO, wakeup_gpio = INVALID_GPIO;
#ifdef CONFIG_OF_LIBFDT
	struct fdt_gpio_state * gpio_dt = NULL;

	gpio_dt = rkkey_get_powerkey();
	if (gpio_dt != NULL)
		wakeup_gpio = gpio_dt->gpio;
#endif
	irq = gpio_to_irq(wakeup_gpio);
	if (irq != INVALID_GPIO) {
		/* gpio pin just use to wakeup, no need isr handle */
		irq_install_handler(irq, (interrupt_handler_t *)-1, NULL);
		irq_set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);
		irq_handler_enable(irq);
	}
}


/*
 * rkpm wakeup gpio deinit
 */
void rk_pm_wakeup_gpio_deinit(void)
{
	int irq = INVALID_GPIO, wakeup_gpio = INVALID_GPIO;
#ifdef CONFIG_OF_LIBFDT
	struct fdt_gpio_state * gpio_dt = NULL;

	gpio_dt = rkkey_get_powerkey();
	if (gpio_dt != NULL)
		wakeup_gpio = gpio_dt->gpio;
#endif
	irq = gpio_to_irq(wakeup_gpio);
	if (irq != INVALID_GPIO) {
		irq_handler_disable(irq);
		irq_uninstall_handler(irq);
	}
}


/*
 * rkpm enter
 * module_pm_conf: callback function that control such as backlight/lcd on or off
 */
void rk_pm_enter(v_pm_cb_f module_pm_conf)
{
	/* disable exceptions */
	disable_interrupts();

	if (module_pm_conf != NULL)
		module_pm_conf(0);

	/* pll enter slow mode */
	rkclk_pll_mode(CPLL_ID, RKCLK_PLL_MODE_SLOW);
	rkclk_pll_mode(GPLL_ID, RKCLK_PLL_MODE_SLOW);
	rkclk_pll_mode(APLL_ID, RKCLK_PLL_MODE_SLOW);

	/* not pressed before, so wfi */
	if (!rkkey_get_wfi_pwrkey_state()) {
		wfi();
		/* mark pressed state to notifier framework */
		rkkey_set_wfi_pwrkey_state(rkkey_power_state());

	/*
	 * if pressed before, not wfi and release key which
	 * allow framework to report a short key pressed event
	 */
	} else {
		rkkey_set_wfi_pwrkey_state(0);
	}

	/* pll enter nornal mode */
	rkclk_pll_mode(APLL_ID, RKCLK_PLL_MODE_NORMAL);
	rkclk_pll_mode(GPLL_ID, RKCLK_PLL_MODE_NORMAL);
	rkclk_pll_mode(CPLL_ID, RKCLK_PLL_MODE_NORMAL);


	if (module_pm_conf != NULL)
		module_pm_conf(1);

	/* enable exceptions */
	enable_interrupts();
}
