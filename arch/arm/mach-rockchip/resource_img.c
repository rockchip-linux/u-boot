/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <adc.h>
#include <android_bootloader.h>
#include <android_image.h>
#include <boot_rkimg.h>
#include <bmp_layout.h>
#include <crypto.h>
#include <fs.h>
#include <malloc.h>
#include <sysmem.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <dm/ofnode.h>
#include <linux/list.h>
#include <u-boot/sha1.h>
#include <u-boot/sha256.h>
#include <asm/arch/resource_img.h>

DECLARE_GLOBAL_DATA_PTR;

#define PART_RESOURCE			"resource"
#define RESOURCE_MAGIC			"RSCE"
#define RESOURCE_MAGIC_SIZE		4
#define RESOURCE_VERSION		0
#define CONTENT_VERSION			0
#define ENTRY_TAG			"ENTR"
#define ENTRY_TAG_SIZE			4
#define MAX_FILE_NAME_LEN		220
#define MAX_HASH_LEN			32

#define DTB_FILE			"rk-kernel.dtb"

/*
 *         resource image structure
 * ----------------------------------------------
 * |                                            |
 * |    header  (1 block)                       |
 * |                                            |
 * ---------------------------------------------|
 * |                      |                     |
 * |    entry0  (1 block) |                     |
 * |                      |                     |
 * ------------------------                     |
 * |                      |                     |
 * |    entry1  (1 block) | contents (n blocks) |
 * |                      |                     |
 * ------------------------                     |
 * |    ......            |                     |
 * ------------------------                     |
 * |                      |                     |
 * |    entryn  (1 block) |                     |
 * |                      |                     |
 * ----------------------------------------------
 * |                                            |
 * |    file0  (x blocks)                       |
 * |                                            |
 * ----------------------------------------------
 * |                                            |
 * |    file1  (y blocks)                       |
 * |                                            |
 * ----------------------------------------------
 * |                   ......                   |
 * |---------------------------------------------
 * |                                            |
 * |    filen  (z blocks)                       |
 * |                                            |
 * ----------------------------------------------
 */

/**
 * struct resource_image_header
 *
 * @magic: should be "RSCE"
 * @version: resource image version, current is 0
 * @c_version: content version, current is 0
 * @blks: the size of the header ( 1 block = 512 bytes)
 * @c_offset: contents offset(by block) in the image
 * @e_blks: the size(by block) of the entry in the contents
 * @e_num: numbers of the entrys.
 */

struct resource_img_hdr {
	char		magic[4];
	uint16_t	version;
	uint16_t	c_version;
	uint8_t		blks;
	uint8_t		c_offset;
	uint8_t		e_blks;
	uint32_t	e_nums;
};

struct resource_entry {
	char		tag[4];
	char		name[MAX_FILE_NAME_LEN];
	char		hash[MAX_HASH_LEN];
	uint32_t	hash_size;
	uint32_t	f_offset;
	uint32_t	f_size;
};

struct resource_file {
	char		name[MAX_FILE_NAME_LEN];
	char		hash[MAX_HASH_LEN];
	uint32_t	hash_size;
	uint32_t	f_offset;	/* Sector addr */
	uint32_t	f_size;		/* Bytes */
	struct list_head link;
	uint32_t	rsce_base;	/* Base addr of resource */
};

static LIST_HEAD(entrys_head);

static int resource_image_check_header(const struct resource_img_hdr *hdr)
{
	int ret;

	ret = memcmp(RESOURCE_MAGIC, hdr->magic, RESOURCE_MAGIC_SIZE);
	if (ret) {
		debug("bad resource image magic: %s\n",
		      hdr->magic ? hdr->magic : "none");
		ret = -EINVAL;
	}

	debug("resource image header:\n");
	debug("magic:%s\n", hdr->magic);
	debug("version:%d\n", hdr->version);
	debug("c_version:%d\n", hdr->c_version);
	debug("blks:%d\n", hdr->blks);
	debug("c_offset:%d\n", hdr->c_offset);
	debug("e_blks:%d\n", hdr->e_blks);
	debug("e_num:%d\n", hdr->e_nums);

	return ret;
}

