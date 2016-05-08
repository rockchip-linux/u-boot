/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <sdhci.h>
#include <asm/arch/rkplat.h>

/* 400KHz is max freq for card ID etc. Use that as min */
#define EMMC_MIN_FREQ	400000

int rk_sdhci_init(u32 regbase, u32 emmc_freq)
{
	struct sdhci_host *host = NULL;

	host = (struct sdhci_host *)malloc(sizeof(struct sdhci_host));
	if (!host) {
		printf("rk_sdhci_init: sdhci_host malloc fail\n");
		return -1;
	}

	host->name = "rk_sdhci";
	host->ioaddr = (void *)(unsigned long)regbase;
	host->version = sdhci_readw(host, SDHCI_HOST_VERSION);
	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;

	add_sdhci(host, emmc_freq, EMMC_MIN_FREQ);

	return 0;
}
