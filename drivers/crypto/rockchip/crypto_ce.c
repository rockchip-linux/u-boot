// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd
 * Author: Troy Lin <troy.lin@rock-chips.com>
 */

#include <asm/io.h>
#include <common.h>
#include <clk.h>
#include <clk-uclass.h>
#include <crypto_manager.h>
#include <dm.h>
#include <linux/delay.h>
#include <rockchip/rkce_core.h>
#include <rockchip/crypto_v2_pka.h>

#define CRYPTO_CE_DRIVER_NAME	"rk_crypto_ce"

#define ROUNDUP(size, alignment)	round_up(size, alignment)

#define RKCE_HASH_TIMEOUT_MS	1000
#define RKCE_SYMM_TIMEOUT_MS	1000

struct rkce_sha_contex {
	enum HASH_ALGO			algo;
	u32				length;

	struct rkce_hash_td_ctrl	ctrl;
	struct rkce_hash_td		*td;
	struct rkce_hash_td_buf		*td_buf;
};

struct rockchip_crypto_plat {
	void __iomem	*base;
	u32		*clocks;
	u32		*frequencies;
	u32		nclocks;
	u32		freq_nclocks;
};

struct rockchip_crypto_priv {
	u32		length;

	void		*hardware;
};

void __iomem *crypto_base;

static int rk_crypto_set_clk(struct udevice *dev, int enable)
{
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);
	struct clk clk;
	int i, ret;

	memset(&clk, 0x00, sizeof(clk));

	for (i = 0; i < plat->nclocks; i++) {
		ret = clk_get_by_index(dev, i, &clk);
		if (ret < 0) {
			printf("Failed to get clk index %d, ret=%d\n", i, ret);
			return ret;
		}

		if (enable)
			ret = clk_enable(&clk);
		else
			ret = clk_disable(&clk);
		if (ret < 0 && ret != -ENOSYS) {
			printf("Failed to enable(%d) clk(%ld): ret=%d\n",
			       enable, clk.id, ret);
			return ret;
		}
	}

	return 0;
}

static int rk_crypto_enable_clk(struct udevice *dev)
{
	return rk_crypto_set_clk(dev, 1);
}

static int rk_crypto_disable_clk(struct udevice *dev)
{
	return rk_crypto_set_clk(dev, 0);
}

static int rockchip_crypto_of_to_plat(struct udevice *dev)
{
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);
	int len, ret = -EINVAL;

	memset(plat, 0x00, sizeof(*plat));

	plat->base = dev_read_addr_ptr(dev);
	if (!plat->base)
		return -EINVAL;

	crypto_base = plat->base;

	/* if there is no clocks in dts, just skip it */
	if (!dev_read_prop(dev, "clocks", &len)) {
		printf("Can't find \"clocks\" property\n");
		return 0;
	}

	memset(plat, 0x00, sizeof(*plat));
	plat->clocks = malloc(len);
	if (!plat->clocks)
		return -ENOMEM;

	plat->nclocks = len / (2 * sizeof(u32));

	if (dev_read_u32_array(dev, "clocks", plat->clocks,
			       plat->nclocks * 2)) {
		printf("Can't read \"clocks\" property\n");
		ret = -EINVAL;
		goto exit;
	}

	if (dev_read_prop(dev, "clock-frequency", &len)) {
		plat->frequencies = malloc(len);
		if (!plat->frequencies) {
			ret = -ENOMEM;
			goto exit;
		}

		plat->freq_nclocks = len / sizeof(u32);
		if (dev_read_u32_array(dev, "clock-frequency", plat->frequencies,
				       plat->freq_nclocks)) {
			printf("Can't read \"clock-frequency\" property\n");
			ret = -EINVAL;
			goto exit;
		}
	}

	return 0;
exit:
	if (plat->clocks)
		free(plat->clocks);

	if (plat->frequencies)
		free(plat->frequencies);

	return ret;
}

static int rk_crypto_clk_init(struct udevice *dev)
{
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);
	struct clk clk;
	int i, ret;

	/* use standard "assigned-clock-rates" props */
	if (dev_read_size(dev, "assigned-clock-rates") > 0)
		return clk_set_defaults(dev, 0);

	/* use "clock-frequency" props */
	if (plat->freq_nclocks == 0 || plat->nclocks == 0)
		return 0;

	memset(&clk, 0x00, sizeof(clk));

	for (i = 0; i < plat->nclocks; i++) {
		ret = clk_get_by_index(dev, i, &clk);
		if (ret < 0) {
			printf("Failed to get clk id %d, ret=%d\n", plat->clocks[2 * i + 1], ret);
			return ret;
		}

		ret = clk_set_rate(&clk, plat->frequencies[i]);
		if (ret < 0) {
			printf("%s: Failed to set clk(%ld): ret=%d\n",
			       __func__, clk.id, ret);
			return ret;
		}
	}

	return 0;
}

