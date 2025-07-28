/*
 * Copyright 2025, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <dm.h>
#include <log.h>
#include <tee.h>
#include <dm/device_compat.h>

#include "optee_msg.h"
#include "optee_private.h"
#include "../../../drivers/ufs/ufs.h"
#include "../../../drivers/ufs/ufs-rockchip-rpmb.h"
/*
 * Request and response definitions must be in sync with the secure side of
 * OP-TEE.
 */

/* Request */
struct rpmb_req {
	u16 cmd;
#define RPMB_CMD_DATA_REQ      0x00
#define RPMB_CMD_GET_DEV_INFO  0x01
	u16 dev_id;
	u16 block_count;
	/* Optional data frames (rpmb_data_frame) follow */
};

#define RPMB_REQ_DATA(req) ((void *)((struct rpmb_req *)(req) + 1))

/* Response to device info request */
struct rpmb_dev_info {
	u8 cid[16];
	u8 rpmb_size_mult;	/* EXT CSD-slice 168: RPMB Size */
	u8 rel_wr_sec_c;	/* EXT CSD-slice 222: Reliable Write Sector */
				/*                    Count */
	u8 ret_code;
#define RPMB_CMD_GET_DEV_INFO_RET_OK     0x00
#define RPMB_CMD_GET_DEV_INFO_RET_ERROR  0x01
};

static uint16_t bedata_to_u16(u8 *d)
{
	return (d[0] << 8) + (d[1]);
}

static uint64_t bedata_to_u64(u8 *d)
{
	uint64_t data = 0;
	uint64_t temp = 0;

	for (int i = 0; i < 8; i++) {
		temp = d[i];
		data += (temp << ((7 - i) * 8));
	}

	return data;
}

static void u16_to_bedata(u16 src, u8 *d)
{
	d[0] = (src >> 8) & 0xff;
	d[1] = src & 0xff;
}

static u32 rpmb_data_req(struct rpmb_data_frame *req_frm,
			 size_t req_nfrm,
			 struct rpmb_data_frame *rsp_frm,
			 size_t rsp_nfrm)
{
	struct rpmb_data_frame *req_packets = NULL;
	struct rpmb_data_frame *rsp_packets = NULL;
	u16 req_type;
	u16 block_count;
	u32 ret = TEE_ERROR_GENERIC;

	req_packets = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(struct rpmb_data_frame) * req_nfrm);
	if (!req_packets)
		goto out;

	rsp_packets = memalign(CONFIG_SYS_CACHELINE_SIZE, sizeof(struct rpmb_data_frame) * rsp_nfrm);
	if (!rsp_packets)
		goto out;

	memcpy(req_packets, req_frm, sizeof(struct rpmb_data_frame) * req_nfrm);

	req_type = bedata_to_u16(req_packets->msg_type);

	switch (req_type) {
	case RPMB_WRITE_KEY:
	case RPMB_WRITE:
		ret = do_rpmb_op(req_packets, req_nfrm, rsp_packets, rsp_nfrm);
		break;
	case RPMB_READ_CNT:
		do_rpmb_op(req_packets, req_nfrm, rsp_packets, rsp_nfrm);
		ret = TEE_SUCCESS;
		break;
	case RPMB_READ:
		block_count = bedata_to_u16(req_packets->block_count);
		if (block_count == 0) {
			u16_to_bedata(rsp_nfrm, req_packets->block_count);
		}
		ret = do_rpmb_op(req_packets, req_nfrm, rsp_packets, rsp_nfrm);
		break;
	default:
		ret = TEE_ERROR_BAD_PARAMETERS;
		break;
	}

	for (int i = 0; i < rsp_nfrm; i++)
		memcpy(rsp_frm + i, rsp_packets + i, sizeof(struct rpmb_data_frame));

out:
	if (req_packets)
		free(req_packets);

	if (rsp_packets)
		free(rsp_packets);

	return ret;
}

