/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef __ROCKCHIP_COMMON_H
#define __ROCKCHIP_COMMON_H

#include <common.h>

extern struct bootm_headers images;

enum {
	BCB_MSG_RECOVERY_NONE,
	BCB_MSG_RECOVERY_RK_FWUPDATE,
	BCB_MSG_RECOVERY_PCBA,
};

#define RK_BLK_SIZE			512

void bootargs_setup(void);
void rockusb_download(void);
void rbrom_download(void);
int usb_boot_init(void);
int misc_get_recovery_msg(void);

int rockchip_read_dtb_file(void *fdt_addr);
int rockchip_ram_read_dtb_file(void *img, void *fdt_addr);

int fit_write_optee_rollback_index(u32 optee_index);
int arch_fpga_init(void);
#endif

