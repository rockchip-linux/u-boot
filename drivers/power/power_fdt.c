/*
 * Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Andy <yxj@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*#define DEBUG*/
#include <common.h>
#include <power/pmic.h>
#include <fdtdec.h>


int fdt_get_regulator_init_data(const void *blob, int node,
				struct fdt_regulator_match *match)
{
	match->name = fdt_getprop(blob, node, "regulator-name", NULL);
	match->min_uV = fdtdec_get_int(blob, node, "regulator-min-microvolt", 0);
	match->max_uV = fdtdec_get_int(blob, node, "regulator-max-microvolt", 0);
	match->init_uV = fdtdec_get_int(blob, node, "regulator-init-microvolt", 0);

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

		if (!prop)
			prop = fdt_get_name(blob, nd, NULL);

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
	int parent;
	u32 addr;
	u32 i2c_bus, i2c_addr, i2c_iobase;

#ifdef CONFIG_ROCKCHIP_ARCH64
	uint32_t *cell = NULL;
	int addrcells = 0;
	int nd;

	/* Note: i2c device address should be 32bit size */
	cell = (uint32_t *)fdt_getprop(blob, node, "reg", NULL);
	addr = (fdt_addr_t)fdt32_to_cpu(*cell);
	i2c_addr = (u32)addr;
	debug("i2c address = 0x%x\n", i2c_addr);

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}
	nd = fdt_parent_offset(blob, parent);
	if (nd < 0) {
		addrcells = 1;
	} else {
		addrcells = fdt_address_cells(blob, nd);
	}

	cell = (uint32_t *)fdt_getprop(blob, parent, "reg", NULL);
	if (addrcells == 2) {
		cell++;
	}
	addr = (u32)fdt32_to_cpu(*cell);
	i2c_iobase = (u32)addr;
	debug("i2c iobase = 0x%08x\n", i2c_iobase);

	i2c_bus = i2c_get_bus_num_fdt(i2c_iobase);
	debug("i2c bus = %d\n", i2c_bus);
#else
	addr = (u32)fdtdec_get_addr(blob, node, "reg");
	i2c_addr = addr;

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}
	i2c_iobase = fdtdec_get_addr(blob, parent, "reg");
	i2c_bus = i2c_get_bus_num_fdt(i2c_iobase);
#endif /* CONFIG_ROCKCHIP_ARCH64 */

	*pbus = i2c_bus;
	*paddr = i2c_addr;

	return parent;
}
