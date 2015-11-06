/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "resource_tool.h"
#include "tests.h"
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

static int inline get_ptn_offset() {
    return 0;
}

static bool StorageWriteLba(int offset_block, void* data, int blocks) {
    bool ret = false;
    FILE* file = fopen(image_path, "rb+");
    if (!file)
        goto end;
    int offset = offset_block * BLOCK_SIZE;
    fseek(file, offset, SEEK_SET);
    if (offset != ftell(file)) {
        LOGE("Failed to seek %s to %d!", image_path, offset);
        goto end;
    }
    if (!fwrite(data, blocks * BLOCK_SIZE, 1, file)) {
        LOGE("Failed to write %s!", image_path);
        goto end;
    }
    ret = true;
end:
    if (file)
        fclose(file);
    return ret;
}

static bool StorageReadLba(int offset_block, void* data, int blocks) {
    bool ret = false;
    FILE* file = fopen(image_path, "rb");
    if (!file)
        goto end;
    int offset = offset_block * BLOCK_SIZE;
    fseek(file, offset, SEEK_SET);
    if (offset != ftell(file)) {
        goto end;
    }
    if (!fread(data, blocks * BLOCK_SIZE, 1, file)) {
        goto end;
    }
    ret = true;
end:
    if (file)
        fclose(file);
    return ret;
}

bool write_data(int offset_block, void* data, size_t len) {
    bool ret = false;
    if (!data)
        goto end;
    int blocks = len / BLOCK_SIZE;
    if (blocks && !StorageWriteLba(offset_block, data, blocks)) {
        goto end;
    }
    int left = len % BLOCK_SIZE;
    if (left) {
        char buf[BLOCK_SIZE] = "\0";
        memcpy(buf, data + blocks * BLOCK_SIZE, left);
        if (!StorageWriteLba(offset_block + blocks, buf, 1))
            goto end;
    }
done:
    ret = true;
end:
    return ret;
}

bool read_data(int offset_block, void* data, int blocks) {
    bool ret = false;
    if (!data)
        goto end;
    if (!StorageReadLba(offset_block, data, blocks))
        goto end;
done:
    ret = true;
end:
    return ret;
}

bool copy_data(int dst_offset, int src_offset, int blocks) {
    char buf[BLOCK_SIZE];
    int i = 0;
    for (i = 0; i < blocks; i++) {
        if (!StorageReadLba(src_offset + i, buf, 1))
            return false;
        if (!StorageWriteLba(dst_offset + i, buf, 1))
            return false;
    }
    return true;
}

/**********************load test************************/
static int load_file(const char* file_path, int offset_block, int blocks);

int test_load(int argc, char** argv) {
    if (argc < 1) {
        LOGE("Nothing to load!");
        return -1;
    }
    const char* file_path;
    int offset_block = 0;
    int blocks = 0;
    if (argc > 0) {
        file_path = (const char*)fix_path(argv[0]);
        argc--, argv++;
    }
    if (argc > 0) {
        offset_block = atoi(argv[0]);
        argc--, argv++;
    }
    if (argc > 0) {
        blocks = atoi(argv[0]);
    }
    return load_file(file_path, offset_block, blocks);
}

static void free_content(resource_content* content) {
    if (content->load_addr) {
        free(content->load_addr);
        content->load_addr = 0;
    }
}

static void dump_file(const char* path, void* data, int len) {
    FILE* file = fopen(path, "wb");
    if (!file)
        return;
    fwrite(data, len, 1, file);
    fclose(file);
}

static bool load_content(resource_content* content) {
    if (content->load_addr)
        return true;
    int blocks = fix_blocks(content->content_size);
    content->load_addr = malloc(blocks * BLOCK_SIZE);
    if (!content->load_addr)
        return false;
    if (!StorageReadLba(get_ptn_offset() +
                content->content_offset, content->load_addr, blocks)) {
        free_content(content);
        return false;
    }

    dump_file(content->path, content->load_addr, content->content_size);
    return true;
}

static bool load_content_data(resource_content* content,
        int offset_block, void* data, int blocks) {
    if (!StorageReadLba(get_ptn_offset() +
                content->content_offset + offset_block, data, blocks)) {
        return false;
    }
    dump_file(content->path, data, blocks * BLOCK_SIZE);
    return true;
}

