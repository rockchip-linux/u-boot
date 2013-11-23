/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#include "rkimage.h"
#include "rkloader.h"
#include "ext_fs.h"
#include "../common/common/crc/sha.h"

#undef ALIGN

#define ALIGN(x,a)      __ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

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

//base on ti's common/cmd_fastboot.c

static int unsparse(unsigned char *source,
                    lbaint_t sector, lbaint_t size_kb)
{
    sparse_header_t *header = (void *) source;
    u32 i;
    unsigned long blksz = RK_BLK_SIZE;
    u64 section_size = (u64)size_kb << 10;
    u64 outlen = 0;

    FBTDBG("sparse_header:\n");
    FBTDBG("\t         magic=0x%08X\n", header->magic);
    FBTDBG("\t       version=%u.%u\n", header->major_version,
                        header->minor_version);
    FBTDBG("\t file_hdr_size=%u\n", header->file_hdr_sz);
    FBTDBG("\tchunk_hdr_size=%u\n", header->chunk_hdr_sz);
    FBTDBG("\t        blk_sz=%u\n", header->blk_sz);
    FBTDBG("\t    total_blks=%u\n", header->total_blks);
    FBTDBG("\t  total_chunks=%u\n", header->total_chunks);
    FBTDBG("\timage_checksum=%u\n", header->image_checksum);

    if (header->magic != SPARSE_HEADER_MAGIC) {
        printf("sparse: bad magic\n");
        return 1;
    }

    if (((u64)header->total_blks * header->blk_sz) > section_size) {
        printf("sparse: section size %llu MB limit: exceeded\n",
                section_size/(1024*1024));
        return 1;
    }

    if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
        (header->file_hdr_sz != sizeof(sparse_header_t)) ||
        (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
        printf("sparse: incompatible format\n");
        return 1;
    }
    /* Skip the header now */
    source += header->file_hdr_sz;

    for (i = 0; i < header->total_chunks; i++) {
        u64 clen = 0;
        lbaint_t blkcnt;
        chunk_header_t *chunk = (void *) source;

        FBTDBG("chunk_header:\n");
        FBTDBG("\t    chunk_type=%u\n", chunk->chunk_type);
        FBTDBG("\t      chunk_sz=%u\n", chunk->chunk_sz);
        FBTDBG("\t      total_sz=%u\n", chunk->total_sz);
        /* move to next chunk */
        source += sizeof(chunk_header_t);

        switch (chunk->chunk_type) {
        case CHUNK_TYPE_RAW:
            clen = (u64)chunk->chunk_sz * header->blk_sz;
            FBTDBG("sparse: RAW blk=%d bsz=%d:"
                   " write(sector=%lu,clen=%llu)\n",
                   chunk->chunk_sz, header->blk_sz, sector, clen);

            if (chunk->total_sz != (clen + sizeof(chunk_header_t))) {
                printf("sparse: bad chunk size for"
                       " chunk %d, type Raw\n", i);
                return 1;
            }

            outlen += clen;
            if (outlen > section_size) {
                printf("sparse: section size %llu MB limit:"
                       " exceeded\n", section_size/(1024*1024));
                return 1;
            }
            blkcnt = clen / blksz;
            FBTDBG("sparse: RAW blk=%d bsz=%d:"
                   " write(sector=%lu,clen=%llu)\n",
                   chunk->chunk_sz, header->blk_sz, sector, clen);

            if (CopyMemory2Flash(source, sector, blkcnt)) {
                printf("sparse: block write to sector %lu"
                    " of %llu bytes (%ld blkcnt) failed\n",
                    sector, clen, blkcnt);
                return 1;
            }

            sector += (clen / blksz);
            source += clen;
            break;

        case CHUNK_TYPE_DONT_CARE:
            if (chunk->total_sz != sizeof(chunk_header_t)) {
                printf("sparse: bogus DONT CARE chunk\n");
                return 1;
            }
            clen = (u64)chunk->chunk_sz * header->blk_sz;
            FBTDBG("sparse: DONT_CARE blk=%d bsz=%d:"
                   " skip(sector=%lu,clen=%llu)\n",
                   chunk->chunk_sz, header->blk_sz, sector, clen);

            outlen += clen;
            if (outlen > section_size) {
                printf("sparse: section size %llu MB limit:"
                       " exceeded\n", section_size/(1024*1024));
                return 1;
            }
            sector += (clen / blksz);
            break;

        default:
            printf("sparse: unknown chunk ID %04x\n",
                   chunk->chunk_type);
            return 1;
        }
    }

    printf("sparse: out-length %llu MB\n", outlen/(1024*1024));
    return 0;
}


