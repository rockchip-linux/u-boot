// SPDX-License-Identifier: GPL-2.0+
/**
 *
 * Driver for ROCKCHIP Fast Ethernet PHYs
 *
 * Copyright (c) 2025, Rockchip Electronics Co., Ltd
 *
 * David Wu <david.wu@rock-chips.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <config.h>
#include <common.h>
#include <misc.h>
#include <phy.h>
#include <regmap.h>
#include <syscon.h>
#include <dm/device_compat.h>

#define ROCKCHIP_FEPHY_ID			0x06808101

#define MII_INTERNAL_CTRL_STATUS		17
#define SMI_ADDR_CFGCNTL			20
#define SMI_ADDR_TSTREAD1			21
#define SMI_ADDR_TSTREAD2			22
#define SMI_ADDR_TSTWRITE			23
#define MII_LED_CTRL				25
#define MII_INT_STATUS				29
#define MII_INT_MASK				30
#define MII_SPECIAL_CONTROL_STATUS		31

#define MII_AUTO_MDIX_EN			BIT(7)
#define MII_MDIX_EN				BIT(6)

#define MII_SPEED_10				BIT(2)
#define MII_SPEED_100				BIT(3)

#define CFGCNTL_WRITE_ADDR			0
#define CFGCNTL_READ_ADDR			5
#define CFGCNTL_GROUP_SEL			11
#define CFGCNTL_RD				(BIT(15) | BIT(10))
#define CFGCNTL_WR				(BIT(14) | BIT(10))

#define CFGCNTL_WRITE(group, reg)		(CFGCNTL_WR | ((group) << CFGCNTL_GROUP_SEL) \
						| ((reg) << CFGCNTL_WRITE_ADDR))
#define CFGCNTL_READ(group, reg)		(CFGCNTL_RD | ((group) << CFGCNTL_GROUP_SEL) \
						| ((reg) << CFGCNTL_READ_ADDR))

#define GAIN_PRE				GENMASK(5, 2)
#define WR_ADDR_A7CFG				0x18

#define MDIX_OFFSET_MIN				-5
#define MDI_OFFSET_MAX				3
#define OFFSET_TIMES_MAX			5

enum {
	GROUP_CFG0 = 0,
	GROUP_WOL,
	GROUP_CFG0_READ,
	GROUP_BIST,
	GROUP_AFE,
	GROUP_CFG1
};

static struct regmap *fephy_regs;

static int rockchip_fephy_group_read(struct phy_device *phydev, u8 group, u32 reg)
{
	int ret;

	ret = phy_write(phydev, MDIO_DEVAD_NONE, SMI_ADDR_CFGCNTL, CFGCNTL_READ(group, reg));
	if (ret)
		return ret;

	if (group)
		return phy_read(phydev, MDIO_DEVAD_NONE, SMI_ADDR_TSTREAD1);
	else
		return (phy_read(phydev, MDIO_DEVAD_NONE, SMI_ADDR_TSTREAD1) |
			(phy_read(phydev, MDIO_DEVAD_NONE, SMI_ADDR_TSTREAD2) << 16));
}

static int rockchip_fephy_group_write(struct phy_device *phydev, u8 group,
				      u32 reg, u16 val)
{
	int ret;

	ret = phy_write(phydev, MDIO_DEVAD_NONE, SMI_ADDR_TSTWRITE, val);
	if (ret)
		return ret;

	return phy_write(phydev, MDIO_DEVAD_NONE, SMI_ADDR_CFGCNTL, CFGCNTL_WRITE(group, reg));
}

static int rockchip_fephy_startup(struct phy_device *phydev)
{
	int ret;

	/* Read the Status (2x to make sure link is right) */
	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	/* Read the Status (2x to make sure link is right) */
	phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

	return genphy_parse_link(phydev);
}

