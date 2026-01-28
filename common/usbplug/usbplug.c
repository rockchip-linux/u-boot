/*
 * (C) Copyright 2020 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <cpu_func.h>
#include <env_internal.h>
#include <init.h>
#include <mapmem.h>
#include <malloc.h>
#include <of_live.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <dm/root.h>
#include <dm/uclass-internal.h>
#include <hang.h>

DECLARE_GLOBAL_DATA_PTR;

size_t rockchip_sdram_size(phys_addr_t reg)
{
	return SZ_64M;
}

/*
 * The below functions are all __weak declared.
 */
int dram_init(void)
{
#if CONFIG_SYS_MALLOC_LEN > SZ_64M
	"CONFIG_SYS_MALLOC_LEN is over 64MB"
#endif
	gd->ram_size = rockchip_sdram_size(0); /* default */

	gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = gd->ram_size;

	return 0;
}

#if defined(CONFIG_ARM) || defined(CONFIG_RISCV)
/*
 * Some of these functions are needed purely because the functions they
 * call return void. If we change them to return 0, these stubs can go away.
 */
static int initr_caches(void)
{
	/* Enable caches */
	enable_caches();
	return 0;
}
#endif

static void initr_malloc(void)
{
	ulong start;

	start = gd->relocaddr - TOTAL_MALLOC_LEN;
	gd_set_malloc_start(start);
	mem_malloc_init(start, TOTAL_MALLOC_LEN);
}

#ifdef CONFIG_DM
static void initr_dm(void)
{
	oftree_reset();

	/* Drop the pre-reloc driver model and start a new one */
	gd->dm_root = NULL;

	dm_init_and_scan(false);
}
#endif

static int initr_of_live(void)
{
	if (CONFIG_IS_ENABLED(OF_LIVE)) {
		int ret;

		ret = of_live_build(gd->fdt_blob,
				    (struct device_node **)gd_of_root_ptr());
		if (ret)
			return ret;
	}

	return 0;
}

#if !defined(CONFIG_DM_USB_GADGET)
__weak int rkusb_dev_bind_to_udc_data(struct udevice *dev)
{
	return 0;
}

static int rkusb_dev_bind(struct udevice *dev)
{
	return rkusb_dev_bind_to_udc_data(dev);
}

U_BOOT_DRIVER(rkusb) = {
	.name = "rkusb",
	.id = UCLASS_USB_GADGET_GENERIC,
	.bind = rkusb_dev_bind,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(rkusb) = {
	.name = "rkusb",
};
#endif

/* Refers to common/board_r.c */
void board_init_r(gd_t *new_gd, ulong dest_addr)
{
	gd->flags |= GD_FLG_RELOC | GD_FLG_FULL_MALLOC_INIT;
#ifdef CONFIG_ARM
	initr_caches();
#endif
	initr_malloc();

	initr_of_live();

#ifdef CONFIG_DM
	initr_dm();
#endif
	/* Setup chipselects, entering usb-plug mode */
	board_init();
	hang();
}
