/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <bidram.h>
#include <boot_rkimg.h>
#include <debug_uart.h>
#include <fdtdec.h>
#include <fdt.h>
#include <fdt_support.h>
#include <memblk.h>
#include <ramdisk.h>
#include <linux/libfdt.h>
#include <asm/io.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/mos.h>
#include <asm/arch/rk_atags.h>
#include <asm/arch/rockchip_smccc.h>
#include <asm/arch/vendor.h>
#include <asm/arch/spl_resource_img.h>

DECLARE_GLOBAL_DATA_PTR;

__weak void mos_board_reset(void) {}

#define FDT_COMAPT_OS0		"mos,domain-os0"
#define FDT_COMAPT_OS1		"mos,domain-os1"
#define FDT_COMPAT_SCP		"mos,domain-scp"
#define FDT_COMPAT_SAFETY	"mos,domain-safety"

#define SBD_OS1_RUN		0xdeadbeaf
#define SBD_FW_ADDR		(CONFIG_MOS_BOOTDEV_SHARED_ADDR + CONFIG_MOS_BOOTDEV_SHARED_ARGS_SIZE)
#define SBD_ARG_MAGIC		0x53415247  /* "SARG" */
#define SBD_ARG_ADDR		(CONFIG_MOS_BOOTDEV_SHARED_ADDR)

#define SBD_DBG_USE_SYNC	0

struct sbd_args {
	u32 magic;
	ulong fw_addr;
	u32 os_lock;
};

static ulong mos_safety_atags_base = 0;

static int mos_spl_syscfg_load(void *resc_hdr)
{
#if defined(CONFIG_SPL_BUILD) && \
	defined(CONFIG_SPL_ROCKCHIP_VENDOR_PARTITION) && \
	defined(CONFIG_SPL_ROCKCHIP_RESOURCE_IMAGE)
	const struct resource_img_hdr *hdr = resc_hdr;
	char name[MAX_FILE_NAME_LEN];
	void *addr;
	u32 file_size;
	int fdt_size;
	int len;

	if (spl_resource_image_check_header(hdr)) {
		printf("Invalid syscfg resource\n");
		return -EINVAL;
	}

	/* read syscfg id for syscfg file name */
	len = vendor_storage_read(SYSCFG_ID, name, sizeof(name) - 1);
	if (len <= 0) {
		printf("Can't get SYSCFG_ID in vendor storage, len=%d\n", len);
		return len;
	}

	printf("syscfg: %s\n", name);

	/* get syscfg file ! */
	name[len] = '\0';
	addr = spl_read_resource_file(hdr, name, &file_size);
	if (!addr) {
		printf("Can't read %s from syscfg resource\n", name);
		return -ENOENT;
	}

	if (fdt_check_header(addr)) {
		printf("Invalid syscfg from resource\n");
		return -EINVAL;
	}

	fdt_size = fdt_totalsize(addr);
	if (fdt_size <= 0 || fdt_size > file_size) {
		printf("Invalid syscfg size: 0x%x\n", fdt_size);
		return -E2BIG;
	}

	memmove((void *)CONFIG_MOS_SYSCFG_ADDR, addr, fdt_size);
#endif

	return 0;
}

static void *mos_syscfg(void)
{
	void *cfg_fdt = (void *)CONFIG_MOS_SYSCFG_ADDR;

	if (!fdt_check_header(cfg_fdt))
		return cfg_fdt;

	mos_spl_syscfg_load(cfg_fdt);
	if (fdt_check_header(cfg_fdt)) {
		printf("## mos: no available syscfg\n");
		return NULL;
	}

	return cfg_fdt;
}

