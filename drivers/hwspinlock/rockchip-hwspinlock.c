// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <hwspinlock-uclass.h>
#include <asm/io.h>

/* Hardware spinlock register offsets */
#define HWSPINLOCK_OFFSET(x)	(0x4 * (x))
#define HWSPINLOCK_ID_MASK	0x0F
#define HWLOCK_DEFAULT_USER	0x01

static u32 hwlock_user_id;

static fdt_addr_t get_lock_addr(struct hwspinlock *lock, uint32_t id)
{
	return dev_read_addr(lock->dev) + HWSPINLOCK_OFFSET(lock->id);
}

static int rockchip_hwspinlock_trylock(struct hwspinlock *lock)
{
	void __iomem *lock_addr = (void __iomem *)get_lock_addr(lock, lock->id);

	debug("%s(lock=%p, id=%d)\n", __func__, lock_addr, lock->id);

	writel(hwlock_user_id, lock_addr);

	/*
	 * Get only first 4 bits and compare to HWSPINLOCK_OWNER_ID,
	 * if equal, we attempt to acquire the lock, otherwise,
	 * someone else has it.
	 */
	return (hwlock_user_id == (readl(lock_addr) & HWSPINLOCK_ID_MASK));
}

static void rockchip_hwspinlock_unlock(struct hwspinlock *lock)
{
	void __iomem *lock_addr = (void __iomem *)get_lock_addr(lock, lock->id);
	u32 lock_owner = readl(lock_addr) & HWSPINLOCK_ID_MASK;

	debug("%s(lock=%p, id=%d, owner=%d)\n", __func__, lock_addr, lock->id, lock_owner);

	if (lock_owner != hwlock_user_id) {
		printf("WARNING: against user %u release a lock held by %u\n",
			hwlock_user_id, lock_owner);
		return;
	}

	/* Release the lock by writing 0 to it */
	writel(0, lock_addr);
}

static int rockchip_hwspinlock_probe(struct udevice *dev)
{
	int ret;

	ret = dev_read_u32_array(dev, "rockchip,hwlock-user-id", &hwlock_user_id, 1);
	if (ret || !hwlock_user_id || hwlock_user_id > HWSPINLOCK_ID_MASK)
		hwlock_user_id = HWLOCK_DEFAULT_USER;

	debug("%s(dev=%s, hwlock_user_id=%u)\n", __func__, dev->name, hwlock_user_id);

	return 0;
}

static const struct udevice_id rockchip_hwspinlock_ids[] = {
	{ .compatible = "rockchip,hwspinlock" },
	{ }
};

struct hwspinlock_ops rockchip_hwspinlock_ops = {
	.trylock = rockchip_hwspinlock_trylock,
	.unlock = rockchip_hwspinlock_unlock,
};

U_BOOT_DRIVER(rockchip_hwspinlock) = {
	.name = "rockchip_hwspinlock",
	.id = UCLASS_HWSPINLOCK,
	.of_match = rockchip_hwspinlock_ids,
	.probe = rockchip_hwspinlock_probe,
	.ops = &rockchip_hwspinlock_ops,
};
