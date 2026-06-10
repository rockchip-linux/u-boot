// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd.
 */
#include <common.h>
#include <blk.h>
#include <env.h>
#include <fdt_support.h>
#include <hotkey.h>
#include <image.h>
#include <malloc.h>
#include <part.h>
#include <xbc.h>
#include <timestamp.h>
#include <version.h>
#include <android_ab.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/atags.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/common.h>

#include <display_options.h>
#include <dm.h>
#include <env.h>
#include <linux/stringify.h>
#include <linux/string.h>

#ifdef CONFIG_MTD_BLK
#include "mtd_blk.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

extern int misc_get_recovery_msg(void);

static void bootargs_add_fuse(void)
{
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
	struct tag *t = atags_get_tag(ATAG_PUB_KEY);

	if (t) {
		/* Pass if efuse/otp programmed */
		if (t->u.pub_key.flag == PUBKEY_FUSE_PROGRAMMED)
			env_update("bootargs", "fuse.programmed=1");
		else
			env_update("bootargs", "fuse.programmed=0");
	}
#endif
}

static void bootargs_add_misc(void)
{
	struct blk_desc *dev_desc;
	int uclass_id, devnum;

	dev_desc = plat_bootdev();
	if (!dev_desc)
		return;

	/*
	 * 1. From rk356x, the sd/udisk recovery update flag was moved from
	 *    IDB to Android BCB.
	 *
	 * 2. Udisk is init at the late boot_from_udisk(), but
	 *    plat_boot_mode() actually only read once,
	 *    we need to update boot mode according to udisk BCB.
	 */
	uclass_id = dev_desc->uclass_id;
	devnum = dev_desc->devnum;
	if ((uclass_id == UCLASS_MMC && devnum == 1) || (uclass_id == UCLASS_USB)) {
		if (misc_get_recovery_msg() == BCB_MSG_RECOVERY_RK_FWUPDATE) {
			if (uclass_id == UCLASS_MMC && devnum == 1) {
				env_update("bootargs", "sdfwupdate");
			} else if (uclass_id == UCLASS_USB) {
				env_update("bootargs", "usbfwupdate");
				env_set("reboot_mode", "recovery-usb");
			}
		} else {
			if (uclass_id == UCLASS_USB)
				env_set("reboot_mode", "normal");
		}
	}

	if (plat_boot_mode() == BOOT_MODE_QUIESCENT)
		env_update("bootargs", "androidboot.quiescent=1 pwm_bl.quiescent=1");

	/* PCBA test needs more permission */
	if (misc_get_recovery_msg() == BCB_MSG_RECOVERY_PCBA)
		env_update("bootargs", "androidboot.selinux=permissive");
}

static void bootargs_add_bootdev(void)
{
	char *boot_media, *devtype;
	ulong devnum;
	char boot_options[128];

	devtype = env_get("devtype");
	devnum = env_get_ulong("devnum", 10, -1);
	if (!devtype || devnum < 0)
		return;

	if (!strcmp(devtype, "mmc"))
		boot_media = devnum == 1 ? "sd" : "emmc";
	else if (!strcmp(devtype, "rknand"))
		boot_media = "nand";
	else if (!strcmp(devtype, "spinand"))
		boot_media = "nand"; /* kernel treat sfc nand as nand device */
	else if (!strcmp(devtype, "spinor"))
		boot_media = "nor";
	else if (!strcmp(devtype, "ramdisk"))
		boot_media = "ramdisk";
	else if (!strcmp(devtype, "mtd"))
		boot_media = "mtd";
	else if (!strcmp(devtype, "scsi"))
		boot_media = "scsi";
	else if (!strcmp(devtype, "nvme"))
		boot_media = "nvme";
	else
		return;

	/*
	 * 1. "storagemedia": This is a legacy variable to indicate board
	 *    storage media for kernel and android.
	 *
	 * 2. "androidboot.storagemedia": The same purpose as "storagemedia",
	 *    but the android framework will auto create property by
	 *    variable with format "androidboot.xxx", eg:
	 *
	 *    "androidboot.storagemedia" => "ro.boot.storagemedia".
	 *
	 *    So, U-Boot pass this new variable is only for the convenience
	 *    to Android.
	 */
	if (env_exist("bootargs", "androidboot.mode=charger"))
		snprintf(boot_options, sizeof(boot_options),
			 "storagemedia=%s androidboot.storagemedia=%s",
			 boot_media, boot_media);
	else
		snprintf(boot_options, sizeof(boot_options),
			 "storagemedia=%s androidboot.storagemedia=%s "
			 "androidboot.mode=normal ", boot_media, boot_media);

	env_update("bootargs", boot_options);
}

/*
 * Pass fwver when any available.
 */
