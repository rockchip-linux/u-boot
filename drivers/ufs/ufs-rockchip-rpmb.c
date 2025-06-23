// SPDX-License-Identifier: GPL-2.0+
/*
 * Rockchip UFS Host Controller driver
 *
 * Copyright (C) 2024 Rockchip Electronics Co.Ltd.
 */

#include <command.h>
#include <charset.h>
#include <common.h>
#include <dm.h>
#include <log.h>
#include <dm/lists.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <malloc.h>
#include <hexdump.h>
#include <scsi.h>
#include <asm/io.h>
#include <asm/dma-mapping.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include "ufs.h"
#include "ufs-rockchip-rpmb.h"
#include <u-boot/sha256.h>

extern int ufs_send_scsi_cmd(struct ufs_hba *hba, struct scsi_cmd *pccb);

#define UFS_OP_SECURITY_PROTOCOL_IN	0xA2
#define UFS_OP_SECURITY_PROTOCOL_OUT	0xB5
#define UFS_OP_TST_U_RDY		0x00
#define UFS_RPMB_KEY_SZ			32
#define SHA256_BLOCK_SIZE		64

static struct ufs_hba *rpmb_hba;

struct lu_info_tbl {
	int lu_index;
	uint32_t log2blksz;
	uint64_t blkcnt;
};

struct lu_info_tbl rpmb_lu_info = {0};
static struct scsi_cmd tempccb;	/* temporary scsi command buffer */

static void u16_to_bytes(uint16_t u16, uint8_t *bytes)
{
	*bytes = (uint8_t) (u16 >> 8);
	*(bytes + 1) = (uint8_t) u16;
}

static void bytes_to_u16(uint8_t *bytes, uint16_t *u16)
{
	*u16 = (uint16_t) ((*bytes << 8) + *(bytes + 1));
}

static void bytes_to_u32(uint8_t *bytes, uint32_t *u32)
{
	*u32 = (uint32_t) ((*(bytes) << 24) +
			   (*(bytes + 1) << 16) +
			   (*(bytes + 2) << 8) + (*(bytes + 3)));
}

static void u32_to_bytes(uint32_t u32, uint8_t *bytes)
{
	*bytes = (uint8_t) (u32 >> 24);
	*(bytes + 1) = (uint8_t) (u32 >> 16);
	*(bytes + 2) = (uint8_t) (u32 >> 8);
	*(bytes + 3) = (uint8_t) u32;
}

static int hba_test(struct ufs_hba *hba)
{
	if (!hba) {
		printf("No UFS device!\n");
		return -ENODEV;
	}

	return 0;
}

static void scsi_secproc_in(struct scsi_cmd *pccb, uint32_t lba, uint32_t size)
{
	pccb->cmd[0] = UFS_OP_SECURITY_PROTOCOL_IN;	/* 0: opcode */
	pccb->cmd[1] = 0xEC;				/* 1: security protocal */
	pccb->cmd[2] = 0;				/* 2: specific */
	pccb->cmd[3] = 0x1;				/* 3: specific */
	pccb->cmd[4] = 0;				/* 4: reserved */
	pccb->cmd[5] = 0;				/* 5: reserved */
	pccb->cmd[6] = (uint8_t)((size >> 24) & 0xff);	/* 6: MSB, shift 24 */
	pccb->cmd[7] = (uint8_t)((size >> 16) & 0xff);	/* 7: MSB, shift 16 */
	pccb->cmd[8] = (uint8_t)((size >> 8) & 0xff);	/* 8: LSB, shift 8 */
	pccb->cmd[9] = (uint8_t)(size & 0xff);		/* 9: LSB */
	pccb->cmd[10] = 0;				/* 10: reserved */
	pccb->cmd[11] = 0;				/* 11: control */

	pccb->cmdlen = 12;
}

static void scsi_secproc_out(struct scsi_cmd *pccb, uint32_t lba, uint32_t size)
{
	pccb->cmd[0] = UFS_OP_SECURITY_PROTOCOL_OUT;	/* 0: opcode */
	pccb->cmd[1] = 0xEC;				/* 1: security protocal */
	pccb->cmd[2] = 0;				/* 2: specific */
	pccb->cmd[3] = 0x1;				/* 3: specific */
	pccb->cmd[4] = 0;				/* 4: reserved */
	pccb->cmd[5] = 0;				/* 5: reserved */
	pccb->cmd[6] = (uint8_t)((size >> 24) & 0xff);	/* 6: MSB, shift 24 */
	pccb->cmd[7] = (uint8_t)((size >> 16) & 0xff);	/* 7: MSB, shift 16 */
	pccb->cmd[8] = (uint8_t)((size >> 8) & 0xff);	/* 8: LSB, shift 8 */
	pccb->cmd[9] = (uint8_t)(size & 0xff);		/* 9: LSB */
	pccb->cmd[10] = 0;				/* 10: reserved */
	pccb->cmd[11] = 0;				/* 11: control */

	pccb->cmdlen = 12;
}

