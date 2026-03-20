// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd.
 */
#include <common.h>
#include <command.h>
#include <env.h>
#include <init.h>
#include <malloc.h>
#include <mmc.h>
#include <mtd.h>
#include <nvme.h>
#include <scsi.h>
#include <sysmem.h>
#include <asm/cache.h>
#include <dm/device.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/common.h>
#include <asm/arch-rockchip/param.h>
#include <asm/arch-rockchip/resource.h>

#ifdef CONFIG_MTD_BLK
#include "mtd_blk.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

/* Don't use env_xxx() for boot device init */
static struct blk_desc *g_bootdev;
static char *g_devnum, *g_devtype;
static int g_idevnum;

void env_import_board(void)
{
	env_set("devtype", g_devtype);
	env_set("devnum", g_devnum);
}

__weak int rk_board_scan_bootdev(char **devtype, char **devnum)
{
	*devtype = "mmc";
	*devnum = "0";
	printf("No bootdev scan list, try eMMC!\n");

	return 0;
}

static int scan_bootdev(char **devtype, char **devnum)
{
#ifdef CONFIG_MMC
	mmc_initialize(gd->bd);
#endif
	return rk_board_scan_bootdev(devtype, devnum);
}

static int bootdev_do_probe(const char *devtype, const char *devnum)
{
	int devnum_ul = simple_strtoul(devnum, NULL, 10);

#ifdef CONFIG_MMC
	if (!strcmp("mmc", devtype))
		mmc_initialize(gd->bd);
#endif
#ifdef CONFIG_NVME
	if (!strcmp("nvme", devtype)) {
		pci_init();
		if (nvme_scan_namespace())
			return -ENODEV;
	}
#endif
#if defined(CONFIG_SCSI) && defined(CONFIG_CMD_SCSI) && (defined(CONFIG_AHCI) || defined(CONFIG_UFS))
	if (!strcmp("scsi", devtype)) {
		if (scsi_scan(true))
			return -ENODEV;
	}
#endif
#ifdef CONFIG_MTD
	if (!strcmp("mtd", devtype)) {
		if (mtd_probe_devices())
			return -ENODEV;
	}
#endif

	/* Ok, let's test whether we can get the expected boot device or not */
	if (!blk_get_devnum_by_uclass_idname(devtype, devnum_ul))
		return -ENODEV;

	return 0;
}

/*
 * Priority: configuration > atags.
 */
static int bootdev_probe(void)
{
	char *devtype, *devnum;
	char *src = "scan";
	int ret;

	/* configuration */
#ifdef CONFIG_ROCKCHIP_BOOTDEV
	if (!param_parse_assign_bootdev(&devtype, &devnum)) {
		if (!bootdev_do_probe(devtype, devnum)) {
			src = "assign";
			goto finish;
		}
	}
#endif
	/* atags */
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
	if (!param_parse_atags_bootdev(&devtype, &devnum)) {
		if (!bootdev_do_probe(devtype, devnum)) {
			src = "atags";
			goto finish;
		}
	}
#endif
	/* scan list */
	ret = scan_bootdev(&devtype, &devnum);
	if (ret) {
		printf("No available boot device\n");
		return ret;
	}

finish:
	g_devtype = (char *)devtype;
	g_devnum = (char *)devnum;
	g_idevnum = simple_strtoul(devnum, NULL, 10);

	printf("Bootdev(%s): %s %s\n", src, g_devtype, g_devnum);

	return 0;
}

static void show_bootdev_info(int devtype, int devnum)
{
#ifdef CONFIG_MMC
	if (devtype == UCLASS_MMC) {
		struct mmc *mmc = find_mmc_device(devnum);

		if (mmc)
			printf("MMC%d: %s\n", devnum, mmc_mode_name(mmc->selected_mode));
	}
#endif

	printf("PartType: %s\n", part_get_name(g_bootdev));
}

struct blk_desc *plat_bootdev(void)
{
	ulong devnum;
	char *devtype;
	int uclass;