static int add_file_to_list(struct resource_entry *entry, int rsce_base)
{
	struct resource_file *file;

	if (memcmp(entry->tag, ENTRY_TAG, ENTRY_TAG_SIZE)) {
		printf("invalid entry tag\n");
		return -ENOENT;
	}

	file = malloc(sizeof(*file));
	if (!file) {
		printf("out of memory\n");
		return -ENOMEM;
	}

	strcpy(file->name, entry->name);
	file->rsce_base = rsce_base;
	file->f_offset = entry->f_offset;
	file->f_size = entry->f_size;
	file->hash_size = entry->hash_size;
	memcpy(file->hash, entry->hash, entry->hash_size);
	list_add_tail(&file->link, &entrys_head);

	debug("entry:%p  %s offset:%d size:%d\n",
	      entry, file->name, file->f_offset, file->f_size);

	return 0;
}

static int replace_resource_entry(const char *f_name, uint32_t base,
				  uint32_t f_offset, uint32_t f_size)
{
	struct resource_entry *entry;
	struct resource_file *file;
	struct list_head *node;

	if (!f_name || !f_size)
		return -EINVAL;

	entry = malloc(sizeof(*entry));
	if (!entry)
		return -ENOMEM;

	strcpy(entry->tag, ENTRY_TAG);
	strcpy(entry->name, f_name);
	entry->f_offset = f_offset;
	entry->f_size = f_size;

	/* Delete exist entry, then add this new */
	list_for_each(node, &entrys_head) {
		file = list_entry(node, struct resource_file, link);
		if (!strcmp(file->name, entry->name)) {
			list_del(&file->link);
			free(file);
			break;
		}
	}

	add_file_to_list(entry, base);
	free(entry);

	return 0;
}

static int read_logo_bmp(const char *name, disk_partition_t *part,
			 uint32_t offset, uint32_t *size)
{
	struct blk_desc *dev_desc;
	struct bmp_header *header;
	u32 blk_start, blk_offset, filesz;
	int ret;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc)
		return -ENODEV;

	blk_offset = DIV_ROUND_UP(offset, dev_desc->blksz);
	blk_start = part->start + blk_offset;
	header = memalign(ARCH_DMA_MINALIGN, dev_desc->blksz);
	if (!header) {
		ret = -ENOMEM;
		goto err;
	}
	ret = blk_dread(dev_desc, blk_start, 1, header);
	if (ret != 1) {
		ret = -EIO;
		goto err;
	}

	if (header->signature[0] != 'B' ||
	    header->signature[1] != 'M') {
		ret = -EINVAL;
		goto err;
	}

	filesz = get_unaligned_le32(&header->file_size);
	ret = replace_resource_entry(name, blk_start, blk_offset, filesz);
	if (!ret) {
		printf("LOGO: %s\n", name);
		if (size)
			*size = filesz;
	}
err:
	free(header);

	return ret;
}
/*
 * There are: logo/battery pictures and dtb file in the resource image by default.
 *
 * This function does:
 *
 * 1. Get resource image from part: boot/recovery(AOSP) > resource(RK);
 * 2. Add all file into resource list(We load them from storage when we need);
 * 3. Add logo picture from logo partition into resource list(replace the
 *    old one in resource file);
 * 4. Add dtb file from dtb position into resource list if boot_img_hdr_v2
 *    (replace the old one in resource file);
 */
