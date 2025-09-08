/*
 * (C) Copyright 2020-2023 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <debug_uart.h>
#include <malloc.h>
#include <mmc.h>
#include <scsi.h>
#include <stdlib.h>
#include <asm/global_data.h>
#include <dm/device.h>
#include <dm/uclass-id.h>

DECLARE_GLOBAL_DATA_PTR;

struct bootdev_list {
	enum uclass_id uclass;
	u8 devnum;
	u8 iomux_routing;
};

#if CONFIG_IS_ENABLED(ROCKCHIP_RK3588)
static const struct bootdev_list dev_list[] = {
	{UCLASS_MMC, 0, 0},
	{UCLASS_MTD, 0, 0}, /* BLK_MTD_NAND */
	{UCLASS_MTD, 1, 0}, /* BLK_MTD_SPI_NAND FSPI M0 */
	{UCLASS_MTD, 1, 1}, /* BLK_MTD_SPI_NAND FSPI M1 */
	{UCLASS_MTD, 1, 2}, /* BLK_MTD_SPI_NAND FSPI M2 */
	{UCLASS_MTD, 2, 0}, /* BLK_MTD_SPI_NOR FSPI M0 */
	{UCLASS_MTD, 2, 1}, /* BLK_MTD_SPI_NOR FSPI M1 */
	{UCLASS_MTD, 2, 2}, /* BLK_MTD_SPI_NOR FSPI M2 */
};
#elif CONFIG_IS_ENABLED(ROCKCHIP_RK3576)
static const struct bootdev_list dev_list[] = {
	{UCLASS_SCSI, 0, 0},
	{UCLASS_MMC, 0, 0},
	{UCLASS_MTD, 1, 0}, /* BLK_MTD_SPI_NAND FSPI0 M0 */
	{UCLASS_MTD, 1, 1}, /* BLK_MTD_SPI_NAND FSPI1 M0 */
	{UCLASS_MTD, 1, 2}, /* BLK_MTD_SPI_NAND FSPI1 M1 */
	{UCLASS_MTD, 2, 0}, /* BLK_MTD_SPI_NOR FSPI0 M0 */
	{UCLASS_MTD, 2, 1}, /* BLK_MTD_SPI_NOR FSPI1 M0 */
	{UCLASS_MTD, 2, 2}, /* BLK_MTD_SPI_NOR FSPI1 M0 */
};
#else
static const struct bootdev_list dev_list[] = {
	{UCLASS_MMC, 0, 0},
	{UCLASS_MTD, 0, 0}, /* BLK_MTD_NAND */
	{UCLASS_MTD, 1, 0}, /* BLK_MTD_SPI_NAND */
	{UCLASS_MTD, 2, 0}, /* BLK_MTD_SPI_NOR */
	{UCLASS_RKNAND, 0, 0},
};
#endif

static struct blk_desc *boot_blk_desc;
struct blk_desc *plat_bootdev(void)
{
	return boot_blk_desc;
}

__weak void board_set_iomux(enum uclass_id uclass, int devnum, int routing)
{
}

__weak void board_unset_iomux(enum uclass_id uclass, int devnum, int routing)
{
}

struct blk_desc *usbplug_blk_get_devnum_by_uclass_id(enum uclass_id uclass, int devnum)
{
	struct blk_desc *blk_desc = NULL;
	u8 iomux_routing;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(dev_list); i++) {
		if (uclass != dev_list[i].uclass || devnum != dev_list[i].devnum)
			continue;
		iomux_routing = dev_list[i].iomux_routing;
		switch (uclass) {
#ifdef CONFIG_MMC
		case UCLASS_MMC:
			board_set_iomux(uclass, devnum, iomux_routing);
			mmc_initialize(gd->bd);
			break;
#endif
		case UCLASS_MTD:
			board_set_iomux(uclass, devnum, iomux_routing);
			break;
#if defined(CONFIG_SCSI) && defined(CONFIG_CMD_SCSI) && (defined(CONFIG_AHCI) || defined(CONFIG_UFS))
		case UCLASS_SCSI:
			scsi_scan(true);
			break;
#endif
		default:
			printf("Bootdev 0x%x is not support\n", uclass);
			return NULL;
		}

		printf("scandev: %s %d m%d\n",
		       blk_get_uclass_name(uclass), devnum, iomux_routing);
		blk_desc = blk_get_devnum_by_uclass_id(uclass, devnum);
		if (blk_desc)
			break;

		board_unset_iomux(uclass, devnum, iomux_routing);
	}

	boot_blk_desc = blk_desc;

	return blk_desc;
}

static char *bootdev_rockusb_cmd(void)
{
	struct blk_desc *blk_desc = NULL;
	enum uclass_id uclass = UCLASS_INVALID;
	u8 devnum, iomux_routing;
	char *cmd;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(dev_list); i++) {
		uclass = dev_list[i].uclass;
		devnum = dev_list[i].devnum;
		iomux_routing = dev_list[i].iomux_routing;
		switch (uclass) {
#ifdef CONFIG_MMC
		case UCLASS_MMC:
			board_set_iomux(uclass, devnum, iomux_routing);
			mmc_initialize(gd->bd);
			break;
#endif
		case UCLASS_MTD:
			board_set_iomux(uclass, devnum, iomux_routing);
			break;
#if defined(CONFIG_SCSI) && defined(CONFIG_CMD_SCSI) && (defined(CONFIG_AHCI) || defined(CONFIG_UFS))
		case UCLASS_SCSI:
			scsi_scan(true);
			break;
#endif
		default:
			printf("Bootdev 0x%x is not support\n", uclass);
			return NULL;
		}

		printf("Scandev: %s %d m%d\n",
		       blk_get_uclass_name(uclass), devnum, iomux_routing);
		blk_desc = blk_get_devnum_by_uclass_id(uclass, devnum);
		if (blk_desc)
			break;

		board_unset_iomux(uclass, devnum, iomux_routing);
	}

	boot_blk_desc = blk_desc;
	if (!uclass) {
		printf("No boot device\n");
		return NULL;
	}

	printf("Bootdev: %s %d\n", blk_get_uclass_name(uclass), devnum);

	cmd = malloc(32);
	if (!cmd)
		return NULL;

	snprintf(cmd, 32, "rockusb 0 %s %d", blk_get_uclass_name(uclass), devnum);

	return cmd;
}

int board_init(void)
{
	return run_command(bootdev_rockusb_cmd(), 0);
}

