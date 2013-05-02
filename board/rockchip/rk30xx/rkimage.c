#include "rkimage.h"
#include "rkloader.h"

static int loadImage(uint32 offset, unsigned char *load_addr, size_t *image_size)
{
    unsigned char buf[RK_BLK_SIZE];
    unsigned blocks;
    KernelImg *image = (KernelImg*)buf;
    unsigned head_offset = 8;//tagKernelImg's tag & size
    if (StorageReadLba(offset, (void *) image, 1) != 0) {
        printf("failed to read image header\n");
        return -1;
    }
    if(image->tag != TAG_KERNEL) {
        printf("bad image magic.\n");
        return -1;
    }
    *image_size = image->size;
    //image not align to blk size, so should memcpy some.
    ftl_memcpy((void *)load_addr, image->image, RK_BLK_SIZE - head_offset);

    //read the rest blks.
    blocks = DIV_ROUND_UP(*image_size, RK_BLK_SIZE);
    if (CopyFlash2Memory((void *) load_addr + RK_BLK_SIZE - head_offset, 
                offset + 1, blocks - 1) != 0) {
        printf("failed to read image\n");
        return -1;
    }

    return 0;
}

int loadRkImage(struct fastboot_boot_img_hdr *hdr, fbt_partition_t *boot_ptn, \
        fbt_partition_t *kernel_ptn)
{
    size_t image_size;
    if(!boot_ptn || !kernel_ptn) {
        return -1;
    }
    snprintf(hdr->magic, FASTBOOT_BOOT_MAGIC_SIZE, "%s\n", "RKIMAGE!");

    hdr->ramdisk_addr = gBootInfo.ramdisk_load_addr;
    if (loadImage(boot_ptn->offset, (unsigned char *)hdr->ramdisk_addr, \
                &image_size) != 0) {
        printf("load boot image failed\n");
        return -1;
    }
    hdr->ramdisk_size = image_size;

    hdr->kernel_addr = gBootInfo.kernel_load_addr;
    if (loadImage(kernel_ptn->offset, (unsigned char *)hdr->kernel_addr, \
                &image_size) != 0) {
        printf("load kernel image failed\n");
        return -1;
    }
    hdr->kernel_size = image_size;
    return 0;
}

int handleFlash(fbt_partition_t *ptn, void *image_start_ptr, loff_t d_bytes)
{
    unsigned blocks;
    if (!ptn) return -1;
    if (!image_start_ptr)
        return handleErase(ptn);
    blocks = DIV_ROUND_UP(d_bytes, RK_BLK_SIZE);
    return CopyMemory2Flash(image_start_ptr, ptn->offset, blocks);
}

int handleErase(fbt_partition_t *ptn)
{
    unsigned blocks;
    if (!ptn) return -1;
    blocks = DIV_ROUND_UP(ptn->size_kb << 10, RK_BLK_SIZE);
    return StorageEraseBlock(ptn->offset, blocks, 1);
}
