/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/arch-rockchip/cpu.h>

static int bsoc_id = 0;

void board_soc_id_init(int id)
{
	bsoc_id = id;
}

int board_soc_id(void)
{
	return bsoc_id;
}