static void bootargs_add_fwver(bool verbose)
{
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
	struct tag *t;
	char *list1 = NULL;
	char *list2 = NULL;
	char *fwver = NULL;
	char *p = PLAIN_VERSION;
	int i, end;

	t = atags_get_tag(ATAG_FWVER);
	if (t) {
		list1 = calloc(1, sizeof(struct tag_fwver));
		if (!list1)
			return;
		for (i = 0; i < FW_MAX; i++) {
			if (t->u.fwver.ver[i][0] != '\0') {
				strcat(list1, t->u.fwver.ver[i]);
				strcat(list1, ",");
			}
		}
	}

	list2 = calloc(1, FWVER_LEN);
	if (!list2)
		goto out;
	strcat(list2, "uboot-");
	/* optional */
#ifdef BUILD_TAG
	strcat(list2, BUILD_TAG);
	strcat(list2, "-");
#endif
	/* optional */
	if (strcmp(PLAIN_VERSION, "v5")) {
		strncat(list2, p + strlen("v5-g"), 10);
		strcat(list2, "-");
	}
	strcat(list2, U_BOOT_DMI_DATE);

	/* merge ! */
	if (list1 || list2) {
		fwver = calloc(1, sizeof(struct tag_fwver));
		if (!fwver)
			goto out;

		strcat(fwver, "androidboot.fwver=");
		if (list1)
			strcat(fwver, list1);
		if (list2) {
			strcat(fwver, list2);
		} else {
			end = strlen(fwver) - 1;
			fwver[end] = '\0'; /* omit last ',' */
		}
		if (verbose)
			printf("## fwver: %s\n\n", fwver);
		env_update("bootargs", fwver);
		env_set("fwver", fwver + strlen("androidboot."));
	}
out:
	if (list1)
		free(list1);
	if (list2)
		free(list2);
	if (fwver)
		free(fwver);
#endif
}

static void bootargs_add_android(bool verbose)
{
#ifdef CONFIG_ANDROID_AB
	ab_update_root_partition();
#endif

	/* Android header v4+ need this handle */
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	struct andr_img_hdr *hdr;
	char *fwver;

	hdr = (void *)env_get_ulong("android_addr_r", 16, 0);
	if (hdr && !android_image_check_header(hdr) && hdr->header_version >= 4) {
		if (env_update_extract_subset("bootargs", "andr_bootargs", "androidboot."))
			printf("extract androidboot.xxx error\n");
		if (verbose)
			printf("## bootargs(android): %s\n\n", env_get("andr_bootargs"));

		/* for kernel cmdline can be read */
		fwver = env_get("fwver");
		if (fwver) {
			env_update("bootargs", fwver);
			env_set("fwver", NULL);
		}
	}
#endif
}

static void bootargs_add_partition(bool verbose)
{
#if defined(CONFIG_ENVF) || defined(CONFIG_ENV_PARTITION)
	char *part_type[] = { "mtdparts", "blkdevparts" };
	char *part_list;
	char *env;
	int id = 0;

	env = env_get(part_type[id]);
	if (!env)
		env = env_get(part_type[++id]);
	if (env) {
		if (!strstr(env, part_type[id])) {
			part_list = calloc(1, strlen(env) + strlen(part_type[id]) + 2);
			if (part_list) {
				strcat(part_list, part_type[id]);
				strcat(part_list, "=");
				strcat(part_list, env);
			}
		} else {
			part_list = env;
		}
		env_update("bootargs", part_list);
		if (verbose)
			printf("## parts: %s\n\n", part_list);
	}

	env = env_get("sys_bootargs");
	if (env) {
		env_update("bootargs", env);
		if (verbose)
			printf("## sys_bootargs: %s\n\n", env);
	}
#endif

#ifdef CONFIG_MTD_BLK
	if (!env_get("mtdparts")) {
		char *mtd_par_info = mtd_part_parse(NULL);

		if (mtd_par_info) {
			if (memcmp(env_get("devtype"), "mtd", 3) == 0)
				env_update("bootargs", mtd_par_info);
		}
	}
#endif
}

static void bootargs_add_dtb_dtbo(void *fdt, bool verbose)
{
	/* bootargs_ext is used when dtbo is applied. */
	const char *arr_bootargs[] = { "bootargs", "bootargs_ext" };
	const char *bootargs;
	char *msg = "kernel";
	int i, noffset;

	/* find or create "/chosen" node. */
	noffset = fdt_find_or_add_subnode(fdt, 0, "chosen");
	if (noffset < 0)
		return;

	for (i = 0; i < ARRAY_SIZE(arr_bootargs); i++) {
		bootargs = fdt_getprop(fdt, noffset, arr_bootargs[i], NULL);
		if (!bootargs)
			continue;
		if (verbose)
			printf("## bootargs(%s-%s): %s\n\n",
			       msg, arr_bootargs[i], bootargs);
		/*
		 * Append kernel bootargs
		 * If use AB system, delete default "root=" which route
		 * to rootfs. Then the ab bootctl will choose the
		 * high priority system to boot and add its UUID
		 * to cmdline. The format is "roo=PARTUUID=xxxx...".
		 */
#ifdef CONFIG_ANDROID_AB
		env_update_filter("bootargs", bootargs, "root=");
#else
		env_update("bootargs", bootargs);
#endif
	}
}

