/*
 * Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_*
 * They come from bootloader/legacy/include/boot/flash.h
 *
 * The boot_img_hdr structure and associated magic numbers also
 * come from the Android project.  They are from
 * system/core/mkbootimg/bootimg.h
 *
 * Here are their copyrights
 *
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef FASTBOOT_H
#define FASTBOOT_H

#include <common.h>
#include <command.h>
#include <part.h>
#ifdef CONFIG_RK_DWC3_UDC
#include <usb/dwc3_rk_udc.h>
#endif

/* This is the interface file between the common cmd_fastboot.c and
   the board specific support.
*/

/* From fastboot client.. */
#define FASTBOOT_INTERFACE_CLASS     0xff
#define FASTBOOT_INTERFACE_SUB_CLASS 0x42
#define FASTBOOT_INTERFACE_PROTOCOL  0x03

#define FASTBOOT_UNLOCKED_ENV_NAME "fastboot_unlocked"
#define FASTBOOT_SERIALNO_BOOTARG "androidboot.serialno"

/* The fastboot client uses a value of 2048 for the
   page size of it boot.img file format.
   Reset this in your board config file as needed. */
#ifndef CONFIG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE
#define CONFIG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE 2048
#endif

/* Fastboot bulk packet sizes */
#define	CONFIG_USBD_FASTBOOT_BULK_PKTSIZE_HS	512
#define	CONFIG_USBD_FASTBOOT_BULK_PKTSIZE_FS	64

/* Max size of commands from host to us */
#define FASTBOOT_COMMAND_SIZE 64

/* Max size of responses from us to host */
#define FASTBOOT_RESPONSE_SIZE 64

/* Flags */
#define FASTBOOT_FLAG_RESPONSE  0x1
#define FASTBOOT_FLAG_HAS_RUN   0x2

/* sparse things */
typedef struct sparse_header {
	__le32    magic;      /* 0xed26ff3a */
	__le16    major_version;  /* (0x1) - reject images with higher major versions */
	__le16    minor_version;  /* (0x0) - allow images with higer minor versions */
	__le16    file_hdr_sz;    /* 28 bytes for first revision of the file format */
	__le16    chunk_hdr_sz;   /* 12 bytes for first revision of the file format */
	__le32    blk_sz;     /* block size in bytes, must be a multiple of 4 (4096) */
	__le32    total_blks; /* total blocks in the non-sparse output image */
	__le32    total_chunks;   /* total chunks in the sparse input image */
	__le32    image_checksum; /* CRC32 checksum of the original data, counting "don't care" */
	/* as 0. Standard 802.3 polynomial, use a Public Domain */
	/* table implementation */
} sparse_header_t;

#define SPARSE_HEADER_MAGIC 0xed26ff3a

#define CHUNK_TYPE_RAW      0xCAC1
#define CHUNK_TYPE_FILL     0xCAC2
#define CHUNK_TYPE_DONT_CARE    0xCAC3

typedef struct chunk_header {
	__le16    chunk_type; /* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
	__le16    reserved1;
	__le32    chunk_sz;   /* in blocks in output image */
	__le32    total_sz;   /* in bytes of chunk input file including chunk header and data */
} chunk_header_t;

#define SPARSE_HEADER_MAJOR_VER 1

/* Following a Raw or Fill chunk is data.  For a Raw chunk, it's the data in chunk_sz * blk_sz.
 *  For a Fill chunk, it's 4 bytes of the fill data.
 */
/* end sparse things */

#define USB_MAX_TRANS_SIZE  0x20000
struct cmd_fastboot_interface {

	/* A getvar string for the serial number
	   It can have a maximum of 60 characters

	   Set by board */
	char *serial_no;

	//fastboot likely to query partition-type before do flash.
	const disk_partition_t *pending_ptn;
	char pending_ptn_name[32];//see include/part.h

	/* Transfer buffer, for handling flash updates
	   Should be multiple of the block size
	   Care should be take so it does not overrun bootloader memory
	   Controlled by the configure variable CONFIG_FASTBOOT_TRANSFER_BUFFER

	   Set by board */
	u8 *buffer[2];
	u8 *transfer_buffer;
	
	//d_legacy set to 0, when we use double buffer to accelerate download.
	u8 d_legacy;

	/* How big is the transfer buffer
	   Controlled by the configure variable
	   CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE

	   Set by board	*/
	u64 transfer_buffer_size;

	/* Download size, if download has to be done. This can be checked to find
	   whether next packet is a command or a data */
	u64 d_size;

	/* Data downloaded so far */
	u64 d_bytes;

	/* Download status, < 0 when error, > 0 when complete */
	int d_status;

	/* Download size, copy downloaded data to storage */ 
	u64 d_direct_size;

