/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd
 */

#ifndef _ROCKCHIP_CRYPTO_MPA_H_
#define _ROCKCHIP_CRYPTO_MPA_H_
#include <common.h>

#define MPA_USE_ALLOC	1

struct mpa_num {
	u32 alloc;
	s32 size;
	u32 *d;
};

int rk_mpa_alloc(struct mpa_num **mpa, void *data, u32 word_size);
void rk_mpa_free(struct mpa_num **mpa);
int rk_check_size(u32 *data, u32 max_word_size);

#endif
