// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd.
 */
#include <common.h>
#include <bidram.h>
#include <env.h>
#include <fdt_support.h>
#include <hotkey.h>
#include <malloc.h>
#include <lmb.h>
#include <part.h>
#include <sysmem.h>
#include <timestamp.h>
#include <version.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/atags.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/param.h>
#include <asm/arch-rockchip/common.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Using last bi_dram[...] to initialize "bootm_low" and "bootm_mapsize".
 * This makes lmb_alloc_base() always alloc from tail of sdram.
 * If we don't assign it, bi_dram[0] is used by default and it may cause
 * lmb_alloc_base() fail when bi_dram[0] range is small.
 */
void bootm_mem_init(void)
{
	char bootm_mapsize[32];
	char bootm_low[32];
	u64 start, size;
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (!gd->bd->bi_dram[i].size)
			break;
	}

	start = gd->bd->bi_dram[i - 1].start;
	size = gd->bd->bi_dram[i - 1].size;

	/*
	 * 32-bit kernel: ramdisk/fdt shouldn't be loaded to highmem area(768MB+),
	 * otherwise "Unable to handle kernel paging request at virtual address ...".
	 *
	 * So that we hope limit highest address at 768M, but there comes the the
	 * problem: ramdisk is a compressed image and it expands after descompress,
	 * so it accesses 768MB+ and brings the above "Unable to handle kernel ...".
	 *
	 * We make a appointment that the highest memory address is 512MB, it
	 * makes lmb alloc safer.
	 */
#ifndef CONFIG_ARM64
	if (start >= ((u64)CFG_SYS_SDRAM_BASE + SZ_512M)) {
		start = gd->bd->bi_dram[i - 2].start;
		size = gd->bd->bi_dram[i - 2].size;
	}

	if ((start + size) > ((u64)CFG_SYS_SDRAM_BASE + SZ_512M))
		size = (u64)CFG_SYS_SDRAM_BASE + SZ_512M - start;
#endif
	sprintf(bootm_low, "0x%llx", start);
	sprintf(bootm_mapsize, "0x%llx", size);
	env_set("bootm_low", bootm_low);
	env_set("bootm_mapsize", bootm_mapsize);
}

#ifdef CONFIG_BIDRAM
int board_bidram_reserve(struct bidram *bidram)
{
	struct memblock mem;
	int ret;

	/* board-specific */
#ifdef BOARD_RESERVE_MEM_BASE
	ret = bidram_reserve_by_name("board-specific",
				     BOARD_RESERVE_MEM_BASE,
				     BOARD_RESERVE_MEM_SIZE);
	if (ret)
		return ret;
#endif
	/* ATF */
	mem = param_parse_atf_mem();
	ret = bidram_reserve(MEM_ATF, mem.base, mem.size);
	if (ret)
		return ret;

	/* PSTORE/ATAGS/SHM */
	mem = param_parse_common_resv_mem();
	ret = bidram_reserve(MEM_SHM, mem.base, mem.size);
	if (ret)
		return ret;

	/* OP-TEE */
	mem = param_parse_optee_mem();
	ret = bidram_reserve(MEM_OPTEE, mem.base, mem.size);
	if (ret)
		return ret;

	return 0;
}

#ifdef CONFIG_SYSMEM
int board_sysmem_reserve(struct sysmem *sysmem)
{
#ifdef CONFIG_SKIP_RELOCATE_UBOOT
	if (!sysmem_alloc_base_by_name("NO-RELOC-CODE", CONFIG_TEXT_BASE, SZ_2M)) {
		printf("Failed to reserve sysmem for U-Boot code\n");
		return -ENOMEM;
	}
#endif
	return 0;
}
#endif

parse_fn_t board_bidram_parse_fn(void)
{
	return param_parse_ddr_mem;
}
#endif

