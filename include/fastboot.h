/*
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

/* This is the interface file between the common cmd_fastboot.c and
   the board specific support.

   To use this interface, define CONFIG_FASTBOOT in your board config file.
   An example is include/configs/omap3430labrador.h
   ...
   #define CONFIG_FASTBOOT	        1    / * Using fastboot interface * /
   ...

   An example of the board specific spupport for omap3 is found at
   cpu/omap3/fastboot.c

*/

/* From fastboot client.. */
#define FASTBOOT_INTERFACE_CLASS     0xff
#define FASTBOOT_INTERFACE_SUB_CLASS 0x42
#define FASTBOOT_INTERFACE_PROTOCOL  0x03

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

struct cmd_fastboot_interface {

	/* A getvar string for the serial number
	   It can have a maximum of 60 characters

	   Set by board */
	char *serial_no;

	block_dev_desc_t *dev_desc;

	/* Transfer buffer, for handling flash updates
	   Should be multiple of the block size
	   Care should be take so it does not overrun bootloader memory
	   Controlled by the configure variable CONFIG_FASTBOOT_TRANSFER_BUFFER

	   Set by board */
	u8 *transfer_buffer;

	/* Pointer to the part of the transfer_buffer to use for
	   flashing.  Typically it's equal to transfer_buffer, but for
	   special cases, it might be some offset into the buffer (e.g.
	   for a multi-image binary).
	*/
	u8 *image_start_ptr;

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
};

/* Status values */
#define FASTBOOT_OK			0
#define FASTBOOT_ERROR			-1
#define FASTBOOT_DISCONNECT		1
#define FASTBOOT_INACTIVE		2

/* Android bootimage file format */
#define FASTBOOT_BOOT_MAGIC "ANDROID!"
#define FASTBOOT_BOOT_MAGIC_SIZE 8
#define FASTBOOT_BOOT_NAME_SIZE 16
#define FASTBOOT_BOOT_ARGS_SIZE 512


/* in a board specific file */
struct fbt_partition {
    const char *name;
    unsigned offset;
    unsigned size_kb;
};

#define FBT_PARTITION_MAX_NUM 16

struct fastboot_boot_img_hdr {
	unsigned char magic[FASTBOOT_BOOT_MAGIC_SIZE];

	unsigned kernel_size;  /* size in bytes */
	unsigned kernel_addr;  /* physical load addr */

	unsigned ramdisk_size; /* size in bytes */
	unsigned ramdisk_addr; /* physical load addr */

	unsigned second_size;  /* size in bytes */
	unsigned second_addr;  /* physical load addr */

	unsigned tags_addr;    /* physical addr for kernel tags */
	unsigned page_size;    /* flash page size we assume */
	unsigned unused[2];    /* future expansion: should be 0 */

	unsigned char name[FASTBOOT_BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[FASTBOOT_BOOT_ARGS_SIZE];

	unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};

#ifdef	CONFIG_CMD_FASTBOOT
enum fbt_reboot_type {
	FASTBOOT_REBOOT_UNKNOWN, /* typically for a cold boot */
	FASTBOOT_REBOOT_NORMAL,
	FASTBOOT_REBOOT_BOOTLOADER,
	FASTBOOT_REBOOT_RECOVERY,
	FASTBOOT_REBOOT_NONE,
	FASTBOOT_REBOOT_RECOVERY_WIPE_DATA,
};
extern void fbt_preboot(void);
extern int fbt_send_info(const char *info);

int board_fbt_oem(const char *cmdbuf);
void board_fbt_set_reboot_type(enum fbt_reboot_type frt);
/* gets the reboot type, automatically clearing it for next boot */
enum fbt_reboot_type board_fbt_get_reboot_type(void);
int board_fbt_key_pressed(void);
void board_fbt_finalize_bootargs(char* args, size_t buf_sz);

#endif /* CONFIG_CMD_FASTBOOT */
#endif /* FASTBOOT_H */