	if (g_bootdev)
		return g_bootdev;

	if (bootdev_probe())
		return NULL;

	/* devtype and devnum are available after bootdev_probe() */
	devtype = g_devtype;
	devnum = g_idevnum;
	if (!devtype || devnum < 0)
		return NULL;

	if (!strcmp(devtype, "mmc"))
		uclass = UCLASS_MMC;
	else if (!strcmp(devtype, "rknand"))
		uclass = UCLASS_RKNAND;
	else if (!strcmp(devtype, "spinand"))
		uclass = UCLASS_SPINAND;
	else if (!strcmp(devtype, "spinor"))
		uclass = UCLASS_SPINOR;
	else if (!strcmp(devtype, "ramdisk"))
		uclass = UCLASS_RAMDISK;
	else if (!strcmp(devtype, "mtd"))
		uclass = UCLASS_MTD;
	else if (!strcmp(devtype, "scsi"))
		uclass = UCLASS_SCSI;
	else if (!strcmp(devtype, "nvme"))
		uclass = UCLASS_NVME;
	else
		return NULL;

	g_bootdev = blk_get_devnum_by_uclass_id(uclass, devnum);
	if (!g_bootdev) {
		printf("No plat g_bootdev found!\n");
		return NULL;
	}

	show_bootdev_info(uclass, devnum);

#ifdef CONFIG_MTD_BLK
	mtd_blk_map_partitions(g_bootdev);
#endif
	return g_bootdev;
}

void plat_set_bootdev(struct blk_desc *desc)
{
	g_bootdev = desc;
}

#ifdef CONFIG_ROCKCHIP_USB_BOOT
int usb_boot_init(void)
{
	struct blk_desc *desc;
	struct udevice *dev;
	void *fdt_addr;
	u32 fdt_size;
	int devnum = -1;
	char buf[32];

	/* Booting priority: mmc1 > udisk */
	if (!strcmp(env_get("devtype"), "mmc") && !strcmp(env_get("devnum"), "1"))
		return 0;

	if (!run_command("usb start", -1)) {
		for (blk_first_device(UCLASS_USB, &dev);
		     dev;
		     blk_next_device(&dev)) {
			desc = dev_get_uclass_plat(dev);
			printf("Scanning usb %d ...\n", desc->devnum);
			if (desc->type == DEV_TYPE_UNKNOWN)
				continue;

			if (desc->lba > 0L && desc->blksz > 0L) {
				devnum = desc->devnum;
				break;
			}
		}
		if (devnum < 0) {
			printf("No usb mass storage found\n");
			return -ENODEV;
		}

		desc = blk_get_devnum_by_uclass_id(UCLASS_USB, devnum);
		if (!desc) {
			printf("No usb %d found\n", devnum);
			return -ENODEV;
		}

		snprintf(buf, 32, "rkimgtest usb %d", devnum);
		if (!run_command(buf, -1)) {
			snprintf(buf, 32, "%d", devnum);
			plat_set_bootdev(desc);
			env_set("devtype", "usb");
			env_set("devnum", buf);
			printf("=== Booting from usb %d ===\n", devnum);
			if (!gd->fdt_blob_kern) {
				fdt_size = fdt_totalsize(gd->fdt_blob);
				fdt_addr = memalign(ARCH_DMA_MINALIGN, fdt_size);
				if (!fdt_addr)
					return -ENOMEM;
				memcpy(fdt_addr, gd->fdt_blob, fdt_size);
				fdt_set_magic((void *)gd->fdt_blob, ~0);
				sysmem_free((phys_addr_t)gd->fdt_blob);
#ifdef CONFIG_ROCKCHIP_RESOURCE_IMAGE
				resource_destroy();
#endif
				gd->fdt_blob_kern = fdt_addr;
				gd->fdt_blob = fdt_addr;
			}
		} else {
			printf("No available udisk image on usb %d\n", devnum);
			return -ENODEV;
		}
	}

	return 0;
}
#endif

