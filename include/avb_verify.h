/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2018, Linaro Limited
 */

#ifndef	_AVB_VERIFY_H
#define _AVB_VERIFY_H

#include <../lib/libavb/libavb.h>
#include <../lib/libavb_ab/libavb_ab.h>
#include <../lib/libavb_atx/libavb_atx.h>
#include <../lib/libavb_user/libavb_user.h>
#include <../lib/libavb_user/rockchip/avb.h>
#include <../lib/libavb_user/rockchip/ab.h>
#include <mapmem.h>
#include <mmc.h>

#define AVB_MAX_ARGS			1024
#define VERITY_TABLE_OPT_RESTART	"restart_on_corruption"
#define VERITY_TABLE_OPT_LOGGING	"ignore_corruption"
#define ALLOWED_BUF_ALIGN		8

enum avb_boot_state {
	AVB_GREEN,
	AVB_YELLOW,
	AVB_ORANGE,
	AVB_RED,
};

struct preloaded_partition {
	uint8_t *addr;
	size_t size; // 0: the partition hasn't yet been preloaded
};

struct AvbOpsData {
	struct AvbOps ops;
	int mmc_dev;
	enum avb_boot_state boot_state;
#ifdef CONFIG_OPTEE_TA_AVB
	struct udevice *tee;
	u32 session;
#endif
	const char *iface;
	const char *devnum;
	const char *slot_suffix;
	struct preloaded_partition boot;
	struct preloaded_partition vendor_boot;
	struct preloaded_partition init_boot;
	struct preloaded_partition resource;
};

struct mmc_part {
	int dev_num;
	struct mmc *mmc;
	struct blk_desc *mmc_blk;
	struct disk_partition info;
};

enum mmc_io_type {
	IO_READ,
	IO_WRITE
};

AvbOps *avb_ops_alloc(int boot_device);
void avb_ops_free(AvbOps *ops);

char *avb_set_state(AvbOps *ops, enum avb_boot_state boot_state);
char *avb_set_enforce_verity(const char *cmdline);
char *avb_set_ignore_corruption(const char *cmdline);

char *append_cmd_line(char *cmdline_orig, char *cmdline_new);
const char *str_avb_io_error(AvbIOResult res);
const char *str_avb_slot_error(AvbSlotVerifyResult res);
/**
 * ============================================================================
 * I/O helper inline functions
 * ============================================================================
 */
static inline uint64_t calc_offset(struct mmc_part *part, int64_t offset)
{
	u64 part_size = part->info.size * part->info.blksz;

	if (offset < 0)
		return part_size + offset;

	return offset;
}

static inline size_t get_sector_buf_size(void)
{
#ifdef CONFIG_AVB_BUF_SIZE
	return (size_t)CONFIG_AVB_BUF_SIZE;
#else
	return 0;
#endif
}

static inline void *get_sector_buf(void)
{
#ifdef CONFIG_AVB_BUF_SIZE
	return map_sysmem(CONFIG_AVB_BUF_ADDR, CONFIG_AVB_BUF_SIZE);
#else
	return NULL;
#endif
}

static inline bool is_buf_unaligned(void *buffer)
{
	return (bool)((uintptr_t)buffer % ALLOWED_BUF_ALIGN);
}

static inline int get_boot_device(AvbOps *ops)
{
	struct AvbOpsData *data;

	if (ops) {
		data = ops->user_data;
		if (data)
			return data->mmc_dev;
	}

	return -1;
}

#endif /* _AVB_VERIFY_H */
