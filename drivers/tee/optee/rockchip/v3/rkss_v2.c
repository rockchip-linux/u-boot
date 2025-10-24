/*
 * Copyright 2025, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <command.h>
#include <part.h>
#include <stdlib.h>
#include "rkss.h"

int tee_supp_rk_fs_init_v2(void)
{
	printf("optee: rkss v2 init not support for optee v3 platform!\n");
	return -1;
}

int tee_supp_rk_fs_process_v2(size_t num_params,
			      struct optee_msg_param *params)
{
	printf("optee: rkss v2 process not support for optee v3 platform!\n");
	return -1;
}
