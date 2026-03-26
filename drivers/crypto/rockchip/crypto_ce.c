// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */

#include <clk.h>
#include <clk-uclass.h>
#include <common.h>
#include <crypto.h>
#include <dm.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>
#include <rockchip/crypto_ecc.h>
#include <rockchip/crypto_v2_pka.h>
#include <rockchip/rkce_core.h>

fdt_addr_t crypto_base;

#define ROUNDUP(size, alignment)	round_up(size, alignment)

#define RKCE_HASH_TIMEOUT_MS	1000
#define RKCE_SYMM_TIMEOUT_MS	1000

struct rkce_sha_contex {
	u32				length;
	struct rkce_hash_td_ctrl	ctrl;
	struct rkce_hash_td		*td;
	struct rkce_hash_td_buf		*td_buf;
};

struct rkce_cipher_contex {
	struct rkce_symm_td		*td;
	struct rkce_symm_td		*td_aad;
	struct rkce_symm_td_buf		*td_buf;
};

struct rockchip_crypto_priv {
	fdt_addr_t			reg;
	u32				frequency;
	char				*clocks;
	u32				*frequencies;
	u32				nclocks;
	u32				freq_nclocks;
	u32				capability;

	void				*hardware;
	struct rkce_sha_contex		*hash_ctx;
	u16				secure;
	u16				enabled;
};

struct rockchip_map {
	u32				crypto;
	u32				rkce;
};

static const struct rockchip_map rk_hash_map[] = {
	{CRYPTO_SM3,         RKCE_HASH_ALGO_SM3},
	{CRYPTO_MD5,         RKCE_HASH_ALGO_MD5},
	{CRYPTO_SHA1,        RKCE_HASH_ALGO_SHA1},
	{CRYPTO_SHA256,      RKCE_HASH_ALGO_SHA256},
	{CRYPTO_SHA512,      RKCE_HASH_ALGO_SHA512},
};

#if CONFIG_IS_ENABLED(ROCKCHIP_HMAC)
static const struct rockchip_map rk_hmac_map[] = {
	{CRYPTO_HMAC_MD5,    RKCE_HASH_ALGO_MD5},
	{CRYPTO_HMAC_SHA1,   RKCE_HASH_ALGO_SHA1},
	{CRYPTO_HMAC_SHA256, RKCE_HASH_ALGO_SHA256},
	{CRYPTO_HMAC_SHA512, RKCE_HASH_ALGO_SHA512},
	{CRYPTO_HMAC_SM3,    RKCE_HASH_ALGO_SM3},
};
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)
static const struct rockchip_map rk_cipher_map[] = {
	{CRYPTO_AES,         RKCE_SYMM_ALGO_AES},
	{CRYPTO_DES,         RKCE_SYMM_ALGO_TDES},
	{CRYPTO_SM4,         RKCE_SYMM_ALGO_SM4},
};
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
static const struct rockchip_map rk_rsa_map[] = {
	{CRYPTO_RSA512,       RKCE_ASYM_ALGO_RSA},
	{CRYPTO_RSA1024,      RKCE_ASYM_ALGO_RSA},
	{CRYPTO_RSA2048,      RKCE_ASYM_ALGO_RSA},
	{CRYPTO_RSA3072,      RKCE_ASYM_ALGO_RSA},
	{CRYPTO_RSA4096,      RKCE_ASYM_ALGO_RSA},
};
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_EC)
static const struct rockchip_map rk_ec_map[] = {
	{CRYPTO_SM2,          RKCE_ASYM_ALGO_SM2},
	{CRYPTO_ECC_192R1,    RKCE_ASYM_ALGO_ECC_P192},
	{CRYPTO_ECC_224R1,    RKCE_ASYM_ALGO_ECC_P224},
	{CRYPTO_ECC_256R1,    RKCE_ASYM_ALGO_ECC_P256},
};
#endif

static int rk_crypto_enable_clk(struct udevice *dev);
static int rk_crypto_disable_clk(struct udevice *dev);

static void rk_crypto_soft_reset(struct udevice *dev, uint32_t reset_sel)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);

	if (!priv->hardware)
		return;

	rk_crypto_enable_clk(dev);

	rkce_soft_reset(priv->hardware, reset_sel);

	rk_crypto_disable_clk(dev);
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

static u32 rk_get_cemode(const struct rockchip_map *map, u32 num, u32 algo)
{
	u32 i, j;
	struct {
		const struct rockchip_map	*map;
		u32				num;
	} map_tbl[] = {
		{rk_hash_map, ARRAY_SIZE(rk_hash_map)},
#if CONFIG_IS_ENABLED(ROCKCHIP_HMAC)
		{rk_hmac_map, ARRAY_SIZE(rk_hmac_map)},
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)
		{rk_cipher_map, ARRAY_SIZE(rk_cipher_map)},
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
		{rk_rsa_map, ARRAY_SIZE(rk_rsa_map)},
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_EC)
		{rk_ec_map, ARRAY_SIZE(rk_ec_map)},
