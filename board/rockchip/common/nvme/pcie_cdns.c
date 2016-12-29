/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "../config.h"
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>
#include "pcie.h"

DECLARE_GLOBAL_DATA_PTR;

static int dbg_enable;
static bool fixed_resource;
#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			printf(args); \
		} \
	} while (0)

#define DBG_ERR(args...) printf(args)

/*
 * The upper 16 bits of PCIE_CLIENT_CONFIG are a write mask for the lower 16
 * bits.  This allows atomic updates of the register without locking.
 */
#define HIWORD_UPDATE(mask, val)	(((mask) << 16) | (val))
#define HIWORD_UPDATE_BIT(val)		HIWORD_UPDATE(val, val)

#define ENCODE_LANES(x)			((((x) >> 1) & 3) << 4)

#define PCIE_CLIENT_BASE		0x0
#define PCIE_CLIENT_CONFIG		(PCIE_CLIENT_BASE + 0x00)
#define   PCIE_CLIENT_CONF_ENABLE	  HIWORD_UPDATE_BIT(0x0001)
#define   PCIE_CLIENT_LINK_TRAIN_ENABLE	  HIWORD_UPDATE_BIT(0x0002)
#define   PCIE_CLIENT_ARI_ENABLE	  HIWORD_UPDATE_BIT(0x0008)
#define   PCIE_CLIENT_CONF_LANE_NUM(x)	  HIWORD_UPDATE(0x0030, ENCODE_LANES(x))
#define   PCIE_CLIENT_MODE_RC		  HIWORD_UPDATE_BIT(0x0040)
#define   PCIE_CLIENT_GEN_SEL_1		  HIWORD_UPDATE(0x0080, 0)
#define   PCIE_CLIENT_GEN_SEL_2		  HIWORD_UPDATE_BIT(0x0080)
#define PCIE_CLIENT_BASIC_STATUS1	(PCIE_CLIENT_BASE + 0x48)
#define   PCIE_CLIENT_LINK_STATUS_UP		0x00300000
#define   PCIE_CLIENT_LINK_STATUS_MASK		0x00300000
#define PCIE_CLIENT_INT_MASK		(PCIE_CLIENT_BASE + 0x4c)
#define PCIE_CLIENT_INT_STATUS		(PCIE_CLIENT_BASE + 0x50)
#define   PCIE_CLIENT_INTR_MASK			0x1e0
#define   PCIE_CLIENT_INTR_SHIFT		5
#define   PCIE_CLIENT_INT_LEGACY_DONE		BIT(15)
#define   PCIE_CLIENT_INT_MSG			BIT(14)
#define   PCIE_CLIENT_INT_HOT_RST		BIT(13)
#define   PCIE_CLIENT_INT_DPA			BIT(12)
#define   PCIE_CLIENT_INT_FATAL_ERR		BIT(11)
#define   PCIE_CLIENT_INT_NFATAL_ERR		BIT(10)
#define   PCIE_CLIENT_INT_CORR_ERR		BIT(9)
#define   PCIE_CLIENT_INT_INTD			BIT(8)
#define   PCIE_CLIENT_INT_INTC			BIT(7)
#define   PCIE_CLIENT_INT_INTB			BIT(6)
#define   PCIE_CLIENT_INT_INTA			BIT(5)
#define   PCIE_CLIENT_INT_LOCAL			BIT(4)
#define   PCIE_CLIENT_INT_UDMA			BIT(3)
#define   PCIE_CLIENT_INT_PHY			BIT(2)
#define   PCIE_CLIENT_INT_HOT_PLUG		BIT(1)
#define   PCIE_CLIENT_INT_PWR_STCG		BIT(0)

#define PCIE_CLIENT_INT_LEGACY \
	(PCIE_CLIENT_INT_INTA | PCIE_CLIENT_INT_INTB | \
	PCIE_CLIENT_INT_INTC | PCIE_CLIENT_INT_INTD)

#define PCIE_CLIENT_INT_CLI \
	(PCIE_CLIENT_INT_CORR_ERR | PCIE_CLIENT_INT_NFATAL_ERR | \
	PCIE_CLIENT_INT_FATAL_ERR | PCIE_CLIENT_INT_DPA | \
	PCIE_CLIENT_INT_HOT_RST | PCIE_CLIENT_INT_MSG | \
	PCIE_CLIENT_INT_LEGACY_DONE | PCIE_CLIENT_INT_LEGACY | \
	PCIE_CLIENT_INT_PHY)

