/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef __RK182X_COMMON_H
#define __RK182X_COMMON_H

#include <linux/sizes.h>

#include "rockchip-common.h"

#define CFG_IRAM_BASE			0x1040000000
#define CFG_SYS_SDRAM_BASE		0x540000000	/* node5 */
#define SDRAM_MAX_SIZE			(SZ_64M + SZ_4M)
#define RISCV_SMODE_TIMER_FREQ		24000000

/* Offset 0~4M for preserved + opensbi */
#define BOARD_RESERVE_MEM_BASE		CFG_SYS_SDRAM_BASE
#define BOARD_RESERVE_MEM_SIZE		SZ_4M

/* atags is original used by RT-Thread, we have to reuse it */
#define PLAT_ATAGS_PHYS_BASE		0x400001000

#ifndef ROCKCHIP_DEVICE_SETTINGS
#define ROCKCHIP_DEVICE_SETTINGS
#endif

#define ENV_MEM_LAYOUT_SETTINGS		\
	"fdt_addr_r=0x540700000\0"	\
	"kernel_addr_r=0x540800000\0"	\
	"ramdisk_addr_r=0x542000000\0"

#define CFG_EXTRA_ENV_SETTINGS		\
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0"	\
	"partitions=" PARTS_DEFAULT	\
	ENV_MEM_LAYOUT_SETTINGS		\
	ROCKCHIP_DEVICE_SETTINGS	\
	"boot_targets=" BOOT_TARGETS "\0"

#endif/* __CONFIG_H */
