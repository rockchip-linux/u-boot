/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <time.h>
#include <irq-generic.h>
#include <button.h>
#include <linux/input.h>
#include <power/rk8xx_pmic.h>

struct button_rk8xx_priv {
	u64 rise_ms;
	u64 fall_ms;
};

static enum button_state_t button_rk8xx_get_state(struct udevice *dev)
{
	struct button_rk8xx_priv *priv = dev_get_priv(dev);
	int state;

	debug("rk8xx button: rise=%llums, down=%llums, delta=%llums\n",
	      priv->rise_ms, priv->fall_ms,
	      priv->rise_ms - priv->fall_ms);

	/* Possible this is machine power-on long pressed, so ignore this */
	if (priv->fall_ms == 0 && priv->rise_ms != 0) {
		state = BUTTON_OFF;
		return state;
	}

	if ((priv->rise_ms > priv->fall_ms) &&
	    (priv->rise_ms - priv->fall_ms) >= BUTTON_ON_HOLD_MS) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON_HOLD;
		debug("rk8xx button: hold on (released)\n");
	} else if (priv->fall_ms &&
		   get_timer(priv->fall_ms) >= BUTTON_ON_HOLD_MS) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON_HOLD;
		debug("rk8xx button: hold on\n");
	} else if ((priv->rise_ms > priv->fall_ms) &&
		   (priv->rise_ms - priv->fall_ms) < BUTTON_ON_HOLD_MS) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON;
		debug("rk8xx button: on\n");
	/* Possible in charge animation, we enable irq after fuel gauge updated */
	} else if (priv->rise_ms && priv->fall_ms &&
		   (priv->rise_ms == priv->fall_ms)) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON;
		debug("rk8xx button: on\n");
	} else {
		state = BUTTON_OFF;
	}

	return state;
}

static void rk8xx_pwron_rise_handler(int irq, void *data)
{
	struct udevice *dev = data;
	struct button_rk8xx_priv *priv = dev_get_priv(dev);

	priv->rise_ms = get_timer(0);
	debug("rk8xx button: rise irq: %lldms\n", priv->rise_ms);
}

static void rk8xx_pwron_fall_handler(int irq, void *data)
{
	struct udevice *dev = data;
	struct button_rk8xx_priv *priv = dev_get_priv(dev);

	priv->fall_ms = get_timer(0);
	debug("rk8xx button: fall irq: %lldms\n", priv->fall_ms);
}

static int button_rk8xx_bind(struct udevice *dev)
{
	struct button_uc_plat *uc_plat = dev_get_uclass_plat(dev);

	uc_plat->code = KEY_POWER;	/* Don't init in probe() */

	return 0;
}

static int button_rk8xx_probe(struct udevice *dev)
{
	struct rk8xx_priv *rk8xx = dev_get_priv(dev->parent);
	int fall_irq, rise_irq;
	int ret;

	if (!rk8xx->irq_chip) {
		printf("Failed to get parent irq chip\n");
		return -ENOENT;
	}

	fall_irq = virq_to_irq(rk8xx->irq_chip, RK8XX_IRQ_PWRON_FALL);
	if (fall_irq < 0) {
		printf("Failed to register pwron fall irq, ret=%d\n", fall_irq);
		return fall_irq;
	}

	rise_irq = virq_to_irq(rk8xx->irq_chip, RK8XX_IRQ_PWRON_RISE);
	if (rise_irq < 0) {
		printf("Failed to register pwron rise irq, ret=%d\n", rise_irq);
		return rise_irq;
	}


	irq_install_handler(fall_irq, rk8xx_pwron_fall_handler, dev);
	irq_install_handler(rise_irq, rk8xx_pwron_rise_handler, dev);
	ret = irq_handler_enable(fall_irq);
	if (ret) {
		printf("rk8xx button: enable rise irq failed, ret=%d\n", ret);
		return ret;
	}

	ret = irq_handler_enable(rise_irq);
	if (ret) {
		printf("rk8xx button: enable fall irq failed, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static const struct button_ops button_rk8xx_ops = {
	.get_state	= button_rk8xx_get_state,
};

U_BOOT_DRIVER(button_rk8xx) = {
	.name		= "button_rk8xx",
	.id		= UCLASS_BUTTON,
	.ops		= &button_rk8xx_ops,
	.priv_auto	= sizeof(struct button_rk8xx_priv),
	.probe		= button_rk8xx_probe,
	.bind		= button_rk8xx_bind,
};
