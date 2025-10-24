/*
 * Copyright 2023, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <command.h>
#include <part.h>
#include <stdlib.h>

#ifdef CONFIG_ROCKCHIP_OPTEE_V2
#include "v2/rkss.h"
#endif

#ifdef CONFIG_ROCKCHIP_OPTEE_V3
#include "v3/rkss.h"
#endif

static bool check_is_rkss_version1(struct blk_desc *dev_desc,
				   struct disk_partition part_info)
{
	u8 *read_buff;
	unsigned long ret;
	u32 t1_version, t2_version;
	u32 t1_checkstr, t2_checkstr;

	read_buff = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, 1024);
	if (!read_buff) {
		printf("%s: Malloc buf failed!\n", __func__);
		return false;
	}

	ret = blk_dread(dev_desc, part_info.start, 2, read_buff);
	if (ret != 1) {
		printf("%s: blk_dread failed!\n", __func__);
		free(read_buff);
		return false;
	}

	t1_version = *(u32 *)(read_buff + 512 - 8);
	t1_checkstr = *(u32 *)(read_buff + 512 - 4);
	t2_version = *(u32 *)(read_buff + 1024 - 8);
	t2_checkstr = *(u32 *)(read_buff + 1024 - 4);

	free(read_buff);

	if (t1_version == 1 && t1_checkstr == 0x12345678 &&
	    t2_version == 1 && t2_checkstr == 0x12345678)
		return true;
	else
		return false;
}

static bool check_is_rkss_version2(struct blk_desc *dev_desc,
				   struct disk_partition part_info)
{
	u8 *read_buff;
	unsigned long ret;
	u32 tag;
	u32 version;

	read_buff = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, 4096);
	if (!read_buff) {
		printf("%s: Malloc buf failed!\n", __func__);
		return false;
	}

	ret = blk_dread(dev_desc, part_info.start, 8, read_buff);
	if (ret != 8)
		ret = blk_dread(dev_desc, part_info.start + 512, 8, read_buff);
	if (ret != 8) {
		printf("%s: blk_dread failed!\n", __func__);
		free(read_buff);
		return false;
	}

	tag = *(u32 *)(read_buff);
	version = *(u32 *)(read_buff + 4);

	free(read_buff);

	if (tag == 0x524B5353 && version == 2)
		return true;
	else
		return false;
}

static int get_rkss_version(void)
{
	static int rkss_version = 0;
	struct blk_desc *dev_desc = NULL;
	struct disk_partition part_info;

	if (rkss_version != 0)
		return rkss_version;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device.\n", __func__);
		return -1;
	}

	if (part_get_info_by_name(dev_desc,
				  "security", &part_info) < 0) {
		printf("%s: Could not find security partition.\n", __func__);
		return -1;
	}

	if (check_is_rkss_version1(dev_desc, part_info))
		rkss_version = RKSS_VERSION_V1;
	else if (check_is_rkss_version2(dev_desc, part_info))
		rkss_version = RKSS_VERSION_V2;
	else
		rkss_version = RKSS_VERSION_V3;

	return rkss_version;
}

static int rkss_init(void)
{
	int version;

	version = get_rkss_version();
	printf("optee: rkss v%d\n", version);

	if (version == RKSS_VERSION_V1)
		return tee_supp_rk_fs_init_v1();
	else if (version == RKSS_VERSION_V2)
		return tee_supp_rk_fs_init_v2();
	else if (version == RKSS_VERSION_V3)
		return tee_supp_rk_fs_init_v3();
	else
		return -1;
}

static int rkss_process_request(u32 num_params,
				struct optee_msg_param *params)
{
	int version;

	version = get_rkss_version();
	debug("%s: get rkss version: %d\n", __func__, version);

	if (version == RKSS_VERSION_V1)
		return tee_supp_rk_fs_process_v1(num_params, params);
	else if (version == RKSS_VERSION_V2)
		return tee_supp_rk_fs_process_v2(num_params, params);
	else if (version == RKSS_VERSION_V3)
		return tee_supp_rk_fs_process_v3(num_params, params);
	else
		return -1;
}

void optee_suppl_cmd_fs(struct optee_msg_arg *arg)
{
	static int rkss_is_init = false;

	if (!rkss_is_init) {
		if (rkss_init() < 0) {
			printf("%s: rkss init failed!", __func__);
			arg->ret = TEE_ERROR_STORAGE_NOT_AVAILABLE;
			return;
		}
		rkss_is_init = true;
	}

	arg->ret = rkss_process_request(arg->num_params, arg->params);
}