static bool get_entry(const char* file_path, index_tbl_entry* entry) {
    bool ret = false;
    char buf[BLOCK_SIZE];
    resource_ptn_header header;
    if (!StorageReadLba(get_ptn_offset(), buf, 1)) {
        LOGE("Failed to read header!");
        goto end;
    }
    memcpy(&header, buf, sizeof(header));

    if (memcmp(header.magic, RESOURCE_PTN_HDR_MAGIC, sizeof(header.magic))) {
        LOGE("Not a resource image(%s)!", image_path);
        goto end;
    }

    //test on pc, switch for be.
    fix_header(&header);

    //TODO: support header_size & tbl_entry_size
    if (header.resource_ptn_version != RESOURCE_PTN_VERSION
            || header.header_size != RESOURCE_PTN_HDR_SIZE
            || header.index_tbl_version != INDEX_TBL_VERSION
            || header.tbl_entry_size != INDEX_TBL_ENTR_SIZE) {
        LOGE("Not supported in this version!");
        goto end;
    }

    int i;
    for (i = 0; i < header.tbl_entry_num; i++) {
        //TODO: support tbl_entry_size
        if (!StorageReadLba(get_ptn_offset() +
                    header.header_size + i * header.tbl_entry_size, buf, 1)) {
            LOGE("Failed to read index entry:%d!", i);
            goto end;
        }
        memcpy(entry, buf, sizeof(*entry));

        if (memcmp(entry->tag, INDEX_TBL_ENTR_TAG, sizeof(entry->tag))) {
            LOGE("Something wrong with index entry:%d!", i);
            goto end;
        }

        if (!strncmp(entry->path, file_path, sizeof(entry->path)))
            break;
    }
    if (i == header.tbl_entry_num) {
        LOGE("Cannot find %s!", file_path);
        goto end;
    }

    //test on pc, switch for be.
    fix_entry(entry);

    printf("Found entry:\n\tpath:%s\n\toffset:%d\tsize:%d\n",
            entry->path, entry->content_offset, entry->content_size);

    ret = true;
end:
    return ret;
}

static bool get_content(resource_content* content) {
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

static int load_file(const char* file_path, int offset_block, int blocks) {
    printf("Try to load:%s", file_path);
    if (blocks) {
        printf(", offset block:%d, blocks:%d\n", offset_block, blocks);
    } else {
        printf("\n");
    }
    bool ret = false;
    resource_content content;
    snprintf(content.path, sizeof(content.path), "%s", file_path);
    content.load_addr = 0;
    if (!get_content(&content)) {
        goto end;
    }
    if (!blocks) {
        if (!load_content(&content)) {
            goto end;
        }
    } else {
        void* data = malloc(blocks * BLOCK_SIZE);
        if (!data)
            goto end;
        if (!load_content_data(&content, offset_block, data, blocks)) {
            goto end;
        }
    }
    ret = true;
end:
    free_content(&content);
    return ret;
}

/**********************load test end************************/
/**********************anim test************************/

static bool parse_level_conf(const char* arg, anim_level_conf* level_conf) {
    memset(level_conf, 0, sizeof(anim_level_conf));
    char* buf = NULL;
    buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_CONF);
    if (buf) {
        level_conf->max_level = atoi(buf + strlen(OPT_CHARGE_ANIM_LEVEL_CONF));
    } else {
        LOGE("Not found:%s", OPT_CHARGE_ANIM_LEVEL_CONF);
        return false;
    }
    buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_NUM);
    if (buf) {
        level_conf->num = atoi(buf + strlen(OPT_CHARGE_ANIM_LEVEL_NUM));
        if (level_conf->num <= 0) {
            return false;
        }
    } else {
        LOGE("Not found:%s", OPT_CHARGE_ANIM_LEVEL_NUM);
        return false;
    }
    buf = strstr(arg, OPT_CHARGE_ANIM_DELAY);
    if (buf) {
        level_conf->delay = atoi(buf + strlen(OPT_CHARGE_ANIM_DELAY));
    }
    buf = strstr(arg, OPT_CHARGE_ANIM_LEVEL_PFX);
    if (buf) {
        snprintf(level_conf->prefix, sizeof(level_conf->prefix),
                "%s", buf + strlen(OPT_CHARGE_ANIM_LEVEL_PFX));
    } else {
        LOGE("Not found:%s", OPT_CHARGE_ANIM_LEVEL_PFX);
        return false;
    }

    LOGD("Found conf:\nmax_level:%d, num:%d, delay:%d, prefix:%s",
            level_conf->max_level, level_conf->num,
            level_conf->delay, level_conf->prefix);
    return true;
}