static u32 rpmb_get_dev_info(struct rpmb_dev_info *info)
{
	u8 manufacture_name_idx;
	u8 rpmb_rw_blocks;
	u64 rpmb_block_count;
	u8 desc_buff[QUERY_DESC_MAX_SIZE] = { 0 };
	u8 str_buff[QUERY_DESC_MAX_SIZE] = { 0 };
	u8 geo_buff[QUERY_DESC_MAX_SIZE] = { 0 };
	u8 unit_buf[QUERY_DESC_MAX_SIZE] = { 0 };

	if (ufs_read_device_desc(desc_buff, sizeof(desc_buff))) {
		printf("get device desc fail!\n");
		return TEE_ERROR_GENERIC;
	}
	manufacture_name_idx = desc_buff[0x14];

	if (ufs_read_string_desc(manufacture_name_idx, str_buff, sizeof(str_buff))) {
		printf("get device desc fail!\n");
		return TEE_ERROR_GENERIC;
	}

	if (str_buff[0] != 0x12) {
		printf("manufacture name string length error!\n");
		return TEE_ERROR_GENERIC;
	}

	if (sizeof(info->cid) != 0x10) {
		printf("cid length error!\n");
		return TEE_ERROR_GENERIC;
	}

	memcpy(info->cid, &str_buff[2], sizeof(info->cid));

	if (ufs_read_geo_desc(geo_buff, sizeof(geo_buff))) {
		printf("get geometry desc fail!\n");
		return TEE_ERROR_GENERIC;
	}
	rpmb_rw_blocks = geo_buff[0x17];

	if (ufs_read_rpmb_unit_desc(unit_buf, sizeof(unit_buf))) {
		printf("get rpmb ubit desc fail!\n");
		return TEE_ERROR_GENERIC;
	}
	rpmb_block_count = bedata_to_u64(&unit_buf[0x0B]);

	info->rel_wr_sec_c = rpmb_rw_blocks;
	info->rpmb_size_mult = rpmb_block_count / 512;
	info->ret_code = RPMB_CMD_GET_DEV_INFO_RET_OK;

	return TEE_SUCCESS;
}

static u32 ufs_rpmb_process_request(void *req, ulong req_size,
				    void *rsp, ulong rsp_size)
{
	struct rpmb_req *sreq = req;
	size_t req_nfrm = 0;
	size_t rsp_nfrm = 0;

	if (req_size < sizeof(*sreq))
		return TEE_ERROR_BAD_PARAMETERS;

	switch (sreq->cmd) {
	case RPMB_CMD_DATA_REQ:
		req_nfrm = (req_size - sizeof(struct rpmb_req)) / 512;
		rsp_nfrm = rsp_size / 512;
		return rpmb_data_req(RPMB_REQ_DATA(req), req_nfrm, rsp, rsp_nfrm);
	case RPMB_CMD_GET_DEV_INFO:
		if (req_size != sizeof(struct rpmb_req) ||
		    rsp_size != sizeof(struct rpmb_dev_info)) {
			debug("Invalid req/rsp size\n");
			return TEE_ERROR_BAD_PARAMETERS;
		}
		return rpmb_get_dev_info(rsp);

	default:
		debug("Unsupported RPMB command: %d\n", sreq->cmd);
		return TEE_ERROR_BAD_PARAMETERS;
	}
}

void optee_suppl_cmd_ufs_rpmb(struct udevice *dev, struct optee_msg_arg *arg)
{
	struct tee_shm *req_shm;
	struct tee_shm *rsp_shm;
	void *req_buf;
	void *rsp_buf;
	ulong req_size;
	ulong rsp_size;

	if (optee_is_support_dynamic_shm(dev)) {
		if (arg->num_params != 2 ||
		    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_RMEM_INPUT ||
		    arg->params[1].attr != OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT) {
			arg->ret = TEE_ERROR_BAD_PARAMETERS;
			return;
		}

		req_shm = (struct tee_shm *)(ulong)arg->params[0].u.rmem.shm_ref;
		req_buf = (u8 *)req_shm->addr + arg->params[0].u.rmem.offs;
		req_size = arg->params[0].u.rmem.size;

		rsp_shm = (struct tee_shm *)(ulong)arg->params[1].u.rmem.shm_ref;
		rsp_buf = (u8 *)rsp_shm->addr + arg->params[1].u.rmem.offs;
		rsp_size = arg->params[1].u.rmem.size;
	} else {
		if (arg->num_params != 2 ||
		    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_TMEM_INPUT ||
		    arg->params[1].attr != OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT) {
			arg->ret = TEE_ERROR_BAD_PARAMETERS;
			return;
		}

		req_shm = (struct tee_shm *)(ulong)arg->params[0].u.tmem.shm_ref;
		req_buf = (u8 *)req_shm->addr;
		req_size = arg->params[0].u.tmem.size;

		rsp_shm = (struct tee_shm *)(ulong)arg->params[1].u.tmem.shm_ref;
		rsp_buf = (u8 *)rsp_shm->addr;
		rsp_size = arg->params[1].u.tmem.size;
	}

	arg->ret = ufs_rpmb_process_request(req_buf, req_size, rsp_buf, rsp_size);
}