#endif
	};

	for (i = 0; i < ARRAY_SIZE(map_tbl); i++) {
		const struct rockchip_map *map = map_tbl[i].map;
		u32 num = map_tbl[i].num;

		for (j = 0; j < num; j++) {
			if (map[j].crypto == algo)
				return map[j].rkce;
		}
	}

	return 0;
}

static u32 rk_load_map(struct rockchip_crypto_priv *priv, u32 algo_type,
		       const struct rockchip_map *map, u32 num)
{
	u32 i;
	u32 capability = 0;

	for (i = 0; i < num; i++) {
		if (rkce_hw_algo_valid(priv->hardware, algo_type, map[i].rkce, 0))
			capability |= map[i].crypto;
	}

	return capability;
}

static u32 rockchip_crypto_capability(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	u32 cap = 0;

	if (!priv->enabled)
		return 0;

	if (priv->capability)
		return priv->capability;

	cap |= rk_load_map(priv, RKCE_ALGO_TYPE_HASH,
			   rk_hash_map, ARRAY_SIZE(rk_hash_map));

#if CONFIG_IS_ENABLED(ROCKCHIP_HMAC)
	cap |= rk_load_map(priv, RKCE_ALGO_TYPE_HMAC,
			   rk_hmac_map, ARRAY_SIZE(rk_hmac_map));
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)
	cap |= rk_load_map(priv, RKCE_ALGO_TYPE_CIPHER,
			   rk_cipher_map, ARRAY_SIZE(rk_cipher_map));
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
	cap |= rk_load_map(priv, RKCE_ALGO_TYPE_ASYM, rk_rsa_map,
			   ARRAY_SIZE(rk_rsa_map));
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_EC)
	cap |= rk_load_map(priv, RKCE_ALGO_TYPE_ASYM, rk_ec_map,
			   ARRAY_SIZE(rk_ec_map));
#endif

	return cap;
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

static int rk_sha_init(struct udevice *dev, sha_context *ctx,
		       u8 *key, u32 key_len, bool is_hmac)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rkce_sha_contex *hash_ctx = NULL;
	u32 ce_algo = 0;
	int ret = 0;

	if ((ctx->algo & priv->capability) == 0)
		return -ENOSYS;

	if (priv->hash_ctx)
		return -EFAULT;

	rk_crypto_soft_reset(dev, RKCE_RESET_HASH);

	hash_ctx = rkce_sha_ctx_alloc();
	if (!hash_ctx)
		return -ENOMEM;

	ret = rkce_init_hash_td(hash_ctx->td, hash_ctx->td_buf);
	if (ret)
		goto exit;

	ce_algo = rk_get_cemode(rk_hash_map, ARRAY_SIZE(rk_hash_map), ctx->algo);

	hash_ctx->ctrl.td_type        = RKCE_TD_TYPE_HASH;
	hash_ctx->ctrl.hw_pad_en      = 1;
	hash_ctx->ctrl.first_pkg      = 1;
	hash_ctx->ctrl.last_pkg       = 0;
	hash_ctx->ctrl.hash_algo      = ce_algo;
	hash_ctx->ctrl.hmac_en        = is_hmac;
	hash_ctx->ctrl.is_preemptible = 0;
	hash_ctx->ctrl.int_en         = 1;

	if (is_hmac) {
		if (key_len > 64) {
			ret = -EINVAL;
			goto exit;
		}

		memcpy(hash_ctx->td_buf->key, key, key_len);
	}

	priv->hash_ctx = hash_ctx;
exit:
	if (ret) {
		rkce_sha_ctx_free(hash_ctx);
		priv->hash_ctx = NULL;
	}

	return ret;
}

static int rk_sha_update(struct udevice *dev, u32 *input, u32 len, bool is_last)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rkce_sha_contex *hash_ctx;
	struct rkce_hash_td *td;
	int ret = 0;

	if (!priv->hash_ctx)
		return -EINVAL;

	if (!is_last && (!input || len == 0))
		return -EINVAL;

	hash_ctx = priv->hash_ctx;
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
		rkce_sha_ctx_free(hash_ctx);
		priv->hash_ctx = NULL;
	}

	crypto_invalidate_cacheline((ulong)hash_ctx->td_buf, sizeof(*hash_ctx->td_buf));

	rk_crypto_disable_clk(dev);

	return ret;
}

static int rockchip_crypto_sha_init(struct udevice *dev, sha_context *ctx)
{
	return rk_sha_init(dev, ctx, NULL, 0, false);
}

static int rockchip_crypto_sha_update(struct udevice *dev, u32 *input, u32 len)
{
	return rk_sha_update(dev, input, len, false);
}

static int rockchip_crypto_sha_final(struct udevice *dev, sha_context *ctx, u8 *output)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rkce_sha_contex *hash_ctx = priv->hash_ctx;
	u32 nbits;
	int ret;

	if (!priv->hash_ctx)
		return -EINVAL;

	nbits = crypto_algo_nbits(ctx->algo);

	if (hash_ctx->length != ctx->length) {
		printf("total length(0x%08x) != init length(0x%08x)!\n",
		       hash_ctx->length, ctx->length);
		ret = -EIO;
		goto exit;
	}

	ret = rk_sha_update(dev, NULL, 0, true);
	if (ret == 0)
		memcpy(output, hash_ctx->td_buf->hash, BITS2BYTE(nbits));

