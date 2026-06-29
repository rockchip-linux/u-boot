// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 Rockchip Electronics Co., Ltd
 * Author: Damon Ding <damon.ding@rock-chips.com>
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/gpio.h>
#include <dm/device.h>
#include <dm/read.h>
#include <dm/uclass.h>
#include <dm/uclass-id.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <power/regulator.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_panel.h"

/**
 * struct panel_delay - Describes delays for a simple panel.
 */
struct panel_delay {
	/**
	 * @hpd_reliable: Time for HPD to be reliable
	 *
	 * The time (in milliseconds) that it takes after powering the panel
	 * before the HPD signal is reliable. Ideally this is 0 but some panels,
	 * board designs, or bad pulldown configs can cause a glitch here.
	 *
	 * NOTE: on some old panel data this number appears to be much too big.
	 * Presumably some old panels simply didn't have HPD hooked up and put
	 * the hpd_absent here because this field predates the
	 * hpd_absent. While that works, it's non-ideal.
	 */
	unsigned int hpd_reliable;

	/**
	 * @hpd_absent: Time to wait if HPD isn't hooked up.
	 *
	 * Add this to the prepare delay if we know Hot Plug Detect isn't used.
	 *
	 * This is T3-max on eDP timing diagrams or the delay from power on
	 * until HPD is guaranteed to be asserted.
	 */
	unsigned int hpd_absent;

	/**
	 * @powered_on_to_enable: Time between panel powered on and enable.
	 *
	 * The minimum time, in milliseconds, that needs to have passed
	 * between when panel powered on and enable may begin.
	 *
	 * This is (T3+T4+T5+T6+T8)-min on eDP timing diagrams or after the
	 * power supply enabled until we can turn the backlight on and see
	 * valid data.
	 *
	 * This doesn't normally need to be set if timings are already met by
	 * prepare_to_enable or enable.
	 */
	unsigned int powered_on_to_enable;

	/**
	 * @prepare_to_enable: Time between prepare and enable.
	 *
	 * The minimum time, in milliseconds, that needs to have passed
	 * between when prepare finished and enable may begin. If at
	 * enable time less time has passed since prepare finished,
	 * the driver waits for the remaining time.
	 *
	 * If a fixed enable delay is also specified, we'll start
	 * counting before delaying for the fixed delay.
	 *
	 * If a fixed prepare delay is also specified, we won't start
	 * counting until after the fixed delay. We can't overlap this
	 * fixed delay with the min time because the fixed delay
	 * doesn't happen at the end of the function if a HPD GPIO was
	 * specified.
	 *
	 * In other words:
	 *   prepare()
	 *     ...
	 *     // do fixed prepare delay
	 *     // wait for HPD GPIO if applicable
	 *     // start counting for prepare_to_enable
	 *
	 *   enable()
	 *     // do fixed enable delay
	 *     // enforce prepare_to_enable min time
	 *
	 * This is not specified in a standard way on eDP timing diagrams.
	 * It is effectively the time from HPD going high till you can
	 * turn on the backlight.
	 */
	unsigned int prepare_to_enable;

	/**
	 * @enable: Time for the panel to display a valid frame.
	 *
	 * The time (in milliseconds) that it takes for the panel to
	 * display the first valid frame after starting to receive
	 * video data.
	 *
	 * This is (T6-min + max(T7-max, T8-min)) on eDP timing diagrams or
	 * the delay after link training finishes until we can turn the
	 * backlight on and see valid data.
	 */
	unsigned int enable;

	/**
	 * @disable: Time for the panel to turn the display off.
	 *
	 * The time (in milliseconds) that it takes for the panel to
	 * turn the display off (no content is visible).
	 *
	 * This is T9-min (delay from backlight off to end of valid video
	 * data) on eDP timing diagrams. It is not common to set.
	 */
	unsigned int disable;

	/**
	 * @unprepare: Time to power down completely.
	 *
	 * The time (in milliseconds) that it takes for the panel
	 * to power itself down completely.
	 *
	 * This time is used to prevent a future "prepare" from
	 * starting until at least this many milliseconds has passed.
	 * If at prepare time less time has passed since unprepare
	 * finished, the driver waits for the remaining time.
	 *
	 * This is T12-min on eDP timing diagrams.
	 */
	unsigned int unprepare;
};

/**
 * struct panel_desc - Describes a simple panel.
 */
struct panel_desc {
	/**
	 * @modes: Pointer to array of fixed modes appropriate for this panel.
	 *
	 * If only one mode then this can just be the address of the mode.
	 * NOTE: cannot be used with "timings" and also if this is specified
	 * then you cannot override the mode in the device tree.
	 */
	const struct drm_display_mode *modes;

	/** @num_modes: Number of elements in modes array. */
	unsigned int num_modes;

	/**
	 * @timings: Pointer to array of display timings
	 *
	 * NOTE: cannot be used with "modes" and also these will be used to
	 * validate a device tree override if one is present.
	 */
	const struct display_timing *timings;

	/** @num_timings: Number of elements in timings array. */
	unsigned int num_timings;

	/** @bpc: Bits per color. */
	unsigned int bpc;

	/** @size: Structure containing the physical size of this panel. */
	struct {
		/**
		 * @size.width: Width (in mm) of the active display area.
		 */
		unsigned int width;

		/**
		 * @size.height: Height (in mm) of the active display area.
		 */
		unsigned int height;
	} size;

	/** @delay: Structure containing various delay values for this panel. */
	struct panel_delay delay;
};

/**
 * struct edp_panel_entry - Maps panel ID to delay / panel name.
 */
struct edp_panel_entry {
	/** @ident: edid identity used for panel matching. */
	const struct drm_edid_ident ident;

	/** @delay: The power sequencing delays needed for this panel. */
	const struct panel_delay *delay;

	/** @override_edid_mode: Override the mode obtained by edid. */
	const struct drm_display_mode *override_edid_mode;
};

struct panel_edp {
	bool no_hpd;

	struct panel_desc *desc;

	struct udevice *supply;
	struct drm_dp_aux *aux;

	struct gpio_desc enable_gpio;
	struct gpio_desc hpd_gpio;

	const struct edp_panel_entry *detected_panel;

	const struct drm_edid *drm_edid;

	struct drm_display_mode override_mode;
};

static void panel_edp_disable(struct rockchip_panel *base)
{
	struct panel_edp *p = dev_get_priv(base->dev);

	if (p->desc->delay.disable)
		mdelay(p->desc->delay.disable);
}

static int panel_edp_suspend(struct udevice *dev)
{
	struct panel_edp *p = dev_get_priv(dev);

	if (dm_gpio_is_valid(&p->enable_gpio))
		dm_gpio_set_value(&p->enable_gpio, 0);
	if (p->supply)
		regulator_set_enable(p->supply, 0);

	return 0;
}

static void panel_edp_unprepare(struct rockchip_panel *base)
{
	panel_edp_suspend(base->dev);
}

