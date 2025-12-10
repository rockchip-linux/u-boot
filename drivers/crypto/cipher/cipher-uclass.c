// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 * Author: Troy Lin <troy.lin@rock-chips.com>
 */

#define LOG_CATEGORY UCLASS_CIPHER

#include <dm.h>
#include <asm/global_data.h>
#include <u-boot/cipher.h>
#include <keylad.h>
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

static const struct cipher_info cipher_mode[CIPHER_MODE_NUM] = {
	[CIPHER_MODE_ECB]     = { "ECB"},
	[CIPHER_MODE_CBC]     = { "CBC"},
	[CIPHER_MODE_CFB]     = { "CFB"},
	[CIPHER_MODE_OFB]     = { "OFB"},
	[CIPHER_MODE_CTS]     = { "CTS"},
	[CIPHER_MODE_CTR]     = { "CTR"},
	[CIPHER_MODE_XTS]     = { "XTS"},
	[CIPHER_MODE_CCM]     = { "CCM"},
	[CIPHER_MODE_GCM]     = { "GCM"},
	[CIPHER_MODE_CMAC]    = { "CMAC"},
	[CIPHER_MODE_CBC_MAC] = { "CBC_MAC"},
	[CIPHER_MODE_BYPASS]  = { "BYPASS"},
};

const char *cipher_algo_name(enum CIPHER_ALGO algo)
{
	if (algo >= CIPHER_ALGO_NUM)
		return NULL;

	return cipher_info[algo].name;
}

const char *cipher_mode_name(enum CIPHER_MODE mode)
{
	if (mode >= CIPHER_MODE_NUM)
		return NULL;

	return cipher_mode[mode].name;
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

int crypto_fw_cipher(struct udevice *dev, cipher_fw_context *ctx,
		    const u8 *in, u8 *out, u32 len, bool enc)
{
#if CONFIG_IS_ENABLED(DM_KEYLAD)
	struct cipher_ops *ops = (struct cipher_ops *)device_get_ops(dev);
	struct udevice *keylad_dev;

	if (!ops || !ops->cipher_fw_crypt)
		return -ENOSYS;

	if (!ctx)
		return -EINVAL;

	if (!ops->is_secure || !ops->is_secure(dev)) {
		printf("Only secure crypto support fwkey cipher.\n");
		return -ENOSYS;
	}

	keylad_dev = keylad_get_device();
	if (!keylad_dev) {
		printf("No keylad device found.\n");
		return -ENOSYS;
	}

	if (keylad_transfer_fwkey(keylad_dev, crypto_keytable_addr(dev),
				  ctx->fw_keyid, ctx->key_len)) {
		printf("Failed to transfer key from keylad.\n");
		return -ENOSYS;
	}

	return ops->cipher_fw_crypt(dev, ctx, in, out, len, enc);
#else
	return -ENOSYS;
#endif
}

ulong crypto_keytable_addr(struct udevice *dev)
{
	struct cipher_ops *ops = (struct cipher_ops *)device_get_ops(dev);

	if (!ops || !ops->keytable_addr)
		return 0;

	return ops->keytable_addr(dev);
}

bool crypto_is_secure(struct udevice *dev)
{
	struct cipher_ops *ops = (struct cipher_ops *)device_get_ops(dev);

	if (!ops || !ops->is_secure)
		return false;

	return ops->is_secure(dev);
}

UCLASS_DRIVER(cipher) = {
	.id	= UCLASS_CIPHER,
	.name	= "cipher",
};