static int init_resource_list(struct resource_img_hdr *hdr)
{
	struct resource_entry *entry;
	struct blk_desc *dev_desc;
	char *boot_partname = PART_BOOT;
	disk_partition_t part_info;
	int resource_found = 0;
	void *content = NULL;
	int rsce_base = 0;
	__maybe_unused int dtb_offset = 0;
	int dtb_size = 0;
	int e_num, cnt;
	int size;
	int ret;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}

	/* If hdr is valid from outside, use it */
	if (hdr) {
		if (resource_image_check_header(hdr))
			return -EEXIST;

		content = (void *)((char *)hdr +
				(hdr->c_offset) * dev_desc->blksz);
		for (e_num = 0; e_num < hdr->e_nums; e_num++) {
			size = e_num * hdr->e_blks * dev_desc->blksz;
			entry = (struct resource_entry *)(content + size);
			add_file_to_list(entry, rsce_base);
		}
		return 0;
	}

	cnt = DIV_ROUND_UP(sizeof(struct andr_img_hdr), dev_desc->blksz);
	hdr = memalign(ARCH_DMA_MINALIGN, dev_desc->blksz * cnt);
	if (!hdr)
		return -ENOMEM;

	/*
	 * Anyway, we must read android hdr firstly from boot partition to get
	 * the 'os_version' for android_bcb_msg_sector_offset() to confirm BCB
	 * message offset of misc partition.
	 */
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	struct andr_img_hdr *andr_hdr;

	ret = part_get_info_by_name(dev_desc, boot_partname, &part_info);
	if (ret < 0) {
		printf("%s: failed to get %s part, ret=%d\n",
		       __func__, boot_partname, ret);
		goto parse_resource_part;
	}

	andr_hdr = (void *)hdr;
	ret = blk_dread(dev_desc, part_info.start, cnt, andr_hdr);
	if (ret != cnt) {
		printf("%s: failed to read %s hdr, ret=%d\n",
		       __func__, part_info.name, ret);
		ret = -EIO;
		goto out;
	}

	ret = android_image_check_header(andr_hdr);
	if (!ret) {
		u32 os_ver = andr_hdr->os_version >> 11;
		u32 os_lvl = andr_hdr->os_version & ((1U << 11) - 1);

		if (os_ver) {
			gd->bd->bi_andr_version = andr_hdr->os_version;
			printf("Android %u.%u, Build %u.%u\n",
			       (os_ver >> 14) & 0x7F, (os_ver >> 7) & 0x7F,
			       (os_lvl >> 4) + 2000, os_lvl & 0x0F);
		}
	}

	/* Get boot mode from misc and read if recovery mode */
#ifndef CONFIG_ANDROID_AB
	if (rockchip_get_boot_mode() == BOOT_MODE_RECOVERY) {
		boot_partname = PART_RECOVERY;

		ret = part_get_info_by_name(dev_desc, boot_partname, &part_info);
		if (ret < 0) {
			printf("%s: failed to get %s part, ret=%d\n",
			       __func__, boot_partname, ret);
			goto parse_resource_part;
		}

		/* Try to find resource from android second position */
		andr_hdr = (void *)hdr;
		ret = blk_dread(dev_desc, part_info.start, cnt, andr_hdr);
		if (ret != cnt) {
			printf("%s: failed to read %s hdr, ret=%d\n",
			       __func__, part_info.name, ret);
			ret = -EIO;
			goto out;
		}
	}
#endif

	ret = android_image_check_header(andr_hdr);
	if (!ret) {
		rsce_base = part_info.start * dev_desc->blksz;
		rsce_base += andr_hdr->page_size;
		rsce_base += ALIGN(andr_hdr->kernel_size, andr_hdr->page_size);
		rsce_base += ALIGN(andr_hdr->ramdisk_size, andr_hdr->page_size);

		if (andr_hdr->header_version >= 2) {
			dtb_offset = rsce_base +
			     ALIGN(andr_hdr->recovery_dtbo_size,
				   andr_hdr->page_size) +
			     ALIGN(andr_hdr->second_size, andr_hdr->page_size);
			dtb_size = andr_hdr->dtb_size;
		}

		rsce_base = DIV_ROUND_UP(rsce_base, dev_desc->blksz);
		dtb_offset =
			DIV_ROUND_UP(dtb_offset, dev_desc->blksz) - rsce_base;
		resource_found = 1;
	}
