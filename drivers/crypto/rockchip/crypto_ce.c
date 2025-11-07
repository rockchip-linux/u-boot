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
#include <rockchip/crypto_ecc.h>
#include <rockchip/crypto_v2_pka.h>

#define CRYPTO_DRIVER_NAME	"rk_crypto_ce"

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

struct rkce_cipher_contex {
	struct rkce_symm_td		*td;
	struct rkce_symm_td		*td_aad;
	struct rkce_symm_td_buf		*td_buf;
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

static int rk_get_cipher_cemode(u32 algo, u32 mode, u32 *ce_algo, u32 *ce_mode)
{
	const u32 rk_cipher_mode_map[] = {
		[CIPHER_MODE_ECB]     =RKCE_SYMM_MODE_ECB,
		[CIPHER_MODE_CBC]     =RKCE_SYMM_MODE_CBC,
		[CIPHER_MODE_CTS]     =RKCE_SYMM_MODE_CTS,
		[CIPHER_MODE_CTR]     =RKCE_SYMM_MODE_CTR,
		[CIPHER_MODE_CFB]     =RKCE_SYMM_MODE_CFB,
		[CIPHER_MODE_OFB]     =RKCE_SYMM_MODE_OFB,
		[CIPHER_MODE_XTS]     =RKCE_SYMM_MODE_XTS,
		[CIPHER_MODE_CCM]     =RKCE_SYMM_MODE_CCM,
		[CIPHER_MODE_GCM]     =RKCE_SYMM_MODE_GCM,
		[CIPHER_MODE_CMAC]    =RKCE_SYMM_MODE_CMAC,
		[CIPHER_MODE_CBC_MAC] = RKCE_SYMM_MODE_CBC_MAC,
	};

	switch (algo) {
	case CIPHER_ALGO_DES:
		*ce_algo = RKCE_SYMM_ALGO_TDES;
		break;
	case CIPHER_ALGO_AES:
		*ce_algo = RKCE_SYMM_ALGO_AES;
		break;
	case CIPHER_ALGO_SM4:
		*ce_algo = RKCE_SYMM_ALGO_SM4;
		break;
	default:
		return -EINVAL;
	}

	if (mode >= CIPHER_MODE_NUM)
		return -EINVAL;

	*ce_mode = rk_cipher_mode_map[mode];

	return 0;
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

static bool cipher_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	u32 ce_algo = 0, ce_mode = 0;
	int ret;

	if ( algo >= CIPHER_ALGO_NUM)
		return false;

	if (mode != CRYPTO_MODE_NONE && mode >= CIPHER_MODE_NUM)
		return false;

	if ( algo >= CIPHER_ALGO_NUM)
		return false;

	if (!dev || !priv || !priv->hardware)
		return false;

	ret = rk_get_cipher_cemode(algo, mode, &ce_algo, &ce_mode);
	if (ret)
		return false;

	return rkce_hw_algo_valid(priv->hardware, RKCE_ALGO_TYPE_CIPHER,
				  ce_algo, ce_mode);
}

static struct crypto_impl rk_crypto_cipher_impl = {
	.name        = "cipher_"CRYPTO_DRIVER_NAME,
	.type        = CRYPTO_TYPE_CIPHER,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = cipher_check_valid,

	.cipher.cipher_crypt = rockchip_crypto_cipher,
	.cipher.cipher_mac   = rockchip_crypto_mac,
	.cipher.cipher_ae    = rockchip_crypto_ae,
};

#endif


static int rockchip_crypto_probe(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);
	int ret = 0;

	rk_crypto_enable_clk(dev);

	priv->hardware = rkce_hardware_alloc((void *)plat->base);

	if (!priv->hardware) {
		ret = -ENOMEM;
		goto exit;
	}

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

static struct crypto_impl rk_crypto_hash_impl = {
	.name        = "hash_"CRYPTO_DRIVER_NAME,
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
	.name        = "mod_exp_"CRYPTO_DRIVER_NAME,
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

#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)

	rk_crypto_cipher_impl.dev = dev;

	ret = crypto_impl_register(&rk_crypto_cipher_impl);
	if (ret) {
		printf("crypto_impl_register rk_crypto_cipher_impl failed.\n");
		goto exit;
	}
#endif

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
	.name       = CRYPTO_DRIVER_NAME,
	.id         = UCLASS_MISC,
	.of_match   = rockchip_crypto_ids,
	.bind       = rockchip_crypto_bind,
	.probe      = rockchip_crypto_probe,
	.of_to_plat = rockchip_crypto_of_to_plat,
	.priv_auto  = sizeof(struct rockchip_crypto_priv),
	.plat_auto  = sizeof(struct rockchip_crypto_plat),
};