#define PCIE_CORE_CTRL_MGMT_BASE	0x900000
#define PCIE_CORE_CTRL			(PCIE_CORE_CTRL_MGMT_BASE + 0x000)
#define   PCIE_CORE_PL_CONF_SPEED_5G		0x00000008
#define   PCIE_CORE_PL_CONF_SPEED_MASK		0x00000018
#define   PCIE_CORE_PL_CONF_LANE_MASK		0x00000006
#define   PCIE_CORE_PL_CONF_LANE_SHIFT		1
#define PCIE_CORE_CTRL_PLC1		(PCIE_CORE_CTRL_MGMT_BASE + 0x004)
#define   PCIE_CORE_CTRL_PLC1_FTS_MASK		0xffff00
#define   PCIE_CORE_CTRL_PLC1_FTS_SHIFT		8
#define   PCIE_CORE_CTRL_PLC1_FTS_CNT		0xffff
#define PCIE_CORE_TXCREDIT_CFG1		(PCIE_CORE_CTRL_MGMT_BASE + 0x020)
#define   PCIE_CORE_TXCREDIT_CFG1_MUI_MASK	0xFFFF0000
#define   PCIE_CORE_TXCREDIT_CFG1_MUI_SHIFT	16
#define   PCIE_CORE_TXCREDIT_CFG1_MUI_ENCODE(x) \
		(((x) >> 3) << PCIE_CORE_TXCREDIT_CFG1_MUI_SHIFT)
#define PCIE_CORE_INT_STATUS		(PCIE_CORE_CTRL_MGMT_BASE + 0x20c)
#define   PCIE_CORE_INT_PRFPE			BIT(0)
#define   PCIE_CORE_INT_CRFPE			BIT(1)
#define   PCIE_CORE_INT_RRPE			BIT(2)
#define   PCIE_CORE_INT_PRFO			BIT(3)
#define   PCIE_CORE_INT_CRFO			BIT(4)
#define   PCIE_CORE_INT_RT			BIT(5)
#define   PCIE_CORE_INT_RTR			BIT(6)
#define   PCIE_CORE_INT_PE			BIT(7)
#define   PCIE_CORE_INT_MTR			BIT(8)
#define   PCIE_CORE_INT_UCR			BIT(9)
#define   PCIE_CORE_INT_FCE			BIT(10)
#define   PCIE_CORE_INT_CT			BIT(11)
#define   PCIE_CORE_INT_UTC			BIT(18)
#define   PCIE_CORE_INT_MMVC			BIT(19)
#define PCIE_CORE_INT_MASK		(PCIE_CORE_CTRL_MGMT_BASE + 0x210)
#define PCIE_RC_BAR_CONF		(PCIE_CORE_CTRL_MGMT_BASE + 0x300)

#define PCIE_CORE_INT \
		(PCIE_CORE_INT_PRFPE | PCIE_CORE_INT_CRFPE | \
		 PCIE_CORE_INT_RRPE | PCIE_CORE_INT_CRFO | \
		 PCIE_CORE_INT_RT | PCIE_CORE_INT_RTR | \
		 PCIE_CORE_INT_PE | PCIE_CORE_INT_MTR | \
		 PCIE_CORE_INT_UCR | PCIE_CORE_INT_FCE | \
		 PCIE_CORE_INT_CT | PCIE_CORE_INT_UTC | \
		 PCIE_CORE_INT_MMVC)

#define PCIE_RC_CONFIG_BASE		0xa00000
#define PCIE_RC_CONFIG_VENDOR		(PCIE_RC_CONFIG_BASE + 0x00)
#define PCIE_RC_CONFIG_RID_CCR		(PCIE_RC_CONFIG_BASE + 0x08)
#define   PCIE_RC_CONFIG_SCC_SHIFT		16
#define PCIE_RC_CONFIG_DCR		(PCIE_RC_CONFIG_BASE + 0xc4)
#define   PCIE_RC_CONFIG_DCR_CSPL_SHIFT		18
#define   PCIE_RC_CONFIG_DCR_CSPL_LIMIT		0xff
#define   PCIE_RC_CONFIG_DCR_CPLS_SHIFT		26
#define PCIE_RC_CONFIG_LINK_CAP		(PCIE_RC_CONFIG_BASE + 0xcc)
#define   PCIE_RC_CONFIG_LINK_CAP_L0S		BIT(10)
#define PCIE_RC_CONFIG_LCS		(PCIE_RC_CONFIG_BASE + 0xd0)
#define PCIE_RC_CONFIG_L1_SUBSTATE_CTRL2 (PCIE_RC_CONFIG_BASE + 0x90c)
#define PCIE_RC_CONFIG_THP_CAP		(PCIE_RC_CONFIG_BASE + 0x274)
#define   PCIE_RC_CONFIG_THP_CAP_NEXT_MASK	0xfff00000