exit:
	rkce_sha_ctx_free(hash_ctx);
	priv->hash_ctx = NULL;

	return ret;
}

#if CONFIG_IS_ENABLED(ROCKCHIP_HMAC)
static int rockchip_crypto_hmac_init(struct udevice *dev,  sha_context *ctx, u8 *key, u32 key_len)
{
	return rk_sha_init(dev, ctx, key, key_len, true);
}

static int rockchip_crypto_hmac_update(struct udevice *dev, u32 *input, u32 len)
{
	return rockchip_crypto_sha_update(dev, input, len);
}

static int rockchip_crypto_hmac_final(struct udevice *dev, sha_context *ctx, u8 *output)
{
	return rockchip_crypto_sha_final(dev, ctx, output);
}
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)

static int hw_crypto_ccm128_setiv(u8 *iv_buf, u8 *nonce, u32 nlen, u32 mlen)
{
	u32 L = iv_buf[0] & 7;	/* the L parameter */

	if (nlen < (14 - L))
		return -1;	/* nonce is too short */

	if (sizeof(mlen) == 8 && L >= 3) {
		iv_buf[8]  = mlen >> (56 % (sizeof(mlen) * 8));
		iv_buf[9]  = mlen >> (48 % (sizeof(mlen) * 8));
		iv_buf[10] = mlen >> (40 % (sizeof(mlen) * 8));
		iv_buf[11] = mlen >> (32 % (sizeof(mlen) * 8));
	}

	iv_buf[12] = mlen >> 24;
	iv_buf[13] = mlen >> 16;
	iv_buf[14] = mlen >> 8;
	iv_buf[15] = mlen;

	iv_buf[0] &= ~0x40;	/* clear aad flag */
	memcpy(&iv_buf[1], nonce, 14 - L);

	return 0;
}

static void hw_get_ccm_aad_padding(u32 aad_len, u8 *padding,  u32 *padding_size)
{
	u32 i = 0;

	if (aad_len == 0) {
		*padding_size = 0;
		return;
	}

	if (aad_len < (0x10000 - 0x100)) {
		i = 2;
	} else if (sizeof(aad_len) == 8 &&
		   aad_len >= (size_t)1 << (32 % (sizeof(aad_len) * 8))) {
		i = 10;
	} else {
		i = 6;
	}

	if (i == 2) {
		padding[0] = aad_len >> 8;
		padding[1] = aad_len;
	} else if (i == 10) {
		padding[0] = 0xFF;
		padding[1] = 0xFF;
		padding[2] = aad_len >> (56 % (sizeof(aad_len) * 8));
		padding[3] = aad_len >> (48 % (sizeof(aad_len) * 8));
		padding[4] = aad_len >> (40 % (sizeof(aad_len) * 8));
		padding[5] = aad_len >> (32 % (sizeof(aad_len) * 8));
		padding[6] = aad_len >> 24;
		padding[7] = aad_len >> 16;
		padding[8] = aad_len >> 8;
		padding[9] = aad_len;
	} else {
		padding[0] = 0xFF;
		padding[1] = 0xFE;
		padding[2] = aad_len >> 24;
		padding[3] = aad_len >> 16;
		padding[4] = aad_len >> 8;
	}

	*padding_size = i;
}

static int hw_compose_ccm_aad_iv(u8 *aad_iv, u32 data_len,
				 u32 aad_len, u32 tag_size)
{
	u32 L;		/* the L parameter */
	u8 nonce[AES_BLOCK_SIZE];

	L = aad_iv[0] & 7;
	aad_iv[0] |= ((u8)(((tag_size - 2) / 2) & 7) << 3);

	if (sizeof(data_len) == 8 && L >= 3) {
		aad_iv[8]  = data_len >> (56 % (sizeof(data_len) * 8));
		aad_iv[9]  = data_len >> (48 % (sizeof(data_len) * 8));
		aad_iv[10] = data_len >> (40 % (sizeof(data_len) * 8));
		aad_iv[11] = data_len >> (32 % (sizeof(data_len) * 8));
	}

	/* save nonce */
	memcpy(nonce, &aad_iv[1], 14 - L);

	aad_iv[12] = data_len >> 24;
	aad_iv[13] = data_len >> 16;
	aad_iv[14] = data_len >> 8;
	aad_iv[15] = data_len;

	/* restore nonce */
	memcpy(&aad_iv[1], nonce, 14 - L);

	aad_iv[0] &= ~0x40;	/* clear Adata flag */

	if (aad_len)
		aad_iv[0] |= 0x40;	//set aad flag

	return 0;
}

static void rkce_destroy_ccm_aad(u8 *new_aad)
{
	rkce_cma_free(new_aad);
}

