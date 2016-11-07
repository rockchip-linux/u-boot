/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/byteorder.h>
#include <asm/arch/rkplat.h>


DECLARE_GLOBAL_DATA_PTR;

#define COMPAT_ROCKCHIP_DRAM "rockchip,rk322x-dram"

struct trust_parameter {
	uint32_t version;
	uint32_t checksum;

	struct {
		char name[8];	/* tee.mem */
		uint64_t base;
		uint32_t size;
		uint32_t flags;
	} tee_mem;

	struct {
		char name[8];	/* drm.mem */
		uint64_t base;
		uint32_t size;
		uint32_t flags;
	} drm_mem;

	uint64_t reserved[8];
};

static uint16_t trust_checksum(const uint8_t *buf, uint16_t len)
{
	uint16_t i;
	uint16_t checksum = 0;

	for (i = 0; i < len; i++) {
		if (i % 2)
			checksum += buf[i] << 8;
		else
			checksum += buf[i];
	}
	checksum = ~checksum;

	return checksum;
}

static void trust_param_print(struct trust_parameter *param)
{
	debug("Trust parameter information:\n");
	debug("	version = 0x%08x\n", param->version);
	debug("	checksum = 0x%08x\n", param->checksum);
	debug("	tee: base = 0x%08llx, size = 0x%08x, flags = 0x%08x\n", \
		param->tee_mem.base, param->tee_mem.size, param->tee_mem.flags);
	debug("	drm: base = 0x%08llx, size = 0x%08x, flags = 0x%08x\n", \
		param->drm_mem.base, param->drm_mem.size, param->drm_mem.flags);
}

static int dram_bank_reserve(u64 base, u64 size)
{
	int i, bank = -1;

	for (i = 0; i < CONFIG_RK_MAX_DRAM_BANKS; i++) {
		if ((gd->bd->rk_dram[i].start <= base) && \
			(base < (gd->bd->rk_dram[i].start + gd->bd->rk_dram[i].size))) {
				bank = i;
				break;
		}
	}
	printf("dram reserve bank: base = 0x%08llx, size = 0x%08llx", base, size);
	if (bank == -1) {
		printf(" error.\n");
		return -1;
	} else {
		printf("\n");
	}

	if (gd->bd->rk_dram[bank].start == base) {
		gd->bd->rk_dram[bank].start = base + size;
		gd->bd->rk_dram[bank].size -= size;
	} else {
		for (i = CONFIG_RK_MAX_DRAM_BANKS - 1; i > bank; i--) {
			gd->bd->rk_dram[i].start = gd->bd->rk_dram[i - 1].start;
			gd->bd->rk_dram[i].size = gd->bd->rk_dram[i - 1].size;
		}

		gd->bd->rk_dram[bank + 1].start = base + size;
		gd->bd->rk_dram[bank + 1].size = gd->bd->rk_dram[bank].size - size - (base - gd->bd->rk_dram[bank].start);
		gd->bd->rk_dram[bank].size = base - gd->bd->rk_dram[bank].start;
	}

	return 0;
}


/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
	gd->ram_size = get_ram_size(
			(void *)CONFIG_SYS_TEXT_BASE,
			CONFIG_SYS_SDRAM_SIZE);

#if defined(CONFIG_RKDDR_PARAM_ADDR)
	u64 *buf = (u64 *)CONFIG_RKDDR_PARAM_ADDR;
	u32 count = ((u32 *)buf)[0];
	u64 start = 0, end = 0, size = 0;

	debug("\n");

	buf++;
	if (count >= CONFIG_RK_MAX_DRAM_BANKS) {
		end = PHYS_SDRAM;
	} else {
		int i;
		for (i = 0; i < count; i++) {
			start = le64_to_cpu(buf[i]);
			size = le64_to_cpu(buf[count + i]);
			if (start < CONFIG_MAX_MEM_ADDR) {
				end = start + size;
				if (end > CONFIG_MAX_MEM_ADDR) {
					end = CONFIG_MAX_MEM_ADDR;
					break;
				}
			}
		}
	}

	gd->arch.ddr_end = (unsigned long)end;
	debug("DDR End Address: 0x%08lx\n", gd->arch.ddr_end);
#endif
	return 0;
}


