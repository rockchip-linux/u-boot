/*
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
 * Written by Jean-Jacques Hiblot  <jjhiblot@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <linux/list.h>

/**
 * struct phy_counts - Init and power-on counts of a single PHY port
 *
 * This structure is used to keep track of PHY initialization and power
 * state change requests, so that we don't power off and deinitialize a
 * PHY instance until all of its users want it done. Otherwise, multiple
 * consumers using the same PHY port can cause problems (e.g. one might
 * call power_off() after another's exit() and hang indefinitely).
 *
 * @id: The PHY ID within a PHY provider
 * @power_on_count: Times generic_phy_power_on() was called for this ID
 *                  without a matching generic_phy_power_off() afterwards
 * @init_count: Times generic_phy_init() was called for this ID
 *              without a matching generic_phy_exit() afterwards
 * @list: Handle for a linked list of these structures corresponding to
 *        ports of the same PHY provider
 */
struct phy_counts {
	unsigned long id;
	int power_on_count;
	int init_count;
	struct list_head list;
};

DECLARE_GLOBAL_DATA_PTR;

static inline struct phy_ops *phy_dev_ops(struct udevice *dev)
{
	return (struct phy_ops *)dev->driver->ops;
}

static struct phy_counts *phy_get_counts(struct phy *phy)
{
	struct list_head *uc_priv;
	struct phy_counts *counts;

	if (!generic_phy_valid(phy))
		return NULL;

	uc_priv = dev_get_uclass_priv(phy->dev);
	list_for_each_entry(counts, uc_priv, list)
		if (counts->id == phy->id)
			return counts;

	return NULL;
}

static int phy_alloc_counts(struct phy *phy)
{
	struct list_head *uc_priv;
	struct phy_counts *counts;

	if (!generic_phy_valid(phy))
		return 0;
	if (phy_get_counts(phy))
		return 0;

	uc_priv = dev_get_uclass_priv(phy->dev);
	counts = kzalloc(sizeof(*counts), GFP_KERNEL);
	if (!counts)
		return -ENOMEM;

	counts->id = phy->id;
	counts->power_on_count = 0;
	counts->init_count = 0;
	list_add(&counts->list, uc_priv);

	return 0;
}

static int phy_uclass_pre_probe(struct udevice *dev)
{
	struct list_head *uc_priv = dev_get_uclass_priv(dev);

	INIT_LIST_HEAD(uc_priv);

	return 0;
}

static int phy_uclass_pre_remove(struct udevice *dev)
{
	struct list_head *uc_priv = dev_get_uclass_priv(dev);
	struct phy_counts *counts, *next;

	list_for_each_entry_safe(counts, next, uc_priv, list)
		kfree(counts);

	return 0;
}

static int generic_phy_xlate_offs_flags(struct phy *phy,
					struct ofnode_phandle_args *args)
{
	debug("%s(phy=%p)\n", __func__, phy);

	if (args->args_count > 1) {
		debug("Invaild args_count: %d\n", args->args_count);
		return -EINVAL;
	}

	if (args->args_count)
		phy->id = args->args[0];
	else
		phy->id = 0;

	return 0;
}

int generic_phy_get_by_index(struct udevice *dev, int index,
			     struct phy *phy)
{
	struct ofnode_phandle_args args;
	struct phy_ops *ops;
	int ret;
	struct udevice *phydev;

	debug("%s(dev=%p, index=%d, phy=%p)\n", __func__, dev, index, phy);

	assert(phy);
	phy->dev = NULL;
	ret = dev_read_phandle_with_args(dev, "phys", "#phy-cells", 0, index,
					 &args);
	if (ret) {
		debug("%s: dev_read_phandle_with_args failed: err=%d\n",
		      __func__, ret);
		return ret;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_PHY, args.node, &phydev);
	if (ret) {
		debug("%s: uclass_get_device_by_ofnode failed: err=%d\n",
		      __func__, ret);
		return ret;
	}

	phy->dev = phydev;

	ops = phy_dev_ops(phydev);

	if (ops->of_xlate)
		ret = ops->of_xlate(phy, &args);
	else
		ret = generic_phy_xlate_offs_flags(phy, &args);
	if (ret) {
		debug("of_xlate() failed: %d\n", ret);
		goto err;
	}

	ret = phy_alloc_counts(phy);
	if (ret) {
		debug("phy_alloc_counts() failed: %d\n", ret);
		goto err;
	}