static int rkce_build_ccm_aad(const u8 *aad, u32 aad_len, u32 data_len,
			      u8 *iv, u32 iv_len,
			      u8 **new_aad, u32 *new_aad_len,
			      u8 *new_iv, u32 *new_iv_len)
{
	int ret = -RKCE_INVAL;
	u32 L;
	u8 nonce[AES_BLOCK_SIZE];
	u8 pad[AES_BLOCK_SIZE];
	u32 pad_size = 0;
	u32 tag_len = AES_BLOCK_SIZE;
	u8 *aad_tmp = NULL;
	u32 aad_tmp_len = 0;

	memset(nonce, 0x00, sizeof(nonce));

	L = 15 - iv_len;
	nonce[0] = (L - 1) & 7;
	ret = hw_crypto_ccm128_setiv(nonce, (u8 *)iv, iv_len, 0);
	if (ret)
		return ret;

	memcpy(new_iv, nonce, sizeof(nonce));
	*new_iv_len = sizeof(nonce);

	memset(pad, 0x00, sizeof(pad));
	hw_get_ccm_aad_padding(aad_len, pad, &pad_size);

	aad_tmp_len = aad_len + AES_BLOCK_SIZE + pad_size;
	aad_tmp_len = ROUNDUP(aad_tmp_len, AES_BLOCK_SIZE);

	aad_tmp = rkce_cma_alloc(aad_tmp_len);
	if (!aad_tmp) {
		ret = -RKCE_NOMEM;
		goto exit;
	}

	/* clear last block */
	memset(aad_tmp + aad_tmp_len - AES_BLOCK_SIZE, 0x00, AES_BLOCK_SIZE);
	memcpy(aad_tmp, nonce, sizeof(nonce));
	hw_compose_ccm_aad_iv(aad_tmp, data_len, aad_len, tag_len);
	memcpy(aad_tmp + AES_BLOCK_SIZE, pad, pad_size);

	memcpy(aad_tmp + AES_BLOCK_SIZE + pad_size, aad, aad_len);

	*new_aad     = aad_tmp;
	*new_aad_len = aad_tmp_len;

exit:
	return ret;
}

static void *rkce_cipher_ctx_alloc(void)
{
	struct rkce_cipher_contex *hw_ctx;

	hw_ctx = malloc(sizeof(*hw_ctx));
	if (!hw_ctx)
		return NULL;

	hw_ctx->td = rkce_cma_alloc(sizeof(struct rkce_symm_td));
	if (!hw_ctx->td)
		goto error;

	memset(hw_ctx->td, 0x00, sizeof(struct rkce_symm_td));

	hw_ctx->td_aad = rkce_cma_alloc(sizeof(struct rkce_symm_td));
	if (!hw_ctx->td_aad)
		goto error;

	memset(hw_ctx->td_aad, 0x00, sizeof(struct rkce_symm_td));

	hw_ctx->td_buf = rkce_cma_alloc(sizeof(struct rkce_symm_td_buf));
	if (!hw_ctx->td_buf)
		goto error;

	memset(hw_ctx->td_buf, 0x00, sizeof(struct rkce_symm_td_buf));

	return hw_ctx;
error:
	rkce_cma_free(hw_ctx->td);
	rkce_cma_free(hw_ctx->td_aad);
	rkce_cma_free(hw_ctx->td_buf);
	free(hw_ctx);

	return NULL;
}

static void rkce_cipher_ctx_free(struct rkce_cipher_contex *hw_ctx)
{
	if (!hw_ctx)
		return;

	rkce_cma_free(hw_ctx->td);
	rkce_cma_free(hw_ctx->td_aad);
	rkce_cma_free(hw_ctx->td_buf);
	free(hw_ctx);
}

static const struct rockchip_map rk_cipher_algo_map[] = {
	{RK_MODE_ECB,     RKCE_SYMM_MODE_ECB},
	{RK_MODE_CBC,     RKCE_SYMM_MODE_CBC},
	{RK_MODE_CTS,     RKCE_SYMM_MODE_CTS},
	{RK_MODE_CTR,     RKCE_SYMM_MODE_CTR},
	{RK_MODE_CFB,     RKCE_SYMM_MODE_CFB},
	{RK_MODE_OFB,     RKCE_SYMM_MODE_OFB},
	{RK_MODE_XTS,     RKCE_SYMM_MODE_XTS},
	{RK_MODE_CCM,     RKCE_SYMM_MODE_CCM},
	{RK_MODE_GCM,     RKCE_SYMM_MODE_GCM},
	{RK_MODE_CMAC,    RKCE_SYMM_MODE_CMAC},
	{RK_MODE_CBC_MAC, RKCE_SYMM_MODE_CBC_MAC},
};

static int rk_get_cipher_cemode(u32 algo, u32 mode, u32 *ce_algo, u32 *ce_mode)
{
	u32 i;

	switch (algo) {
	case CRYPTO_DES:
		*ce_algo = RKCE_SYMM_ALGO_TDES;
		break;
	case CRYPTO_AES:
		*ce_algo = RKCE_SYMM_ALGO_AES;
		break;
	case CRYPTO_SM4:
		*ce_algo = RKCE_SYMM_ALGO_SM4;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(rk_cipher_algo_map); i++) {
		if (mode == rk_cipher_algo_map[i].crypto) {
			*ce_mode = rk_cipher_algo_map[i].rkce;
			return 0;
		}
	}

	return -EINVAL;
}

