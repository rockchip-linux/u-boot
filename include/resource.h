/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef RESOURCE_H
#define RESOURCE_H

#define BLOCK_SIZE                  512

#define RESOURCE_PTN_HDR_SIZE       1
#define INDEX_TBL_ENTR_SIZE         1

#define RESOURCE_PTN_VERSION        0
#define INDEX_TBL_VERSION           0

#define RESOURCE_PTN_HDR_MAGIC      "RSCE"
typedef struct {
	char     magic[4];//tag, "RSCE"
	uint16_t resource_ptn_version;
	uint16_t index_tbl_version;
	uint8_t  header_size;//blocks, size of ptn header.
	uint8_t  tbl_offset;//blocks, offset of index table.
	uint8_t  tbl_entry_size;//blocks, size of index table's entry.
	uint32_t tbl_entry_num;//numbers of index table's entry.
} resource_ptn_header;

#define INDEX_TBL_ENTR_TAG          "ENTR"
#define MAX_INDEX_ENTRY_PATH_LEN    256
typedef struct {
	char     tag[4];//tag, "ENTR"
	char     path[MAX_INDEX_ENTRY_PATH_LEN];
	uint32_t content_offset;//blocks, offset of resource content.
	uint32_t content_size;//bytes, size of resource content.
} index_tbl_entry;

typedef struct {
	char     path[MAX_INDEX_ENTRY_PATH_LEN];
	uint32_t content_offset;//blocks, offset of resource content.
	uint32_t content_size;//bytes, size of resource content.
	void*    load_addr;
} resource_content;


bool get_content(int base_offset, resource_content* content);
void free_content(resource_content* content);
bool load_content(resource_content* content);
bool load_content_data(resource_content* content,
        int offset_block, void* data, int blocks);

bool get_content_ram(void *buf, size_t len,
		resource_content* content);

bool show_resource_image(const char* image_path);
struct bmp_header *get_bmp_header(const char *bmp_name);
int load_bmp_content(const char *logo, void *bmp, int size);

#endif //RESOURCE_H
