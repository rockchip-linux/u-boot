/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef _UBOOT_HMAC_H
#define _UBOOT_HMAC_H

enum HMAC_ALGO {
	HMAC_ALGO_MD5,
	HMAC_ALGO_SHA1,
	HMAC_ALGO_SHA256,
	HMAC_ALGO_SHA512,
	HMAC_ALGO_SM3,

	HMAC_ALGO_NUM,

	HMAC_ALGO_INVALID = 0xffffffff,
};

/* general APIs for hmac algo information */

/*
 * struct hmac_ops - Driver model for Hmac operations
 *
 * The uclass interface is implemented by all hmac devices
 * which use driver model.
 */
struct hmac_ops {
	/* progressive operations */
	int (*hmac_init)(struct udevice *dev, enum HMAC_ALGO algo,
			 const char *key, uint32_t keylen, void **ctxp);
	int (*hmac_update)(struct udevice *dev, void *ctx,
			   const void *ibuf, const uint32_t ilen);
	int (*hmac_finish)(struct udevice *dev, void *ctx, void *obuf);

	/* all-in-one operation */
	int (*hmac_digest)(struct udevice *dev, enum HMAC_ALGO algo,
			   const char *key, uint32_t keylen, 
			   const void *ibuf, const uint32_t ilen,
			   void *obuf);

	/* all-in-one operation with watchdog triggering every chunk_sz */
	int (*hmac_digest_wd)(struct udevice *dev, enum HMAC_ALGO algo,
			      const char *key, uint32_t keylen, 
			      const void *ibuf, const uint32_t ilen,
			      void *obuf, uint32_t chunk_sz);

};
#endif
