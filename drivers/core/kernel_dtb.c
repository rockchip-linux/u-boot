// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd.
 */

#include <common.h>
#include <fs.h>
#include <env.h>
#include <hotkey.h>
#include <sysmem.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <dm/root.h>
#include <of_live.h>
#include <memalign.h>

#ifdef CONFIG_SYSMEM
/**
 * boot_fdt_add_mem_rsv_regions - Mark the memreserve sections as unusable
 * @sysmem: pointer to sysmem handle, will be used for memory mgmt
 * @fdt_blob: pointer to fdt blob base address
 */
static int boot_fdt_add_sysmem_rsv_regions(void *fdt_blob)
{
	uint64_t addr, size;
	int i, total;
	int rsv_offset, offset;
	fdt_size_t rsv_size;
	fdt_addr_t rsv_addr;
	static int rsv_done;
	char resvname[32];
	const void *prop;

	if (fdt_check_header(fdt_blob) != 0 || rsv_done)
		return -EINVAL;

	rsv_done = 1;

	total = fdt_num_mem_rsv(fdt_blob);
	for (i = 0; i < total; i++) {
		if (fdt_get_mem_rsv(fdt_blob, i, &addr, &size) != 0)
			continue;
		debug("   sysmem: reserving fdt memory region: addr=%llx size=%llx\n",
		      (unsigned long long)addr, (unsigned long long)size);
		sprintf(resvname, "fdt-memory-reserved%d", i);
		if (!sysmem_fdt_reserve_alloc_base(resvname, addr, size))
			return -ENOMEM;
	}

	rsv_offset = fdt_subnode_offset(fdt_blob, 0, "reserved-memory");
	if (rsv_offset == -FDT_ERR_NOTFOUND)
		return -EINVAL;

	for (offset = fdt_first_subnode(fdt_blob, rsv_offset);
	     offset >= 0;
	     offset = fdt_next_subnode(fdt_blob, offset)) {
		prop = fdt_getprop(fdt_blob, offset, "status", NULL);
		if (prop && !strcmp(prop, "disabled"))
			continue;

		rsv_addr = fdtdec_get_addr_size_auto_noparent(fdt_blob, offset,
							      "reg", 0,
							      &rsv_size, false);
		/*
		 * kernel will alloc reserved memory dynamically for the node
		 * with start address from 0.
		 */
		if (rsv_addr == FDT_ADDR_T_NONE || !rsv_addr || !rsv_size)
			continue;
		debug("  sysmem: 'reserved-memory' %s: addr=%llx size=%llx\n",
		      fdt_get_name(fdt_blob, offset, NULL),
		      (unsigned long long)rsv_addr, (unsigned long long)rsv_size);
		if (!sysmem_fdt_reserve_alloc_base(fdt_get_name(fdt_blob, offset, NULL),
					           rsv_addr, rsv_size))
			return -ENOMEM;
	}

	return 0;
}
#endif

static int kernel_dtb_build(const void *blob)
{
	if (!blob || fdt_check_header(blob))
		return -EINVAL;

	gd->fdt_blob = blob;
	gd->of_root_f = gd->of_root;
	gd->flags |= GD_FLG_KDTB_READY;

	of_live_build(gd->fdt_blob, (struct device_node **)gd_of_root_ptr());
	dm_scan_fdt(false);

	return boot_fdt_add_sysmem_rsv_regions((void *)gd->fdt_blob);
}

struct device_node *kernel_dtb_lookup_phandle(phandle handle)
{
	struct device_node *np;

	for (np = gd->of_root_f; np; np = of_find_all_nodes(np)) {
		if (np->phandle == handle)
			return np;
	}

	return NULL;
}

static struct list_head *kernel_dtb_find_insert_head(struct uclass *uc)
{
	struct udevice *dev;

	uclass_foreach_dev(dev, uc) {
		if (!(dev_get_flags(dev) & DM_FLAG_KNRL_DTB))
			return &dev->uclass_node;
	}

	return &uc->dev_head;
}

static bool kernel_dtb_head_is_valid(struct uclass *uc, struct list_head *head)
{
	struct udevice *dev;

	if (!head)
		return false;
	if (head == &uc->dev_head)
		return true;

	uclass_foreach_dev(dev, uc) {
		if (&dev->uclass_node == head)
			return true;
	}

	return false;
}

static struct list_head *kernel_dtb_get_insert_head(struct uclass *uc)
{
	if (!kernel_dtb_head_is_valid(uc, uc->u_boot_dev_head))
		uc->u_boot_dev_head = kernel_dtb_find_insert_head(uc);

	return uc->u_boot_dev_head;
}

static int bind_after_uboot_dev(const char *drv_name,
				 const char *uc_drv_name[], int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (!strcmp(drv_name, uc_drv_name[i]))
			return 0;
	}

	return 1;
}