static int overlay_apply_one_node(void *fdt, int target, void *fdto, int node)
{
	int property;
	int subnode;

	fdt_for_each_property_offset(property, fdto, node) {
		const char *name;
		const void *prop;
		int prop_len;
		int ret;

		prop = fdt_getprop_by_offset(fdto, property, &name, &prop_len);
		if (prop_len == -FDT_ERR_NOTFOUND)
			return -FDT_ERR_INTERNAL;
		if (prop_len < 0)
			return prop_len;

		debug("## adding prop: %s\n", name);
		ret = fdt_setprop(fdt, target, name, prop, prop_len);
		if (ret)
			return ret;
	}

	fdt_for_each_subnode(subnode, fdto, node) {
		const char *name = fdt_get_name(fdto, subnode, NULL);
		int nnode;
		int ret;

		debug("## adding subnode: %s\n", name);
		nnode = fdt_add_subnode(fdt, target, name);
		if (nnode == -FDT_ERR_EXISTS) {
			nnode = fdt_subnode_offset(fdt, target, name);
			if (nnode == -FDT_ERR_NOTFOUND)
				return -FDT_ERR_INTERNAL;
		}
		if (nnode < 0)
			return nnode;

		ret = overlay_apply_one_node(fdt, nnode, fdto, subnode);
		if (ret)
			return ret;
	}

	return 0;
}

static int mos_fdt_overlay_gic(void *fdt, void *cfg_fdt, int target_noffset, int os_id)
{
	int amp_noffset;
	int gic_noffset;
	int ret;

	if (fdt_check_header(fdt) || fdt_check_header(cfg_fdt))
		return 0;

	ret = fdt_increase_size(fdt, fdt_totalsize((void *)cfg_fdt));
	if (ret)
		return ret;

	gic_noffset = fdt_path_offset(fdt, "/rockchip-amp/mos-gic-irqs");
	if (gic_noffset > 0)
		fdt_del_node(fdt, gic_noffset);

	amp_noffset = fdt_path_offset(fdt, "/rockchip-amp");
	if (amp_noffset < 0)
		return amp_noffset;

	gic_noffset = fdt_add_subnode(fdt, amp_noffset, "mos-gic-irqs");
	if (gic_noffset < 0)
		return gic_noffset;

	ret = fdt_setprop_u32(fdt, gic_noffset, "cur-os-id", os_id);
	if (ret)
		return ret;

	printf("mos: fdt overlay\n");

	return overlay_apply_one_node(fdt, gic_noffset, cfg_fdt, target_noffset);
}

static int mos_atags_setup_mem(void *cfg_fdt)
{
	struct tag_ddr_mem mem;
	fdt_size_t addr;
	fdt_addr_t size;
	u64 start, end;
	int i, len, count;
	int n = 0;
	int noffset;

#ifdef CONFIG_MOS_SECONDARY
	noffset = fdt_node_offset_by_compatible(cfg_fdt, 0, FDT_COMAPT_OS1);
#else
	noffset = fdt_node_offset_by_compatible(cfg_fdt, 0, FDT_COMAPT_OS0);
#endif
	if (noffset < 0)
		return noffset;

	printf("\n## mos syscfg at 0x%08lx with compat \"%s\"\n",
	       (ulong)cfg_fdt, (char *)fdt_getprop(cfg_fdt, 0, "compatible", NULL));

	if (!fdt_getprop(cfg_fdt, noffset, "memory", &len))
		panic("No available 'memory' property in os domain\n");

	count = len / (sizeof(u32) * 4);
	memset(&mem, 0, sizeof(mem));
	mem.version = 0;

	for (i = 0; i < count; i++) {
		addr = fdtdec_get_addr_size_fixed(cfg_fdt, noffset,
					"memory", i, 2, 2, &size, false);
		if (addr < 0)
			return addr;

		start = addr;
		end = addr + size;
		printf("## mos memory: 0x%08lx - 0x%08lx (size: 0x%08lx)\n",
		       (ulong)start, (ulong)end, (ulong)size);

		mem.bank[n] = start;
		mem.bank[n + count] = end - start;
		n++;
	}

	mem.count = count;
	atags_set_tag(ATAG_DDR_MEM, &mem);

	return 0;
}

