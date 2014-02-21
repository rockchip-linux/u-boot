#include "resource_tool.h"
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

static const char* PROG = NULL;
static resource_ptn_header header;
static bool just_print = false;

static void usage() {
    printf("Usage: %s [options] [FILES]\n", PROG);
    printf("Pack or unpack Rockchip's resource image (Default action is to pack.)\n");
    printf("Options:\n");
    printf("\t" OPT_PACK "\t\t\tPack image from given files.\n");
    printf("\t" OPT_UNPACK "\t\tUnpack given image to current dir.\n");
    printf("\t" OPT_PRINT "\t\tJust print informations.\n");
    printf("\t" OPT_VERBOSE "\t\tDisplay more runtime informations.\n");
    printf("\t" OPT_HELP "\t\t\tDisplay this information.\n");
    printf("\t" OPT_VERSION "\t\tDisplay version information.\n");
}

static int pack_image(const int file_num, const char** files);
static int unpack_image(const char* image);

int main(int argc, char** argv) {
    PROG = argv[0];
    bool pack = true;
    const int file_num;
    const char** files;

    argc--, argv++;
    while (argc > 0 && argv[0][0] == '-') {
        //it's a opt arg.
        const char* arg = argv[0];
        argc--, argv++;
        if (!strcmp(OPT_VERBOSE, arg)) {
            g_debug = true;
        } else if (!strcmp(OPT_HELP, arg)) {
            usage();
            return 0;
        } else if (!strcmp(OPT_VERSION, arg)) {
            printf("%s (cjf@rock-chips.com)\t" VERSION "\n", PROG);
            return 0;
        } else if (!strcmp(OPT_PRINT, arg)) {
            just_print = true;
        } else if (!strcmp(OPT_PACK, arg)) {
            pack = true;
        } else if (!strcmp(OPT_UNPACK, arg)) {
            pack = false;
        } else {
            LOGE("Unknown opt:%s", arg);
            usage();
            return -1;
        }
    }

    if (pack) {
        const int file_num = argc;
        const char** files = argv;
        if (!file_num) {
            LOGE("No file to pack!");
            return 0;
        }
        LOGD("try to pack %d files.", file_num);
        return pack_image(file_num, files);
    } else {
        if (argc < 1) {
            LOGE("Need to specify image to unpack!");
            return -1;
        }
        const char* image_path = argv[0];
        LOGD("try to unpack:%s.", image_path);
#ifdef TEST_LOAD
        if (argc == 1)
#endif
        return unpack_image(image_path);
#ifdef TEST_LOAD
        else {
           return test_load(argc, argv);
        }
#endif
    }
    //not reach here.
    return -1;
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

/************unpack code****************/
static bool dump_file(FILE* file, index_tbl_entry entry) {
    LOGD("try to dump entry:%s", entry.path);
    bool ret = false;
    FILE* out_file = NULL;
    if (just_print) {
        ret = true;
        goto done;
    }

    long int pos = ftell(file);
    out_file = fopen(entry.path, "wb");
    if (!out_file) {
        LOGE("Failed to create:%s", entry.path);
        goto end;
    }
    long int offset = entry.content_offset * BLOCK_SIZE;
    fseek(file, offset, SEEK_SET);
    if (offset != ftell(file)) {
        LOGE("Failed to read content:%s", entry.path);
        goto end;
    }
    char buf[BLOCK_SIZE];
    int n;
    int len = entry.content_size;
    while (len > 0) {
        n = len > BLOCK_SIZE ? BLOCK_SIZE : len;
        if (!fread(buf, n, 1, file)) {
            LOGE("Failed to read content:%s", entry.path);
            goto end;
        }
        if (!fwrite(buf, n, 1, out_file)) {
            LOGE("Failed to write:%s", entry.path);
            goto end;
        }
        len -= n;
    }
done:
    ret = true;
end:
    if (out_file)
        fclose(out_file);
    fseek(file, pos, SEEK_SET);
    return ret;
}

static int unpack_image(const char* image_path) {
    bool ret = false;
    FILE* image_file = fopen(image_path, "rb");
    char buf[BLOCK_SIZE];
    if (!image_file) {
        LOGE("Failed to open:%s", image_path);
        goto end;
    }
    if (!fread(buf, BLOCK_SIZE, 1, image_file)) {
        LOGE("Failed to read header!");
        goto end;
    }
    memcpy(&header, buf, sizeof(header));

    if (memcmp(header.magic, RESOURCE_PTN_HDR_MAGIC, sizeof(header.magic))) {
        LOGE("Not a resource image(%s)!", image_path);
        goto end;
    }

    //switch for be.
    header.resource_ptn_version = switch_short(header.resource_ptn_version);
    header.index_tbl_version = switch_short(header.index_tbl_version);
    header.tbl_entry_num = switch_int(header.tbl_entry_num);

    printf("Dump header:\n");
    printf("partition version:%d.%d\n",
            header.resource_ptn_version, header.index_tbl_version);
    printf("header size:%d\n", header.header_size);
    printf("index tbl:\n\toffset:%d\tentry size:%d\tentry num:%d\n",
            header.tbl_offset, header.tbl_entry_size, header.tbl_entry_num);

    //TODO: support header_size & tbl_entry_size
    if (header.resource_ptn_version != RESOURCE_PTN_VERSION
            || header.header_size != RESOURCE_PTN_HDR_SIZE
            || header.index_tbl_version != INDEX_TBL_VERSION
            || header.tbl_entry_size != INDEX_TBL_ENTR_SIZE) {
        LOGE("Not supported in this version!");
        goto end;
    }

    index_tbl_entry entry;
    int i;
    for (i = 0; i < header.tbl_entry_num; i++) {
        //TODO: support tbl_entry_size
        if (!fread(buf, BLOCK_SIZE, 1, image_file)) {
            LOGE("Failed to read index entry:%d!", i);
            goto end;
        }
        memcpy(&entry, buf, sizeof(entry));

        if (memcmp(entry.tag, INDEX_TBL_ENTR_TAG, sizeof(entry.tag))) {
            LOGE("Something wrong with index entry:%d!", i);
            goto end;
        }

        //switch for be.
        entry.content_offset = switch_int(entry.content_offset);
        entry.content_size = switch_int(entry.content_size);

        printf("entry(%d):\n\tpath:%s\n\toffset:%d\tsize:%d\n",
                i, entry.path, entry.content_offset, entry.content_size);
        if (!dump_file(image_file, entry)) {
            goto end;
        }
    }
    printf("Unack %s successed!\n", image_path);
    ret = true;
end:
    if (image_file)
        fclose(image_file);
    return ret ? 0 : -1;
}

/************unpack code end****************/
/************pack code****************/

static inline size_t get_file_size(const char* path) {
    LOGD("try to get size(%s)...", path);
    struct stat st;
    if(stat(path, &st) < 0) {
        LOGE("Failed to get size:%s", path);
        return -1;
    }
    LOGD("path:%s, size:%d", path, st.st_size);
    return st.st_size;
}

static inline int fix_blocks(size_t size) {
    return (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

static bool write_data(FILE* file, void* data, size_t len) {
    bool ret = false;
    if (!file || !data)
        goto end;
    if (!fwrite(data, len, 1, file))
        goto end;
    int fill_len = BLOCK_SIZE - (len % BLOCK_SIZE);
    char fill_buf[BLOCK_SIZE] = "\0";
    if (fill_len != BLOCK_SIZE) {
        if (!fwrite(fill_buf, fill_len, 1, file))
            goto end;
    }
done:
    ret = true;
end:
    return ret;
}

static int write_file(FILE* file, const char* src_path) {
    char buf[BLOCK_SIZE];
    int ret = 0;
    size_t file_size;
    int blocks;
    FILE* src_file = fopen(src_path, "rb");
    if (!src_file) {
        LOGE("Failed to open:%s", src_path);
        goto end;
    }
    
    file_size = get_file_size(src_path);
    if(file_size < 0) {
        goto end;
    }
    blocks = fix_blocks(file_size);

    int i;
    for (i = 0; i < blocks; i++) {
        memset(buf, 0, sizeof(buf));
        if (!fread(buf, 1, BLOCK_SIZE, src_file)) {
            LOGE("Failed to read:%s", src_path);
            goto end;
        }
        if (!fwrite(buf, BLOCK_SIZE, 1, file)) {
            LOGE("Failed to write:%s", file);
            goto end;
        }
    }
done:
    ret = blocks;
end:
    if (src_file)
        fclose(src_file);
    return ret;
}

static bool write_header(FILE* file, const int file_num) {
    if (!file)
        return false;
    LOGD("try to write header...");
    memcpy(header.magic, RESOURCE_PTN_HDR_MAGIC, sizeof(header.magic));
    header.resource_ptn_version = RESOURCE_PTN_VERSION;
    header.index_tbl_version = INDEX_TBL_VERSION;
    header.header_size = RESOURCE_PTN_HDR_SIZE;
    header.tbl_offset = header.header_size;
    header.tbl_entry_size = INDEX_TBL_ENTR_SIZE;
    header.tbl_entry_num = file_num;

    //switch for le.
    resource_ptn_header hdr = header;
    hdr.resource_ptn_version = switch_short(hdr.resource_ptn_version);
    hdr.index_tbl_version = switch_short(hdr.index_tbl_version);
    hdr.tbl_entry_num = switch_int(hdr.tbl_entry_num);
    return write_data(file, &hdr, sizeof(hdr));
}

static bool write_index_tbl(FILE* file,
        const int file_num, const char** files) {
    if (!file)
        return false;
    LOGD("try to write index table...");
    bool ret = false;
    int offset = header.header_size +
        header.tbl_entry_size * header.tbl_entry_num;
    index_tbl_entry entry;
    memcpy(entry.tag, INDEX_TBL_ENTR_TAG, sizeof(entry.tag));
    int i;
    for (i = 0; i < file_num; i++) {
        LOGD("try to write index entry(%s)...", files[i]);
        size_t file_size = get_file_size(files[i]);
        if (file_size < 0)
            goto end;
        entry.content_size = file_size;
        entry.content_offset = offset;

        //switch for le.
        entry.content_offset = switch_int(entry.content_offset);
        entry.content_size = switch_int(entry.content_size);

        int pos = 0;
        if (!memcmp(files[i], "./", 2)) {
            pos = 2;
        } else if (!memcmp(files[i], "/", 1)) {
            pos = 1;
        }

        memset(entry.path, 0, sizeof(entry.path));
        snprintf(entry.path, sizeof(entry.path), "%s", files[i] + pos);
        offset += fix_blocks(file_size);
        if (!write_data(file, &entry, sizeof(entry)))
            goto end;
    }
    ret = true;
end:
    return ret;
}

static bool write_files(FILE* file,
        const int file_num, const char** files) {
    if (!file)
        return false;
    bool ret = false;
    int i;
    for (i = 0; i < file_num; i++) {
        LOGD("try to write file(%s)...", files[i]);
        if (!write_file(file, files[i]))
            goto end;
    }
    ret = true;
end:
    return ret;
}

static int pack_image(const int file_num, const char** files) {
    bool ret = false;
    FILE* image_file = fopen(RESOURCE_IMAGE, "wb");
    if (!image_file) {
        LOGE("Failed to create:%s", RESOURCE_IMAGE);
        goto end;
    }
    if (!write_header(image_file, file_num)) {
        LOGE("Failed to write header!");
        goto end;
    }
    if (!write_index_tbl(image_file, file_num, files)) {
        LOGE("Failed to write index table!");
        goto end;
    }
    if (!write_files(image_file, file_num, files)) {
        LOGE("Failed to write files!");
        goto end;
    }
    printf("Pack to %s successed!\n", RESOURCE_IMAGE);
    ret = true;
end:
    if (image_file)
        fclose(image_file);
    return ret ? 0 : -1;
}

/************pack code end****************/