int handleFlash(fbt_partition_t *ptn, void *image_start_ptr, loff_t d_bytes)
{
    unsigned blocks;
    if (!ptn) return -1;
    if (!image_start_ptr)
        return handleErase(ptn);


    //base on ti's common/cmd_fastboot.c
    if (((sparse_header_t *)image_start_ptr)->magic
            == SPARSE_HEADER_MAGIC) {
        FBTDBG("fastboot: %s is in sparse format\n", ptn->name);
        return unsparse(image_start_ptr,
                    ptn->offset, ptn->size_kb);
    }

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

#define RSA_KEY_OFFSET 0x10//according to dumped data, the key is here.
#define RSA_KEY_LEN    0x102//258, public key's length
    char buf[RK_BLK_SIZE];
    memcpy(buf, new_loader + new_hdr->uiFlashBootOffset, RK_BLK_SIZE);
    P_RC4(buf, RK_BLK_SIZE);

    if (buf[RSA_KEY_OFFSET] != 0 || buf[RSA_KEY_OFFSET + 1] != 4) {
        FBTDBG("try to flash unsigned loader\n");
    }
    if (gDrmKeyInfo.publicKeyLen == 0) {
        FBTERR("current loader unsigned, allow flash loader anyway\n");
        return 0;
    }

#ifdef FBT_DEBUG
    printf("dump new loader's key:\n");
    for (i = 0;i < 32;i++) {
        for (j = 0;j < 16;j++) {
            printf("%02x", buf[RSA_KEY_OFFSET + i * 16 + j]);
        }
        printf("\n");
    }
#endif
    return memcmp(buf + RSA_KEY_OFFSET, gDrmKeyInfo.publicKey, RSA_KEY_LEN);
}

bool checkBootImageSha(rk_boot_img_hdr* boothdr)
{
    uint8_t* sha;
    SHA_CTX ctx;
    int size = SHA_DIGEST_SIZE > sizeof(boothdr->hdr.id) ? sizeof(boothdr->hdr.id) : SHA_DIGEST_SIZE;

    void *kernel_data = (void*)boothdr + boothdr->hdr.page_size;
    void *ramdisk_data = kernel_data + ALIGN(boothdr->hdr.kernel_size, boothdr->hdr.page_size);
    void *second_data = 0;
    if (boothdr->hdr.second_size) {
        second_data = kernel_data + ALIGN(boothdr->hdr.ramdisk_size, boothdr->hdr.page_size);
    }

    FBTDBG("compute real sha\n");

    SHA_init(&ctx);
    SHA_update(&ctx, kernel_data, boothdr->hdr.kernel_size);
    SHA_update(&ctx, &boothdr->hdr.kernel_size, sizeof(boothdr->hdr.kernel_size));
    SHA_update(&ctx, ramdisk_data, boothdr->hdr.ramdisk_size);
    SHA_update(&ctx, &boothdr->hdr.ramdisk_size, sizeof(boothdr->hdr.ramdisk_size));
    SHA_update(&ctx, second_data, boothdr->hdr.second_size);
    SHA_update(&ctx, &boothdr->hdr.second_size, sizeof(boothdr->hdr.second_size));

    //only rockchip's image do these.
    SHA_update(&ctx, &boothdr->hdr.tags_addr, sizeof(boothdr->hdr.tags_addr));
    SHA_update(&ctx, &boothdr->hdr.page_size, sizeof(boothdr->hdr.page_size));
    SHA_update(&ctx, &boothdr->hdr.unused, sizeof(boothdr->hdr.unused));
    SHA_update(&ctx, &boothdr->hdr.name, sizeof(boothdr->hdr.name));
    SHA_update(&ctx, &boothdr->hdr.cmdline, sizeof(boothdr->hdr.cmdline));

    sha = SHA_final(&ctx);


#ifdef FBT_DEBUG
    int i = 0;
    printf("\nreal sha:\n");
    for (i = 0;i < size;i++) {
        printf("%02x", (char)sha[i]);
    }
    printf("\nsha from image header:\n");
    for (i = 0;i < size;i++) {
        printf("%02x", ((char*)boothdr->hdr.id)[i]);
    }
    printf("\n");
#endif

    return !memcmp(boothdr->hdr.id, sha, size);
}

