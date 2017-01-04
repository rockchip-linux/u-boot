/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <malloc.h>

#include "../config.h"

DECLARE_GLOBAL_DATA_PTR;

#undef ALIGN

#define ALIGN(x,a)      __ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
extern uint8* g_pLoader;
extern void* ftl_memcpy(void* pvTo, const void* pvForm, unsigned int  size);

static int rkimg_load_image(uint32 offset, unsigned char *load_addr, size_t *image_size)
{
#ifdef CONFIG_RK_NVME_BOOT_EN
	ALLOC_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE, SZ_4K);
#else
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE);
#endif
	unsigned blocks;
	rk_kernel_image *image = (rk_kernel_image *)buf;
	unsigned head_offset = 8;//tag_rk_kernel_image's tag & size

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

	//read the rest blks: load image block = image size + (8 + 4)bytes, for head and crc32.
	blocks = DIV_ROUND_UP(*image_size + (8 + 4), RK_BLK_SIZE);
	if (rkloader_CopyFlash2Memory((uint32)(unsigned long) load_addr + RK_BLK_SIZE - head_offset, 
				offset + 1, blocks - 1) != 0) {
		printf("failed to read image\n");
		return -1;
	}

	return 0;
}


int rkimage_load_image(rk_boot_img_hdr *hdr, const disk_partition_t *boot_ptn, \
		const disk_partition_t *kernel_ptn)
{
	size_t image_size;
	if(!boot_ptn || !kernel_ptn) {
		return -1;
	}
#if !defined(CONFIG_RM_RAMDISK_LOAD)
	if (rkimg_load_image(boot_ptn->start, (unsigned char *)(unsigned long)hdr->ramdisk_addr, \
				&image_size) != 0) {
		printf("load boot image failed\n");
		return -1;
	}
	hdr->ramdisk_size = image_size;
#endif
	if (rkimg_load_image(kernel_ptn->start, (unsigned char *)(unsigned long)hdr->kernel_addr, \
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

				if (rkloader_CopyMemory2Flash((uint32)(unsigned long)source, sector, blkcnt)) {
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



static int rkimg_store_image(const disk_partition_t *ptn, void *image_start_ptr, loff_t d_bytes)
{
	unsigned blocks;
	if (!ptn) return -1;
	if (!image_start_ptr)
		return rkimage_partition_erase(ptn);


	//base on ti's common/cmd_fastboot.c
	if (((sparse_header_t *)image_start_ptr)->magic
			== SPARSE_HEADER_MAGIC) {
		FBTDBG("fastboot: %s is in sparse format\n", ptn->name);
		return unsparse(image_start_ptr,
				ptn->start, ptn->size * ptn->blksz >> 10);
	}

	blocks = DIV_ROUND_UP(d_bytes, RK_BLK_SIZE);
	return rkloader_CopyMemory2Flash((uint32)(unsigned long)image_start_ptr, ptn->start, blocks);
}


int rkimage_partition_erase(const disk_partition_t *ptn)
{
	if (!ptn) return -1;
	return StorageEraseBlock(ptn->start, ptn->size, 1);
}


#define PARAMETER_HEAD_OFFSET   8
static int rkimg_buildParameter(unsigned char *parameter, int len)
{
	int i;
	uint32 crc = crc32_rk(0, parameter, len);
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


static bool rkimg_make_loader_data(const char* old_loader, char* new_loader, int *new_loader_size)
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
		return false;

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
	new_hdr->ucRc4Flag = boot_hdr->ucRc4Flag;
	memcpy(new_loader+new_hdr->uiFlashDataOffset, old_loader+pFlashDataEntry->dwDataOffset, pFlashDataEntry->dwDataSize);
	memcpy(new_loader+new_hdr->uiFlashBootOffset, old_loader+pFlashBootEntry->dwDataOffset, pFlashBootEntry->dwDataSize);
	*new_loader_size = new_hdr->uiFlashBootOffset+new_hdr->uiFlashBootLen;
	//    dump_data(new_loader, HEADINFO_SIZE);

	return SecureModeVerifyLoader(new_hdr);
}


int rkimage_store_image(const char *name, const disk_partition_t *ptn,
		struct cmd_fastboot_interface *priv)
{
	if (!strcmp(PARAMETER_NAME, name))
	{
		//flash parameter.
		int i, ret = -1, len = 0;
		len = rkimg_buildParameter(priv->transfer_buffer, priv->d_bytes);

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
		}
		goto fail;
	} else if (!strcmp(LOADER_NAME, name))
	{
		//flash loader.
		int size = 0;
		if (!rkimg_make_loader_data((char *)priv->transfer_buffer, (char *)g_pLoader, &size))
		{
			printf("make loader data failed(loader's key not match)\n");
			goto fail;
		}
		if (rkidb_update_loader(true))
		{
			printf("update_loader failed\n");
			goto fail;
		}
		goto ok;
	} else if (!priv->d_legacy) {
		if (!priv->pending_ptn) {
			goto fail;
		}
		FBTDBG("accelerated download, ptn:%s, target:%s\n",
				priv->pending_ptn->name, name);
		if (!strcmp((const char*)priv->pending_ptn->name, name) && priv->d_status > 0)
			goto ok;
		goto fail;
	} else if (!strcmp((const char*)priv->pending_ptn->name, RECOVERY_NAME) ||
			!strcmp((const char*)priv->pending_ptn->name, BOOT_NAME)) {
		//flash boot/recovery.
		if (!SecureModeVerifyBootImage((rk_boot_img_hdr *)priv->transfer_buffer)) {
			goto fail;
		}
	} else if (!strcmp((const char*)priv->pending_ptn->name, UBOOT_NAME)) {
		//flash uboot
		if (!SecureModeVerifyUbootImage((second_loader_hdr *)priv->transfer_buffer)) {
			goto fail;
		}
	}
	//legacy case, let's flash it!
	if (!ptn) {
		FBTERR("ptn not found!!(%s)\n", name);
		goto fail;
	}
	if (rkimg_store_image(ptn, priv->transfer_buffer, priv->d_bytes) != 0)
		goto fail;
ok:
	return 0;
fail:
	return -1;
}

#define CONSUMED(buf, len, size) \
	{ \
		buf += size; len -= size; \
	}

//direct download to storage, and return wrote len.
static int rkimg_handleDirectDownload(unsigned char *buffer, 
		int length, struct cmd_fastboot_interface *priv)
{
	int size = priv->d_direct_size;
	int avail_len = length;
	int write_len = 0;
	if (avail_len >= size) {
		write_len = size;
	} else {
		write_len = avail_len / RK_BLK_SIZE * RK_BLK_SIZE;
	}
	int blocks = DIV_ROUND_UP(write_len, RK_BLK_SIZE);

	FBTDBG("direct download, size:%d, offset:%lld, rest:%lld\n",
			size, priv->d_direct_offset, priv->d_direct_size - write_len);

	/**
	 * Don't know why, but sometimes sparse image would have zero size blocks...
	 * After ignore it, everything seems fine...
	 */
	if(blocks && StorageWriteLba(priv->d_direct_offset + priv->pending_ptn->start,
				buffer, blocks, 0)) {
		FBTDBG("rkimg_handleDirectDownload failed\n");
		return -1;
	}
	priv->d_direct_offset += blocks;
	priv->d_direct_size -= write_len;
	return write_len;
}

static void rkimg_noBuffer(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
	if (length) {
		//cache rest buffer.
		memcpy(priv->d_cache, buffer, length);
		priv->d_cache_pos = length;
		FBTDBG("rest:%d\n", length);
	}
}

static int rkimg_ImageDownload(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
	if (!priv->d_direct_size) {
		//should not reach here.
		return 0;
	}
	int ret = rkimg_handleDirectDownload(buffer, length, priv);
	if (ret < 0) {
		priv->d_status = -1;
		return 0;
	}
	CONSUMED(buffer, length, ret);

	//not done yet!
	if (priv->d_direct_size) {
		rkimg_noBuffer(buffer, length, priv);
		return 1;
	}
	FBTDBG("image download compelete\n");
	priv->d_status = 1;
	return 0;
}

static int rkimg_handleSparseDownload(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
	int ret;
	if (priv->d_direct_size) {
		//continue direct download
		ret = rkimg_handleDirectDownload(buffer, length, priv);
		if (ret < 0) {
			priv->d_status = -1;
			return 0;
		}
		CONSUMED(buffer, length, ret);

		//not done yet!
	  	if (priv->d_direct_size) {
				rkimg_noBuffer(buffer, length, priv);
				return 1;
		}
	}
	chunk_header_t* chunk = (chunk_header_t*)calloc(sizeof(chunk_header_t), 1);
	sparse_header_t* header = &priv->sparse_header;
	u64 clen = 0;
	while (priv->sparse_cur_chunk < header->total_chunks) {
		if (length < sizeof(chunk_header_t)) {
			rkimg_noBuffer(buffer, length, priv);
			return 1;
		}
		priv->sparse_cur_chunk++;
		memcpy(chunk, buffer, sizeof(chunk_header_t));
		CONSUMED(buffer, length, sizeof(chunk_header_t));

		switch (chunk->chunk_type) {
			case CHUNK_TYPE_RAW:
				clen = (u64)chunk->chunk_sz * header->blk_sz;
				FBTDBG("sparse: RAW blk=%d bsz=%d:"
						" write(sector=%llu,clen=%llu)\n",
						chunk->chunk_sz, header->blk_sz,
						priv->d_direct_offset, clen);

				if (chunk->total_sz != (clen + sizeof(chunk_header_t))) {
					printf("sparse: bad chunk size for"
							" chunk %d, type Raw\n", priv->sparse_cur_chunk);
					goto failed;
				}

				priv->d_direct_size = clen;

				ret = rkimg_handleDirectDownload(buffer, length, priv);
				if (ret < 0) {
					priv->d_status = -1;
					return 0;
				}
				CONSUMED(buffer, length, ret);

				//not done yet!
				if (priv->d_direct_size) {
					rkimg_noBuffer(buffer, length, priv);
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
						" skip(sector=%llu,clen=%llu)\n",
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

static int rkimg_startDownload(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
#if 0
	printf("start download, receive:\n");
	int i = 0;
	for (i = 0; i < 512; i++) {
		if (!(i%8))
			printf("\n");
		printf("0x%02x ", buffer[i]);
	}
	printf("\n ---------------------------\n");
#endif
	priv->d_status = 0;
	priv->flag_sparse = false;
	priv->d_direct_size = 0;
	priv->d_direct_offset = 0;
	priv->sparse_cur_chunk = 0;
	priv->d_cache_pos = 0;

	//check sparse image
	sparse_header_t* header = &priv->sparse_header;
	memcpy(header, buffer, sizeof(sparse_header_t));
	if ((header->magic == SPARSE_HEADER_MAGIC) &&
			(header->major_version == SPARSE_HEADER_MAJOR_VER) &&
			(header->file_hdr_sz == sizeof(sparse_header_t)) &&
			(header->chunk_hdr_sz == sizeof(chunk_header_t))) {
		priv->flag_sparse = true;

		CONSUMED(buffer, length, sizeof(sparse_header_t));

		FBTDBG("found sparse image\n");
		return rkimg_handleSparseDownload(buffer, length, priv);
	}

#if 1
	priv->d_direct_size = priv->d_size;
	return rkimg_ImageDownload(buffer, length, priv);
#else //only support ext image
	//check ext image
	filesystem* fs = (filesystem*) buffer;
	if (fs->sb.s_magic == EXT2_MAGIC_NUMBER ||
			fs->sb.s_magic == EXT3_MAGIC_NUMBER) {
		priv->d_direct_size = priv->d_size;
		FBTDBG("found ext image\n");
		return rkimg_ImageDownload(buffer, length, priv);
	}

	priv->d_status = -1;
	return 0;
#endif
}

int rkimage_handleDownload(unsigned char *buffer,
		int length, struct cmd_fastboot_interface *priv)
{
	if (!priv->pending_ptn || !priv->pending_ptn->start) {
		//fastboot flash with "-u" opt? or no parameter?
		if (!priv->pending_ptn) {
			FBTDBG("no pending_ptn\n");
		} else {
			FBTDBG("pending ptn(%s) offset:%x\n", priv->pending_ptn->name,
					priv->pending_ptn->start);
		}
		priv->d_status = -1;
		return 0;
	}

	if (length + priv->d_bytes < priv->d_size &&
			length < priv->transfer_buffer_size) {
		//keep downloading, util buffer is full or end of download.
		return 1;
	}

	if (priv->d_status) {
		//nothing to do with these.
		return 0;
	}

	//start to download something.
	bool start = priv->d_bytes == 0;

	if (start) {
		FBTDBG("start download, length:%d\n", length);
		return rkimg_startDownload(buffer, length, priv);
	}

	//check buffer size.
	if (length + priv->d_cache_pos > sizeof(priv->d_cache)) {
		FBTDBG("something wrong with d_cache, pose:%d, len %d\n",
				priv->d_cache_pos, length);
		priv->d_status = -1;
		return 0;
	}

	if (priv->d_cache_pos) {
		memcpy(priv->d_cache + priv->d_cache_pos, buffer, length);
		length += priv->d_cache_pos;
		buffer = priv->d_cache;
		priv->d_cache_pos = 0;
		if (!priv->flag_sparse) {
			FBTERR("normal image should not get here!\n");
		}
	}

	FBTDBG("continue download, length:%d\n", length);
	if (priv->flag_sparse) {
		return rkimg_handleSparseDownload(buffer, length, priv);
	} else {
		return rkimg_ImageDownload(buffer, length, priv);
	}
}


#define FDT_PATH        "rk-kernel.dtb"
static const char* get_fdt_name(void)
{
	if (!gBootInfo.fdt_name[0]) {
		return FDT_PATH;
	}
	return gBootInfo.fdt_name;
}


resource_content rkimage_load_fdt(const disk_partition_t* ptn)
{
	resource_content content;
	snprintf(content.path, sizeof(content.path), "%s", get_fdt_name());
	content.load_addr = 0;

#ifndef CONFIG_RESOURCE_PARTITION
	return content;
#else
	if (!ptn)
		return content;

	if (!strcmp((char*)ptn->name, BOOT_NAME)
			|| !strcmp((char*)ptn->name, RECOVERY_NAME)) {
		//load from bootimg's second data area.
		unsigned long blksz = ptn->blksz;
		int offset = 0;
		rk_boot_img_hdr *hdr = NULL;
#ifdef CONFIG_RK_NVME_BOOT_EN
		hdr = memalign(SZ_4K, blksz << 2);
#else
		hdr = memalign(ARCH_DMA_MINALIGN, blksz << 2);
#endif
		if (StorageReadLba(ptn->start, (void *) hdr, 1 << 2) != 0) {
			return content;
		}
		if (!memcmp(hdr->magic, BOOT_MAGIC,
					BOOT_MAGIC_SIZE) && hdr->second_size) {
			//compute second data area's offset.
			offset = ptn->start + (hdr->page_size / blksz);
			offset += ALIGN(hdr->kernel_size, hdr->page_size) / blksz;
			offset += ALIGN(hdr->ramdisk_size, hdr->page_size) / blksz;

			if (get_content(offset, &content))
				load_content(&content);
		}
		return content;
	}
	//load from spec partition.
	if (get_content(ptn->start, &content))
		load_content(&content);
	return content;
#endif
}


resource_content rkimage_load_fdt_ram(void* addr, size_t len)
{
	resource_content content;
	snprintf(content.path, sizeof(content.path), "%s", get_fdt_name());
	content.load_addr = 0;

#ifndef CONFIG_RESOURCE_PARTITION
	return content;
#else
	if (!addr || !len)
		return content;

	get_content_ram(addr, len, &content);
	return content;
#endif
}


void rkimage_prepare_fdt(void)
{
	gd->fdt_blob = NULL;
	gd->fdt_size = 0;
#ifdef CONFIG_RESOURCE_PARTITION
	resource_content content = rkimage_load_fdt(get_disk_partition(BOOT_NAME));
	if (!content.load_addr) {
		debug("Failed to prepare fdt from boot!\n");
	} else {
		printf("Load FDT from boot image.\n");

		gd->fdt_blob = content.load_addr;
		gd->fdt_size = content.content_size;
		return;
	}
#ifdef CONFIG_OF_FROM_RESOURCE
	content = rkimage_load_fdt(get_disk_partition(RESOURCE_NAME));
	if (!content.load_addr) {
		debug("Failed to prepare fdt from resource!\n");
	} else {
		printf("Load FDT from resource image.\n");

		gd->fdt_blob = content.load_addr;
		gd->fdt_size = content.content_size;
		return;
	}
#endif
#endif
}

