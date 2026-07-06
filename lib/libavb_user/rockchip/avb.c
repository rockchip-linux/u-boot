/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <image.h>
#include <android_image.h>
#include <malloc.h>
#include <mapmem.h>
#include <errno.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <tee.h>
#include <android_avb/avb.h>
#include <tee/optee.h>
#include <u-boot/sha256.h>
#include <asm/arch-rockchip/atags.h>

/*
 * RK used
 * 1. Need call optee functions.
 * 2. Need call avb_ops_user_new first.
 */
AvbIOResult avb_get_pub_key(struct rk_pub_key *pub_key)
{
	struct tag *t = NULL;

	t = atags_get_tag(ATAG_PUB_KEY);
	if (!t)
		return AVB_IO_RESULT_ERROR_IO;

	memcpy(pub_key, t->u.pub_key.data, sizeof(struct rk_pub_key));

	return AVB_IO_RESULT_OK;
}

AvbIOResult avb_get_permanent_attributes_cer(uint8_t *cer, uint32_t size)
{
#ifdef CONFIG_OPTEE
	if (optee_read_permanent_attributes_cer((uint8_t *)cer, size)) {
		printf("AVB: perm attr cer is not exist.\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_set_permanent_attributes_cer(uint8_t *cer, uint32_t size)
{
#ifdef CONFIG_OPTEE
	if (optee_write_permanent_attributes_cer((uint8_t *)cer, size))
		return AVB_IO_RESULT_ERROR_IO;

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_read_flash_lock_state(uint8_t *flash_lock_state)
{
#ifdef CONFIG_OPTEE
	int ret;

	ret = optee_read_flash_lock_state(flash_lock_state);
	switch (ret) {
	case TEE_SUCCESS:
		break;
	case TEE_ERROR_GENERIC:
	case TEE_ERROR_NO_DATA:
	case TEE_ERROR_ITEM_NOT_FOUND:
		*flash_lock_state = 1;
		if (optee_write_flash_lock_state(*flash_lock_state)) {
			printf("optee_write_flash_lock_state error!");
			ret = AVB_IO_RESULT_ERROR_IO;
		} else {
			ret = optee_read_flash_lock_state(flash_lock_state);
		}
		break;
	default:
		printf("%s: optee_read_flash_lock_state failed\n", __FILE__);
	}

	return ret;
#else
	*flash_lock_state = 1;

	return AVB_IO_RESULT_OK;
#endif
}

AvbIOResult avb_write_flash_lock_state(uint8_t flash_lock_state)
{
#ifdef CONFIG_OPTEE
	if (optee_write_flash_lock_state(flash_lock_state)) {
		printf("optee_write_flash_lock_state error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_write_lock_state(uint8_t lock_state)
{
#ifdef CONFIG_OPTEE
	if (optee_write_lock_state(lock_state)) {
		printf("optee_write_lock_state error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_read_lock_state(uint8_t *lock_state)
{
#ifdef CONFIG_OPTEE
	uint8_t vboot_flag = 0;
	int ret;

	ret = optee_read_lock_state(lock_state);
	switch (ret) {
	case TEE_SUCCESS:
		break;
	case TEE_ERROR_GENERIC:
	case TEE_ERROR_NO_DATA:
	case TEE_ERROR_ITEM_NOT_FOUND:
		if (optee_read_vbootkey_enable_flag(&vboot_flag)) {
			printf("Can't read vboot flag\n");
			return AVB_IO_RESULT_ERROR_IO;
		}

		if (vboot_flag)
			*lock_state = 0;
		else
			*lock_state = 1;

		if (avb_write_lock_state(*lock_state)) {
			printf("avb_write_lock_state error!");
			ret = AVB_IO_RESULT_ERROR_IO;
		} else {
			ret = optee_read_lock_state(lock_state);
		}
		break;
	default:
		printf("%s: optee_read_lock_state failed\n", __FILE__);
	}

	return ret;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_write_permanent_attributes_flag(uint8_t flag)
{
#ifdef CONFIG_OPTEE
	if (optee_write_permanent_attributes_flag(flag)) {
		printf("optee_write_permanent_attributes_flag error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_read_permanent_attributes_flag(uint8_t *flag)
{
#ifdef CONFIG_OPTEE
	int ret;

	ret = optee_read_permanent_attributes_flag(flag);
	switch (ret) {
	case TEE_SUCCESS:
		break;
	case TEE_ERROR_GENERIC:
	case TEE_ERROR_NO_DATA:
	case TEE_ERROR_ITEM_NOT_FOUND:
		*flag = 0;
		if (avb_write_permanent_attributes_flag(*flag)) {
			printf("avb_write_permanent_attributes_flag error!");
			ret = AVB_IO_RESULT_ERROR_IO;
		} else {
			ret = optee_read_permanent_attributes_flag(flag);
		}
		break;
	default:
		printf("%s: optee_read_permanent_attributes_flag failed",
		       __FILE__);
	}

	return ret;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_read_vbootkey_hash(uint8_t *buf, uint8_t length)
{
#ifdef CONFIG_OPTEE
	if (optee_read_vbootkey_hash((uint32_t *)buf,
				      (uint32_t)length / sizeof(uint32_t))) {
		printf("optee_read_vbootkey_hash error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_write_vbootkey_hash(uint8_t *buf, uint8_t length)
{
#ifdef CONFIG_OPTEE
	if (optee_write_vbootkey_hash((uint32_t *)buf,
				       (uint32_t)length / sizeof(uint32_t))) {
		printf("optee_write_vbootkey_hash error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_close_optee_client(void)
{
#ifdef CONFIG_OPTEE
	if(optee_notify_uboot_end()) {
		printf("optee_notify_uboot_end error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_write_permanent_attributes_hash(uint8_t *buf, uint8_t length)
{
#ifdef CONFIG_OPTEE
	if (optee_write_attribute_hash((uint32_t *)buf,
	    (uint32_t)(length/sizeof(uint32_t)))) {
		printf("optee_write_attribute_hash error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_read_all_rollback_index(char *buffer)
{
	AvbOps* ops;
	uint64_t stored_rollback_index = 0;
	AvbIOResult io_ret;
	char temp[ROLLBACK_MAX_SIZE] = {0};

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	/* Actually the rollback_index_location 0 is used. */
	io_ret = ops->read_rollback_index(ops, 0, &stored_rollback_index);
	if (io_ret != AVB_IO_RESULT_OK)
		goto out;
	snprintf(temp, sizeof(int) + 1, "%d", 0);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ":", 1);
	snprintf(temp, sizeof(uint64_t) + 1, "%lld",
		 stored_rollback_index);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ",", 1);

	io_ret =
		ops->read_rollback_index(ops,
					 AVB_ATX_PIK_VERSION_LOCATION,
					 &stored_rollback_index);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("Failed to read PIK minimum version.\n");
		goto out;
	}
	/* PIK rollback index */
	snprintf(temp, sizeof(int) + 1, "%d", AVB_ATX_PIK_VERSION_LOCATION);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ":", 1);
	snprintf(temp, sizeof(uint64_t) + 1, "%lld", stored_rollback_index);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ",", 1);
	io_ret = ops->read_rollback_index(ops,
					  AVB_ATX_PSK_VERSION_LOCATION,
					  &stored_rollback_index);
	if (io_ret != AVB_IO_RESULT_OK) {
		printf("Failed to read PSK minimum version.\n");
		goto out;
	}
	/* PSK rollback index */
	snprintf(temp, sizeof(int) + 1, "%d", AVB_ATX_PSK_VERSION_LOCATION);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ":", 1);
	snprintf(temp, sizeof(uint64_t) + 1, "%lld", stored_rollback_index);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	debug("%s\n", buffer);
	avb_ops_user_free(ops);

	return AVB_IO_RESULT_OK;
out:
	avb_ops_user_free(ops);

	return AVB_IO_RESULT_ERROR_IO;
}

AvbIOResult avb_get_state(char *buf)
{
	char temp_flag = 0;
	char *lock_val = NULL;
	char *unlock_dis_val = NULL;
	char *permanent_attributes_flag = NULL;
	char *bootloader_locked_flag = NULL;
	char *rollback_indices;
	char min_versions[ROLLBACK_MAX_SIZE + 1] = {0};
	int n;

	if (avb_read_permanent_attributes_flag((uint8_t *)&temp_flag)) {
		printf("Can not read permanent_attributes_flag!");
		permanent_attributes_flag = "";
	} else {
		permanent_attributes_flag = temp_flag ? "1" : "0";
	}

	temp_flag = 0;
	if (avb_read_lock_state((uint8_t *)&temp_flag)) {
		printf("Can not read lock state!");
		lock_val = "";
		unlock_dis_val = "";
	} else {
		lock_val = (temp_flag & LOCK_MASK) ? "0" : "1";
		unlock_dis_val = (temp_flag & DISABLE_UNLOCK_MASK) ? "1" : "0";
	}

	temp_flag = 0;
	if (optee_read_vbootkey_enable_flag((uint8_t *)&temp_flag)) {
		printf("Can not read bootloader locked flag!");
		bootloader_locked_flag = "";
	} else {
		bootloader_locked_flag = temp_flag ? "1" : "0";
	}

	rollback_indices = malloc(AVB_STATE_SIZE);
	if (!rollback_indices) {
		printf("No buff to malloc!");
		return AVB_IO_RESULT_ERROR_OOM;
	}

	memset(rollback_indices, 0, AVB_STATE_SIZE);
	if (avb_read_all_rollback_index(rollback_indices))
		printf("Can not read avb_min_ver!");

	n = snprintf(buf, AVB_STATE_SIZE - 1,
		     "avb-perm-attr-set=%s\n"
		     "avb-locked=%s\n"
		     "avb-unlock-disabled=%s\n"
		     "bootloader-locked=%s\n"
		     "avb-min-versions=%s\n"
		     "bootloader-min-versions=%s\n",
		     permanent_attributes_flag,
		     lock_val,
		     unlock_dis_val,
		     bootloader_locked_flag,
		     rollback_indices,
		     min_versions);
	if (n >= AVB_STATE_SIZE) {
		printf("The VBOOT_STATE buf is truncated\n");
		buf[AVB_STATE_SIZE - 1] = 0;
	}
	debug("The avb state buf is %s\n", buf);
	free(rollback_indices);

	return AVB_IO_RESULT_OK;
}

int avb_auth_unlock(void *buffer, char *out_is_trusted)
{
	AvbOps* ops;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!");
		return -1;
	}

	if (avb_atx_validate_unlock_credential(ops->atx_ops,
					       (AvbAtxUnlockCredential*)buffer,
					       (bool*)out_is_trusted)) {
		avb_ops_user_free(ops);
		return -1;
	}
	avb_ops_user_free(ops);
	if (*out_is_trusted == true)
		return 0;
	else
		return -1;
}

int avb_generate_unlock_challenge(void *buffer, uint32_t *challenge_len)
{
	AvbOps* ops;
	AvbIOResult result = AVB_IO_RESULT_OK;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!");
		return -1;
	}

	result = avb_atx_generate_unlock_challenge(ops->atx_ops,
						   (AvbAtxUnlockChallenge *)buffer);
	avb_ops_user_free(ops);
	*challenge_len = sizeof(AvbAtxUnlockChallenge);
	if (result == AVB_IO_RESULT_OK)
		return 0;
	else
		return -1;
}

int avb_read_permanent_attributes_all(u16 id, void *pbuf, u16 size)
{
	AvbOps* ops;
	int ret = 0;

	debug("%s %d\n", __func__, size);

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!");
		return -1;
	}

	switch (id) {
	case AT_PERM_ATTR_FUSE:
		size = sizeof(AvbAtxPermanentAttributes);
		ret = avb_read_permanent_attributes(ops->atx_ops,
						    (AvbAtxPermanentAttributes *)pbuf);
		break;
	case AT_PERM_ATTR_CER_FUSE:
		size = RK_AVB_PERM_ATTR_CER_SIZE;
		ret = avb_get_permanent_attributes_cer((uint8_t *)pbuf, size);
		break;
	case AT_LOCK_VBOOT:
		break;
	}

	avb_ops_user_free(ops);

	/* return bytes when operations all succeed. */
	if (!ret)
		ret = size;

	return ret;
}

int avb_write_permanent_attributes_all(u16 id, void *pbuf, u16 size)
{
	AvbOps* ops;
	uint8_t lock_state;
#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
	sha256_context ctx;
	uint8_t digest[SHA256_SUM_LEN] = {0};
	uint8_t digest_temp[SHA256_SUM_LEN] = {0};
	uint8_t permanent_attributes_temp[PERM_ATTR_TOTAL_SIZE] = {0};
	uint8_t flag = 0;
#endif
	int ret = 0;

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!");
		return -1;
	}

	switch (id) {
	case AT_PERM_ATTR_FUSE:
		if (size != PERM_ATTR_TOTAL_SIZE) {
			debug("%s Permanent attribute size is not equal!\n", __func__);
			ret = -EINVAL;
			goto out;
		}

#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
		if (avb_read_permanent_attributes_flag(&flag)) {
			debug("%s avb_read_permanent_attributes_flag error!\n", __func__);
			ret = -EIO;
			goto out;
		}

		if (flag == PERM_ATTR_SUCCESS_FLAG) {
			if (avb_read_permanent_attributes_hash(ops->atx_ops,
							       digest_temp)) {
				debug("%s The efuse IO can not be used!\n", __func__);
				ret = -EIO;
				goto out;
			}

			if (memcmp(digest, digest_temp, SHA256_SUM_LEN) != 0) {
				if (avb_read_permanent_attributes(ops->atx_ops,
								  (AvbAtxPermanentAttributes *)permanent_attributes_temp)) {
					debug("%s avb_read_permanent_attributes error!\n", __func__);
					ret = -EIO;
					goto out;
				}

				sha256_starts(&ctx);
				sha256_update(&ctx,
					      (const uint8_t *)permanent_attributes_temp,
					      PERM_ATTR_TOTAL_SIZE);
				sha256_finish(&ctx, digest);
				if (memcmp(digest, digest_temp, SHA256_SUM_LEN) == 0) {
					debug("%s The hash has been written!\n", __func__);
					ret = 0;
					goto out;
				}
			}

			if (avb_write_permanent_attributes_flag(0)) {
				debug("%s permanent attributes flag write failure\n", __func__);
				ret = -EIO;
				goto out;
			}
		}
#endif
		if (avb_write_permanent_attributes(ops->atx_ops,
						   (AvbAtxPermanentAttributes *)pbuf)) {
			if (avb_write_permanent_attributes_flag(0)) {
				debug("%s permanent attributes flag write failure\n", __func__);
				ret = -EIO;
				goto out;
			}

			debug("%s permanent attributes write failed\n", __func__);
			ret = -EIO;
			goto out;
		}
#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
		memset(digest, 0, SHA256_SUM_LEN);
		sha256_starts(&ctx);
		sha256_update(&ctx, (const uint8_t *)pbuf,
			      PERM_ATTR_TOTAL_SIZE);
		sha256_finish(&ctx, digest);

		if (avb_write_permanent_attributes_hash((uint8_t *)digest,
							SHA256_SUM_LEN)) {
			if (avb_read_permanent_attributes_hash(ops->atx_ops,
							       digest_temp)) {
				debug("%s The efuse IO can not be used!\n", __func__);
				ret = -EIO;
				goto out;
			}

			if (memcmp(digest, digest_temp, SHA256_SUM_LEN) != 0) {
				if (avb_write_permanent_attributes_flag(0)) {
					debug("%s permanent attributes flag write failure\n", __func__);
					ret = -EIO;
					goto out;
				}
				debug("%s The hash has been written, but is different!\n", __func__);
				ret = -EIO;
				goto out;
			}
		}
#endif
		if (avb_write_permanent_attributes_flag(PERM_ATTR_SUCCESS_FLAG)) {
			debug("%s, permanent attributes flag write failure\n", __func__);
			ret = -EIO;
			goto out;
		}

		break;
	case AT_PERM_ATTR_CER_FUSE:
		if (size != RK_AVB_PERM_ATTR_CER_SIZE) {
			debug("%s Permanent attribute rsahash size is not equal!\n",
			      __func__);
			ret = -EINVAL;
			goto out;
		}
		if (avb_set_permanent_attributes_cer((uint8_t *)pbuf, size)) {
			debug("%s Set perm attr cer fail!\n", __func__);
			ret = -EIO;
			goto out;
		}
		break;
	case AT_LOCK_VBOOT:
		lock_state = 0;
		if (avb_write_lock_state(lock_state)) {
			debug("%s Write lock state failed\n", __func__);
			ret = -EIO;
			goto out;
		} else {
			debug("%s OKAY\n", __func__);
		}
		break;
	}

out:
	avb_ops_user_free(ops);

	return ret;
}
