// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <malloc.h>
#include <rockchip/crypto_mpa.h>

int rk_mpa_alloc(struct mpa_num **mpa, void *data, u32 word_size)
{
	u32 alignment = sizeof(u32);
	u32 byte_size = word_size * sizeof(u32);
	struct mpa_num *tmp_mpa = NULL;

	if (!mpa || word_size == 0)
		return -EINVAL;

	*mpa = NULL;

	tmp_mpa = malloc(sizeof(*tmp_mpa));
	if (!tmp_mpa)
		return -ENOMEM;

	memset(tmp_mpa, 0x00, sizeof(*tmp_mpa));

	if (!data || (unsigned long)data % alignment) {
		tmp_mpa->d = memalign(alignment, byte_size);
		if (!tmp_mpa->d) {
			free(tmp_mpa);
			return -ENOMEM;
		}

		if (data)
			memcpy(tmp_mpa->d, data, byte_size);
		else
			memset(tmp_mpa->d, 0x00, byte_size);

		tmp_mpa->alloc = MPA_USE_ALLOC;
	} else {
		tmp_mpa->d = data;
	}

	tmp_mpa->size  = word_size;

	*mpa = tmp_mpa;

	return 0;
}

void rk_mpa_free(struct mpa_num **mpa)
{
	struct mpa_num *tmp_mpa = NULL;

	if (mpa && (*mpa)) {
		tmp_mpa = *mpa;
		if (tmp_mpa->alloc == MPA_USE_ALLOC)
			free(tmp_mpa->d);

		free(tmp_mpa);
	}
}

/*get bignum data length*/
int rk_check_size(u32 *data, u32 max_word_size)
{
	for (int i = (max_word_size - 1); i >= 0; i--) {
		if (data[i] == 0)
			continue;
		else
			return (i + 1);
	}
	return 0;
}