#define PCIE_CORE_AXI_CONF_BASE		0xc00000
#define PCIE_CORE_OB_REGION_ADDR0	(PCIE_CORE_AXI_CONF_BASE + 0x0)
#define   PCIE_CORE_OB_REGION_ADDR0_NUM_BITS	0x3f
#define   PCIE_CORE_OB_REGION_ADDR0_LO_ADDR	0xffffff00
#define PCIE_CORE_OB_REGION_ADDR1	(PCIE_CORE_AXI_CONF_BASE + 0x4)
#define PCIE_CORE_OB_REGION_DESC0	(PCIE_CORE_AXI_CONF_BASE + 0x8)
#define PCIE_CORE_OB_REGION_DESC1	(PCIE_CORE_AXI_CONF_BASE + 0xc)

#define PCIE_CORE_AXI_INBOUND_BASE	0xc00800
#define PCIE_RP_IB_ADDR0		(PCIE_CORE_AXI_INBOUND_BASE + 0x0)
#define   PCIE_CORE_IB_REGION_ADDR0_NUM_BITS	0x3f
#define   PCIE_CORE_IB_REGION_ADDR0_LO_ADDR	0xffffff00
#define PCIE_RP_IB_ADDR1		(PCIE_CORE_AXI_INBOUND_BASE + 0x4)

/* Size of one AXI Region (not Region 0) */
#define AXI_REGION_SIZE				BIT(20)
/* Size of Region 0, equal to sum of sizes of other regions */
#define AXI_REGION_0_SIZE			(32 * (0x1 << 20))
#define OB_REG_SIZE_SHIFT			5
#define IB_ROOT_PORT_REG_SIZE_SHIFT		3
#define AXI_WRAPPER_IO_WRITE			0x6
#define AXI_WRAPPER_MEM_WRITE			0x2

#define MAX_AXI_IB_ROOTPORT_REGION_NUM		3
#define MIN_AXI_ADDR_BITS_PASSED		8
#define ROCKCHIP_VENDOR_ID			0x1d87
#define PCIE_ECAM_BUS(x)			(((x) & 0xff) << 20)
#define PCIE_ECAM_DEV(x)			(((x) & 0x1f) << 15)
#define PCIE_ECAM_FUNC(x)			(((x) & 0x7) << 12)
#define PCIE_ECAM_REG(x)			(((x) & 0xfff) << 0)
#define PCIE_ECAM_ADDR(bus, dev, func, reg) \
	  (PCIE_ECAM_BUS(bus) | PCIE_ECAM_DEV(dev) | \
	   PCIE_ECAM_FUNC(func) | PCIE_ECAM_REG(reg))

#define RC_REGION_0_ADDR_TRANS_H		0x00000000
#define RC_REGION_0_ADDR_TRANS_L		0x00000000
#define RC_REGION_0_PASS_BITS			(25 - 1)
#define MAX_AXI_WRAPPER_REGION_NUM		33

#define PCI_CLASS_BRIDGE_PCI		0x0604

#define HIWORD_UPDATE_PHY(val, mask, shift) \
		((val) << (shift) | (mask) << ((shift) + 16))

#define PHY_MAX_LANE_NUM      4
#define PHY_CFG_DATA_SHIFT    7
#define PHY_CFG_ADDR_SHIFT    1
#define PHY_CFG_DATA_MASK     0xf
#define PHY_CFG_ADDR_MASK     0x3f
#define PHY_CFG_RD_MASK       0x3ff
#define PHY_CFG_WR_ENABLE     1
#define PHY_CFG_WR_DISABLE    1
#define PHY_CFG_WR_SHIFT      0
#define PHY_CFG_WR_MASK       1
#define PHY_CFG_PLL_LOCK      0x10
#define PHY_CFG_CLK_TEST      0x10
#define PHY_CFG_CLK_SCC       0x12
#define PHY_CFG_SEPE_RATE     BIT(3)
#define PHY_CFG_PLL_100M      BIT(3)
#define PHY_PLL_LOCKED        BIT(9)
#define PHY_PLL_OUTPUT        BIT(10)
#define PHY_LANE_A_STATUS     0x30
#define PHY_LANE_B_STATUS     0x31
#define PHY_LANE_C_STATUS     0x32
#define PHY_LANE_D_STATUS     0x33
#define PHY_LANE_RX_DET_SHIFT 11
#define PHY_LANE_RX_DET_TH    0x1
#define PHY_LANE_IDLE_OFF     0x1
#define PHY_LANE_IDLE_MASK    0x1
#define PHY_LANE_IDLE_A_SHIFT 3
#define PHY_LANE_IDLE_B_SHIFT 4
#define PHY_LANE_IDLE_C_SHIFT 5
#define PHY_LANE_IDLE_D_SHIFT 6

struct rockchip_pcie_phy {
	u32 reg_base;
	u32 pcie_conf;
	u32 pcie_status;
	u32 rst_addr;
};