static void scsi_test_unit_ready(struct scsi_cmd *pccb)
{
	pccb->cmd[0] = UFS_OP_TST_U_RDY;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	pccb->cmd[4] = 0;
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
}

static int rpmb_send_scsi_cmd(struct ufs_hba *hba, uint32_t opcode, int dma_dir, int lun,
			      void *buf_addr, lbaint_t start, lbaint_t blkcnt)
{
	struct scsi_cmd *pccb = (struct scsi_cmd *)&tempccb;

	pccb->lun = lun;
	pccb->pdata = buf_addr;
	pccb->dma_dir = dma_dir;
	pccb->datalen = blkcnt * sizeof(struct rpmb_data_frame);

	if (opcode == UFS_OP_SECURITY_PROTOCOL_OUT) {
		scsi_secproc_out(pccb, start, pccb->datalen);
		pccb->cmdlen = 12;
	} else if (opcode == UFS_OP_SECURITY_PROTOCOL_IN) {
		scsi_secproc_in(pccb, start, pccb->datalen);
		pccb->cmdlen = 12;
	} else if (opcode == UFS_OP_TST_U_RDY) {
		scsi_test_unit_ready(pccb);
	} else {
		return -EINVAL;
	}

	return  ufs_send_scsi_cmd(hba, pccb);
}

static void ufs_rpmb_hmac(unsigned char *key, struct rpmb_data_frame *frames_in, ssize_t blocks_cnt,
			  unsigned char *output)
{
	sha256_context ctx;
	int i;
	unsigned char k_ipad[SHA256_BLOCK_SIZE];
	unsigned char k_opad[SHA256_BLOCK_SIZE];

	sha256_starts(&ctx);

	/* According to RFC 4634, the HMAC transform looks like:
	   SHA(K XOR opad, SHA(K XOR ipad, text))

	   where K is an n byte key.
	   ipad is the byte 0x36 repeated blocksize times
	   opad is the byte 0x5c repeated blocksize times
	   and text is the data being protected.
	*/

	for (i = 0; i < UFS_RPMB_KEY_SZ; i++) {
		k_ipad[i] = key[i] ^ 0x36;
		k_opad[i] = key[i] ^ 0x5c;
	}
	/* remaining pad bytes are '\0' XOR'd with ipad and opad values */
	for ( ; i < SHA256_BLOCK_SIZE; i++) {
		k_ipad[i] = 0x36;
		k_opad[i] = 0x5c;
	}
	sha256_update(&ctx, k_ipad, SHA256_BLOCK_SIZE);

	for (i = 0; i < blocks_cnt; i++)
		sha256_update(&ctx, frames_in[i].data, RPMB_DATA_HAM_SIZE);

	sha256_finish(&ctx, output);

	/* Init context for second pass */
	sha256_starts(&ctx);

	/* start with outer pad */
	sha256_update(&ctx, k_opad, SHA256_BLOCK_SIZE);

	/* then results of 1st hash */
	sha256_update(&ctx, output, UFS_RPMB_KEY_SZ);

	/* finish up 2nd pass */
	sha256_finish(&ctx, output);
}

int prepare_rpmb_lu(void)
{
	struct ufs_rpmb_unit_desc_tbl rpmb_unit_desc;
	uint64_t block_count = 0;
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	ret = ufshcd_read_desc_param(rpmb_hba, QUERY_DESC_IDN_UNIT, 0xc4, 0, (u8 *)&rpmb_unit_desc,
				     QUERY_DESC_UNIT_DEF_SIZE);
	if (ret) {
		dev_err(rpmb_hba->dev, "%s: Failed reading RPMB Desc. err = %d\n", __func__, ret);
		return ret;
	}

	block_count = be64_to_cpu(rpmb_unit_desc.qLogicalBlockCount);
	if (block_count) {
		rpmb_lu_info.lu_index = 0xc4;
		rpmb_lu_info.blkcnt = block_count;
		rpmb_lu_info.log2blksz = rpmb_unit_desc.bLogicalBlockSize;
		/*
		* write key,read counter,write data,
		* need this test_unit_ready operation.
		*/
		ret = rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_TST_U_RDY, DMA_NONE, rpmb_lu_info.lu_index,
					 NULL, 0, 0);
	}

	return ret;
}

