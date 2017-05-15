/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * Rockchip SD Host Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/hardware.h>
#include <common.h>
#include <dm.h>
#include <dt-structs.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <mapmem.h>
#include <sdhci.h>
#include <clk.h>

DECLARE_GLOBAL_DATA_PTR;
/* 400KHz is max freq for card ID etc. Use that as min */
#define EMMC_MIN_FREQ	400000

struct rockchip_sdhc_plat {
#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_rockchip_rk3399_sdhci_5_1 dtplat;
#endif
	struct mmc_config cfg;
	struct mmc mmc;
};

struct rockchip_emmc_phy {
	u32 emmcphy_con[7];
	u32 reserved;
	u32 emmcphy_status;
};

struct rockchip_sdhc {
	struct sdhci_host host;
	void *base;
	struct rockchip_emmc_phy *phy;
};

#define PHYCTRL_CALDONE_MASK		0x1
#define PHYCTRL_CALDONE_SHIFT		0x6
#define PHYCTRL_CALDONE_DONE		0x1

#define PHYCTRL_DLLRDY_MASK		0x1
#define PHYCTRL_DLLRDY_SHIFT		0x5
#define PHYCTRL_DLLRDY_DONE		0x1

#define PHYCTRL_FREQSEL_200M            0x0
#define PHYCTRL_FREQSEL_50M             0x1
#define PHYCTRL_FREQSEL_100M            0x2
#define PHYCTRL_FREQSEL_150M            0x3

#define KHz	(1000)
#define MHz	(1000 * KHz)

static void rk3399_emmc_phy_power_on(struct rockchip_emmc_phy *phy, u32 clock)
{
	u32 caldone, dllrdy, freqsel;
	uint start;

	writel(RK_CLRSETBITS(7 << 4, 0), &phy->emmcphy_con[6]);
	writel(RK_CLRSETBITS(1 << 11, 1 << 11), &phy->emmcphy_con[0]);
	writel(RK_CLRSETBITS(0xf << 7, 4 << 7), &phy->emmcphy_con[0]);

	/*
	 * According to the user manual, calpad calibration
	 * cycle takes more than 2us without the minimal recommended
	 * value, so we may need a little margin here
	 */
	udelay(3);
	writel(RK_CLRSETBITS(1, 1), &phy->emmcphy_con[6]);

	/*
	 * According to the user manual, it asks driver to
	 * wait 5us for calpad busy trimming
	 */
	udelay(5);
	caldone = readl(&phy->emmcphy_status);
	caldone = (caldone >> PHYCTRL_CALDONE_SHIFT) & PHYCTRL_CALDONE_MASK;
	if (caldone != PHYCTRL_CALDONE_DONE) {
		debug("%s: caldone timeout.\n", __func__);
		return;
	}

	/* Set the frequency of the DLL operation */
	if (clock < 75 * MHz)
		freqsel = PHYCTRL_FREQSEL_50M;
	else if (clock < 125 * MHz)
		freqsel = PHYCTRL_FREQSEL_100M;
	else if (clock < 175 * MHz)
		freqsel = PHYCTRL_FREQSEL_150M;
	else
		freqsel = PHYCTRL_FREQSEL_200M;

	/* Set the frequency of the DLL operation */
	writel(RK_CLRSETBITS(3 << 12, freqsel << 12), &phy->emmcphy_con[0]);
	writel(RK_CLRSETBITS(1 << 1, 1 << 1), &phy->emmcphy_con[6]);

	start = get_timer(0);

	do {
		udelay(1);
		dllrdy = readl(&phy->emmcphy_status);
		dllrdy = (dllrdy >> PHYCTRL_DLLRDY_SHIFT) & PHYCTRL_DLLRDY_MASK;
		if (dllrdy == PHYCTRL_DLLRDY_DONE)
			break;
	} while (get_timer(start) < 50000);

	if (dllrdy != PHYCTRL_DLLRDY_DONE)
		debug("%s: dllrdy timeout.\n", __func__);
}

static void rk3399_emmc_phy_power_off(struct rockchip_emmc_phy *phy)
{
	writel(RK_CLRSETBITS(1, 0), &phy->emmcphy_con[6]);
	writel(RK_CLRSETBITS(1 << 1, 0), &phy->emmcphy_con[6]);
}