static int mos_atags_setup_console(void *cfg_fdt)
{
	const struct fdt_property *prop;
	struct fdtdec_phandle_args args;
	struct tag_console console;
	int domain_noffset, domains, node;
	int acs_num, acs_idx;
	int ret, owner;
	int len, i = 0;

	domains = fdt_path_offset(cfg_fdt, "/domains");
	if (domains < 0)
		return domains;

	memset(&console, 0, sizeof(console));
	fdt_for_each_subnode(domain_noffset, cfg_fdt, domains) {
		if (!fdt_getprop(cfg_fdt, domain_noffset, "access", &len))
			continue;

		acs_num = len / 4;
		for (acs_idx = 0; acs_idx < acs_num; acs_idx++) {
			ret = fdtdec_parse_phandle_with_args(cfg_fdt,
					domain_noffset, "access", NULL, 0,
					acs_idx, &args);
			if (ret)
				return ret;

			debug("##   - access%d: %s\n",
			      acs_idx, fdt_get_name(cfg_fdt, args.node, NULL));

			node = args.node;
			prop = fdt_get_property(cfg_fdt, node, "device_type", &len);
			if (!prop || strcmp(prop->data, "console"))
				continue;
			if (!fdtdec_get_is_enabled(cfg_fdt, node))
				continue;
			owner = fdtdec_get_uint(cfg_fdt, node, "owner", 0);
			if (!owner)
				continue;

			console.hw[i].owner = owner;
			console.hw[i].uart_id =
				fdtdec_get_uint(cfg_fdt, node, "uart_id", -1);
			console.hw[i].uart_m_mode =
				fdtdec_get_uint(cfg_fdt, node, "uart_m_mode", -1);
			console.hw[i].uart_base =
				fdtdec_get_uint(cfg_fdt, node, "uart_base", -1);
			console.hw[i].uart_baudrate =
				fdtdec_get_uint(cfg_fdt, node, "uart_baudrate", -1);
			debug("%s:\n", fdt_get_name(cfg_fdt, node, NULL));
			debug("    owner         = %d\n", console.hw[i].owner);
			debug("    uart_id       = %d\n", console.hw[i].uart_id);
			debug("    uart_m_mode   = %d\n", console.hw[i].uart_m_mode);
			debug("    uart_base     = 0x%08x\n", console.hw[i].uart_base);
			debug("    uart_baudrate = %d\n", console.hw[i].uart_baudrate);

			i++;
			if (i >= MAX_CONSOLE)
				goto finish;
		}
	}

finish:
	atags_set_tag(ATAG_CONSOLE, &console);

	return 0;
}

#ifdef CONFIG_MOS_SECONDARY
static int mos_atags_setup_serial(void *cfg_fdt)
{
	const struct fdt_property *prop;
	struct fdtdec_phandle_args args;
	struct tag_serial t_serial;
	int os_noffset;
	int acs_num, acs_idx;
	int node, owner;
	int ret, len;

	os_noffset = fdt_node_offset_by_compatible(cfg_fdt, 0, FDT_COMAPT_OS1);
	if (os_noffset < 0)
		return os_noffset;

	if (!fdt_getprop(cfg_fdt, os_noffset, "access", &len))
		return 0;

	acs_num = len / 4;
	for (acs_idx = 0; acs_idx < acs_num; acs_idx++) {
		ret = fdtdec_parse_phandle_with_args(cfg_fdt,
				os_noffset, "access", NULL, 0,
				acs_idx, &args);
		if (ret)
			return ret;

		node = args.node;
		prop = fdt_get_property(cfg_fdt, node, "device_type", &len);
		if (!prop || strcmp(prop->data, "console"))
			continue;

		owner = fdtdec_get_uint(cfg_fdt, node, "owner", 0);
		if (owner == OWNER_LOADER1) {
			memset(&t_serial, 0, sizeof(t_serial));
			t_serial.version = 0;
			t_serial.enable =
				fdtdec_get_is_enabled(cfg_fdt, node);
			t_serial.id =
				fdtdec_get_uint(cfg_fdt, node, "uart_id", -1);
			t_serial.m_mode =
				fdtdec_get_uint(cfg_fdt, node, "uart_m_mode", -1);
			t_serial.addr =
				fdtdec_get_uint(cfg_fdt, node, "uart_base", -1);
			t_serial.baudrate =
				fdtdec_get_uint(cfg_fdt, node, "uart_baudrate", -1);
			atags_set_tag(ATAG_SERIAL, &t_serial);

			/* Must do debug_uart_init() ! */
			gd->serial.addr = t_serial.addr;
			debug_uart_init();
			break;
		}
	}

	return 0;
}