u32 rk_get_td_keysize(u32 ce_algo, u32 ce_mode,  u32 key_len)
{
	u32 key_size = 0;

	if (ce_algo == RKCE_SYMM_ALGO_AES) {
		if (key_len == AES_KEYSIZE_128)
			key_size = RKCE_KEY_AES_128;
		else if (key_len == AES_KEYSIZE_192)
			key_size = RKCE_KEY_AES_192;
		else if (key_len == AES_KEYSIZE_256)
			key_size = RKCE_KEY_AES_256;
		else
			;
	}

	return key_size;
}

int rk_set_symm_td_buf_key(struct rkce_symm_td_buf *td_buf,
			   u32 ce_algo, u32 ce_mode, cipher_context *ctx)
{
	memset(td_buf->key1, 0x00, sizeof(td_buf->key1));
	memset(td_buf->key2, 0x00, sizeof(td_buf->key2));

	if (ce_mode == RKCE_SYMM_MODE_XTS) {
		memcpy(td_buf->key1, ctx->key, ctx->key_len);
		memcpy(td_buf->key2, ctx->twk_key, ctx->key_len);
	} else {
		memcpy(td_buf->key1, ctx->key, ctx->key_len);
	}

	if (ctx->key_len == DES_KEYSIZE * 2 &&
	    (ce_algo == RKCE_SYMM_ALGO_DES || ce_algo == RKCE_SYMM_ALGO_TDES))
		memcpy(td_buf->key1 + DES_KEYSIZE * 2, td_buf->key1, DES_KEYSIZE);

	return 0;
}

int rk_set_symm_td_sg(struct rkce_symm_td *td,
		      const u8 *in, u32 in_len, u8 *out, u32 out_len)
{
	memset(td->sg, 0x00, sizeof(td->sg));

#ifdef CONFIG_ARM64
	td->sg[0].src_addr_h = rkce_cma_virt2phys(in) >> 32;
#endif
	td->sg[0].src_addr_l = rkce_cma_virt2phys(in) & 0xffffffff;
	td->sg[0].src_size   = in_len;

	if (out && out_len) {
#ifdef CONFIG_ARM64
		td->sg[0].dst_addr_h = rkce_cma_virt2phys(out) >> 32;
#endif
		td->sg[0].dst_addr_l = rkce_cma_virt2phys(out) & 0xffffffff;
		td->sg[0].dst_size   = out_len;
	}

	td->next_task = 0;

	return 0;
}

