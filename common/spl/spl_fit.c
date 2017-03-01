/*
 * Copyright (C) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <image.h>
#include <libfdt.h>
#include <spl.h>

static ulong fdt_getprop_u32(const void *fdt, int node, const char *prop)
{
	const u32 *cell;
	int len;

	cell = fdt_getprop(fdt, node, prop, &len);
	if (len != sizeof(*cell))
		return -1UL;
	return fdt32_to_cpu(*cell);
}

static int spl_fit_find_config_node(const void *fdt)
{
	const char *name;
	int conf, node, len;

	conf = fdt_path_offset(fdt, FIT_CONFS_PATH);
	if (conf < 0) {
		debug("%s: Cannot find /configurations node: %d\n", __func__,
		      conf);
		return -EINVAL;
	}
	for (node = fdt_first_subnode(fdt, conf);
	     node >= 0;
	     node = fdt_next_subnode(fdt, node)) {
		name = fdt_getprop(fdt, node, "description", &len);
		if (!name) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
			printf("%s: Missing FDT description in DTB\n",
			       __func__);
#endif
			return -EINVAL;
		}
		if (board_fit_config_name_match(name))
			continue;

		debug("Selecting config '%s'", name);

		return node;
	}

	return -1;
}

static int spl_fit_get_image_node(const void *fit, int images,
				  const char *type, int index)
{
	const char *name, *str;
	int node, conf_node;
	int len, i;

	conf_node = spl_fit_find_config_node(fit);
	if (conf_node < 0) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		printf("No matching DT out of these options:\n");
		for (node = fdt_first_subnode(fit, conf_node);
		     node >= 0;
		     node = fdt_next_subnode(fit, node)) {
			name = fdt_getprop(fit, node, "description", &len);
			printf("   %s\n", name);
		}
#endif
		return -ENOENT;
	}

	name = fdt_getprop(fit, conf_node, type, &len);
	if (!name) {
		debug("cannot find property '%s': %d\n", type, len);
		return -EINVAL;
	}

	str = name;
	for (i = 0; i < index; i++) {
		str = strchr(str, '\0') + 1;
		if (!str || (str - name >= len)) {
			debug("no string for index %d\n", index);
			return -E2BIG;
		}
	}

	debug("%s: '%s'\n", type, str);
	node = fdt_subnode_offset(fit, images, str);
	if (node < 0) {
		debug("cannot find image node '%s': %d\n", str, node);
		return -EINVAL;
	}

	return node;
}

static int get_aligned_image_offset(struct spl_load_info *info, int offset)
{
	/*
	 * If it is a FS read, get the first address before offset which is
	 * aligned to ARCH_DMA_MINALIGN. If it is raw read return the
	 * block number to which offset belongs.
	 */
	if (info->filename)
		return offset & ~(ARCH_DMA_MINALIGN - 1);

	return offset / info->bl_len;
}

static int get_aligned_image_overhead(struct spl_load_info *info, int offset)
{
	/*
	 * If it is a FS read, get the difference between the offset and
	 * the first address before offset which is aligned to
	 * ARCH_DMA_MINALIGN. If it is raw read return the offset within the
	 * block.
	 */
	if (info->filename)
		return offset & (ARCH_DMA_MINALIGN - 1);

	return offset % info->bl_len;
}

static int get_aligned_image_size(struct spl_load_info *info, int data_size,
				  int offset)
{
	data_size = data_size + get_aligned_image_overhead(info, offset);

	if (info->filename)
		return data_size;

	return (data_size + info->bl_len - 1) / info->bl_len;
}

static int spl_load_fit_image(struct spl_load_info *info, ulong sector,
			      void *fit, ulong base_offset, int node,
			      struct spl_image_info *image_info)
{
	ulong offset;
	size_t length;
	ulong load_addr, load_ptr, entry;
	void *src;
	ulong overhead;
	int nr_sectors;
	int align_len = ARCH_DMA_MINALIGN - 1;