parse_resource_part:
#endif  /* CONFIG_ANDROID_BOOT_IMAGE*/

	/* If not find android image, get resource file from resource part */
	if (!resource_found) {
		boot_partname = PART_RESOURCE;
		ret = part_get_info_by_name(dev_desc, boot_partname, &part_info);
		if (ret < 0) {
			printf("%s: failed to get resource part, ret=%d\n",
			       __func__, ret);
			goto out;
		}
		rsce_base = part_info.start;
	}

	/*
	 * Now, the "rsce_base" points to the resource file sector.
	 */
	ret = blk_dread(dev_desc, rsce_base, 1, hdr);
	if (ret != 1) {
		printf("%s: failed to read resource hdr, ret=%d\n",
		       __func__, ret);
		ret = -EIO;
		goto out;
	}

	ret = resource_image_check_header(hdr);
	if (ret < 0) {
		ret = -EINVAL;
		goto parse_second_pos_dtb;
	}

	content = memalign(ARCH_DMA_MINALIGN,
			   hdr->e_blks * hdr->e_nums * dev_desc->blksz);
	if (!content) {
		printf("%s: failed to alloc memory for content\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	ret = blk_dread(dev_desc, rsce_base + hdr->c_offset,
			hdr->e_blks * hdr->e_nums, content);
	if (ret != (hdr->e_blks * hdr->e_nums)) {
		printf("%s: failed to read resource entries, ret=%d\n",
		       __func__, ret);
		ret = -EIO;
		goto err;
	}

	/*
	 * Add all file into resource file list, and load what we want from
	 * storage when we really need it.
	 */
	for (e_num = 0; e_num < hdr->e_nums; e_num++) {
		size = e_num * hdr->e_blks * dev_desc->blksz;
		entry = (struct resource_entry *)(content + size);
		add_file_to_list(entry, rsce_base);
	}

	ret = 0;
	printf("Found DTB in %s part\n", boot_partname);

parse_second_pos_dtb:
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	/*
	 * If not find resource file on above, we try to get dtb file from
	 * android second position.
	 */
	if (!content && !fdt_check_header((void *)hdr)) {
		entry = malloc(sizeof(*entry));
		if (!entry) {
			ret = -ENOMEM;
			goto parse_logo;
		}

		memcpy(entry->tag, ENTRY_TAG, sizeof(ENTRY_TAG));
		memcpy(entry->name, DTB_FILE, sizeof(DTB_FILE));
		entry->f_size = fdt_totalsize((void *)hdr);
		entry->f_offset = 0;
		entry->hash_size = 0;

		add_file_to_list(entry, rsce_base);
		free(entry);
		ret = 0;
		printf("Found DTB in %s part(second pos)\n", boot_partname);
	}

parse_logo:
#endif
	/*
	 * Add logo.bmp and logo_kernel.bmp from "logo" parititon
	 *
	 * Provide a "logo" partition for user to store logo.bmp and
	 * logo_kernel.bmp, so that the users can update them from
	 * kernel or user-space dynamically.
	 *
	 * "logo" partition layout, not change order:
	 *
	 *   |----------------------| 0x00
	 *   | raw logo.bmp         |
	 *   |----------------------| N*512-byte aligned
	 *   | raw logo_kernel.bmp  |
	 *   |----------------------|
	 *
	 * N: the sector count of logo.bmp
	 */
	if (part_get_info_by_name(dev_desc, PART_LOGO, &part_info) >= 0) {
		u32 filesz;

		if (!read_logo_bmp("logo.bmp", &part_info, 0, &filesz))
			read_logo_bmp("logo_kernel.bmp", &part_info,
				      filesz, NULL);
	}

	/*
	 * boot_img_hdr_v2 feature.
	 *
	 * If dtb position is present, replace the old with new one if
	 * we don't need to verify DTB hash from resource.img file entry.
	 */
	if (dtb_size) {
#ifndef CONFIG_ROCKCHIP_DTB_VERIFY
		ret = replace_resource_entry(DTB_FILE, rsce_base,
					     dtb_offset, dtb_size);
		if (ret)
			printf("Failed to load dtb from dtb position\n");
		else
#endif
			env_update("bootargs", "androidboot.dtb_idx=0");
	}
err:
	if (content)
		free(content);
out:
	free(hdr);

	return ret;
}