struct rockchip_pcie {
	struct rockchip_pcie_phy *phy;
	u32 rst_addr;
	u32 axi_base;
	u32 axi_size;
	u32 apb_base;
	struct fdt_gpio_state gpio;
	struct pcie_bus *bus;
};

enum soc_id {
	RK3399 = 1,
};

static inline void phy_wr_cfg(struct rockchip_pcie_phy *rk_phy,
			      u32 addr, u32 data)
{
	writel(HIWORD_UPDATE_PHY(data, PHY_CFG_DATA_MASK,
				 PHY_CFG_DATA_SHIFT) |
	       HIWORD_UPDATE_PHY(addr, PHY_CFG_ADDR_MASK,
				 PHY_CFG_ADDR_SHIFT),
	       rk_phy->reg_base + rk_phy->pcie_conf);

	udelay(3);
	writel(HIWORD_UPDATE_PHY(PHY_CFG_WR_ENABLE,
				 PHY_CFG_WR_MASK,
				 PHY_CFG_WR_SHIFT),
	       rk_phy->reg_base + rk_phy->pcie_conf);

	udelay(3);
	writel(HIWORD_UPDATE_PHY(PHY_CFG_WR_DISABLE,
				 PHY_CFG_WR_MASK,
				 PHY_CFG_WR_SHIFT),
	       rk_phy->reg_base + rk_phy->pcie_conf);
}

static inline u32 phy_rd_cfg(struct rockchip_pcie_phy *rk_phy,
			     u32 addr)
{
	writel(HIWORD_UPDATE_PHY(addr,
				 PHY_CFG_RD_MASK,
				 PHY_CFG_ADDR_SHIFT),
	       rk_phy->reg_base + rk_phy->pcie_conf);

	return readl(rk_phy->reg_base + rk_phy->pcie_status);
}

static int phy_init(struct rockchip_pcie_phy *rk_phy)
{
	/*
	 * uboot should enable all the clks and please don't
	 * modify the PCIe clock hierarchy. We need 24M from OSC
	 * for the phy.
	 */

	/* assert phy-rst SOFTRST_CON8 */
	rkcru_pcie_soft_reset(PCIE_RESET_PHY, 1);
	return 0;
}

static int phy_power_on(struct rockchip_pcie_phy *rk_phy)
{
	u32 status;
	u32 timeout = 2000;

	DBG("PCIe phy_power_on start!\n");

	/* deassert phy-rst */
	rkcru_pcie_soft_reset(PCIE_RESET_PHY, 0);

	/* lock-> sel & scc -> relock */
	writel(HIWORD_UPDATE_PHY(PHY_CFG_PLL_LOCK,
				 PHY_CFG_ADDR_MASK,
				 PHY_CFG_ADDR_SHIFT),
	       rk_phy->reg_base  + rk_phy->pcie_conf);

	while (timeout--) {
		status = readl(rk_phy->reg_base + rk_phy->pcie_status);
		if (status & PHY_PLL_LOCKED) {
			DBG("pll locked!\n");
			break;
		}
		mdelay(1);
	}

	if (timeout == 0) {
		DBG("pll lock timeout\n");
		return -1;
	}

	phy_wr_cfg(rk_phy, PHY_CFG_CLK_TEST, PHY_CFG_SEPE_RATE);
	phy_wr_cfg(rk_phy, PHY_CFG_CLK_SCC, PHY_CFG_PLL_100M);

	timeout = 2000;
	while (timeout--) {
		status = readl(rk_phy->reg_base + rk_phy->pcie_status);
		if (!(status & PHY_PLL_OUTPUT)) {
			DBG("pll output enable done!\n");
			break;
		}
		mdelay(1);
	}

	if (timeout == 0) {
		DBG("pll output enable timeout\n");
		return -1;
	}

	writel(HIWORD_UPDATE_PHY(PHY_CFG_PLL_LOCK,
				 PHY_CFG_ADDR_MASK,
				 PHY_CFG_ADDR_SHIFT),
	       rk_phy->reg_base  + rk_phy->pcie_conf);

	timeout = 2000;
	while (timeout--) {
		status = readl(rk_phy->reg_base + rk_phy->pcie_status);
		if (status & PHY_PLL_LOCKED) {
			DBG("pll relocked!\n");
			break;
		}
		mdelay(1);
	}

	if (timeout == 0) {
		DBG("pll relock timeout\n");
		return -1;
	}

	DBG("PCIe phy_power_on end!\n");

	return 0;
}

static u32 rockchip_pcie_read(struct rockchip_pcie *rockchip, u32 reg)
{
	return readl(rockchip->apb_base + reg);
}

static void rockchip_pcie_write(struct rockchip_pcie *rockchip,
				u32 val, u32 reg)
{
	writel(val, rockchip->apb_base + reg);
}

