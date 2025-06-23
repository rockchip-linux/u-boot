// SPDX-License-Identifier: GPL-2.0+
/*
 * Rockchip UFS Host Controller driver
 *
 * Copyright (C) 2024 Rockchip Electronics Co.Ltd.
 */

 #ifndef _UFS_ROCKCHIP_RPMB_H
 #define _UFS_ROCKCHIP_RPMB_H

#define RPMB_DATA_FRAME_SIZE        512

#define RPMB_RESP_AUTH_KEY_PROGRAM		0x0100
#define RPMB_RESP_WRITE_COUNTER_VAL_READ	0x0200
#define RPMB_RESP_AUTH_DATA_WRITE		0x0300
#define RPMB_RESP_AUTH_DATA_READ		0x0400

#define RPMB_DATA_HAM_SIZE			284
#define RPMB_STUFF_DATA_SIZE			196
#define RPMB_KEY_MAC_SIZE			32
#define RPMB_DATA_SIZE				256
#define RPMB_NONCE_SIZE				16
#define RPMB_RESULT_OK				0x00
#define RPMB_RES_NO_AUTH_KEY			0x0007

enum rpmb_op_type {
	RPMB_WRITE_KEY		= 0x01,
	RPMB_READ_CNT		= 0x02,
	RPMB_WRITE		= 0x03,
	RPMB_READ		= 0x04,
	RPMB_READ_RESP		= 0x05,
	RPMB_SEC_CONF_WRITE	= 0x06,
	RPMB_SEC_CONF_READ	= 0x07,
	RPMB_PURGE_ENABLE	= 0x08,
	READ_RPMB_PURGE_STATUS	= 0x09
};

struct ufs_rpmb_unit_desc_tbl {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bUnitIndex;
	uint8_t bLUEnable;
	uint8_t bBootLunID;
	uint8_t bLUWriteProtect;
	uint8_t bLUQueueDepth;
	uint8_t reserved;
	uint8_t bMemoryType;
	uint8_t reserved1;
	uint8_t bLogicalBlockSize;
	uint64_t qLogicalBlockCount;
	uint32_t dEraseBlockSize;
	uint8_t bProvisioningType;
	uint64_t qPhyMemResourceCount;
	uint8_t reserved2[3];
} __attribute__ ((packed));

struct rpmb_data_frame {
	uint8_t stuff_bytes[RPMB_STUFF_DATA_SIZE];
	uint8_t key_mac[RPMB_KEY_MAC_SIZE];
	uint8_t data[RPMB_DATA_SIZE];
	uint8_t nonce[RPMB_NONCE_SIZE];
	uint8_t write_counter[4];
	uint8_t address[2];
	uint8_t block_count[2];
	uint8_t op_result[2];
	uint8_t msg_type[2];
};

extern struct lu_info_tbl rpmb_lu_info;
int ufs_rpmb_blk_read(char *blk_data, uint8_t *key, uint16_t blk_index, uint16_t block_count);
int ufs_rpmb_blk_write(char *write_data, uint8_t *key, uint16_t blk_index, uint16_t blk_count);
uint32_t ufs_rpmb_read_writecount(void);
/* @retrun 0 rpmb key unwritten */
int is_wr_ufs_rpmb_key(void);
int prepare_rpmb_lu(void);
int ufs_rpmb_init(struct ufs_hba *hba);
int ufs_rpmb_write_key(uint8_t * key, uint8_t len);

int ufs_read_device_desc(u8 *buf, u32 size);
int ufs_read_string_desc(int desc_index, u8 *buf, u32 size);
int ufs_read_geo_desc(u8 *buf, u32 size);
int ufs_read_rpmb_unit_desc(u8 *buf, u32 size);
int do_rpmb_op(struct rpmb_data_frame *frame_in, uint32_t in_cnt,
	       struct rpmb_data_frame *frame_out, uint32_t out_cnt);

#endif
