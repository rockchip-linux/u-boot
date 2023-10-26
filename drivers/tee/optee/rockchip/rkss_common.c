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

static int get_rkss_version(void)
{
	static int rkss_version = 0;
	struct blk_desc *dev_desc = NULL;
	struct disk_partition part_info;
	u8 *read_buff;
	ulong ret = 0;
	u32 *version;
	u32 *checkstr;

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

	read_buff = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, 512);
	if (!read_buff) {
		printf("%s: Malloc buf failed!\n", __func__);
		return -1;
	}

	ret = blk_dread(dev_desc, part_info.start, 1, read_buff);
	if (ret != 1) {
		printf("%s: blk_dread failed!\n", __func__);
		free(read_buff);
		return -1;
	}

	version = (u32 *)(read_buff + 512 - 8);
	checkstr = (u32 *)(read_buff + 512 - 4);

	if (*version == 1 && *checkstr == 0x12345678)
		rkss_version = RKSS_VERSION_V1;
	else
		rkss_version = RKSS_VERSION_V2;

	free(read_buff);
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
	else
		return -1;
}

#ifdef CONFIG_ROCKCHIP_OPTEE_V2
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
	else
		return -1;
}
#endif

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

#ifdef CONFIG_ROCKCHIP_OPTEE_V2
	arg->ret = rkss_process_request(arg->num_params, arg->params);
#endif
}
