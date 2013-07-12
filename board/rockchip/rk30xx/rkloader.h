/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#ifndef RKLOADER_H
#define RKLOADER_H

#include <fastboot.h>
#include "../common/armlinux/config.h"

extern BootInfo gBootInfo;

typedef struct tag_rk_boot_img_hdr {
    struct fastboot_boot_img_hdr hdr;
    
    unsigned char reserved[0x400-0x260];
    unsigned long signTag; //0x4E474953
    unsigned long signlen; //128
    unsigned char rsaHash[128];
} rk_boot_img_hdr;

#define SECURE_BOOT_SIGN_TAG    0x4E474953

int secureCheck(struct fastboot_boot_img_hdr *hdr, int unlocked);
int fixHdr(struct fastboot_boot_img_hdr *hdr);
int getSn(char* buf);
void getParameter();
int setBootloaderMsg(struct bootloader_message* bmsg);
int checkMisc();
void ReSizeRamdisk(PBootInfo pboot_info,uint32 ImageSize);
int CopyMemory2Flash(uint32 src_addr, uint32 dest_offset, int sectors);
int32 CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec);
void SysLowFormatCheck(void);
void Switch2MSC(void);
void setup_space(uint32 begin_addr);
int get_bootloader_ver(char *boot_ver);
int execute_cmd(PBootInfo pboot_info, char* cmdlist, bool* reboot);

#endif /* RKLOADER_H */