bool checkBootImageSign(rk_boot_img_hdr* boothdr)
{
    //flash boot/recovery.
    if (gDrmKeyInfo.publicKeyLen == 0) {
        FBTERR("current loader unsigned, allow flash anyway\n");
        return true;
    }
    if (!memcmp(boothdr->hdr.magic, FASTBOOT_BOOT_MAGIC, FASTBOOT_BOOT_MAGIC_SIZE)) {
        if (boothdr->signTag == SECURE_BOOT_SIGN_TAG) {
            //signed image, check with signature.
            //check sha here.
            if (!checkBootImageSha(boothdr)) {
                FBTERR("sha mismatch!\n");
                goto fail;
            }
            if (!SecureBootEn) {
                FBTERR("loader sign mismatch, not allowed to flash!\n");
                goto fail;
            }
            //check rsa sign here.
            if(SecureBootSignCheck(boothdr->rsaHash, boothdr->hdr.id,
                        boothdr->signlen) ==  FTL_OK) {
                return true;
            } else {
                FBTERR("signature mismatch!\n");
                goto fail;
            }
        } else {
            FBTERR("unsigned image!\n");
            goto fail;
        }
    } else {
        FBTERR("unrecognized image format!\n");
        goto fail;
    }
fail:
    return false;
}

bool checkUbootImageSha(second_loader_hdr* hdr)
{
    uint8_t* sha;
    SHA_CTX ctx;
    int size = SHA_DIGEST_SIZE > hdr->hash_len ? hdr->hash_len : SHA_DIGEST_SIZE;

    FBTDBG("compute real sha\n");

    SHA_init(&ctx);
    SHA_update(&ctx, (void*)hdr + sizeof(second_loader_hdr), hdr->loader_load_size);
    SHA_update(&ctx, &hdr->loader_load_addr, sizeof(hdr->loader_load_addr));
    SHA_update(&ctx, &hdr->loader_load_size, sizeof(hdr->loader_load_size));
    SHA_update(&ctx, &hdr->hash_len, sizeof(hdr->hash_len));
    sha = SHA_final(&ctx);

#ifdef FBT_DEBUG
    int i = 0;
    printf("\nreal sha:\n");
    for (i = 0;i < size;i++) {
        printf("%02x", (char)sha[i]);
    }
    printf("\nsha from image header:\n");
    for (i = 0;i < size;i++) {
        printf("%02x", ((char*)hdr->hash)[i]);
    }
    printf("\n");
#endif

    return !memcmp(hdr->hash, sha, size);
}


bool checkUbootImageSign(second_loader_hdr* hdr)
{
    //flash uboot.
    if (gDrmKeyInfo.publicKeyLen == 0) {
        FBTERR("current loader unsigned, allow flash anyway\n");
        return true;
    }
    if (!memcmp(hdr->magic, RK_UBOOT_MAGIC, sizeof(RK_UBOOT_MAGIC))) {
        if (hdr->signTag == RK_UBOOT_SIGN_TAG) {
            //signed image, check with signature.
            //check sha here.
            if (!checkUbootImageSha(hdr)) {
                FBTERR("sha mismatch!\n");
                goto fail;
            }
            if (!SecureBootEn) {
                FBTERR("loader sign mismatch, not allowed to flash!\n");
                goto fail;
            }
            //check rsa sign here.
            if(SecureBootSignCheck(hdr->rsaHash, hdr->hash,
                        hdr->signlen) ==  FTL_OK) {
                return true;
            } else {
                FBTERR("signature mismatch!\n");
                goto fail;
            }
        } else {
            FBTERR("unsigned image!\n");
            goto fail;
        }
    } else {
        FBTERR("unrecognized image format!\n");
        goto fail;
    }
fail:
    return false;
}