static int rk_crypto_cipher(struct udevice *dev, cipher_context *ctx,
			    const u8 *in, u8 *out, u32 len, bool enc,
			    const u8 *aad, u32 aad_len, u8 *tag)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rkce_cipher_contex *hw_ctx = NULL;
	u32 ce_algo = 0, ce_mode = 0;
	bool use_otpkey = false;
	int ret = 0;

	rk_crypto_soft_reset(dev, RKCE_RESET_SYMM);

	if (!ctx->key && ctx->key_len)
		use_otpkey = true;

	ret = rk_get_cipher_cemode(ctx->algo, ctx->mode, &ce_algo, &ce_mode);
	if (ret)
		return ret;

	hw_ctx = rkce_cipher_ctx_alloc();
	if (!hw_ctx)
		return -ENOMEM;

	rkce_init_symm_td(hw_ctx->td, hw_ctx->td_buf);

	hw_ctx->td->ctrl.td_type   = RKCE_TD_TYPE_SYMM;
	hw_ctx->td->ctrl.is_dec    = !enc;
	hw_ctx->td->ctrl.symm_algo = ce_algo;
	hw_ctx->td->ctrl.symm_mode = ce_mode;
	hw_ctx->td->ctrl.key_size  = rk_get_td_keysize(ce_algo, ce_mode, ctx->key_len);
	hw_ctx->td->ctrl.first_pkg = 1;
	hw_ctx->td->ctrl.last_pkg  = 1;
	hw_ctx->td->ctrl.int_en    = 1;
	hw_ctx->td->ctrl.key_sel   = use_otpkey ? RKCE_KEY_SEL_KT : RKCE_KEY_SEL_USER;

	memcpy(hw_ctx->td_buf->iv, ctx->iv, ctx->iv_len);
	hw_ctx->td->ctrl.iv_len    = ctx->iv_len;

	if (!use_otpkey) {
		ret = rk_set_symm_td_buf_key(hw_ctx->td_buf, ce_algo, ce_mode, ctx);
		if (ret)
			goto exit;
	}

	ret = rk_set_symm_td_sg(hw_ctx->td, in, len, out, len);
	if (ret)
		goto exit;

	if (ce_mode == RKCE_SYMM_MODE_CCM) {
		u8 *new_aad = NULL;
		u32 new_aad_len = 0, new_iv_len = 0;

		rkce_init_symm_td(hw_ctx->td_aad, hw_ctx->td_buf);

		ret = rkce_build_ccm_aad(aad, aad_len, len,
					 hw_ctx->td_buf->iv, ctx->iv_len,
					 &new_aad, &new_aad_len,
					 hw_ctx->td_buf->iv, &new_iv_len);
		if (ret)
			goto exit;

		ret = rk_set_symm_td_sg(hw_ctx->td_aad, new_aad, new_aad_len, NULL, 0);
		if (ret)
			goto exit;

		hw_ctx->td->ctrl.iv_len = new_iv_len;

		hw_ctx->td_buf->gcm_len.aad_len_l = new_aad_len;

		hw_ctx->td_aad->ctrl = hw_ctx->td->ctrl;
		hw_ctx->td_aad->ctrl.is_aad = 1;

		crypto_flush_cacheline((ulong)hw_ctx->td_aad, sizeof(*hw_ctx->td_aad));
		crypto_flush_cacheline((ulong)hw_ctx->td_buf, sizeof(*hw_ctx->td_buf));
		crypto_flush_cacheline((ulong)new_aad, new_aad_len);

		rk_crypto_enable_clk(dev);

		ret = rkce_push_td_sync(priv->hardware, hw_ctx->td_aad, RKCE_SYMM_TIMEOUT_MS);

		crypto_invalidate_cacheline((ulong)hw_ctx->td_buf, sizeof(*hw_ctx->td_buf));

		rk_crypto_disable_clk(dev);

		rkce_destroy_ccm_aad(new_aad);

		if (ret) {
			printf("CCM calc aad data failed.\n");
			goto exit;
		}
	} else if (ce_mode == RKCE_SYMM_MODE_GCM) {
		rkce_init_symm_td(hw_ctx->td_aad, hw_ctx->td_buf);

		ret = rk_set_symm_td_sg(hw_ctx->td_aad, aad, aad_len, NULL, 0);
		if (ret)
			goto exit;

		hw_ctx->td_buf->gcm_len.aad_len_l = aad_len;
		hw_ctx->td_buf->gcm_len.pc_len_l = len;

		hw_ctx->td_aad->ctrl = hw_ctx->td->ctrl;
		hw_ctx->td_aad->ctrl.is_aad = 1;

		crypto_flush_cacheline((ulong)hw_ctx->td_aad, sizeof(*hw_ctx->td_aad));
		crypto_flush_cacheline((ulong)hw_ctx->td_buf, sizeof(*hw_ctx->td_buf));
		crypto_flush_cacheline((ulong)aad, aad_len);

		rk_crypto_enable_clk(dev);

		ret = rkce_push_td_sync(priv->hardware, hw_ctx->td_aad, RKCE_SYMM_TIMEOUT_MS);

		crypto_invalidate_cacheline((ulong)hw_ctx->td_buf, sizeof(*hw_ctx->td_buf));

		rk_crypto_disable_clk(dev);
		if (ret) {
			printf("GCM calc aad data failed.\n");
			goto exit;
		}
	}

	crypto_flush_cacheline((ulong)hw_ctx->td, sizeof(*hw_ctx->td));
	crypto_flush_cacheline((ulong)hw_ctx->td_buf, sizeof(*hw_ctx->td_buf));
	crypto_flush_cacheline((ulong)in, len);
	if (in != out)
		crypto_flush_cacheline((ulong)out, len);

	rk_crypto_enable_clk(dev);

	ret = rkce_push_td_sync(priv->hardware, hw_ctx->td, RKCE_SYMM_TIMEOUT_MS);

	crypto_invalidate_cacheline((ulong)out, len);
	crypto_invalidate_cacheline((ulong)hw_ctx->td_buf, sizeof(*hw_ctx->td_buf));

	rk_crypto_disable_clk(dev);

	if (tag)
		memcpy(tag, hw_ctx->td_buf->tag, sizeof(hw_ctx->td_buf->tag));
exit:
	rkce_cipher_ctx_free(hw_ctx);

	return ret;
}

static int rockchip_crypto_cipher(struct udevice *dev, cipher_context *ctx,
				  const u8 *in, u8 *out, u32 len, bool enc)
{
	return rk_crypto_cipher(dev, ctx, in, out, len, enc, NULL, 0, NULL);
}

static int rockchip_crypto_mac(struct udevice *dev, cipher_context *ctx,
			       const u8 *in, u32 len, u8 *tag)
{
	return rk_crypto_cipher(dev, ctx, in, NULL, len, true, NULL, 0, tag);
}

static int rockchip_crypto_ae(struct udevice *dev, cipher_context *ctx,
			      const u8 *in, u32 len, const u8 *aad, u32 aad_len,
			      u8 *out, u8 *tag)
{
	return rk_crypto_cipher(dev, ctx, in, out, len, true, aad, aad_len, tag);
}

#if CONFIG_IS_ENABLED(DM_KEYLAD)
int rockchip_crypto_fw_cipher(struct udevice *dev, cipher_fw_context *ctx,
			      const u8 *in, u8 *out, u32 len, bool enc)
{
	cipher_context cipher_ctx;

	memset(&cipher_ctx, 0x00, sizeof(cipher_ctx));

	cipher_ctx.algo    = ctx->algo;
	cipher_ctx.mode    = ctx->mode;
	cipher_ctx.key_len = ctx->key_len;
	cipher_ctx.iv      = ctx->iv;
	cipher_ctx.iv_len  = ctx->iv_len;

	return rk_crypto_cipher(dev, &cipher_ctx, in, out, len, enc, NULL, 0, NULL);
}

