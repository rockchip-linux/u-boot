/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <resource.h>
#include <asm/arch/rkplat.h>
#include <asm/unaligned.h>
#include <linux/list.h>
#include <linux/media-bus-format.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_panel.h"

#define msleep(a)	udelay(a * 1000)

#ifdef CONFIG_RK_PWM_BL
extern int rk_pwm_bl_config(int brightness);
#endif

struct panel_simple {
	const void *blob;
	int node;

	const struct drm_display_mode *mode;
	int bus_format;

	struct fdt_gpio_state enable_gpio;

	int delay_prepare;
	int delay_unprepare;
	int delay_enable;
	int delay_disable;
};

static int panel_simple_prepare(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	struct panel_simple *panel = panel_state->private;

	fdtdec_set_gpio(&panel->enable_gpio, 1);
	mdelay(panel->delay_prepare);

	return 0;
}

static int panel_simple_unprepare(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	struct panel_simple *panel = panel_state->private;

	fdtdec_set_gpio(&panel->enable_gpio, 0);
	mdelay(panel->delay_unprepare);

	return 0;
}

static int panel_simple_enable(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	struct panel_simple *panel = panel_state->private;

#ifdef CONFIG_RK_PWM_BL
	rk_pwm_bl_config(-1);
#endif
	mdelay(panel->delay_enable);

	return 0;
}

static int panel_simple_disable(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	struct panel_simple *panel = panel_state->private;

#ifdef CONFIG_RK_PWM_BL
	rk_pwm_bl_config(0);
#endif
	mdelay(panel->delay_disable);

	return 0;
}

static int panel_simple_parse_dt(const void *blob, int node,
				 struct panel_simple *panel)
{
	struct fdt_gpio_state *enable_gpio = &panel->enable_gpio;

	fdtdec_decode_gpio(blob, node, "enable-gpios", enable_gpio);

	panel->delay_prepare = fdtdec_get_int(blob, node, "delay,prepare", 0);
	panel->delay_unprepare = fdtdec_get_int(blob, node, "delay,unprepare", 0);
	panel->delay_enable = fdtdec_get_int(blob, node, "delay,enable", 0);
	panel->delay_disable = fdtdec_get_int(blob, node, "delay,disable", 0);
	panel->bus_format = fdtdec_get_int(blob, node, "bus-format", MEDIA_BUS_FMT_RBG888_1X24);

	printf("delay prepare[%d] unprepare[%d] enable[%d] disable[%d]\n",
	       panel->delay_prepare, panel->delay_unprepare,
	       panel->delay_enable, panel->delay_disable);

	/* keep panel blank on init. */
	gpio_direction_output(enable_gpio->gpio, !!(enable_gpio->flags & OF_GPIO_ACTIVE_LOW));

#ifdef CONFIG_RK_PWM_BL
	rk_pwm_bl_config(0);
#endif
	return 0;
}

static int panel_simple_init(struct display_state *state)
{
	const void *blob = state->blob;
	struct connector_state *conn_state = &state->conn_state;
	struct panel_state *panel_state = &state->panel_state;
	int node = panel_state->node;
	const struct drm_display_mode *mode = panel_state->panel->data;
	struct panel_simple *panel;
	int ret;

	panel = malloc(sizeof(*panel));
	if (!panel)
		return -ENOMEM;

	panel->blob = blob;
	panel->node = node;
	panel->mode = mode;
	panel_state->private = panel;

	ret = panel_simple_parse_dt(blob, node, panel);
	if (ret) {
		printf("%s: failed to parse DT\n", __func__);
		free(panel);
		return ret;
	}

	conn_state->bus_format = panel->bus_format;

	return 0;
}

static void panel_simple_deinit(struct display_state *state)
{
	struct panel_state *panel_state = &state->panel_state;
	struct panel_simple *panel = panel_state->private;

	free(panel);
}

const struct rockchip_panel_funcs panel_simple_funcs = {
	.init		= panel_simple_init,
	.deinit		= panel_simple_deinit,
	.prepare	= panel_simple_prepare,
	.unprepare	= panel_simple_unprepare,
	.enable		= panel_simple_enable,
	.disable	= panel_simple_disable,
};
