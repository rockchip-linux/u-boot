// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014 Freescale Semiconductor, Inc.
 * Author: Ruchika Gupta <ruchika.gupta@freescale.com>
 */

#include <config.h>
#include <crypto_manager.h>
#include <dm.h>
#include <log.h>
#include <u-boot/rsa-mod-exp.h>

static int mod_exp_sw(struct udevice *dev, const uint8_t *sig, uint32_t sig_len,
		      struct key_prop *prop, uint8_t *out)
{
	int ret = 0;

	ret = rsa_mod_exp_sw(sig, sig_len, prop, out);
	if (ret) {
		debug("%s: RSA failed to verify: %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

#if CONFIG_IS_ENABLED(CRYPTO_MANAGER)
static bool sw_mod_exp_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	if (mode != CRYPTO_MODE_NONE)
		return false;

	if (algo != ASYM_ALGO_RSA)
		return false;

	return true;
}

static struct crypto_impl sw_mod_exp_impl = {
	.type        = CRYPTO_TYPE_ASYM,
	.uclass_id   = UCLASS_MOD_EXP,
	.priority    = CRYPTO_PRIORITY_SW,
	.check_valid = sw_mod_exp_check_valid,
	.asym.rsa.mod_exp = mod_exp_sw,
};

static int sw_mod_exp_bind(struct udevice *dev)
{
	sw_mod_exp_impl.dev = dev;

	return crypto_impl_register(&sw_mod_exp_impl);
}
#else
static int sw_mod_exp_bind(struct udevice *dev)
{
	return 0;
}
#endif

static const struct mod_exp_ops mod_exp_ops_sw = {
	.mod_exp	= mod_exp_sw,
};

U_BOOT_DRIVER(mod_exp_sw) = {
	.name	= "mod_exp_sw",
	.id	= UCLASS_MOD_EXP,
	.ops	= &mod_exp_ops_sw,
	.bind   = sw_mod_exp_bind,
	.flags	= DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(mod_exp_sw) = {
	.name = "mod_exp_sw",
};
