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
#include "avb.h"
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

AvbIOResult avb_get_perm_attr_cer(uint8_t *cer, uint32_t size)
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

AvbIOResult avb_set_perm_attr_cer(uint8_t *cer, uint32_t size)
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

AvbIOResult avb_write_perm_attr_flag(uint8_t flag)
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

AvbIOResult avb_read_perm_attr_flag(uint8_t *flag)
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
		if (avb_write_perm_attr_flag(*flag)) {
			printf("avb_write_perm_attr_flag error!");
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

AvbIOResult avb_write_attribute_hash(uint8_t *buf, uint8_t length)
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

#ifdef CONFIG_SUPPORT_EMMC_RPMB
static int curr_device = -1;

int rpmb_rollback_index_read(uint32_t offset, uint32_t bytes,
				      void *rb_index)
{

	struct mmc *mmc;
	uint8_t rpmb_buf[256] = {0};
	uint32_t n;
	char original_part;

	if ((offset + bytes) > 256)
		return -1;

	if (curr_device < 0) {
		if (get_mmc_num() > 0)
			curr_device = 0;
		else {
			printf("No MMC device available");
			return -1;
		}
	}

	mmc = find_mmc_device(curr_device);
	/* Switch to the RPMB partition */
#ifndef CONFIG_BLK
	original_part = mmc->block_dev.hwpart;
#else
	original_part = mmc_get_blk_desc(mmc)->hwpart;
#endif
	if (blk_select_hwpart_devnum(UCLASS_MMC, curr_device, MMC_PART_RPMB) !=
	    0)
		return -1;

	n =  mmc_rpmb_read(mmc, rpmb_buf, RPMB_BASE_ADDR, 1, NULL);
	if (n != 1)
		return -1;

	/* Return to original partition */
	if (blk_select_hwpart_devnum(UCLASS_MMC, curr_device, original_part) !=
	    0)
		return -1;

	memcpy(rb_index, (void*)&rpmb_buf[offset], bytes);

	return 0;
}

int avb_get_bootloader_min_version(char *buffer)
{
	uint32_t rb_index;
	char temp[ROLLBACK_MAX_SIZE] = {0};

	if (rpmb_rollback_index_read(UBOOT_RB_INDEX_OFFSET,
					      sizeof(uint32_t), &rb_index)) {
		printf("Can not read uboot rollback index");
		return -1;
	}
	snprintf(temp, sizeof(int) + 1, "%d", 0);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ":", 1);
	snprintf(temp, sizeof(uint32_t) + 1, "%d", rb_index);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ",", 1);

	if (rpmb_rollback_index_read(TRUST_RB_INDEX_OFFSET,
					      sizeof(uint32_t), &rb_index)) {
		printf("Can not read trust rollback index");
		return -1;
	}

	snprintf(temp, sizeof(int) + 1, "%d", 1);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);
	strncat(buffer, ":", 1);
	snprintf(temp, sizeof(uint32_t) + 1, "%d", rb_index);
	strncat(buffer, temp, ROLLBACK_MAX_SIZE);

	return 0;
}
#endif

AvbIOResult avb_get_at_vboot_state(char *buf)
{
	char temp_flag = 0;
	char *lock_val = NULL;
	char *unlock_dis_val = NULL;
	char *perm_attr_flag = NULL;
	char *bootloader_locked_flag = NULL;
	char *rollback_indices;
	char min_versions[ROLLBACK_MAX_SIZE + 1] = {0};
	int n;

	if (avb_read_perm_attr_flag((uint8_t *)&temp_flag)) {
		printf("Can not read perm_attr_flag!");
		perm_attr_flag = "";
	} else {
		perm_attr_flag = temp_flag ? "1" : "0";
	}

	temp_flag = 0;
	if (avb_read_lock_state((uint8_t *)&temp_flag)) {
		printf("Can not read lock state!");
		lock_val = "";
		unlock_dis_val = "";
	} else {
		lock_val = (temp_flag & LOCK_MASK) ? "0" : "1";
		unlock_dis_val = (temp_flag & UNLOCK_DISABLE_MASK) ? "1" : "0";
	}

	temp_flag = 0;
	if (optee_read_vbootkey_enable_flag((uint8_t *)&temp_flag)) {
		printf("Can not read bootloader locked flag!");
		bootloader_locked_flag = "";
	} else {
		bootloader_locked_flag = temp_flag ? "1" : "0";
	}

	rollback_indices = malloc(VBOOT_STATE_SIZE);
	if (!rollback_indices) {
		printf("No buff to malloc!");
		return AVB_IO_RESULT_ERROR_OOM;
	}

	memset(rollback_indices, 0, VBOOT_STATE_SIZE);
	if (avb_read_all_rollback_index(rollback_indices))
		printf("Can not read avb_min_ver!");
#ifdef CONFIG_SUPPORT_EMMC_RPMB
	/* bootloader-min-versions */
	if (avb_get_bootloader_min_version(min_versions))
		printf("Call avb_get_bootloader_min_version error!");
#endif
	n = snprintf(buf, VBOOT_STATE_SIZE - 1,
		     "avb-perm-attr-set=%s\n"
		     "avb-locked=%s\n"
		     "avb-unlock-disabled=%s\n"
		     "bootloader-locked=%s\n"
		     "avb-min-versions=%s\n"
		     "bootloader-min-versions=%s\n",
		     perm_attr_flag,
		     lock_val,
		     unlock_dis_val,
		     bootloader_locked_flag,
		     rollback_indices,
		     min_versions);
	if (n >= VBOOT_STATE_SIZE) {
		printf("The VBOOT_STATE buf is truncated\n");
		buf[VBOOT_STATE_SIZE - 1] = 0;
	}
	debug("The vboot state buf is %s\n", buf);
	free(rollback_indices);

	return AVB_IO_RESULT_OK;
}

