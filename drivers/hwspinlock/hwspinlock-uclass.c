// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <hwspinlock.h>
#include <hwspinlock-uclass.h>

DECLARE_GLOBAL_DATA_PTR;

#define HWSPINLOCK_RETRY_DELAY_US	100

static inline struct hwspinlock_ops *hwspin_lock_dev_ops(struct udevice *dev)
{
	return (struct hwspinlock_ops *)dev->driver->ops;
}

static int hwspin_lock_of_xlate_default(struct hwspinlock *hwlock,
				 struct ofnode_phandle_args *args)
{
	debug("%s\n", __func__);

	if (args->args_count != 1) {
		debug("Invaild args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	hwlock->id = args->args[0];

	return 0;
}

int hwspin_lock_get_id_by_index(struct udevice *dev, int index, struct hwspinlock *hwlock)
{
	struct udevice *dev_hwlock;
	struct ofnode_phandle_args args;
	int ret;

	debug("%s(dev=%s, index=%d)\n", __func__, dev->name, index);

	ret = dev_read_phandle_with_args(dev, "hwlocks", "#hwlock-cells", 0, index,
					 &args);
	if (ret) {
		debug("%s: dev_read_phandle_with_args failed: %d\n", __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_HWSPINLOCK, args.node, &dev_hwlock);
	if (ret) {
		debug("%s: uclass_get_device_by_of_offset failed: %d\n", __func__, ret);
		return ret;
	}

	hwlock->dev = dev_hwlock;

	ret = hwspin_lock_of_xlate_default(hwlock, &args);
	if (ret) {
		debug("of_xlate() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

int hwspin_lock_get_id_by_name(struct udevice *dev, const char *name,
		     struct hwspinlock *hwlock)
{
	int index;

	debug("%s(dev=%s, name=%s)\n", __func__, dev->name, name);

	index = dev_read_stringlist_search(dev, "hwlock-names", name);
	if (index < 0) {
		debug("fdt_stringlist_search() failed: %d\n", index);
		return index;
	}

	return hwspin_lock_get_id_by_index(dev, index, hwlock);
}

int hwspin_trylock_timeout(struct hwspinlock *hwlock, ulong timeout_us)
{
	struct hwspinlock_ops *ops = hwspin_lock_dev_ops(hwlock->dev);
	int ret;
	ulong start_time;

	debug("%s(id=%d, timeout_us=%ld)\n", __func__, hwlock->id, timeout_us);

	start_time = timer_get_us();

	/*
	 * Account for partial us ticks, but if timeout_us is 0, ensure we
	 * still don't wait at all.
	 */
	if (timeout_us)
		timeout_us++;

	for (;;) {
		ret = ops->trylock(hwlock);
		if (ret)
			return 0;
		if ((timer_get_us() - start_time) >= timeout_us)
			return -ETIMEDOUT;
	}
}

void hwspin_lock(struct hwspinlock *hwlock)
{
	struct hwspinlock_ops *ops = hwspin_lock_dev_ops(hwlock->dev);
	int ret;

	debug("%s(id=%d)\n", __func__, hwlock->id);

	do {
		ret = ops->trylock(hwlock);
		udelay(HWSPINLOCK_RETRY_DELAY_US);
	} while (!ret);
}

void hwspin_unlock(struct hwspinlock *hwlock)
{
	struct hwspinlock_ops *ops;

	if (WARN_ON(!hwlock))
		return;

	debug("%s(id=%d)\n", __func__, hwlock->id);

	ops = hwspin_lock_dev_ops(hwlock->dev);
	ops->unlock(hwlock);
}

UCLASS_DRIVER(hwspinlock) = {
	.id		= UCLASS_HWSPINLOCK,
	.name		= "hwspinlock",
};
