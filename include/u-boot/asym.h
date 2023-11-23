/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef _UBOOT_ASYM_H
#define _UBOOT_ASYM_H

#include <u-boot/rsa-mod-exp.h>

enum ASYM_ALGO {
	ASYM_ALGO_RSA,
	ASYM_ALGO_ECC,
	ASYM_ALGO_SM2,

	ASYM_ALGO_NUM,

	ASYM_ALGO_INVALID = 0xffffffff,
};

/* general APIs for asym algo information */

/*
 * struct asym_ops - Driver model for Asym operations
 *
 * The uclass interface is implemented by all hash devices
 * which use driver model.
 */
struct asym_ops {
	union {
		struct mod_exp_ops rsa;
	};
};

#endif
