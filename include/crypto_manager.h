/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef _CRYPTO_MANAGER_H_
#define _CRYPTO_MANAGER_H_

#include <common.h>
#include <image.h>
#include <u-boot/hash.h>
#include <u-boot/hmac.h>
#include <u-boot/cipher.h>
#include <u-boot/aead.h>
#include <u-boot/mac.h>
#include <u-boot/asym.h>
#include <u-boot/rsa-mod-exp.h>
#include <u-boot/ecdsa.h>

#define CRYPTO_MISC_MANAGER		"crypto_manager"
#define CRYPTO_DRIVER_MAX		64
#define CRYPTO_MODE_NONE		0xdeadbeef

#define CRYPTO_PRIORITY_BEST		255
#define CRYPTO_PRIORITY_HW		200
#define CRYPTO_PRIORITY_SW		100

enum crypto_type {
	CRYPTO_TYPE_HASH,
	CRYPTO_TYPE_HMAC,
	CRYPTO_TYPE_CIPHER,
	CRYPTO_TYPE_ASYM,
	CRYPTO_TYPE_MAX,
};

struct crypto_impl {
	struct udevice       *dev;
	enum crypto_type     type;
	const char           *name;
	u32                  uclass_id;
	u32                  priority;
	u32  (*dynamic_priority)(struct udevice *dev, u32 algo, u32 mode);
	bool (*check_valid)(struct udevice *dev, u32 algo, u32 mode);

	union {
		struct cipher_ops cipher;
		struct aead_ops   aead;
		struct mac_ops    mac;
		struct hash_ops   hash;
		struct hmac_ops   hmac;
		struct asym_ops   asym;
	};
};

int crypto_impl_register(const struct crypto_impl *impl);

void crypto_impl_unregister(const struct crypto_impl *impl);

const struct crypto_impl *crypto_get_impl_by_index(enum crypto_type type, u32 index);

const struct crypto_impl *crypto_get_impl(enum crypto_type type, u32 algo, u32 mode);

const char *crypto_get_driver_name(const struct crypto_impl *impl);

#endif