static int panel_edp_get_hpd_gpio(struct udevice *dev, struct panel_edp *p)
{
	int ret;

	ret = gpio_request_by_name(dev, "hpd-gpios", 0,
				   &p->hpd_gpio, GPIOD_IS_IN);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "failed to get 'hpd' GPIO\n");
		return ret;
	}

	return 0;
}

static bool panel_edp_can_read_hpd(struct panel_edp *p)
{
	return !p->no_hpd && (dm_gpio_is_valid(&p->hpd_gpio) || (p->aux && p->aux->wait_hpd_asserted));
}

static int panel_edp_prepare_once(struct udevice *dev)
{
	struct panel_edp *p = dev_get_priv(dev);
	unsigned int delay;
	int ret;
	int hpd_asserted;
	unsigned long hpd_wait_us;

	mdelay(p->desc->delay.unprepare);

	if (p->supply)
		regulator_set_enable(p->supply, 1);

	if (dm_gpio_is_valid(&p->enable_gpio))
		dm_gpio_set_value(&p->enable_gpio, 1);

	delay = p->desc->delay.hpd_reliable;
	if (p->no_hpd)
		delay = max(delay, p->desc->delay.hpd_absent);
	if (delay)
		mdelay(delay);

	if (panel_edp_can_read_hpd(p)) {
		if (p->desc->delay.hpd_absent)
			hpd_wait_us = p->desc->delay.hpd_absent * 1000UL;
		else
			hpd_wait_us = 2000000;

		if (dm_gpio_is_valid(&p->hpd_gpio)) {
			ret = readx_poll_timeout(dm_gpio_get_value,
						 &p->hpd_gpio, hpd_asserted,
						 hpd_asserted, hpd_wait_us + 1000);
			if (hpd_asserted < 0)
				ret = hpd_asserted;
		} else {
			ret = p->aux->wait_hpd_asserted(p->aux, hpd_wait_us);
		}

		if (ret) {
			if (ret != -ETIMEDOUT)
				dev_err(dev,
					"error waiting for hpd GPIO: %d\n", ret);
			goto error;
		}
	}

	return 0;

error:
	if (dm_gpio_is_valid(&p->enable_gpio))
		dm_gpio_set_value(&p->enable_gpio, 0);
	if (p->supply)
		regulator_set_enable(p->supply, 0);

	return ret;
}

/*
 * Some panels simply don't always come up and need to be power cycled to
 * work properly.  We'll allow for a handful of retries.
 */
#define MAX_PANEL_PREPARE_TRIES		5

static int panel_edp_resume(struct udevice *dev)
{
	int ret;
	int try;

	for (try = 0; try < MAX_PANEL_PREPARE_TRIES; try++) {
		ret = panel_edp_prepare_once(dev);
		if (ret != -ETIMEDOUT)
			break;
	}

	if (ret == -ETIMEDOUT)
		dev_err(dev, "Prepare timeout after %d tries\n", try);
	else if (try)
		dev_warn(dev, "Prepare needed %d retries\n", try);

	return ret;
}

static const struct edp_panel_entry *find_edp_panel(u32 panel_id, const struct drm_edid *edid);

static void panel_edp_set_conservative_timings(struct panel_edp *panel, struct panel_desc *desc)
{
	/*
	 * It's highly likely that the panel will work if we use very
	 * conservative timings, so let's do that.
	 *
	 * Nearly all panels have a "unprepare" delay of 500 ms though
	 * there are a few with 1000. Let's stick 2000 in just to be
	 * super conservative.
	 *
	 * An "enable" delay of 80 ms seems the most common, but we'll
	 * throw in 200 ms to be safe.
	 */
	desc->delay.unprepare = 2000;
	desc->delay.enable = 200;

	panel->detected_panel = ERR_PTR(-EINVAL);
}

static int generic_edp_panel_probe(struct udevice *dev, struct rockchip_panel *base)
{
	struct panel_edp *panel = dev_get_priv(dev);
	struct panel_desc *desc = panel->desc;
	const struct drm_edid *base_block = NULL;
	u32 panel_id;
	char vend[4];
	u16 product_id;
	u32 reliable_ms = 0;
	u32 absent_ms = 0;
	u8 *edid;

	memset(panel->desc, 0, sizeof(struct panel_desc));

	/*
	 * Read the dts properties for the initial probe. These are used by
	 * the runtime resume code which will get called by the
	 * pm_runtime_get_sync() call below.
	 */
	dev_read_u32(dev, "hpd-reliable-delay-ms", &reliable_ms);
	desc->delay.hpd_reliable = reliable_ms;
	dev_read_u32(dev, "hpd-absent-delay-ms", &absent_ms);
	desc->delay.hpd_absent = absent_ms;

	edid = drm_do_get_edid(&panel->aux->ddc);
	if (edid)
		base_block = drm_edid_alloc(edid, EDID_LENGTH);
	if (base_block) {
		panel_id = drm_edid_get_panel_id(base_block);
	} else {
		dev_err(dev, "Couldn't read EDID for ID; using conservative timings\n");
		panel_edp_set_conservative_timings(panel, desc);
		return 0;
	}
	drm_edid_decode_panel_id(panel_id, vend, &product_id);

	panel->detected_panel = find_edp_panel(panel_id, base_block);
	drm_edid_free(base_block);

	/*
	 * We're using non-optimized timings and want it really obvious that
	 * someone needs to add an entry to the table, so we'll do a WARN_ON
	 * splat.
	 */
	if (WARN_ON(!panel->detected_panel)) {
		dev_warn(dev,
			 "Unknown panel %s %#06x, using conservative timings\n",
			 vend, product_id);
		panel_edp_set_conservative_timings(panel, desc);
	} else {
		dev_info(dev, "Detected %s %s (%#06x)\n",
			 vend, panel->detected_panel->ident.name, product_id);

		/* Update the delay; everything else comes from EDID */
		desc->delay = *panel->detected_panel->delay;
	}

	return 0;
}

static void panel_edp_prepare(struct rockchip_panel *base)
{
	struct udevice *dev = base->dev;
	struct panel_edp *panel = dev_get_priv(dev);

	panel_edp_resume(base->dev);

	panel->aux = base->conn->aux;
}

static void panel_edp_enable(struct rockchip_panel *base)
{
	struct panel_edp *p = dev_get_priv(base->dev);
	unsigned int delay;

	delay = p->desc->delay.enable;

	/*
	 * If there is a "prepare_to_enable" delay then that's supposed to be
	 * the delay from HPD going high until we can turn the backlight on.
	 * However, we can only count this if HPD is readable by the panel
	 * driver.
	 *
	 * If we aren't handling the HPD pin ourselves then the best we
	 * can do is assume that HPD went high immediately before we were
	 * called (and link training took zero time). Note that "no-hpd"
	 * actually counts as handling HPD ourselves since we're doing the
	 * worst case delay (in prepare) ourselves.
	 *
	 * NOTE: if we ever end up in this "if" statement then we're
	 * guaranteed that the panel_edp_wait() call below will do no delay.
	 * It already handles that case, though, so we don't need any special
	 * code for it.
	 */
	if (p->desc->delay.prepare_to_enable &&
	    !panel_edp_can_read_hpd(p) && !p->no_hpd)
		delay = max(delay, p->desc->delay.prepare_to_enable);

	if (delay)
		mdelay(delay);

	mdelay(p->desc->delay.powered_on_to_enable);
}