static int rockchip_fephy_get_txamp_from_nvmem(struct phy_device *phydev, int *txamp)
{
#if defined(CONFIG_ROCKCHIP_EFUSE) || defined(CONFIG_ROCKCHIP_OTP)
	unsigned char efuse_buf[2];
	struct udevice *dev;
	u32 regs[2] = {0};
	int txamp_type;
	ofnode node;
	int ret;

	/* retrieve the device */
	if (IS_ENABLED(CONFIG_ROCKCHIP_EFUSE))
		ret = uclass_get_device_by_driver(UCLASS_MISC,
						  DM_DRIVER_GET(rockchip_efuse),
						  &dev);
	else
		ret = uclass_get_device_by_driver(UCLASS_MISC,
						  DM_DRIVER_GET(rockchip_otp),
						  &dev);

	if (ret) {
		dev_err(phydev->dev, "%s: could not find efuse/otp device\n", __func__);
		return ret;
	}

	node = dev_read_subnode(dev, "macphy-txamp");
	if (!ofnode_valid(node))
		return -EINVAL;

	ret = ofnode_read_u32_array(node, "reg", regs, 2);
	if (ret) {
		dev_err(phydev->dev, "Cannot get efuse reg\n");
		return -EINVAL;
	}

	/* read the txamp from the efuses */
	ret = misc_read(dev, regs[0], &efuse_buf, 2);
	if (ret) {
		dev_err(phydev->dev, "%s: read txamp from efuse/otp failed, ret = %d\n",
		        __func__, ret);
		return ret;
	}

	*txamp = efuse_buf[0] & 0x1f;
	txamp_type = efuse_buf[1] & 0x7f;
	/* For some cases, if it's an odd number, add 3 */
	if (txamp_type == 0x8 && (*txamp & 1))
		*txamp += 3;

	return 0;
#else
	return -EINVAL;
#endif
}

static int rockchip_fephy_get_adc_offset_from_nvmem(struct phy_device *phydev, int *mdi_offset, int *mdix_offset)
{
#if defined(CONFIG_ROCKCHIP_EFUSE) || defined(CONFIG_ROCKCHIP_OTP)
	unsigned char efuse_buf[2];
	struct udevice *dev;
	u32 regs[2] = {0};
	ofnode node;
	int ret;

	/* retrieve the device */
	if (IS_ENABLED(CONFIG_ROCKCHIP_EFUSE))
		ret = uclass_get_device_by_driver(UCLASS_MISC,
						  DM_DRIVER_GET(rockchip_efuse),
						  &dev);
	else
		ret = uclass_get_device_by_driver(UCLASS_MISC,
						  DM_DRIVER_GET(rockchip_otp),
						  &dev);

	if (ret) {
		dev_err(phydev->dev, "%s: could not find efuse/otp device\n", __func__);
		return ret;
	}

	node = dev_read_subnode(dev, "macphy-adc-offset");
	if (!ofnode_valid(node))
		return -EINVAL;

	ret = ofnode_read_u32_array(node, "reg", regs, 2);
	if (ret) {
		dev_err(phydev->dev, "Cannot get efuse reg\n");
		return -EINVAL;
	}

	/* read the adc offset from the efuses */
	ret = misc_read(dev, regs[0], &efuse_buf, 2);
	if (ret) {
		dev_err(phydev->dev, "%s: read adc_offset from efuse/otp failed, ret=%d\n",
			__func__, ret);
		return ret;
	}

	*mdi_offset = efuse_buf[0] & 0x7f;
	*mdix_offset = efuse_buf[1] & 0x7f;

	return 0;
#else
	return -EINVAL;
#endif
}

static int rockchip_fephy_fix_offset_v0(struct phy_device *phydev, int priv_mdi_offset, int priv_mdix_offset)
{
	int offset, mdi_offset, mdix_offset, sum;
	int mdi_fix, mdix_fix;
	int ret = 0;

	mdi_offset = (priv_mdi_offset < 0x40) ? priv_mdi_offset : (priv_mdi_offset - 0x80);
	mdix_offset = (priv_mdix_offset < 0x40) ? priv_mdix_offset : (priv_mdix_offset - 0x80);

	sum = mdi_offset + mdix_offset;
	/* Balance to smaller side */
	offset = ((sum >= 0) ? (sum + 1) : sum) / 2;
	offset -= (MDI_OFFSET_MAX + MDIX_OFFSET_MIN) / 2;
	mdi_fix = mdi_offset - offset;
	mdix_fix = mdix_offset - offset;
	if (mdi_fix > MDI_OFFSET_MAX || mdix_fix < MDIX_OFFSET_MIN) {
		int reg;

		reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_INTERNAL_CTRL_STATUS);
		if (reg < 0)
			return reg;

		if ((abs(mdi_offset)) <= abs(mdix_offset)) {
			offset = mdi_offset;
			/* Force MDI */
			reg &= ~(MII_MDIX_EN | MII_AUTO_MDIX_EN);
		} else {
			offset = mdix_offset;
			/* Force MDIX */
			reg &= ~MII_AUTO_MDIX_EN;
			reg |= MII_MDIX_EN;
		}

		/* update fix */
		mdi_fix = mdi_offset - offset;
		mdix_fix = mdix_offset - offset;

		ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_INTERNAL_CTRL_STATUS, reg);
		if (ret)
			return ret;
	}

	offset = (offset >= 0) ? offset : (offset + 0x80);
	offset &= 0x7f;
	regmap_write(fephy_regs, 0xC0, offset);
	regmap_write(fephy_regs, 0xA0, 0xFFFF0008);

	return ret;
}

