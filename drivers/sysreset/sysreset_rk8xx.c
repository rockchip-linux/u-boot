// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd
 *
 */

#include <dm.h>
#include <sysreset.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <power/pmic.h>
#include <power/rk8xx_pmic.h>
#include <time.h>

/*
 * By default, the PMIC performs a full power cycle during reset.
 * To prevent the loss of the reboot mode, it now needs to be changed
 * to perform a power reset only.
 */
static int rk8xx_sysreset_request_prepare(struct udevice *dev, const char *mode)
{
	struct udevice *pmic_dev = dev->parent;
	struct rk8xx_priv *priv = dev_get_priv(pmic_dev);
	int powon_sts, start_timestamp;
	u8 value;

	/* Configure PMIC reset behavior before system reset */
	switch (priv->variant) {
	case RK805_ID:
		/* RK805B specific reset configuration */
		if (priv->vsel_table) {
			value = pmic_reg_read(pmic_dev, RK805B_PMIC_SYS_CFG2);
			value &= RK8xx_RESET_FUN_CLR;
			value |= (RK8xx_RST_MODE1 << 6);
			pmic_reg_write(pmic_dev, RK805B_PMIC_SYS_CFG2, value);
		}
		break;
	case RK809_ID:
	case RK817_ID:
		/*
		 * Poll the RK817_PMIC_SYS_STS register, waiting for the power key to be
		 * released (RK817_PWRON_STS bit set). The polling interval is 2ms,
		 * and the loop continues until the condition is met. When the time exceeds
		 * 12 seconds, the PMIC will force a power-down.
		 */
		start_timestamp = get_timer(0);
		while (1) {
			powon_sts = pmic_reg_read(pmic_dev, RK817_PMIC_SYS_STS);
			if (powon_sts >= 0 && ((powon_sts & RK817_PWRON_STS) == 0))
				mdelay(2);
			else
				break;
			/* Print status message every second while waiting for power key release */
			if (get_timer(start_timestamp) > 1000) {
				start_timestamp = get_timer(0);
				printf("sysreset: rk%x waiting for power key release\n", priv->variant);
			}
		}
		/* RK809/RK817 common reset configuration */
		value = pmic_reg_read(pmic_dev, RK817_PMIC_SYS_CFG3);
		value &=  RK8xx_RESET_FUN_CLR;
		value |= (RK8xx_RST_MODE1 << 6);
		pmic_reg_write(pmic_dev, RK817_PMIC_SYS_CFG3, value);
		break;
	default:
		break;
	}

	return 0;
}

static const struct sysreset_ops sysreset_ops_rk8xx = {
	.request_prepare = rk8xx_sysreset_request_prepare,
};

U_BOOT_DRIVER(sysreset_rk8xx) = {
	.name = "sysreset-rk8xx",
	.id = UCLASS_SYSRESET,
	.ops = &sysreset_ops_rk8xx,
};