static int mos_atags_init_cntfrq(void)
{
#if defined(CONFIG_ROCKCHIP_PRELOADER_ATAGS)
	struct tag *t;

	t = atags_get_tag(ATAG_BOOT1_PARAM);
	if (t && t->u.boot1p.param[2] == 0x100)
		gd->arch.timer_rate_hz = 1000000000;
	else
		gd->arch.timer_rate_hz = 24000000;

/* FIXUP ME: For secondary spl can get right sys timer tick when reboot */
#ifdef CONFIG_ROCKCHIP_RK3576
	if (!t)
		gd->arch.timer_rate_hz = 1000000000;
#endif
#ifdef CONFIG_ARM64
	asm volatile("msr CNTFRQ_EL0, %0" : : "r" (gd->arch.timer_rate_hz));
#else
	asm volatile("mcr p15, 0, %0, c14, c0, 0" : : "r"(gd->arch.timer_rate_hz));
#endif
#endif

	return 0;
}

#else
static int fdt_pack_reg(const void *fdt, void *buf, u64 *address, u64 *size, int n)
{
	int address_cells = 2;
	int size_cells = 2;
	int i;
	char *p = buf;

	for (i = 0; i < n; i++) {
		*(fdt64_t *)p = cpu_to_fdt64(address[i]);
		p += 4 * address_cells;

		*(fdt64_t *)p = cpu_to_fdt64(size[i]);
		p += 4 * size_cells;
	}

	return p - (char *)buf;
}

static int mos_syscfg_fixup_mem(void *cfg_fdt)
{
	const char *domains_compat[] = {
		FDT_COMAPT_OS0, FDT_COMAPT_OS1, FDT_COMPAT_SCP, FDT_COMPAT_SAFETY,
	};
	struct tag *t;
	u64 new_start[CONFIG_NR_DRAM_BANKS];
	u64 new_size[CONFIG_NR_DRAM_BANKS];
	u8 tmp[CONFIG_NR_DRAM_BANKS * 16];
	fdt_size_t addr;
	fdt_addr_t size;
	u64 dram_end = -1;
	u64 start, end;
	int i, len, count;
	int n, bank, err;
	int noffset;
	int fixup;

	t = atags_get_tag(ATAG_DDR_MEM);
	if (t) {
		count = t->u.ddr_mem.count;
		for (i = 0; i < count; i++)
			dram_end = t->u.ddr_mem.bank[i] + t->u.ddr_mem.bank[i + count];
	}

	for (n = 0; n < ARRAY_SIZE(domains_compat); n++) {
		noffset = fdt_node_offset_by_compatible(cfg_fdt, 0, domains_compat[n]);
		if (noffset < 0)
			return noffset;

		if (!fdt_getprop(cfg_fdt, noffset, "memory", &len)) {
			debug("WARN: %s: No available 'memory' prop\n", domains_compat[n]);
			continue;
		}

		bank = 0;
		fixup = 0;
		count = len / (sizeof(u32) * 4);
		for (i = 0; i < count; i++) {
			addr = fdtdec_get_addr_size_fixed(cfg_fdt, noffset,
					"memory", i, 2, 2, &size, false);
			if (addr < 0)
				return addr;

			start = addr;
			end = addr + size;

			debug("%s: org bank%d: 0x%08lx - 0x%08lx\n",
			      domains_compat[n], bank, (ulong)start, (ulong)end);

			if (start >= dram_end) {
				debug("    skip: 0x%08lx - 0x%08lx\n",
				      (ulong)start, (ulong)end);
				end = dram_end;
				end = 0;
				fixup = 1;
			} else if (end > dram_end) {
				debug("    end:  0x%08lx => 0x%08lx\n",
				      (ulong)end, (ulong)dram_end);
				end = dram_end;
				fixup = 1;
			}

			if (end) {
				debug("%s: new bank%d: 0x%08lx - 0x%08lx\n",
				      domains_compat[n], bank, (ulong)start, (ulong)end);
				new_start[bank] = start;
				new_size[bank] = end - start;
				bank++;
			}
			if (!fixup)
				continue;

			len = fdt_pack_reg(cfg_fdt, tmp, new_start, new_size, bank);
			err = fdt_setprop(cfg_fdt, noffset, "memory", tmp, len);
			if (err < 0) {
				printf("WARN: could not set memory %s.\n",
				       fdt_strerror(err));
				return err;
			}
			/* next compat */
			break;
		}
	}

	return 0;
}
#endif

