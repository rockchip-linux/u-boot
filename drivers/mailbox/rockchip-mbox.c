// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Fuzhou Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <mailbox-uclass.h>
#include <asm/io.h>
#include <rockchip/mailbox.h>

#define MAILBOX_A2B_INTEN	0x00
#define MAILBOX_A2B_STATUS	0x04
#define MAILBOX_A2B_CMD		0x08
#define MAILBOX_A2B_DAT		0x0c
#define MAILBOX_B2A_INTEN	0x10
#define MAILBOX_B2A_STATUS	0x14
#define MAILBOX_B2A_CMD		0x18
#define MAILBOX_B2A_DAT		0x1c

#define MAILBOX_TRIGGER_SHIFT	8
#define BIT_WRITEABLE_SHIFT	16

struct rockchip_mbox_priv {
	void *base;
	/* 0 = write cmd, 1 = write cmd first, then write data */
	unsigned char trigger_method;
};

static int rockchip_mbox_request(struct mbox_chan *chan)
{
	return 0;
}

static int rockchip_mbox_free(struct mbox_chan *chan)
{
	return 0;
}

static int rockchip_mbox_send(struct mbox_chan *chan, const void *data)
{
	struct rockchip_mbox_priv *priv = dev_get_priv(chan->dev);
	const struct rockchip_mbox_msg *msg = data;

	if (priv->trigger_method) {
		writel(msg->cmd, priv->base + MAILBOX_A2B_CMD);
		writel(msg->data, priv->base + MAILBOX_A2B_DAT);
	} else {
		writel(msg->cmd, priv->base + MAILBOX_A2B_CMD);
	}

	return 0;
}

static int rockchip_mbox_recv(struct mbox_chan *chan, void *data)
{
	return 0;
}

static int rockchip_mbox_bind(struct udevice *dev)
{
	return 0;
}

static int rockchip_mbox_ofdata_to_platdata(struct udevice *dev)
{
	struct rockchip_mbox_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);

	return 0;
}

static int rockchip_mbox_probe(struct udevice *dev)
{
	struct rockchip_mbox_priv *priv = dev_get_priv(dev);

	if (dev_read_bool(dev, "rockchip,enable-cmd-trigger"))
		priv->trigger_method = 0;
	else
		priv->trigger_method = 1;

	/* Set the TX interrupt trigger method */
	writel((1U << (BIT_WRITEABLE_SHIFT + MAILBOX_TRIGGER_SHIFT) |
	       (priv->trigger_method << MAILBOX_TRIGGER_SHIFT)),
	       priv->base + MAILBOX_A2B_INTEN);

	return 0;
}

static const struct udevice_id rockchip_mbox_ids[] = {
	{ .compatible = "rockchip,rk3576-mailbox" },
	{ }
};

struct mbox_ops rockchip_mbox_mbox_ops = {
	.request = rockchip_mbox_request,
	.free = rockchip_mbox_free,
	.send = rockchip_mbox_send,
	.recv = rockchip_mbox_recv,
};

U_BOOT_DRIVER(rockchip_mbox) = {
	.name = "rockchip_mbox",
	.id = UCLASS_MAILBOX,
	.of_match = rockchip_mbox_ids,
	.bind = rockchip_mbox_bind,
	.priv_auto_alloc_size = sizeof(struct rockchip_mbox_priv),
	.ofdata_to_platdata = rockchip_mbox_ofdata_to_platdata,
	.probe = rockchip_mbox_probe,
	.ops = &rockchip_mbox_mbox_ops,
};
