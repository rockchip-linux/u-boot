// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017 Masahiro Yamada <yamada.masahiro@socionext.com>
 */

#include <dm.h>
#include <sysreset.h>
#include <linux/errno.h>
#include <linux/psci.h>

__weak int psci_sysreset_get_status(struct udevice *dev, char *buf, int size)
{
	return -EOPNOTSUPP;
}

static int psci_sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	switch (type) {
	case SYSRESET_WARM:
	case SYSRESET_COLD:
		printf("PSCI: sysreset %s\n",
		       type == SYSRESET_WARM ? "warm" : "cold");
		psci_sys_reset(type);
		break;
	case SYSRESET_POWER_OFF:
		psci_sys_poweroff();
		break;
	default:
		return -EPROTONOSUPPORT;
	}

	return -EINPROGRESS;
}

static struct sysreset_ops psci_sysreset_ops = {
	.request = psci_sysreset_request,
	.get_status = psci_sysreset_get_status,
};

/* Add an 'a_' prefix so it comes the first sysreset path. */
U_BOOT_DRIVER(a_psci_sysreset) = {
	.name = "psci-sysreset",
	.id = UCLASS_SYSRESET,
	.ops = &psci_sysreset_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
