// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Rockchip Electronics Co. Ltd.
 *
 * Author: Guochun Huang <hero.huang@rock-chips.com>
 */

#include <drm/drm_mipi_dsi.h>

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <video.h>
#include <backlight.h>
#include <spi.h>
#include <asm/gpio.h>
#include <dm/device.h>
#include <dm/read.h>
#include <dm/uclass.h>
#include <dm/uclass-id.h>
#include <linux/media-bus-format.h>
#include <power/regulator.h>

#include "rk628.h"
#include "panel.h"

void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
	void *p;

	p = kmalloc(len, gfp);
	if (p)
		memcpy(p, src, len);
	return p;
}

static int
dsi_panel_parse_cmds(const u8 *data, int blen, struct panel_cmds *pcmds)
{
	unsigned int len;
	u8 *buf, *bp;
	struct cmd_ctrl_hdr *dchdr;
	int i, cnt;

	if (!pcmds)
		return -EINVAL;

	buf = kmemdup(data, blen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* scan init commands */
	bp = buf;
	len = blen;
	cnt = 0;
	while (len > sizeof(*dchdr)) {
		dchdr = (struct cmd_ctrl_hdr *)bp;

		if (dchdr->dlen > len) {
			pr_err("%s: error, len=%d", __func__, dchdr->dlen);
			return -EINVAL;
		}

		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}

	if (len != 0) {
		pr_err("%s: dcs_cmd=%x len=%d error!", __func__, buf[0], blen);
		kfree(buf);
		return -EINVAL;
	}

	pcmds->cmds = kcalloc(cnt, sizeof(struct cmd_desc), GFP_KERNEL);
	if (!pcmds->cmds) {
		kfree(buf);
		return -ENOMEM;
	}

	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;

	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct cmd_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dchdr = *dchdr;
		pcmds->cmds[i].payload = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}

	return 0;
}

static int dsi_panel_get_cmds(struct rk628 *rk628, ofnode dsi_np)
{
	ofnode np;
	const void *data;
	int len;
	int ret, err;

	np = ofnode_find_subnode(dsi_np, "rk628-panel");
	if (!ofnode_valid(np))
		return -EINVAL;

	data = ofnode_get_property(np, "panel-init-sequence", &len);
	if (data) {
		rk628->panel->on_cmds = kcalloc(1, sizeof(struct panel_cmds), GFP_KERNEL);
		if (!rk628->panel->on_cmds)
			return -ENOMEM;

		err = dsi_panel_parse_cmds(data, len, rk628->panel->on_cmds);
		if (err) {
			printf("rk628 failed to parse dsi panel init sequence\n");
			ret = err;
			goto init_err;
		}
	}

	data = ofnode_get_property(np, "panel-exit-sequence", &len);
	if (data) {
		rk628->panel->off_cmds = kcalloc(1, sizeof(struct panel_cmds), GFP_KERNEL);
		if (!rk628->panel->off_cmds) {
			ret = -ENOMEM;
			goto on_err;
		}

		err = dsi_panel_parse_cmds(data, len, rk628->panel->off_cmds);
		if (err) {
			printf("rk628 failed to parse dsi panel exit sequence\n");
			ret = err;
			goto exit_err;
		}
	}

	return 0;

exit_err:
	kfree(rk628->panel->off_cmds);
on_err:
	kfree(rk628->panel->on_cmds->cmds);
	kfree(rk628->panel->on_cmds->buf);
init_err:
	kfree(rk628->panel->on_cmds);

	return ret;
}

int rk628_panel_info_get(struct rk628 *rk628, ofnode np)
{
	struct rk628_panel_simple *panel;
	struct udevice *dev = rk628->dev;
	int ret;

	panel = devm_kzalloc(dev, sizeof(struct rk628_panel_simple), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev, "panel-power-supply", &panel->supply);
	if (ret && ret != -ENOENT) {
		printf("rk628 failed to get power supply: %d\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "panel-enable-gpios", 0,
				   &panel->enable_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		printf("%s: Cannot get enable GPIO: %d\n", __func__, ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "panel-reset-gpios", 0,
				   &panel->reset_gpio, GPIOD_IS_OUT);
	if (ret && ret != -ENOENT) {
		printf("%s: Cannot get reset GPIO: %d\n", __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "panel-backlight", &panel->backlight);
	if (ret && ret != -ENOENT) {
		printf("%s: Cannot get backlight: %d\n", __func__, ret);
		return ret;
	}

	panel->delay.prepare = dev_read_u32_default(dev, "panel-prepare-delay-ms", 0);
	panel->delay.enable = dev_read_u32_default(dev, "panel-enable-delay-ms", 0);
	panel->delay.disable = dev_read_u32_default(dev, "panel-disable-delay-ms", 0);
	panel->delay.unprepare = dev_read_u32_default(dev, "panel-unprepare-delay-ms", 0);
	panel->delay.reset = dev_read_u32_default(dev, "panel-reset-delay-ms", 0);
	panel->delay.init = dev_read_u32_default(dev, "panel-init-delay-ms", 0);

	rk628->panel = panel;

	if (rk628_output_is_dsi(rk628)) {
		ret = dsi_panel_get_cmds(rk628, np);
		if (ret) {
			dev_err(dev, "failed to get cmds\n");
			return ret;
		}
	}

	return 0;
}

void rk628_panel_prepare(struct rk628 *rk628)
{
	struct rk628_panel_simple *p = rk628->panel;

	if (!p)
		return;

	if (p->supply)
		regulator_set_enable(p->supply, 1);

	if (dm_gpio_is_valid(&p->enable_gpio))
		dm_gpio_set_value(&p->enable_gpio, 1);

	if (p->delay.prepare)
		mdelay(p->delay.prepare);

	if (dm_gpio_is_valid(&p->reset_gpio))
		dm_gpio_set_value(&p->reset_gpio, 1);

	if (p->delay.reset)
		mdelay(p->delay.reset);

	if (dm_gpio_is_valid(&p->reset_gpio))
		dm_gpio_set_value(&p->reset_gpio, 0);

	if (p->delay.init)
		mdelay(p->delay.init);
}

void rk628_panel_enable(struct rk628 *rk628)
{
	struct rk628_panel_simple *p = rk628->panel;

	if (!p)
		return;

	if (p->delay.enable)
		mdelay(p->delay.enable);


	if (p->backlight)
		backlight_enable(p->backlight);
}

void rk628_panel_unprepare(struct rk628 *rk628)
{
	struct rk628_panel_simple *p = rk628->panel;

	if (!p)
		return;

	if (dm_gpio_is_valid(&p->reset_gpio))
		dm_gpio_set_value(&p->reset_gpio, 1);

	if (dm_gpio_is_valid(&p->enable_gpio))
		dm_gpio_set_value(&p->enable_gpio, 0);

	if (rk628->panel->supply)
		regulator_set_enable(p->supply, 0);

	if (p->delay.unprepare)
		mdelay(p->delay.unprepare);
}

void rk628_panel_disable(struct rk628 *rk628)
{
	struct rk628_panel_simple *p = rk628->panel;

	if (!p)
		return;

	if (p->backlight)
		backlight_disable(p->backlight);

	if (p->delay.disable)
		mdelay(p->delay.disable);
}