int mos_fdt_overlay(void *fdt)
{
	void *cfg_fdt;
	struct fdtdec_phandle_args args1;
	struct fdtdec_phandle_args args2;
	int acs_idx, inc_idx;
	int acs_num, inc_num;
	int os_noffset, os_id;
	int noffset = -1;
	int len, ret;

	cfg_fdt = mos_syscfg();
	if (!cfg_fdt)
		return -EINVAL;

#ifdef CONFIG_MOS_SECONDARY
	os_id = 1;
	os_noffset = fdt_node_offset_by_compatible(cfg_fdt, 0, FDT_COMAPT_OS1);
#else
	os_id = 0;
	os_noffset = fdt_node_offset_by_compatible(cfg_fdt, 0, FDT_COMAPT_OS0);
#endif
	if (os_noffset < 0)
		return os_noffset;

	if (!fdt_getprop(cfg_fdt, os_noffset, "include", &len))
		return 0;

	inc_num = len / 4;
	for (inc_idx = 0; inc_idx < inc_num; inc_idx++) {
		ret = fdtdec_parse_phandle_with_args(cfg_fdt,
					os_noffset, "include", NULL, 0,
					inc_idx, &args1);
		if (ret)
			return ret;

		debug("## inc%d: %s\n",
		      inc_idx, fdt_get_name(cfg_fdt, args1.node, NULL));

		if (!fdt_getprop(cfg_fdt, args1.node, "access", &len))
			continue;

		acs_num = len / 4;
		for (acs_idx = 0; acs_idx < acs_num; acs_idx++) {
			ret = fdtdec_parse_phandle_with_args(cfg_fdt,
					args1.node, "access", NULL, 0,
					acs_idx, &args2);
			if (ret)
				return ret;

			debug("##   - access%d: %s\n",
			      acs_idx, fdt_get_name(cfg_fdt, args2.node, NULL));

			if (!fdt_node_check_compatible(cfg_fdt, args2.node, "mos,gic-irqs")) {
				noffset = args2.node;
				goto found;
			}
		}
	}

found:
	if (noffset < 0)
		return noffset;

	ret = mos_fdt_overlay_gic(fdt, cfg_fdt, noffset, os_id);
	if (ret)
		printf("mos fdt overlay gic failed: %d\n", ret);

	return ret;
}

int mos_spl_init(void)
{
#ifdef CONFIG_MOS_SECONDARY
	void *cfg_fdt;

	cfg_fdt = mos_syscfg();
	if (!cfg_fdt)
		return -EINVAL;

	mos_atags_setup_serial(cfg_fdt);
	mos_atags_init_cntfrq();
#else
	/* for secondary SPL */
	memcpy((void *)SECONDARY_ATAGS_BASE, (void *)ATAGS_PHYS_BASE, ATAGS_SIZE);
	flush_dcache_range(SECONDARY_ATAGS_BASE, SECONDARY_ATAGS_BASE + ATAGS_SIZE);
#endif

	return 0;
}

int mos_spl_late_init(void)
{
	void *cfg_fdt;

	cfg_fdt = mos_syscfg();
	if (!cfg_fdt)
		return -EINVAL;

#ifndef CONFIG_MOS_SECONDARY
	mos_syscfg_fixup_mem(cfg_fdt);
#endif
	mos_atags_setup_mem(cfg_fdt);

	return 0;
}

int mos_spl_cfg_init(void)
{
	void *cfg_fdt;
	int domains, noffset;
	fdt_size_t sizep;

	cfg_fdt = mos_syscfg();
	if (!cfg_fdt)
		return -EINVAL;

	domains = fdt_path_offset(cfg_fdt, "/domains");
	if (domains < 0)
		return domains;

	noffset = fdt_node_offset_by_compatible(cfg_fdt, domains, FDT_COMPAT_SAFETY);
	if (noffset < 0)
		return noffset;

	mos_safety_atags_base = fdtdec_get_addr_size_fixed(cfg_fdt, noffset,
					"memory", 1, 2, 2, &sizep, false);
	if (mos_safety_atags_base == FDT_ADDR_T_NONE)
		return 0;

	/* prepare console/atags for safety */
	mos_atags_setup_console(cfg_fdt);
	memcpy((void *)mos_safety_atags_base, (void *)ATAGS_PHYS_BASE, ATAGS_SIZE);
	flush_dcache_range(mos_safety_atags_base, mos_safety_atags_base + ATAGS_SIZE);

	/* prepare vendor storage for safety */
#ifdef CONFIG_SPL_ROCKCHIP_VENDOR_PARTITION
	ulong vendor_addr;

	vendor_addr = fdtdec_get_addr_size_fixed(cfg_fdt, noffset,
				"vendor_storage_memory", 0, 2, 2, &sizep, false);
	if (vendor_addr == FDT_ADDR_T_NONE || !sizep)
		return 0;

	vendor_storage_fixup((void *)vendor_addr);
#endif
	return 0;
}

