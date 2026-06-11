// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd.
 */
#include <common.h>
#include <android_ab.h>
#include <fastboot.h>
#include <env.h>
#ifdef CONFIG_AVB_VERIFY
#include <avb_verify.h>
#endif
#include <u-boot/sha256.h>

#ifdef CONFIG_FASTBOOT_OEM_BOARD
static void oem_permanent_attributes(char *response)
{
#ifdef CONFIG_LIBAVB_USER
#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
	sha256_context ctx;
	AvbAtxPermanentAttributes permanent_attributes_temp;
	uint8_t digest[SHA256_SUM_LEN] = {0};
	uint8_t digest_temp[SHA256_SUM_LEN] = {0};
	uint8_t flag = 0;
	u32 image_size = env_get_hex("filesize", 0);

	if (image_size != PERM_ATTR_TOTAL_SIZE) {
		printf("Permanent attribute size is not equal!, image_size is %x\n", image_size);
		fastboot_fail("Incorrect perm attribute size", response);
		return;
	}

	memset(&permanent_attributes_temp, 0, sizeof(permanent_attributes_temp));
#endif
	AvbOps* ops;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return;
	}

#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
	if (avb_read_permanent_attributes_flag(&flag)) {
		printf("avb_read_permanent_attributes_flag error!\n");
		fastboot_fail("permanent attributes read failed", response);
		return;
	}

	if (flag == PERM_ATTR_SUCCESS_FLAG) {
		if (ops->atx_ops->read_permanent_attributes_hash(ops->atx_ops,
								 digest_temp)) {
			printf("The Efuse/OTP IO can not be used!\n");
			fastboot_fail("Efuse/OTP IO can not be used", response);
			return;
		}

		if (memcmp(digest, digest_temp, SHA256_SUM_LEN) != 0) {
			if (ops->atx_ops->read_permanent_attributes(ops->atx_ops,
								    &permanent_attributes_temp)) {
				printf("avb_write_permanent_attributes error!\n");
				fastboot_fail("Read perm attr error", response);
				return;
			}

			sha256_starts(&ctx);
			sha256_update(&ctx,
				      (const uint8_t *)&permanent_attributes_temp,
				      PERM_ATTR_TOTAL_SIZE);
			sha256_finish(&ctx, digest);
			if (memcmp(digest, digest_temp, SHA256_SUM_LEN) == 0) {
				printf("The hash has been written!\n");
				fastboot_okay(NULL, response);
				return;
			}
		}

		if (avb_write_permanent_attributes_flag(0)) {
			fastboot_fail("permanent attributes flag write failure", response);
			return;
		}
	}
#endif
	if (avb_write_permanent_attributes(ops->atx_ops,
					   (AvbAtxPermanentAttributes *)CONFIG_FASTBOOT_BUF_ADDR)) {
		if (avb_write_permanent_attributes_flag(0)) {
			fastboot_fail("permanent attributes flag write failure", response);
			return;
		}
		fastboot_fail("permanent attributes flag write failed", response);
		return;
	}
#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
	memset(digest, 0, SHA256_SUM_LEN);
	sha256_starts(&ctx);
	sha256_update(&ctx, (const uint8_t *)CONFIG_FASTBOOT_BUF_ADDR,
		      PERM_ATTR_TOTAL_SIZE);
	sha256_finish(&ctx, digest);

	if (avb_write_permanent_attributes_hash((uint8_t *)digest,
						SHA256_SUM_LEN)) {
		if (avb_read_permanent_attributes_hash(ops->atx_ops,
						       digest_temp)) {
			printf("The efuse IO can not be used!\n");
			fastboot_fail("Efuse IO can not be used", response);
			return;
		}
		if (memcmp(digest, digest_temp, SHA256_SUM_LEN) != 0) {
			if (avb_write_permanent_attributes_flag(0)) {
				fastboot_fail("permanent attributes flag write failure", response);
				return;
			}
			printf("The hash has been written, but is different!\n");
			fastboot_fail("Hash comparison failure", response);
			return;
		}
	}
#endif
	if (avb_write_permanent_attributes_flag(PERM_ATTR_SUCCESS_FLAG)) {
		fastboot_fail("permanent attributes flag write failure", response);
		return;
	}

	fastboot_okay(NULL, response);
#else
	fastboot_fail("Not implemented", response);
#endif
}

