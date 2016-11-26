/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "pcie.h"

struct pcie_bus *root_bus = NULL;
static int pcie_configure_link(struct pcie_bus *bus);

int pcie_scan_bus(void)
{
	int err;
	struct pcie_bus *pbus = NULL;

#if defined(CONFIG_RKCHIP_RK3399)
	err = rockchip_pcie_cdns_probe();
#else
	printf("PCIe: haven't assgin scan bus operation\n");
	err = -EINVAL;
#endif
	if (err)
		return err;

	pbus = pcie_claim_bus();
	if (!pbus)
		return -ENOMEM;

	err = pcie_configure_link(pbus);
	if (err)
		printf("PCIe: fail to configure link\n");

	pcie_release_bus(pbus);
	return err;
}

struct pcie_bus *pcie_alloc_bus(void *priv, struct pcie_ops *ops)
{
	struct pcie_bus *pbus = NULL;

	pbus = (struct pcie_bus *)malloc(sizeof(struct pcie_bus));
	if (!pbus)
		return NULL;

	memset(pbus, 0, sizeof(*pbus));

	pbus->priv = priv;
	pbus->ops = ops;
	pbus->claimed = 0;
	root_bus = pbus;

	return pbus;
}

struct pcie_bus *pcie_claim_bus(void)
{
	if (!root_bus) {
		printf("PCIe: haven't alloc bus\n");
		return NULL;
	}

	if (root_bus->claimed) {
		printf("PCIe: unbalanced claim for root_bus\n");
		return NULL;
	}

	root_bus->claimed++;
	return root_bus;
}

void pcie_release_bus(struct pcie_bus *pbus)
{
	if (--root_bus->claimed < 0)
		printf("PCIe: unbalanced release for root_bus\n");
}

