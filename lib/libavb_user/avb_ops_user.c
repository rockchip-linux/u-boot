/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * This file is mainly from avb_ops_user.cpp by Google.
 * Don't add unnecessary functions to this file.
 */
#include <common.h>
#include <image.h>
#include <android_image.h>
#include <malloc.h>
#include <mapmem.h>
#include <errno.h>
#include <command.h>
#include <mmc.h>
#include <blk.h>
#include <part.h>
#include <stdio.h>
#include <rand.h>
#include <time.h>
#include <tee.h>
#include <tee/optee.h>
#include <asm/cache.h>
#include <android_avb/libavb.h>
#include <android_avb/libavb_ab.h>
#include <android_avb/libavb_atx.h>
#include <android_avb/libavb_user.h>
#include <avb_verify.h>

/* Refer from avb_ops_user.cpp */
static void byte_to_block(int64_t *offset,
			  size_t *num_bytes,
			  lbaint_t *offset_blk,
			  lbaint_t *blkcnt)
{
	*offset_blk = (lbaint_t)(*offset / 512);
	if (*num_bytes % 512 == 0) {
		if (*offset % 512 == 0)
			*blkcnt = (lbaint_t)(*num_bytes / 512);
		else
			*blkcnt = (lbaint_t)(*num_bytes / 512) + 1;
	} else {
		if (*offset % 512 == 0) {
			*blkcnt = (lbaint_t)(*num_bytes / 512) + 1;
		} else {
			if ((*offset % 512) + (*num_bytes % 512) < 512 ||
			    (*offset % 512) + (*num_bytes % 512) == 512) {
				*blkcnt = (lbaint_t)(*num_bytes / 512) + 1;
			} else {
				*blkcnt = (lbaint_t)(*num_bytes / 512) + 2;
			}
		}
	}
}

static AvbIOResult get_size_of_partition(AvbOps *ops,
					 const char *partition,
					 uint64_t *out_size_in_bytes)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0)
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

	*out_size_in_bytes = (part_info.size) * 512;
	return AVB_IO_RESULT_OK;
}

static AvbIOResult read_from_partition(AvbOps *ops,
				       const char *partition,
				       int64_t offset,
				       size_t num_bytes,
				       void *buffer,
				       size_t *out_num_read)
{
	struct blk_desc *dev_desc;
	lbaint_t offset_blk, blkcnt;
	struct disk_partition part_info;
	uint64_t partition_size;

	if (offset < 0) {
		if (get_size_of_partition(ops, partition, &partition_size))
			return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;

		if (-offset > partition_size)
			return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;

		offset = partition_size - (-offset);
	}

	byte_to_block(&offset, &num_bytes, &offset_blk, &blkcnt);
	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		printf("Could not find \"%s\" partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if ((offset % 512 == 0) && (num_bytes % 512 == 0)) {
		blk_dread(dev_desc, part_info.start + offset_blk,
			  blkcnt, buffer);
		*out_num_read = blkcnt * 512;
	} else {
		char *buffer_temp;

		buffer_temp = malloc(512 * blkcnt);
		if (!buffer_temp) {
			printf("malloc error!\n");
			return AVB_IO_RESULT_ERROR_OOM;
		}
		blk_dread(dev_desc, part_info.start + offset_blk,
			  blkcnt, buffer_temp);
		memcpy(buffer, buffer_temp + (offset % 512), num_bytes);
		*out_num_read = num_bytes;
		free(buffer_temp);
	}

	return AVB_IO_RESULT_OK;
}