static void crypto_flush_cacheline(ulong addr, ulong size)
{
	ulong alignment = CONFIG_SYS_CACHELINE_SIZE;
	ulong aligned_input, aligned_len;

	if (!addr || !size)
		return;

	/* Must flush dcache before crypto DMA fetch data region */
	aligned_input = round_down(addr, alignment);
	aligned_len = round_up(size + (addr - aligned_input), alignment);
	flush_cache(aligned_input, aligned_len);
}

static void crypto_invalidate_cacheline(uint32_t addr, uint32_t size)
{
	ulong alignment = CONFIG_SYS_CACHELINE_SIZE;
	ulong aligned_input, aligned_len;

	if (!addr || !size)
		return;

	/* Must invalidate dcache after crypto DMA write data region */
	aligned_input = round_down(addr, alignment);
	aligned_len = round_up(size + (addr - aligned_input), alignment);
	invalidate_dcache_range(aligned_input, aligned_input + aligned_len);
}

static void rk_crypto_soft_reset(struct udevice *dev, uint32_t reset_sel)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);

	if (!priv->hardware)
		return;

	rk_crypto_enable_clk(dev);

	rkce_soft_reset(priv->hardware, reset_sel);

	rk_crypto_disable_clk(dev);
}

static u32 rk_hash_get_cemode(u32 algo)
{
	const u32 hash_bitmap[HASH_ALGO_NUM] = {
		[HASH_ALGO_MD5]    = RKCE_HASH_ALGO_MD5,
		[HASH_ALGO_SHA1]   = RKCE_HASH_ALGO_SHA1,
		[HASH_ALGO_SHA256] = RKCE_HASH_ALGO_SHA256,
		[HASH_ALGO_SHA384] = RKCE_HASH_ALGO_SHA384,
		[HASH_ALGO_SHA512] = RKCE_HASH_ALGO_SHA512,
	};

	if (algo >= HASH_ALGO_NUM)
		return ~((u32)0);

	return hash_bitmap[algo];
}

static u32 rk_hash_get_disgest_size(enum HASH_ALGO algo)
{
	switch (algo) {
	case HASH_ALGO_MD5:
		return 16;
	case HASH_ALGO_SHA1:
		return 20;
	case HASH_ALGO_SHA256:
		return 32;
	case HASH_ALGO_SHA384:
		return 48;
	case HASH_ALGO_SHA512:
		return 64;
	default:
		return 0; /* Invalid algorithm */
	}
}

static bool hash_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);

	if (mode != CRYPTO_MODE_NONE || algo >= HASH_ALGO_NUM)
		return false;

	if (!dev || !priv || !priv->hardware)
		return false;

	return rkce_hw_algo_valid(priv->hardware, RKCE_ALGO_TYPE_HASH,
				  rk_hash_get_cemode(algo), 0);
}

static void *rkce_sha_ctx_alloc(void)
{
	struct rkce_sha_contex *hw_ctx;

	hw_ctx = malloc(sizeof(*hw_ctx));
	if (!hw_ctx)
		return NULL;

	memset(hw_ctx, 0x00, sizeof(*hw_ctx));

	hw_ctx->td = rkce_cma_alloc(sizeof(struct rkce_hash_td));
	if (!hw_ctx->td)
		goto error;

	memset(hw_ctx->td, 0x00, sizeof(struct rkce_hash_td));

	hw_ctx->td_buf = rkce_cma_alloc(sizeof(struct rkce_hash_td_buf));
	if (!hw_ctx->td_buf)
		goto error;

	memset(hw_ctx->td_buf, 0x00, sizeof(struct rkce_hash_td_buf));

	return hw_ctx;
error:
	rkce_cma_free(hw_ctx->td);
	rkce_cma_free(hw_ctx->td_buf);
	free(hw_ctx);

	return NULL;
}