static int pcie_configure_link(struct pcie_bus *pbus)
{
	u32 value, timeout, i;
	u32 pointer, next_pointer;
	u32 table_size, msix_table_addr = 0x0;
	bool is_msi = false, is_msix = false;

	if (!pbus || !pbus->ops || !pbus->ops->rd_rc_conf ||
	    !pbus->ops->rd_ep_conf ||
	    !pbus->ops->wr_rc_conf || !pbus->ops->wr_ep_conf) {
		printf("PCIe: pcie_configure_link: missing ops...\n");
		return -EINVAL;
	}

	pbus->ops->rd_ep_conf(pbus->priv, PCI_CLASS_REVISION, 4, &value);
	if ((value & (0xffff << 16)) !=
	    (PCI_CLASS_MSC | PCI_SUBCLASS_NVME)) {
		printf("PCIe: device's classe code & revision ID = 0x%x\n",
		       value);
		printf("PCIe: We only support NVMe\n");
		return -EINVAL;
	}

	pbus->ops->rd_ep_conf(pbus->priv, PCI_VENDOR_ID, 2, &value);
	pbus->pdev.vendor = (u16)value;
	pbus->ops->rd_ep_conf(pbus->priv, PCI_DEVICE_ID, 2, &value);
	pbus->pdev.device = (u16)value;

	pbus->ops->wr_rc_conf(pbus->priv, PCI_PRIMARY_BUS, 4, 0x0);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_BRIDGE_CONTROL, 2, 0x0);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_COMMAND + 0x2, 2, 0xffff);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_PRIMARY_BUS, 4, 0xff0100);

	/* only support 64bit non-prefetchable 16k mem region: BAR0 + BAR1
	 * clear BAR1 for upper 32bit, no need to wr all 1s to see the size
	 */
	pbus->ops->wr_ep_conf(pbus->priv, PCI_BASE_ADDRESS_1, 4, 0x0);

	/* clear CCC and enable retrain link */
	pbus->ops->rd_rc_conf(pbus->priv, PCI_LNKCTL, 2, &value);
	value &= ~PCI_EXP_LNKCTL_CCC;
	value |= PCI_EXP_LNKCTL_RL;
	pbus->ops->wr_rc_conf(pbus->priv, PCI_LNKCTL, 2, value);

	/* wait for clear of LTS */
	timeout = 2000;
	while (timeout--) {
		pbus->ops->rd_rc_conf(pbus->priv, PCI_LNKCTL + 0x2, 2, &value);
		if (!(value & BIT(11)))
			break;
		mdelay(1);
	}

	if (!timeout) {
		printf("PCIe: fail to clear LTS\n");
		return -ETIMEDOUT;
	}

	/* write SBN */
	pbus->ops->wr_rc_conf(pbus->priv, PCI_SUBORDINATE_BUS, 1, 0x1);
	/* clear some enable bits for error */
	pbus->ops->wr_rc_conf(pbus->priv, PCI_BRIDGE_CONTROL, 2, 0x0);
	/* write EP's command register, disable EP */
	pbus->ops->wr_ep_conf(pbus->priv, PCI_COMMAND, 2, 0x0);

	for (i = 0; i < pbus->region_count; i++) {
		if (pbus->regions[i].flags == PCI_REGION_MEM) {
			/* configre BAR0 */
			pbus->ops->wr_ep_conf(pbus->priv, PCI_BASE_ADDRESS_0, 4,
					      pbus->regions[i].bus_start);
			/* configre BAR1 */
			pbus->ops->wr_ep_conf(pbus->priv, PCI_BASE_ADDRESS_1,
					      4, 0x0);
			break;
		}
	}

	/* write EP's command register */
	pbus->ops->wr_ep_conf(pbus->priv, PCI_COMMAND, 2, 0x0);

	/* write RC's IO base and limit including upper */
	pbus->ops->wr_rc_conf(pbus->priv, PCI_IO_BASE_UPPER16, 4, 0xffff);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_IO_BASE, 2, 0xf0);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_IO_BASE_UPPER16, 4, 0x0);
	/* write RC's Mem base and limit including upper */
	value = (pbus->regions[i].bus_start) >> 16;
	pbus->ops->wr_rc_conf(pbus->priv, PCI_MEMORY_BASE, 4,
			      value | (value << 16));
	pbus->ops->wr_rc_conf(pbus->priv, PCI_PREF_LIMIT_UPPER32, 4, 0x0);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_PREF_MEMORY_BASE, 4, 0xfff0);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_PREF_BASE_UPPER32, 4, 0x0);
	pbus->ops->wr_rc_conf(pbus->priv, PCI_PREF_LIMIT_UPPER32, 4, 0x0);

	/* clear some enable bits for error */
	pbus->ops->wr_rc_conf(pbus->priv, PCI_BRIDGE_CONTROL, 2, 0x0);

	/* read RC root control and cap, clear some int enable */
	pbus->ops->wr_rc_conf(pbus->priv, PCI_RC_CTRL_CAP, 2, 0x0);

	/* clear RC's error status, correctable and uncorectable error */
	pbus->ops->wr_rc_conf(pbus->priv, 0x130, 4, 0x0);
	pbus->ops->wr_rc_conf(pbus->priv, 0x110, 4, 0x0);
	pbus->ops->wr_rc_conf(pbus->priv, 0x104, 4, 0x0);

	value = 0;
	pbus->ops->rd_ep_conf(pbus->priv, 0x34, 1, &pointer);
	printf("PCIe: cap pointer = 0x%x\n", pointer);

	for (;;) {
		pbus->ops->rd_ep_conf(pbus->priv, pointer, 2, &next_pointer);
		if ((next_pointer & 0xff) == PCI_CAP_ID_MSI) {
			is_msi = true;
			break;
		} else if ((next_pointer & 0xff) == PCI_CAP_ID_MSIX) {
			is_msix = true;
			break;
		}

		pointer = (next_pointer >> 8);
		if (pointer == 0)
			break;
	}

	if (is_msi) {
		printf("PCIe: msi cap pointer = 0x%x\n", pointer);
		pbus->ops->rd_ep_conf(pbus->priv, pointer + 2, 2, &value);
		value |= 0x1;
		pbus->ops->wr_ep_conf(pbus->priv, pointer + 2, 2, value);
		pbus->ops->wr_ep_conf(pbus->priv, pointer + 4, 4,
				      pbus->msi_base);
		pbus->ops->wr_ep_conf(pbus->priv, pointer + 8, 4, 0x0);
	} else if (is_msix) {
		printf("PCIe: msi-x cap pointer = 0x%x\n", pointer);
		pbus->ops->rd_ep_conf(pbus->priv, pointer + 2, 2, &value);
		printf("PCIe: msi-x table size = %d\n", value & 0x7ff);
		table_size = value & 0x7ff;
		pbus->ops->rd_ep_conf(pbus->priv, pointer + 8, 2, &value);
		printf("PCIe: msi-x BIR = 0x%x\n", value & 0x7);
		printf("PCIe: msi-x table offset = 0x%x\n", value & 0xfffffff8);

		for (i = 0; i < pbus->region_count; i++) {
			if (pbus->regions[i].flags == PCI_REGION_MEM)
				msix_table_addr = pbus->regions[i].bus_start +
							(value & 0xfffffff8);
		}

		if (msix_table_addr == 0)
			return -EINVAL;

		printf("PCIe: msi-x table begin addr = 0x%x\n",
		       msix_table_addr);
		for (i = 0; i < table_size; i++) {
			writel(pbus->msi_base,	msix_table_addr + i * 0x0);
			writel(0x0,		msix_table_addr + i * 0x4);
			writel(i,		msix_table_addr + i * 0x8);
			writel(0x0,		msix_table_addr + i * 0xc);
		}
		pbus->ops->wr_ep_conf(pbus->priv, pointer + 2, 2, 0x20);
		pbus->ops->wr_ep_conf(pbus->priv, pointer + 2, 2, 0xc020);
		pbus->ops->wr_ep_conf(pbus->priv, pointer + 2, 2, 0x8020);
	} else {
		printf("PCIe: no msi and msi-x\n");
	}

	pbus->ops->rd_ep_conf(pbus->priv, PCI_COMMAND, 2, &value);
	value |= PCI_COMMAND_INTX_DISABLE;
	pbus->ops->wr_ep_conf(pbus->priv, PCI_COMMAND, 2, value);

	return 0;
}

