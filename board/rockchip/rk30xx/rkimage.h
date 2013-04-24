#ifndef RKIMAGE_H
#define RKIMAGE_H

#include <fastboot.h>
#include "../common/armlinux/config.h"

#define TAG_KERNEL          0x4C4E524B

extern BootInfo gBootInfo;

typedef struct tagKernelImg {
    uint32  tag;
    uint32  size;
    char    image[1];
    uint32  crc;
}KernelImg;

static int loadImage(uint32 offset, unsigned char *load_addr, size_t *image_size);

int loadRkImage(struct fastboot_boot_img_hdr *hdr, fbt_partition_t *boot_ptn, \
        fbt_partition_t *kernel_ptn);

#endif /* RKIMAGE_H */