static void rkce_sha_ctx_free(struct rkce_sha_contex *hw_ctx)
{
	if (!hw_ctx)
		return;

	rkce_cma_free(hw_ctx->td);
	rkce_cma_free(hw_ctx->td_buf);
	free(hw_ctx);
}

static int rk_hash_init(struct udevice *dev, enum HASH_ALGO algo, void **ctx)
{
	struct rkce_sha_contex *hash_ctx = NULL;
	u32 ce_algo = 0;
	int ret = 0;

	if (!dev || algo >= HASH_ALGO_NUM || !ctx)
		return -EINVAL;

	rk_crypto_soft_reset(dev, RKCE_RESET_HASH);

	hash_ctx = rkce_sha_ctx_alloc();
	if (!hash_ctx)
		return -ENOMEM;

	hash_ctx->algo = algo;

	ret = rkce_init_hash_td(hash_ctx->td, hash_ctx->td_buf);
	if (ret)
		goto exit;

	ce_algo = rk_hash_get_cemode(algo);

	hash_ctx->ctrl.td_type        = RKCE_TD_TYPE_HASH;
	hash_ctx->ctrl.hw_pad_en      = 1;
	hash_ctx->ctrl.first_pkg      = 1;
	hash_ctx->ctrl.last_pkg       = 0;
	hash_ctx->ctrl.hash_algo      = ce_algo;
	hash_ctx->ctrl.hmac_en        = 0;
	hash_ctx->ctrl.is_preemptible = 0;
	hash_ctx->ctrl.int_en         = 1;

	*ctx = hash_ctx;

exit:
	if (ret && hash_ctx) {
		rkce_sha_ctx_free(hash_ctx);
		*ctx = NULL;
	}

	return ret;
}

static int rk_hash_update(struct udevice *dev, void *ctx, const void *input, const u32 len)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rkce_sha_contex *hash_ctx = NULL;
	struct rkce_hash_td *td = NULL;
	bool is_last = (len == 0);
	int ret = 0;

	if (len && !input)
		return -EINVAL;

	if (!dev || !ctx)
		return -EINVAL;

	hash_ctx = (struct rkce_sha_contex *)ctx;
	if (!hash_ctx || !hash_ctx->td || !hash_ctx->td_buf ||
	    hash_ctx->ctrl.td_type != RKCE_TD_TYPE_HASH) {
		printf("Invalid hash context\n");
		return -EINVAL;
	}

	td = hash_ctx->td;

	td->ctrl = hash_ctx->ctrl;
	memset(&td->sg, 0x00, sizeof(*td->sg));

	if (hash_ctx->ctrl.first_pkg == 1)
		hash_ctx->ctrl.first_pkg = 0;

	if (is_last) {
		td->ctrl.last_pkg = 1;
	} else {
#ifdef CONFIG_ARM64
		td->sg[0].src_addr_h = rkce_cma_virt2phys(input) >> 32;
#endif
		td->sg[0].src_addr_l = rkce_cma_virt2phys(input) & 0xffffffff;
		td->sg[0].src_size   = len;
		hash_ctx->length += len;
		crypto_flush_cacheline((ulong)input, len);
	}

	rk_crypto_enable_clk(dev);

	crypto_flush_cacheline((ulong)hash_ctx->td, sizeof(*hash_ctx->td));
	crypto_flush_cacheline((ulong)hash_ctx->td_buf, sizeof(*hash_ctx->td_buf));

	ret = rkce_push_td_sync(priv->hardware, td, RKCE_HASH_TIMEOUT_MS);
	if (ret) {
		printf("rkce_push_td_sync failed: %d\n", ret);
		goto exit;
	}

	crypto_invalidate_cacheline((ulong)hash_ctx->td_buf, sizeof(*hash_ctx->td_buf));

exit:
	rk_crypto_disable_clk(dev);

	return ret;
}

static int rk_hash_finish(struct udevice *dev, void *ctx, void *digest)
{
	struct rkce_sha_contex *hash_ctx = ctx;
	int ret;

	ret = rk_hash_update(dev, ctx, NULL, 0);
	if (ret == 0)
		memcpy(digest, hash_ctx->td_buf->hash, rk_hash_get_disgest_size(hash_ctx->algo));

	rkce_sha_ctx_free(hash_ctx);

	return ret;
}

static int rockchip_crypto_probe(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);
	int ret = 0;

	ret = rk_crypto_clk_init(dev);
	if (ret)
		return ret;

	rk_crypto_enable_clk(dev);

	priv->hardware = rkce_hardware_alloc((void *)plat->base);

	if (!priv->hardware) {
		ret = -ENOMEM;
		goto exit;
	}

