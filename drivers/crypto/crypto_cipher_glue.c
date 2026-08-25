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
#include <u-boot/hash.h>
#include <u-boot/schedule.h>

//#define DEBUG

#ifdef DEBUG
#define DMSG(format, ...) printf("[%s %s, %05d]-trace: " format "\n", \
				 __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DMSG(format, ...)
#endif

static const struct crypto_impl *crypto_get_cipher_impl(struct udevice *dev, enum CIPHER_ALGO algo)
{
	const struct crypto_impl *impl;
	impl = crypto_get_impl(CRYPTO_TYPE_CIPHER, algo, CRYPTO_MODE_NONE);
	if (!impl)
		return NULL;

	return impl;
}

static int crypto_cipher_crypt(struct udevice *dev, cipher_context *ctx,
			       const u8 *in, u8 *out, u32 len, bool enc)
{
	const struct crypto_impl *impl;
	const struct cipher_ops *ops;

	impl = crypto_get_cipher_impl(dev, ctx->algo);
	if (!impl)
		return -ENOSYS;

	ops = &impl->cipher;
	if (!ops || !ops->cipher_crypt)
		return -ENOSYS;

	return ops->cipher_crypt(impl->dev, ctx, in, out, len, enc);
}

static int crypto_cipher_mac(struct udevice *dev, cipher_context *ctx,
			     const u8 *in, u32 len, u8 *tag)
{
	const struct crypto_impl *impl;
	const struct cipher_ops *ops;

	impl = crypto_get_cipher_impl(dev, ctx->algo);
	if (!impl)
		return -ENOSYS;

	ops = &impl->cipher;
	if (!ops || !ops->cipher_mac)
		return -ENOSYS;

	return ops->cipher_mac(impl->dev, ctx, in, len, tag);
}

static int crypto_cipher_ae(struct udevice *dev, cipher_context *ctx,
			    const u8 *in, u32 len, const u8 *aad, u32 aad_len,
			    u8 *out, u8 *tag)
{
	const struct crypto_impl *impl;
	const struct cipher_ops *ops;

	impl = crypto_get_cipher_impl(dev, ctx->algo);
	if (!impl)
		return -ENOSYS;

	ops = &impl->cipher;
	if (!ops || !ops->cipher_ae)
		return -ENOSYS;

	return ops->cipher_ae(impl->dev, ctx, in, len, aad, aad_len, out, tag);
}

static int crypto_cipher_fw_crypt(struct udevice *dev, cipher_fw_context *ctx,
				  const u8 *in, u8 *out, u32 len, bool enc)
{
	const struct crypto_impl *impl;
	const struct cipher_ops *ops;

	impl = crypto_get_cipher_impl(dev, ctx->algo);
	if (!impl)
		return -ENOSYS;

	ops = &impl->cipher;
	if (!ops || !ops->cipher_fw_crypt)
		return -ENOSYS;

	return ops->cipher_fw_crypt(impl->dev, ctx, in, out, len, enc);
}

static ulong crypto_cipher_keytable_addr(struct udevice *dev)
{
	const struct crypto_impl *impl;
	const struct cipher_ops *ops;

	impl = crypto_get_cipher_impl(dev, CIPHER_ALGO_AES);
	if (!impl)
		return 0;

	ops = &impl->cipher;
	if (!ops || !ops->keytable_addr)
		return 0;

	return ops->keytable_addr(impl->dev);
}

static bool crypto_cipher_is_secure(struct udevice *dev)
{
	const struct crypto_impl *impl;
	const struct cipher_ops *ops;

	impl = crypto_get_cipher_impl(dev, CIPHER_ALGO_AES);
	if (!impl)
		return false;

	ops = &impl->cipher;
	if (!ops || !ops->is_secure)
		return false;

	return ops->is_secure(impl->dev);
}

static const struct cipher_ops crypto_cipher_ops = {
	.cipher_crypt = crypto_cipher_crypt,
	.cipher_mac   = crypto_cipher_mac,
	.cipher_ae    = crypto_cipher_ae,

	.cipher_fw_crypt = crypto_cipher_fw_crypt,
	.keytable_addr   = crypto_cipher_keytable_addr,
	.is_secure       = crypto_cipher_is_secure,
};

/* Add an 'aaa_' prefix so it comes as the first one in the linker list. */
U_BOOT_DRIVER(aaa_crypto_cipher_glue) = {
	.name = "crypto_cipher_glue",
	.id = UCLASS_CIPHER,
	.ops = &crypto_cipher_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(aaa_crypto_cipher_glue) = {
	.name = "crypto_cipher_glue",
};
