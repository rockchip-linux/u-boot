/*
 * Copyright 2017 Rockchip Electronics Co., Ltd
 * Frank Wang <frank.wang@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <rockusb.h>

#define ROCKUSB_INTERFACE_CLASS	0xff
#define ROCKUSB_INTERFACE_SUB_CLASS	0x06
#define ROCKUSB_INTERFACE_PROTOCOL	0x05

static struct usb_interface_descriptor rkusb_intf_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= ROCKUSB_INTERFACE_CLASS,
	.bInterfaceSubClass	= ROCKUSB_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= ROCKUSB_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *rkusb_fs_function[] = {
	(struct usb_descriptor_header *)&rkusb_intf_desc,
	(struct usb_descriptor_header *)&fsg_fs_bulk_in_desc,
	(struct usb_descriptor_header *)&fsg_fs_bulk_out_desc,
	NULL,
};

static struct usb_descriptor_header *rkusb_hs_function[] = {
	(struct usb_descriptor_header *)&rkusb_intf_desc,
	(struct usb_descriptor_header *)&fsg_hs_bulk_in_desc,
	(struct usb_descriptor_header *)&fsg_hs_bulk_out_desc,
	NULL,
};

struct rk_flash_info {
	u32	flash_size;
	u16	block_size;
	u8	page_size;
	u8	ecc_bits;
	u8	access_time;
	u8	manufacturer;
	u8	flash_mask;
} __packed;

int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	/* Enumerate as a loader device */
	if (IS_RKUSB_UMS_DNL(name))
		dev->bcdUSB = cpu_to_le16(0x0201);

	return 0;
}

__maybe_unused
static inline void dump_cbw(struct fsg_bulk_cb_wrap *cbw)
{
	assert(!cbw);

	debug("%s:\n", __func__);
	debug("Signature %x\n", cbw->Signature);
	debug("Tag %x\n", cbw->Tag);
	debug("DataTransferLength %x\n", cbw->DataTransferLength);
	debug("Flags %x\n", cbw->Flags);
	debug("LUN %x\n", cbw->Lun);
	debug("Length %x\n", cbw->Length);
	debug("OptionCode %x\n", cbw->CDB[0]);
	debug("SubCode %x\n", cbw->CDB[1]);
	debug("SectorAddr %x\n", get_unaligned_be32(&cbw->CDB[2]));
	debug("BlkSectors %x\n\n", get_unaligned_be16(&cbw->CDB[7]));
}

static int rkusb_check_lun(struct fsg_common *common)
{
	struct fsg_lun *curlun;

	/* Check the LUN */
	if (common->lun >= 0 && common->lun < common->nluns) {
		curlun = &common->luns[common->lun];
		if (common->cmnd[0] != SC_REQUEST_SENSE) {
			curlun->sense_data = SS_NO_SENSE;
			curlun->info_valid = 0;
		}
	} else {
		curlun = NULL;
		common->bad_lun_okay = 0;

		/*
		 * INQUIRY and REQUEST SENSE commands are explicitly allowed
		 * to use unsupported LUNs; all others may not.
		 */
		if (common->cmnd[0] != SC_INQUIRY &&
		    common->cmnd[0] != SC_REQUEST_SENSE) {
			debug("unsupported LUN %d\n", common->lun);
			return -EINVAL;
		}
	}

	return 0;
}

static void __do_reset(struct usb_ep *ep, struct usb_request *req)
{
	do_reset(NULL, 0, 0, NULL);
}

static int rkusb_do_reset(struct fsg_common *common,
			  struct fsg_buffhd *bh)
{
	common->data_size_from_cmnd = common->cmnd[4];
	common->residue = 0;
	bh->inreq->complete = __do_reset;
	bh->state = BUF_STATE_EMPTY;

	return 0;
}

static int rkusb_do_test_unit_ready(struct fsg_common *common,
				    struct fsg_buffhd *bh)
{
	common->residue = 0x06 << 24; /* Max block xfer support from host */
	common->data_dir = DATA_DIR_NONE;
	bh->state = BUF_STATE_EMPTY;

	return 0;
}

static int rkusb_do_read_flash_id(struct fsg_common *common,
				  struct fsg_buffhd *bh)
{
	u8 *buf = (u8 *)bh->buf;
	u32 len = common->data_size;

	memcpy((void *)&buf[0], "EMMC ", 5);

	/* Set data xfer size */
	common->residue = common->data_size_from_cmnd = len;

	return len;
}

static int rkusb_do_test_bad_block(struct fsg_common *common,
				   struct fsg_buffhd *bh)
{
	u8 *buf = (u8 *)bh->buf;
	u32 len = common->data_size;

	memset((void *)&buf[0], 0, len);

	/* Set data xfer size */
	common->residue = common->data_size_from_cmnd = len;

	return len;
}

