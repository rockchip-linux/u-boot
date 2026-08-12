// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 */

#include <config.h>
#include <bidram.h>
#include <dm.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#ifdef CONFIG_ARM
#include <asm/armv8/mmu.h>
#endif
#include <asm/global_data.h>
#include <asm/arch-rockchip/param.h>
#include <asm/io.h>
#include <asm/arch-rockchip/sdram.h>
#include <dm/uclass-internal.h>

DECLARE_GLOBAL_DATA_PTR;

#define TRUST_PARAMETER_OFFSET    (34 * 1024 * 1024)

size_t rockchip_sdram_size(phys_addr_t reg)
{
	u32 rank, cs0_col, bk, cs0_row, cs1_row, bw, row_3_4;
	size_t chipsize_mb = 0;
	size_t size_mb = 0;
	u32 ch;
	u32 cs1_col = 0;
	u32 bg = 0;
	u32 dbw, dram_type;
	u32 sys_reg2 = readl((void *)reg);
	u32 sys_reg3 = readl((void *)(reg + 4));
	u32 ch_num = 1 + ((sys_reg2 >> SYS_REG_NUM_CH_SHIFT)
		       & SYS_REG_NUM_CH_MASK);
	u32 version = (sys_reg3 >> SYS_REG_VERSION_SHIFT) &
		      SYS_REG_VERSION_MASK;

	dram_type = (sys_reg2 >> SYS_REG_DDRTYPE_SHIFT) & SYS_REG_DDRTYPE_MASK;
	if (version >= 3)
		dram_type |= ((sys_reg3 >> SYS_REG_EXTEND_DDRTYPE_SHIFT) &
			      SYS_REG_EXTEND_DDRTYPE_MASK) << 3;
	debug("%s %x %x\n", __func__, (u32)reg, sys_reg2);
	debug("%s %x %x\n", __func__, (u32)reg + 4, sys_reg3);
	for (ch = 0; ch < ch_num; ch++) {
		rank = 1 + (sys_reg2 >> SYS_REG_RANK_SHIFT(ch) &
			SYS_REG_RANK_MASK);
		cs0_col = 9 + (sys_reg2 >> SYS_REG_COL_SHIFT(ch) &
			  SYS_REG_COL_MASK);
		cs1_col = cs0_col;

		bk = 3 - ((sys_reg2 >> SYS_REG_BK_SHIFT(ch)) & SYS_REG_BK_MASK);
		/*
		 * SYS_REG_BK(Version 3):
		 * 1) Except LPDDR5 0:8bank(bk=3), 1:4bank(bk=2)
		 * 2) LPDDR5 0:8bank(bk=3), 1:16bank(bk=4)
		 */
		if (version == 3 && dram_type == LPDDR5 && bk == 2)
			bk = 4;

		if (version >= 2) {
			cs1_col = 9 + (sys_reg3 >> SYS_REG_CS1_COL_SHIFT(ch) &
				  SYS_REG_CS1_COL_MASK);
			if (((sys_reg3 >> SYS_REG_EXTEND_CS0_ROW_SHIFT(ch) &
			    SYS_REG_EXTEND_CS0_ROW_MASK) << 2) + (sys_reg2 >>
			    SYS_REG_CS0_ROW_SHIFT(ch) &
			    SYS_REG_CS0_ROW_MASK) == 7)
				cs0_row = 12;
			else
				cs0_row = 13 + (sys_reg2 >>
					  SYS_REG_CS0_ROW_SHIFT(ch) &
					  SYS_REG_CS0_ROW_MASK) +
					  ((sys_reg3 >>
					  SYS_REG_EXTEND_CS0_ROW_SHIFT(ch) &
					  SYS_REG_EXTEND_CS0_ROW_MASK) << 2);
			if (((sys_reg3 >> SYS_REG_EXTEND_CS1_ROW_SHIFT(ch) &
			    SYS_REG_EXTEND_CS1_ROW_MASK) << 2) + (sys_reg2 >>
			    SYS_REG_CS1_ROW_SHIFT(ch) &
			    SYS_REG_CS1_ROW_MASK) == 7)
				cs1_row = 12;
			else
				cs1_row = 13 + (sys_reg2 >>
					  SYS_REG_CS1_ROW_SHIFT(ch) &
					  SYS_REG_CS1_ROW_MASK) +
					  ((sys_reg3 >>
					  SYS_REG_EXTEND_CS1_ROW_SHIFT(ch) &
					  SYS_REG_EXTEND_CS1_ROW_MASK) << 2);
		} else {
			cs0_row = 13 + (sys_reg2 >> SYS_REG_CS0_ROW_SHIFT(ch) &
				SYS_REG_CS0_ROW_MASK);
			cs1_row = 13 + (sys_reg2 >> SYS_REG_CS1_ROW_SHIFT(ch) &
				SYS_REG_CS1_ROW_MASK);
		}
		bw = (2 >> ((sys_reg2 >> SYS_REG_BW_SHIFT(ch)) &
			SYS_REG_BW_MASK));
		row_3_4 = sys_reg2 >> SYS_REG_ROW_3_4_SHIFT(ch) &
			SYS_REG_ROW_3_4_MASK;
		if (dram_type == DDR4) {
			dbw = (sys_reg2 >> SYS_REG_DBW_SHIFT(ch)) &
				SYS_REG_DBW_MASK;
			bg = (dbw == 2) ? 2 : 1;
		}
		chipsize_mb = (1 << (cs0_row + cs0_col + bk + bg + bw - 20));

		if (rank > 1)
			chipsize_mb += chipsize_mb >> ((cs0_row - cs1_row) +
				       (cs0_col - cs1_col));
		if (row_3_4)
			chipsize_mb = chipsize_mb * 3 / 4;
		size_mb += chipsize_mb;
		if (rank > 1)
			debug("rank %d cs0_col %d cs1_col %d bk %d cs0_row %d\
			       cs1_row %d bw %d row_3_4 %d\n",
			       rank, cs0_col, cs1_col, bk, cs0_row,
			       cs1_row, bw, row_3_4);
		else
			debug("rank %d cs0_col %d bk %d cs0_row %d\
			       bw %d row_3_4 %d\n",
			       rank, cs0_col, bk, cs0_row,
			       bw, row_3_4);
	}

	/*
	 * This is workaround for issue we can't get correct size for 4GB ram
	 * in 32bit system and available before we really need ram space
	 * out of 4GB, eg.enable ARM LAPE(rk3288 supports 8GB ram).
	 * The size of 4GB is '0x1 00000000', and this value will be truncated
	 * to 0 in 32bit system, and system can not get correct ram size.
	 * Rockchip SoCs reserve a blob of space for peripheral near 4GB,
	 * and we are now setting SDRAM_MAX_SIZE as max available space for
	 * ram in 4GB, so we can use this directly to workaround the issue.
	 * TODO:
	 *   1. update correct value for SDRAM_MAX_SIZE as what dram
	 *   controller sees.
	 *   2. update board_get_usable_ram_top() and dram_init_banksize()
	 *   to reserve memory for peripheral space after previous update.
	 */
	if (!IS_ENABLED(CONFIG_ARM64) && size_mb > (SDRAM_MAX_SIZE >> 20))
		size_mb = (SDRAM_MAX_SIZE >> 20);

	return (size_t)size_mb << 20;
}