static int rockchip_pcie_rd_own_conf(void *priv, int where, int size, u32 *val)
{
	struct rockchip_pcie *rockchip = (struct rockchip_pcie *)priv;
	u32 addr = rockchip->apb_base + PCIE_RC_CONFIG_BASE + where;

	if (size == 4) {
		*val = readl(addr);
	} else if (size == 2) {
		*val = readw(addr);
	} else if (size == 1) {
		*val = readb(addr);
	} else {
		*val = 0;
		return -1;
	}
	return 0;
}

static int rockchip_pcie_wr_own_conf(void *priv, int where, int size, u32 val)
{
	struct rockchip_pcie *rockchip = (struct rockchip_pcie *)priv;
	u32 mask, tmp, offset;

	offset = where & ~0x3;

	if (size == 4) {
		writel(val, rockchip->apb_base + PCIE_RC_CONFIG_BASE + offset);
		return 0;
	}

	mask = ~(((1 << (size * 8)) - 1) << ((where & 0x3) * 8));

	/*
	 * N.B. This read/modify/write isn't safe in general because it can
	 * corrupt RW1C bits in adjacent registers.  But the hardware
	 * doesn't support smaller writes.
	 */
	tmp = readl(rockchip->apb_base + PCIE_RC_CONFIG_BASE + offset) & mask;
	tmp |= val << ((where & 0x3) * 8);
	writel(tmp, rockchip->apb_base + PCIE_RC_CONFIG_BASE + offset);

	return 0;
}

static int rockchip_pcie_rd_other_conf(void *priv, int where,
				       int size, u32 *val)
{
	u32 busdev;
	struct rockchip_pcie *rockchip = (struct rockchip_pcie *)priv;

	/*
	 * BDF = 01:00:00
	 * end-to-end support, no hierarchy....
	 */
	busdev = PCIE_ECAM_ADDR(1, 0, 0, where);

	if (size == 4) {
		*val = readl(rockchip->axi_base + busdev);
	} else if (size == 2) {
		*val = readw(rockchip->axi_base + busdev);
	} else if (size == 1) {
		*val = readb(rockchip->axi_base + busdev);
	} else {
		*val = 0;
		return -1;
	}
	return 0;
}

static int rockchip_pcie_wr_other_conf(void *priv, int where, int size, u32 val)
{
	struct rockchip_pcie *rockchip = (struct rockchip_pcie *)priv;
	u32 busdev;

	/*
	 * BDF = 01:00:00
	 * end-to-end support, no hierarchy....
	 */
	busdev = PCIE_ECAM_ADDR(1, 0, 0, where);

	if (size == 4)
		writel(val, rockchip->axi_base + busdev);
	else if (size == 2)
		writew(val, rockchip->axi_base + busdev);
	else if (size == 1)
		writeb(val, rockchip->axi_base + busdev);
	else
		return -1;

	return 0;
}

