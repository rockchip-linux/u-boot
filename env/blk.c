// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2008-2011 Freescale Semiconductor, Inc.
 */

/* #define DEBUG */

#include <common.h>
#include <asm/global_data.h>

#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <fdtdec.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <memalign.h>
#include <mmc.h>
#include <part.h>
#include <search.h>
#include <errno.h>
#include <dm/ofnode.h>

#define ENV_BLK_INVALID_OFFSET ((s64)-1)

#if defined(CONFIG_ENV_OFFSET_REDUND)
#define ENV_BLK_OFFSET_REDUND	CONFIG_ENV_OFFSET_REDUND
#else
#define ENV_BLK_OFFSET_REDUND	ENV_BLK_INVALID_OFFSET
#endif

DECLARE_GLOBAL_DATA_PTR;

static inline s64 blk_offset(struct blk_desc *blk_desc, int copy)
{
	s64 offset = CONFIG_ENV_OFFSET;

	if (IS_ENABLED(CONFIG_SYS_REDUNDAND_ENVIRONMENT) && copy)
		offset = ENV_BLK_OFFSET_REDUND;

	return offset;
}

__weak int blk_get_env_addr(struct blk_desc *blk_desc, int copy, u32 *env_addr)
{
	s64 offset = blk_offset(blk_desc, copy);

	if (offset == ENV_BLK_INVALID_OFFSET) {
		printf("Invalid ENV offset, copy=%d\n", copy);
		return -ENOENT;
	}

	if (offset < 0)
		return -EINVAL;

	*env_addr = offset;

	return 0;
}

static const char *init_blk_for_env(struct blk_desc *blk_desc)
{
	/* no nothing */
	return NULL;
}

static void fini_blk_for_env(struct blk_desc *blk_desc)
{
	/* do nothing */
}

#if defined(CONFIG_CMD_SAVEENV) && !defined(CONFIG_SPL_BUILD)
static inline int write_env(struct blk_desc *blk_desc, unsigned long size,
			    unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, n;

	blk_start = ALIGN(offset, blk_desc->blksz) / blk_desc->blksz;
	blk_cnt	  = ALIGN(size, blk_desc->blksz) / blk_desc->blksz;

	n = blk_dwrite(blk_desc, blk_start, blk_cnt, (u_char *)buffer);

	return (n == blk_cnt) ? 0 : -1;
}

static int env_blk_save(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	struct blk_desc *blk_desc;
	u32	offset;
	int	ret, copy = 0;
	const char *errmsg;

	blk_desc = plat_bootdev();
	if (!blk_desc)
		return 1;

	errmsg = init_blk_for_env(blk_desc);
	if (errmsg) {
		printf("%s\n", errmsg);
		return 1;
	}

	ret = env_export(env_new);
	if (ret)
		goto fini;

	if (IS_ENABLED(CONFIG_SYS_REDUNDAND_ENVIRONMENT)) {
		if (gd->env_valid == ENV_VALID)
			copy = 1;
	}

	if (blk_get_env_addr(blk_desc, copy, &offset)) {
		ret = 1;
		goto fini;
	}

	printf("Writing to %s%s(%s)... ", copy ? "redundant " : "",
	       env_get("devtype"), env_get("devnum"));

	if (write_env(blk_desc, CONFIG_ENV_SIZE, offset, (u_char *)env_new)) {
		puts("failed\n");
		ret = 1;
		goto fini;
	}

	ret = 0;

	if (IS_ENABLED(CONFIG_SYS_REDUNDAND_ENVIRONMENT))
		gd->env_valid = gd->env_valid == ENV_REDUND ? ENV_VALID : ENV_REDUND;

fini:
	fini_blk_for_env(blk_desc);

	return ret;
}

static inline int erase_env(struct blk_desc *blk_desc, unsigned long size,
			    unsigned long offset)
{
	uint blk_start, blk_cnt, n;
	void *buf;

	blk_start = ALIGN_DOWN(offset, blk_desc->blksz) / blk_desc->blksz;
	blk_cnt = ALIGN(size, blk_desc->blksz) / blk_desc->blksz;
	buf = calloc(1, blk_desc->blksz * blk_cnt);
	if (!buf) {
		printf("BLK env: no memory\n");
		return 1;
	}

	n = blk_dwrite(blk_desc, blk_start, blk_cnt, buf);
	printf("%d blocks erased at 0x%x: %s\n", n, blk_start,
	       (n == blk_cnt) ? "OK" : "ERROR");

	free(buf);

	return (n == blk_cnt) ? 0 : 1;
}