int test_charge(int argc, char** argv) {
    const char* desc;
    if (argc > 0) {
        desc = argv[0];
    } else {
        desc = DEF_CHARGE_DESC_PATH;
    }

    resource_content content;
    snprintf(content.path, sizeof(content.path), "%s", desc);
    content.load_addr = 0;
    if (!get_content(&content)) {
        goto end;
    }
    if (!load_content(&content)) {
        goto end;
    }

    char* buf = (char*)content.load_addr;
    char* end = buf + content.content_size - 1;
    *end = '\0';
    LOGD("desc:\n%s", buf);

    int pos = 0;
    while (1) {
        char* line = (char*) memchr(buf + pos, '\n', strlen(buf + pos));
        if (!line)
            break;
        *line = '\0';
        LOGD("splite:%s", buf + pos);
        pos += (strlen(buf + pos) + 1);
    }

    int delay = 900;
    int only_current_level = false;
    anim_level_conf* level_confs = NULL;
    int level_conf_pos = 0;
    int level_conf_num = 0;

    while (true) {
        if (buf >= end)
            break;
        const char* arg = buf;
        buf += (strlen(buf) + 1);

        LOGD("parse arg:%s", arg);
        if (!memcmp(arg, OPT_CHARGE_ANIM_LEVEL_CONF,
                    strlen(OPT_CHARGE_ANIM_LEVEL_CONF))) {
            if (!level_confs) {
                LOGE("Found level conf before levels!");
                goto end;
            }
            if (level_conf_pos >= level_conf_num) {
                LOGE("Too many level confs!(%d >= %d)", level_conf_pos, level_conf_num);
                goto end;
            }
            if (!parse_level_conf(arg, level_confs + level_conf_pos)) {
                LOGE("Failed to parse level conf:%s", arg);
                goto end;
            }
            level_conf_pos ++;
        } else if (!memcmp(arg, OPT_CHARGE_ANIM_DELAY,
                    strlen(OPT_CHARGE_ANIM_DELAY))) {
            delay = atoi(arg + strlen(OPT_CHARGE_ANIM_DELAY));
            LOGD("Found delay:%d", delay);
        } else if (!memcmp(arg, OPT_CHARGE_ANIM_LOOP_CUR,
                    strlen(OPT_CHARGE_ANIM_LOOP_CUR))) {
            only_current_level =
                !memcmp(arg + strlen(OPT_CHARGE_ANIM_LOOP_CUR), "true", 4);
            LOGD("Found only_current_level:%d", only_current_level);
        } else if (!memcmp(arg, OPT_CHARGE_ANIM_LEVELS,
                    strlen(OPT_CHARGE_ANIM_LEVELS))) {
            if (level_conf_num) {
                goto end;
            }
            level_conf_num = atoi(arg + strlen(OPT_CHARGE_ANIM_LEVELS));
            if (!level_conf_num) {
                goto end;
            }
            level_confs =
                (anim_level_conf*) malloc(level_conf_num * sizeof(anim_level_conf));
            LOGD("Found levels:%d", level_conf_num);
        } else {
            LOGE("Unknown arg:%s", arg);
            goto end;
        }
    }

    if (level_conf_pos != level_conf_num || !level_conf_num) {
        LOGE("Something wrong with level confs!");
        goto end;
    }

    int i = 0, j = 0, k = 0;
    for (i = 0; i < level_conf_num; i++) {
        if (!level_confs[i].delay) {
            level_confs[i].delay = delay;
        }
        if (!level_confs[i].delay) {
            LOGE("Missing delay in level conf:%d", i);
            goto end;
        }
        for (j = 0; j < i; j++) {
            if (level_confs[j].max_level == level_confs[i].max_level) {
                LOGE("Dup level conf:%d", i);
                goto end;
            }
            if (level_confs[j].max_level > level_confs[i].max_level) {
                anim_level_conf conf = level_confs[i];
                memmove(level_confs + j + 1, level_confs + j,
                        (i - j) * sizeof(anim_level_conf));
                level_confs[j] = conf;
            }
        }
    }

    printf("Parse anim desc(%s):\n", desc);
    printf("only_current_level=%d\n", only_current_level);
    printf("level conf:\n");
    for (i = 0; i < level_conf_num; i++) {
        printf("\tmax=%d, delay=%d, num=%d, prefix=%s\n",
                level_confs[i].max_level, level_confs[i].delay,
                level_confs[i].num, level_confs[i].prefix);
    }

end:
    free_content(&content);
    return 0;
}

/**********************anim test end************************/
/**********************append file************************/