const char *board_fdt_chosen_bootargs(void *fdt)
{
	int verbose = is_hotkey(HK_CMDLINE);
	const char *bootargs;

	/* debug */
	hotkey_run(HK_INITCALL);
	if (verbose)
		printf("## bootargs(u-boot): %s\n\n", env_get("bootargs"));

	bootargs_add_dtb_dtbo(fdt, verbose);
	bootargs_add_partition(verbose);
	bootargs_add_fwver(verbose);
	bootargs_add_android(verbose);

	/*
	 * Initrd fixup: remove unused "initrd=0x...,0x...",
	 * this for compatible with legacy parameter.txt
	 */
	env_delete("bootargs", "initrd=", 0);

	/*
	 * If uart is required to be disabled during
	 * power on, it would be not initialized by
	 * any pre-loader and U-Boot.
	 *
	 * If we don't remove earlycon from commandline,
	 * kernel hangs while using earlycon to putc/getc
	 * which may dead loop for waiting uart status.
	 * (It seems the root cause is baundrate is not
	 * initilalized)
	 *
	 * So let's remove earlycon from commandline.
	 */
	if (gd->flags & GD_FLG_DISABLE_CONSOLE)
		env_delete("bootargs", "earlycon=", 0);

	bootargs = env_get("bootargs");
	if (verbose)
		printf("## bootargs(merged): %s\n\n", bootargs);

	return (char *)bootargs;
}

int ft_verify_fdt(void *fdt)
{
	/* for android header v4+, we load bootparams and fixup initrd */
#if defined(CONFIG_ANDROID_BOOT_IMAGE) && defined(CONFIG_XBC)
	struct andr_img_hdr *hdr;
	uint64_t initrd_start, initrd_end;
	char *bootargs, *p;
	int nodeoffset;
	int is_u64, err;
	u32 len;

	hdr = (void *)env_get_ulong("android_addr_r", 16, 0);
	if (!hdr || android_image_check_header(hdr) ||
	    hdr->header_version < 4)
		return 1;

	bootargs = env_get("andr_bootargs");
	if (!bootargs)
		return 1;

	/* trans character: space to new line */
	p = bootargs;
	while (*p++) {
		if (*p == ' ')
			*p = '\n';
	}

	debug("## andr_bootargs: %s\n", bootargs);

	/*
	 * add boot params right after bootconfig
	 *
	 * because we can get final full bootargs in board_fdt_chosen_bootargs(),
	 * android_image_get_ramdisk() is early than that.
	 *
	 * we have to add boot params by now.
	 */
	len = addBootConfigParameters((char *)bootargs, strlen(bootargs),
		(u64)hdr->ramdisk_addr + hdr->ramdisk_size +
		hdr->vendor_ramdisk_size, hdr->vendor_bootconfig_size);
	if (len < 0) {
		printf("error: addBootConfigParameters\n");
		return 0;
	}

	nodeoffset = fdt_subnode_offset(fdt, 0, "chosen");
	if (nodeoffset < 0) {
		printf("error: No /chosen node\n");
		return 0;
	}

	/* fixup initrd with real value */
	fdt_delprop(fdt, nodeoffset, "linux,initrd-start");
	fdt_delprop(fdt, nodeoffset, "linux,initrd-end");

	is_u64 = (fdt_address_cells(fdt, 0) == 2);
	initrd_start = hdr->ramdisk_addr;
	initrd_end = initrd_start + hdr->ramdisk_size +
			hdr->vendor_ramdisk_size +
			hdr->vendor_bootconfig_size + len;
	err = fdt_setprop_uxx(fdt, nodeoffset, "linux,initrd-start",
			      initrd_start, is_u64);
	if (err < 0) {
		printf("WARNING: could not set linux,initrd-start %s.\n",
		       fdt_strerror(err));
		return 0;
	}
	err = fdt_setprop_uxx(fdt, nodeoffset, "linux,initrd-end",
			      initrd_end, is_u64);
	if (err < 0) {
		printf("WARNING: could not set linux,initrd-end %s.\n",
		       fdt_strerror(err));
		return 0;
	}
#endif
	return 1;
}

void bootargs_setup(void)
{
	bootargs_add_bootdev();
	bootargs_add_fuse();
	bootargs_add_misc();
}

