/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 * Wenrui Li <wenrui.li@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __NVME_H
#define __NVME_H

extern u32 nvme_init(u32 base);
extern u32 nvme_write(u8 chip, u32 lba, void *buf, u32 nsec, u32 mode);
extern u32 nvme_read(u8 chip, u32 lba, void *buf, u32 nsec);
extern int nvme_flush(void);
extern u32 nvme_get_capacity(u8);
extern void nvme_read_flash_info(void *buf);
#endif /* __NVME_H */