static int rkusb_do_read_flash_info(struct fsg_common *common,
				    struct fsg_buffhd *bh)
{
	u8 *buf = (u8 *)bh->buf;
	u32 len = common->data_size;
	struct rk_flash_info finfo = {
		.block_size = 1024,
		.ecc_bits = 0,
		.page_size = 4,
		.access_time = 40,
		.manufacturer = 0,
		.flash_mask = 0
	};

	finfo.flash_size = (u32)ums[common->lun].block_dev.lba;
	if (finfo.flash_size)
		finfo.flash_mask = 1;

	memset((void *)&buf[0], 0, len);
	memcpy((void *)&buf[0], (void *)&finfo, len);

	/* Set data xfer size */
	common->residue = common->data_size_from_cmnd = len;

	return len;
}

static int rkusb_do_lba_erase(struct fsg_common *common,
			      struct fsg_buffhd *bh)
{
	struct fsg_lun *curlun = &common->luns[common->lun];
	u32 lba, amount;
	loff_t file_offset;
	int rc;

	lba = get_unaligned_be32(&common->cmnd[2]);
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		rc = -EINVAL;
		goto out;
	}

	file_offset = ((loff_t) lba) << 9;
	amount = get_unaligned_be16(&common->cmnd[7]) << 9;
	if (unlikely(amount == 0)) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		rc = -EIO;
		goto out;
	}

	/* Perform the erase */
	rc = ums[common->lun].erase_sector(&ums[common->lun],
			       file_offset / SECTOR_SIZE,
			       amount / SECTOR_SIZE);
	if (!rc) {
		curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
		rc = -EIO;
	}

out:
	common->data_dir = DATA_DIR_NONE;
	bh->state = BUF_STATE_EMPTY;

	return rc;
}

static int rkusb_do_read_capacity(struct fsg_common *common,
				    struct fsg_buffhd *bh)
{
	u8 *buf = (u8 *)bh->buf;
	u32 len = common->data_size;

	/*
	 * bit[0]: Direct LBA, 0: Disabled;
	 * bit[1:63}: Reserved.
	 */
	memset((void *)&buf[0], 0, len);

	/* Set data xfer size */
	common->residue = common->data_size_from_cmnd = len;

	return len;
}

static int rkusb_cmd_process(struct fsg_common *common,
			     struct fsg_buffhd *bh, int *reply)
{
	struct usb_request	*req = bh->outreq;
	struct fsg_bulk_cb_wrap	*cbw = req->buf;
	int rc;

	dump_cbw(cbw);

	if (rkusb_check_lun(common)) {
		*reply = -EINVAL;
		return RKUSB_RC_ERROR;
	}

	switch (common->cmnd[0]) {
	case RKUSB_TEST_UNIT_READY:
		*reply = rkusb_do_test_unit_ready(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_READ_FLASH_ID:
		*reply = rkusb_do_read_flash_id(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_TEST_BAD_BLOCK:
		*reply = rkusb_do_test_bad_block(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_LBA_READ_10:
		common->cmnd[0] = SC_READ_10;
		common->cmnd[1] = 0; /* Not support */
		rc = RKUSB_RC_CONTINUE;
		break;

	case RKUSB_LBA_WRITE_10:
		common->cmnd[0] = SC_WRITE_10;
		common->cmnd[1] = 0; /* Not support */
		rc = RKUSB_RC_CONTINUE;
		break;

	case RKUSB_READ_FLASH_INFO:
		*reply = rkusb_do_read_flash_info(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_LBA_ERASE:
		*reply = rkusb_do_lba_erase(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_READ_CAPACITY:
		*reply = rkusb_do_read_capacity(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_RESET:
		*reply = rkusb_do_reset(common, bh);
		rc = RKUSB_RC_FINISHED;
		break;

	case RKUSB_SET_DEVICE_ID:
	case RKUSB_READ_10:
	case RKUSB_WRITE_10:
	case RKUSB_ERASE_10:
	case RKUSB_WRITE_SPARE:
	case RKUSB_READ_SPARE:
	case RKUSB_ERASE_10_FORCE:
	case RKUSB_GET_VERSION:
	case RKUSB_ERASE_SYS_DISK:
	case RKUSB_SDRAM_READ_10:
	case RKUSB_SDRAM_WRITE_10:
	case RKUSB_SDRAM_EXECUTE:
	case RKUSB_GET_CHIP_VER:
	case RKUSB_LOW_FORMAT:
	case RKUSB_SET_RESET_FLAG:
	case RKUSB_SPI_READ_10:
	case RKUSB_SPI_WRITE_10:
	case RKUSB_SESSION:
		/* Fall through */
	default:
		rc = RKUSB_RC_UNKNOWN_CMND;
		break;
	}

	return rc;
}

DECLARE_GADGET_BIND_CALLBACK(rkusb_ums_dnl, fsg_add);