static int panel_edp_get_mode(struct rockchip_panel *base, struct drm_display_mode *mode)
{
	struct udevice *dev = base->dev;
	struct panel_edp *panel = dev_get_priv(dev);
	int ret;

	/* Power up panel for EDID read */
	rockchip_panel_prepare(base);

	if (of_device_is_compatible(ofnode_to_np(dev_ofnode(dev)), "edp-panel", NULL, NULL)) {
		ret = generic_edp_panel_probe(dev, base);
		if (ret) {
			dev_err(dev, "Couldn't detect panel nor find a fallback\n");
			goto unprepare;
		}
	} else if (panel->desc->bpc != 6 && panel->desc->bpc != 8 && panel->desc->bpc != 10) {
		dev_warn(dev, "Expected bpc in {6,8,10} but got: %u\n", panel->desc->bpc);
	}

	if (!base->backlight && panel->aux) {
		ret = drm_panel_dp_aux_backlight(base, panel->aux);

		/*
		 * Warn if we get an error, but don't consider it fatal. Having
		 * a panel where we can't control the backlight is better than
		 * no panel.
		 */
		if (ret)
			dev_warn(dev, "failed to register dp aux backlight: %d\n", ret);
	}

	/*
	 * The .get_mode() only fetches &panel.desc config without reading &drm_display_mode
	 * from EDID, which is retrieved via connector .get_timing() as before. Return invalid
	 * value accordingly.
	 */
	return -EINVAL;

unprepare:
	rockchip_panel_unprepare(base);
	return ret;
}

static const struct rockchip_panel_funcs rockchip_edp_panel_funcs = {
	.prepare = panel_edp_prepare,
	.unprepare = panel_edp_unprepare,
	.enable = panel_edp_enable,
	.disable = panel_edp_disable,
	.get_mode = panel_edp_get_mode,
};

static int rockchip_edp_panel_probe(struct udevice *dev)
{
	struct panel_edp *panel = dev_get_priv(dev);
	struct panel_desc *data = (struct panel_desc *)dev_get_driver_data(dev);
	struct panel_desc *desc = NULL;
	struct rockchip_panel *base;
	int ret;

	desc = calloc(1, sizeof(*desc));
	if (!desc)
		return -ENOMEM;
	if (data)
		memcpy(desc, data, sizeof(struct panel_desc));

	panel->desc = desc;

	panel->no_hpd = dev_read_bool(dev, "no-hpd");
	if (!panel->no_hpd) {
		ret = panel_edp_get_hpd_gpio(dev, panel);
		if (ret)
			goto free_desc;
	}

	device_get_supply_regulator(dev, "power-supply", &panel->supply);

	ret = gpio_request_by_name(dev, "enable-gpios", 0,
				   &panel->enable_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "failed to request GPIO\n");
		goto free_desc;
	}

	base = calloc(1, sizeof(*base));
	if (!base) {
		ret = -ENOMEM;
		goto free_desc;
	}

	dev->driver_data = (ulong)base;
	base->dev = dev;
	base->funcs = &rockchip_edp_panel_funcs;

	return 0;

free_desc:
	free(desc);
	return ret;
}

static const struct display_timing auo_b101ean01_timing = {
	.pixelclock = { 65300000, 72500000, 75000000 },
	.hactive = { 1280, 1280, 1280 },
	.hfront_porch = { 18, 119, 119 },
	.hback_porch = { 21, 21, 21 },
	.hsync_len = { 32, 32, 32 },
	.vactive = { 800, 800, 800 },
	.vfront_porch = { 4, 4, 4 },
	.vback_porch = { 8, 8, 8 },
	.vsync_len = { 18, 20, 20 },
};

static const struct panel_desc auo_b101ean01 = {
	.timings = &auo_b101ean01_timing,
	.num_timings = 1,
	.bpc = 6,
	.size = {
		.width = 217,
		.height = 136,
	},
};

static const struct drm_display_mode auo_b116xa3_mode = {
	.clock = 70589,
	.hdisplay = 1366,
	.hsync_start = 1366 + 40,
	.hsync_end = 1366 + 40 + 40,
	.htotal = 1366 + 40 + 40 + 32,
	.vdisplay = 768,
	.vsync_start = 768 + 10,
	.vsync_end = 768 + 10 + 12,
	.vtotal = 768 + 10 + 12 + 6,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct drm_display_mode auo_b116xak01_mode = {
	.clock = 69300,
	.hdisplay = 1366,
	.hsync_start = 1366 + 48,
	.hsync_end = 1366 + 48 + 32,
	.htotal = 1366 + 48 + 32 + 10,
	.vdisplay = 768,
	.vsync_start = 768 + 4,
	.vsync_end = 768 + 4 + 6,
	.vtotal = 768 + 4 + 6 + 15,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct panel_desc auo_b116xak01 = {
	.modes = &auo_b116xak01_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 256,
		.height = 144,
	},
	.delay = {
		.hpd_absent = 200,
		.unprepare = 500,
		.enable = 50,
	},
};

static const struct drm_display_mode auo_b133htn01_mode = {
	.clock = 150660,
	.hdisplay = 1920,
	.hsync_start = 1920 + 172,
	.hsync_end = 1920 + 172 + 80,
	.htotal = 1920 + 172 + 80 + 60,
	.vdisplay = 1080,
	.vsync_start = 1080 + 25,
	.vsync_end = 1080 + 25 + 10,
	.vtotal = 1080 + 25 + 10 + 10,
};

static const struct panel_desc auo_b133htn01 = {
	.modes = &auo_b133htn01_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 293,
		.height = 165,
	},
	.delay = {
		.hpd_reliable = 105,
		.enable = 20,
		.unprepare = 50,
	},
};

static const struct drm_display_mode auo_b133xtn01_mode = {
	.clock = 69500,
	.hdisplay = 1366,
	.hsync_start = 1366 + 48,
	.hsync_end = 1366 + 48 + 32,
	.htotal = 1366 + 48 + 32 + 20,
	.vdisplay = 768,
	.vsync_start = 768 + 3,
	.vsync_end = 768 + 3 + 6,
	.vtotal = 768 + 3 + 6 + 13,
};

static const struct panel_desc auo_b133xtn01 = {
	.modes = &auo_b133xtn01_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 293,
		.height = 165,
	},
};

