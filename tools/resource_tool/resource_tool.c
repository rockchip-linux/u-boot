/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "resource_tool.h"
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

static const char* PROG = NULL;
static resource_ptn_header header;
static bool just_print = false;
char image_path[MAX_INDEX_ENTRY_PATH_LEN] = "\0";
char root_path[MAX_INDEX_ENTRY_PATH_LEN] = "\0";

static void version() {
    printf("%s (cjf@rock-chips.com)\t" VERSION "\n", PROG);
}

static void usage() {
    printf("Usage: %s [options] [FILES]\n", PROG);
    printf("Tools for Rockchip's resource image.\n");
    version();
    printf("Options:\n");
    printf("\t" OPT_PACK    "\t\t\tPack image from given files.\n");
    printf("\t" OPT_UNPACK  "\t\tUnpack given image to current dir.\n");
    printf("\t" OPT_IMAGE "path"  "\t\tSpecify input/output image path.\n");
    printf("\t" OPT_PRINT   "\t\t\tJust print informations.\n");
    printf("\t" OPT_VERBOSE "\t\tDisplay more runtime informations.\n");
    printf("\t" OPT_HELP    "\t\t\tDisplay this information.\n");
    printf("\t" OPT_VERSION "\t\tDisplay version information.\n");
    printf("\t" OPT_ROOT "path" "\t\tSpecify resources' root dir.\n");
}

static int pack_image(int file_num, const char** files);
static int unpack_image(const char* unpack_dir);

int fix_blocks(size_t size) {
    return (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

const char* fix_path(const char* path) {
    if (!memcmp(path, "./", 2)) {
        return path + 2;
    }
    return path;
}

enum ACTION {
    ACTION_PACK,
    ACTION_UNPACK,
    ACTION_TEST_LOAD,
    ACTION_TEST_CHARGE,
};

int main(int argc, char** argv) {
    PROG = fix_path(argv[0]);
    bool pack = true;

    enum ACTION action = ACTION_PACK;

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
            version();
            return 0;
        } else if (!strcmp(OPT_PRINT, arg)) {
            just_print = true;
        } else if (!strcmp(OPT_PACK, arg)) {
            action = ACTION_PACK;
        } else if (!strcmp(OPT_UNPACK, arg)) {
            action = ACTION_UNPACK;
        } else if (!strcmp(OPT_TEST_LOAD, arg)) {
            action = ACTION_TEST_LOAD;
        } else if (!strcmp(OPT_TEST_CHARGE, arg)) {
            action = ACTION_TEST_CHARGE;
        } else if (!memcmp(OPT_IMAGE, arg, strlen(OPT_IMAGE))) {
            snprintf(image_path, sizeof(image_path),
                    "%s", arg + strlen(OPT_IMAGE));
        } else if (!memcmp(OPT_ROOT, arg, strlen(OPT_ROOT))) {
            snprintf(root_path, sizeof(root_path),
                    "%s", arg + strlen(OPT_ROOT));
        } else {
            LOGE("Unknown opt:%s", arg);
            usage();
            return -1;
        }
    }

    if (!image_path[0]) {
        snprintf(image_path, sizeof(image_path), "%s", DEFAULT_IMAGE_PATH);
    }

    switch (action) {
        case ACTION_PACK:
            {
                int file_num = argc;
                const char** files = (const char**)argv;
                if (!file_num) {
                    LOGE("No file to pack!");
                    return 0;
                }
                LOGD("try to pack %d files.", file_num);
                return pack_image(file_num, files);
             }
        case ACTION_UNPACK:
            {
                return unpack_image(argc > 0? argv[0] : DEFAULT_UNPACK_DIR);
            }
        case ACTION_TEST_LOAD:
            {
                return test_load(argc, argv);
            }
        case ACTION_TEST_CHARGE:
            {
                return test_charge(argc, argv);
            }
    }
    //not reach here.
    return -1;
}

uint16_t switch_short(uint16_t x)
{
    uint16_t val;
    uint8_t *p = (uint8_t *)(&x);

    val =  (*p++ & 0xff) << 0;
    val |= (*p & 0xff) << 8;

    return val;
}

uint32_t switch_int(uint32_t x)
{
    uint32_t val;
    uint8_t *p = (uint8_t *)(&x);

    val =  (*p++ & 0xff) << 0;
    val |= (*p++ & 0xff) << 8;
    val |= (*p++ & 0xff) << 16;
    val |= (*p & 0xff) << 24;

    return val;
}

void fix_header(resource_ptn_header* header) {
    //switch for be.
    header->resource_ptn_version = switch_short(header->resource_ptn_version);
    header->index_tbl_version = switch_short(header->index_tbl_version);
    header->tbl_entry_num = switch_int(header->tbl_entry_num);
}

void fix_entry(index_tbl_entry* entry) {
    //switch for be.
    entry->content_offset = switch_int(entry->content_offset);
    entry->content_size = switch_int(entry->content_size);
}


/************unpack code****************/
static bool mkdirs(char* path) {
    char* tmp = path;
    char* pos = NULL;
    char buf[MAX_INDEX_ENTRY_PATH_LEN];
    bool ret = true;
    while(pos = memchr(tmp, '/', strlen(tmp))) {
        strcpy(buf, path);
        buf[pos - path] = '\0';
        tmp = pos + 1;
        LOGD("mkdir:%s", buf);
        if (!mkdir(buf, 0)) {
            ret = false;
        }
    }
    if (!ret)
        LOGD("Failed to mkdir(%s)!", path);
    return ret;
}