static int rockchip_pcie_init_port(struct rockchip_pcie *rockchip)
{
	u32 status;
	u32 timeout;
	u32 reg;

	/* assert aclk_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_ACLK, 1);
	/* assert pclk_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_ACLK, 1);
	/* assert pm_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_PM, 1);

	udelay(10); /* need a nearly 10us delay per design */

	/* deassert aclk_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_PM, 0);
	/* deassert pclk_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_ACLK, 0);
	/* deassert pm_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_PCLK, 0);

	phy_init(rockchip->phy);

	/* assert: mgmt_sticky_rst->core_rst->mgmt_rst->pipe_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_NOFATAL, 1);

	rockchip_pcie_write(rockchip, PCIE_CLIENT_GEN_SEL_1,
			    PCIE_CLIENT_CONFIG);

	rockchip_pcie_write(rockchip,
			    PCIE_CLIENT_CONF_ENABLE |
			    PCIE_CLIENT_LINK_TRAIN_ENABLE |
			    PCIE_CLIENT_ARI_ENABLE |
			    PCIE_CLIENT_CONF_LANE_NUM(4) |
			    PCIE_CLIENT_MODE_RC,
			    PCIE_CLIENT_CONFIG);

	phy_power_on(rockchip->phy);

	/* assert: mgmt_sticky_rst->core_rst->mgmt_rst->pipe_rst */
	rkcru_pcie_soft_reset(PCIE_RESET_NOFATAL, 0);

	/* Enable Gen1 training */
	rockchip_pcie_write(rockchip, PCIE_CLIENT_LINK_TRAIN_ENABLE,
			    PCIE_CLIENT_CONFIG);

	if (!fixed_resource) {
		gpio_direction_output(rockchip->gpio.gpio, rockchip->gpio.flags);
	} else {
		#ifdef CONFIG_RKCHIP_RK3399
		/* GPIO3 A7 output high level */
		printf("pcie_cdns: warning: double check your reset io\n");
		reg = readl(0xff788000);
		reg |= BIT(7);
		writel(reg, 0xff788000);
		#else
		DBG_ERR("please do the correct reset ops for pcie_cdns\n");
		#endif
	}

	timeout = 2000;
	while (timeout--) {
		status = rockchip_pcie_read(rockchip,
					    PCIE_CLIENT_BASIC_STATUS1);
		if ((status & PCIE_CLIENT_LINK_STATUS_MASK) ==
			PCIE_CLIENT_LINK_STATUS_UP) {
			DBG("PCIe link training gen1 pass!\n");
			break;
		}
		mdelay(1);
	}

	if (!timeout) {
		DBG_ERR("PCIe link training gen1 timeout!\n");
		return -ETIMEDOUT;
	}

	/* Check the final link width from negotiated lane counter from MGMT */
	status = rockchip_pcie_read(rockchip, PCIE_CORE_CTRL);
	status = 0x1 << ((status & PCIE_CORE_PL_CONF_LANE_MASK) >>
			  PCIE_CORE_PL_CONF_LANE_SHIFT);
	DBG("current link width is x%d\n", status);

	rockchip_pcie_write(rockchip, ROCKCHIP_VENDOR_ID,
			    PCIE_RC_CONFIG_VENDOR);
	rockchip_pcie_write(rockchip,
			    PCI_CLASS_BRIDGE_PCI << PCIE_RC_CONFIG_SCC_SHIFT,
			    PCIE_RC_CONFIG_RID_CCR);
	rockchip_pcie_write(rockchip, 0x0, PCIE_RC_BAR_CONF);
	rockchip_pcie_write(rockchip,
			    (RC_REGION_0_ADDR_TRANS_L + RC_REGION_0_PASS_BITS),
			    PCIE_CORE_OB_REGION_ADDR0);
	rockchip_pcie_write(rockchip, RC_REGION_0_ADDR_TRANS_H,
			    PCIE_CORE_OB_REGION_ADDR1);
	rockchip_pcie_write(rockchip, 0x0080000a, PCIE_CORE_OB_REGION_DESC0);

	return 0;
}

static int rockchip_pcie_prog_ob_atu(struct rockchip_pcie *rockchip,
				     int region_no, int type, u8 num_pass_bits,
				     u32 lower_addr, u32 upper_addr)
{
	u32 ob_addr_0;
	u32 ob_addr_1;
	u32 ob_desc_0;
	u32 aw_offset;

	if (region_no >= MAX_AXI_WRAPPER_REGION_NUM)
		return -1;
	if (num_pass_bits + 1 < 8)
		return -1;
	if (num_pass_bits > 63)
		return -1;
	if (region_no == 0) {
		if (AXI_REGION_0_SIZE < (2ULL << num_pass_bits))
		return -1;
	}
	if (region_no != 0) {
		if (AXI_REGION_SIZE < (2ULL << num_pass_bits))
			return -1;
	}

	aw_offset = (region_no << OB_REG_SIZE_SHIFT);

	ob_addr_0 = num_pass_bits & PCIE_CORE_OB_REGION_ADDR0_NUM_BITS;
	ob_addr_0 |= lower_addr & PCIE_CORE_OB_REGION_ADDR0_LO_ADDR;
	ob_addr_1 = upper_addr;
	ob_desc_0 = (1 << 23 | type);

	rockchip_pcie_write(rockchip, ob_addr_0,
			    PCIE_CORE_OB_REGION_ADDR0 + aw_offset);
	rockchip_pcie_write(rockchip, ob_addr_1,
			    PCIE_CORE_OB_REGION_ADDR1 + aw_offset);
	rockchip_pcie_write(rockchip, ob_desc_0,
			    PCIE_CORE_OB_REGION_DESC0 + aw_offset);
	rockchip_pcie_write(rockchip, 0,
			    PCIE_CORE_OB_REGION_DESC1 + aw_offset);

	return 0;
}

static int rockchip_pcie_prog_ib_atu(struct rockchip_pcie *rockchip,
				     int region_no, u8 num_pass_bits,
				     u32 lower_addr, u32 upper_addr)
{
	u32 ib_addr_0;
	u32 ib_addr_1;
	u32 aw_offset;

	if (region_no > MAX_AXI_IB_ROOTPORT_REGION_NUM)
		return -1;
	if (num_pass_bits + 1 < MIN_AXI_ADDR_BITS_PASSED)
		return -1;
	if (num_pass_bits > 63)
		return -1;

	aw_offset = (region_no << IB_ROOT_PORT_REG_SIZE_SHIFT);

	ib_addr_0 = num_pass_bits & PCIE_CORE_IB_REGION_ADDR0_NUM_BITS;
	ib_addr_0 |= (lower_addr << 8) & PCIE_CORE_IB_REGION_ADDR0_LO_ADDR;
	ib_addr_1 = upper_addr;

	rockchip_pcie_write(rockchip, ib_addr_0, PCIE_RP_IB_ADDR0 + aw_offset);
	rockchip_pcie_write(rockchip, ib_addr_1, PCIE_RP_IB_ADDR1 + aw_offset);

	return 0;
}