static const struct drm_display_mode boe_nv101wxmn51_modes[] = {
	{
		.clock = 71900,
		.hdisplay = 1280,
		.hsync_start = 1280 + 48,
		.hsync_end = 1280 + 48 + 32,
		.htotal = 1280 + 48 + 32 + 80,
		.vdisplay = 800,
		.vsync_start = 800 + 3,
		.vsync_end = 800 + 3 + 5,
		.vtotal = 800 + 3 + 5 + 24,
	},
	{
		.clock = 57500,
		.hdisplay = 1280,
		.hsync_start = 1280 + 48,
		.hsync_end = 1280 + 48 + 32,
		.htotal = 1280 + 48 + 32 + 80,
		.vdisplay = 800,
		.vsync_start = 800 + 3,
		.vsync_end = 800 + 3 + 5,
		.vtotal = 800 + 3 + 5 + 24,
	},
};

static const struct panel_desc boe_nv101wxmn51 = {
	.modes = boe_nv101wxmn51_modes,
	.num_modes = ARRAY_SIZE(boe_nv101wxmn51_modes),
	.bpc = 8,
	.size = {
		.width = 217,
		.height = 136,
	},
	.delay = {
		/* TODO: should be hpd-absent and no-hpd should be set? */
		.hpd_reliable = 210,
		.enable = 50,
		.unprepare = 160,
	},
};

static const struct drm_display_mode boe_nv110wtm_n61_modes[] = {
	{
		.clock = 207800,
		.hdisplay = 2160,
		.hsync_start = 2160 + 48,
		.hsync_end = 2160 + 48 + 32,
		.htotal = 2160 + 48 + 32 + 100,
		.vdisplay = 1440,
		.vsync_start = 1440 + 3,
		.vsync_end = 1440 + 3 + 6,
		.vtotal = 1440 + 3 + 6 + 31,
		.flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC,
	},
	{
		.clock = 138500,
		.hdisplay = 2160,
		.hsync_start = 2160 + 48,
		.hsync_end = 2160 + 48 + 32,
		.htotal = 2160 + 48 + 32 + 100,
		.vdisplay = 1440,
		.vsync_start = 1440 + 3,
		.vsync_end = 1440 + 3 + 6,
		.vtotal = 1440 + 3 + 6 + 31,
		.flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC,
	},
};

static const struct panel_desc boe_nv110wtm_n61 = {
	.modes = boe_nv110wtm_n61_modes,
	.num_modes = ARRAY_SIZE(boe_nv110wtm_n61_modes),
	.bpc = 8,
	.size = {
		.width = 233,
		.height = 155,
	},
	.delay = {
		.hpd_absent = 200,
		.prepare_to_enable = 80,
		.enable = 50,
		.unprepare = 500,
	},
};

/* Also used for boe_nv133fhm_n62 */
static const struct drm_display_mode boe_nv133fhm_n61_modes = {
	.clock = 147840,
	.hdisplay = 1920,
	.hsync_start = 1920 + 48,
	.hsync_end = 1920 + 48 + 32,
	.htotal = 1920 + 48 + 32 + 200,
	.vdisplay = 1080,
	.vsync_start = 1080 + 3,
	.vsync_end = 1080 + 3 + 6,
	.vtotal = 1080 + 3 + 6 + 31,
	.flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC,
};

/* Also used for boe_nv133fhm_n62 */
static const struct panel_desc boe_nv133fhm_n61 = {
	.modes = &boe_nv133fhm_n61_modes,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 294,
		.height = 165,
	},
	.delay = {
		/*
		 * When power is first given to the panel there's a short
		 * spike on the HPD line.  It was explained that this spike
		 * was until the TCON data download was complete.  On
		 * one system this was measured at 8 ms.  We'll put 15 ms
		 * in the prepare delay just to be safe.  That means:
		 * - If HPD isn't hooked up you still have 200 ms delay.
		 * - If HPD is hooked up we won't try to look at it for the
		 *   first 15 ms.
		 */
		.hpd_reliable = 15,
		.hpd_absent = 200,

		.unprepare = 500,
	},
};

static const struct drm_display_mode boe_nv140fhmn49_modes[] = {
	{
		.clock = 148500,
		.hdisplay = 1920,
		.hsync_start = 1920 + 48,
		.hsync_end = 1920 + 48 + 32,
		.htotal = 2200,
		.vdisplay = 1080,
		.vsync_start = 1080 + 3,
		.vsync_end = 1080 + 3 + 5,
		.vtotal = 1125,
	},
};

static const struct panel_desc boe_nv140fhmn49 = {
	.modes = boe_nv140fhmn49_modes,
	.num_modes = ARRAY_SIZE(boe_nv140fhmn49_modes),
	.bpc = 6,
	.size = {
		.width = 309,
		.height = 174,
	},
	.delay = {
		/* TODO: should be hpd-absent and no-hpd should be set? */
		.hpd_reliable = 210,
		.enable = 50,
		.unprepare = 160,
	},
};

static const struct drm_display_mode innolux_n116bca_ea1_mode = {
	.clock = 76420,
	.hdisplay = 1366,
	.hsync_start = 1366 + 136,
	.hsync_end = 1366 + 136 + 30,
	.htotal = 1366 + 136 + 30 + 60,
	.vdisplay = 768,
	.vsync_start = 768 + 8,
	.vsync_end = 768 + 8 + 12,
	.vtotal = 768 + 8 + 12 + 12,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static const struct panel_desc innolux_n116bca_ea1 = {
	.modes = &innolux_n116bca_ea1_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 256,
		.height = 144,
	},
	.delay = {
		.hpd_absent = 200,
		.enable = 80,
		.disable = 50,
		.unprepare = 500,
	},
};

/*
 * Datasheet specifies that at 60 Hz refresh rate:
 * - total horizontal time: { 1506, 1592, 1716 }
 * - total vertical time: { 788, 800, 868 }
 *
 * ...but doesn't go into exactly how that should be split into a front
 * porch, back porch, or sync length.  For now we'll leave a single setting
 * here which allows a bit of tweaking of the pixel clock at the expense of
 * refresh rate.
 */
static const struct display_timing innolux_n116bge_timing = {
	.pixelclock = { 72600000, 76420000, 80240000 },
	.hactive = { 1366, 1366, 1366 },
	.hfront_porch = { 136, 136, 136 },
	.hback_porch = { 60, 60, 60 },
	.hsync_len = { 30, 30, 30 },
	.vactive = { 768, 768, 768 },
	.vfront_porch = { 8, 8, 8 },
	.vback_porch = { 12, 12, 12 },
	.vsync_len = { 12, 12, 12 },
	.flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW,
};

static const struct panel_desc innolux_n116bge = {
	.timings = &innolux_n116bge_timing,
	.num_timings = 1,
	.bpc = 6,
	.size = {
		.width = 256,
		.height = 144,
	},
};

static const struct drm_display_mode innolux_n125hce_gn1_mode = {
	.clock = 162000,
	.hdisplay = 1920,
	.hsync_start = 1920 + 40,
	.hsync_end = 1920 + 40 + 40,
	.htotal = 1920 + 40 + 40 + 80,
	.vdisplay = 1080,
	.vsync_start = 1080 + 4,
	.vsync_end = 1080 + 4 + 4,
	.vtotal = 1080 + 4 + 4 + 24,
};