	offset = fdt_getprop_u32(fit, node, "data-offset") + base_offset;
	length = fdt_getprop_u32(fit, node, "data-size");
	load_addr = fdt_getprop_u32(fit, node, "load");
	if (load_addr == -1UL && image_info)
		load_addr = image_info->load_addr;
	load_ptr = (load_addr + align_len) & ~align_len;
	entry = fdt_getprop_u32(fit, node, "entry");

	overhead = get_aligned_image_overhead(info, offset);
	nr_sectors = get_aligned_image_size(info, length, offset);

	if (info->read(info, sector + get_aligned_image_offset(info, offset),
		       nr_sectors, (void*)load_ptr) != nr_sectors)
		return -EIO;
	debug("image: dst=%lx, offset=%lx, size=%lx\n", load_ptr, offset,
	      (unsigned long)length);

	src = (void *)load_ptr + overhead;
#ifdef CONFIG_SPL_FIT_IMAGE_POST_PROCESS
	board_fit_image_post_process(&src, &length);
#endif

	memcpy((void*)load_addr, src, length);

	if (image_info) {
		image_info->load_addr = load_addr;
		image_info->size = length;
		if (entry == -1UL)
			image_info->entry_point = load_addr;
		else
			image_info->entry_point = entry;
	}

	return 0;
}

int spl_load_simple_fit(struct spl_image_info *spl_image,
			struct spl_load_info *info, ulong sector, void *fit)
{
	int sectors;
	ulong size;
	unsigned long count;
	struct spl_image_info image_info;
	int node, images;
	int base_offset, align_len = ARCH_DMA_MINALIGN - 1;

	/*
	 * Figure out where the external images start. This is the base for the
	 * data-offset properties in each image.
	 */
	size = fdt_totalsize(fit);
	size = (size + 3) & ~3;
	base_offset = (size + 3) & ~3;

	/*
	 * So far we only have one block of data from the FIT. Read the entire
	 * thing, including that first block, placing it so it finishes before
	 * where we will load the image.
	 *
	 * Note that we will load the image such that its first byte will be
	 * at the load address. Since that byte may be part-way through a
	 * block, we may load the image up to one block before the load
	 * address. So take account of that here by subtracting an addition
	 * block length from the FIT start position.
	 *
	 * In fact the FIT has its own load address, but we assume it cannot
	 * be before CONFIG_SYS_TEXT_BASE.
	 */
	fit = (void *)((CONFIG_SYS_TEXT_BASE - size - info->bl_len -
			align_len) & ~align_len);
	sectors = get_aligned_image_size(info, size, 0);
	count = info->read(info, sector, sectors, fit);
	debug("fit read sector %lx, sectors=%d, dst=%p, count=%lu\n",
	      sector, sectors, fit, count);
	if (count == 0)
		return -EIO;

	/* find the node holding the images information */
	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0) {
		debug("%s: Cannot find /images node: %d\n", __func__, images);
		return -1;
	}

	/* find the U-Boot image */
	node = spl_fit_get_image_node(fit, images, "firmware", 0);
	if (node < 0) {
		debug("could not find firmware image, trying loadables...\n");
		node = spl_fit_get_image_node(fit, images, "loadables", 0);
	}
	if (node < 0) {
		debug("%s: Cannot find u-boot image node: %d\n",
		      __func__, node);
		return -1;
	}

	/* Load the image and set up the spl_image structure */
	spl_load_fit_image(info, sector, fit, base_offset, node, spl_image);
	spl_image->os = IH_OS_U_BOOT;

	/* Figure out which device tree the board wants to use */
	node = spl_fit_get_image_node(fit, images, FIT_FDT_PROP, 0);
	if (node < 0) {
		debug("%s: cannot find FDT node\n", __func__);
		return node;
	}

	/*
	 * Read the device tree and place it after the image.
	 * Align the destination address to ARCH_DMA_MINALIGN.
	 */
	image_info.load_addr = spl_image->load_addr + spl_image->size;
	spl_load_fit_image(info, sector, fit, base_offset, node, &image_info);

	return 0;
}