static int env_blk_erase(void)
{
	struct blk_desc *blk_desc;
	int	ret, copy = 0;
	u32	offset;
	const char *errmsg;

	blk_desc = plat_bootdev();
	if (!blk_desc)
		return 1;

	errmsg = init_blk_for_env(blk_desc);
	if (errmsg) {
		printf("%s\n", errmsg);
		return 1;
	}

	if (blk_get_env_addr(blk_desc, copy, &offset)) {
		ret = CMD_RET_FAILURE;
		goto fini;
	}

	printf("\n");
	ret = erase_env(blk_desc, CONFIG_ENV_SIZE, offset);

	if (IS_ENABLED(CONFIG_SYS_REDUNDAND_ENVIRONMENT)) {
		copy = 1;

		if (blk_get_env_addr(blk_desc, copy, &offset)) {
			ret = CMD_RET_FAILURE;
			goto fini;
		}

		ret |= erase_env(blk_desc, CONFIG_ENV_SIZE, offset);
	}

fini:
	fini_blk_for_env(blk_desc);
	return ret;
}
#endif /* CONFIG_CMD_SAVEENV && !CONFIG_SPL_BUILD */

static inline int read_env(struct blk_desc *blk_desc, unsigned long size,
			   unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, n;

	blk_start = ALIGN(offset, blk_desc->blksz) / blk_desc->blksz;
	blk_cnt	  = ALIGN(size, blk_desc->blksz) / blk_desc->blksz;

	n = blk_dread(blk_desc, blk_start, blk_cnt, (uchar *)buffer);

	return (n == blk_cnt) ? 0 : -1;
}

#if defined(ENV_IS_EMBEDDED)
static int env_blk_load(void)
{
	return 0;
}
#elif defined(CONFIG_SYS_REDUNDAND_ENVIRONMENT)
static int env_blk_load(void)
{
	struct blk_desc *blk_desc;
	u32 offset1, offset2;
	int read1_fail = 0, read2_fail = 0;
	int ret;
	const char *errmsg = NULL;

	ALLOC_CACHE_ALIGN_BUFFER(env_t, tmp_env1, 1);
	ALLOC_CACHE_ALIGN_BUFFER(env_t, tmp_env2, 1);

	blk_desc = plat_bootdev();
	if (!blk_desc)
		return -EIO;

	errmsg = init_blk_for_env(blk_desc);
	if (errmsg) {
		ret = -EIO;
		goto err;
	}

	if (blk_get_env_addr(blk_desc, 0, &offset1) ||
	    blk_get_env_addr(blk_desc, 1, &offset2)) {
		ret = -EIO;
		goto fini;
	}

	read1_fail = read_env(blk_desc, CONFIG_ENV_SIZE, offset1, tmp_env1);
	read2_fail = read_env(blk_desc, CONFIG_ENV_SIZE, offset2, tmp_env2);

	ret = env_import_redund((char *)tmp_env1, read1_fail, (char *)tmp_env2,
				read2_fail, H_EXTERNAL);

fini:
	fini_blk_for_env(blk_desc);
err:
	if (ret)
		env_set_default(errmsg, 0);

	return ret;
}
#else /* ! CONFIG_SYS_REDUNDAND_ENVIRONMENT */
static int env_blk_load(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, CONFIG_ENV_SIZE);
	struct blk_desc *blk_desc;
	u32 offset;
	int ret;
	const char *errmsg;
	env_t *ep = NULL;

	blk_desc = plat_bootdev();
	if (!blk_desc) {
		puts("No available bootdev\n");
		return -EIO;
	}

	errmsg = init_blk_for_env(blk_desc);
	if (errmsg) {
		ret = -EIO;
		goto err;
	}

	if (blk_get_env_addr(blk_desc, 0, &offset)) {
		ret = -EIO;
		goto fini;
	}

	if (read_env(blk_desc, CONFIG_ENV_SIZE, offset, buf)) {
		errmsg = "!read failed";
		ret = -EIO;
		goto fini;
	}

	ret = env_import(buf, 1, H_EXTERNAL);
	if (!ret) {
		ep = (env_t *)buf;
		gd->env_addr = (ulong)&ep->data;
	}

fini:
	fini_blk_for_env(blk_desc);
err:
	if (ret)
		env_set_default(errmsg, 0);

	return ret;
}
#endif /* CONFIG_SYS_REDUNDAND_ENVIRONMENT */

U_BOOT_ENV_LOCATION(env_blk) = {
	.location	= ENVL_BLK,
	ENV_NAME("ENV_BLK")
	.load		= env_blk_load,
#if defined(CONFIG_CMD_SAVEENV) && !defined(CONFIG_XPL_BUILD)
	.save		= env_save_ptr(env_blk_save),
	.erase		= ENV_ERASE_PTR(env_blk_erase)
#endif
};