/* @retrun 0 rpmb key unwritten */
int is_wr_ufs_rpmb_key(void)
{
	int ret = 0;
	struct rpmb_data_frame *data_frame;
	uint16_t msg_type;
	uint16_t op_result;
	uint8_t nonce[RPMB_NONCE_SIZE] = {
		0xa5, 0x5a, 0xff, 0x00, 0xbe, 0xef, 0xbe, 0xef,
		0xbe, 0xef, 0xbe, 0xef, 0x00, 0xff, 0x5a, 0xa5
	};

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	if (rpmb_lu_info.log2blksz == 0) {
		ret = prepare_rpmb_lu();
		if(0 != ret)
			return ret;
	}

	msg_type = RPMB_READ;
	data_frame = memalign(ARCH_DMA_MINALIGN, RPMB_DATA_FRAME_SIZE);
	if (!data_frame) {
		printf("%s malloc error\n", __func__);
		return -1;
	}

	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	u16_to_bytes(msg_type, data_frame->msg_type);
	memcpy(data_frame->nonce, nonce, RPMB_NONCE_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
		 	   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);


	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_IN, DMA_FROM_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);

	/* result check */
	bytes_to_u16(data_frame->op_result, &op_result);
	bytes_to_u16(data_frame->msg_type, &msg_type);
	if (op_result == RPMB_RES_NO_AUTH_KEY) {
		printf("%s rpmb key not write\n", __func__);
		ret = 0;
	} else {
		printf("%s rpmb key has been written\n", __func__);
		ret = -1;
	}
	free(data_frame);

	return ret;
}

uint32_t ufs_rpmb_read_writecount(void)
{
	struct rpmb_data_frame *data_frame;
	uint16_t msg_type;
	uint16_t op_result;
	uint32_t writecount;
	uint8_t nonce[RPMB_NONCE_SIZE] = {
		0xa5, 0x5a, 0xff, 0x00, 0xbe, 0xef, 0xbe, 0xef,
		0xbe, 0xef, 0xbe, 0xef, 0x00, 0xff, 0x5a,0xa5
	};
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	if (rpmb_lu_info.log2blksz == 0) {
		ret = prepare_rpmb_lu();
		if (ret != 0) {
			printf("prepare rpmb unit failed!\n");
			return ret;
		}
	}

	msg_type = RPMB_READ_CNT;
	data_frame = memalign(ARCH_DMA_MINALIGN, RPMB_DATA_FRAME_SIZE);
	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	u16_to_bytes(msg_type, data_frame->msg_type);
	memcpy(data_frame->nonce, nonce, RPMB_NONCE_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);

	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_IN, DMA_FROM_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);

	/* result check */
	bytes_to_u16(data_frame->op_result, &op_result);
	bytes_to_u16(data_frame->msg_type, &msg_type);
	if ((op_result == RPMB_RESULT_OK) &&
	    (msg_type == RPMB_RESP_WRITE_COUNTER_VAL_READ)) {
		bytes_to_u32(data_frame->write_counter, &writecount);
		printf("read write count successed\n");
		free(data_frame);
		return writecount;
	}
	else
		printf(" read write count:0x%x ,msg_type:0x%x\n",
		       op_result,msg_type);
	free(data_frame);

	return 1;
}

/*
 * blk_data: for save read data;
 * blk_index: the block index for read;
 * block_count: the read count;
 * success return 0;
 */