static bool dump_file(FILE* file, const char* unpack_dir,
        index_tbl_entry entry) {
    LOGD("try to dump entry:%s", entry.path);
    bool ret = false;
    FILE* out_file = NULL;
    long int pos = 0;
    char path[MAX_INDEX_ENTRY_PATH_LEN];
    if (just_print) {
        ret = true;
        goto done;
    }

    pos = ftell(file);
    snprintf(path, sizeof(path), "%s/%s", unpack_dir, entry.path);
    mkdirs(path);
    out_file = fopen(path, "wb");
    if (!out_file) {
        LOGE("Failed to create:%s", path);
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
	if (pos)
		fseek(file, pos, SEEK_SET);
	return ret;
}

static int unpack_image(const char* dir) {
    bool ret = false;
    char unpack_dir[MAX_INDEX_ENTRY_PATH_LEN];
	if (just_print)
		dir = ".";
    snprintf(unpack_dir, sizeof(unpack_dir), "%s", dir);
    if (!strlen(unpack_dir)) {
        goto end;
    } else if (unpack_dir[strlen(unpack_dir) - 1] == '/') {
        unpack_dir[strlen(unpack_dir) - 1] = '\0';
    }

    mkdir(unpack_dir, 0);
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
    fix_header(&header);

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

    printf("Dump Index table:\n");
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
        fix_entry(&entry);

        printf("entry(%d):\n\tpath:%s\n\toffset:%d\tsize:%d\n",
                i, entry.path, entry.content_offset, entry.content_size);
        if (!dump_file(image_file, unpack_dir, entry)) {
            goto end;
        }
    }
    printf("Unack %s to %s successed!\n", image_path, unpack_dir);
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

static int write_file(int offset_block, const char* src_path) {
    LOGD("try to write file(%s) to offset:%d...", src_path, offset_block);
    char buf[BLOCK_SIZE];
    int ret = -1;
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
        if (!write_data(offset_block + i, buf, BLOCK_SIZE)) {
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

static bool write_header(const int file_num) {
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
    fix_header(&hdr);
    return write_data(0, &hdr, sizeof(hdr));
}

static bool write_index_tbl(const int file_num, const char** files) {
    LOGD("try to write index table...");
    bool ret = false;
    bool foundFdt = false;
    int offset = header.header_size +
        header.tbl_entry_size * header.tbl_entry_num;
    index_tbl_entry entry;
    memcpy(entry.tag, INDEX_TBL_ENTR_TAG, sizeof(entry.tag));
    int i;
    for (i = 0; i < file_num; i++) {
        size_t file_size = get_file_size(files[i]);
        if (file_size < 0)
            goto end;
        entry.content_size = file_size;
        entry.content_offset = offset;

        if (write_file(offset, files[i]) < 0)
            goto end;

        LOGD("try to write index entry(%s)...", files[i]);

        //switch for le.
        fix_entry(&entry);
        memset(entry.path, 0, sizeof(entry.path));
        const char* path = files[i];
		if (root_path[0]) {
			if (!strncmp(path, root_path, strlen(root_path))) {
				path += strlen(root_path);
				if (path[0] == '/')
					path++;
			}
		}
		path = fix_path(path);
        if (!strcmp(files[i] + strlen(files[i]) - strlen(DTD_SUBFIX),
                    DTD_SUBFIX)) {
            if (!foundFdt) {
                //use default path.
                LOGD("mod fdt path:%s -> %s...", files[i], FDT_PATH);
                path = FDT_PATH;
                foundFdt = true;
            }
        }
        snprintf(entry.path, sizeof(entry.path), "%s", path);
        offset += fix_blocks(file_size);
        if (!write_data(header.header_size + i * header.tbl_entry_size,
                    &entry, sizeof(entry)))
            goto end;
    }
    ret = true;
end:
    return ret;
}

static int pack_image(int file_num, const char** files) {
    bool ret = false;
    FILE* image_file = fopen(image_path, "wb");
    if (!image_file) {
        LOGE("Failed to create:%s", image_path);
        goto end;
    }
    fclose(image_file);

    //prepare files
    int i = 0;
    int pos = 0;
    const char* tmp;
    for (i = 0; i < file_num; i++) {
        if (!strcmp(files[i] + strlen(files[i]) - strlen(DTD_SUBFIX),
                    DTD_SUBFIX)) {
            //dtb files for kernel.
            tmp = files[pos];
            files[pos] = files[i];
            files[i] = tmp;
            pos++;
        } else if (!strcmp(fix_path(image_path), fix_path(files[i]))) {
            //not to pack image itself!
            tmp = files[file_num - 1];
            files[file_num - 1] = files[i];
            files[i] = tmp;
            file_num --;
        }
    }

    if (!write_header(file_num)) {
        LOGE("Failed to write header!");
        goto end;
    }
    if (!write_index_tbl(file_num, files)) {
        LOGE("Failed to write index table!");
        goto end;
    }
    printf("Pack to %s successed!\n", image_path);
    ret = true;
end:
    return ret ? 0 : -1;
}

/************pack code end****************/
