/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>
#include <configs/rk3036_common.h>

#define PRODUCT_NAME			"Rockchip Kylin"
#define BOOTLOADER_VERSION		"RK3036_BOOTLOADER_V6"
#define DEFAULT_SERIAL_NUMBER		"0123456789ABCDEF"

#ifndef CONFIG_SPL_BUILD

/* Store env in emmc */
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_SIZE			SZ_32K
#undef CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0 /* emmc */
#define CONFIG_SYS_MMC_ENV_PART		0 /* user area */
#define CONFIG_ENV_OFFSET		(SZ_4M - SZ_64K) /* reserved area */
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT

/* Enable gpt partition table */
#define CONFIG_CMD_GPT
#define CONFIG_RANDOM_UUID
#define CONFIG_EFI_PARTITION
#define PARTS_DEFAULT \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=loader,start=32K,size=4000K,uuid=${uuid_gpt_loader};" \
	"name=reserved,size=64K,uuid=${uuid_gpt_reserved};" \
	"name=misc,size=4M,uuid=${uuid_gpt_misc};" \
	"name=recovery,size=32M,uuid=${uuid_gpt_recovery};" \
	"name=boot_a,size=32M,uuid=${uuid_gpt_boot_a};" \
	"name=boot_b,size=32M,uuid=${uuid_gpt_boot_b};" \
	"name=system_a,size=818M,uuid=${uuid_gpt_system_a};" \
	"name=system_b,size=818M,uuid=${uuid_gpt_system_b};" \
	"name=vendor_a,size=50M,uuid=${uuid_gpt_vendor_a};" \
	"name=vendor_b,size=50M,uuid=${uuid_gpt_vendor_b};" \
	"name=cache,size=100M,uuid=${uuid_gpt_cache};" \
	"name=metadata,size=16M,uuid=${uuid_gpt_metadata};" \
	"name=persist,size=4M,uuid=${uuid_gpt_persist};" \
	"name=userdata,size=-,uuid=${uuid_gpt_userdata};\0" \

#define BOOTARGS \
	"androidboot.serialno=" DEFAULT_SERIAL_NUMBER \
	" androidboot.bootloader=" BOOTLOADER_VERSION \
	"\0"

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
	"partitions=" PARTS_DEFAULT \
	"loadaddr=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"android_bootargs=" BOOTARGS \
	"serial#=" DEFAULT_SERIAL_NUMBER "\0" \

#define CONFIG_BOARD_LATE_INIT
#define CONFIG_PREBOOT

#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_SYS_HUSH_PARSER

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND \
	"echo Failed to boot, enter fastboot...;" \
	"fastboot 0;reset;" \

/* Enable atags */
#define CONFIG_SYS_BOOTPARAMS_LEN	(64*1024)
#define CONFIG_INITRD_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_MISC
#define CONFIG_EFUSE_RK3036

#endif

#endif