int handleRkFlash(char *name,
        struct cmd_fastboot_interface *priv)
{
    if (!strcmp(PARAMETER_NAME, name))
    {
        //flash parameter.
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
            getParameter();
            goto ok;
        }
        goto fail;
    } else if (!strcmp(LOADER_NAME, name))
    {
        //flash loader.
        int size = 0, ret = -1;
        if (make_loader_data(priv->transfer_buffer, g_pLoader, &size))
        {
            printf("err! make_loader_data failed(loader's key not match)\n");
            goto fail;
        }
        if (update_loader(true))
        {
            printf("err! update_loader failed\n");
            goto fail;
        }
        goto ok;
    } else if (priv->d_bytes > priv->transfer_buffer_size) {
        //flash large image with dma.
        if (!priv->pending_ptn) {
            goto fail;
        }
        FBTDBG("download large file, ptn:%s, target:%s\n", priv->pending_ptn->name, name);
        if (!strcmp(priv->pending_ptn->name, name) && priv->d_status > 0)
            goto ok;
        goto fail;
    } else if (!strcmp(priv->pending_ptn->name, RECOVERY_NAME) ||
            !strcmp(priv->pending_ptn->name, BOOT_NAME)) {
        //flash boot/recovery.
        if (!checkBootImageSign((rk_boot_img_hdr *)priv->transfer_buffer)) {
            goto fail;
        }
    } else if (!strcmp(priv->pending_ptn->name, UBOOT_NAME)) {
        //flash uboot
        if (!checkUbootImageSign((second_loader_hdr *)priv->transfer_buffer)) {
            goto fail;
        }
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

int handleDirectDownload(unsigned char *buffer, 
       int length, struct cmd_fastboot_interface *priv)
{
    int size = priv->d_direct_size;
    int avail_len = length - priv->transfer_buffer_pos;
    int write_len = 0;
    if (avail_len >= size) {
        write_len = size;
    } else {
        write_len = avail_len / RK_BLK_SIZE * RK_BLK_SIZE;
    }
    int blocks = DIV_ROUND_UP(write_len, RK_BLK_SIZE);

    FBTDBG("direct download, size:%d, offset:%lld, rest:%lld\n",
            size, priv->d_direct_offset, priv->d_direct_size - write_len);

	if(StorageWriteLba(priv->d_direct_offset + priv->pending_ptn->offset, buffer + priv->transfer_buffer_pos, blocks, 0)) {
        FBTDBG("handleDirectDownload failed\n");
        return -1;
    }
    priv->d_direct_offset += blocks;
    priv->d_direct_size -= write_len;
    priv->transfer_buffer_pos += write_len;
    return priv->d_direct_size;
}

void noBuffer(unsigned char *buffer,
        int length, struct cmd_fastboot_interface *priv)
{
    int rest = length - priv->transfer_buffer_pos;
    priv->d_bytes += priv->transfer_buffer_pos;
    priv->transfer_buffer_pos = RK_BLK_SIZE - rest;
    FBTDBG("rest:%d, len:%d, new pos:%lld\n", rest, length, priv->transfer_buffer_pos);
}

int handleImageDownload(unsigned char *buffer,
        int length, struct cmd_fastboot_interface *priv)
{
    if (!priv->d_direct_size) {
        return 0;
    }
    int ret = handleDirectDownload(buffer, length, priv);
    if (ret < 0) {
        priv->d_status = -1;
        return 0;
    } else if (ret > 0) {
        noBuffer(buffer, length, priv);
        return 1;
    }
    FBTDBG("image download compelete\n");
    priv->d_status = 1;
    return 0;
}

int handleSparseDownload(unsigned char *buffer,
        int length, struct cmd_fastboot_interface *priv)
{
    int ret;
    if (priv->d_direct_size) {
        //continue direct download
        ret = handleDirectDownload(buffer, length, priv);
        if (ret < 0) {
            priv->d_status = -1;
            return 0;
        } else if (ret > 0) {
            noBuffer(buffer, length, priv);
            return 1;
        }
    }
    chunk_header_t* chunk = (chunk_header_t*)calloc(sizeof(chunk_header_t), 1);
    sparse_header_t* header = &priv->sparse_header;
    u64 clen = 0;
    lbaint_t blkcnt;
    while (priv->sparse_cur_chunk < header->total_chunks) {
        ret = length - priv->transfer_buffer_pos;
        if (ret < sizeof(chunk_header_t)) {
            noBuffer(buffer, length, priv);
            return 1;
        }
        priv->sparse_cur_chunk++;
        memcpy(chunk, buffer + priv->transfer_buffer_pos, sizeof(chunk_header_t));
        priv->transfer_buffer_pos += sizeof(chunk_header_t);

        switch (chunk->chunk_type) {
            case CHUNK_TYPE_RAW:
                clen = (u64)chunk->chunk_sz * header->blk_sz;
                FBTDBG("sparse: RAW blk=%d bsz=%d:"
                        " write(sector=%lu,clen=%llu)\n",
                        chunk->chunk_sz, header->blk_sz, priv->d_direct_offset, clen);

                if (chunk->total_sz != (clen + sizeof(chunk_header_t))) {
                    printf("sparse: bad chunk size for"
                            " chunk %d, type Raw\n", priv->sparse_cur_chunk);
                    goto failed;
                }

                priv->d_direct_size = clen;

                ret = handleDirectDownload(buffer, length, priv);
                if (ret < 0) {
                    priv->d_status = -1;
                    return 0;
                } else if (ret > 0) {
                    noBuffer(buffer, length, priv);
                    return 1;
                }
                break;
            case CHUNK_TYPE_DONT_CARE:
                if (chunk->total_sz != sizeof(chunk_header_t)) {
                    printf("sparse: bogus DONT CARE chunk\n");
                    goto failed;
                }
                clen = (u64)chunk->chunk_sz * header->blk_sz;
                FBTDBG("sparse: DONT_CARE blk=%d bsz=%d:"
                        " skip(sector=%lu,clen=%llu)\n",
                        chunk->chunk_sz, header->blk_sz, priv->d_direct_offset, clen);

                priv->d_direct_offset += (clen / RK_BLK_SIZE);
                break;

            default:
                FBTERR("sparse: unknown chunk ID %04x\n",
                        chunk->chunk_type);
                goto failed;
        }
    }

    //complete
    priv->d_status = 1;
    return 0;

failed:
    priv->d_status = -1;
    return 0;
}

int startDownload(unsigned char *buffer,
        int length, struct cmd_fastboot_interface *priv)
{
    priv->d_status = 0;
    priv->flag_sparse = false;
    priv->d_direct_size = 0;
    priv->d_direct_offset = 0;
    priv->sparse_cur_chunk = 0;
    priv->transfer_buffer_pos = 0;

    //check sign before flash large boot/recovery.
    if (!strcmp(priv->pending_ptn->name, RECOVERY_NAME) ||
            !strcmp(priv->pending_ptn->name, BOOT_NAME)) {
        /*
        if (!checkImageSign((rk_boot_img_hdr *)buffer)) {
            priv->d_status = -1;
            return 0;
        }*/
        //should not reach here, check size before.
        FBTERR("boot/recovery image should not be so large.\n");
        priv->d_status = -1;
        return 0;
    }

    //check sparse image
    sparse_header_t* header = &priv->sparse_header;
    memcpy(header, buffer, sizeof(sparse_header_t));
    if ((header->magic == SPARSE_HEADER_MAGIC) &&
            (header->major_version == SPARSE_HEADER_MAJOR_VER) &&
            (header->file_hdr_sz == sizeof(sparse_header_t)) &&
            (header->chunk_hdr_sz == sizeof(chunk_header_t))) {
        priv->flag_sparse = true;
        priv->transfer_buffer_pos += sizeof(sparse_header_t);
        FBTDBG("found sparse image\n");
        return handleSparseDownload(buffer, length, priv);
    }

#if 1
    priv->d_direct_size = priv->d_size;
    return handleImageDownload(buffer, length, priv);
#else //only support ext image
    //check ext image
    filesystem* fs = (filesystem*) buffer;
    if (fs->sb.s_magic == EXT2_MAGIC_NUMBER ||
            fs->sb.s_magic == EXT3_MAGIC_NUMBER) {
        priv->d_direct_size = priv->d_size;
        FBTDBG("found ext image\n");
        return handleImageDownload(buffer, length, priv);
    }

    priv->d_status = -1;
    return 0;
#endif
}

int handleDownload(unsigned char *buffer,
        int length, struct cmd_fastboot_interface *priv)
{
    if (priv->d_size <= priv->transfer_buffer_size) {
        //nothing to do with these.
        return 0;
    }

    if (!priv->pending_ptn || !priv->pending_ptn->offset) {
        //fastboot flash with "-u" opt? or no parameter?
        if (!priv->pending_ptn) {
            FBTDBG("no pending_ptn\n");
        } else {
            FBTDBG("pending ptn(%s) offset:%x\n", priv->pending_ptn->name, priv->pending_ptn->offset);
        }
        priv->d_status = -1;
        return 0;
    }

    if ((length - priv->transfer_buffer_pos)/*rcved data len*/
            + priv->d_bytes < priv->d_size &&
            length < priv->transfer_buffer_size) {
        //keep downloading, util buffer is full or end of download.
        return 1;
    }

    if (priv->d_status) {
        //nothing to do with these.
        return 0;
    }

    if (!priv->d_bytes) {
        FBTDBG("start download, length:%d\n", length);
        return startDownload(buffer, length, priv);
    }

    buffer += priv->transfer_buffer_pos;
    length -= priv->transfer_buffer_pos;
    priv->transfer_buffer_pos = 0;

    FBTDBG("continue download, length:%d\n", length);
    if (priv->flag_sparse) {
        return handleSparseDownload(buffer, length, priv);
    } else {
        return handleImageDownload(buffer, length, priv);
    }
}
