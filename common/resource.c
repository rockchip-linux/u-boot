/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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

/*
 * charge animation functions
 */
#include <common.h>
#include <command.h>
#include <linux/sizes.h>

#include <resource.h>
#include <fastboot.h>
#include <malloc.h>
#include <../board/rockchip/common/config.h>

#include <fdt_support.h>
#include <lcd.h>

static bool inline read_storage(lbaint_t offset, void* buf, uint16_t blocks) {
#if 1
	return !StorageReadLba(offset, buf, blocks);
#else
	//we may use block_read in the future.
	block_dev_desc_t* blkdev = get_dev_by_name("mmc0");
	int read = blkdev->block_read(blkdev->dev, offset,
			blocks, buf);
	return read == blocks;
#endif
}

static int inline get_base_offset(void) {
	const disk_partition_t* ptn;
#ifdef CONFIG_ROCKCHIP
	ptn	= get_disk_partition(RESOURCE_NAME);
#else
	//TODO: find disk_partition_t in other way.
	ptn = NULL;
#endif
	if (!ptn) {
		FBTDBG("%s ptn not found.\n", RESOURCE_NAME);

		/* if no resource partition, load logo from boot partition second data area */
		ptn = get_disk_partition(BOOT_NAME);
		if (ptn != NULL) {
			unsigned long blksz = ptn->blksz;
			int offset = 0;
			rk_boot_img_hdr *hdr = NULL;

			hdr = memalign(ARCH_DMA_MINALIGN, blksz << 2);
			if (StorageReadLba(ptn->start, (void *) hdr, 1 << 2) != 0) {
				return 0;
			}
			//load from bootimg's second data area.
			if (!memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) && hdr->second_size) {
				//compute second data area's offset.
				offset = ptn->start + (hdr->page_size / blksz);
				offset += ALIGN(hdr->kernel_size, hdr->page_size) / blksz;
				offset += ALIGN(hdr->ramdisk_size, hdr->page_size) / blksz;

				return offset;
			}
		}

		return 0;
	}
	return ptn->start;
}

static bool get_entry_ram(resource_ptn_header header, void *table,
		size_t table_len, const char* file_path,
		index_tbl_entry* entry) {
	bool ret = false;

	if (memcmp(header.magic, RESOURCE_PTN_HDR_MAGIC,
				sizeof(header.magic))) {
		FBTERR("Not a resource image!\n");
		goto end;
	}

	//TODO: support header_size & tbl_entry_size
	if (header.resource_ptn_version != RESOURCE_PTN_VERSION
			|| header.header_size != RESOURCE_PTN_HDR_SIZE
			|| header.index_tbl_version != INDEX_TBL_VERSION
			|| header.tbl_entry_size != INDEX_TBL_ENTR_SIZE) {
		FBTERR("Not supported in this version!\n");
		goto end;
	}

	if (header.tbl_entry_num * header.tbl_entry_size <= 0xFFFF) {
		if (!table)
			goto end;
		if (table_len < (header.tbl_entry_num
				* header.tbl_entry_size * BLOCK_SIZE))
			goto end;
	}
	int i;
	for (i = 0; i < header.tbl_entry_num; i++) {
		//TODO: support tbl_entry_size
		memcpy(entry, table + i * header.tbl_entry_size * BLOCK_SIZE,
				sizeof(*entry));

		if (memcmp(entry->tag, INDEX_TBL_ENTR_TAG,
					sizeof(entry->tag))) {
			FBTERR("Something wrong with index entry:%d!\n", i);
			goto end;
		}

		FBTDBG("Lookup entry(%d):\n\tpath:%s\n\toffset:%d\tsize:%d\n",
				i, entry->path, entry->content_offset,
				entry->content_size);

		if (!strncmp(entry->path, file_path, sizeof(entry->path)))
			break;
	}
	if (i == header.tbl_entry_num) {
		FBTERR("Cannot find %s!\n", file_path);
		goto end;
	}

	FBTDBG("Found entry:\n\tpath:%s\n\toffset:%d\tsize:%d\n",
			entry->path, entry->content_offset,
			entry->content_size);

	ret = true;
end:
	return ret;
}

bool get_content_ram(void *buf, size_t len,
		resource_content* content) {
	bool ret = false;
	index_tbl_entry entry;
	resource_ptn_header header;
	size_t header_size;

	if (!buf)
		goto end;

	memcpy(&header, buf, sizeof(header));

	header_size = header.header_size * BLOCK_SIZE;

	if (!get_entry_ram(header, buf + header_size,
				len - header_size,
				content->path, &entry))
		goto end;

	content->content_offset = 0;
	content->content_size = entry.content_size;
	content->load_addr = buf + entry.content_offset * BLOCK_SIZE;

	ret = true;
end:
	return ret;
}

