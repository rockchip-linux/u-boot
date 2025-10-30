// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 * Author: Troy Lin <troy.lin@rock-chips.com>
 */

#define LOG_CATEGORY UCLASS_CIPHER

#include <dm.h>
#include <asm/global_data.h>
#include <u-boot/cipher.h>
#include <errno.h>
#include <fdtdec.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/list.h>

struct cipher_info {
	char *name;
};

static const struct cipher_info cipher_info[CIPHER_ALGO_NUM] = {
	[CIPHER_ALGO_DES] = { "DES"},
	[CIPHER_ALGO_AES] = { "AES"},
	[CIPHER_ALGO_SM4] = { "SM4"},
};

const char *cipher_algo_name(enum CIPHER_ALGO algo)
{
	if (algo >= CIPHER_ALGO_NUM)
		return NULL;

	return cipher_info[algo].name;
}

int crypto_cipher(struct udevice *dev, cipher_context *ctx,
		  const u8 *in, u8 *out, u32 len, bool enc)
{
	struct cipher_ops *ops = (struct cipher_ops *)device_get_ops(dev);

	if (!ops || !ops->cipher_crypt)
		return -ENOSYS;

	if (!ctx || !ctx->key || ctx->key_len == 0)
		return -EINVAL;

	return ops->cipher_crypt(dev, ctx, in, out, len, enc);
}

int crypto_mac(struct udevice *dev, cipher_context *ctx,
	       const u8 *in, u32 len, u8 *tag)
{
	struct cipher_ops *ops = (struct cipher_ops *)device_get_ops(dev);

	if (!ops || !ops->cipher_mac)
		return -ENOSYS;

	if (!ctx || !ctx->key || ctx->key_len == 0)
		return -EINVAL;

	return ops->cipher_mac(dev, ctx, in, len, tag);
}

int crypto_ae(struct udevice *dev, cipher_context *ctx,
	      const u8 *in, u32 len, const u8 *aad, u32 aad_len,
	      u8 *out, u8 *tag)
{
	struct cipher_ops *ops = (struct cipher_ops *)device_get_ops(dev);

	if (!ops || !ops->cipher_ae)
		return -ENOSYS;

	if (!ctx || !ctx->key || ctx->key_len == 0)
		return -EINVAL;

	return ops->cipher_ae(dev, ctx, in, len, aad, aad_len, out, tag);
}

UCLASS_DRIVER(cipher) = {
	.id	= UCLASS_CIPHER,
	.name	= "cipher",
};
