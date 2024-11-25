/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd
 */

#ifndef _HWSPINLOCK_H
#define _HWSPINLOCK_H

struct udevice;

/**
 * struct hwspinlock - this struct represents a single hwspinlock instance
 * @dev:      the device which implements the HWSpinlock.
 * @id:       the hwspinlock ID within the provider.
 */
struct hwspinlock {
	struct udevice *dev;
	unsigned int id; /*Written by of_xlate. */
};

/**
 * hwspin_lock_get_id_by_index - Get/request a lock id by integer index
 *
 * This function provides a means for DT users of the hwspinlock module to
 * get the global lock id of a specific hwspinlock using the phandle of the
 * hwspinlock device.
 *
 * @dev:	the device from which to request the specific hwlock.
 * @index:      index of the hwlock in the list of values
 * @hwlock	a pointer to a hwspinlock object to initialize.
 * @return 0 if OK, or a negative error code.
 */
int hwspin_lock_get_id_by_index(struct udevice *dev, int index,
				struct hwspinlock *hwlock);

/**
 * hwspin_lock_get_id_by_index - Get/request a lock id by name
 *
 * This function provides a means for DT users of the hwspinlock module to
 * get the global lock id of a specific hwspinlock using the specified name of
 * the hwspinlock device.
 *
 * @dev:	the device from which to request the specific hwlock.
 * @name:       hwlock name
 * @hwlock	a pointer to a hwspinlock object to initialize.
 * @return 0 if OK, or a negative error code.
 */
int hwspin_lock_get_id_by_name(struct udevice *dev, const char *name,
			       struct hwspinlock *hwlock);

/**
 * hwspin_trylock_timeout - lock an hwspinlock with timeout limit.
 *
 * This function locks the given @hwlock. If the @hwlock
 * is already taken, the function will busy loop waiting for it to
 * be released, but give up after @timeout msecs have elapsed.
 *
 * @hwlock:	the hwspinlock to be locked.
 * @timeout_us: timeout value in microseconds.
 * Returns 0 when the @hwlock was successfully taken, and an appropriate
 * error code otherwise (most notably -ETIMEDOUT if the @hwlock is still
 * busy after @timeout msecs).
 */
int hwspin_trylock_timeout(struct hwspinlock *hwlock, ulong timeout_us);

/**
 * hwspin_lock - lock an hwspinlock.
 *
 * This function locks the given @hwlock. If the @hwlock
 * is already taken, the function will busy loop waiting for it to
 * be released.
 *
 * @hwlock:	the hwspinlock to be locked.
 */
void hwspin_lock(struct hwspinlock *hwlock);

/**
 * hwspin_unlock - unlock a specific hwspinlock
 *
 * This function will unlock a specific hwspinlock, @hwlock must be already
 * locked before calling this function:
 *
 * @hwlock:	a previously-acquired hwspinlock which we want to unlock.
 */
void hwspin_unlock(struct hwspinlock *hwlock);

#endif
