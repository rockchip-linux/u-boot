/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <adc.h>
#include <asm/io.h>
#include <malloc.h>
#include <linux/list.h>
#include <asm/arch/resource_img.h>
#include <boot_rkimg.h>
#include <dm/ofnode.h>
#ifdef CONFIG_ANDROID_AB
#include <android_avb/libavb_ab.h>
#include <android_avb/rk_avb_ops_user.h>
#endif
#ifdef CONFIG_ANDROID_BOOT_IMAGE
#include <android_bootloader.h>
#include <android_image.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define PART_RESOURCE			"resource"
#define RESOURCE_MAGIC			"RSCE"
#define RESOURCE_MAGIC_SIZE		4
#define RESOURCE_VERSION		0
#define CONTENT_VERSION			0
#define ENTRY_TAG			"ENTR"
#define ENTRY_TAG_SIZE			4
#define MAX_FILE_NAME_LEN		256

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
	uint32_t	f_offset;
	uint32_t	f_size;
};

struct resource_file {
	char		name[MAX_FILE_NAME_LEN];
	uint32_t	f_offset;
	uint32_t	f_size;
	struct list_head link;
	uint32_t 	rsce_base;	/* Base addr of resource */
};

static LIST_HEAD(entrys_head);

static int resource_image_check_header(const struct resource_img_hdr *hdr)
{
	int ret;

	ret = memcmp(RESOURCE_MAGIC, hdr->magic, RESOURCE_MAGIC_SIZE);
	if (ret) {
		printf("bad resource image magic: %s\n",
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
	list_add_tail(&file->link, &entrys_head);
	debug("entry:%p  %s offset:%d size:%d\n",
	      entry, file->name, file->f_offset, file->f_size);

	return 0;
}

static int init_resource_list(struct resource_img_hdr *hdr)
{
	struct resource_entry *entry;
	void *content;
	int size;
	int ret;
	int e_num;
	int offset = 0;
	int resource_found = 0;
	struct blk_desc *dev_desc;
	disk_partition_t part_info;

/*
 * Primary detect AOSP format image, try to get resource image from
 * boot/recovery partition. If not, it's an RK format image and try
 * to get from resource partition.
 */
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	struct andr_img_hdr *andr_hdr;
	char *boot_partname = PART_BOOT;
#endif

	if (hdr) {
		content = (void *)((char *)hdr
				   + (hdr->c_offset) * RK_BLK_SIZE);
		for (e_num = 0; e_num < hdr->e_nums; e_num++) {
			size = e_num * hdr->e_blks * RK_BLK_SIZE;
			entry = (struct resource_entry *)(content + size);
			add_file_to_list(entry, offset);
		}
		return 0;
	}

	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}
	hdr = memalign(ARCH_DMA_MINALIGN, RK_BLK_SIZE);
	if (!hdr) {
		printf("%s: out of memory!\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_ANDROID_BOOT_IMAGE
	/* Get boot mode from misc */
#ifndef CONFIG_ANDROID_AB
	if (rockchip_get_boot_mode() == BOOT_MODE_RECOVERY)
		boot_partname = PART_RECOVERY;
#endif

	/* Read boot/recovery and chenc if this is an AOSP img */
#ifdef CONFIG_ANDROID_AB
	char slot_suffix[3] = {0};

	if (rk_avb_get_current_slot(slot_suffix))
		goto out;
	boot_partname = android_str_append(boot_partname, slot_suffix);
	if (boot_partname == NULL)
		goto out;
#endif
	ret = part_get_info_by_name(dev_desc, boot_partname, &part_info);
	if (ret < 0) {
		printf("%s: failed to get %s part, ret=%d\n",
		       __func__, boot_partname, ret);
		/* RKIMG can support part table without 'boot' */
		goto next;
	}

	/*
	 * Only read header and check magic, is a AOSP format image?
	 * If so, get resource image from second part.
	 */
	andr_hdr = (void *)hdr;
	ret = blk_dread(dev_desc, part_info.start, 1, andr_hdr);
	if (ret != 1) {
		printf("%s: failed to read %s hdr, ret=%d\n",
		       __func__, part_info.name, ret);
		goto out;
	}
	ret = android_image_check_header(andr_hdr);
	if (!ret) {
		debug("%s: Load resource from %s second pos\n",
		      __func__, part_info.name);
		/* Read resource from second offset */
		offset = part_info.start * RK_BLK_SIZE;
		offset += andr_hdr->page_size;
		offset += ALIGN(andr_hdr->kernel_size, andr_hdr->page_size);
		offset += ALIGN(andr_hdr->ramdisk_size, andr_hdr->page_size);
		offset = offset / RK_BLK_SIZE;

		resource_found = 1;
	}
next:
#endif
	/*
	 * If not found resource image in AOSP format images(boot/recovery part),
	 * try to read RK format images(resource part).
	 */
	if (!resource_found) {
		debug("%s: Load resource from resource part\n", __func__);
		/* Read resource from Rockchip Resource partition */
		ret = part_get_info_by_name(dev_desc, PART_RESOURCE, &part_info);
		if (ret < 0) {
			printf("%s: failed to get resource part, ret=%d\n",
			       __func__, ret);
			goto out;
		}
		offset = part_info.start;
	}

	/* Only read header and check magic */
	ret = blk_dread(dev_desc, offset, 1, hdr);
	if (ret != 1) {
		printf("%s: failed to read resource hdr, ret=%d\n",
		       __func__, ret);
		goto out;
	}

	ret = resource_image_check_header(hdr);
	if (ret < 0)
		goto out;

	content = memalign(ARCH_DMA_MINALIGN,
			   hdr->e_blks * hdr->e_nums * RK_BLK_SIZE);
	if (!content) {
		printf("%s: failed to alloc memory for content\n", __func__);
		goto out;
	}

	/* Read all entries from resource image */
	ret = blk_dread(dev_desc, offset + hdr->c_offset,
			hdr->e_blks * hdr->e_nums, content);
	if (ret != (hdr->e_blks * hdr->e_nums)) {
		printf("%s: failed to read resource entries, ret=%d\n",
		       __func__, ret);
		goto err;
	}

	for (e_num = 0; e_num < hdr->e_nums; e_num++) {
		size = e_num * hdr->e_blks * RK_BLK_SIZE;
		entry = (struct resource_entry *)(content + size);
		add_file_to_list(entry, offset);
	}

err:
	free(content);
out:
	free(hdr);

	return 0;
}

static struct resource_file *get_file_info(struct resource_img_hdr *hdr,
					   const char *name)
{
	struct resource_file *file;
	struct list_head *node;

	if (list_empty(&entrys_head))
		init_resource_list(hdr);

	list_for_each(node, &entrys_head) {
		file = list_entry(node, struct resource_file, link);
		if (!strcmp(file->name, name))
			return file;
	}

	return NULL;
}

int rockchip_get_resource_file(void *buf, const char *name)
{
	struct resource_file *file;

	file = get_file_info(buf, name);

	return file->f_offset;
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
	int ret = 0;
	int blks;
	struct blk_desc *dev_desc;

	file = get_file_info(NULL, name);
	if (!file) {
		printf("Can't find file:%s\n", name);
		return -ENOENT;
	}

	if (len <= 0 || len > file->f_size)
		len = file->f_size;
	blks = DIV_ROUND_UP(len, RK_BLK_SIZE);
	dev_desc = rockchip_get_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return -ENODEV;
	}
	ret = blk_dread(dev_desc, file->rsce_base + file->f_offset + offset,
			blks, buf);
	if (ret != blks)
		ret = -EIO;
	else
		ret = len;

	return ret;
}

#define is_digit(c)		((c) >= '0' && (c) <= '9')
#define is_abcd(c)		((c) >= 'a' && (c) <= 'd')
#define is_equal(c)		((c) == '=')

#define DTB_FILE		"rk-kernel.dtb"
#define KEY_WORDS_ADC_CTRL	"#_"
#define KEY_WORDS_ADC_CH	"_ch"
#define KEY_WORDS_GPIO		"#gpio"
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
	uint32_t raw_adc;
	unsigned long dtb_adc;
	char *stradc, *strch, *p;
	char adc_v_string[10];
	char dev_name[32];

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
	static int initial;
	ofnode parent, node;
	int i = 0;

	if (initial)
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

		gpio_base_addr[i++] = ofnode_get_addr(node);
		debug("   - gpio%d: 0x%x\n", i - 1, (uint32_t)gpio_base_addr[i - 1]);
	}

	if (i == 0) {
		debug("   - parse gpio address failed\n");
		return -EINVAL;
	}

	initial = 1;

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

		/* Parse gpio address */
		ret = gpio_parse_base_address(gpio_base_addr);
		if (ret) {
			debug("   - Can't parse gpio base address: %d\n", ret);
			return ret;
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
		if (cached_v[port] == 0)
			cached_v[port] =
				readl(gpio_base_addr[port] + GPIO_EXT_PORT);

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
		      port, bank + 'a', pin, lvl, val, found ? "(Y)" : "");
	}

	return found ? 0 : -ENOENT;
}

