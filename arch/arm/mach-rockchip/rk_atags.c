// SPDX-License-Identifier:     GPL-2.0+
/*
 * (C) Copyright 2018 Rockchip Electronics Co., Ltd.
 *
 */

#include <common.h>
#include <asm/arch/rk_atags.h>

#define tag_next(t)	((struct tag *)((u32 *)(t) + (t)->hdr.size))
#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)
#define for_each_tag(t, base)		\
	for (t = base; t->hdr.size; t = tag_next(t))

static int inline bad_magic(u32 magic)
{
	return ((magic != ATAG_CORE) &&
		(magic != ATAG_NONE) &&
		(magic != ATAG_SERIAL) &&
		(magic != ATAG_BOOTDEV) &&
		(magic != ATAG_DDR_MEM) &&
		(magic != ATAG_TOS_MEM));
}

static int inline atags_size_overflow(struct tag *t, u32 tag_size)
{
	return (unsigned long)t + (tag_size << 2) - ATAGS_PHYS_BASE > ATAGS_SIZE;
}

int atags_is_available(void)
{
	struct tag *t = (struct tag *)ATAGS_PHYS_BASE;

	return (t->hdr.magic == ATAG_CORE);
}

int atags_set_tag(u32 magic, void *tagdata)
{
	u32 length, size = 0;
	struct tag *t = (struct tag *)ATAGS_PHYS_BASE;

	if (!tagdata)
		return -ENODATA;

	if (bad_magic(magic)) {
		printf("%s: magic(%x) is not support\n", __func__, magic);
		return -EINVAL;
	}

	/* Not allowed to be set by user directly, so do nothing */
	if ((magic == ATAG_CORE) || (magic == ATAG_NONE))
		return -EPERM;

	/* If not initialized, setup now! */
	if (t->hdr.magic != ATAG_CORE) {
		t->hdr.magic = ATAG_CORE;
		t->hdr.size = tag_size(tag_core);
		t->u.core.flags = 0;
		t->u.core.pagesize = 0;
		t->u.core.rootdev = 0;

		t = tag_next(t);
	} else {
		/* Find the end, and use it as a new tag */
		for_each_tag(t, (struct tag *)ATAGS_PHYS_BASE) {
			/*
			 * We had better check magic to avoid traversing an
			 * unknown tag, in case of atags has been damaged by
			 * some unknown reason.
			 */
			if (bad_magic(t->hdr.magic)) {
				printf("%s: find unknown magic(%x)\n",
				       __func__, t->hdr.magic);
				return -EINVAL;
			}

			if (t->hdr.magic == ATAG_NONE)
				break;
		}
	}

	/* Initialize new tag */
	switch (magic) {
	case ATAG_SERIAL:
		size = tag_size(tag_serial);
		break;
	case ATAG_BOOTDEV:
		size = tag_size(tag_bootdev);
		break;
	case ATAG_TOS_MEM:
		size = tag_size(tag_tos_mem);
		break;
	case ATAG_DDR_MEM:
		size = tag_size(tag_ddr_mem);
		break;
	};

	if (atags_size_overflow(t, size)) {
		printf("%s: failed! no memory to setup magic(%x), max_mem=0x%x\n",
		       __func__, magic, ATAGS_SIZE);
		return -ENOMEM;
	}

	/* It's okay to setup a new tag */
	t->hdr.magic = magic;
	t->hdr.size = size;
	length = (t->hdr.size << 2) - sizeof(struct tag_header);
	memcpy(&t->u, (char *)tagdata, length);

	/* Next tag */
	t = tag_next(t);

	/* Setup done */
	t->hdr.magic = ATAG_NONE;
	t->hdr.size = 0;

	return 0;
}

struct tag *atags_get_tag(u32 magic)
{
	struct tag *t;

	for_each_tag(t, (struct tag *)ATAGS_PHYS_BASE) {
		if (bad_magic(t->hdr.magic)) {
			printf("%s: find unknown magic(%x)\n",
			       __func__, t->hdr.magic);
			return NULL;
		}

		if (t->hdr.magic == magic)
			return t;
	}

	return NULL;
}

void atags_destroy(void)
{
	memset((char *)ATAGS_PHYS_BASE, 0, sizeof(struct tag));
}

#if (defined(CONFIG_DEBUG_ATAGS) || defined(DEBUG)) && \
    !defined(CONFIG_SPL_BUILD) && !defined(CONFIG_TPL_BUILD)
