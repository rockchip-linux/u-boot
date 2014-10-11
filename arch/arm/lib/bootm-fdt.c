/*
 * Copyright (c) 2013, Google Inc.
 *
 * Copyright (C) 2011
 * Corscience GmbH & Co. KG - Simon Schwarz <schwarz@corscience.de>
 *  - Added prep subcommand support
 *  - Reorganized source - modeled after powerpc version
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdt_support.h>
#include <asm/armv7.h>

DECLARE_GLOBAL_DATA_PTR;

int arch_fixup_fdt(void *blob)
{
	bd_t *bd = gd->bd;
	int bank, ret;

#if defined(CONFIG_ROCKCHIP) && defined(CONFIG_RK_MAX_DRAM_BANKS)
	u64 _start[CONFIG_RK_MAX_DRAM_BANKS];
	u64 _size[CONFIG_RK_MAX_DRAM_BANKS];
	for (bank = 0; bank < CONFIG_RK_MAX_DRAM_BANKS; bank++) {
		if (!bd->rk_dram[bank].size)
			break;
		_start[bank] = bd->rk_dram[bank].start;
		_size[bank] = bd->rk_dram[bank].size;
#ifdef CONFIG_MAX_MEM_ADDR
		if (_start[bank] < CONFIG_MAX_MEM_ADDR) {
			if ((_start[bank] + _size[bank]) >= CONFIG_MAX_MEM_ADDR) {
				//resize bank.
				_size[bank] = CONFIG_MAX_MEM_ADDR - _start[bank];
			}
		} else {
			_start[bank] = CONFIG_MAX_MEM_ADDR;
			_size[bank] = 0;
		}
#endif
		printf("Add bank:%016llx, %016llx\n", _start[bank], _size[bank]);
	}
	if (bank) {
		return fdt_fixup_memory_banks(blob, _start, _size, bank);
	}
#endif /* CONFIG_ROCKCHIP */

	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] = bd->bi_dram[bank].start;
		size[bank] = bd->bi_dram[bank].size;
	}

	ret = fdt_fixup_memory_banks(blob, start, size, CONFIG_NR_DRAM_BANKS);
#if defined(CONFIG_ARMV7_NONSEC) || defined(CONFIG_ARMV7_VIRT)
	if (ret)
		return ret;

	ret = armv7_update_dt(blob);
#endif
	return ret;
}
