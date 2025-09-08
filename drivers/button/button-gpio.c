// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Philippe Reynes <philippe.reynes@softathome.com>
 */

#include <button.h>
#include <dm.h>
#include <log.h>
#include <time.h>
#include <asm/gpio.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <linux/input.h>
#include <linux/delay.h>
#ifdef CONFIG_IRQ
#include <irq-generic.h>
#endif

struct button_gpio_priv {
	struct gpio_desc gpio;
	int linux_code;
	u64 rise_ms;
	u64 fall_ms;
	int irq;
};

#ifdef CONFIG_IRQ
static enum button_state_t button_gpio_get_pwrkey_state(struct udevice *dev)
{
	struct button_gpio_priv *priv = dev_get_priv(dev);
	int state;

	debug("gpio button: rise=%llums, down=%llums, delta=%llums\n",
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
		debug("gpio button: hold on (released)\n");
	} else if (priv->fall_ms &&
		   get_timer(priv->fall_ms) >= BUTTON_ON_HOLD_MS) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON_HOLD;
		debug("gpio button: hold on\n");
	} else if ((priv->rise_ms > priv->fall_ms) &&
		   (priv->rise_ms - priv->fall_ms) < BUTTON_ON_HOLD_MS) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON;
		debug("gpio button: on\n");
	/* Possible in charge animation, we enable irq after fuel gauge updated */
	} else if (priv->rise_ms && priv->fall_ms &&
		   (priv->rise_ms == priv->fall_ms)) {
		priv->rise_ms = 0;
		priv->fall_ms = 0;
		state = BUTTON_ON;
		debug("gpio button: on\n");
	} else {
		state = BUTTON_OFF;
	}

	return state;
}
#endif

static enum button_state_t button_gpio_get_state(struct udevice *dev)
{
	struct button_gpio_priv *priv = dev_get_priv(dev);
	int ret;

	if (!priv)
		return -ENODATA;
#ifdef CONFIG_IRQ
	if (priv->linux_code == KEY_POWER)
		return button_gpio_get_pwrkey_state(dev);
#endif
	if (!dm_gpio_is_valid(&priv->gpio))
		return -EREMOTEIO;
	ret = dm_gpio_get_value(&priv->gpio);
	if (ret < 0)
		return ret;

	return ret ? BUTTON_ON : BUTTON_OFF;
}

static int button_gpio_get_code(struct udevice *dev)
{
	struct button_gpio_priv *priv = dev_get_priv(dev);
	if (!priv)
		return -ENODATA;
	int code = priv->linux_code;

	if (!code)
		return -ENODATA;

	return code;
}

#ifdef CONFIG_IRQ
static void button_gpio_irq_handler(int irq, void *data)
{
	struct udevice *dev = data;
	struct button_gpio_priv *priv = dev_get_priv(dev);

	if (priv->irq != irq)
		return;

	if (irq_get_gpio_level(irq)) {
		priv->rise_ms = get_timer(0);
		debug("button gpio: down %llu ms\n", priv->fall_ms);
	} else {
		priv->fall_ms = get_timer(0);
		debug("button gpio: up %llu ms\n", priv->rise_ms);
	}

	/* Must delay */
	mdelay(10);
	irq_revert_irq_type(irq);
}

static int button_gpio_request_irq(struct udevice *dev)
{
	struct button_gpio_priv *priv = dev_get_priv(dev);
	u32 gpios[2];
	int ret;

	if (dev_read_u32_array(dev, "gpios", gpios, ARRAY_SIZE(gpios)))
		return -EINVAL;

	priv->irq = phandle_gpio_to_irq(gpios[0], gpios[1]);
	if (priv->irq < 0) {
		printf("button gpio: failed to request irq, ret=%d\n", priv->irq);
		return priv->irq;
	}

	irq_install_handler(priv->irq, button_gpio_irq_handler, dev);
	irq_set_irq_type(priv->irq, IRQ_TYPE_EDGE_FALLING);
	ret = irq_handler_enable(priv->irq);
	if (ret) {
		printf("button gpio: enable irq failed, ret=%d\n", ret);
		return ret;
	}

	return 0;
}
#endif

static int button_gpio_probe(struct udevice *dev)
{
	struct button_uc_plat *uc_plat = dev_get_uclass_plat(dev);
	struct button_gpio_priv *priv = dev_get_priv(dev);
	int ret;

	/* Ignore the top-level button node */
	if (!uc_plat->label)
		return 0;

	ret = gpio_request_by_name(dev, "gpios", 0, &priv->gpio, GPIOD_IS_IN);
	if (ret || !dm_gpio_is_valid(&priv->gpio))
		return ret;

	ret = dev_read_u32(dev, "linux,code", &priv->linux_code);
#ifdef CONFIG_IRQ
	return (priv->linux_code == KEY_POWER) ? button_gpio_request_irq(dev) : ret;
#else
	return ret;
#endif
}

static int button_gpio_remove(struct udevice *dev)
{
	/*
	 * The GPIO driver may have already been removed. We will need to
	 * address this more generally.
	 */
	if (!IS_ENABLED(CONFIG_SANDBOX)) {
		struct button_gpio_priv *priv = dev_get_priv(dev);

		if (dm_gpio_is_valid(&priv->gpio))
			dm_gpio_free(dev, &priv->gpio);
	}

	return 0;
}

static int button_gpio_bind(struct udevice *parent)
{
	struct udevice *dev;
	ofnode node;
	int ret;

	dev_for_each_subnode(node, parent) {
		struct button_uc_plat *uc_plat;
		const char *label;

		label = ofnode_read_string(node, "label");
		if (!label) {
			debug("%s: node %s has no label\n", __func__,
			      ofnode_get_name(node));
			return -EINVAL;
		}
		ret = device_bind_driver_to_node(parent, "button_gpio",
						 ofnode_get_name(node),
						 node, &dev);
		if (ret)
			return ret;
		uc_plat = dev_get_uclass_plat(dev);
		uc_plat->label = label;
		debug("Button '%s' bound to driver '%s'\n", label,
		      dev->driver->name);
	}

	return 0;
}

static const struct button_ops button_gpio_ops = {
	.get_state	= button_gpio_get_state,
	.get_code	= button_gpio_get_code,
};

static const struct udevice_id button_gpio_ids[] = {
	{ .compatible = "gpio-keys" },
	{ .compatible = "gpio-keys-polled" },
	{ }
};

U_BOOT_DRIVER(button_gpio) = {
	.name		= "button_gpio",
	.id		= UCLASS_BUTTON,
	.of_match	= button_gpio_ids,
	.ops		= &button_gpio_ops,
	.priv_auto	= sizeof(struct button_gpio_priv),
	.bind		= button_gpio_bind,
	.probe		= button_gpio_probe,
	.remove		= button_gpio_remove,
};
