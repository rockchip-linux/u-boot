/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */
#ifndef __RK_LOADER_H__
#define __RK_LOADER_H__


struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
};


int rkloader_secureCheck(rk_boot_img_hdr *hdr, int unlocked);
int rkloader_CopyMemory2Flash(uint32 src_addr, uint32 dest_offset, int sectors);
int32 rkloader_CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec);

void rkloader_change_cmd_for_recovery(PBootInfo boot_info, char * rec_cmd);
int rkloader_run_misc_cmd(void);
void rkloader_fixInitrd(PBootInfo pboot_info, int ramdisk_addr, int ramdisk_sz);
int rkloader_set_bootloader_msg(struct bootloader_message* bmsg);

#endif /* __RK_LOADER_H__ */
