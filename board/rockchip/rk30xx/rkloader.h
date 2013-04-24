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

void checkBoot(struct fastboot_boot_img_hdr *hdr);

#endif /* RKLOADER_H */
