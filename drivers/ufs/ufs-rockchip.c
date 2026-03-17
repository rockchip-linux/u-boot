// SPDX-License-Identifier: GPL-2.0+
/*
 * Rockchip UFS Host Controller driver
 *
 * Copyright (C) 2024 Rockchip Electronics Co.Ltd.
 */

#include <asm/io.h>
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/ioport.h>
#include <reset.h>
#include <ufs.h>

#include "ufs.h"
#include "unipro.h"
#include "ufs-rockchip.h"

extern int ufshcd_dme_enable(struct ufs_hba *hba);

static int ufs_rockchip_hce_enable_notify(struct ufs_hba *hba,
					  enum ufs_notify_change_status status)
{
	int err = 0;

	if (status == POST_CHANGE) {
		ufshcd_dme_reset(hba);
		ufshcd_dme_enable(hba);

		if (hba->ops->phy_initialization) {
			err = hba->ops->phy_initialization(hba);
			if (err) {
				dev_err(hba->dev, "Phy setup failed (%d)\n", err);
			}
		}
	}

	return err;
}

static int ufs_rockchip_startup_notify(struct ufs_hba *hba,
				       enum ufs_notify_change_status status)
{
	int err = 0;

	if (status == POST_CHANGE) {
		if (hba->ops->phy_parameter_initialization) {
			err = hba->ops->phy_parameter_initialization(hba);
			if (err) {
				dev_err(hba->dev, "Phy setup failed (%d)\n", err);
			}
		}
	}

	return err;
}

static const unsigned char rk3576_phy_value[15][4] = {
	{0x03, 0x38, 0x50, 0x80},
	{0x03, 0x14, 0x58, 0x80},
	{0x03, 0x26, 0x58, 0x80},
	{0x03, 0x49, 0x58, 0x80},
	{0x03, 0x5A, 0x58, 0x80},
	{0xC3, 0x38, 0x50, 0xC0},
	{0xC3, 0x14, 0x58, 0xC0},
	{0xC3, 0x26, 0x58, 0xC0},
	{0xC3, 0x49, 0x58, 0xC0},
	{0xC3, 0x5A, 0x58, 0xC0},
	{0x43, 0x38, 0x50, 0xC0},
	{0x43, 0x14, 0x58, 0xC0},
	{0x43, 0x26, 0x58, 0xC0},
	{0x43, 0x49, 0x58, 0xC0},
	{0x43, 0x5A, 0x58, 0xC0}
};

static int ufs_rockchip_rk3576_phy_parameter_init(struct ufs_hba *hba)
{
	struct ufs_rockchip_host *host = dev_get_priv(hba->dev);
	int try_case = host->phy_config_mode, value;

	ufs_sys_writel(host->mphy_base, 0x80, 0x08C);
	ufs_sys_writel(host->mphy_base, 0xB5, 0x110);
	ufs_sys_writel(host->mphy_base, 0xB5, 0x250);

	value = rk3576_phy_value[try_case][0];
	ufs_sys_writel(host->mphy_base, value, 0x134);
	ufs_sys_writel(host->mphy_base, value, 0x274);

	value = rk3576_phy_value[try_case][1];
	ufs_sys_writel(host->mphy_base, value, 0x0E0);
	ufs_sys_writel(host->mphy_base, value, 0x220);

	value = rk3576_phy_value[try_case][2];
	ufs_sys_writel(host->mphy_base, value, 0x164);
	ufs_sys_writel(host->mphy_base, value, 0x2A4);

	value = rk3576_phy_value[try_case][3];
	ufs_sys_writel(host->mphy_base, value, 0x178);
	ufs_sys_writel(host->mphy_base, value, 0x2B8);

	ufs_sys_writel(host->mphy_base, 0x18, 0x1B0);
	ufs_sys_writel(host->mphy_base, 0x18, 0x2F0);

	ufs_sys_writel(host->mphy_base, 0xC0, 0x120);
	ufs_sys_writel(host->mphy_base, 0xC0, 0x260);

	ufs_sys_writel(host->mphy_base, 0x03, 0x094);

	ufs_sys_writel(host->mphy_base, 0x03, 0x1B4);
	ufs_sys_writel(host->mphy_base, 0x03, 0x2F4);

	ufs_sys_writel(host->mphy_base, 0xC0, 0x08C);
	udelay(1);
	ufs_sys_writel(host->mphy_base, 0x00, 0x08C);

	/*
	 * If the phy parameters are modified after linkup, the delay time
	 * needs to be increased
	 */
	udelay(500);

	return 0;
}

