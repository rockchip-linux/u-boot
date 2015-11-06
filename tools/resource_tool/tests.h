/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
typedef struct {
    char     path[MAX_INDEX_ENTRY_PATH_LEN];
    uint32_t content_offset;//blocks, offset of resource content.
    uint32_t content_size;//bytes, size of resource content.
    void*    load_addr;
} resource_content;

typedef struct {
    int max_level;
    int num;
    int delay;
    char prefix[MAX_INDEX_ENTRY_PATH_LEN];
} anim_level_conf;

#define DEF_CHARGE_DESC_PATH        "charge_anim_desc.txt"

#define OPT_CHARGE_ANIM_DELAY       "delay="
#define OPT_CHARGE_ANIM_LOOP_CUR    "only_current_level="
#define OPT_CHARGE_ANIM_LEVELS      "levels="
#define OPT_CHARGE_ANIM_LEVEL_CONF  "max_level="
#define OPT_CHARGE_ANIM_LEVEL_NUM   "num="
#define OPT_CHARGE_ANIM_LEVEL_PFX   "prefix="


extern char image_path[MAX_INDEX_ENTRY_PATH_LEN];

