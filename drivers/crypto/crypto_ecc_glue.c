// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
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

#if CONFIG_IS_ENABLED(ECDSA_VERIFY)

int crypto_ecdsa_verify(struct udevice *dev, const struct ecdsa_public_key *pubkey,
			const void *hash, size_t hash_len,
			const void *signature, size_t sig_len)
{
	const struct crypto_impl *impl = NULL;
	const struct ecdsa_ops *ops = NULL;

	if (!pubkey || !hash || !signature)
		return -EINVAL;

	DMSG("enter");
	impl = crypto_get_impl(CRYPTO_TYPE_ASYM, ASYM_ALGO_ECC, CRYPTO_MODE_NONE);
	if (!impl) {
		DMSG("crypto_get_impl CRYPTO_TYPE_ASYM ECC faild\n");
		return -ENOSYS;
	}

	ops = &impl->asym.ecc;
	if (!ops->verify)
		return -ENOSYS;

	return ops->verify(impl->dev, pubkey, hash, hash_len, signature, sig_len);
}
static const struct ecdsa_ops crypto_ecdsa_op = {
	.verify = crypto_ecdsa_verify,
};

/* Add an 'aaa_' prefix so it comes as the first one in the linker list. */
U_BOOT_DRIVER(aaa_crypto_ecc_glue) = {
	.name  = "crypto_ecc_glue",
	.id    = UCLASS_ECDSA,
	.ops   = &crypto_ecdsa_op,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(aaa_crypto_ecc_glue) = {
	.name = "crypto_ecc_glue",
};

#endif
