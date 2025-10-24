/*
 * Copyright 2025, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <stdlib.h>
#include <command.h>
#include <common.h>
#include <mmc.h>
#include <part.h>
#include <tee.h>
#include "../optee_msg.h"
#include "../optee_private.h"
#include "optee_load_ta.h"

int is_uuid_equal(struct tee_optee_ta_uuid uuid1, struct tee_optee_ta_uuid uuid2)
{
	bool a, b, c;

	a = (uuid1.time_low == uuid2.time_low);
	b = (uuid1.time_mid == uuid2.time_mid);
	c = (uuid1.time_hi_and_version == uuid2.time_hi_and_version);
	if ((a & b & c) == 0) {
		return 0;
	} else {
		if (memcmp(uuid1.clock_seq_and_node,
			   uuid2.clock_seq_and_node, 8) == 0) {
			return 1;
		} else {
			return 0;
		}
	}
}

void tee_uuid_from_octets(struct tee_optee_ta_uuid *d, const uint8_t *s)
{
	d->time_low = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
	d->time_mid = (s[4] << 8) | s[5];
	d->time_hi_and_version = (s[6] << 8) | s[7];
	memcpy(d->clock_seq_and_node, s + 8, sizeof(d->clock_seq_and_node));
}

static struct blk_desc *dev_desc;
static struct disk_partition part_info;

int search_ta(void *uuid_octets, void *ta, size_t *ta_size)
{
	char fname[255];
	char *format;
	unsigned long ret = 0;
	struct tee_optee_ta_uuid uuid;
	struct tee_optee_ta_uuid ta_uuid;
	uint8_t *userta;
	struct userta_header *header;
	struct userta_item *item;
	int ta_ver;
	int res;

	if (!uuid_octets || !ta_size) {
		printf("TEEC: wrong parameter to search_ta\n");
		return -1;
	}

	if (!dev_desc) {
		dev_desc = plat_bootdev();
		if (!dev_desc) {
			printf("TEEC: %s: Could not find device\n", __func__);
			return -1;
		}

		if (part_get_info_by_name(dev_desc,
					  "userta", &part_info) < 0) {
			dev_desc = NULL;
			printf("TEEC: Could not find userta partition\n");
			return -1;
		}
	}

#ifdef CONFIG_ROCKCHIP_OPTEE_V1
	memcpy(&uuid, uuid_octets, sizeof(TEEC_UUID));
	ta_ver = 1;
	format = "%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x.ta";
#endif

#if defined(CONFIG_ROCKCHIP_OPTEE_V2) || defined(CONFIG_ROCKCHIP_OPTEE_V3)
	tee_uuid_from_octets(&uuid, uuid_octets);
	ta_ver = 2;
	format = "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x.ta";
#endif

	snprintf(fname, 255,
			format,
			uuid.time_low,
			uuid.time_mid,
			uuid.time_hi_and_version,
			uuid.clock_seq_and_node[0],
			uuid.clock_seq_and_node[1],
			uuid.clock_seq_and_node[2],
			uuid.clock_seq_and_node[3],
			uuid.clock_seq_and_node[4],
			uuid.clock_seq_and_node[5],
			uuid.clock_seq_and_node[6],
			uuid.clock_seq_and_node[7]);

	printf("Attempt to load %s \n", fname);

	userta = (uint8_t *)memalign(CONFIG_SYS_CACHELINE_SIZE, part_info.size * part_info.blksz);
	if (!userta) {
		printf("TEEC: Malloc failed!\n");
		res = -1;
		goto exit;
	}

	ret = blk_dread(dev_desc, part_info.start, part_info.size, userta);
	if (ret != part_info.size) {
		printf("TEEC: blk_dread fail\n");
		res = -1;
		goto exit;
	}

	header = (struct userta_header *)userta;
	if (header->magic != 0x524B5441 || header->img_ver != 1) {
		printf("TEEC: userta_header format error! \n");
		res = -1;
		goto exit;
	}

	item = (struct userta_item *)(header + 1);

	for (int i = 0; i < header->ta_num; i++) {
		tee_uuid_from_octets(&ta_uuid, item->ta_uuid);
		snprintf(fname, 255,
				format,
				ta_uuid.time_low,
				ta_uuid.time_mid,
				ta_uuid.time_hi_and_version,
				ta_uuid.clock_seq_and_node[0],
				ta_uuid.clock_seq_and_node[1],
				ta_uuid.clock_seq_and_node[2],
				ta_uuid.clock_seq_and_node[3],
				ta_uuid.clock_seq_and_node[4],
				ta_uuid.clock_seq_and_node[5],
				ta_uuid.clock_seq_and_node[6],
				ta_uuid.clock_seq_and_node[7]);
		debug("search TA %s \n", fname);
		debug("item->ta_offset=0x%x item->ta_len=0x%x *ta_size=%zu\n",
				item->ta_offset, item->ta_len, *ta_size);

		if (is_uuid_equal(ta_uuid, uuid) && item->ta_ver == ta_ver) {
			if (item->ta_len <= *ta_size && ta)
				memcpy(ta, userta + item->ta_offset, item->ta_len);
			*ta_size = item->ta_len;
			res = TA_BINARY_FOUND;
			goto exit;
		} else {
			item++;
		}
	}
	res = TA_BINARY_NOT_FOUND;

exit:
	if (userta)
		free(userta);
	return res;
}

void optee_suppl_cmd_load_ta(struct optee_msg_arg *arg)
{
	struct optee_msg_param *params = NULL;
	void *uuid = NULL;
	void *ta_data = NULL;
	size_t size = 0;
	int ta_found = 0;

	if (arg->num_params != 2) {
		printf("%s: num_params error!\n", __func__);
		arg->ret = TEE_ERROR_BAD_PARAMETERS;
		return;
	}

	params = arg->params;

	uuid = (void *)&params[0].u.value;

	ta_data = tee_supp_param_to_va(params + 1);

	size = params[1].u.rmem.size;

	ta_found = search_ta(uuid, ta_data, &size);
	if (ta_found == TA_BINARY_FOUND) {
		params[1].u.rmem.size = size;
		arg->ret = TEE_SUCCESS;
	} else {
		printf("%s: TA not found!\n", __func__);
		arg->ret = TEE_ERROR_ITEM_NOT_FOUND;
	}

	debug("%s: load ta exit!\n", __func__);

	return;
}
