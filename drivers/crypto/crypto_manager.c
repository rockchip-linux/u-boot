// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd
 * Author: Troy Lin <troy.lin@rock-chips.com>
 */
#include <common.h>
#include <crypto_manager.h>
#include <dm.h>
#include <linux/err.h>
#include <linux/list.h>
#include <log.h>
#include <malloc.h>

//#define DEBUG

#ifdef DEBUG
#define DMSG(format, ...) printf("[%s %s, %05d]-trace: " format "\n", \
				 __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DMSG(format, ...)
#endif

enum crypto_impl_status {
	CRYPTO_IMPL_EMPTY = 0,
	CRYPTO_IMPL_UNINITED,
	CRYPTO_IMPL_INITED,
	CRYPTO_IMPL_FAILED,
};

struct crypto_driver_node {
	struct list_head	list;
	const char		*driver_name;
	const struct crypto_impl *impl;
	enum crypto_impl_status status;
};

static struct list_head crypto_algo_lists[CRYPTO_TYPE_MAX] = {
	[CRYPTO_TYPE_HASH]   = LIST_HEAD_INIT(crypto_algo_lists[CRYPTO_TYPE_HASH]),
	[CRYPTO_TYPE_HMAC]   = LIST_HEAD_INIT(crypto_algo_lists[CRYPTO_TYPE_HMAC]),
	[CRYPTO_TYPE_CIPHER] = LIST_HEAD_INIT(crypto_algo_lists[CRYPTO_TYPE_CIPHER]),
	[CRYPTO_TYPE_AEAD]   = LIST_HEAD_INIT(crypto_algo_lists[CRYPTO_TYPE_AEAD]),
	[CRYPTO_TYPE_MAC]    = LIST_HEAD_INIT(crypto_algo_lists[CRYPTO_TYPE_MAC]),
	[CRYPTO_TYPE_ASYM]   = LIST_HEAD_INIT(crypto_algo_lists[CRYPTO_TYPE_ASYM]),
};

static bool crypto_check_node_valid(struct crypto_driver_node *node)
{
	const struct crypto_impl *impl = node->impl;

	if (!impl)
		return false;

	if (node->status != CRYPTO_IMPL_UNINITED && node->status != CRYPTO_IMPL_INITED)
		return false;

	if (node->status == CRYPTO_IMPL_UNINITED) {
		struct udevice *crypto_dev;
		int ret;

		ret = uclass_get_device_by_driver(impl->uclass_id, impl->dev->driver, &crypto_dev);
		if (ret) {
			debug("driver_name %s , dev = %p, driver = %p is not exist.\n",
				  crypto_get_driver_name(impl), impl->dev, impl->dev->driver);
			node->status = CRYPTO_IMPL_FAILED;
			return false;
		}

		node->status = CRYPTO_IMPL_INITED;
	}

	return true;
}

static struct crypto_driver_node *crypto_list_find_driver(const struct list_head *head,
							  const char *driver_name, u32 index)
{
	struct crypto_driver_node *node = NULL;
	struct list_head *save = NULL;
	struct list_head *pos = NULL;
	u32 idx = 0;

	list_for_each_safe(pos, save, head) {
		node = list_entry(pos, struct crypto_driver_node, list);

		DMSG("node = %p, %s, idx = %u, index = %u\n",
		     node, node->driver_name, idx, index);

		if (!crypto_check_node_valid(node))
			continue;

		if (!driver_name || strcmp(node-> driver_name, driver_name) == 0) {
			if (idx == index)
				return node;

			idx++;
		}
	}

	return NULL;
}

