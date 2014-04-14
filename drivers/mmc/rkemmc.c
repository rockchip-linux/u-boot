/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#include <config.h>
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <linux/list.h>
#include <div64.h>
#include "../../board/rockchip/common/common/typedef.h"
#include "../../board/rockchip/common/common/emmc/sdmmcBoot.h"
/* Set block count limit because of 16 bit register limit on some hardware*/
#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
#define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif

static struct list_head mmc_devices;
static int cur_dev_num = -1;

int __weak board_mmc_getwp(struct mmc *mmc)
{
	return -1;
}

int mmc_getwp(struct mmc *mmc)
{
	int wp;

	wp = board_mmc_getwp(mmc);

	if (wp < 0) {
		if (mmc->getwp)
			wp = mmc->getwp(mmc);
		else
			wp = 0;
	}

	return wp;
}

int __board_mmc_getcd(struct mmc *mmc) {
	return -1;
}

int board_mmc_getcd(struct mmc *mmc)__attribute__((weak,
	alias("__board_mmc_getcd")));

struct mmc *find_mmc_device(int dev_num)
{
	struct mmc *m;
	struct list_head *entry;
	printf("RKEMMC find_mmc_device,dev = %d\n", dev_num);

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		if (m->block_dev.dev == dev_num)
			return m;
	}

	printf("MMC Device %d not found\n", dev_num);

	return NULL;
}

static unsigned long
mmc_berase(int dev_num, unsigned long start, lbaint_t blkcnt)
{
	printf("RKEMMC mmc_berase\n");
	return 0;
}

static ulong
mmc_bwrite(int dev_num, ulong start, lbaint_t blkcnt, const void*src)
{
	lbaint_t cur, blocks_todo = blkcnt;
	printf("RKEMMC mmc_bwrite:start = %x, blkcnt = %x\n, src = %x", start, blkcnt, src);
	
	return StorageWriteLba(start, src, blkcnt, 0);
}

static ulong mmc_bread(int dev_num, ulong start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;
	
	printf("RKEMMC mmc_bread:start = %x, blkcnt = %x\n, dst = %x", start, blkcnt, dst);
	return StorageReadLba(start, dst, blkcnt);
}

int mmc_register(struct mmc *mmc)
{
	/* Setup the universal parts of the block interface just once */
	mmc->block_dev.if_type = IF_TYPE_MMC;
	mmc->block_dev.dev = cur_dev_num++;
	mmc->block_dev.removable = 1;
	mmc->block_dev.block_read = mmc_bread;
	mmc->block_dev.block_write = mmc_bwrite;
	mmc->block_dev.block_erase = mmc_berase;
	if (!mmc->b_max)
		mmc->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	INIT_LIST_HEAD (&mmc->link);

	list_add_tail (&mmc->link, &mmc_devices);

	return 0;
}

#ifdef CONFIG_PARTITIONS
block_dev_desc_t *mmc_get_dev(int dev)
{
	struct mmc *mmc = find_mmc_device(dev);
	printf("RKEMMC mmc_get_dev:dev = %d\n", dev);
	if (!mmc || mmc_init(mmc))
		return NULL;

	return &mmc->block_dev;
}
#endif

int mmc_init(struct mmc *mmc)
{
	int err = 0;
	SdmmcInit(2);
	return err;
}

/*
 * CPU and board-specific MMC initializations.  Aliased function
 * signals caller to move on
 */
static int __def_mmc_init(bd_t *bis)
{
	return -1;
}

int cpu_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));
int board_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));

void print_mmc_devices(char separator)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		printf("%s: %d", m->name, m->block_dev.dev);

		if (entry->next != &mmc_devices)
			printf("%c ", separator);
	}

	printf("\n");
}

int get_mmc_num(void)
{
	return cur_dev_num;
}

int mmc_initialize(bd_t *bis)
{
	printf("emmc_initialize\n");
	INIT_LIST_HEAD (&mmc_devices);
	cur_dev_num = 0;
	struct mmc *mmc = malloc(sizeof(struct mmc));

	if (!mmc)
		return -1;
	strcpy(mmc->name, "rkemmc");
	mmc_register(mmc);
	print_mmc_devices(',');
	 /* set up exceptions */
	interrupt_init();
	/* enable exceptions */
	enable_interrupts();
	//SdmmcInit(2);
	if( StorageInit() == 0)
		printf("emmc init OK!\n");
	else
		printf("Fail!\n");
	return 0;
}
