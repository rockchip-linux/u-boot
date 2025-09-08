/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

void __spin_lock(uspinlock_t *lock);
void __spin_unlock(uspinlock_t *lock);

void u_spin_lock(uspinlock_t *lock)
{
	if (!(gd->flags & GD_FLG_SMP))
		return;

	__spin_lock(lock);
}

void u_spin_unlock(uspinlock_t *lock)
{
	if (!(gd->flags & GD_FLG_SMP))
		return;

	__spin_unlock(lock);
}