static const struct panel_desc innolux_n125hce_gn1 = {
	.modes = &innolux_n125hce_gn1_mode,
	.num_modes = 1,
	.bpc = 8,
	.size = {
		.width = 276,
		.height = 155,
	},
};

static const struct drm_display_mode innolux_p120zdg_bf1_mode = {
	.clock = 206016,
	.hdisplay = 2160,
	.hsync_start = 2160 + 48,
	.hsync_end = 2160 + 48 + 32,
	.htotal = 2160 + 48 + 32 + 80,
	.vdisplay = 1440,
	.vsync_start = 1440 + 3,
	.vsync_end = 1440 + 3 + 10,
	.vtotal = 1440 + 3 + 10 + 27,
	.flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC,
};

static const struct panel_desc innolux_p120zdg_bf1 = {
	.modes = &innolux_p120zdg_bf1_mode,
	.num_modes = 1,
	.bpc = 8,
	.size = {
		.width = 254,
		.height = 169,
	},
	.delay = {
		.hpd_absent = 200,
		.unprepare = 500,
	},
};

static const struct drm_display_mode kingdisplay_kd116n21_30nv_a010_mode = {
	.clock = 81000,
	.hdisplay = 1366,
	.hsync_start = 1366 + 40,
	.hsync_end = 1366 + 40 + 32,
	.htotal = 1366 + 40 + 32 + 62,
	.vdisplay = 768,
	.vsync_start = 768 + 5,
	.vsync_end = 768 + 5 + 5,
	.vtotal = 768 + 5 + 5 + 122,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct panel_desc kingdisplay_kd116n21_30nv_a010 = {
	.modes = &kingdisplay_kd116n21_30nv_a010_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 256,
		.height = 144,
	},
	.delay = {
		.hpd_absent = 200,
	},
};

static const struct drm_display_mode lg_lp079qx1_sp0v_mode = {
	.clock = 200000,
	.hdisplay = 1536,
	.hsync_start = 1536 + 12,
	.hsync_end = 1536 + 12 + 16,
	.htotal = 1536 + 12 + 16 + 48,
	.vdisplay = 2048,
	.vsync_start = 2048 + 8,
	.vsync_end = 2048 + 8 + 4,
	.vtotal = 2048 + 8 + 4 + 8,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

static const struct panel_desc lg_lp079qx1_sp0v = {
	.modes = &lg_lp079qx1_sp0v_mode,
	.num_modes = 1,
	.size = {
		.width = 129,
		.height = 171,
	},
};

static const struct drm_display_mode lg_lp097qx1_spa1_mode = {
	.clock = 205210,
	.hdisplay = 2048,
	.hsync_start = 2048 + 150,
	.hsync_end = 2048 + 150 + 5,
	.htotal = 2048 + 150 + 5 + 5,
	.vdisplay = 1536,
	.vsync_start = 1536 + 3,
	.vsync_end = 1536 + 3 + 1,
	.vtotal = 1536 + 3 + 1 + 9,
};

static const struct panel_desc lg_lp097qx1_spa1 = {
	.modes = &lg_lp097qx1_spa1_mode,
	.num_modes = 1,
	.size = {
		.width = 208,
		.height = 147,
	},
};

static const struct drm_display_mode lg_lp120up1_mode = {
	.clock = 162300,
	.hdisplay = 1920,
	.hsync_start = 1920 + 40,
	.hsync_end = 1920 + 40 + 40,
	.htotal = 1920 + 40 + 40 + 80,
	.vdisplay = 1280,
	.vsync_start = 1280 + 4,
	.vsync_end = 1280 + 4 + 4,
	.vtotal = 1280 + 4 + 4 + 12,
};

static const struct panel_desc lg_lp120up1 = {
	.modes = &lg_lp120up1_mode,
	.num_modes = 1,
	.bpc = 8,
	.size = {
		.width = 267,
		.height = 183,
	},
};

static const struct drm_display_mode lg_lp129qe_mode = {
	.clock = 285250,
	.hdisplay = 2560,
	.hsync_start = 2560 + 48,
	.hsync_end = 2560 + 48 + 32,
	.htotal = 2560 + 48 + 32 + 80,
	.vdisplay = 1700,
	.vsync_start = 1700 + 3,
	.vsync_end = 1700 + 3 + 10,
	.vtotal = 1700 + 3 + 10 + 36,
};

static const struct panel_desc lg_lp129qe = {
	.modes = &lg_lp129qe_mode,
	.num_modes = 1,
	.bpc = 8,
	.size = {
		.width = 272,
		.height = 181,
	},
};

static const struct drm_display_mode neweast_wjfh116008a_modes[] = {
	{
		.clock = 138500,
		.hdisplay = 1920,
		.hsync_start = 1920 + 48,
		.hsync_end = 1920 + 48 + 32,
		.htotal = 1920 + 48 + 32 + 80,
		.vdisplay = 1080,
		.vsync_start = 1080 + 3,
		.vsync_end = 1080 + 3 + 5,
		.vtotal = 1080 + 3 + 5 + 23,
		.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
	}, {
		.clock = 110920,
		.hdisplay = 1920,
		.hsync_start = 1920 + 48,
		.hsync_end = 1920 + 48 + 32,
		.htotal = 1920 + 48 + 32 + 80,
		.vdisplay = 1080,
		.vsync_start = 1080 + 3,
		.vsync_end = 1080 + 3 + 5,
		.vtotal = 1080 + 3 + 5 + 23,
		.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
	}
};

static const struct panel_desc neweast_wjfh116008a = {
	.modes = neweast_wjfh116008a_modes,
	.num_modes = 2,
	.bpc = 6,
	.size = {
		.width = 260,
		.height = 150,
	},
	.delay = {
		.hpd_reliable = 110,
		.enable = 20,
		.unprepare = 500,
	},
};

static const struct drm_display_mode samsung_lsn122dl01_c01_mode = {
	.clock = 271560,
	.hdisplay = 2560,
	.hsync_start = 2560 + 48,
	.hsync_end = 2560 + 48 + 32,
	.htotal = 2560 + 48 + 32 + 80,
	.vdisplay = 1600,
	.vsync_start = 1600 + 2,
	.vsync_end = 1600 + 2 + 5,
	.vtotal = 1600 + 2 + 5 + 57,
};

static const struct panel_desc samsung_lsn122dl01_c01 = {
	.modes = &samsung_lsn122dl01_c01_mode,
	.num_modes = 1,
	.size = {
		.width = 263,
		.height = 164,
	},
};

static const struct drm_display_mode samsung_ltn140at29_301_mode = {
	.clock = 76300,
	.hdisplay = 1366,
	.hsync_start = 1366 + 64,
	.hsync_end = 1366 + 64 + 48,
	.htotal = 1366 + 64 + 48 + 128,
	.vdisplay = 768,
	.vsync_start = 768 + 2,
	.vsync_end = 768 + 2 + 5,
	.vtotal = 768 + 2 + 5 + 17,
};

static const struct panel_desc samsung_ltn140at29_301 = {
	.modes = &samsung_ltn140at29_301_mode,
	.num_modes = 1,
	.bpc = 6,
	.size = {
		.width = 320,
		.height = 187,
	},
};

static const struct drm_display_mode sharp_ld_d5116z01b_mode = {
	.clock = 168480,
	.hdisplay = 1920,
	.hsync_start = 1920 + 48,
	.hsync_end = 1920 + 48 + 32,
	.htotal = 1920 + 48 + 32 + 80,
	.vdisplay = 1280,
	.vsync_start = 1280 + 3,
	.vsync_end = 1280 + 3 + 10,
	.vtotal = 1280 + 3 + 10 + 57,
	.flags = DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC,
};

static const struct panel_desc sharp_ld_d5116z01b = {
	.modes = &sharp_ld_d5116z01b_mode,
	.num_modes = 1,
	.bpc = 8,
	.size = {
		.width = 260,
		.height = 120,
	},
};

static const struct display_timing sharp_lq123p1jx31_timing = {
	.pixelclock = { 252750000, 252750000, 266604720 },
	.hactive = { 2400, 2400, 2400 },
	.hfront_porch = { 48, 48, 48 },
	.hback_porch = { 80, 80, 84 },
	.hsync_len = { 32, 32, 32 },
	.vactive = { 1600, 1600, 1600 },
	.vfront_porch = { 3, 3, 3 },
	.vback_porch = { 33, 33, 120 },
	.vsync_len = { 10, 10, 10 },
	.flags = DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_HSYNC_LOW,
};

static const struct panel_desc sharp_lq123p1jx31 = {
	.timings = &sharp_lq123p1jx31_timing,
	.num_timings = 1,
	.bpc = 8,
	.size = {
		.width = 259,
		.height = 173,
	},
	.delay = {
		.hpd_reliable = 110,
		.enable = 50,
		.unprepare = 550,
	},
};

static const struct panel_delay delay_200_500_p2e80 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.prepare_to_enable = 80,
};

