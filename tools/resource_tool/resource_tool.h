/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef RESOURCE_TOOL_H
#define RESOURCE_TOOL_H

#include "common.h"

//sync with ./board/rockchip/rk30xx/rkloader.c #define FDT_PATH
#define FDT_PATH                    "rk-kernel.dtb"
#define DTD_SUBFIX                  ".dtb"

#define DEFAULT_IMAGE_PATH          "resource.img"
#define DEFAULT_UNPACK_DIR          "out"
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

#define OPT_VERBOSE         "--verbose"
#define OPT_HELP            "--help"
#define OPT_VERSION         "--version"
#define OPT_PRINT           "--print"
#define OPT_PACK            "--pack"
#define OPT_UNPACK          "--unpack"
#define OPT_TEST_LOAD       "--test_load"
#define OPT_TEST_CHARGE     "--test_charge"
#define OPT_IMAGE           "--image="
#define OPT_ROOT            "--root="

#define VERSION             "2014-5-31 14:43:42"

#endif //RESOURCE_TOOL_H
