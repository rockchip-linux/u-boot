/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 * Wenrui Li <wenrui.li@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __NVME_H
#define __NVME_H

extern int nvme_init(void);
extern int nvme_write(uint8 *buf, uint32 lba, uint32 nsec);
extern int nvme_read(uint8 *buf, uint32 lba, uint32 nsec);
extern int nvme_flush(void);
extern int nvme_get_capacity(void);

#endif /* __NVME_H */