static const struct panel_delay delay_200_500_e50_p2e80 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 50,
	.prepare_to_enable = 80,
};

static const struct panel_delay delay_200_500_p2e100 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.prepare_to_enable = 100,
};

static const struct panel_delay delay_200_500_e50 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 50,
};

static const struct panel_delay delay_200_500_e50_p2e200 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 50,
	.prepare_to_enable = 200,
};

static const struct panel_delay delay_200_500_e80 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 80,
};

static const struct panel_delay delay_200_500_e80_d50 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 80,
	.disable = 50,
};

static const struct panel_delay delay_80_500_e50 = {
	.hpd_absent = 80,
	.unprepare = 500,
	.enable = 50,
};

static const struct panel_delay delay_100_500_e200 = {
	.hpd_absent = 100,
	.unprepare = 500,
	.enable = 200,
};

static const struct panel_delay delay_200_500_e200 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 200,
};

static const struct panel_delay delay_200_500_e200_d200 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 200,
	.disable = 200,
};

static const struct panel_delay delay_200_500_e200_d10 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 200,
	.disable = 10,
};

static const struct panel_delay delay_200_150_e200 = {
	.hpd_absent = 200,
	.unprepare = 150,
	.enable = 200,
};

static const struct panel_delay delay_200_500_e50_po2e200 = {
	.hpd_absent = 200,
	.unprepare = 500,
	.enable = 50,
	.powered_on_to_enable = 200,
};

#define EDP_PANEL_ENTRY(vend_chr_0, vend_chr_1, vend_chr_2, product_id, _delay, _name) \
{ \
	.ident = { \
		.name = _name, \
		.panel_id = drm_edid_encode_panel_id(vend_chr_0, vend_chr_1, vend_chr_2, \
						     product_id), \
	}, \
	.delay = _delay \
}

#define EDP_PANEL_ENTRY2(vend_chr_0, vend_chr_1, vend_chr_2, product_id, _delay, _name, _mode) \
{ \
	.ident = { \
		.name = _name, \
		.panel_id = drm_edid_encode_panel_id(vend_chr_0, vend_chr_1, vend_chr_2, \
						     product_id), \
	}, \
	.delay = _delay, \
	.override_edid_mode = _mode \
}

/*
 * This table is used to figure out power sequencing delays for panels that
 * are detected by EDID. Entries here may point to entries in the
 * platform_of_match table (if a panel is listed in both places).
 *
 * Sort first by vendor, then by product ID.
 */