	return 0;

err:
	return ret;
}

int generic_phy_get_by_name(struct udevice *dev, const char *phy_name,
			    struct phy *phy)
{
	int index;

	debug("%s(dev=%p, name=%s, phy=%p)\n", __func__, dev, phy_name, phy);

	index = dev_read_stringlist_search(dev, "phy-names", phy_name);
	if (index < 0) {
		debug("dev_read_stringlist_search() failed: %d\n", index);
		return index;
	}

	return generic_phy_get_by_index(dev, index, phy);
}

int generic_phy_init(struct phy *phy)
{
	struct phy_counts *counts;
	struct phy_ops const *ops;
	int ret;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);
	if (!ops->init)
		return 0;

	counts = phy_get_counts(phy);
	if (counts->init_count > 0) {
		counts->init_count++;
		return 0;
	}

	ret = ops->init(phy);
	if (ret)
		dev_err(phy->dev, "PHY: Failed to init %s: %d.\n",
			phy->dev->name, ret);
	else
		counts->init_count = 1;

	return ret;
}

int generic_phy_reset(struct phy *phy)
{
	struct phy_ops const *ops;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);

	return ops->reset ? ops->reset(phy) : 0;
}

int generic_phy_exit(struct phy *phy)
{
	struct phy_counts *counts;
	struct phy_ops const *ops;
	int ret;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);
	if (!ops->exit)
		return 0;

	counts = phy_get_counts(phy);
	if (counts->init_count == 0)
		return 0;
	if (counts->init_count > 1) {
		counts->init_count--;
		return 0;
	}

	ret = ops->exit(phy);
	if (ret)
		dev_err(phy->dev, "PHY: Failed to exit %s: %d.\n",
			phy->dev->name, ret);
	else
		counts->init_count = 0;

	return ret;
}

int generic_phy_power_on(struct phy *phy)
{
	struct phy_counts *counts;
	struct phy_ops const *ops;
	int ret;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);
	if (!ops->power_on)
		return 0;

	counts = phy_get_counts(phy);
	if (counts->power_on_count > 0) {
		counts->power_on_count++;
		return 0;
	}

	ret = ops->power_on(phy);
	if (ret)
		dev_err(phy->dev, "PHY: Failed to power on %s: %d.\n",
			phy->dev->name, ret);
	else
		counts->power_on_count = 1;

	return 0;
}

int generic_phy_power_off(struct phy *phy)
{
	struct phy_counts *counts;
	struct phy_ops const *ops;
	int ret;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);
	if (!ops->power_off)
		return 0;

	counts = phy_get_counts(phy);
	if (counts->power_on_count == 0)
		return 0;
	if (counts->power_on_count > 1) {
		counts->power_on_count--;
		return 0;
	}

	ret = ops->power_off(phy);
	if (ret)
		dev_err(phy->dev, "PHY: Failed to power off %s: %d.\n",
			phy->dev->name, ret);
	else
		counts->power_on_count = 0;

	return 0;
}

int generic_phy_configure(struct phy *phy, union phy_configure_opts *opts)
{
	struct phy_ops const *ops;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);

	return ops->configure ? ops->configure(phy, opts) : 0;
}

int generic_phy_validate(struct phy *phy, enum phy_mode mode, int submode,
			 union phy_configure_opts *opts)
{
	struct phy_ops const *ops;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);

	return ops->validate ? ops->validate(phy, mode, submode, opts) : 0;
}

int generic_phy_set_mode_ext(struct phy *phy, enum phy_mode mode, int submode)
{
	struct phy_ops const *ops;
	int ret;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);

	if (!ops->set_mode)
		return 0;

	ret = ops->set_mode(phy, mode, submode);
	if (!ret)
		phy->attrs.mode = mode;

	return ret;
}

int generic_phy_calibrate(struct phy *phy)
{
	struct phy_ops const *ops;

	if (!generic_phy_valid(phy))
		return 0;
	ops = phy_dev_ops(phy->dev);

	return ops->calibrate ? ops->calibrate(phy) : 0;
}

UCLASS_DRIVER(phy) = {
	.id		= UCLASS_PHY,
	.name		= "phy",
	.pre_probe	= phy_uclass_pre_probe,
	.pre_remove	= phy_uclass_pre_remove,
	.per_device_auto_alloc_size = sizeof(struct list_head),
};