static int ufs_rockchip_rk3576_phy_init(struct ufs_hba *hba)
{
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(PA_LOCAL_TX_LCC_ENABLE, 0x0), 0x0);
	/* enable the mphy DME_SET cfg */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x200, 0x0), 0x40);
	for (int i = 0; i < 2; i++) {
		/* Configuration M-TX */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xaa, SEL_TX_LANE0 + i), 0x06);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xa9, SEL_TX_LANE0 + i), 0x02);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xad, SEL_TX_LANE0 + i), 0x44);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xac, SEL_TX_LANE0 + i), 0xe6);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xab, SEL_TX_LANE0 + i), 0x07);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x94, SEL_TX_LANE0 + i), 0x93);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x93, SEL_TX_LANE0 + i), 0xc9);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x7f, SEL_TX_LANE0 + i), 0x00);
		/* Configuration M-RX */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x12, SEL_RX_LANE0 + i), 0x06);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x11, SEL_RX_LANE0 + i), 0x00);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x1d, SEL_RX_LANE0 + i), 0x58);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x1c, SEL_RX_LANE0 + i), 0x8c);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x1b, SEL_RX_LANE0 + i), 0x02);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x25, SEL_RX_LANE0 + i), 0xf6);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x2f, SEL_RX_LANE0 + i), 0x69);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x1e, SEL_RX_LANE0 + i), 0x18);
	}

	/* disable the mphy DME_SET cfg */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x200, 0x0), 0x00);

	/* start link up */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(MIB_T_DBG_CPORT_TX_ENDIAN, 0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(MIB_T_DBG_CPORT_RX_ENDIAN, 0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(N_DEVICEID, 0), 0x0);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(N_DEVICEID_VALID, 0), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(T_PEERDEVICEID, 0), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(T_CONNECTIONSTATE, 0), 0x1);

	return 0;
}

static int ufs_rockchip_common_init(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct ufs_rockchip_host *host = dev_get_priv(dev);
	struct resource res;
	int err = 0;

	/* system control register for hci */
	err = dev_read_resource_byname(dev, "hci_grf", &res);
	if (err) {
		dev_err(dev, "cannot ioremap for hci system control register\n");
		return -ENODEV;
	}
	host->ufs_sys_ctrl = (void *)(res.start);

	/* system control register for mphy */
	err = dev_read_resource_byname(dev, "mphy_grf", &res);
	if (err) {
		dev_err(dev, "cannot ioremap for mphy system control register\n");
		return -ENODEV;
	}
	host->ufs_phy_ctrl = (void *)(res.start);

	/* mphy base register */
	err = dev_read_resource_byname(dev, "mphy", &res);
	if (err) {
		dev_err(dev, "cannot ioremap for mphy base register\n");
		return -ENODEV;
	}
	host->mphy_base = (void *)(res.start);

	host->phy_config_mode = dev_read_u32_default(dev, "ufs-phy-config-mode", 0);

	err = reset_get_bulk(dev, &host->rsts);
	if (err) {
		dev_err(dev, "Can't get reset: %d\n", err);
		return err;
	}

	host->hba = hba;

	return 0;
}

__weak void rk_board_ufs_reset_device(void) { ; }

static int ufs_rockchip_init(struct ufs_hba *hba)
{
	struct udevice *dev = hba->dev;
	struct ufs_rockchip_host *host = dev_get_priv(dev);
	int ret = 0;

	ret = ufs_rockchip_common_init(hba);
	if (ret) {
		dev_err(hba->dev, "%s: ufs common init fail\n", __func__);
		return ret;
	}

	rk_board_ufs_reset_device();

	/* Reset ufs controller */
	reset_assert_bulk(&host->rsts);
	udelay(20);
	reset_deassert_bulk(&host->rsts);
	udelay(20);

	return 0;
}

static struct ufs_hba_ops ufs_hba_rk3576_vops = {
	.init = ufs_rockchip_init,
	.phy_initialization = ufs_rockchip_rk3576_phy_init,
	.hce_enable_notify = ufs_rockchip_hce_enable_notify,
	.link_startup_notify = ufs_rockchip_startup_notify,
	.phy_parameter_initialization = ufs_rockchip_rk3576_phy_parameter_init,
};

static const struct udevice_id ufs_rockchip_of_match[] = {
	{ .compatible = "rockchip,rk3576-ufs", .data = (ulong)&ufs_hba_rk3576_vops},
	{},
};

static int ufs_rockchip_probe(struct udevice *dev)
{
	struct ufs_hba_ops *ops = (struct ufs_hba_ops *)dev_get_driver_data(dev);
	int err;

	err = ufshcd_probe(dev, ops);
	if (err)
		dev_err(dev, "ufshcd_pltfrm_init() failed %d\n", err);

	return err;
}

static int ufs_rockchip_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;

	return ufs_scsi_bind(dev, &scsi_dev);
}

U_BOOT_DRIVER(ti_j721e_ufs) = {
	.name		= "ufshcd-rockchip",
	.id		= UCLASS_UFS,
	.of_match	= ufs_rockchip_of_match,
	.probe		= ufs_rockchip_probe,
	.bind		= ufs_rockchip_bind,
	.priv_auto	= sizeof(struct ufs_rockchip_host),
};