static int rockchip_pcie_cfg_atu(struct rockchip_pcie *rockchip)
{
	u32 mem_bus_addr = rockchip->axi_base + rockchip->axi_size;
	u32 mem_size = 0;
	u32 io_bus_addr = mem_bus_addr + mem_size;
	u32 io_size = 0;
	struct pcie_bus *pbus = rockchip->bus;
	int reg_no;
	int offset;
	int err, i;

	for (i = 0; i < pbus->region_count; i++) {
		if (pbus->regions[i].flags == PCI_REGION_MEM) {
			mem_size = (u32)(pbus->regions[i].size);
			DBG("mem_size = 0x%x\n", mem_size);
		} else if (pbus->regions[i].flags == PCI_REGION_IO) {
			io_size = pbus->regions[i].size;
			DBG("io_size = 0x%x\n", io_size);
		}
	}

	if (mem_size) {
		for (reg_no = 0; reg_no < (mem_size >> 20); reg_no++) {
			err = rockchip_pcie_prog_ob_atu(rockchip, reg_no + 1,
							AXI_WRAPPER_MEM_WRITE,
							20 - 1,
							mem_bus_addr +
							(reg_no << 20),
							0);
			if (err)
				DBG_ERR("program RC mem outbound ATU failed\n");
		}
	}

	err = rockchip_pcie_prog_ib_atu(rockchip, 2, 32 - 1, 0x0, 0);
	if (err)
		DBG_ERR("program RC mem inbound ATU failed\n");

	offset = mem_size >> 20;

	if (io_size) {
		for (reg_no = 0; reg_no < (io_size >> 20); reg_no++) {
			err = rockchip_pcie_prog_ob_atu(rockchip,
							reg_no + 1 + offset,
							AXI_WRAPPER_IO_WRITE,
							20 - 1,
							io_bus_addr +
							(reg_no << 20),
							0);
			if (err)
				DBG_ERR("program RC io outbound ATU failed\n");
		}
	}

	return 0;
}

const char *compat[] = {
	"rockchip,rk3399-pcie",
};