int ufs_rpmb_blk_read(char *blk_data, uint8_t *key, uint16_t blk_index, uint16_t block_count)
{
	struct rpmb_data_frame *data_frame;
	struct rpmb_data_frame *resp_buf;
	uint16_t msg_type;
	uint16_t op_result;
	uint8_t nonce[RPMB_NONCE_SIZE] = {
		0xa5, 0x5a, 0xff, 0x00, 0xbe, 0xef, 0xbe, 0xef,
		0xbe, 0xef, 0xbe, 0xef, 0x00, 0xef, 0x5a,0xa5
	};
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	if (rpmb_lu_info.log2blksz == 0) {
		ret = prepare_rpmb_lu();
		if (ret != 0)
		return ret;
	}

	if (!blk_data) {
		printf("rpmb_blk_read null \n");
		return 0;
	}

	msg_type = RPMB_READ;
	data_frame = memalign(ARCH_DMA_MINALIGN, RPMB_DATA_FRAME_SIZE);
	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	u16_to_bytes(msg_type, data_frame->msg_type);
	u16_to_bytes(blk_index, data_frame->address);
	u16_to_bytes(block_count, data_frame->block_count);
	memcpy(data_frame->nonce, nonce, RPMB_NONCE_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);

	resp_buf = memalign(ARCH_DMA_MINALIGN, RPMB_DATA_FRAME_SIZE * block_count);
	memset(resp_buf, 0, RPMB_DATA_FRAME_SIZE * block_count);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_IN, DMA_FROM_DEVICE,
			   rpmb_lu_info.lu_index, (void*)resp_buf, 0, block_count);

	/* result check */
	bytes_to_u16((resp_buf + block_count - 1)->op_result, &op_result);
	bytes_to_u16((resp_buf + block_count - 1)->msg_type, &msg_type);
	if ((op_result == RPMB_RESULT_OK) &&
            (msg_type == RPMB_RESP_AUTH_DATA_READ)) {
		uint8_t i = 0;
		for (i = 0; i < block_count; i++)
			memcpy((blk_data + i * RPMB_DATA_SIZE),
			       ((uint8_t *) (resp_buf + i) +
				RPMB_STUFF_DATA_SIZE + RPMB_KEY_MAC_SIZE),
			       RPMB_DATA_SIZE);
		printf("read  successed\n");
		free(resp_buf);
		free(data_frame);
		return block_count;
	} else
		printf("read write count:0x%x ,msg_type:0x%x\n", op_result, msg_type);

	free(resp_buf);
	free(data_frame);

	return 0;
}

/*
 * write_data: data for write
 * blk_index: the block will be write to;
 * success return 0
 */

/* for single data */
int ufs_rpmb_blk_write(char *write_data, uint8_t *key, uint16_t blk_index, uint16_t blk_count)
{
	struct rpmb_data_frame *data_frame;
	uint16_t msg_type;
	uint16_t op_result;
	uint32_t writecount;
	int ret = 0, i;


	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	if(rpmb_lu_info.log2blksz == 0) {
		ret = prepare_rpmb_lu();
		if (ret != 0)
			return ret;
	}

	if (!write_data)
		return 1;

	/* TODO: The following codes is multiple block write
	 * for(int i=0;i<(strlen(write_data)/RPMB_DATA_SIZE),i++){
	 */

	msg_type = RPMB_WRITE;
	data_frame = memalign(ARCH_DMA_MINALIGN, RPMB_DATA_FRAME_SIZE * blk_count);
	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE * blk_count);
	writecount = ufs_rpmb_read_writecount();

	for (i = 0; i < blk_count; i++) {
		u16_to_bytes(msg_type, data_frame[i].msg_type);
		u16_to_bytes(blk_index, data_frame[i].address);
		u16_to_bytes(blk_count, data_frame[i].block_count);
		u32_to_bytes(writecount, data_frame[i].write_counter);
		memcpy(data_frame[i].data, write_data, RPMB_DATA_SIZE);
		write_data += RPMB_DATA_FRAME_SIZE;
	}

	ufs_rpmb_hmac(key, data_frame, blk_count, data_frame[blk_count - 1].key_mac);

	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, blk_count);

	/* for read result req */
	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	msg_type = RPMB_READ_RESP;
	u16_to_bytes(msg_type, data_frame->msg_type);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);

	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_IN, DMA_FROM_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, 1);

	/* result check */
	bytes_to_u16(data_frame->op_result, &op_result);
	bytes_to_u16(data_frame->msg_type, &msg_type);
	if ((op_result == RPMB_RESULT_OK) && (msg_type == RPMB_RESP_AUTH_DATA_WRITE)) {
		printf(" data  write successed\n");
		free(data_frame);
		return blk_count;
	} else {
		printf(" data write fail op_result:0x%x ,msg_type:0x%x\n", op_result, msg_type);
	}

	free(data_frame);

	return 0;
}

/*
 * key: the key will write must 32 len;
 * len :must be 32;
 * reutrn 0 success
 */