exit:
	rk_crypto_disable_clk(dev);

	return 0;
}

static const struct udevice_id rockchip_crypto_ids[] = {
	{
		.compatible = "rockchip,crypto-ce",
	},
	{ }
};

static struct crypto_impl rk_crypto_hash_impl = {
	.type        = CRYPTO_TYPE_HASH,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = hash_check_valid,

	.hash.hash_init   = rk_hash_init,
	.hash.hash_update = rk_hash_update,
	.hash.hash_finish = rk_hash_finish,
};

#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)

static int rk_mod_exp(struct udevice *dev, const uint8_t *sig, uint32_t sig_len,
		      struct key_prop *prop, uint8_t *out)
{
	struct mpa_num *mpa_m = NULL, *mpa_e = NULL;;
	struct mpa_num *mpa_n = NULL, *mpa_result = NULL;
	u32 n_words, n_bytes;
	int ret;

	if (!dev || !sig || !prop || !out || sig_len != prop->num_bits / 8)
		return -EINVAL;

	n_words = prop->num_bits / 32;
	n_bytes = prop->num_bits / 8;

	ret = rk_mpa_alloc(&mpa_m, (void *)sig, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&mpa_e, NULL, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&mpa_n, (void *)prop->modulus, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&mpa_result, NULL, n_words);
	if (ret)
		goto exit;

	/* mpa need little endian data */
	util_reverse_buff((void *)mpa_m->d, n_bytes);
	util_reverse_buff((void *)mpa_n->d, n_bytes);
	util_reverse_memcpy((void *)mpa_e->d, prop->public_exponent, prop->exp_len);

	rk_crypto_enable_clk(dev);
	ret = rk_exptmod_np(mpa_m, mpa_e, mpa_n, NULL, mpa_result);
	if (!ret)
		util_reverse_memcpy(out, (void *)mpa_result->d, n_bytes);

	rk_crypto_disable_clk(dev);

exit:
	rk_mpa_free(&mpa_m);
	rk_mpa_free(&mpa_e);
	rk_mpa_free(&mpa_n);
	rk_mpa_free(&mpa_result);

	return ret;
}

static bool rk_asym_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	const u32 asym_bitmap[ASYM_ALGO_NUM] = {
		[ASYM_ALGO_RSA] = RKCE_ASYM_ALGO_RSA,
		[ASYM_ALGO_ECC] = RKCE_ASYM_ALGO_ECC_P256,
		[ASYM_ALGO_SM2] = RKCE_ASYM_ALGO_SM2,
	};

	if (mode != CRYPTO_MODE_NONE || algo >= ASYM_ALGO_NUM)
		return false;

	if (!dev || !priv || !priv->hardware)
		return false;

	return rkce_hw_algo_valid(priv->hardware, RKCE_ALGO_TYPE_ASYM, asym_bitmap[algo], 0);
}

static struct crypto_impl rk_mod_exp_impl = {
	.type        = CRYPTO_TYPE_ASYM,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = rk_asym_check_valid,

	.asym.rsa.mod_exp = rk_mod_exp,
};

#endif

static int rockchip_crypto_bind(struct udevice *dev)
{
	int ret = 0;

	rk_crypto_hash_impl.dev = dev;

	ret = crypto_impl_register(&rk_crypto_hash_impl);
	if (ret) {
		printf("crypto_impl_register rk_crypto_hash_impl failed.\n");
		goto exit;
	}

#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)

	rk_mod_exp_impl.dev = dev;

	ret = crypto_impl_register(&rk_mod_exp_impl);
	if (ret) {
		printf("crypto_impl_register rk_mod_exp_impl failed.\n");
		goto exit;
	}
#endif

exit:
	return ret;
}

U_BOOT_DRIVER(rk_crypto_ce) = {
	.name       = CRYPTO_CE_DRIVER_NAME,
	.id         = UCLASS_MISC,
	.of_match   = rockchip_crypto_ids,
	.bind       = rockchip_crypto_bind,
	.probe      = rockchip_crypto_probe,
	.of_to_plat = rockchip_crypto_of_to_plat,
	.priv_auto  = sizeof(struct rockchip_crypto_priv),
	.plat_auto  = sizeof(struct rockchip_crypto_plat),
};