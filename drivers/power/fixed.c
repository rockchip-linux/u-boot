/*
 *  Copyright (C) 2015 Samsung Electronics
 *
 *  Elaine <zhangqing@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <power/rockchip_power.h>
#include <errno.h>
#include <asm/arch/rkplat.h>
#include <fdtdec.h>

DECLARE_GLOBAL_DATA_PTR;

#define FIXED_COMPATIBLE		"regulator-fixed"

struct fixed_regulator {
	struct fdt_gpio_state fixed_gpio;
	unsigned int startup_delay_us;
	const char *name;
	int min_uV;
	int max_uV;
	int boot_on;
};

#if 0
static int fixed_regulator_get_value(struct fixed_regulator *dev_pdata)
{
	if (dev_pdata->min_uV != dev_pdata->max_uV) {
		debug("Invalid constraints for: %s\n", dev_pdata->name);
		return -EINVAL;
	}

	return dev_pdata->min_uV;
}

static bool fixed_regulator_get_enable(struct fixed_regulator *dev_pdata)
{
	/* Enable GPIO is optional */
	if (dev_pdata->fixed_gpio.gpio != INVALID_GPIO)
		return gpio_get_value(dev_pdata->fixed_gpio.gpio);
	else
		return true;
}
#endif

static int fixed_regulator_set_enable(struct fixed_regulator *dev_pdata,
				      bool enable)
{
	/* Enable GPIO is optional */
	if (dev_pdata->fixed_gpio.gpio != INVALID_GPIO) {
		gpio_set_value(dev_pdata->fixed_gpio.gpio, enable);
		if (enable && dev_pdata->startup_delay_us)
			udelay(dev_pdata->startup_delay_us);
	}

	return 0;
}

static int fixed_regulator_parse_dt(const void *blob, int node)
{
	struct fixed_regulator *dev_pdata;
	int ret = 0;

	dev_pdata = calloc(1, sizeof(struct fixed_regulator));
	if (!dev_pdata) {
		printf("%s: No available memory for allocation!\n", __func__);
		return -EINVAL;
	}

	/* Get fixed regulator optional enable GPIO desc */
	if (!fdtdec_decode_gpio(blob, node, "gpio",
				&dev_pdata->fixed_gpio)) {
		if (fdtdec_get_bool(blob, node, "enable-active-high"))
			dev_pdata->fixed_gpio.flags = 1;
		else
			dev_pdata->fixed_gpio.flags = 0;
		gpio_direction_output(dev_pdata->fixed_gpio.gpio,
				      dev_pdata->fixed_gpio.flags);
	}

	dev_pdata->startup_delay_us = fdtdec_get_int(blob, node,
						     "startup-delay-us",
						     0);
	dev_pdata->name = fdt_getprop(blob, node, "regulator-name", NULL);
	dev_pdata->min_uV = fdtdec_get_int(blob, node,
					   "regulator-min-microvolt",
					   0);
	dev_pdata->max_uV = fdtdec_get_int(blob, node,
					   "regulator-max-microvolt",
					   0);
	if (fdt_get_property(blob, node, "regulator-boot-on", NULL))
		dev_pdata->boot_on = 1;
	else
		dev_pdata->boot_on = 0;
	if (dev_pdata->boot_on)
		ret = fixed_regulator_set_enable(dev_pdata, dev_pdata->fixed_gpio.flags);

	return ret;
}


static void fixed_regulator_parse_node(const void *blob)
{
	int startoff = 0, node;

	do {
		node = fdt_node_offset_by_compatible(blob,
				startoff, FIXED_COMPATIBLE);
		if (node < 0) {
			printf("can't find dts node for fixed\n");
		} else {
			fixed_regulator_parse_dt(blob, node);
			startoff = node;
		}
	} while (node >= 0);
}

int fixed_regulator_init(void)
{
	if (!gd->fdt_blob)
		return -1;

	fixed_regulator_parse_node(gd->fdt_blob);
	return 0;
}
