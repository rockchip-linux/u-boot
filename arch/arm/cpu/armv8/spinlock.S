/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <asm/macro.h>
#include <asm-offsets.h>
#include <config.h>
#include <linux/linkage.h>

	.globl	__spin_lock
	.globl	__spin_unlock

/*
 * Acquire lock using load-/store-exclusive instruction pair.
 *
 * void __spin_lock(spinlock_t *lock);
 */
ENTRY(__spin_lock)
	mov	w2, #1
	sevl
l1: wfe
l2: ldaxr	w1, [x0]
	cbnz	w1, l1
	stxr	w1, w2, [x0]
	cbnz	w1, l2
	ret
ENDPROC(__spin_lock)

/*
 * Release lock previously acquired by __spin_lock.
 *
 * Use store-release to unconditionally clear the spinlock variable.
 * Store operation generates an event to all cores waiting in WFE
 * when address is monitored by the global monitor.
 *
 * void __spin_unlock(spinlock_t *lock);
 */
ENTRY(__spin_unlock)
	stlr	wzr, [x0]
	ret
ENDPROC(__spin_unlock)