static const struct edp_panel_entry edp_panels[] = {
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x105c, &delay_200_500_e50, "B116XTN01.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x1062, &delay_200_500_e50, "B120XAN01.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x125c, &delay_200_500_e50, "Unknown"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x145c, &delay_200_500_e50, "B116XAB01.4"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x1999, &delay_200_500_e50, "Unknown"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x1e9b, &delay_200_500_e50, "B133UAN02.1"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x1ea5, &delay_200_500_e50, "B116XAK01.6"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x203d, &delay_200_500_e50, "B140HTN02.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x208d, &delay_200_500_e50, "B140HTN02.1"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x235c, &delay_200_500_e50, "B116XTN02.3"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x239b, &delay_200_500_e50, "B116XAN06.1"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x255c, &delay_200_500_e50, "B116XTN02.5"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x403d, &delay_200_500_e50, "B140HAN04.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x405c, &auo_b116xak01.delay, "B116XAN04.0"),
	EDP_PANEL_ENTRY2('A', 'U', 'O', 0x405c, &auo_b116xak01.delay, "B116XAK01.0",
			 &auo_b116xa3_mode),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x435c, &delay_200_500_e50, "Unknown"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x582d, &delay_200_500_e50, "B133UAN01.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x615c, &delay_200_500_e50, "B116XAN06.1"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x635c, &delay_200_500_e50, "B116XAN06.3"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x639c, &delay_200_500_e50, "B140HAK02.7"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x723c, &delay_200_500_e50, "B140XTN07.2"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x73aa, &delay_200_500_e50, "B116XTN02.3"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0x8594, &delay_200_500_e50, "B133UAN01.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0xa199, &delay_200_500_e50, "B116XAN06.1"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0xa7b3, &delay_200_500_e50, "B140UAN04.4"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0xc4b4, &delay_200_500_e50, "B116XAT04.1"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0xd497, &delay_200_500_e50, "B120XAN01.0"),
	EDP_PANEL_ENTRY('A', 'U', 'O', 0xf390, &delay_200_500_e50, "B140XTN07.7"),

	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0607, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0608, &delay_200_500_e50, "NT116WHM-N11"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0609, &delay_200_500_e50_po2e200, "NT116WHM-N21 V4.1"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0623, &delay_200_500_e200, "NT116WHM-N21 V4.0"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0668, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x068f, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x06e5, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0705, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0715, &delay_200_150_e200, "NT116WHM-N21"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0717, &delay_200_500_e50_po2e200, "NV133FHM-N42"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0731, &delay_200_500_e80, "NT116WHM-N42"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0741, &delay_200_500_e200, "NT116WHM-N44"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0744, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x074c, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0751, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0754, &delay_200_500_e50_po2e200, "NV116WHM-N45"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0771, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0786, &delay_200_500_p2e80, "NV116WHM-T01"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0797, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x07a8, &delay_200_500_e50_po2e200, "NT116WHM-N21"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x07d1, &boe_nv133fhm_n61.delay, "NV133FHM-N61"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x07d3, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x07f6, &delay_200_500_e200, "NT140FHM-N44"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x07f8, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0813, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0827, &delay_200_500_e50_p2e80, "NT140WHM-N44 V8.0"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x082d, &boe_nv133fhm_n61.delay, "NV133FHM-N62"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0843, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x08b2, &delay_200_500_e200, "NT140WHM-N49"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0848, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0849, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x09c3, &delay_200_500_e50, "NT116WHM-N21,836X2"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x094b, &delay_200_500_e50, "NT116WHM-N21"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0951, &delay_200_500_e80, "NV116WHM-N47"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x095f, &delay_200_500_e50, "NE135FBM-N41 v8.1"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x096e, &delay_200_500_e50_po2e200, "NV116WHM-T07 V8.0"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0979, &delay_200_500_e50, "NV116WHM-N49 V8.0"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0985, &delay_200_500_e50, "NE160QDM-NY1"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x098d, &boe_nv110wtm_n61.delay, "NV110WTM-N61"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0993, &delay_200_500_e80, "NV116WHM-T14 V8.0"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x09ad, &delay_200_500_e80, "NV116WHM-N47"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x09ae, &delay_200_500_e200, "NT140FHM-N45"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x09dd, &delay_200_500_e50, "NT116WHM-N21"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0a1b, &delay_200_500_e50, "NV133WUM-N63"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0a36, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0a3e, &delay_200_500_e80, "NV116WHM-N49"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0a5d, &delay_200_500_e50, "NV116WHM-N45"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0ac5, &delay_200_500_e50, "NV116WHM-N4C"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0ae8, &delay_200_500_e50_p2e80, "NV140WUM-N41"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0b34, &delay_200_500_e80, "NV122WUM-N41"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0b43, &delay_200_500_e200, "NV140FHM-T09"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0b56, &delay_200_500_e80, "NT140FHM-N47"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0b66, &delay_200_500_e80, "NE140WUM-N6G"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0c20, &delay_200_500_e80, "NT140FHM-N47"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0cb6, &delay_200_500_e200, "NT116WHM-N44"),
	EDP_PANEL_ENTRY('B', 'O', 'E', 0x0cfa, &delay_200_500_e50, "NV116WHM-A4D"),

	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1130, &delay_200_500_e50, "N116BGE-EB2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1132, &delay_200_500_e80_d50, "N116BGE-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1138, &innolux_n116bca_ea1.delay, "N116BCA-EA1-RC4"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1139, &delay_200_500_e80_d50, "N116BGE-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1141, &delay_200_500_e80_d50, "Unknown"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1145, &delay_200_500_e80_d50, "N116BCN-EB1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x114a, &delay_200_500_e80_d50, "Unknown"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x114c, &innolux_n116bca_ea1.delay, "N116BCA-EA1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1152, &delay_200_500_e80_d50, "N116BCN-EA1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1153, &delay_200_500_e80_d50, "N116BGE-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1154, &delay_200_500_e80_d50, "N116BCA-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1156, &delay_200_500_e80_d50, "Unknown"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1157, &delay_200_500_e80_d50, "N116BGE-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x115b, &delay_200_500_e80_d50, "N116BCN-EB1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x115d, &delay_200_500_e80_d50, "N116BCA-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x115e, &delay_200_500_e80_d50, "N116BCA-EA1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1160, &delay_200_500_e80_d50, "N116BCJ-EAK"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1161, &delay_200_500_e80, "N116BCP-EA2"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1247, &delay_200_500_e80_d50, "N120ACA-EA1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x142b, &delay_200_500_e80_d50, "N140HCA-EAC"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x142e, &delay_200_500_e80_d50, "N140BGA-EA4"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x144f, &delay_200_500_e80_d50, "N140HGA-EA1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x1468, &delay_200_500_e80, "N140HGA-EA1"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x14d4, &delay_200_500_e80_d50, "N140HCA-EAC"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x14d6, &delay_200_500_e80_d50, "N140BGA-EA4"),
	EDP_PANEL_ENTRY('C', 'M', 'N', 0x14e5, &delay_200_500_e80_d50, "N140HGA-EA1"),

	EDP_PANEL_ENTRY('C', 'S', 'O', 0x1200, &delay_200_500_e50_p2e200, "MNC207QS1-1"),

	EDP_PANEL_ENTRY('C', 'S', 'W', 0x1100, &delay_200_500_e80_d50, "MNB601LS1-1"),
	EDP_PANEL_ENTRY('C', 'S', 'W', 0x1104, &delay_200_500_e50, "MNB601LS1-4"),
	EDP_PANEL_ENTRY('C', 'S', 'W', 0x1448, &delay_200_500_e50, "MNE007QS3-7"),

	EDP_PANEL_ENTRY('H', 'K', 'C', 0x2d51, &delay_200_500_e200, "Unknown"),
	EDP_PANEL_ENTRY('H', 'K', 'C', 0x2d5b, &delay_200_500_e200, "MB116AN01"),
	EDP_PANEL_ENTRY('H', 'K', 'C', 0x2d5c, &delay_200_500_e200, "MB116AN01-2"),

	EDP_PANEL_ENTRY('I', 'V', 'O', 0x048e, &delay_200_500_e200_d10, "M116NWR6 R5"),
	EDP_PANEL_ENTRY('I', 'V', 'O', 0x057d, &delay_200_500_e200, "R140NWF5 RH"),
	EDP_PANEL_ENTRY('I', 'V', 'O', 0x854a, &delay_200_500_p2e100, "M133NW4J"),
	EDP_PANEL_ENTRY('I', 'V', 'O', 0x854b, &delay_200_500_p2e100, "R133NW4K-R0"),
	EDP_PANEL_ENTRY('I', 'V', 'O', 0x8c4d, &delay_200_150_e200, "R140NWFM R1"),

	EDP_PANEL_ENTRY('K', 'D', 'B', 0x044f, &delay_200_500_e80_d50, "Unknown"),
	EDP_PANEL_ENTRY('K', 'D', 'B', 0x0624, &kingdisplay_kd116n21_30nv_a010.delay, "116N21-30NV-A010"),
	EDP_PANEL_ENTRY('K', 'D', 'B', 0x1118, &delay_200_500_e50, "KD116N29-30NK-A005"),
	EDP_PANEL_ENTRY('K', 'D', 'B', 0x1120, &delay_200_500_e80_d50, "116N29-30NK-C007"),
	EDP_PANEL_ENTRY('K', 'D', 'B', 0x1212, &delay_200_500_e50, "KD116N0930A16"),

	EDP_PANEL_ENTRY('K', 'D', 'C', 0x044f, &delay_200_500_e50, "KD116N9-30NH-F3"),
	EDP_PANEL_ENTRY('K', 'D', 'C', 0x05f1, &delay_200_500_e80_d50, "KD116N5-30NV-G7"),
	EDP_PANEL_ENTRY('K', 'D', 'C', 0x0809, &delay_200_500_e50, "KD116N2930A15"),

	EDP_PANEL_ENTRY('L', 'G', 'D', 0x0000, &delay_200_500_e200_d200, "Unknown"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x048d, &delay_200_500_e200_d200, "Unknown"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x0497, &delay_200_500_e200_d200, "LP116WH7-SPB1"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x052c, &delay_200_500_e200_d200, "LP133WF2-SPL7"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x0537, &delay_200_500_e200_d200, "Unknown"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x054a, &delay_200_500_e200_d200, "LP116WH8-SPC1"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x0567, &delay_200_500_e200_d200, "Unknown"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x05af, &delay_200_500_e200_d200, "Unknown"),
	EDP_PANEL_ENTRY('L', 'G', 'D', 0x05f1, &delay_200_500_e200_d200, "Unknown"),

	EDP_PANEL_ENTRY('S', 'H', 'P', 0x1511, &delay_200_500_e50, "LQ140M1JW48"),
	EDP_PANEL_ENTRY('S', 'H', 'P', 0x1523, &delay_80_500_e50, "LQ140M1JW46"),
	EDP_PANEL_ENTRY('S', 'H', 'P', 0x153a, &delay_200_500_e50, "LQ140T1JH01"),
	EDP_PANEL_ENTRY('S', 'H', 'P', 0x154c, &delay_200_500_p2e100, "LQ116M1JW10"),

	EDP_PANEL_ENTRY('S', 'T', 'A', 0x0004, &delay_200_500_e200, "116KHD024006"),
	EDP_PANEL_ENTRY('S', 'T', 'A', 0x0100, &delay_100_500_e200, "2081116HHD028001-51D"),

	{ /* sentinal */ }
};