	/* Offset of storage, will download 'd_direct_size' data to this offset */
	u64 d_direct_offset;

	/* May cache some data here. */
	unsigned char d_cache[USB_MAX_TRANS_SIZE + RK_BLK_SIZE];
	int d_cache_pos;

	/* Upload size, if download has to be done */
	u64 u_size;

	/* Data uploaded so far */
	u64 u_bytes;

	/* response with a NULL following to stop strlen() */
	char response[FASTBOOT_RESPONSE_SIZE];
	char null_term;

	/* Indicate response to be sent, data to be recieved */
	unsigned int flag;

	/* configuration status  */
	unsigned int configured;

	/* processing a command */
	unsigned int executing_command;

	unsigned int exit;

	unsigned int unlocked;
	unsigned long unlock_pending_start_time;

	/* device specific info */
	/*
	   unsigned int dev_info_uninitialized;
	   unsigned int num_device_info;
	   struct device_info dev_info[FASTBOOT_MAX_NUM_DEVICE_INFO];
	   */
	//TODO:maybe we need device info?

	/* sparse things */
	int flag_sparse;
	sparse_header_t sparse_header;
	int sparse_cur_chunk;
#if defined(CONFIG_RK_DWC3_UDC)
	u8 tx_buffer[512];
	u8 rx_buffer[512];
	uint8_t rxbuf_num;
	uint8_t txbuf_num;
	struct dwc3_giveback_data rx_giveback;
	struct dwc3_giveback_data tx_giveback;
#endif
};

/* Status values */
#define FASTBOOT_OK			0
#define FASTBOOT_ERROR			-1
#define FASTBOOT_DISCONNECT		1
#define FASTBOOT_INACTIVE		2


enum fbt_reboot_type {
	FASTBOOT_REBOOT_UNKNOWN,		/* typically for a cold boot */
	FASTBOOT_REBOOT_NORMAL,			/* normal */
	FASTBOOT_REBOOT_BOOTLOADER,		/* rockusb */
	FASTBOOT_REBOOT_RECOVERY,		/* recovery */
	FASTBOOT_REBOOT_RECOVERY_WIPE_DATA,	/* recovery and wipe data */
	FASTBOOT_REBOOT_NORECOVER,		/* do not enter recover */
	FASTBOOT_REBOOT_FASTBOOT,		/* android fastboot */
	FASTBOOT_REBOOT_CHARGE,			/* charge */
};


#ifdef CONFIG_FASTBOOT_LOG
extern int fbt_send_info(const char *info);
extern int fbt_log(const char *str, const int len, bool send);
#endif

int board_fbt_is_cold_boot(void);
int board_fbt_is_charging(void);
void board_fbt_set_reboot_type(enum fbt_reboot_type frt);
/* gets the reboot type, automatically clearing it for next boot */
enum fbt_reboot_type board_fbt_get_reboot_type(void);
int board_fbt_key_pressed(void);
void board_fbt_finalize_bootargs(char* args, int buf_sz, 
		int ramdisk_addr, int ramdisk_sz, int recovery);
void board_fbt_boot_failed(const char* boot);
#ifdef CONFIG_CMD_FASTBOOT
int board_fbt_oem(const char *cmdbuf);
int board_fbt_handle_erase(const disk_partition_t *ptn);
int board_fbt_handle_flash(const char *name, const disk_partition_t *ptn,
		struct cmd_fastboot_interface *priv);
int board_fbt_handle_download(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv);
#endif
int board_fbt_load_partition_table(void);
const disk_partition_t* board_fbt_get_partition(const char* name);
void board_fbt_run_recovery_wipe_data(void);
void board_fbt_preboot(void);


#define FBT_ERR
#undef  FBT_WARN
#undef  FBT_INFO
#undef  FBT_DEBUG

#ifdef FBT_DEBUG
#define FBTDBG(fmt, args...)\
	printf("DEBUG: [%s]: %d:\n"fmt, __func__, __LINE__, ##args)
#else
#define FBTDBG(fmt, args...) do {} while (0)
#endif

#ifdef FBT_INFO
#define FBTINFO(fmt, args...)\
	printf("INFO: [%s]: "fmt, __func__, ##args)
#else
#define FBTINFO(fmt, args...) do {} while (0)
#endif

#ifdef FBT_WARN
#define FBTWARN(fmt, args...)\
	printf("WARNING: [%s]: "fmt, __func__, ##args)
#else
#define FBTWARN(fmt, args...) do {} while (0)
#endif

#ifdef FBT_ERR
#define FBTERR(fmt, args...)\
	printf("ERROR: [%s]: "fmt, __func__, ##args)
#else
#define FBTERR(fmt, args...) do {} while (0)
#endif

#endif /* FASTBOOT_H */
