/*
 * Copyright (C) 2014 RockChip inc
 * Andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*#define DEBUG*/
#include <common.h>
#include <power/pmic.h>
#include <fdtdec.h>


static int fdt_get_regulator_init_data(const void *blob, int node, struct fdt_regulator_match *match)
{
	match->name = fdt_getprop(blob, node, "regulator-name", NULL);
	match->min_uV = fdtdec_get_int(blob, node, "regulator-min-microvolt", 0);
	match->max_uV = fdtdec_get_int(blob, node, "regulator-max-microvolt", 0);
	
	if (fdt_get_property(blob, node, "regulator-boot-on", NULL))
			match->boot_on = 1;
	else
			match->boot_on = 0;
	debug("%s--%s\n"
		"min_uV:%d\n"
		"max_uV:%d\n"
		"boot_on:%d\n\n",
		match->prop, match->name, match->min_uV,
		match->max_uV, match->boot_on);

	return 0;
}


int fdt_regulator_match(const void *blob, int node,
		struct fdt_regulator_match *matches, int num_matches)
{
	int nd, i;
	const char *prop;
	for (nd = fdt_first_subnode(blob, node); nd >= 0;
		 		nd = fdt_next_subnode(blob, nd)) { 
		prop = fdt_getprop(blob,nd,"regulator-compatible", NULL);
		for (i = 0; i < num_matches; i++) {
			struct fdt_regulator_match *match = &matches[i];
			if (!strcmp(match->prop, prop))
				fdt_get_regulator_init_data(blob, nd, match);
		}
			
	 }
	return 0;
}


int fdt_get_regulator_node(const void * blob, int node)
{
	return fdt_subnode_offset_namelen(blob, node, "regulators", 10);
}


int fdt_get_i2c_info(const void* blob, int node, u32 *pbus, u32 *paddr)
{
	int parent, nd;
	u32 addr;
	u32 i2c_bus, i2c_addr, i2c_iobase;

	addr = (u32)fdtdec_get_addr(blob, node, "reg");
	i2c_addr = addr;

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}
	i2c_iobase = fdtdec_get_addr(blob, parent, "reg");
	i2c_bus = i2c_get_bus_num_fdt(i2c_iobase);

	*pbus = i2c_bus;
	*paddr = i2c_addr;

	return 0;
}