static struct resource_file *get_file_info(struct resource_img_hdr *hdr,
					   const char *name)
{
	struct resource_file *file;
	struct list_head *node;

	if (list_empty(&entrys_head)) {
		if (init_resource_list(hdr))
			return NULL;
	}

	list_for_each(node, &entrys_head) {
		file = list_entry(node, struct resource_file, link);
		if (!strcmp(file->name, name))
			return file;
	}

	return NULL;
}

/*
 * read file from resource partition
 * @buf: destination buf to store file data;
 * @name: file name
 * @offset: blocks offset in the file, 1 block = 512 bytes
 * @len: the size(by bytes) of file to read.
 */
int rockchip_read_resource_file(void *buf, const char *name,
				int offset, int len)
{
	struct resource_file *file;
	struct blk_desc *dev_desc;
	int ret = 0;
	int blks;

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}

	file = get_file_info(NULL, name);
	if (!file) {
		printf("Can't find file:%s\n", name);
		return -ENOENT;
	}

	if (len <= 0 || len > file->f_size)
		len = file->f_size;

	blks = DIV_ROUND_UP(len, dev_desc->blksz);
	ret = blk_dread(dev_desc, file->rsce_base + file->f_offset + offset,
			blks, buf);
	if (ret != blks)
		ret = -EIO;
	else
		ret = len;

	return ret;
}

#ifdef CONFIG_ROCKCHIP_HWID_DTB
#define is_digit(c)		((c) >= '0' && (c) <= '9')
#define is_abcd(c)		((c) >= 'a' && (c) <= 'd')
#define is_equal(c)		((c) == '=')

#define KEY_WORDS_ADC_CTRL	"#_"
#define KEY_WORDS_ADC_CH	"_ch"
#define KEY_WORDS_GPIO		"#gpio"
#define GPIO_SWPORT_DDR		0x04
#define GPIO_EXT_PORT		0x50
#define MAX_ADC_CH_NR		10
#define MAX_GPIO_NR		10

/*
 * How to make it works ?
 *
 * 1. pack dtb into rockchip resource.img, require:
 *    (1) file name end with ".dtb";
 *    (2) file name contains key words, like: ...#_[controller]_ch[channel]=[value]...dtb
 *	  @controller: adc controller name in dts, eg. "saradc", ...;
 *	  @channel: adc channel;
 *	  @value: adc value;
 *    eg: ...#_saradc_ch1=223#_saradc_ch2=650....dtb
 *
 * 2. U-Boot dtsi about adc controller node:
 *    (1) enable "u-boot,dm-pre-reloc;";
 *    (2) must set status "okay";
 */
static int rockchip_read_dtb_by_adc(const char *file_name)
{
	static int cached_v[MAX_ADC_CH_NR];
	int offset_ctrl = strlen(KEY_WORDS_ADC_CTRL);
	int offset_ch = strlen(KEY_WORDS_ADC_CH);
	int ret, channel, len = 0, found = 0, margin = 30;
	char *stradc, *strch, *p;
	char adc_v_string[10];
	char dev_name[32];
	uint32_t raw_adc;
	ulong dtb_adc;

	debug("%s: %s\n", __func__, file_name);

	/* Invalid format ? */
	stradc = strstr(file_name, KEY_WORDS_ADC_CTRL);
	while (stradc) {
		debug("   - substr: %s\n", stradc);

		/* Parse controller name */
		strch = strstr(stradc, KEY_WORDS_ADC_CH);
		len = strch - (stradc + offset_ctrl);
		strlcpy(dev_name, stradc + offset_ctrl, len + 1);

		/* Parse adc channel */
		p = strch + offset_ch;
		if (is_digit(*p) && is_equal(*(p + 1))) {
			channel = *p - '0';
		} else {
			debug("   - invalid format: %s\n", stradc);
			return -EINVAL;
		}

		/*
		 * Read raw adc value
		 *
		 * It doesn't need to read adc value every loop, reading once
		 * is enough. We use cached_v[] to save what we have read, zero
		 * means not read before.
		 */
		if (cached_v[channel] == 0) {
			ret = adc_channel_single_shot(dev_name,
						      channel, &raw_adc);
			if (ret) {
				debug("   - failed to read adc, ret=%d\n", ret);
				return ret;
			}
			cached_v[channel] = raw_adc;
		}

		/* Parse dtb adc value */
		p = strch + offset_ch + 2;	/* 2: channel and '=' */
		while (*p && is_digit(*p)) {
			len++;
			p++;
		}
		strlcpy(adc_v_string, strch + offset_ch + 2, len + 1);
		dtb_adc = simple_strtoul(adc_v_string, NULL, 10);

		if (abs(dtb_adc - cached_v[channel]) <= margin) {
			found = 1;
			stradc = strstr(p, KEY_WORDS_ADC_CTRL);
		} else {
			found = 0;
			break;
		}

		debug("   - parse: controller=%s, channel=%d, dtb_adc=%ld, read=%d %s\n",
		      dev_name, channel, dtb_adc, cached_v[channel], found ? "(Y)" : "");
	}

	return found ? 0 : -ENOENT;
}