static void oem_permanent_attributes_rsa_cer(char *response)
{
	u32 image_size = env_get_hex("filesize", 0);

#ifdef CONFIG_LIBAVB_USER
	if (image_size != 256) {
		printf("Permanent attribute rsa hash size is not equal!\n");
		fastboot_fail("Permanent attribute rsa hash size error", response);
		return;
	}

	if (avb_set_permanent_attributes_cer((uint8_t *)CONFIG_FASTBOOT_BUF_ADDR,
					     image_size)) {
		fastboot_fail("Set Permanent attribute cert fail!", response);
		return;
	}

	fastboot_okay(NULL, response);
#else
	fastboot_fail("Not implemented", response);
#endif
}

void fastboot_oem_board(char *cmd_parameter, void *data, u32 size, char *response)
{
	if (strncmp("at-get-unlock-challenge", cmd_parameter, 29) == 0) {
#ifdef CONFIG_LIBAVB_USER
		uint32_t challenge_len = 0;
		int ret = 0;

		ret = avb_generate_unlock_challenge((void *)CONFIG_FASTBOOT_BUF_ADDR,
						    &challenge_len);
		if (ret == 0) {
			env_set_hex("filesize", challenge_len);
			fastboot_okay(NULL, response);
		} else {
			fastboot_fail("Generate unlock challenge fail!", response);
		}
#else
		fastboot_fail("Not implemented", response);
		return;
#endif
	} else if (strncmp("lock", cmd_parameter, 4) == 0) {
#ifdef CONFIG_LIBAVB_USER
		uint8_t lock_state;
		lock_state = 0;
		if (avb_write_lock_state(lock_state))
			fastboot_fail("Write lock state failed", response);
		else
			fastboot_okay(NULL, response);
#else
		fastboot_fail("Not implemented", response);
#endif
	} else if (strncmp("unlock", cmd_parameter, 6) == 0) {
#ifdef CONFIG_LIBAVB_USER
		uint8_t lock_state;
		char out_is_trusted = true;

		if (avb_read_lock_state(&lock_state))
			fastboot_fail("Lock state read failure", response);
		if (lock_state >> 1 == 1) {
			fastboot_fail("Wrong lock state", response);
		} else {
			lock_state = 1;
#ifdef CONFIG_LIBAVB_ATH_UNLOCK_SUPPORT
			if (avb_auth_unlock((void *)CONFIG_FASTBOOT_BUF_ADDR,
					    &out_is_trusted)) {
				printf("avb_auth_unlock ops error!\n");
				fastboot_fail("avb_auth_unlock ops error!", response);
				return;
			}
#endif
			if (out_is_trusted == true) {
				if (avb_write_lock_state(lock_state))
					fastboot_fail("Write lock state failed", response);
				else
					fastboot_okay(NULL, response);
			} else {
				fastboot_fail("authenticated unlock fail", response);
			}
		}
#else
		fastboot_fail("Not implemented", response);
#endif
	} else if (strncmp("fuse at-perm-attr", cmd_parameter, 16) == 0) {
		oem_permanent_attributes(response);
	} else if (strncmp("fuse at-rsa-perm-attr", cmd_parameter, 25) == 0) {
		oem_permanent_attributes_rsa_cer(response);
	} else if (strncmp("fuse at-bootloader-vboot-key", cmd_parameter, 27) == 0) {
#ifdef CONFIG_LIBAVB_USER
		sha256_context ctx;
		uint8_t digest[SHA256_SUM_LEN];
		u32 image_size = env_get_hex("filesize", 0);

		if (image_size != VBOOT_KEY_SIZE) {
			fastboot_fail("Invalid vboot key length", response);
			printf("The vboot key size error!\n");
			return;
		}

		sha256_starts(&ctx);
		sha256_update(&ctx, (const uint8_t *)CONFIG_FASTBOOT_BUF_ADDR,
			      VBOOT_KEY_SIZE);
		sha256_finish(&ctx, digest);

		if (avb_write_vbootkey_hash((uint8_t *)digest,
					    SHA256_SUM_LEN)) {
			fastboot_fail("Vbootkey hash write failure", response);
			return;
		}
		fastboot_okay(NULL, response);
#else
		fastboot_fail("Not implemented", response);
#endif
	} else if (strncmp("init-ab-metadata", cmd_parameter, 16) == 0) {
#ifdef CONFIG_LIBAVB_USER
		if (ab_init_metadata()) {
			fastboot_fail("Init ab data fail!", response);
			return;
		}
		fastboot_okay(NULL, response);
#else
		fastboot_fail("Not implemented", response);
#endif
	} else {
		fastboot_fail("Unknown oem command", response);
	}
}
#endif
