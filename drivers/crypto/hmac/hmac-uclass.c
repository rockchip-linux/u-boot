// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 * Author: Troy Lin <troy.lin@rock-chips.com>
 */

#define LOG_CATEGORY UCLASS_HMAC

#include <dm.h>
#include <asm/global_data.h>
#include <u-boot/hmac.h>
#include <errno.h>
#include <fdtdec.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/list.h>

struct hmac_info {
	char *name;
	uint32_t digest_size;
};

static const struct hmac_info hmac_info[HMAC_ALGO_NUM] = {
	[HMAC_ALGO_MD5]    = { "hmac-md5", 16 },
	[HMAC_ALGO_SHA1]   = { "hmac-sha1", 20 },
	[HMAC_ALGO_SHA256] = { "hmac-sha256", 32 },
	[HMAC_ALGO_SHA512] = { "hmac-sha512", 64},
	[HMAC_ALGO_SM3]    = { "hmac-sm3", 32},
};

enum HMAC_ALGO hmac_algo_lookup_by_name(const char *name)
{
	int i;

	if (!name)
		return HMAC_ALGO_INVALID;

	for (i = 0; i < HMAC_ALGO_NUM; ++i)
		if (!strcmp(name, hmac_info[i].name))
			return i;

	return HMAC_ALGO_INVALID;
}

ssize_t hmac_algo_digest_size(enum HMAC_ALGO algo)
{
	if (algo >= HMAC_ALGO_NUM)
		return -EINVAL;

	return hmac_info[algo].digest_size;
}

const char *hmac_algo_name(enum HMAC_ALGO algo)
{
	if (algo >= HMAC_ALGO_NUM)
		return NULL;

	return hmac_info[algo].name;
}

int hmac_digest(struct udevice *dev, enum HMAC_ALGO algo,
		const char *key, uint32_t keylen,
		const void *ibuf, const uint32_t ilen,
		void *obuf)
{
	struct hmac_ops *ops = (struct hmac_ops *)device_get_ops(dev);

	if (algo >= HMAC_ALGO_NUM)
		return -EINVAL;

	if (ilen != 0 && !ibuf)
		return -EINVAL;

	if (!obuf)
		return -EINVAL;

	if (!ops || !ops->hmac_digest)
		return -ENOSYS;


	return ops->hmac_digest(dev, algo, key, keylen, ibuf, ilen, obuf);
}

int hmac_digest_wd(struct udevice *dev, enum HMAC_ALGO algo,
		   const char *key, uint32_t keylen,
		   const void *ibuf, const uint32_t ilen,
		   void *obuf, uint32_t chunk_sz)
{
	struct hmac_ops *ops = (struct hmac_ops *)device_get_ops(dev);

	if (algo >= HMAC_ALGO_NUM)
		return -EINVAL;

	if (ilen != 0 && !ibuf)
		return -EINVAL;

	if (!obuf || chunk_sz == 0)
		return -EINVAL;

	if (!ops || !ops->hmac_digest_wd)
		return -ENOSYS;

	return ops->hmac_digest_wd(dev, algo, key, keylen, ibuf, ilen, obuf, chunk_sz);
}

int hmac_init(struct udevice *dev, enum HMAC_ALGO algo,
	      const char *key, uint32_t keylen, void **ctxp)
{
	struct hmac_ops *ops = (struct hmac_ops *)device_get_ops(dev);

	if (algo >= HMAC_ALGO_NUM)
		return -EINVAL;

	if (!ctxp)
		return -EINVAL;

	if (!ops || !ops->hmac_init)
		return -ENOSYS;

	return ops->hmac_init(dev, algo, key, keylen, ctxp);
}

int hmac_update(struct udevice *dev, void *ctx, const void *ibuf, const uint32_t ilen)
{
	struct hmac_ops *ops = (struct hmac_ops *)device_get_ops(dev);

	if (!ctx)
		return -EINVAL;

	if (ilen != 0 && !ibuf)
		return -EINVAL;

	if (!ops || !ops->hmac_update)
		return -ENOSYS;

	return ops->hmac_update(dev, ctx, ibuf, ilen);
}

int hmac_finish(struct udevice *dev, void *ctx, void *obuf)
{
	struct hmac_ops *ops = (struct hmac_ops *)device_get_ops(dev);

	if (!ctx)
		return -EINVAL;

	if (!obuf)
		return -EINVAL;

	if (!ops || !ops->hmac_finish)
		return -ENOSYS;

	return ops->hmac_finish(dev, ctx, obuf);
}

UCLASS_DRIVER(hmac) = {
	.id	= UCLASS_HMAC,
	.name	= "hmac",
};
