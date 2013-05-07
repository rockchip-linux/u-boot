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

#define PARAMETER_HEAD_OFFSET   8
static int buildParameter(unsigned char *parameter, int len)
{
    int i;
    uint32 crc = crc32(0, parameter, len);
    for(i=0; i<4; i++)
    {
        parameter[len + i] = crc&0xFF;
        crc >>= 8;
    }

    memmove(parameter + PARAMETER_HEAD_OFFSET, parameter, len + 4);
    PLoaderParam param = (PLoaderParam)
        (parameter);

    param->tag = PARM_TAG;
    param->length = len;
    return len + PARAMETER_HEAD_OFFSET;
}

static int make_loader_data(const char* old_loader, char* new_loader, int *new_loader_size)//path, RKIMAGE_HDR *hdr)
{
    int i,j;
    PSTRUCT_RKBOOT_ENTRY pFlashDataEntry = NULL;
    PSTRUCT_RKBOOT_ENTRY pFlashBootEntry = NULL;
    STRUCT_RKBOOT_HEAD *boot_hdr = NULL;
    RK28BOOT_HEAD *new_hdr = NULL;

    boot_hdr = (STRUCT_RKBOOT_HEAD*)old_loader;

// µÃµ½FlashData/FlashBootÊý¾Ý¿éµÄÐÅÏ¢
    for (i=0;i<boot_hdr->ucLoaderEntryCount;i++)
    {
        PSTRUCT_RKBOOT_ENTRY pEntry;
        pEntry = (PSTRUCT_RKBOOT_ENTRY)(old_loader+boot_hdr->dwLoaderEntryOffset+(boot_hdr->ucLoaderEntrySize*i));
        char name[10] = "";
        for(j=0; j<20 && pEntry->szName[j]; j+=2)
            name[j/2] = pEntry->szName[j];
        if( !strcmp( name, "FlashData" ) )
            pFlashDataEntry = pEntry;
        else if( !strcmp( name, "FlashBoot" ) )
            pFlashBootEntry = pEntry;
    }
    if(pFlashDataEntry == NULL || pFlashBootEntry == NULL)
        return -1;

// ¹¹ÔìÐÂµÄLoaderÊý¾Ý£¬ÒÔ´«¸øLoader½øÐÐ±¾µØÉý¼¶
    new_hdr = (RK28BOOT_HEAD*)new_loader;
    memset(new_hdr, 0, HEADINFO_SIZE);
    strcpy(new_hdr->szSign, BOOTSIGN);
    new_hdr->tmCreateTime.usYear = boot_hdr->stReleaseTime.usYear;
    new_hdr->tmCreateTime.usMonth= boot_hdr->stReleaseTime.ucMonth;
    new_hdr->tmCreateTime.usDate= boot_hdr->stReleaseTime.ucDay;
    new_hdr->tmCreateTime.usHour = boot_hdr->stReleaseTime.ucHour;
    new_hdr->tmCreateTime.usMinute = boot_hdr->stReleaseTime.ucMinute;
    new_hdr->tmCreateTime.usSecond = boot_hdr->stReleaseTime.ucSecond;
    new_hdr->uiMajorVersion = (boot_hdr->dwVersion&0x0000FF00)>>8;
    new_hdr->uiMajorVersion = BCD2INT(new_hdr->uiMajorVersion);
    new_hdr->uiMinorVersion = boot_hdr->dwVersion&0x000000FF;
    new_hdr->uiMinorVersion = BCD2INT(new_hdr->uiMinorVersion);
    new_hdr->uiFlashDataOffset = HEADINFO_SIZE;
    new_hdr->uiFlashDataLen = pFlashDataEntry->dwDataSize;
    new_hdr->uiFlashBootOffset = new_hdr->uiFlashDataOffset+new_hdr->uiFlashDataLen;
    new_hdr->uiFlashBootLen = pFlashBootEntry->dwDataSize;
    memcpy(new_loader+new_hdr->uiFlashDataOffset, old_loader+pFlashDataEntry->dwDataOffset, pFlashDataEntry->dwDataSize);
    memcpy(new_loader+new_hdr->uiFlashBootOffset, old_loader+pFlashBootEntry->dwDataOffset, pFlashBootEntry->dwDataSize);
    *new_loader_size = new_hdr->uiFlashBootOffset+new_hdr->uiFlashBootLen;
//    dump_data(new_loader, HEADINFO_SIZE);

    return 0;
}

int handleRkFlash(char *name,
        struct cmd_fastboot_interface *priv)
{
    if (!strcmp(PARAMETER_NAME, name))
    {
        int i, ret = -1, len = 0;
        PLoaderParam param = (PLoaderParam) 
            (priv->transfer_buffer + priv->d_bytes);

        len = buildParameter(priv->transfer_buffer, priv->d_bytes);
        
        printf("Write parameter\n");
        for(i=0; i<PARAMETER_NUM; i++)
        {
            if (!StorageWriteLba(i*PARAMETER_OFFSET, priv->transfer_buffer, 
                    DIV_ROUND_UP(len, RK_BLK_SIZE), 0))
            {
                ret = 0;
            }
        }
        if (!ret)
        {
            goto ok;
        } else
        {
            goto fail;
        }
        return 1;
    }
    if (!strcmp(LOADER_NAME, name))
    {
        int size = 0, ret = -1;
        if (make_loader_data(priv->transfer_buffer, g_pLoader, &size))
        {
            printf("err! make_loader_data failed\n");
            goto fail;
        }
        if (update_loader(true))
        {
            printf("err! update_loader failed\n");
            goto fail;
        }
        goto ok;
    }
    return 0;
ok:
    sprintf(priv->response, "OKAY");
    return 1;
fail:
    sprintf(priv->response,
            "FAILWrite partition");
    return -1;


}

