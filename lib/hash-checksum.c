// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013, Andreas Oetken.
 */

#ifndef USE_HOSTCC
#include <dm.h>
#include <fdtdec.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <linux/errno.h>
#include <u-boot/hash.h>
#else
#include "fdt_host.h"
#endif
#include <hash.h>
#include <image.h>

#ifdef USE_HOSTCC
int hash_calculate(const char *name,
		    const struct image_region *region,
		    int region_count, uint8_t *checksum)
{
	struct hash_algo *algo;
	int ret = 0;
	void *ctx;
	int i;

	if (region_count < 1)
		return -EINVAL;

	ret = hash_progressive_lookup_algo(name, &algo);
	if (ret)
		return ret;

	ret = algo->hash_init(algo, &ctx);
	if (ret)
		return ret;

	for (i = 0; i < region_count - 1; i++) {
		ret = algo->hash_update(algo, ctx, region[i].data,
					region[i].size, 0);
		if (ret)
			return ret;
	}

	ret = algo->hash_update(algo, ctx, region[i].data, region[i].size, 1);
	if (ret)
		return ret;
	ret = algo->hash_finish(algo, ctx, checksum, algo->digest_size);
	if (ret)
		return ret;

	return 0;
}

#else
int hash_calculate(const char *name,
		    const struct image_region *region,
		    int region_count, uint8_t *checksum)
{
	enum HASH_ALGO algo = HASH_ALGO_INVALID;
	struct udevice *dev;
	void *ctx = NULL;
	int i, ret;

	ret = uclass_get_device(UCLASS_HASH, 0, &dev);
	if (ret) {
		printf("%s: No crypto-hash device, ret=%d\n", __func__, ret);
		return ret;
	}

	if (!strcmp(name, "sha1"))
		algo = HASH_ALGO_SHA1;
	else if (!strcmp(name, "sha256"))
		algo = HASH_ALGO_SHA256;
	else if (!strcmp(name, "sha512"))
		algo = HASH_ALGO_SHA512;
	else if (!strcmp(name, "sha384"))
		algo = HASH_ALGO_SHA384;
	else if (!strcmp(name, "md5"))
		algo = HASH_ALGO_MD5;
	else if (!strcmp(name, "crc16-ccitt"))
		algo = HASH_ALGO_CRC16_CCITT;
	else if (!strcmp(name, "crc32"))
		algo = HASH_ALGO_CRC32;

	if (algo == HASH_ALGO_INVALID) {
		printf("%s: No available algo '%s'\n", __func__, name);
		return -EINVAL;
	}

	ret = hash_init(dev, algo, &ctx);
	if (ret)
		return ret;

	for (i = 0; i < region_count; i++) {
		ret = hash_update(dev, ctx, region[i].data, region[i].size);
		if (ret)
			return ret;
	}

	ret = hash_finish(dev, ctx, checksum);
	if (ret)
		return ret;

	return 0;
}

#endif