int crypto_impl_register(const struct crypto_impl *impl)
{
	struct crypto_driver_node *new_node = NULL;
	struct crypto_driver_node *node = NULL;
	struct list_head *save = NULL;
	struct list_head *pos = NULL;
	struct list_head *head;
	const char *driver_name;

	if (!gd_reloc_available())
		return 0;

	if (!impl || impl->type >= CRYPTO_TYPE_MAX)
		return -EINVAL;

	head = &crypto_algo_lists[impl->type];
	driver_name = crypto_get_driver_name(impl);
	if (!driver_name) {
		DMSG("register with empty driver_name.\n");
		return -EINVAL;
	}

	list_for_each_safe(pos, save, head) {
		node = list_entry(pos, struct crypto_driver_node, list);
		if (driver_name && !strcmp(node->driver_name, driver_name)) {
			if (impl->type == CRYPTO_TYPE_ASYM &&
			    node->impl->asym.algo != impl->asym.algo) {
				continue;
			}

			DMSG("driver_name %s is already exist, cannot be registered multiple times\n",
			     driver_name);
			return -EINVAL;
		}
	}

	new_node = calloc(1, sizeof(*new_node));
	if (!new_node)
		return -ENOMEM;

	new_node->impl        = impl;
	new_node->driver_name = driver_name;
	new_node->status      = CRYPTO_IMPL_UNINITED;
	list_add_tail(&new_node->list, head);

	DMSG("%s: %s registered ok!\n", CRYPTO_MISC_MANAGER, driver_name);

	return 0;
}

void crypto_impl_unregister(const struct crypto_impl *impl)
{
	struct crypto_driver_node *node;
	const struct list_head *head;

	if (!gd_reloc_available())
		return;

	if (!impl || impl->type >= CRYPTO_TYPE_MAX)
		return;

	head = &crypto_algo_lists[impl->type];
	node = crypto_list_find_driver(head, crypto_get_driver_name(impl), 0);
	if (node)
		list_del(&node->list);
}

const struct crypto_impl *crypto_get_impl(enum crypto_type type, u32 algo, u32 mode)
{
	const struct crypto_impl *best_fit_algt = NULL;
	const struct crypto_impl *impl = NULL;
	struct crypto_driver_node *node = NULL;
	struct list_head *pos = NULL;
	struct list_head *save = NULL;
	u32 best_priority = 0;
	u32 this_priority = 0;

	if (!gd_reloc_available())
		return NULL;

	if (type >= CRYPTO_TYPE_MAX)
		return NULL;

	list_for_each_safe(pos, save, &crypto_algo_lists[type]) {
		node = list_entry(pos, struct crypto_driver_node, list);
		impl = node->impl;
		DMSG("node = %p, %s\n", node, node->driver_name);

		if (!crypto_check_node_valid(node))
			continue;

		if (impl->dynamic_priority)
			this_priority = impl->dynamic_priority(impl->dev, algo, mode);
		else
			this_priority = impl->priority;

		if (impl->check_valid && impl->check_valid(impl->dev, algo, mode)) {
			if (!best_fit_algt || this_priority > best_priority) {
				best_fit_algt = impl;
				best_priority = this_priority;
			}
		}
	}

	DMSG("using impl %s\n", crypto_get_driver_name(best_fit_algt));
	return best_fit_algt;
}

const struct crypto_impl *crypto_get_impl_by_index(enum crypto_type type, u32 index)
{
	struct crypto_driver_node *node = NULL;

	if (!gd_reloc_available())
		return NULL;

	if (type >= CRYPTO_TYPE_MAX)
		return NULL;

	node = crypto_list_find_driver(&crypto_algo_lists[type], NULL, index);
	if (!node)
		return NULL;

	return node->impl;
}

const char *crypto_get_driver_name(const struct crypto_impl *impl)
{
	if (!gd_reloc_available())
		return NULL;

	if (!impl || !impl->dev || !impl->dev->driver || !impl->dev->driver->name)
		return NULL;

	return impl->dev->driver->name;
}

U_BOOT_DRIVER(crypto_manager) = {
	.name      = CRYPTO_MISC_MANAGER,
	.id        = UCLASS_MISC,
};