ulong mos_safety_atags_addr(void)
{
	return mos_safety_atags_base;
}

#if !defined(CONFIG_MOS_SECONDARY) && defined(CONFIG_MOS_BOOTDEV_SHARED)
static int mos_secondary_fw(ulong *base, ulong *size)
{
	void *cfg_fdt;
	int domains, noffset;
	fdt_size_t sizep;
	fdt_addr_t start;

	cfg_fdt = mos_syscfg();
	if (!cfg_fdt)
		return -EINVAL;

	domains = fdt_path_offset(cfg_fdt, "/domains");
	if (domains < 0)
		return domains;

	noffset = fdt_node_offset_by_compatible(cfg_fdt, domains, FDT_COMAPT_OS1);
	if (noffset < 0)
		return noffset;

	start = fdtdec_get_addr_size_fixed(cfg_fdt, noffset, "memory",
					   0, 2, 2, &sizep, false);
	if (start == FDT_ADDR_T_NONE)
		return -EINVAL;

	*base = start;
	*size = sizep;

	return 0;
}

static int mos_sbd_load_secondary(bool is_stage1)
{
	const char *full_name[] = {
		"security", "uboot", "dtbo", "vbmeta",
		"baseparameter", "boot", "recovery",
	};
	const char *s1_name[] = {
		"security", "uboot", "misc",
	};
	ulong addr = SBD_FW_ADDR;
	struct ramdisk_info *ri = (void *)addr;
	struct blk_desc *desc;
	disk_partition_t last_full_part, last_s1_part; /* s1: stage1 */
	disk_partition_t part;
	lbaint_t last_full_lba = 0;
	lbaint_t last_s1_lba = 0;
	u32 blks, blkcnt, bgpt_blks;
	ulong base = 0;
	ulong size = 0;
	ulong fw_addr;
	int i, num = 1;
	u32 op_flag;

	desc = blk_get_devnum_by_type(IF_TYPE_RVD, 0);
	if (!desc) {
		printf("## mos: no rvd 0 blk device\n");
		return -ENODEV;
	}

	while (part_get_info(desc, num++, &part) == 0) {
		/* s1 parts */
		for (i = 0; i < ARRAY_SIZE(s1_name); i++) {
			if (!strncmp((char *)part.name,
				s1_name[i], strlen(s1_name[i]))) {
				if (part.start + part.size > last_s1_lba) {
					last_s1_lba = part.start + part.size;
					last_s1_part = part;
				}
				break;
			}
		}
		/* full parts */
		for (i = 0; i < ARRAY_SIZE(full_name); i++) {
			if (!strncmp((char *)part.name,
				full_name[i], strlen(full_name[i]))) {
				if (part.start + part.size > last_full_lba) {
					last_full_lba = part.start + part.size;
					last_full_part = part;
				}
				break;
			}
		}
	}

	if (!last_s1_lba || !last_full_lba) {
		printf("## mos: no available secondary images !\n");
		return -EIO;
	}

	if (is_stage1) {
		fw_addr = addr + RAMDISK_INFO_SIZE;

		mos_secondary_fw(&base, &size);
		printf("## mos: syscfg secondary memory 0x%08lx - 0x%08lx\n",
		       base, base + size);
		printf("## mos: full secondary images at 0x%08lx - 0x%08lx (final: %s)\n",
		       addr, addr + RAMDISK_INFO_SIZE + (last_full_lba * desc->blksz) + SZ_32K,
		       last_full_part.name);
		printf("## mos: s1 secondary images at 0x%08lx - 0x%08lx (final: %s)... ",
		       fw_addr, fw_addr + (last_s1_lba * desc->blksz), last_s1_part.name);

		/* Use last_s1_lba ! */
		blkcnt = blk_dread(desc, 0, last_s1_lba, (void *)fw_addr);
		if (blkcnt != last_s1_lba) {
			printf("Failed with only %d blks read\n", blkcnt);
			return -EIO;
		}

		/*
		 *		Append backup GPT.
		 *
		 * - WARNING: 32KB align is required for backup gpt in part_efi.c.
		 * - Use last_full_lba.
		 */
		fw_addr += last_full_lba * desc->blksz;
		bgpt_blks = SZ_32K / desc->blksz;
		blkcnt = blk_dread(desc, desc->lba - bgpt_blks, bgpt_blks, (void *)fw_addr);
		if (blkcnt != bgpt_blks) {
			printf("Failed with %d blks read\n", blkcnt);
			return -EIO;
		}

		/* Private data */
		memset(ri, 0, sizeof(struct ramdisk_info));
		ri->magic = RAMDISK_INFO_MAGIC;
		ri->base = (ulong)ri + RAMDISK_INFO_SIZE;
		ri->lba = desc->lba;
		ri->bgpt_lba = last_full_lba + (bgpt_blks - 33);
		ri->second_fw_addr = ri->base + (last_s1_lba * desc->blksz);
	} else {
		blks = last_full_lba - last_s1_lba;
		fw_addr = ri->second_fw_addr;
		printf("## mos: s2 secondary images at 0x%08lx - 0x%08lx (final: %s)... ",
		       fw_addr, fw_addr + (blks * desc->blksz), last_full_part.name);

		op_flag = desc->op_flag;
#if SBD_DBG_USE_SYNC
		desc->op_flag = 0;
#else
		desc->op_flag = BLK_PRE_RW;
#endif
		blkcnt = blk_dread(desc, last_s1_lba, blks, (void *)fw_addr);
		desc->op_flag = op_flag;
		if (blkcnt != blks) {
			printf("Failed with %d blks read\n", blkcnt);
			return -EIO;
		}
	}

	flush_dcache_all();
	printf("OK\n");

	return 0;
}
#endif