static int arasan_sdhci_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct rockchip_sdhc *priv =
			container_of(host, struct rockchip_sdhc, host);
	int cycle_phy = host->clock != clock &&
			clock > EMMC_MIN_FREQ;

	if (cycle_phy)
		rk3399_emmc_phy_power_off(priv->phy);

	sdhci_set_clock(host, clock);

	if (cycle_phy)
		rk3399_emmc_phy_power_on(priv->phy, clock);

	return 0;
}

static struct sdhci_ops arasan_sdhci_ops = {
	.set_clock	= arasan_sdhci_set_clock,
};

static int arasan_get_phy(struct udevice *dev)
{
	struct rockchip_sdhc *priv = dev_get_priv(dev);

#if CONFIG_IS_ENABLED(OF_PLATDATA)
	priv->phy = (struct rockchip_emmc_phy *)0xff77f780;
#else
	int phy_node, grf_node;
	fdt_addr_t grf_base, grf_phy_offset;

	phy_node = fdtdec_lookup_phandle(gd->fdt_blob,
					 dev_of_offset(dev), "phys");
	if (phy_node <= 0) {
		debug("Not found emmc phy device\n");
		return -ENODEV;
	}

	grf_node = fdt_parent_offset(gd->fdt_blob, phy_node);
	if (grf_node <= 0) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	grf_base = fdtdec_get_addr(gd->fdt_blob, grf_node, "reg");
	grf_phy_offset = fdtdec_get_addr_size_auto_parent(gd->fdt_blob,
				grf_node, phy_node, "reg", 0, NULL, false);

	priv->phy = (struct rockchip_emmc_phy *)(grf_base + grf_phy_offset);
#endif
	return 0;
}

static int arasan_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct rockchip_sdhc_plat *plat = dev_get_platdata(dev);
	struct rockchip_sdhc *prv = dev_get_priv(dev);
	struct sdhci_host *host = &prv->host;
	int max_frequency, ret;
	struct clk clk;

#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_rockchip_rk3399_sdhci_5_1 *dtplat = &plat->dtplat;

	host->name = dev->name;
	host->ioaddr = map_sysmem(dtplat->reg[1], dtplat->reg[3]);
	host->host_caps |= MMC_MODE_8BIT;
	max_frequency = dtplat->max_frequency;
	ret = clk_get_by_index_platdata(dev, 0, dtplat->clocks, &clk);
#else
	max_frequency = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
			"max-frequency", 0);
	switch (fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
			       "bus-width", 4)) {
	case 8:
		host->host_caps |= MMC_MODE_8BIT;
		break;
	case 4:
		host->host_caps |= MMC_MODE_4BIT;
		break;
	case 1:
		break;
	default:
		printf("Invalid \"bus-width\" value\n");
		return -EINVAL;
	}
	ret = clk_get_by_index(dev, 0, &clk);
#endif
	if (!ret) {
		ret = clk_set_rate(&clk, max_frequency);
		if (IS_ERR_VALUE(ret))
			printf("%s clk set rate fail!\n", __func__);
	} else {
		printf("%s fail to get clk\n", __func__);
	}

	ret = arasan_get_phy(dev);
	if (ret)
		return ret;

	host->ops = &arasan_sdhci_ops;

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;
	host->max_clk = max_frequency;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, EMMC_MIN_FREQ);

	host->mmc = &plat->mmc;
	if (ret)
		return ret;
	host->mmc->priv = &prv->host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static int arasan_sdhci_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct sdhci_host *host = dev_get_priv(dev);

	host->name = dev->name;
	host->ioaddr = dev_get_addr_ptr(dev);
#endif

	return 0;
}

static int rockchip_sdhci_bind(struct udevice *dev)
{
	struct rockchip_sdhc_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id arasan_sdhci_ids[] = {
	{ .compatible = "arasan,sdhci-5.1" },
	{ }
};

U_BOOT_DRIVER(arasan_sdhci_drv) = {
	.name		= "rockchip_rk3399_sdhci_5_1",
	.id		= UCLASS_MMC,
	.of_match	= arasan_sdhci_ids,
	.ofdata_to_platdata = arasan_sdhci_ofdata_to_platdata,
	.ops		= &sdhci_ops,
	.bind		= rockchip_sdhci_bind,
	.probe		= arasan_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct rockchip_sdhc),
	.platdata_auto_alloc_size = sizeof(struct rockchip_sdhc_plat),
};