static bool get_entry(int base_offset, const char* file_path,
		index_tbl_entry* entry) {
	bool ret = false;
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, BLOCK_SIZE);
	char* table = NULL;
	resource_ptn_header header;
	if (!base_offset) {
		base_offset = get_base_offset();
	}
	if (!base_offset) {
		FBTERR("base offset is NULL!\n");
		goto end;
	}
	if (!read_storage(base_offset, buf, 1)) {
		FBTERR("Failed to read header!\n");
		goto end;
	}
	memcpy(&header, buf, sizeof(header));

	if (memcmp(header.magic, RESOURCE_PTN_HDR_MAGIC,
				sizeof(header.magic))) {
		FBTERR("Not a resource image!\n");
		goto end;
	}

	//TODO: support header_size & tbl_entry_size
	if (header.resource_ptn_version != RESOURCE_PTN_VERSION
			|| header.header_size != RESOURCE_PTN_HDR_SIZE
			|| header.index_tbl_version != INDEX_TBL_VERSION
			|| header.tbl_entry_size != INDEX_TBL_ENTR_SIZE) {
		FBTERR("Not supported in this version!\n");
		goto end;
	}

	if (header.tbl_entry_num * header.tbl_entry_size <= 0xFFFF) {
		table = (char *)memalign(ARCH_DMA_MINALIGN,
				header.tbl_entry_num
				* header.tbl_entry_size * BLOCK_SIZE);
		if (!table)
			goto end;
		if (!read_storage(base_offset + header.header_size, table,
					header.tbl_entry_num
					* header.tbl_entry_size)) {
			FBTERR("Failed to read index entries!\n");
			goto end;
		}
	}
	ret = get_entry_ram(header, table, header.tbl_entry_num
			* header.tbl_entry_size * BLOCK_SIZE,
			file_path, entry);
end:
	if (table) {
		free(table);
	}
	return ret;
}

bool get_content(int base_offset, resource_content* content) {
	bool ret = false;
	index_tbl_entry entry;
	if (!base_offset) {
		base_offset = get_base_offset();
	}
	if (!base_offset) {
		FBTERR("base offset is NULL!\n");
		goto end;
	}
	if (!get_entry(base_offset, content->path, &entry))
		goto end;
	content->content_offset = entry.content_offset + base_offset;
	content->content_size = entry.content_size;
	ret = true;
end:
	return ret;
}

void free_content(resource_content* content) {
	if (content->content_offset && content->load_addr) {
		free(content->load_addr);
		content->load_addr = 0;
	}
}

bool load_content(resource_content* content) {
	if (content->load_addr || !content->content_offset)
		return true;

	int blocks = (content->content_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
	content->load_addr = (void*)memalign(ARCH_DMA_MINALIGN, blocks * BLOCK_SIZE);
	if (!content->load_addr)
		return false;
	if (!load_content_data(content, 0,
				content->load_addr, blocks)) {
		free_content(content);
		return false;
	}
	return true;
}

bool load_content_data(resource_content* content,
		int offset_block, void* data, int blocks) {
	if (!content->content_offset)
		return false;

	if (!read_storage(content->content_offset + offset_block, data, blocks)) {
		return false;
	}
	return true;
}

bool show_resource_image(const char* image_path) {
	bool ret = false;
#ifdef CONFIG_LCD
	bmp_image_t *bmp = NULL;
	const disk_partition_t* ptn = get_disk_partition(LOGO_NAME);
	resource_content image;
	memset(&image, 0, sizeof(image));
	snprintf(image.path, sizeof(image.path), "%s", image_path);

	if(ptn)
	{
		//bmp = (void*)memalign(ARCH_DMA_MINALIGN, CONFIG_MAX_BMP_BLOCKS * BLOCK_SIZE);
		bmp = (void *)gd->arch.rk_boot_buf_addr;
		read_storage(ptn->start, bmp, CONFIG_MAX_BMP_BLOCKS);
		printf("bmp image at 0x%lx, sign:%c%c\n", bmp, bmp->header.signature[0], bmp->header.signature[1]);
	}

	if(ptn && bmp && bmp->header.signature[0] == 'B' && bmp->header.signature[1] == 'M')
	{
		printf("%s:show logo.bmp from logo partition\n",__func__);
		lcd_display_bitmap_center((uint32_t)bmp);
		ret = true;
	}
	else
	{
		if (get_content(0, &image)) {
		printf("%s:show logo from resource or boot partition\n",__func__);
		int blocks = (image.content_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

		if (image.content_size > CONFIG_RK_BOOT_BUFFER_SIZE) {
			FBTERR("Failed to bmp image too large, %d\n",
			       image.content_size);
			return false;
		}

		image.load_addr = (void *)gd->arch.rk_boot_buf_addr;
		if (!load_content_data(&image, 0, image.load_addr, blocks)) {
			return false;
		}
		FBTDBG("Try to show:%s\n", image_path);
		lcd_display_bitmap_center((uint32_t)image.load_addr);

		ret = true;
		} else {
			FBTERR("Failed to load image:%s\n", image_path);
		}
	}

#endif
	return ret;
}