void mos_secondary_wfe(void)
{
#if defined(CONFIG_MOS_SECONDARY) && defined(CONFIG_MOS_BOOTDEV_SHARED)
	struct ramdisk_info *ri = (void *)SBD_FW_ADDR;
	struct sbd_args *sarg = (void *)SBD_ARG_ADDR;
	struct blk_desc *desc;
	disk_partition_t part;
	ulong start, size;
	ulong us;
	char args[64];

	if (sarg->os_lock == SBD_OS1_RUN)
		goto finish;

	/* wait firmware ready */
	printf("## mos: WFE... ");
	us = timer_get_us();
	for (;;) {
		invalidate_dcache_range((ulong)&sarg->os_lock,
			(ulong)&sarg->os_lock + sizeof(sarg->os_lock));
		if (sarg->os_lock == SBD_OS1_RUN)
			break;
		dsb();
		isb();
		__asm("wfe");
	}
	printf("%ld us\n", timer_get_us() - us);

finish:
	/* pass "misc" partition 'start,size' info */
	desc = rockchip_get_bootdev();
	if (!desc)
		return;

	if (part_get_info_by_name(desc, PART_MISC, &part) < 0)
		return;

	start = ri->base + part.start * desc->blksz;
	size = part.size * part.blksz;
	snprintf(args, 64, "sbd.partition-misc=0x%08lx,0x%08lx", start, size);
	env_update("bootargs", args);

	/* pass "security" partition 'start,size' info */
	if (part_get_info_by_name(desc, PART_SECURITY, &part) < 0)
		return;

	start = ri->base + part.start * desc->blksz;
	size = part.size * part.blksz;
	snprintf(args, 64, "sbd.partition-security=0x%08lx,0x%08lx", start, size);
	env_update("bootargs", args);
#endif
}