static int gpio_parse_base_address(fdt_addr_t *gpio_base_addr)
{
	static int initialized;
	ofnode parent, node;
	const char *name;
	int idx, nr = 0;

	if (initialized)
		return 0;

	parent = ofnode_path("/pinctrl");
	if (!ofnode_valid(parent)) {
		debug("   - Can't find pinctrl node\n");
		return -EINVAL;
	}

	ofnode_for_each_subnode(node, parent) {
		if (!ofnode_get_property(node, "gpio-controller", NULL)) {
			debug("   - Can't find gpio-controller\n");
			continue;
		}

		name = ofnode_get_name(node);
		if (!is_digit((char)*(name + 4))) {
			debug("   - bad gpio node name: %s\n", name);
			continue;
		}

		nr++;
		idx = *(name + 4) - '0';
		gpio_base_addr[idx] = ofnode_get_addr(node);
		debug("   - gpio%d: 0x%x\n", idx, (uint32_t)gpio_base_addr[idx]);
	}

	if (nr == 0) {
		debug("   - parse gpio address failed\n");
		return -EINVAL;
	}

	initialized = 1;

	return 0;
}

/*
 * How to make it works ?
 *
 * 1. pack dtb into rockchip resource.img, require:
 *    (1) file name end with ".dtb";
 *    (2) file name contains key words, like: ...#gpio[pin]=[value]...dtb
 *	  @pin: gpio name, eg. 0a2 means GPIO0A2;
 *	  @value: gpio level, 0 or 1;
 *    eg: ...#gpio0a6=1#gpio1c2=0....dtb
 *
 * 2. U-Boot dtsi about gpio node:
 *    (1) enable "u-boot,dm-pre-reloc;" for all gpio node;
 *    (2) set all gpio status "disabled"(Because we just want their property);
 */
