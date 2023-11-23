/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef _UBOOT_MAC_H
#define _UBOOT_MAC_H

enum MAC_ALGO {
	MAC_ALGO_DES,
	MAC_ALGO_AES,
	MAC_ALGO_SM4,

	MAC_ALGO_NUM,

	MAC_ALGO_INVALID = 0xffffffff,
};

enum MAC_MODE {
	MAC_MODE_CMAC,
	MAC_MODE_CBC_MAC,
};


/* general APIs for mac algo information */

/*
 * struct mac_ops - Driver model for mac operations
 *
 * The uclass interface is implemented by all mac devices
 * which use driver model.
 */
struct mac_ops {
	/* progressive operations */
	int (*mac_init)(struct udevice *dev, enum MAC_ALGO algo, enum MAC_MODE mode,
			const char *key, uint32_t keylen, void **ctxp);
	int (*mac_update)(struct udevice *dev, void *ctx, const void *ibuf, const uint32_t ilen);
	int (*mac_finish)(struct udevice *dev, void *ctx, void *obuf);

	/* all-in-one operation */
	int (*mac_digest)(struct udevice *dev, enum MAC_ALGO algo, enum MAC_MODE mode,
			  const char *key, uint32_t keylen, 
			  const void *ibuf, const uint32_t ilen,
			  void *obuf);

	/* all-in-one operation with watchdog triggering every chunk_sz */
	int (*mac_digest_wd)(struct udevice *dev, enum MAC_ALGO algo, enum MAC_MODE mode,
			     const char *key, uint32_t keylen, 
			     const void *ibuf, const uint32_t ilen,
			     void *obuf, uint32_t chunk_sz);
};
#endif
