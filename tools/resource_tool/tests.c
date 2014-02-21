#include "resource_tool.h"
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

static const char* image_path;
static int load_file(const char* file_path, int offset_block, int blocks);

static int inline get_ptn_offset() {
    return 0;
}

int test_load(int argc, char** argv) {
    if (argc < 2)
        return -1;
    image_path = argv[0];
    const char* file_path;
    int offset_block = 0;
    int blocks = 0;
    argc--, argv++;
    if (argc > 0) {
        int pos = 0;
        if (!memcmp(argv[0], "./", 2)) {
            pos = 2;
        } else if (!memcmp(argv[0], "/", 1)) {
            pos = 1;
        }
        file_path = argv[0] + pos;
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

static uint16_t switch_short(uint16_t x)
{
    uint16_t val;
    uint8_t *p = (uint8_t *)(&x);

    val =  (*p++ & 0xff) << 0;
    val |= (*p & 0xff) << 8;

    return val;
}

static uint32_t switch_int(uint32_t x)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)(&x);

    val =  (*p++ & 0xff) << 0;
    val |= (*p++ & 0xff) << 8;
    val |= (*p++ & 0xff) << 16;
    val |= (*p & 0xff) << 24;

    return val;
}

static inline int fix_blocks(size_t size) {
    return (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
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
    header.resource_ptn_version = switch_short(header.resource_ptn_version);
    header.index_tbl_version = switch_short(header.index_tbl_version);
    header.tbl_entry_num = switch_int(header.tbl_entry_num);

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
    entry->content_offset = switch_int(entry->content_offset);
    entry->content_size = switch_int(entry->content_size);

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