static int rockchip_read_dtb_by_gpio(const char *file_name)
{
	static uint32_t cached_v[MAX_GPIO_NR];
	fdt_addr_t gpio_base_addr[MAX_GPIO_NR];
	int ret, found = 0, offset = strlen(KEY_WORDS_GPIO);
	uint8_t port, pin, bank, lvl, val;
	char *strgpio, *p;
	uint32_t bit;

	debug("%s\n", file_name);

	/* Parse gpio address */
	memset(gpio_base_addr, 0, sizeof(gpio_base_addr));
	ret = gpio_parse_base_address(gpio_base_addr);
	if (ret) {
		debug("   - Can't parse gpio base address: %d\n", ret);
		return ret;
	}

	strgpio = strstr(file_name, KEY_WORDS_GPIO);
	while (strgpio) {
		debug("   - substr: %s\n", strgpio);

		p = strgpio + offset;

		/* Invalid format ? */
		if (!(is_digit(*(p + 0)) && is_abcd(*(p + 1)) &&
		      is_digit(*(p + 2)) && is_equal(*(p + 3)) &&
		      is_digit(*(p + 4)))) {
			debug("   - invalid format: %s\n", strgpio);
			return -EINVAL;
		}

		/* Read gpio value */
		port = *(p + 0) - '0';
		bank = *(p + 1) - 'a';
		pin  = *(p + 2) - '0';
		lvl  = *(p + 4) - '0';

		/*
		 * It doesn't need to read gpio value every loop, reading once
		 * is enough. We use cached_v[] to save what we have read, zero
		 * means not read before.
		 */
		if (cached_v[port] == 0) {
			if (!gpio_base_addr[port]) {
				debug("   - can't find gpio%d base address\n", port);
				return 0;
			}

			/* Input mode */
			val = readl(gpio_base_addr[port] + GPIO_SWPORT_DDR);
			val &= ~(1 << (bank * 8 + pin));
			writel(val, gpio_base_addr[port] + GPIO_SWPORT_DDR);

			cached_v[port] =
				readl(gpio_base_addr[port] + GPIO_EXT_PORT);
		}

		/* Verify result */
		bit = bank * 8 + pin;
		val = cached_v[port] & (1 << bit) ? 1 : 0;

		if (val == !!lvl) {
			found = 1;
			strgpio = strstr(p, KEY_WORDS_GPIO);
		} else {
			found = 0;
			break;
		}

		debug("   - parse: gpio%d%c%d=%d, read=%d %s\n",
		      port, bank + 'a', pin, lvl, val, found ? "(Y)" : "(N)");
	}

	return found ? 0 : -ENOENT;
}

/* Get according to hardware id(GPIO/ADC) */
static struct resource_file *rockchip_read_hwid_dtb(void)
{
	struct resource_file *file;
	struct list_head *node;

	/* Find dtb file according to hardware id(GPIO/ADC) */
	list_for_each(node, &entrys_head) {
		file = list_entry(node, struct resource_file, link);
		if (!strstr(file->name, ".dtb"))
			continue;

		if (strstr(file->name, KEY_WORDS_ADC_CTRL) &&
		    strstr(file->name, KEY_WORDS_ADC_CH) &&
		    !rockchip_read_dtb_by_adc(file->name)) {
			return file;
		} else if (strstr(file->name, KEY_WORDS_GPIO) &&
			   !rockchip_read_dtb_by_gpio(file->name)) {
			return file;
		}
	}

	return NULL;
}
#endif

#ifdef CONFIG_ROCKCHIP_EARLY_DISTRO_DTB
static int rockchip_read_distro_dtb(void *fdt_addr)
{
	const char *cmd = "part list ${devtype} ${devnum} -bootable devplist";
	char *devnum, *devtype, *devplist;
	char devnum_part[12];
	char fdt_hex_str[19];
	char *fs_argv[5];
	int size;
	int ret;

	if (!rockchip_get_bootdev() || !fdt_addr)
		return -ENODEV;

	ret = run_command_list(cmd, -1, 0);
	if (ret)
		return ret;

	devplist = env_get("devplist");
	if (!devplist)
		return -ENODEV;

	devtype = env_get("devtype");
	devnum = env_get("devnum");
	sprintf(devnum_part, "%s:%s", devnum, devplist);
	sprintf(fdt_hex_str, "0x%lx", (ulong)fdt_addr);

#ifdef CONFIG_CMD_FS_GENERIC
	fs_argv[0] = "load";
	fs_argv[1] = devtype,
	fs_argv[2] = devnum_part;
	fs_argv[3] = fdt_hex_str;
	fs_argv[4] = CONFIG_ROCKCHIP_EARLY_DISTRO_DTB_PATH;

	if (do_load(NULL, 0, 5, fs_argv, FS_TYPE_ANY))
		return -EIO;
#endif
	if (fdt_check_header(fdt_addr))
		return -EIO;

	size = fdt_totalsize(fdt_addr);
	if (!sysmem_alloc_base(MEMBLK_ID_FDT, (phys_addr_t)fdt_addr,
			       ALIGN(size, RK_BLK_SIZE) + CONFIG_SYS_FDT_PAD))
		return -ENOMEM;

	printf("Distro DTB: %s\n", CONFIG_ROCKCHIP_EARLY_DISTRO_DTB_PATH);

	return size;
}
#endif