void dram_init_banksize(void)
{
#if defined(CONFIG_RKDDR_PARAM_ADDR)
	u64 *buf = (u64 *)CONFIG_RKDDR_PARAM_ADDR;
	u32 count = ((u32 *)buf)[0];
	int i;

	for (i = 0; i < CONFIG_RK_MAX_DRAM_BANKS; i++) {
		gd->bd->rk_dram[i].start = 0;
		gd->bd->rk_dram[i].size = 0;
	}

	if (count >= CONFIG_RK_MAX_DRAM_BANKS) {
		printf("Wrong bank count: %d(%d)\n", count, CONFIG_RK_MAX_DRAM_BANKS);
	} else {
		printf("Found dram banks: %d\n", count);

		buf++;
		for (i = 0; i < count; i++) {
			gd->bd->rk_dram[i].start = le64_to_cpu(buf[i]);
			gd->bd->rk_dram[i].size = le64_to_cpu(buf[count + i]);
			/* TODO: add check, if start|size not valide, goto failed. */
			/*
			if (check) {
				gd->bd->rk_dram[0].start = gd->bd->rk_dram[0].size = 0;
				goto failed;
			}*/

			/* reserve CONFIG_SYS_TEXT_BASE size for Runtime Firmware bin */
			if ((gd->bd->rk_dram[i].start == CONFIG_RAM_PHY_START) && (gd->bd->rk_dram[i].size != 0)) {
				gd->bd->rk_dram[i].start += (CONFIG_SYS_TEXT_BASE - CONFIG_RAM_PHY_START);
				gd->bd->rk_dram[i].size -= (CONFIG_SYS_TEXT_BASE - CONFIG_RAM_PHY_START);
			}
			printf("Adding bank:%016llx(%016llx)\n",
					gd->bd->rk_dram[i].start,
					gd->bd->rk_dram[i].size);
			gd->bd->rk_dram[i+1].start = 0;
			gd->bd->rk_dram[i+1].size = 0;
		}

		/* dram reserve bank for trust */
		struct trust_parameter *trust_param = (struct trust_parameter *)CONFIG_RKTRUST_PARAM_ADDR;
		uint32_t checksum = trust_checksum((uint8_t *)(unsigned long)trust_param + 8, sizeof(struct trust_parameter) - 8);

		if (checksum == trust_param->checksum) {
			printf("Reserve memory for trust os.\n");
			trust_param_print(trust_param);
			if (trust_param->tee_mem.flags != 0)
				dram_bank_reserve(trust_param->tee_mem.base, trust_param->tee_mem.size);
			if (trust_param->drm_mem.flags != 0)
				dram_bank_reserve(trust_param->drm_mem.base, trust_param->drm_mem.size);
		}
	}
#endif /* CONFIG_RKDDR_PARAM_ADDR */

	gd->bd->bi_dram[0].start = PHYS_SDRAM;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_SIZE;
}

#ifdef CONFIG_RK_DCF
struct ddr_timing {
	uint32_t freq;
	uint32_t dram_spd_bin;
	uint32_t sr_idle;
	uint32_t pd_idle;
	uint32_t dram_dll_disb_freq;
	uint32_t phy_dll_disb_freq;
	uint32_t dram_odt_disb_freq;
	uint32_t phy_odt_disb_freq;
	uint32_t ddr3_drv;
	uint32_t ddr3_odt;
	uint32_t lpddr3_drv;
	uint32_t lpddr3_odt;
	uint32_t lpddr2_drv;
	uint32_t phy_ddr3_clk_drv;
	uint32_t phy_ddr3_cmd_drv;
	uint32_t phy_ddr3_dqs_drv;
	uint32_t phy_ddr3_odt;
	uint32_t phy_lp23_clk_drv;
	uint32_t phy_lp23_cmd_drv;
	uint32_t phy_lp23_dqs_drv;
	uint32_t phy_lp3_odt;
};

struct ddr_timing __attribute__((aligned(64))) dram_timing;