static ulong rockchip_get_keytable_addr(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	ulong addr;

	rk_crypto_enable_clk(dev);

	addr = rkce_get_keytable_addr(priv->hardware);

	rk_crypto_disable_clk(dev);

	return addr;
}
#endif

#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
static int rockchip_crypto_rsa_verify(struct udevice *dev, rsa_key *ctx,
				      u8 *sign, u8 *output)
{
	struct mpa_num *mpa_m = NULL, *mpa_e = NULL, *mpa_n = NULL;
	struct mpa_num *mpa_c = NULL, *mpa_result = NULL;
	u32 n_bits, n_words;
	int ret;

	if (!ctx)
		return -EINVAL;

	if (ctx->algo != CRYPTO_RSA512 &&
	    ctx->algo != CRYPTO_RSA1024 &&
	    ctx->algo != CRYPTO_RSA2048 &&
	    ctx->algo != CRYPTO_RSA3072 &&
	    ctx->algo != CRYPTO_RSA4096)
		return -EINVAL;

	n_bits = crypto_algo_nbits(ctx->algo);
	n_words = BITS2WORD(n_bits);

	ret = rk_mpa_alloc(&mpa_m, sign, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&mpa_e, ctx->e, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&mpa_n, ctx->n, n_words);
	if (ret)
		goto exit;

	if (ctx->c) {
		ret = rk_mpa_alloc(&mpa_c, ctx->c, n_words);
		if (ret)
			goto exit;
	}

	ret = rk_mpa_alloc(&mpa_result, NULL, n_words);
	if (ret)
		goto exit;

	rk_crypto_enable_clk(dev);
	ret = rk_exptmod_np(mpa_m, mpa_e, mpa_n, mpa_c, mpa_result);
	if (!ret)
		memcpy(output, mpa_result->d, BITS2BYTE(n_bits));
	rk_crypto_disable_clk(dev);

exit:
	rk_mpa_free(&mpa_m);
	rk_mpa_free(&mpa_e);
	rk_mpa_free(&mpa_n);
	rk_mpa_free(&mpa_c);
	rk_mpa_free(&mpa_result);

	return ret;
}
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_EC)
static int rockchip_crypto_ec_verify(struct udevice *dev, ec_key *ctx,
				     u8 *hash, u32 hash_len, u8 *sign)
{
	struct mpa_num *bn_sign = NULL;
	struct rk_ecp_point point_P, point_sign;
	u32 n_bits, n_words;
	int ret;

	if (!ctx)
		return -EINVAL;

	if (ctx->algo != CRYPTO_SM2 &&
	    ctx->algo != CRYPTO_ECC_192R1 &&
	    ctx->algo != CRYPTO_ECC_224R1 &&
	    ctx->algo != CRYPTO_ECC_256R1)
		return -EINVAL;

	n_bits = crypto_algo_nbits(ctx->algo);
	n_words = BITS2WORD(n_bits);

	ret = rk_mpa_alloc(&bn_sign, sign, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&point_P.x, ctx->x, n_words);
	ret |= rk_mpa_alloc(&point_P.y, ctx->y, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&point_sign.x, sign, n_words);
	ret |= rk_mpa_alloc(&point_sign.y, sign + WORD2BYTE(n_words), n_words);
	if (ret)
		goto exit;

	rk_crypto_enable_clk(dev);
	ret = rockchip_ecc_verify(ctx->algo, hash, hash_len, &point_P, &point_sign);
	rk_crypto_disable_clk(dev);
exit:
	rk_mpa_free(&bn_sign);
	rk_mpa_free(&point_P.x);
	rk_mpa_free(&point_P.y);
	rk_mpa_free(&point_sign.x);
	rk_mpa_free(&point_sign.y);

	return ret;
}
#endif

static bool rockchip_crypto_is_secure(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);

	return priv->secure;
}

static const struct dm_crypto_ops rockchip_crypto_ops = {
	.capability   = rockchip_crypto_capability,
	.sha_init     = rockchip_crypto_sha_init,
	.sha_update   = rockchip_crypto_sha_update,
	.sha_final    = rockchip_crypto_sha_final,
#if CONFIG_IS_ENABLED(ROCKCHIP_HMAC)
	.hmac_init    = rockchip_crypto_hmac_init,
	.hmac_update  = rockchip_crypto_hmac_update,
	.hmac_final   = rockchip_crypto_hmac_final,
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
	.rsa_verify   = rockchip_crypto_rsa_verify,
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_EC)
	.ec_verify    = rockchip_crypto_ec_verify,
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)
	.cipher_crypt    = rockchip_crypto_cipher,
	.cipher_mac      = rockchip_crypto_mac,
	.cipher_ae       = rockchip_crypto_ae,

