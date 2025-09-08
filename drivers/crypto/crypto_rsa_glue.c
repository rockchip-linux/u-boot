// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd
 * Author: Troy Lin <troy.lin@rock-chips.com>
 */
#include <config.h>
#include <common.h>
#include <crypto_manager.h>
#include <dm.h>
#include <log.h>
#include <malloc.h>

//#define DEBUG

#ifdef DEBUG
#define DMSG(format, ...) printf("[%s %s, %05d]-trace: " format "\n", \
				 __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DMSG(format, ...)
#endif

static int crypto_mod_exp(struct udevice *dev, const uint8_t *sig, uint32_t sig_len,
			  struct key_prop *prop, uint8_t *out)
{
	const struct crypto_impl *impl = NULL;
	const struct mod_exp_ops *ops = NULL;

	DMSG("enter");
	impl = crypto_get_impl(CRYPTO_TYPE_ASYM, ASYM_ALGO_RSA, CRYPTO_MODE_NONE);
	if (!impl) {
		DMSG("crypto_get_impl CRYPTO_TYPE_HASH faild\n");
		return -EINVAL;
	}

	ops = &impl->asym.rsa;
	if (!ops->mod_exp)
		return -ENOSYS;

	return ops->mod_exp(impl->dev, sig, sig_len, prop, out);
}

static const struct mod_exp_ops crypto_mod_exp_op = {
	.mod_exp = crypto_mod_exp,
};

/* Add an 'aaa_' prefix so it comes as the first one in the linker list. */
U_BOOT_DRIVER(aaa_crypto_rsa_glue) = {
	.name  = "crypto_rsa_glue",
	.id    = UCLASS_MOD_EXP,
	.ops   = &crypto_mod_exp_op,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(aaa_crypto_rsa_glue) = {
	.name = "crypto_rsa_glue",
};