int ufs_rpmb_write_key(uint8_t * key, uint8_t len)
{
	struct rpmb_data_frame *data_frame = NULL;
	uint16_t msg_type;
	uint16_t op_result;
	int ret = 0;
	lbaint_t transfer_blkcnt = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	if (rpmb_lu_info.log2blksz == 0) {
		ret = prepare_rpmb_lu();
		if (ret != 0)
			return ret;
	}

	/* rpmb_lu_info.log2blksz=0x08,256B */
	transfer_blkcnt = RPMB_DATA_FRAME_SIZE >> (rpmb_lu_info.log2blksz);

	if (!key || len != RPMB_KEY_MAC_SIZE)
		return 1;

	/* for write rpmb key req */
	msg_type = RPMB_WRITE_KEY;
	data_frame = memalign(ARCH_DMA_MINALIGN, RPMB_DATA_FRAME_SIZE);
	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	u16_to_bytes(msg_type, data_frame->msg_type);
	memcpy(data_frame->key_mac, key, RPMB_KEY_MAC_SIZE);

	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, transfer_blkcnt);

	/* for read result req */
	memset(data_frame, 0, RPMB_DATA_FRAME_SIZE);
	msg_type = RPMB_READ_RESP;
	u16_to_bytes(msg_type, data_frame->msg_type);

	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, transfer_blkcnt);

	memset(data_frame, 0xcc, RPMB_DATA_FRAME_SIZE);
	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_IN, DMA_FROM_DEVICE,
			   rpmb_lu_info.lu_index, (void*)data_frame, 0, transfer_blkcnt);

	/* result check */
	bytes_to_u16(data_frame->op_result, &op_result);
	bytes_to_u16(data_frame->msg_type, &msg_type);
	if ((op_result == RPMB_RESULT_OK) && (msg_type == RPMB_RESP_AUTH_KEY_PROGRAM)) {
		printf(" key write successed\n");
		free(data_frame);
		return 0;
	} else {
		printf(" key write fail op_result:0x%x ,msg_type:0x%x\n", op_result, msg_type);
	}

	free(data_frame);

	return 1;
}

static int ufs_read_desc(struct ufs_hba *hba, enum desc_idn desc_id,
		  int desc_index, u8 *buf, u32 size)
{
	return ufshcd_read_desc_param(hba, desc_id, desc_index, 0, buf, size);
}

int ufs_read_device_desc(u8 *buf, u32 size)
{
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	return ufs_read_desc(rpmb_hba, QUERY_DESC_IDN_DEVICE, 0, buf, size);
}

int ufs_read_string_desc(int desc_index, u8 *buf, u32 size)
{
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	return ufs_read_desc(rpmb_hba, QUERY_DESC_IDN_STRING, desc_index, buf, size);
}

int ufs_read_geo_desc(u8 *buf, u32 size)
{
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	return ufs_read_desc(rpmb_hba, QUERY_DESC_IDN_GEOMETRY, 0, buf, size);
}

int ufs_read_rpmb_unit_desc(u8 *buf, u32 size)
{
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	return ufs_read_desc(rpmb_hba, QUERY_DESC_IDN_UNIT, 0xc4, buf, size);
}

int do_rpmb_op(struct rpmb_data_frame *frame_in, uint32_t in_cnt,
	       struct rpmb_data_frame *frame_out, uint32_t out_cnt)
{
	uint16_t msg_type = 0;
	int ret = 0;

	ret = hba_test(rpmb_hba);
	if (ret)
		return ret;

	if (rpmb_lu_info.log2blksz == 0) {
		ret = prepare_rpmb_lu();
		if (ret != 0) {
			printf("prepare rpmb unit failed!\n");
			return ret;
		}
	}

	if (!frame_in || !frame_out || !in_cnt || !out_cnt) {
		printf("Wrong rpmb parameters\n");
		return -1;
	}

	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
			   rpmb_lu_info.lu_index, (void*)frame_in, 0, in_cnt);

	bytes_to_u16(frame_in->msg_type, &msg_type);
	if ((msg_type == RPMB_WRITE) || (msg_type == RPMB_WRITE_KEY) ||
	    (msg_type == RPMB_SEC_CONF_WRITE)) {
		memset(&frame_in[0], 0, sizeof(frame_in[0]));
		msg_type = RPMB_READ_RESP;
		u16_to_bytes(msg_type, frame_in->msg_type);

		rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_OUT, DMA_TO_DEVICE,
				   rpmb_lu_info.lu_index, (void*)frame_in, 0, 1);
	}

	rpmb_send_scsi_cmd(rpmb_hba, UFS_OP_SECURITY_PROTOCOL_IN, DMA_FROM_DEVICE,
			   rpmb_lu_info.lu_index, (void*)frame_out, 0, out_cnt);
	return 0;
}

int ufs_rpmb_init(struct ufs_hba *hba)
{
	rpmb_hba = hba;

	return 0;
}