#if CONFIG_IS_ENABLED(DM_KEYLAD)
	.cipher_fw_crypt  = rockchip_crypto_fw_cipher,
	.cipher_otp_crypt = rockchip_crypto_fw_cipher,
	.keytable_addr    = rockchip_get_keytable_addr,
#endif

#endif

	.is_secure       = rockchip_crypto_is_secure,
};

/*
 * Only use "clocks" to parse crypto clock id and use rockchip_get_clk().
 * Because we always add crypto node in U-Boot dts, when kernel dtb enabled :
 *
 *   1. There is cru phandle mismatch between U-Boot and kernel dtb;
 *   2. CONFIG_OF_SPL_REMOVE_PROPS removes clock property;
 */
static int rockchip_crypto_ofdata_to_platdata(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	int len, ret = -EINVAL;

	memset(priv, 0x00, sizeof(*priv));

	priv->reg = (fdt_addr_t)dev_read_addr_ptr(dev);
	if (priv->reg == FDT_ADDR_T_NONE)
		return -EINVAL;

	crypto_base = priv->reg;

	priv->secure = dev_read_bool(dev, "secure");
	priv->enabled = true;

#if !defined(CONFIG_SPL_BUILD)
	/* uboot disabled secure crypto */
	priv->enabled = !priv->secure;
#endif
	if (!priv->enabled)
		return 0;

	/* if there is no clocks in dts, just skip it */
	if (!dev_read_prop(dev, "clocks", &len)) {
		printf("Can't find \"clocks\" property\n");
		return 0;
	}

	priv->clocks = malloc(len);
	if (!priv->clocks)
		return -ENOMEM;

	priv->nclocks = len / (2 * sizeof(u32));
	if (dev_read_u32_array(dev, "clocks", (u32 *)priv->clocks,
			       priv->nclocks)) {
		printf("Can't read \"clocks\" property\n");
		ret = -EINVAL;
		goto exit;
	}

	if (dev_read_prop(dev, "clock-frequency", &len)) {
		priv->frequencies = malloc(len);
		if (!priv->frequencies) {
			ret = -ENOMEM;
			goto exit;
		}
		priv->freq_nclocks = len / sizeof(u32);
		if (dev_read_u32_array(dev, "clock-frequency", priv->frequencies,
				       priv->freq_nclocks)) {
			printf("Can't read \"clock-frequency\" property\n");
			ret = -EINVAL;
			goto exit;
		}
	}

	return 0;
exit:
	if (priv->clocks)
		free(priv->clocks);

	if (priv->frequencies)
		free(priv->frequencies);

	return ret;
}

static int rk_crypto_do_enable_clk(struct udevice *dev, int enable)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct clk clk;
	int i, ret;

	for (i = 0; i < priv->nclocks; i++) {
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
			debug("Failed to enable(%d) clk(%ld): ret=%d\n",
			       enable, clk.id, ret);
			return ret;
		}
	}

	return 0;
}

static int rk_crypto_enable_clk(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);

	crypto_base = priv->reg;

	return rk_crypto_do_enable_clk(dev, 1);
}

static int rk_crypto_disable_clk(struct udevice *dev)
{
	crypto_base = 0;

	return rk_crypto_do_enable_clk(dev, 0);
}

static int rk_crypto_set_clk(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct clk clk;
	int i, ret;

	/* use standard "assigned-clock-rates" props */
	if (dev_read_size(dev, "assigned-clock-rates") > 0)
		return clk_set_defaults(dev);

	/* use "clock-frequency" props */
	if (priv->freq_nclocks == 0)
		return 0;

	for (i = 0; i < priv->freq_nclocks; i++) {
		ret = clk_get_by_index(dev, i, &clk);
		if (ret < 0) {
			printf("Failed to get clk index %d, ret=%d\n", i, ret);
			return ret;
		}
		ret = clk_set_rate(&clk, priv->frequencies[i]);
		if (ret < 0) {
			printf("%s: Failed to set clk(%ld): ret=%d\n",
			       __func__, clk.id, ret);
			return ret;
		}
	}

	return 0;
}

static int rockchip_crypto_probe(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	int ret = 0;

	if (!priv->enabled)
		return 0;

	ret = rk_crypto_set_clk(dev);
	if (ret)
		return ret;

	rk_crypto_enable_clk(dev);

	priv->hardware = rkce_hardware_alloc((void *)priv->reg);

	if (!priv->hardware) {
		ret = -ENOMEM;
		goto exit;
	}

	priv->capability = rockchip_crypto_capability(dev);
exit:
	rk_crypto_disable_clk(dev);

	return ret;
}

static const struct udevice_id rockchip_crypto_ids[] = {
	{
		.compatible = "rockchip,crypto-ce",
	},
	{ }
};

U_BOOT_DRIVER(rockchip_crypto_ce) = {
	.name		= "rockchip_crypto_ce",
	.id		= UCLASS_CRYPTO,
	.of_match	= rockchip_crypto_ids,
	.ops		= &rockchip_crypto_ops,
	.probe		= rockchip_crypto_probe,
	.ofdata_to_platdata = rockchip_crypto_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct rockchip_crypto_priv),
};
