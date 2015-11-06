/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK_LOADER_H__
#define __RK_LOADER_H__


struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
};


int rkloader_CopyMemory2Flash(uint32 src_addr, uint32 dest_offset, int sectors);
int32 rkloader_CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec);

void rkloader_change_cmd_for_recovery(PBootInfo boot_info, char * rec_cmd);
int rkloader_run_misc_cmd(void);
void rkloader_fixInitrd(PBootInfo pboot_info, int ramdisk_addr, int ramdisk_sz);
int rkloader_set_bootloader_msg(struct bootloader_message* bmsg);

#endif /* __RK_LOADER_H__ */