void kernel_dtb_device_bind(struct uclass *uc, struct udevice *dev,
			    const struct driver *drv, int *after_u_boot_dev)
{
	/*
	 * Put these U-Boot devices ahead of kernel dtb devices, in order to
	 * be early got in uclass_get_device_xxx().
	 *
	 * U: u-boot dev:
	 * K: kernel dev:
	 *	device-list order: U01234...K01234...
	 *	device-list order: K01234...U01234...
	 */
	u32 i, prior_u_boot_uclass_id[] = {
		UCLASS_AHCI,		/* boot devices */
		UCLASS_BLK,
		UCLASS_MMC,
		UCLASS_MTD,
		UCLASS_PCI,
		UCLASS_RKNAND,
		UCLASS_SPI_FLASH,
		UCLASS_UFS,

		UCLASS_BOOTDEV,
		UCLASS_BOOTMETH,
		UCLASS_BOOTSTD,

		UCLASS_ADC,		/* ADC for Button */
		UCLASS_BUTTON,		/* Button */
		UCLASS_FIRMWARE,	/* psci sysreset */
		UCLASS_MISC,		/* RSA/Crypto... security; otp/efuse */
		UCLASS_RNG,		/* ramdom number */
		UCLASS_SYSCON,		/* grf, pmugrf */
		UCLASS_SYSRESET,	/* psci sysreset */
		UCLASS_TPM,		/* Security */
		UCLASS_TEE,		/* optee */
		UCLASS_WDT,		/* reliable sysreset */
	};
	const char *misc_drv_before_uboot[] = {
		"rockchip_otp",
		"rockchip_efuse",
	};

	if (gd->flags & GD_FLG_KDTB_READY) {
		*after_u_boot_dev = 0;
		dev_or_flags(dev, DM_FLAG_KNRL_DTB);
		debug("### binding : %s\n", dev->name);

		for (i = 0; i < ARRAY_SIZE(prior_u_boot_uclass_id); i++) {
			if (drv->id == prior_u_boot_uclass_id[i]) {
				*after_u_boot_dev =
					bind_after_uboot_dev(drv->name,
						misc_drv_before_uboot,
						ARRAY_SIZE(misc_drv_before_uboot));
				break;
			}
		}
		if (!kernel_dtb_head_is_valid(uc, uc->u_boot_dev_head))
			uc->u_boot_dev_head = kernel_dtb_find_insert_head(uc);
	} else if (!uc->u_boot_dev_head) {
		uc->u_boot_dev_head = &dev->uclass_node;
	}
}

void kernel_dtb_list_add(struct uclass *uc, struct udevice *dev,
			 int after_u_boot_dev)
{
	if (after_u_boot_dev) {
		list_add_tail(&dev->uclass_node, &uc->dev_head);
		debug("### after_u : %s\n", dev->name);
	} else {
		struct list_head *head = kernel_dtb_get_insert_head(uc);

		list_add_tail(&dev->uclass_node, head);
		debug("### before_u: %s\n", dev->name);
	}
}

int kernel_dtb_init(void)
{
	void *fdt_addr = NULL;
	int ret = -ENODEV;

	if (gd->ram_size <= SZ_128M)
		fdt_addr = (void *)env_get_ulong("fdt_addr1_r", 16, 0);
	if (!fdt_addr)
		fdt_addr = (void *)env_get_ulong("fdt_addr_r", 16, 0);
	if (!fdt_addr)
		return -EINVAL;

#ifdef CONFIG_EMBED_KERNEL_DTB_ALWAYS
	printf("Always embed kernel dtb\n");
	goto dtb_embed;
#endif
	ret = board_kernel_dtb_read(fdt_addr);
	if (!ret)
		goto dtb_okay;

#ifdef CONFIG_EMBED_KERNEL_DTB
#ifdef CONFIG_EMBED_KERNEL_DTB_ALWAYS
dtb_embed:
#endif
	if (gd->fdt_blob_kern) {
		fdt_addr = memalign(ARCH_DMA_MINALIGN, fdt_totalsize(gd->fdt_blob_kern));
		if (!fdt_addr)
			return -ENOMEM;

		/*
		 * Alloc another space for this embed kernel dtb.
		 * Because "fdt_addr_r" *MUST* be the fdt passed to kernel.
		 */
		memcpy(fdt_addr, gd->fdt_blob_kern,
		       fdt_totalsize(gd->fdt_blob_kern));
		printf("DTB: %s\n", CONFIG_EMBED_KERNEL_DTB_PATH);
	} else
#endif
	{
		printf("Failed to get kernel dtb, ret=%d\n", ret);
		return -ENOENT;
	}

dtb_okay:
	gd->fdt_blob = fdt_addr;
	hotkey_run(HK_FDT);

	return kernel_dtb_build(gd->fdt_blob);
}

/* board specific implement */
__weak int board_kernel_dtb_read(void *fdt)
{
	return -ENOSYS;
}

