/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _ROCKCHIP_COMMON_H_
#define _ROCKCHIP_COMMON_H_
#include <linux/sizes.h>

#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>

/* First try to boot from SD (index 0), then eMMC (index 1 */
#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(USB, usb, 0) \
	func(PXE, pxe, na) \
	func(DHCP, dchp, na)
#else
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(PXE, pxe, na) \
	func(DHCP, dchp, na)
#endif

#define CONFIG_RANDOM_UUID

#ifdef CONFIG_ARM64
#define FDTFILE "rockchip/" CONFIG_DEFAULT_DEVICE_TREE ".dtb" "\0"
#else
#define FDTFILE CONFIG_DEFAULT_DEVICE_TREE ".dtb" "\0"
#endif

#ifdef CONFIG_ARM64
#define ROOT_UUID "B921B045-1DF0-41C3-AF44-4C6F280D3FAE;\0"
#else
#define ROOT_UUID "69DAD710-2CE4-4E3C-B16C-21A1D49ABED3;\0"
#endif
#define PARTS_DEFAULT \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=loader1,start=32K,size=4000K,uuid=${uuid_gpt_loader1};" \
	"name=loader2,start=8MB,size=4MB,uuid=${uuid_gpt_loader2};" \
	"name=trust,size=4M,uuid=${uuid_gpt_atf};" \
	"name=boot,size=112M,bootable,uuid=${uuid_gpt_boot};" \
	"name=rootfs,size=-,uuid="ROOT_UUID

#define PARTS_RKIMG \
	"uuid_disk=${uuid_gpt_disk};" \
	"name=uboot,start=8MB,size=4MB,uuid=${uuid_gpt_loader2};" \
	"name=trust,size=4M,uuid=${uuid_gpt_atf};" \
	"name=misc,size=4MB,uuid=${uuid_gpt_misc};" \
	"name=resource,size=16MB,uuid=${uuid_gpt_resource};" \
	"name=kernel,size=32M,uuid=${uuid_gpt_kernel};" \
	"name=boot,size=32M,bootable,uuid=${uuid_gpt_boot};" \
	"name=recovery,size=32M,uuid=${uuid_gpt_recovery};" \
	"name=backup,size=112M,uuid=${uuid_gpt_backup};" \
	"name=cache,size=512M,uuid=${uuid_gpt_cache};" \
	"name=system,size=2048M,uuid=${uuid_gpt_system};" \
	"name=metadata,size=16M,uuid=${uuid_gpt_metadata};" \
	"name=vendor,size=32M,uuid=${uuid_gpt_vendor};" \
	"name=oem,size=32M,uuid=${uuid_gpt_oem};" \
	"name=frp,size=512K,uuid=${uuid_gpt_frp};" \
	"name=security,size=2M,uuid=${uuid_gpt_security};" \
	"name=userdata,size=-,uuid=${uuid_gpt_userdata};"

#define RKIMG_BOOTCOMMAND \
	"if mmc dev 0; then setenv devtype mmc; setenv devnum 0;" \
	"else if rknand dev 0; then setenv devtype mmc; setenv devnum 0; fi;" \
	"fi; boot_android ${devtype} ${devnum};" \
	"bootrkp;"

#endif

/*
 * Rockchip SoCs use fixed ENV 32KB@(4MB-32KB)
 */
#define CONFIG_ENV_OFFSET	(SZ_4M - SZ_32K)
#define CONFIG_ENV_SIZE		SZ_32K

#define CONFIG_DISPLAY_BOARDINFO_LATE

#endif /* _ROCKCHIP_COMMON_H_ */