static AvbIOResult write_to_partition(AvbOps *ops,
				      const char *partition,
				      int64_t offset,
				      size_t num_bytes,
				      const void *buffer)
{
	struct blk_desc *dev_desc;
	char *buffer_temp;
	struct disk_partition part_info;
	lbaint_t offset_blk, blkcnt;

	byte_to_block(&offset, &num_bytes, &offset_blk, &blkcnt);
	buffer_temp = malloc(512 * blkcnt);
	if (!buffer_temp) {
		printf("malloc error!\n");
		return AVB_IO_RESULT_ERROR_OOM;
	}
	memset(buffer_temp, 0, 512 * blkcnt);
	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		printf("Could not find \"%s\" partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if ((offset % 512 != 0) && (num_bytes % 512) != 0)
		blk_dread(dev_desc, part_info.start + offset_blk,
			  blkcnt, buffer_temp);

	memcpy(buffer_temp, buffer + (offset % 512), num_bytes);
	blk_dwrite(dev_desc, part_info.start + offset_blk, blkcnt, buffer);
	free(buffer_temp);

	return AVB_IO_RESULT_OK;
}

static AvbIOResult validate_vbmeta_public_key(AvbOps *ops,
			   const uint8_t *public_key_data,
			   size_t public_key_length,
			   const uint8_t *public_key_metadata,
			   size_t public_key_metadata_length,
			   bool *out_is_trusted)
{
#if defined(CONFIG_LIBAVB_VBMETA_PUBLIC_KEY_VALIDATE)
	if (out_is_trusted) {
		avb_atx_validate_vbmeta_public_key(ops,
						   public_key_data,
						   public_key_length,
						   public_key_metadata,
						   public_key_metadata_length,
						   out_is_trusted);
	}
#else
	if (out_is_trusted)
		*out_is_trusted = true;
#endif
	return AVB_IO_RESULT_OK;
}

static AvbIOResult read_rollback_index(AvbOps *ops,
				       size_t rollback_index_location,
				       uint64_t *out_rollback_index)
{
	if (out_rollback_index) {
#ifdef CONFIG_OPTEE
		int ret;

		ret = optee_read_rollback_index(rollback_index_location,
						 out_rollback_index);
		switch (ret) {
		case TEE_SUCCESS:
			ret = AVB_IO_RESULT_OK;
			break;
		case TEE_ERROR_GENERIC:
		case TEE_ERROR_NO_DATA:
		case TEE_ERROR_ITEM_NOT_FOUND:
			*out_rollback_index = 0;
			ret = optee_write_rollback_index(rollback_index_location,
							  *out_rollback_index);
			if (ret) {
				printf("%s: init rollback index error\n",
				       __FILE__);
				ret = AVB_IO_RESULT_ERROR_IO;
			} else {
				ret =
				optee_read_rollback_index(rollback_index_location,
							   out_rollback_index);
				if (ret)
					ret = AVB_IO_RESULT_ERROR_IO;
				else
					ret = AVB_IO_RESULT_OK;
			}
			break;
		default:
			ret = AVB_IO_RESULT_ERROR_IO;
			printf("%s: optee_read_rollback_index failed",
			       __FILE__);
		}

		return ret;
#else
		*out_rollback_index = 0;

		return AVB_IO_RESULT_OK;
#endif
	}

	return AVB_IO_RESULT_ERROR_IO;
}

static AvbIOResult write_rollback_index(AvbOps *ops,
					size_t rollback_index_location,
					uint64_t rollback_index)
{
#ifdef CONFIG_OPTEE
	if (optee_write_rollback_index(rollback_index_location,
					rollback_index)) {
		printf("%s: Fail to write rollback index\n", __FILE__);
		return AVB_IO_RESULT_ERROR_IO;
	}
	return AVB_IO_RESULT_OK;
#endif
	return AVB_IO_RESULT_ERROR_IO;
}

static AvbIOResult read_is_device_unlocked(AvbOps *ops, bool *out_is_unlocked)
{
	if (out_is_unlocked) {
#ifdef CONFIG_OPTEE
		uint8_t vboot_flag = 0;
		int ret;

		ret = optee_read_lock_state((uint8_t *)out_is_unlocked);
		switch (ret) {
		case TEE_SUCCESS:
			ret = AVB_IO_RESULT_OK;
			break;
		case TEE_ERROR_GENERIC:
		case TEE_ERROR_NO_DATA:
		case TEE_ERROR_ITEM_NOT_FOUND:
			if (optee_read_vbootkey_enable_flag(&vboot_flag)) {
				printf("Can't read vboot flag\n");
				return AVB_IO_RESULT_ERROR_IO;
			}

			if (vboot_flag)
				*out_is_unlocked = 0;
			else
				*out_is_unlocked = 1;

			if (optee_write_lock_state(*out_is_unlocked)) {
				printf("%s: init lock state error\n", __FILE__);
				ret = AVB_IO_RESULT_ERROR_IO;
			} else {
				ret =
				optee_read_lock_state((uint8_t *)out_is_unlocked);
				if (ret == 0)
					ret = AVB_IO_RESULT_OK;
				else
					ret = AVB_IO_RESULT_ERROR_IO;
			}
			break;
		default:
			ret = AVB_IO_RESULT_ERROR_IO;
			printf("%s: optee_read_lock_state failed\n", __FILE__);
		}
		return ret;
#else
		*out_is_unlocked = 1;

		return AVB_IO_RESULT_OK;
#endif
	}
	return AVB_IO_RESULT_ERROR_IO;
}

AvbIOResult write_is_device_unlocked(AvbOps *ops, bool *out_is_unlocked)
{
	if (out_is_unlocked) {
#ifdef CONFIG_OPTEE
		if (optee_write_lock_state(*out_is_unlocked)) {
			printf("%s: Fail to write lock state\n", __FILE__);
			return AVB_IO_RESULT_ERROR_IO;
		}
		return AVB_IO_RESULT_OK;
#endif
	}
	return AVB_IO_RESULT_ERROR_IO;
}

static AvbIOResult get_unique_guid_for_partition(AvbOps *ops,
						 const char *partition,
						 char *guid_buf,
						 size_t guid_buf_size)
{
	struct blk_desc *dev_desc;
	struct disk_partition part_info;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		printf("Could not find \"%s\" partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}
	if (guid_buf && guid_buf_size > 0)
		memcpy(guid_buf, part_info.uuid, guid_buf_size);

	return AVB_IO_RESULT_OK;
}

/*
 * RK-defined ops
 * 1. Have statement but no implementation.
 * 2. Both statement & implementation don't exist.
 */
AvbIOResult validate_public_key_for_partition(AvbOps *ops,
					      const char *partition,
					      const uint8_t *public_key_data,
					      size_t public_key_length,
					      const uint8_t *public_key_metadata,
					      size_t public_key_metadata_length,
					      bool *out_is_trusted,
					      uint32_t *out_rollback_index_location)
{
#if defined(CONFIG_LIBAVB_VBMETA_PUBLIC_KEY_VALIDATE)
	if (out_is_trusted) {
		avb_atx_validate_vbmeta_public_key(ops,
						   public_key_data,
						   public_key_length,
						   public_key_metadata,
						   public_key_metadata_length,
						   out_is_trusted);
	}
#else
	if (out_is_trusted)
		*out_is_trusted = true;
#endif
	*out_rollback_index_location = 0;
	return AVB_IO_RESULT_OK;
}

AvbIOResult avb_read_permanent_attributes(AvbAtxOps *atx_ops,
			       AvbAtxPermanentAttributes *attributes)
{
	if (!attributes)
		return AVB_IO_RESULT_ERROR_IO;
#ifdef CONFIG_OPTEE
	if(optee_read_permanent_attributes((void *)attributes, sizeof(struct AvbAtxPermanentAttributes))) {
		printf("optee_read_permanent_attributes failed!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_write_permanent_attributes(AvbAtxOps *atx_ops,
					   AvbAtxPermanentAttributes *attributes)
{
	if (!attributes)
		return AVB_IO_RESULT_ERROR_IO;
#ifdef CONFIG_OPTEE
	if(optee_write_permanent_attributes((void *)attributes, sizeof(struct AvbAtxPermanentAttributes))) {
		printf("optee_write_permanent_attributes failed!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	return AVB_IO_RESULT_ERROR_IO;
#endif
}

AvbIOResult avb_read_permanent_attributes_hash(AvbAtxOps *atx_ops,
					       uint8_t hash[AVB_SHA256_DIGEST_SIZE])
{
#ifndef CONFIG_LIBAVB_RK_PRELOADER_PUB_KEY
#ifdef CONFIG_OPTEE
	if (optee_read_attribute_hash((uint32_t *)hash,
				       AVB_SHA256_DIGEST_SIZE / 4)) {
		printf("optee_read_permanent_attributes_hash error!\n");
		return AVB_IO_RESULT_ERROR_IO;
	}

	return AVB_IO_RESULT_OK;
#else
	printf("Please open the macro CONFIG_OPTEE!\n");
	return AVB_IO_RESULT_ERROR_IO;
#endif
#else
	printf("No perm attr hash for efuse platforms!\n");

	return AVB_IO_RESULT_ERROR_IO;
#endif
}

static void avb_set_key_version(AvbAtxOps *atx_ops,
				size_t rollback_index_location,
				uint64_t key_version)
{
#ifdef CONFIG_OPTEE
	uint64_t key_version_temp = 0;

	if (optee_read_rollback_index(rollback_index_location, &key_version_temp))
		printf("%s: Fail to read rollback index\n", __FILE__);
	if (key_version_temp >= key_version)
		return;
	if (optee_write_rollback_index(rollback_index_location, key_version))
		printf("%s: Fail to write rollback index\n", __FILE__);
#endif
}

static AvbIOResult get_random(AvbAtxOps *atx_ops,
			  size_t num_bytes,
			  uint8_t *output)
{
	int i;
	unsigned int seed;

	seed = (unsigned int)get_timer(0);
	for (i = 0; i < num_bytes; i++) {
		srand(seed);
		output[i] = (uint8_t)(rand());
		seed = (unsigned int)(output[i]);
	}

	return 0;
}

static AvbIOResult get_preloaded_partition(AvbOps* ops,
					   const char* partition,
					   size_t num_bytes,
					   uint8_t** out_pointer,
					   size_t* out_num_bytes_preloaded,
					   int allow_verification_error)
{
	struct preloaded_partition *preload_info = NULL;
	struct AvbOpsData *data = ops->user_data;
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
	ulong load_addr;
	AvbIOResult ret;
	int full_preload = 0;

	dev_desc = plat_bootdev();
	if (!dev_desc)
		return AVB_IO_RESULT_ERROR_IO;

	if (part_get_info_by_name(dev_desc, partition, &part_info) < 0) {
		printf("Could not find \"%s\" partition\n", partition);
		return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
	}

	if (!allow_verification_error) {
		if (!strncmp(partition, ANDROID_PARTITION_BOOT, 4) ||
		    !strncmp(partition, ANDROID_PARTITION_RECOVERY, 8))
			preload_info = &data->boot;
		else if (!strncmp(partition, ANDROID_PARTITION_VENDOR_BOOT, 11))
			preload_info = &data->vendor_boot;
		else if (!strncmp(partition, ANDROID_PARTITION_INIT_BOOT, 9))
			preload_info = &data->init_boot;
		else if (!strncmp(partition, ANDROID_PARTITION_RESOURCE, 8))
			preload_info = &data->resource;

		if (!preload_info) {
			printf("Error: unknown full load partition '%s'\n", partition);
			return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
		}

		printf("preloaded(s): %sfull image from '%s' at 0x%08lx - 0x%08lx\n",
		       preload_info->size ? "pre-" : "", partition,
		       (ulong)preload_info->addr,
		       (ulong)preload_info->addr + num_bytes);

		/* If the partition hasn't yet been preloaded, do it now.*/
		if (preload_info->size == 0) {
			ret = ops->read_from_partition(ops, partition,
						       0, num_bytes,
						       preload_info->addr,
						       &preload_info->size);
			if (ret != AVB_IO_RESULT_OK)
				return ret;
		}
		*out_pointer = preload_info->addr;
		*out_num_bytes_preloaded = preload_info->size;
		ret = AVB_IO_RESULT_OK;
	} else {
		if (!strncmp(partition, ANDROID_PARTITION_INIT_BOOT, 9) ||
		    !strncmp(partition, ANDROID_PARTITION_VENDOR_BOOT, 11) ||
		    !strncmp(partition, ANDROID_PARTITION_BOOT, 4) ||
		    !strncmp(partition, ANDROID_PARTITION_RECOVERY, 8) ||
		    !strncmp(partition, ANDROID_PARTITION_RESOURCE, 8)) {
			/* If already full preloaded, just use it */
			if (!strncmp(partition, ANDROID_PARTITION_BOOT, 4) ||
			    !strncmp(partition, ANDROID_PARTITION_RECOVERY, 8)) {
				preload_info = &data->boot;
				if (preload_info->size) {
					*out_pointer = preload_info->addr;
					*out_num_bytes_preloaded = num_bytes;
					full_preload = 1;
				}
			}
			printf("preloaded: %s image from '%s\n",
			       full_preload ? "pre-full" : "distribute", partition);
		} else {
			printf("Error: unknown preloaded partition '%s'\n", partition);
			return AVB_IO_RESULT_ERROR_OOM;
		}

		/*
		 * Already preloaded during boot/recovery loading,
		 * here we just return a dummy buffer.
		 */
		if (!strncmp(partition, ANDROID_PARTITION_INIT_BOOT, 9) ||
		    !strncmp(partition, ANDROID_PARTITION_VENDOR_BOOT, 11) ||
		    !strncmp(partition, ANDROID_PARTITION_RESOURCE, 8)) {
			*out_pointer = (u8 *)avb_malloc(ARCH_DMA_MINALIGN);
			*out_num_bytes_preloaded = num_bytes; /* return what it expects */
			return AVB_IO_RESULT_OK;
		}

		/* If already full preloaded, there is nothing to do and just return */
		if (full_preload)
			return AVB_IO_RESULT_OK;

		/*
		 * only boot/recovery partition can reach here
		 * and init/vendor_boot are loaded at this round.
		 */
		load_addr = env_get_ulong("kernel_addr_r", 16, 0);
		if (!load_addr)
			return AVB_IO_RESULT_ERROR_NO_SUCH_VALUE;

		ret = android_image_load_by_partname(dev_desc, partition, &load_addr);
		if (!ret) {
			*out_pointer = (u8 *)load_addr;
			*out_num_bytes_preloaded = num_bytes; /* return what it expects */
			ret = AVB_IO_RESULT_OK;
		} else {
			ret = AVB_IO_RESULT_ERROR_IO;
		}
	}

	return ret;
}

AvbOps *avb_ops_user_new(void)
{
	AvbOps *ops = NULL;
	struct AvbOpsData *ops_data = NULL;

	ops = calloc(1, sizeof(AvbOps));
	if (!ops) {
		printf("Error allocating memory for AvbOps.\n");
		goto out;
	}
	ops->ab_ops = calloc(1, sizeof(AvbABOps));
	if (!ops->ab_ops) {
		printf("Error allocating memory for AvbABOps.\n");
		free(ops);
		goto out;
	}

	ops->atx_ops = calloc(1, sizeof(AvbAtxOps));
	if (!ops->atx_ops) {
		printf("Error allocating memory for AvbAtxOps.\n");
		free(ops->ab_ops);
		free(ops);
		goto out;
	}

	ops_data = calloc(1, sizeof(struct AvbOpsData));
	if (!ops_data) {
		printf("Error allocating memory for AvbOpsData.\n");
		free(ops->atx_ops);
		free(ops->ab_ops);
		free(ops);
		goto out;
	}

	ops->ab_ops->ops = ops;
	ops->atx_ops->ops = ops;
	ops_data->ops = *ops;
	ops->user_data = ops_data;

	/* from libavb */
	ops->read_from_partition = read_from_partition;
	ops->validate_vbmeta_public_key = validate_vbmeta_public_key;
	ops->read_rollback_index = read_rollback_index;
	ops->write_rollback_index = write_rollback_index;
	ops->read_is_device_unlocked = read_is_device_unlocked;
	ops->get_unique_guid_for_partition = get_unique_guid_for_partition;
	ops->get_size_of_partition = get_size_of_partition;

	/* from libavb_ab */
#ifdef CONFIG_LIBAVB_AB
	ops->ab_ops->read_ab_metadata = avb_ab_data_read;
	ops->ab_ops->write_ab_metadata = avb_ab_data_write;
#endif
	/* from libavb_atx */
	ops->atx_ops->read_permanent_attributes = avb_read_permanent_attributes;
	ops->atx_ops->read_permanent_attributes_hash = avb_read_permanent_attributes_hash;
	ops->atx_ops->set_key_version = avb_set_key_version;
	ops->atx_ops->get_random = get_random;

	/* implemented by rk */
	ops->validate_public_key_for_partition = validate_public_key_for_partition;
	ops->get_preloaded_partition = get_preloaded_partition;
	ops->write_to_partition = write_to_partition;
	ops->write_is_device_unlocked = write_is_device_unlocked;

	return ops;
out:
	return NULL;
}

void avb_ops_user_free(AvbOps *ops)
{
	free(ops->user_data);
	free(ops->ab_ops);
	free(ops->atx_ops);
	free(ops);
}