static const struct edp_panel_entry *find_edp_panel(u32 panel_id, const struct drm_edid *edid)
{
	const struct edp_panel_entry *panel;

	if (!panel_id)
		return NULL;

	/*
	 * Match with identity first. This allows handling the case where
	 * vendors incorrectly reused the same panel ID for multiple panels that
	 * need different settings. If there's no match, try again with panel
	 * ID, which should be unique.
	 */
	for (panel = edp_panels; panel->ident.panel_id; panel++)
		if (drm_edid_match(edid, &panel->ident))
			return panel;

	for (panel = edp_panels; panel->ident.panel_id; panel++)
		if (panel->ident.panel_id == panel_id)
			return panel;

	return NULL;
}

static const struct udevice_id rockchip_edp_panel_ids[] = {
	{
		/* Must be first */
		.compatible = "edp-panel",
	},
	/*
	 * Do not add panels to the list below unless they cannot be handled by
	 * the generic edp-panel compatible.
	 *
	 * The only two valid reasons are:
	 * - Because of the panel issues (e.g. broken EDID or broken
	 *   identification).
	 * - Because the eDP drivers didn't wire up the AUX bus properly.
	 *   NOTE that, though this is a marginally valid reason,
	 *   some justification needs to be made for why the platform can't
	 *   wire up the AUX bus properly.
	 *
	 * In all other cases the platform should use the aux-bus and declare
	 * the panel using the 'edp-panel' compatible as a device on the AUX
	 * bus.
	 */
	{
		.compatible = "auo,b101ean01",
		.data = (ulong)&auo_b101ean01,
	}, {
		.compatible = "auo,b116xa01",
		.data = (ulong)&auo_b116xak01,
	}, {
		.compatible = "auo,b133htn01",
		.data = (ulong)&auo_b133htn01,
	}, {
		.compatible = "auo,b133xtn01",
		.data = (ulong)&auo_b133xtn01,
	}, {
		.compatible = "boe,nv101wxmn51",
		.data = (ulong)&boe_nv101wxmn51,
	}, {
		.compatible = "boe,nv110wtm-n61",
		.data = (ulong)&boe_nv110wtm_n61,
	}, {
		.compatible = "boe,nv133fhm-n61",
		.data = (ulong)&boe_nv133fhm_n61,
	}, {
		.compatible = "boe,nv133fhm-n62",
		.data = (ulong)&boe_nv133fhm_n61,
	}, {
		.compatible = "boe,nv140fhmn49",
		.data = (ulong)&boe_nv140fhmn49,
	}, {
		.compatible = "innolux,n116bca-ea1",
		.data = (ulong)&innolux_n116bca_ea1,
	}, {
		.compatible = "innolux,n116bge",
		.data = (ulong)&innolux_n116bge,
	}, {
		.compatible = "innolux,n125hce-gn1",
		.data = (ulong)&innolux_n125hce_gn1,
	}, {
		.compatible = "innolux,p120zdg-bf1",
		.data = (ulong)&innolux_p120zdg_bf1,
	}, {
		.compatible = "kingdisplay,kd116n21-30nv-a010",
		.data = (ulong)&kingdisplay_kd116n21_30nv_a010,
	}, {
		.compatible = "lg,lp079qx1-sp0v",
		.data = (ulong)&lg_lp079qx1_sp0v,
	}, {
		.compatible = "lg,lp097qx1-spa1",
		.data = (ulong)&lg_lp097qx1_spa1,
	}, {
		.compatible = "lg,lp120up1",
		.data = (ulong)&lg_lp120up1,
	}, {
		.compatible = "lg,lp129qe",
		.data = (ulong)&lg_lp129qe,
	}, {
		.compatible = "neweast,wjfh116008a",
		.data = (ulong)&neweast_wjfh116008a,
	}, {
		.compatible = "samsung,lsn122dl01-c01",
		.data = (ulong)&samsung_lsn122dl01_c01,
	}, {
		.compatible = "samsung,ltn140at29-301",
		.data = (ulong)&samsung_ltn140at29_301,
	}, {
		.compatible = "sharp,ld-d5116z01b",
		.data = (ulong)&sharp_ld_d5116z01b,
	}, {
		.compatible = "sharp,lq123p1jx31",
		.data = (ulong)&sharp_lq123p1jx31,
	}, {
		/* sentinel */
	}
};

U_BOOT_DRIVER(rockchip_edp_panel) = {
	.name = "rockchip_edp_panel",
	.id = UCLASS_PANEL,
	.of_match = rockchip_edp_panel_ids,
	.probe = rockchip_edp_panel_probe,
	.priv_auto = sizeof(struct panel_edp),
};
