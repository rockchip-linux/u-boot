/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/* #define DEBUG */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <search.h>
#include <errno.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

char *env_name_spec = "RK STORAGE";

#ifdef ENV_IS_EMBEDDED
env_t *env_ptr = &environment;
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr;
#endif /* ENV_IS_EMBEDDED */

DEFINE_CACHE_ALIGN_BUFFER(char, env_buf, CONFIG_ENV_SIZE);

extern uint32 StorageUbootSysDataStore(uint32 Index, void *Buf);
extern uint32 StorageUbootSysDataLoad(uint32 Index, void *Buf);

#if !defined(CONFIG_ENV_OFFSET)
#define CONFIG_ENV_OFFSET           0
#endif

int env_init(void)
{
	/* use default */
	gd->env_addr	= (ulong)&default_environment[0];
	gd->env_valid	= 1;

	return 0;
}

#ifdef CONFIG_CMD_SAVEENV
static inline int write_env(unsigned long size,
			    unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, i;
	blk_start   = ALIGN(offset, RK_BLK_SIZE) / RK_BLK_SIZE;
	blk_cnt     = ALIGN(size, RK_BLK_SIZE) / RK_BLK_SIZE;

	for (i = 0;i < blk_cnt;i++)
	{
		if(StorageUbootSysDataStore(blk_start + i,
				(void*)buffer + i * RK_BLK_SIZE))
		{
			printf("write_env failed at %d\n", blk_start + i);
			return -1;
		}
	}

	return 0;
}

int saveenv(void)
{
	env_t *env_new = (env_t *)env_buf;
	ssize_t	len;
	char	*res;

	res = (char *)env_new->data;
	len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
	if (len < 0) {
		error("Cannot export environment: errno = %d\n", errno);
		return -1;
	}

	env_new->crc = crc32_rk(0, env_new->data, ENV_SIZE);
	printf("Writing env to storage... \n");
	if (write_env(CONFIG_ENV_SIZE, CONFIG_ENV_OFFSET, (u_char *)env_new)) {
		puts("failed\n");
		return -1;
	}

	puts("done\n");
	return 0;
}
#endif /* CONFIG_CMD_SAVEENV */

static inline int read_env(unsigned long size,
			   unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, i;
	blk_start	= ALIGN(offset, RK_BLK_SIZE) / RK_BLK_SIZE;
	blk_cnt		= ALIGN(size, RK_BLK_SIZE) / RK_BLK_SIZE;

	for (i = 0;i < blk_cnt;i++)
	{
		if(StorageUbootSysDataLoad(blk_start + i, 
				(void*)buffer + i * RK_BLK_SIZE))
		{
			printf("read_env failed at %d\n", blk_start + i);
			return -1;
		}
	}

	return 0;
}

//base on env_import
static int env_append(const char *buf, int check)
{
	env_t *ep = (env_t *)buf;

	if (check) {
		uint32_t crc;

        	memcpy(&crc, &ep->crc, sizeof(crc));

		if (crc32_rk(0, ep->data, ENV_SIZE) != crc) {
			return 0;
		}
	}

	if (himport_r(&env_htab, (char *)ep->data, ENV_SIZE, '\0', H_NOCLEAR, 0,
			0, NULL)) {
		gd->flags |= GD_FLG_ENV_READY;
		return 1;
	}

	printf("Cannot import environment: errno = %d\n", errno);

	return 0;
}

void env_relocate_spec(void)
{
#if !defined(ENV_IS_EMBEDDED)
	//setup default env.
	set_default_env(NULL);

	//override with saved env.
	if (!read_env(CONFIG_ENV_SIZE, CONFIG_ENV_OFFSET, env_buf)) {
		env_append(env_buf, 1);
	}
#endif
}