static int rk_dram_parse_dt(const void *blob)
{
	int node, freq, phandle, timing_node;
	int ret;
	const char *cell;

	if(!blob)
		return -1;

	node = fdt_node_offset_by_compatible(blob,
					     0, COMPAT_ROCKCHIP_DRAM);
	if (node < 0) {
		debug("can't find dts node for dram\n");
		return -ENODEV;
	}

	cell = fdt_getprop(blob, node, "status", NULL);
	if ((cell != NULL) && (strcmp(cell, "okay") != 0)) {
		debug("status of dram node is disabled\n");
		return -ENODEV;
	}

	freq = fdtdec_get_int(blob, node, "dram_freq", -1);
	if (freq == -1) {
		debug("Cannot decode DRAM property dram_freq\n");
		return -ENODEV;
	}
	dram_timing.freq = freq;

	phandle = fdtdec_get_int(blob, node, "rockchip,dram_timing", -1);

	if (phandle == -1) {
		debug("Cannot decode DRAM property rockchip,dram_timing\n");
		return -ENODEV;
	}
	timing_node = fdt_node_offset_by_phandle(blob, phandle);
	ret = 0;
	dram_timing.dram_spd_bin = fdtdec_get_int(blob, timing_node,
						  "dram_spd_bin", -1);
	if (dram_timing.dram_spd_bin == -1)
		ret = -1;
	dram_timing.sr_idle = fdtdec_get_int(blob, timing_node, "sr_idle", -1);
	if (dram_timing.sr_idle == -1)
		ret = -1;
	dram_timing.pd_idle = fdtdec_get_int(blob, timing_node, "pd_idle", -1);
	if (dram_timing.pd_idle == -1)
		ret = -1;

	dram_timing.dram_dll_disb_freq = fdtdec_get_int(blob, timing_node,
							"dram_dll_disb_freq",
							-1);
	if (dram_timing.dram_dll_disb_freq == -1)
		ret = -1;

	dram_timing.phy_dll_disb_freq = fdtdec_get_int(blob, timing_node,
						       "phy_dll_disb_freq",
						       -1);
	if (dram_timing.phy_dll_disb_freq == -1)
		ret = -1;

	dram_timing.dram_odt_disb_freq = fdtdec_get_int(blob, timing_node,
							"dram_odt_disb_freq",
							-1);
	if (dram_timing.dram_odt_disb_freq == -1)
		ret = -1;

	dram_timing.phy_odt_disb_freq = fdtdec_get_int(blob, timing_node,
						       "phy_odt_disb_freq",
						       -1);
	if (dram_timing.phy_odt_disb_freq == -1)
		ret = -1;

	dram_timing.ddr3_drv = fdtdec_get_int(blob, timing_node, "ddr3_drv",
					      -1);
	if (dram_timing.ddr3_drv == -1)
		ret = -1;

	dram_timing.ddr3_odt = fdtdec_get_int(blob, timing_node, "ddr3_odt",
					      -1);
	if (dram_timing.ddr3_odt == -1)
		ret = -1;

	dram_timing.lpddr3_drv = fdtdec_get_int(blob, timing_node,
						"lpddr3_drv", -1);
	if (dram_timing.lpddr3_drv == -1)
		ret = -1;

	dram_timing.lpddr3_odt = fdtdec_get_int(blob, timing_node,
						"lpddr3_odt", -1);
	if (dram_timing.lpddr3_odt == -1)
		ret = -1;

	dram_timing.lpddr2_drv = fdtdec_get_int(blob, timing_node,
						"lpddr2_drv", -1);
	if (dram_timing.lpddr2_drv == -1)
		ret = -1;

	dram_timing.phy_ddr3_clk_drv = fdtdec_get_int(blob, timing_node,
						 "phy_ddr3_clk_drv", -1);
	if (dram_timing.phy_ddr3_clk_drv == -1)
		ret = -1;

	dram_timing.phy_ddr3_cmd_drv = fdtdec_get_int(blob, timing_node,
						 "phy_ddr3_cmd_drv", -1);
	if (dram_timing.phy_ddr3_cmd_drv == -1)
		ret = -1;

	dram_timing.phy_ddr3_dqs_drv = fdtdec_get_int(blob, timing_node,
						 "phy_ddr3_dqs_drv", -1);
	if (dram_timing.phy_ddr3_dqs_drv == -1)
		ret = -1;

	dram_timing.phy_ddr3_odt = fdtdec_get_int(blob, timing_node,
						  "phy_ddr3_odt", -1);
	if (dram_timing.phy_ddr3_odt == -1)
		ret = -1;

	dram_timing.phy_lp23_clk_drv = fdtdec_get_int(blob, timing_node,
						 "phy_lp23_clk_drv", -1);
	if (dram_timing.phy_lp23_clk_drv == -1)
		ret = -1;

	dram_timing.phy_lp23_cmd_drv = fdtdec_get_int(blob, timing_node,
						 "phy_lp23_cmd_drv", -1);
	if (dram_timing.phy_lp23_cmd_drv == -1)
		ret = -1;

	dram_timing.phy_lp23_dqs_drv = fdtdec_get_int(blob, timing_node,
						 "phy_lp23_dqs_drv", -1);
	if (dram_timing.phy_lp23_dqs_drv == -1)
		ret = -1;

	dram_timing.phy_lp3_odt = fdtdec_get_int(blob, timing_node,
						 "phy_lp3_odt", -1);
	if (dram_timing.phy_lp3_odt == -1)
		ret = -1;

	if (ret)
		return ENODEV;
	else
		return 0;
}

void dram_freq_init(void)
{
#ifdef CONFIG_RK_PSCI
	uint32_t ret;

	if (0 == rk_dram_parse_dt(gd->fdt_blob)) {
		flush_cache((uint32)&dram_timing.freq, sizeof(dram_timing));
		printf("DRAM: dram freq:%dMHz\n", dram_timing.freq / 1000000);
		ret = invoke_psci_fn(PSCI_SIP_DDR_FREQ,
				     dram_timing.freq / 1000000,
				     (uint32)&dram_timing.dram_spd_bin, 0);
		if (ret)
			printf("DRAM: dram freq init error\n");
	} else {
		debug("DRAM: can't found dram parameter for dts\n");
	}
#endif /* CONFIG_RK_PSCI */
}
#endif /* CONFIG_RK_DCF */
