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
#include <u-boot/hash.h>
#include <u-boot/schedule.h>

//#define DEBUG

#ifdef DEBUG
#define DMSG(format, ...) printf("[%s %s, %05d]-trace: " format "\n", \
				 __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DMSG(format, ...)
#endif

struct crypto_hash_ctx {
	const struct crypto_impl *impl;
	enum HASH_ALGO algo;
	void *sha_ctx;
};

static int crypto_hash_init(struct udevice *dev, enum HASH_ALGO algo, void **ctxp)
{
	struct crypto_hash_ctx *hctx;
	const struct hash_ops *ops;
	int ret = -EINVAL;

	hctx = calloc(1, sizeof(struct crypto_hash_ctx));
	if (!hctx)
		return -ENOMEM;

	hctx->algo = algo;
	hctx->impl = crypto_get_impl(CRYPTO_TYPE_HASH, algo, CRYPTO_MODE_NONE);
	if (!hctx->impl) {
		printf("crypto-hash: No available algo '%s'\n", hash_algo_name(algo));
		goto exit;
	}

	ops = &hctx->impl->hash;
	if (!ops->hash_init)
		goto exit;

	ret = ops->hash_init(hctx->impl->dev, algo, &hctx->sha_ctx);
	*ctxp = hctx;
exit:
	return ret;
}

static int crypto_hash_update(struct udevice *dev, void *ctx, const void *ibuf, uint32_t ilen)
{
	struct crypto_hash_ctx *hctx = (struct crypto_hash_ctx *)ctx;
	const struct hash_ops *ops;
	int ret = -EINVAL;

	if (!hctx || !hctx->impl || !hctx->sha_ctx)
		goto exit;

	ops = &hctx->impl->hash;
	if (!ops->hash_update)
		goto exit;

	ret = ops->hash_update(hctx->impl->dev, hctx->sha_ctx, ibuf, ilen);
exit:
	return ret;
}

static int crypto_hash_finish(struct udevice *dev, void *ctx, void *obuf)
{
	struct crypto_hash_ctx *hctx = (struct crypto_hash_ctx *)ctx;
	const struct hash_ops *ops;
	int ret = -EINVAL;

	if (!hctx || !hctx->impl || !hctx->sha_ctx)
		goto exit;

	ops = &hctx->impl->hash;
	if (!ops->hash_finish)
		goto exit;

	ret = ops->hash_finish(hctx->impl->dev, hctx->sha_ctx, obuf);
	free(hctx);
exit:
	return 0;
}

static int crypto_hash_digest_wd(struct udevice *dev, enum HASH_ALGO algo,
				 const void *ibuf, const uint32_t ilen,
				 void *obuf, uint32_t chunk_sz)
{
	const void *cur, *end;
	uint32_t chunk;
	void *ctx;
	int rc;

	rc = crypto_hash_init(dev, algo, &ctx);
	if (rc)
		return rc;

	if (IS_ENABLED(CONFIG_HW_WATCHDOG) || CONFIG_IS_ENABLED(WATCHDOG)) {
		cur = ibuf;
		end = ibuf + ilen;

		while (cur < end) {
			chunk = end - cur;
			if (chunk > chunk_sz)
				chunk = chunk_sz;

			rc = crypto_hash_update(dev, ctx, cur, chunk);
			if (rc)
				return rc;

			cur += chunk;
			schedule();
		}
	} else {
		rc = crypto_hash_update(dev, ctx, ibuf, ilen);
		if (rc)
			return rc;
	}

	return crypto_hash_finish(dev, ctx, obuf);
}

static int crypto_hash_digest(struct udevice *dev, enum HASH_ALGO algo,
			      const void *ibuf, const uint32_t ilen,
			      void *obuf)
{
	/* re-use the watchdog version with input length as the chunk_sz */
	return crypto_hash_digest_wd(dev, algo, ibuf, ilen, obuf, ilen);
}

static const struct hash_ops crypto_hash_ops = {
	.hash_init      = crypto_hash_init,
	.hash_update    = crypto_hash_update,
	.hash_finish    = crypto_hash_finish,
	.hash_digest_wd = crypto_hash_digest_wd,
	.hash_digest    = crypto_hash_digest,
};

/* Add an 'aaa_' prefix so it comes as the first one in the linker list. */
U_BOOT_DRIVER(aaa_crypto_hash_glue) = {
	.name = "crypto_hash_glue",
	.id = UCLASS_HASH,
	.ops = &crypto_hash_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(aaa_crypto_hash_glue) = {
	.name = "crypto_hash_glue",
};