int pcie_decode_regions(struct pcie_bus *pbus, const void *blob, int node)
{
	int pci_addr_cells, addr_cells, size_cells;
	int cells_per_record;
	const u32 *prop;
	int len;
	int i;
	int pos = 0;

	prop = fdt_getprop(blob, node, "ranges", &len);
	if (!prop)
		return -EINVAL;
	pci_addr_cells = fdt_address_cells(blob, node);
	addr_cells = fdt_address_cells(blob, 0);
	size_cells = fdt_size_cells(blob, node);

	/* PCI addresses are always 3-cells */
	len /= sizeof(u32);
	cells_per_record = pci_addr_cells + addr_cells + size_cells;
	pbus->region_count = 0;
	for (i = 0; i < MAX_PCI_REGIONS; i++, len -= cells_per_record) {
		u64 pci_addr, addr, size;
		int space_code;
		u32 flags;
		int type;

		if (len < cells_per_record)
			break;
		flags = fdt32_to_cpu(prop[0]);
		space_code = (flags >> 24) & 3;
		pci_addr = fdtdec_get_number(prop + 1, 2);
		prop += pci_addr_cells;
		addr = fdtdec_get_number(prop, addr_cells);
		prop += addr_cells;
		size = fdtdec_get_number(prop, size_cells);
		prop += size_cells;
		printf("%s: region %d, pci_addr=0x%llx , addr=0x%llx , size=0x%llx, ",
		       __func__, pbus->region_count, pci_addr, addr, size);
		if (space_code & 2) {
			type = flags & (1U << 30) ? PCI_REGION_PREFETCH :
					PCI_REGION_MEM;
			printf("type=%s\n", (type == PCI_REGION_PREFETCH) ?
			       "PCI_REGION_PREFETCH" : "PCI_REGION_MEM");
		} else if (space_code & 1) {
			type = PCI_REGION_IO;
			printf("type=PCI_REGION_IO\n");
		} else {
			printf("type=unknown\n");
			continue;
		}

		pbus->regions[pos].flags = type;
		pbus->regions[pos].phys_start = pci_addr;
		pbus->regions[pos].bus_start = addr;
		pbus->regions[pos].size = size;
		pos++;
		pbus->region_count++;
	}

	return 0;
}