#ifdef CONFIG_ROCKCHIP_DTB_VERIFY
#ifdef CONFIG_DM_CRYPTO
static int crypto_csum(u32 cap, char *input, u32 input_len, u8 *output)
{
	sha_context csha_ctx;
	struct udevice *dev;

	dev = crypto_get_device(cap);
	if (!dev) {
		printf("Can't find expected crypto device\n");
		return -ENODEV;
	}

	csha_ctx.algo = cap;
	csha_ctx.length = input_len;
	crypto_sha_csum(dev, &csha_ctx, (char *)input,
			input_len, output);

	return 0;
}

static int fdt_check_hash(void *fdt_addr, struct resource_file *file)
{
	uchar hash[32];

	if (!file->hash_size)
		return 0;

	if (file->hash_size == 20)
		crypto_csum(CRYPTO_SHA1, fdt_addr, file->f_size, hash);
	else if (file->hash_size == 32)
		crypto_csum(CRYPTO_SHA256, fdt_addr, file->f_size, hash);
	else
		return -EINVAL;

	if (memcmp(hash, file->hash, file->hash_size))
		return -EBADF;

	printf("HASH: OK(c)\n");

	return 0;
}

#else
static int fdt_check_hash(void *fdt_addr, struct resource_file *file)
{
	uchar hash[32];

	if (!file->hash_size)
		return 0;

	if (file->hash_size == 20)
		sha1_csum((const uchar *)fdt_addr, file->f_size, hash);
	else if (file->hash_size == 32)
		sha256_csum((const uchar *)fdt_addr, file->f_size, hash);
	else
		return -EINVAL;

	if (memcmp(hash, file->hash, file->hash_size))
		return -EBADF;

	printf("HASH: OK(s)\n");

	return 0;
}
#endif
#endif	/* CONFIG_ROCKCHIP_DTB_VERIFY */

int rockchip_read_dtb_file(void *fdt_addr)
{
	struct resource_file *file;
	char *def_dtb = DTB_FILE;
	int ret;

	/* search order: "rk-kernel.dtb" -> distro -> hwid */
	file = get_file_info(NULL, def_dtb);
	if (!file) {
#ifdef CONFIG_ROCKCHIP_EARLY_DISTRO_DTB
		ret = rockchip_read_distro_dtb(fdt_addr);
		if (ret > 0)
			return ret; /* found & load done */
#endif
#ifdef CONFIG_ROCKCHIP_HWID_DTB
		file = rockchip_read_hwid_dtb();
#endif
		if (!file)
			return -ENODEV;
	}

	/* found! */
	printf("DTB: %s\n", file->name);
	ret = rockchip_read_resource_file(fdt_addr, file->name, 0, 0);
	if (ret < 0)
		return ret;

	if (fdt_check_header(fdt_addr)) {
		printf("Get a bad DTB file !\n");
		return -EBADF;
	}

	/* Note: We only load the DTB from resource.img to verify */
#ifdef CONFIG_ROCKCHIP_DTB_VERIFY
	if (fdt_check_hash(fdt_addr, file)) {
		printf("Get a bad hash of DTB !\n");
		return -EBADF;
	}
#endif

	if (!sysmem_alloc_base(MEMBLK_ID_FDT, (phys_addr_t)fdt_addr,
			       ALIGN(file->f_size, RK_BLK_SIZE) +
			       CONFIG_SYS_FDT_PAD))
		return -ENOMEM;

	/* Apply DTBO */
#if defined(CONFIG_CMD_DTIMG) && defined(CONFIG_OF_LIBFDT_OVERLAY)
	android_fdt_overlay_apply((void *)fdt_addr);
#endif

	return file->f_size;
}
