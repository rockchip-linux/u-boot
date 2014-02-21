#ifndef RESOURCE_TOOL_H
#define RESOURCE_TOOL_H

#include "common.h"

#define RESOURCE_IMAGE              "resource.img"
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

#if TEST_LOAD
typedef struct {
    char     path[MAX_INDEX_ENTRY_PATH_LEN];
    uint32_t content_offset;//blocks, offset of resource content.
    uint32_t content_size;//bytes, size of resource content.
    void*    load_addr;
} resource_content;
#endif


#define OPT_VERBOSE         "--verbose"
#define OPT_HELP            "--help"
#define OPT_VERSION         "--version"
#define OPT_PRINT           "--print"
#define OPT_PACK            "--pack"
#define OPT_UNPACK          "--unpack"

#define VERSION             "2014-2-14 10:22:23"


#endif //RESOURCE_TOOL_H