static int rockchip_fephy_fix_offset_v1(struct phy_device *phydev, int priv_mdi_offset, int priv_mdix_offset)
{
	int ret = 0, val;

	priv_mdi_offset &= 0x7f;
	priv_mdix_offset &= 0x7f;

	val = 0xFFFF0000 | (priv_mdi_offset | (priv_mdix_offset << 8));
	regmap_write(fephy_regs, 0xB4, val);
	regmap_write(fephy_regs, 0xA0, 0xFFFF0008);

	return ret;
}

static int rockchip_fephy_config_init(struct phy_device *phydev)
{
	int ret, val = 0, txamp, mdi_offset, mdix_offset;
	struct ofnode_phandle_args args;
	struct udevice *phy_dev;

	ret = dev_read_phandle_with_args(phydev->dev, "phy-handle", NULL, 0, 0, &args);
	if (ret) {
		dev_err(phydev->dev, "Cannot get phy phandle: ret=%d\n", ret);
		return ret;
	}

	ret = uclass_get_device_by_ofnode(UCLASS_ETH_PHY, args.node, &phy_dev);
	if (ret) {
		dev_err(phydev->dev, "Get phydev by ofnode failed: err=%d\n", ret);
		return ret;
	}

	fephy_regs = syscon_regmap_lookup_by_phandle(phy_dev, "rockchip,macphy");
	if (IS_ERR(fephy_regs)) {
		dev_err(phydev->dev, "Missing rockchip,macphy property\n");
		return -EINVAL;
	}

	/* LED Control, default:0x7f */
	ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_LED_CTRL, 0x7aa);
	if (ret)
		return ret;

	/* off-energy level0 threshold */
	ret = rockchip_fephy_group_write(phydev, GROUP_CFG0, 0xa, 0x6664);
	if (ret)
		return ret;

	ret = rockchip_fephy_get_txamp_from_nvmem(phydev, &txamp);
	if (ret)
		txamp = 0x9;

	/* 100M amplitude control */
	ret = rockchip_fephy_group_write(phydev, GROUP_CFG0, 0x18, txamp);
	if (ret)
		return ret;

	/* 10M amplitude control */
	ret = rockchip_fephy_group_write(phydev, GROUP_CFG0, 0x1f, 0x7);
	if (ret)
		return ret;

	/* 24M */
	{
		int sel;

		/* pll cp cur sel */
		sel = rockchip_fephy_group_read(phydev, GROUP_AFE, 0x3);
		if (sel < 0)
			return sel;
		ret = rockchip_fephy_group_write(phydev, GROUP_AFE, 0x3, sel | 0x2);
		if (ret)
			return ret;

		/* pll lpf res sel */
		ret = rockchip_fephy_group_write(phydev, GROUP_CFG0, 0x1a, 0x6);
		if (ret)
			return ret;
	}

	ret = rockchip_fephy_get_adc_offset_from_nvmem(phydev, &mdi_offset, &mdix_offset);
	if (ret) {
		mdi_offset = 0;
		mdix_offset = 0;
	}

	regmap_read(fephy_regs, 0xfc, &val);
	if (val == 0x100)
		return rockchip_fephy_fix_offset_v1(phydev, mdi_offset, mdix_offset);
	else
		return rockchip_fephy_fix_offset_v0(phydev, mdi_offset, mdix_offset);

	return ret;
}

U_BOOT_PHY_DRIVER(rockchip_fephy) = {
	.name = "Rockchip 10/100Mbps FEPHY",
	.uid = ROCKCHIP_FEPHY_ID,
	.mask = 0xfffffff,
	.features = PHY_BASIC_FEATURES,
	.config = &rockchip_fephy_config_init,
	.startup = &rockchip_fephy_startup,
};
