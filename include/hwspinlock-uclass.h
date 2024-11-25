/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd
 */

#ifndef _HWSPINLOCK_UCLASS_H
#define _HWSPINLOCK_UCLASS_H

#include <hwspinlock.h>

struct udevice;

/**
 * struct hwspinlock_ops - platform-specific hwspinlock handlers
 *
 * @trylock: make a single attempt to take the lock. returns 0 on
 *	     failure and true on success. may _not_ sleep.
 * @unlock:  release the lock. always succeed. may _not_ sleep.
 */
struct hwspinlock_ops {
	int (*trylock)(struct hwspinlock *lock);
	void (*unlock)(struct hwspinlock *lock);
};

#endif
