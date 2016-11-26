/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PCIE_H
#define __PCIE_H

#include "../config.h"
#include <pci.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

#define BIT(x)				(1 << (x))
/* Capability lists */
#define PCI_CAP_LIST_ID		0	/* Capability ID */
#define  PCI_CAP_ID_PM		0x01	/* Power Management */
#define  PCI_CAP_ID_AGP		0x02	/* Accelerated Graphics Port */
#define  PCI_CAP_ID_VPD		0x03	/* Vital Product Data */
#define  PCI_CAP_ID_SLOTID	0x04	/* Slot Identification */
#define  PCI_CAP_ID_MSI		0x05	/* Message Signalled Interrupts */
#define  PCI_CAP_ID_CHSWP	0x06	/* CompactPCI HotSwap */
#define  PCI_CAP_ID_PCIX	0x07	/* PCI-X */
#define  PCI_CAP_ID_HT		0x08	/* HyperTransport */
#define  PCI_CAP_ID_VNDR	0x09	/* Vendor-Specific */
#define  PCI_CAP_ID_DBG		0x0A	/* Debug port */
#define  PCI_CAP_ID_CCRC	0x0B	/* CompactPCI Central Resource Control */
#define  PCI_CAP_ID_SHPC	0x0C	/* PCI Standard Hot-Plug Controller */
#define  PCI_CAP_ID_SSVID	0x0D	/* Bridge subsystem vendor/device ID */
#define  PCI_CAP_ID_AGP3	0x0E	/* AGP Target PCI-PCI bridge */
#define  PCI_CAP_ID_SECDEV	0x0F	/* Secure Device */
#define  PCI_CAP_ID_EXP		0x10	/* PCI Express */
#define  PCI_CAP_ID_MSIX	0x11	/* MSI-X */
#define  PCI_CAP_ID_SATA	0x12	/* SATA Data/Index Conf. */
#define  PCI_CAP_ID_AF		0x13	/* PCI Advanced Features */
#define  PCI_CAP_ID_EA		0x14	/* PCI Enhanced Allocation */
#define  PCI_CAP_ID_MAX		PCI_CAP_ID_EA
#define PCI_CAP_LIST_NEXT	1	/* Next capability in the list */
#define PCI_CAP_FLAGS		2	/* Capability defined flags (16 bits) */
#define PCI_CAP_SIZEOF		4

#define PCI_PRIMARY_BUS         0x18    /* Primary bus number */
#define PCI_SUBORDINATE_BUS     0x1a    /* Highest bus number behind the bridge */
#define PCI_BRIDGE_CONTROL      0x3e
#define PCI_VENDOR_ID           0x00    /* 16 bits */
#define PCI_DEVICE_ID           0x02    /* 16 bits */
#define PCI_COMMAND             0x04    /* 16 bits */
#define  PCI_COMMAND_IO         0x1     /* Enable response in I/O space */
#define  PCI_COMMAND_MEMORY     0x2     /* Enable response in Memory space */
#define  PCI_COMMAND_MASTER     0x4     /* Enable bus mastering */
#define  PCI_COMMAND_SPECIAL    0x8     /* Enable response to special cycles */
#define  PCI_COMMAND_INVALIDATE 0x10    /* Use memory write and invalidate */
#define  PCI_COMMAND_VGA_PALETTE 0x20   /* Enable palette snooping */
#define  PCI_COMMAND_PARITY     0x40    /* Enable parity checking */
#define  PCI_COMMAND_WAIT       0x80    /* Enable address/data stepping */
#define  PCI_COMMAND_SERR       0x100   /* Enable SERR */
#define  PCI_COMMAND_FAST_BACK  0x200   /* Enable back-to-back writes */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx Emulation Disable */
#define PCI_CLASS_REVISION      0x08    /* High 24 bits are class, low 8 revision */
#define  PCI_CLASS_MSC		(0x01 << 24)
#define   PCI_SUBCLASS_NVME	(0x08 << 16)
#define PCI_BASE_ADDRESS_0      0x10    /* 32 bits */
#define PCI_BASE_ADDRESS_1      0x14    /* 32 bits [htype 0,1 only] */
#define PCI_IO_BASE             0x1c    /* I/O range behind the bridge */
#define PCI_LNKCTL		0xd0
#define  PCI_EXP_LNKCTL_RL      0x0020  /* Retrain Link */
#define  PCI_EXP_LNKCTL_CCC     0x0040  /* Common Clock Configuration */
#define PCI_RC_CTRL_CAP		0xdc	/* Root control and cap */
#define PCI_IO_BASE_UPPER16     0x30    /* Upper half of I/O addresses */
#define PCI_MEMORY_BASE         0x20    /* Memory range behind */
#define PCI_PREF_MEMORY_BASE    0x24    /* Prefetchable memory range behind */
#define PCI_PREF_BASE_UPPER32   0x28    /* Upper half of prefetchable memory range */
#define PCI_PREF_LIMIT_UPPER32  0x2c

#define MAX_PCI_REGIONS         7

/* vendor ID */
#define PCI_VENDOR_ID_INTEL	0x8086

struct pcie_ops {
	/* These callbacks only support bdf = 01:00:00 */
	int (*rd_rc_conf)(void *priv, int where, int size, u32 *value);
	int (*wr_rc_conf)(void *priv, int where, int size, u32 value);
	int (*rd_ep_conf)(void *priv, int where, int size, u32 *value);
	int (*wr_ep_conf)(void *priv, int where, int size, u32 value);
};

struct pcie_dev {
	u16 vendor; /* vendor ID */
	u16 device; /* device ID */
};

struct pcie_bus {
	void *priv; /* Host controller's priv data*/
	int claimed;
	struct pcie_ops *ops;
	struct pci_region regions[MAX_PCI_REGIONS];
	struct pcie_dev pdev;
	u32 region_count;
	u32 msi_base;
};

/* Global visible */
extern struct pcie_bus *root_bus;

extern struct pcie_bus *pcie_alloc_bus(void *priv, struct pcie_ops *ops);
extern struct pcie_bus *pcie_claim_bus(void);
extern void pcie_release_bus(struct pcie_bus *pbus);
extern int pcie_scan_bus(void);
extern int pcie_decode_regions(struct pcie_bus *pbus,
			       const void *blob, int node);

/* From individual host */
extern int rockchip_pcie_cdns_probe(void);
#endif /* __PCIE_H */