#ifndef CONFIG_SPL_BUILD
int dram_init_banksize(void)
{
#if CONFIG_IS_ENABLED(BIDRAM)
	bidram_gen_gd_bi_dram();
#else
	param_simple_parse_ddr_mem(1);
#endif
	return 0;
}
#endif

int dram_init(void)
{
#if CONFIG_IS_ENABLED(BIDRAM)
	gd->ram_size = bidram_get_ram_size();
#elif defined(CONFIG_SPL_BUILD)
	gd->ram_size = param_simple_parse_ddr_mem(0);
	if (!gd->ram_size)
		return -ENOMEM;
#else
	struct ram_info ram;
	struct udevice *dev;
	int ret;

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return ret;
	}
	ret = ram_get_info(dev, &ram);
	if (ret) {
		debug("Cannot get DRAM size: %d\n", ret);
		return ret;
	}
	gd->ram_base = ram.base;
	gd->ram_size = ram.size;
	debug("SDRAM base=%lx, size=%lx\n",
	      (unsigned long)ram.base, (unsigned long)ram.size);
#endif

	return 0;
}

phys_addr_t board_get_usable_ram_top(phys_size_t total_size)
{
	u64 top = CFG_SYS_SDRAM_BASE + SDRAM_MAX_SIZE;

	/* Make sure U-Boot only uses the space below the 4G address boundary if possible */
	if (CFG_SYS_SDRAM_BASE < SZ_4G)
		top = min_t(u64, top, SZ_4G);

	return (gd->ram_top > top) ? top : gd->ram_top;
}