int mos_secondary_boot(void)
{
	int ret = 0;
#if !defined(CONFIG_MOS_SECONDARY) && defined(CONFIG_MOS_BOOTDEV_SHARED)
	struct sbd_args *sarg = (void *)SBD_ARG_ADDR;
	char args[32];

	ret = mos_sbd_load_secondary(true);
	if (!ret) {
		sarg->magic = SBD_ARG_MAGIC;
		sarg->fw_addr = SBD_FW_ADDR;
		sarg->os_lock = 0;
		flush_dcache_range((ulong)&sarg->os_lock,
			(ulong)&sarg->os_lock + sizeof(sarg->os_lock));
		snprintf(args, 32, "sbd.os1-lockaddr=0x%08lx", (ulong)&sarg->os_lock);
		env_update("bootargs", args);
	}
#endif
	if (!ret) {
		printf("## mos: booting secondary os!\n");
		sip_smc_mos_cfg(MOS_CFG_BOOT, 0, 0);
		mdelay(10);
	}

	return ret;
}

int mos_secondary_late_boot(void)
{
	int ret = 0;

#if !defined(CONFIG_MOS_SECONDARY) && defined(CONFIG_MOS_BOOTDEV_SHARED)
	ret = mos_sbd_load_secondary(false);
#if SBD_DBG_USE_SYNC
	struct sbd_args *sarg = (void *)SBD_ARG_ADDR;

	sarg->os_lock = SBD_OS1_RUN;
	flush_dcache_range((ulong)&sarg->os_lock,
		(ulong)&sarg->os_lock + sizeof(sarg->os_lock));
	dsb();
	isb();
	__asm("sev");
#endif
#endif
	return ret;
}

#ifdef CONFIG_MOS_ONE_IMAGE
static int mos_vendor_buffer(ulong *vendor_addr, u32 *vendor_size)
{
	void *cfg_fdt;
	int domains, noffset;
	fdt_size_t sizep;
	fdt_addr_t addr;

	cfg_fdt = mos_syscfg();
	if (!cfg_fdt)
		return -EINVAL;

	domains = fdt_path_offset(cfg_fdt, "/domains");
	if (domains < 0)
		return domains;

	noffset = fdt_node_offset_by_compatible(cfg_fdt, domains, FDT_COMPAT_SAFETY);
	if (noffset < 0)
		return noffset;

	addr = fdtdec_get_addr_size_fixed(cfg_fdt, noffset,
					  "vendor_storage_memory",
					  0, 2, 2, &sizep, false);
	if (addr == FDT_ADDR_T_NONE || !sizep)
		return -EINVAL;

	*vendor_addr = addr;
	*vendor_size = sizep;

	return 0;
}

#define OS_DTB_SIZE	64
const char *mos_vendor_dtb_name(void)
{
	u16 os_id = IS_ENABLED(CONFIG_MOS_SECONDARY) ? OS1_DTB_ID : OS0_DTB_ID;
	static char os_data[OS_DTB_SIZE]; /* static ! */
	ulong vendor_addr;
	u32 vendor_size;
	int size;
	int ret;

	ret = mos_vendor_buffer(&vendor_addr, &vendor_size);
	if (ret)
		return NULL;

	size = vendor_storage_buffer_read(os_id, os_data, OS_DTB_SIZE - 1,
					  (void *)vendor_addr, vendor_size);
	if (size <= 0)
		return NULL;

	os_data[size] = '\0';

	return os_data;
}
#endif

void mos_system_reset(void)
{
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_MOS_SECONDARY)
	mos_board_reset();
	printf("## mos: system reset failed !\n");
#endif
}

#ifndef CONFIG_MOS_SECONDARY
/* Secondary: Reserve SBD ramdisk in bidram_fixup(). */
int board_bidram_reserve(struct bidram *bidram)
{
	/* all lowlevel firmwares */
	bidram_reserve(MEM_MOS, MOS_LOWLEVEL_FW_BASE, MOS_LOWLEVEL_FW_SIZE);

	/* SBD ramdisk */
#ifdef CONFIG_MOS_BOOTDEV_SHARED
	ulong base, size;
	int ret;

	ret = mos_secondary_fw(&base, &size);
	if (ret) {
		base = MOS_LOWLEVEL_FW_BASE + MOS_LOWLEVEL_FW_SIZE;
		size = SZ_256M;
	}
	bidram_reserve(MEM_SHM, base, size);
#endif
	return 0;
}
#endif

#if !defined(CONFIG_SPL_BUILD) && \
    defined(CONFIG_MOS_SUPPORT) && !defined(CONFIG_MOS_SECONDARY)
int mos_set_boot_stage(u32 stage)
{
	return sip_smc_mos_cfg(MOS_CFG_BOOT_STAGE, stage, 0);
}
#endif