static int rockchip_pcie_parse_dt(struct rockchip_pcie *rockchip)
{
	int node = -1;
	int i;
	u32 reg;
	struct fdt_resource res_axi, res_apb;
	struct pcie_bus *pbus = rockchip->bus;

	if (!gd->fdt_blob) {
		DBG_ERR("rockchip_pcie_parse_dt: gd->fdt_blob no found\n");
		fixed_resource = true;
		goto do_fixed;
	}

	for (i = 0; i < ARRAY_SIZE(compat); i++) {
		node = fdt_node_offset_by_compatible(gd->fdt_blob,
						     0, compat[i]);
		if (node > 0)
			break;
	}

	if (node < 0) {
		DBG_ERR("rockchip_pcie_parse_dt: no compat found\n");
		return node;
	}

	if (!fdt_device_is_available(gd->fdt_blob, node)) {
		DBG_ERR("PCIe is disabled\n");
		return -EPERM;
	}

	i = fdt_get_named_resource(gd->fdt_blob, node, "reg", "reg-names",
				   "axi-base", &res_axi);
	if (i) {
		DBG_ERR("can't get regs axi-base addresses!\n");
		return -ENOMEM;
	}

	i = fdt_get_named_resource(gd->fdt_blob, node, "reg", "reg-names",
				   "apb-base", &res_apb);
	if (i) {
		DBG_ERR("can't get regs apb-base addresses!\n");
		return -ENOMEM;
	}

do_fixed:
	if (fixed_resource) {
		#ifdef CONFIG_RKCHIP_RK3399
		rockchip->axi_base = 0xf8000000;
		rockchip->apb_base = 0xfd000000;
		rockchip->axi_size = 0x2000000;
		#else
		DBG_ERR("please assign your PCIe resource\n");
		return -EINVAL;
		#endif
	} else {
		rockchip->axi_base = res_axi.start;
		rockchip->apb_base = res_apb.start;
		rockchip->axi_size = res_axi.end - res_axi.start + 1;
	}

	DBG("rockchip->axi_base = 0x%x\n", rockchip->axi_base);
	DBG("rockchip->apb_base = 0x%x\n", rockchip->apb_base);
	DBG("rockchip->axi_size = 0x%x\n", rockchip->axi_size);

	if (fixed_resource) {
		#ifdef CONFIG_RKCHIP_RK3399
		rockchip->phy->reg_base = RKIO_GRF_PHYS;
		rockchip->phy->rst_addr = RKIO_CRU_PHYS + 0x420;
		rockchip->phy->pcie_conf = 0xe220;
		rockchip->phy->pcie_status = 0xe2a4;
		rockchip->rst_addr = RKIO_CRU_PHYS + 0x420;
		rockchip->bus->msi_base = 0xfee30040;
		#else
		DBG_ERR("please assign your PCIe resource\n");
		return -EINVAL;
		#endif
	} else if (!strcmp(compat[i], "rockchip,rk3399-pcie")) {
		rockchip->phy->reg_base = RKIO_GRF_PHYS;
		rockchip->phy->rst_addr = RKIO_CRU_PHYS + 0x420;
		rockchip->phy->pcie_conf = 0xe220;
		rockchip->phy->pcie_status = 0xe2a4;
		rockchip->rst_addr = RKIO_CRU_PHYS + 0x420;
		rockchip->bus->msi_base = 0xfee30040;
	} else {
		DBG_ERR("unknown Soc using pcie_cdns\n");
		return -EINVAL;
	}

	if (fixed_resource)
		goto fixed_rst;

	fdtdec_decode_gpio(gd->fdt_blob, node, "ep-gpios", &rockchip->gpio);
	rockchip->gpio.flags = !(rockchip->gpio.flags & OF_GPIO_ACTIVE_LOW);
	gpio_direction_output(rockchip->gpio.gpio, !rockchip->gpio.flags);
	pcie_decode_regions(rockchip->bus, gd->fdt_blob, node);
	return 0;

fixed_rst:
	#ifdef CONFIG_RKCHIP_RK3399
	/* set GPIO3 A7 as output low now*/
	printf("pcie_cdns: warning: double check your PCIe reset gpio!\n");
	writel((0x3 << 30) | (0x0 << 14), RKIO_GRF_PHYS + 0xe010);
	reg = readl(0xff788000 + 0x4);
	reg |= BIT(7);
	writel(reg, 0xff788000 + 0x4);
	reg = readl(0xff788000);
	reg &= ~BIT(7);
	writel(reg, 0xff788000);

	pbus->regions[0].flags = PCI_REGION_MEM;
	pbus->regions[0].phys_start = 0xfa000000;
	pbus->regions[0].bus_start = 0xfa000000;
	pbus->regions[0].size = 0x600000;
	pbus->regions[1].flags = PCI_REGION_IO;
	pbus->regions[1].phys_start = 0xfa600000;
	pbus->regions[1].bus_start = 0xfa600000;
	pbus->regions[1].size = 0x100000;
	pbus->region_count = 2;
	return 0;
	#else
	DBG_ERR("please assign the fixed_rst\n");
	return -EINVAL;
	#endif
}

struct pcie_ops bus_ops = {
	.rd_rc_conf = rockchip_pcie_rd_own_conf,
	.wr_rc_conf = rockchip_pcie_wr_own_conf,
	.rd_ep_conf = rockchip_pcie_rd_other_conf,
	.wr_ep_conf = rockchip_pcie_wr_other_conf,
};

int rockchip_pcie_cdns_probe(void)
{
	struct rockchip_pcie *rockchip;
	struct pcie_bus *pbus = NULL;
	int err;

	/* disable verbos log by default */
	dbg_enable = 0;

	rockchip = (struct rockchip_pcie *)malloc(sizeof(*rockchip));
	if (!rockchip) {
		DBG_ERR("no memory for pcie init\n");
		return -ENOMEM;
	}

	rockchip->phy = (struct rockchip_pcie_phy *)malloc
				(sizeof(struct rockchip_pcie_phy));
	if (!rockchip->phy) {
		DBG_ERR("no memory for pcie init\n");
		free(rockchip);
		return -ENOMEM;
	}

	pbus = pcie_alloc_bus(rockchip, &bus_ops);
	if (!pbus) {
		DBG_ERR("fail to alooc bus\n");
		err = -ENOMEM;
		goto err_free;
	}

	rockchip->bus = pbus;

	err = rockchip_pcie_parse_dt(rockchip);
	if (err)
		goto err_free;

	/* Setup link */
	err = rockchip_pcie_init_port(rockchip);
	if (err) {
		DBG_ERR("failed to init port\n");
		goto err_free;
	}

	/* Setup ATU */
	err = rockchip_pcie_cfg_atu(rockchip);
	if (err) {
		DBG_ERR("failed to configure atu\n");
		goto err_free;
	}

	return 0;
err_free:
	free(rockchip->phy);
	free(rockchip);
	return err;
}