AvbIOResult avb_write_permanent_attributes(uint8_t *attributes, uint32_t size)
{
#ifndef CONFIG_ROCKCHIP_PRELOADER_PUB_KEY
	sha256_context ctx;
	uint8_t digest[SHA256_SUM_LEN] = {0};
	uint8_t digest_temp[SHA256_SUM_LEN] = {0};
	AvbAtxPermanentAttributes perm_attr_temp[PERM_ATTR_TOTAL_SIZE] = {0};
	uint8_t flag = 0;
	AvbOps* ops;
#endif

	if (size != PERM_ATTR_TOTAL_SIZE) {
		debug("%s Permanent attribute size is not equal!\n", __func__);
		return AVB_IO_RESULT_ERROR_IO;
	}

#ifndef CONFIG_ROCKCHIP_PRELOADER_PUB_KEY
	if (avb_read_perm_attr_flag(&flag)) {
		debug("%s avb_read_perm_attr_flag error!\n", __func__);
		return AVB_IO_RESULT_ERROR_IO;
	}

	ops = avb_ops_user_new();
	if (ops == NULL) {
		printf("avb_ops_user_new() failed!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	if (flag == PERM_ATTR_SUCCESS_FLAG) {
		if (ops->atx_ops->read_permanent_attributes_hash(ops->atx_ops, digest_temp)) {
			debug("%s The efuse IO can not be used!\n", __func__);
			return AVB_IO_RESULT_ERROR_IO;
		}

		if (memcmp(digest, digest_temp, SHA256_SUM_LEN) != 0) {
			if (ops->atx_ops->read_permanent_attributes(ops->atx_ops, perm_attr_temp)) {
				debug("%s avb_read_permanent_attributes error!\n", __func__);
				return AVB_IO_RESULT_ERROR_IO;
			}

			sha256_starts(&ctx);
			sha256_update(&ctx,
						(const uint8_t *)perm_attr_temp,
						PERM_ATTR_TOTAL_SIZE);
			sha256_finish(&ctx, digest);
			if (memcmp(digest, digest_temp, SHA256_SUM_LEN) == 0) {
				debug("%s The hash has been written!\n", __func__);
				return AVB_IO_RESULT_OK;
			}
		}

		if (avb_write_perm_attr_flag(0)) {
			debug("%s Perm attr flag write failure\n", __func__);
			return AVB_IO_RESULT_ERROR_IO;
		}
	}
#endif
	if (optee_write_permanent_attributes(attributes, size)) {
		if (avb_write_perm_attr_flag(0)) {
			debug("%s Perm attr flag write failure\n", __func__);
			return AVB_IO_RESULT_ERROR_IO;
		}

		debug("%s Perm attr write failed\n", __func__);
		return AVB_IO_RESULT_ERROR_IO;
	}
#ifndef CONFIG_ROCKCHIP_PRELOADER_PUB_KEY
	memset(digest, 0, SHA256_SUM_LEN);
	sha256_starts(&ctx);
	sha256_update(&ctx, attributes,
				PERM_ATTR_TOTAL_SIZE);
	sha256_finish(&ctx, digest);

	if (avb_write_attribute_hash((uint8_t *)digest,
					SHA256_SUM_LEN)) {
		if (ops->atx_ops->read_permanent_attributes_hash(ops->atx_ops, digest_temp)) {
			debug("%s The efuse IO can not be used!\n", __func__);
			return AVB_IO_RESULT_ERROR_IO;
		}

		if (memcmp(digest, digest_temp, SHA256_SUM_LEN) != 0) {
			if (avb_write_perm_attr_flag(0)) {
				debug("%s Perm attr flag write failure\n", __func__);
				return AVB_IO_RESULT_ERROR_IO;
			}
			debug("%s The hash has been written, but is different!\n", __func__);
			return AVB_IO_RESULT_ERROR_IO;
		}
	}
	avb_ops_user_free(ops);
#endif
	if (avb_write_perm_attr_flag(PERM_ATTR_SUCCESS_FLAG)) {
		debug("%s, Perm attr flag write failure\n", __func__);
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
}