int rockchip_read_dtb_file(void *fdt_addr)
{
	struct resource_file *file;
	struct list_head *node;
	char *dtb_name = DTB_FILE;
	int ret;

	if (list_empty(&entrys_head))
		init_resource_list(NULL);

	list_for_each(node, &entrys_head) {
		file = list_entry(node, struct resource_file, link);
		if (!strstr(file->name, ".dtb"))
			continue;

		if (strstr(file->name, KEY_WORDS_ADC_CTRL) &&
		    strstr(file->name, KEY_WORDS_ADC_CH) &&
		    !rockchip_read_dtb_by_adc(file->name)) {
			dtb_name = file->name;
			break;
		} else if (strstr(file->name, KEY_WORDS_GPIO) &&
			   !rockchip_read_dtb_by_gpio(file->name)) {
			dtb_name = file->name;
			break;
		}
	}

	printf("DTB: %s\n", dtb_name);

	ret = rockchip_read_resource_file((void *)fdt_addr, dtb_name, 0, 0);
	if (ret < 0)
		return ret;

#if defined(CONFIG_CMD_DTIMG) && \
    defined(CONFIG_OF_LIBFDT_OVERLAY) && defined(CONFIG_USING_KERNEL_DTB)
	android_fdt_overlay_apply((void *)fdt_addr);
#endif

	return ret;
}