void atags_print_tag(struct tag *t)
{
	u32 i;

	if (!t)
		return;

	switch (t->hdr.magic) {
	case ATAG_SERIAL:
		printf("[serial]:\n");
		printf("     magic = 0x%x\n", t->hdr.magic);
		printf("      size = 0x%x\n\n", t->hdr.size << 2);
		printf("   version = 0x%x\n", t->u.serial.version);
		printf("    enable = 0x%x\n", t->u.serial.enable);
		printf("      addr = 0x%llx\n", t->u.serial.addr);
		printf("  baudrate = 0x%x\n", t->u.serial.baudrate);
		printf("    m_mode = 0x%x\n", t->u.serial.m_mode);
		for (i = 0; i < ARRAY_SIZE(t->u.serial.reserved); i++)
			printf("    res[%d] = 0x%x\n", i, t->u.serial.reserved[i]);
		break;
	case ATAG_BOOTDEV:
		printf("[bootdev]:\n");
		printf("     magic = 0x%x\n", t->hdr.magic);
		printf("      size = 0x%x\n\n", t->hdr.size << 2);
		printf("   version = 0x%x\n", t->u.bootdev.version);
		printf("   devtype = 0x%x\n", t->u.bootdev.devtype);
		printf("    devnum = 0x%x\n", t->u.bootdev.devnum);
		printf("      mode = 0x%x\n", t->u.bootdev.mode);
		for (i = 0; i < ARRAY_SIZE(t->u.bootdev.reserved); i++)
			printf("    res[%d] = 0x%x\n",
			       i, t->u.bootdev.reserved[i]);
		break;
	case ATAG_TOS_MEM:
		printf("[tos_mem]:\n");
		printf("     magic = 0x%x\n", t->hdr.magic);
		printf("      size = 0x%x\n\n", t->hdr.size << 2);
		printf("   version = 0x%x\n", t->u.tos_mem.version);
		printf("   tee_mem:\n");
		printf("            name = %s\n", t->u.tos_mem.tee_mem.name);
		printf("        phy_addr = 0x%llx\n", t->u.tos_mem.tee_mem.phy_addr);
		printf("            size = 0x%x\n", t->u.tos_mem.tee_mem.size);
		printf("           flags = 0x%x\n", t->u.tos_mem.tee_mem.flags);
		printf("   drm_mem:\n");
		printf("            name = %s\n", t->u.tos_mem.drm_mem.name);
		printf("        phy_addr = 0x%llx\n", t->u.tos_mem.drm_mem.phy_addr);
		printf("            size = 0x%x\n", t->u.tos_mem.drm_mem.size);
		printf("           flags = 0x%x\n", t->u.tos_mem.drm_mem.flags);
		for (i = 0; i < ARRAY_SIZE(t->u.tos_mem.reserved); i++)
			printf("   res[%d] = 0x%llx\n", i, t->u.tos_mem.reserved[i]);
		break;
	case ATAG_DDR_MEM:
		printf("[ddr_mem]:\n");
		printf("     magic = 0x%x\n", t->hdr.magic);
		printf("      size = 0x%x\n\n", t->hdr.size << 2);
		printf("     count = 0x%x\n", t->u.ddr_mem.count);
		printf("   version = 0x%x\n", t->u.ddr_mem.version);
		for (i = 0; i < ARRAY_SIZE(t->u.ddr_mem.bank); i++)
			printf("  bank[%d] = 0x%llx\n", i, t->u.ddr_mem.bank[i]);
		for (i = 0; i < ARRAY_SIZE(t->u.ddr_mem.reserved); i++)
			printf("    res[%d] = 0x%x\n", i, t->u.ddr_mem.reserved[i]);
		break;
	case ATAG_CORE:
		printf("[core]:\n");
		printf("     magic = 0x%x\n", t->hdr.magic);
		printf("      size = 0x%x\n\n", t->hdr.size << 2);
		printf("     flags = 0x%x\n", t->u.core.flags);
		printf("  pagesize = 0x%x\n", t->u.core.pagesize);
		printf("   rootdev = 0x%x\n", t->u.core.rootdev);
		break;
	default:
		printf("%s: magic(%x) is not support\n", __func__, t->hdr.magic);
	}

	printf("\n");
}

void atags_print_all_tags(void)
{
	struct tag *t;

	for_each_tag(t, (struct tag *)ATAGS_PHYS_BASE) {
		if (bad_magic(t->hdr.magic)) {
			printf("%s: find unknown magic(%x)\n",
			       __func__, t->hdr.magic);
			return;
		}

		atags_print_tag(t);
	}
}

void atags_test(void)
{
	struct tag_serial t_serial;
	struct tag_bootdev t_bootdev;
	struct tag_ddr_mem t_ddr_mem;
	struct tag_tos_mem t_tos_mem;

	memset(&t_serial,  0x1, sizeof(t_serial));
	memset(&t_bootdev, 0x2, sizeof(t_bootdev));
	memset(&t_ddr_mem, 0x3, sizeof(t_ddr_mem));
	memset(&t_tos_mem, 0x4, sizeof(t_tos_mem));

	memcpy(&t_tos_mem.tee_mem.name, "tee_mem", 8);
	memcpy(&t_tos_mem.drm_mem.name, "drm_mem", 8);

	/* First pre-loader must call it before atags_set_tag() */
	atags_destroy();

	atags_set_tag(ATAG_SERIAL,  &t_serial);
	atags_set_tag(ATAG_BOOTDEV, &t_bootdev);
	atags_set_tag(ATAG_DDR_MEM, &t_ddr_mem);
	atags_set_tag(ATAG_TOS_MEM, &t_tos_mem);

	atags_print_all_tags();
}

static int dump_atags(cmd_tbl_t *cmdtp, int flag,
		      int argc, char * const argv[])
{
	atags_print_all_tags();
	return 0;
}

U_BOOT_CMD(
	atags, 1, 1, dump_atags,
	"Dump the content of the atags",
	""
);
#else
void inline atags_print_tag(struct tag *t) {}
void inline atags_print_all_tags(void) {}
void inline atags_test(void) {}
#endif
