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
#include <asm/sizes.h>

#include <resource.h>
#include <fastboot.h>
#include <malloc.h>
#include <../board/rockchip/common/config.h>
#include <../board/rockchip/common/storage.h>

static int inline get_ptn_offset(void) {
	const disk_partition_t* ptn = fastboot_find_ptn(RESOURCE_NAME);
	if (!ptn)
		return 0;
	return ptn->start;
}

static bool get_entry(const char* file_path, index_tbl_entry* entry) {
	bool ret = false;
	char buf[BLOCK_SIZE];
	char* cache = NULL;
	resource_ptn_header header;
	int ptn_offset = get_ptn_offset();
	if (!ptn_offset) {
		FBTERR("%s ptn not found.\n", RESOURCE_NAME);
		goto end;
	}
	if (StorageReadLba(ptn_offset, buf, 1)) {
		FBTERR("Failed to read header!\n");
		goto end;
	}
	memcpy(&header, buf, sizeof(header));

	if (memcmp(header.magic, RESOURCE_PTN_HDR_MAGIC, sizeof(header.magic))) {
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
		cache = (char*) malloc(header.tbl_entry_num * header.tbl_entry_size * BLOCK_SIZE);
		if (cache) {
			if (StorageReadLba(ptn_offset + header.header_size, cache,
						header.tbl_entry_num * header.tbl_entry_size)) {
				FBTERR("Failed to read index entries!\n");
				goto end;
			}
		}
	}
	int i;
	for (i = 0; i < header.tbl_entry_num; i++) {
		//TODO: support tbl_entry_size
		if (!cache) {
			if (StorageReadLba(ptn_offset +
						header.header_size + i * header.tbl_entry_size, buf, 1)) {
				FBTERR("Failed to read index entry:%d!\n", i);
				goto end;
			}
			memcpy(entry, buf, sizeof(*entry));
		} else {
			memcpy(entry, cache + i * header.tbl_entry_size * BLOCK_SIZE,
					sizeof(*entry));
		}

		if (memcmp(entry->tag, INDEX_TBL_ENTR_TAG, sizeof(entry->tag))) {
			FBTERR("Something wrong with index entry:%d!\n", i);
			goto end;
		}

		FBTDBG("Lookup entry(%d):\n\tpath:%s\n\toffset:%d\tsize:%d\n",i ,
				entry->path, entry->content_offset, entry->content_size);

		if (!strncmp(entry->path, file_path, sizeof(entry->path)))
			break;
	}
	if (i == header.tbl_entry_num) {
		FBTERR("Cannot find %s!\n", file_path);
		goto end;
	}

	FBTDBG("Found entry:\n\tpath:%s\n\toffset:%d\tsize:%d\n",
			entry->path, entry->content_offset, entry->content_size);

	ret = true;
end:
	if (cache) {
		free(cache);
	}
	return ret;
}

bool get_content(resource_content* content) {
	bool ret = false;
	index_tbl_entry entry;
	if (!get_entry(content->path, &entry))
		goto end;
	content->content_offset = entry.content_offset;
	content->content_size = entry.content_size;
	ret = true;
end:
	return ret;
}

void free_content(resource_content* content) {
	if (content->load_addr) {
		free(content->load_addr);
		content->load_addr = 0;
	}
}

bool load_content(resource_content* content) {
	if (content->load_addr)
		return true;

	int blocks = (content->content_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
	content->load_addr = (void*)malloc(blocks * BLOCK_SIZE);
	if (!content->load_addr)
		return false;
	if (StorageReadLba(get_ptn_offset() +
				content->content_offset, content->load_addr, blocks)) {
		free_content(content);
		return false;
	}
	return true;
}

bool load_content_data(resource_content* content,
		int offset_block, void* data, int blocks) {
	int ptn_offset = get_ptn_offset();
	if (!ptn_offset) {
		return false;
	}
	if (StorageReadLba(get_ptn_offset() +
				content->content_offset + offset_block, data, blocks)) {
		return false;
	}
	return true;
}

#if 0
void test_content() {
	const char* file_path = "git.diff";
	int offset_block = 0;
	int blocks = 1;
	resource_content content;
	snprintf(content.path, sizeof(content.path), "%s", file_path);
	content.load_addr = 0;
	if (!get_content(&content)) {
		return;
	}
	if (!blocks) {
		if (!load_content(&content)) {
			goto end;
		}
		((char*)content.load_addr)[content.content_size - 1] = '\0';
		printf("%s\n", content.load_addr);
	} else {
		void* data = malloc(blocks * BLOCK_SIZE);
		if (!data)
			goto end;
		if (!load_content_data(&content, offset_block, data, blocks)) {
			goto end;
		}
		((char*)data)[blocks * BLOCK_SIZE - 1] = '\0';
		printf("%s\n", data);
	}
end:
	free_content(&content);
}
#endif

