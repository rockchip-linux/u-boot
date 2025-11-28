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
#include <rockchip/crypto_ecc.h>
#include <rockchip/crypto_hash_cache.h>
#include <rockchip/crypto_v2.h>
#include <rockchip/crypto_v2_pka.h>

//#define DEBUG

#ifdef DEBUG
#define DMSG(format, ...) printf("[%s, %05d]-trace: " format "\n", \
				 __func__, __LINE__, ##__VA_ARGS__)
#define IMSG(format, ...) printf(format "\n", ##__VA_ARGS__)
#else
#define DMSG(format, ...)
#define IMSG(format, ...)
#endif

#define	RK_HASH_CTX_MAGIC	0x1A1A1A1A
#define CRYPTO_DRIVER_NAME	"rk_crypto_v2"

#define LLI_ADDR_ALIGN_SIZE	8
#define DATA_ADDR_ALIGN_SIZE	8
#define DATA_LEN_ALIGN_SIZE	64

/* crypto timeout 500ms, must support more than 32M data per times*/
#define HASH_UPDATE_LIMIT	(32 * 1024 * 1024)
#define RK_CRYPTO_TIMEOUT	500000

#define RK_POLL_TIMEOUT(condition, timeout) \
({ \
	int time_out = timeout; \
	while (condition) { \
		if (--time_out <= 0) { \
			debug("[%s] %d: time out!\n", __func__,\
				__LINE__); \
			break; \
		} \
		udelay(1); \
	} \
	(time_out <= 0) ? -ETIMEDOUT : 0; \
})

#define WAIT_TAG_VALID(channel, timeout) ({ \
	u32 tag_mask = CRYPTO_CH0_TAG_VALID << (channel);\
	int ret;\
	ret = RK_POLL_TIMEOUT(!(crypto_read(CRYPTO_TAG_VALID) & tag_mask),\
			      timeout);\
	crypto_write(crypto_read(CRYPTO_TAG_VALID) & tag_mask, CRYPTO_TAG_VALID);\
	ret;\
})

#define virt_to_phys(addr)		(((unsigned long)addr) & 0xffffffff)
#define phys_to_virt(addr, area)	((unsigned long)addr)

#define align_malloc(bytes, alignment)	memalign(alignment, bytes)
#define align_free(addr)		do {if (addr) free(addr);} while (0)

#define ROUNDUP(size, alignment)	round_up(size, alignment)
#define cache_op_inner(type, addr, size) \
					crypto_flush_cacheline((ulong)addr, size)

#define IS_NEED_IV(rk_mode) ((rk_mode) != CIPHER_MODE_ECB && \
			     (rk_mode) != CIPHER_MODE_CMAC && \
			     (rk_mode) != CIPHER_MODE_CBC_MAC)

#define IS_NEED_TAG(rk_mode) ((rk_mode) == CIPHER_MODE_CMAC || \
			      (rk_mode) == CIPHER_MODE_CBC_MAC || \
			      (rk_mode) == CIPHER_MODE_CCM || \
			      (rk_mode) == CIPHER_MODE_GCM)

#define IS_MAC_MODE(rk_mode) ((rk_mode) == CIPHER_MODE_CMAC || \
			      (rk_mode) == CIPHER_MODE_CBC_MAC)

#define IS_AE_MODE(rk_mode) ((rk_mode) == CIPHER_MODE_CCM || \
			     (rk_mode) == CIPHER_MODE_GCM)

struct crypto_lli_desc {
	u32 src_addr;
	u32 src_len;
	u32 dst_addr;
	u32 dst_len;
	u32 user_define;
	u32 reserve;
	u32 dma_ctrl;
	u32 next_addr;
};

struct rk_hash_ctx {
	struct crypto_lli_desc		data_lli;	/* lli desc */
	struct crypto_hash_cache	*hash_cache;
	u32				magic;		/* to check ctx */
	u32				algo;		/* hash algo */
	u8				digest_size;	/* hash out length */
	u8				reserved[2];
	u8				last_chunk_size;
	u8				last_chunk[64];
};

struct rk_crypto_ver {
	u32 aes_ver;
	u32 des_ver;
	u32 sm4_ver;
	u32 hash_ver;
	u32 hmac_ver;
	u32 pka_ver;
	u32 extra_feature;
};

struct rk_crypto_soc_data {
	struct rk_crypto_ver crypto_ver;
	void (*dynamic_ver_init)(struct rk_crypto_ver *ver);
};

struct rockchip_crypto_plat {
	void __iomem			*base;
	u32				*clocks;
	u32				*frequencies;
	u32				nclocks;
	u32				freq_nclocks;
};

struct rockchip_crypto_priv {
	u32				length;
	struct rk_hash_ctx		*hw_ctx;
	struct rk_crypto_soc_data	*soc_data;
	u16 secure;
	u16 enabled;
};

void __iomem *crypto_base;

static inline void word2byte_be(u32 word, u8 *ch)
{
	ch[0] = (word >> 24) & 0xff;
	ch[1] = (word >> 16) & 0xff;
	ch[2] = (word >> 8) & 0xff;
	ch[3] = (word >> 0) & 0xff;
}

static inline u32 byte2word_be(const u8 *ch)
{
	return (*ch << 24) + (*(ch + 1) << 16) + (*(ch + 2) << 8) + *(ch + 3);
}

static inline void clear_regs(u32 base, u32 words)
{
	int i;

	/*clear out register*/
	for (i = 0; i < words; i++)
		crypto_write(0, base + 4 * i);
}

static inline void clear_hash_out_reg(void)
{
	clear_regs(CRYPTO_HASH_DOUT_0, 16);
}

static inline void clear_key_regs(void)
{
	clear_regs(CRYPTO_CH0_KEY_0, CRYPTO_KEY_CHANNEL_NUM * 4);
}

static inline void read_regs(u32 base, u8 *data, u32 data_len)
{
	u8 tmp_buf[4];
	u32 i;

	for (i = 0; i < data_len / 4; i++)
		word2byte_be(crypto_read(base + i * 4),
			     data + i * 4);

	if (data_len % 4) {
		word2byte_be(crypto_read(base + i * 4), tmp_buf);
		memcpy(data + i * 4, tmp_buf, data_len % 4);
	}
}

static inline void write_regs(u32 base, const u8 *data, u32 data_len)
{
	u8 tmp_buf[4];
	u32 i;

	for (i = 0; i < data_len / 4; i++, base += 4)
		crypto_write(byte2word_be(data + i * 4), base);

	if (data_len % 4) {
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
		memcpy((u8 *)tmp_buf, data + i * 4, data_len % 4);
		crypto_write(byte2word_be(tmp_buf), base);
	}
}

static inline void write_key_reg(u32 chn, const u8 *key, u32 key_len)
{
	write_regs(CRYPTO_CH0_KEY_0 + chn * 0x10, key, key_len);
}

static inline void set_iv_reg(u32 chn, const u8 *iv, u32 iv_len)
{
	u32 base_iv;

	base_iv = CRYPTO_CH0_IV_0 + chn * 0x10;

	/* clear iv */
	clear_regs(base_iv, 4);

	if (!iv || iv_len == 0)
		return;

	write_regs(base_iv, iv, iv_len);

	crypto_write(iv_len, CRYPTO_CH0_IV_LEN_0 + 4 * chn);
}

static inline void get_iv_reg(u32 chn, u8 *iv, u32 iv_len)
{
	u32 base_iv;

	base_iv = CRYPTO_CH0_IV_0 + chn * 0x10;

	read_regs(base_iv, iv, iv_len);
}

static inline void get_tag_from_reg(u32 chn, u8 *tag, u32 tag_len)
{
	u32 i;
	u32 chn_base = CRYPTO_CH0_TAG_0 + 0x10 * chn;

	for (i = 0; i < tag_len / 4; i++, chn_base += 4)
		word2byte_be(crypto_read(chn_base), tag + 4 * i);
}

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
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);

	crypto_base = plat->base;

	return rk_crypto_set_clk(dev, 1);
}

static int rk_crypto_disable_clk(struct udevice *dev)
{
	crypto_base = 0;

	return rk_crypto_set_clk(dev, 0);
}

static int hw_crypto_reset(void)
{
	u32 val = 0, mask = 0;
	int ret;

	val = CRYPTO_SW_PKA_RESET | CRYPTO_SW_CC_RESET;
	mask = val << CRYPTO_WRITE_MASK_SHIFT;

	/* reset pka and crypto modules*/
	crypto_write(val | mask, CRYPTO_RST_CTL);

	/* wait reset compelete */
	ret = RK_POLL_TIMEOUT(crypto_read(CRYPTO_RST_CTL), RK_CRYPTO_TIMEOUT);

	return ret;
}

static void hw_hash_clean_ctx(struct rk_hash_ctx *ctx)
{
	/* clear hash status */
	crypto_write(CRYPTO_WRITE_MASK_ALL | 0, CRYPTO_HASH_CTL);

	assert(ctx);
	assert(ctx->magic == RK_HASH_CTX_MAGIC);

	crypto_hash_cache_free(ctx->hash_cache);

	memset(ctx, 0x00, sizeof(*ctx));
}

static int rk_hash_direct_calc(void *hw_data, const u8 *data,
			       u32 data_len, u8 *started_flag, u8 is_last)
{
	struct rockchip_crypto_priv *priv = hw_data;
	struct rk_hash_ctx *hash_ctx = priv->hw_ctx;
	struct crypto_lli_desc *lli = &hash_ctx->data_lli;
	int ret = -EINVAL;
	u32 tmp = 0, mask = 0;

	assert(IS_ALIGNED((ulong)data, DATA_ADDR_ALIGN_SIZE));
	assert(is_last || IS_ALIGNED(data_len, DATA_LEN_ALIGN_SIZE));

	debug("%s: data = %p, len = %u, s = %x, l = %x\n",
	      __func__, data, data_len, *started_flag, is_last);

	memset(lli, 0x00, sizeof(*lli));
	lli->src_addr = (u32)virt_to_phys(data);
	lli->src_len = data_len;
	lli->dma_ctrl = LLI_DMA_CTRL_SRC_DONE;

	if (is_last) {
		lli->user_define |= LLI_USER_STRING_LAST;
		lli->dma_ctrl |= LLI_DMA_CTRL_LAST;
	} else {
		lli->next_addr = (u32)virt_to_phys(lli);
		lli->dma_ctrl |= LLI_DMA_CTRL_PAUSE;
	}

	if (!(*started_flag)) {
		lli->user_define |=
			(LLI_USER_STRING_START | LLI_USER_CIPHER_START);
		crypto_write((u32)virt_to_phys(lli), CRYPTO_DMA_LLI_ADDR);
		crypto_write((CRYPTO_HASH_ENABLE << CRYPTO_WRITE_MASK_SHIFT) |
			     CRYPTO_HASH_ENABLE, CRYPTO_HASH_CTL);
		tmp = CRYPTO_DMA_START;
		*started_flag = 1;
	} else {
		tmp = CRYPTO_DMA_RESTART;
	}

	/* flush cache */
	crypto_flush_cacheline((ulong)lli, sizeof(*lli));
	crypto_flush_cacheline((ulong)data, data_len);

	/* start calculate */
	crypto_write(tmp << CRYPTO_WRITE_MASK_SHIFT | tmp,
		     CRYPTO_DMA_CTL);

	/* mask CRYPTO_SYNC_LOCKSTEP_INT_ST flag */
	mask = ~(mask | CRYPTO_SYNC_LOCKSTEP_INT_ST);

	/* wait calc ok */
	ret = RK_POLL_TIMEOUT(!(crypto_read(CRYPTO_DMA_INT_ST) & mask),
			      RK_CRYPTO_TIMEOUT);

	/* clear interrupt status */
	tmp = crypto_read(CRYPTO_DMA_INT_ST);
	crypto_write(tmp, CRYPTO_DMA_INT_ST);

	if ((tmp & mask) != CRYPTO_SRC_ITEM_DONE_INT_ST &&
	    (tmp & mask) != CRYPTO_ZERO_LEN_INT_ST) {
		ret = -EFAULT;
		debug("[%s] %d: CRYPTO_DMA_INT_ST = 0x%x\n",
		      __func__, __LINE__, tmp);
		goto exit;
	}

	priv->length += data_len;
exit:
	return ret;
}

/*
 * Only use "clocks" to parse crypto clock id and use rockchip_get_clk().
 * Because we always add crypto node in U-Boot dts, when kernel dtb enabled :
 *
 *   1. There is cru phandle mismatch between U-Boot and kernel dtb;
 *   2. CONFIG_OF_SPL_REMOVE_PROPS removes clock property;
 */
static int rockchip_crypto_of_to_plat(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rockchip_crypto_plat *plat = dev_get_plat(dev);
	int len, ret = -EINVAL;

	memset(plat, 0x00, sizeof(*plat));

	plat->base = dev_read_addr_ptr(dev);
	if (!plat->base)
		return -EINVAL;

	priv->secure = dev_read_bool(dev, "secure");
	priv->enabled = true;

#if !defined(CONFIG_SPL_BUILD)
	/* uboot disabled secure crypto */
	priv->enabled = !priv->secure;
#else
	/* spl disabled no-secure crypto */
	priv->enabled = priv->secure;
#endif
	if (!priv->enabled)
		return 0;

	crypto_base = plat->base;

	/* if there is no clocks in dts, just skip it */
	if (!dev_read_prop(dev, "clocks", &len)) {
		printf("Can't find \"clocks\" property\n");
		return 0;
	}

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

static bool hash_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = NULL;

	const u32 hash_bitmap[HASH_ALGO_NUM] = {
		[HASH_ALGO_MD5]    = CRYPTO_HASH_MD5_FLAG,
		[HASH_ALGO_SHA1]   = CRYPTO_HASH_SHA1_FLAG,
		[HASH_ALGO_SHA256] = CRYPTO_HASH_SHA256_FLAG,
		[HASH_ALGO_SHA384] = CRYPTO_HASH_SHA384_FLAG,
		[HASH_ALGO_SHA512] = CRYPTO_HASH_SHA512_FLAG,
		[HASH_ALGO_SM3]    = CRYPTO_HASH_SM3_FLAG,
	};

	if (!dev)
		return false;

	priv = dev_get_priv(dev);
	if (!priv)
		return false;

	if (!priv->enabled)
		return false;

	if (mode != CRYPTO_MODE_NONE || algo >= HASH_ALGO_NUM)
		return false;

	return !!(hash_bitmap[algo] & priv->soc_data->crypto_ver.hash_ver);
}

static int rk_hash_init(struct udevice *dev, enum HASH_ALGO algo, void **ctx)
{
	struct rockchip_crypto_priv *priv = NULL;
	struct rk_hash_ctx *tmp_ctx = NULL;
	u32 reg_ctrl = 0;
	int ret = -1;

	if (!dev)
		return -EINVAL;

	priv = dev_get_priv(dev);
	if (!priv)
		return -EINVAL;

	rk_crypto_enable_clk(dev);

	tmp_ctx = priv->hw_ctx;

	memset(tmp_ctx, 0x00, sizeof(*tmp_ctx));

	tmp_ctx->hash_cache = crypto_hash_cache_alloc(rk_hash_direct_calc, priv, DATA_ADDR_ALIGN_SIZE, DATA_LEN_ALIGN_SIZE);
	if (!tmp_ctx->hash_cache) {
		free(tmp_ctx);
		return -ENOMEM;
	}

	reg_ctrl = CRYPTO_SW_CC_RESET;
	crypto_write(reg_ctrl | (reg_ctrl << CRYPTO_WRITE_MASK_SHIFT),
			 CRYPTO_RST_CTL);

	/* wait reset compelete */
	ret = RK_POLL_TIMEOUT(crypto_read(CRYPTO_RST_CTL), RK_CRYPTO_TIMEOUT);

	reg_ctrl = 0;
	tmp_ctx->algo = algo;
	switch (algo) {
	case HASH_ALGO_MD5:
		reg_ctrl |= CRYPTO_MODE_MD5;
		tmp_ctx->digest_size = 16;
		break;
	case HASH_ALGO_SHA1:
		reg_ctrl |= CRYPTO_MODE_SHA1;
		tmp_ctx->digest_size = 20;
		break;
	case HASH_ALGO_SHA256:
		reg_ctrl |= CRYPTO_MODE_SHA256;
		tmp_ctx->digest_size = 32;
		break;
	case HASH_ALGO_SHA384:
		reg_ctrl |= CRYPTO_MODE_SHA384;
		tmp_ctx->digest_size = 48;
		break;
	case HASH_ALGO_SHA512:
		reg_ctrl |= CRYPTO_MODE_SHA512;
		tmp_ctx->digest_size = 64;
		break;
	case HASH_ALGO_SM3:
		reg_ctrl |= CRYPTO_MODE_SM3;
		tmp_ctx->digest_size = 32;
		break;
	default:
		ret = -EINVAL;
		goto exit;
	}

	clear_hash_out_reg();

	/* enable hardware padding */
	reg_ctrl |= CRYPTO_HW_PAD_ENABLE;
	crypto_write(reg_ctrl | CRYPTO_WRITE_MASK_ALL, CRYPTO_HASH_CTL);

	/* FIFO input and output data byte swap */
	/* such as B0, B1, B2, B3 -> B3, B2, B1, B0 */
	reg_ctrl = CRYPTO_DOUT_BYTESWAP | CRYPTO_DOIN_BYTESWAP;
	crypto_write(reg_ctrl | CRYPTO_WRITE_MASK_ALL, CRYPTO_FIFO_CTL);

	/* enable src_item_done interrupt */
	crypto_write(0, CRYPTO_DMA_INT_EN);

	tmp_ctx->magic = RK_HASH_CTX_MAGIC;

	*ctx = tmp_ctx;

	return 0;
exit:
	/* clear hash setting if init failed */
	crypto_write(CRYPTO_WRITE_MASK_ALL | 0, CRYPTO_HASH_CTL);

	return ret;
}

int rk_hash_update(struct udevice *dev, void *ctx, const void *input, const u32 len)
{
	struct rk_hash_ctx *tmp_ctx = (struct rk_hash_ctx *)ctx;
	int ret = -EINVAL;

	if (!dev || !tmp_ctx || !input || len == 0)
		goto exit;

	if (tmp_ctx->digest_size == 0 || tmp_ctx->magic != RK_HASH_CTX_MAGIC)
		goto exit;

	if (tmp_ctx->last_chunk_size) {
		ret = crypto_hash_update_with_cache(tmp_ctx->hash_cache,
						    tmp_ctx->last_chunk,
						    tmp_ctx->last_chunk_size, false);
		if (ret)
			goto exit;
	}

	tmp_ctx->last_chunk_size = len % sizeof(tmp_ctx->last_chunk);
	if (tmp_ctx->last_chunk_size == 0)
		tmp_ctx->last_chunk_size = sizeof(tmp_ctx->last_chunk);

	memcpy(tmp_ctx->last_chunk,
	       input + len - tmp_ctx->last_chunk_size,
	       tmp_ctx->last_chunk_size);

	ret = crypto_hash_update_with_cache(tmp_ctx->hash_cache,
					    input, len - tmp_ctx->last_chunk_size, false);
exit:
	/* free lli list */
	if (ret)
		hw_hash_clean_ctx(tmp_ctx);

	return ret;
}

int rk_hash_finish(struct udevice *dev, void *ctx, void *digest)
{
	struct rk_hash_ctx *tmp_ctx = (struct rk_hash_ctx *)ctx;
	int ret = -EINVAL;

	if (!dev || !digest)
		goto exit;

	if (!tmp_ctx ||
	    tmp_ctx->digest_size == 0 ||
	    tmp_ctx->magic != RK_HASH_CTX_MAGIC) {
		goto exit;
	}

	if (tmp_ctx->last_chunk_size == 0)
		goto exit;

	ret = crypto_hash_update_with_cache(tmp_ctx->hash_cache, tmp_ctx->last_chunk,
					    tmp_ctx->last_chunk_size, true);
	if (ret)
		goto exit;

	/* wait hash value ok */
	ret = RK_POLL_TIMEOUT(!crypto_read(CRYPTO_HASH_VALID),
			      RK_CRYPTO_TIMEOUT);

	read_regs(CRYPTO_HASH_DOUT_0, digest, tmp_ctx->digest_size);

	/* clear hash status */
	crypto_write(CRYPTO_HASH_IS_VALID, CRYPTO_HASH_VALID);
	crypto_write(CRYPTO_WRITE_MASK_ALL | 0, CRYPTO_HASH_CTL);

exit:
	rk_crypto_disable_clk(dev);

	return ret;
}

static void crypto_v3_dynamic_ver_init(struct rk_crypto_ver *ver)
{
	memset(ver, 0x00, sizeof(*ver));

	ver->aes_ver       = crypto_read(CRYPTO_AES_VERSION);
	ver->des_ver       = crypto_read(CRYPTO_DES_VERSION);
	ver->sm4_ver       = crypto_read(CRYPTO_SM4_VERSION);
	ver->hash_ver      = crypto_read(CRYPTO_HASH_VERSION);
	ver->hmac_ver      = crypto_read(CRYPTO_HMAC_VERSION);
	ver->pka_ver       = crypto_read(CRYPTO_PKA_VERSION);
	ver->extra_feature = crypto_read(CRYPTO_EXTRA_FEATURE);
}

static int rockchip_crypto_probe(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rk_crypto_soc_data *sdata;

	sdata = (struct rk_crypto_soc_data *)dev_get_driver_data(dev);
	if (!sdata)
		return -EINVAL;

	if (!priv->enabled)
		return 0;

	priv->soc_data = sdata;

	priv->hw_ctx = memalign(LLI_ADDR_ALIGN_SIZE,
				sizeof(struct rk_hash_ctx));
	if (!priv->hw_ctx)
		return -ENOMEM;

	rk_crypto_enable_clk(dev);

	hw_crypto_reset();

	if (priv->soc_data->dynamic_ver_init)
		priv->soc_data->dynamic_ver_init(&priv->soc_data->crypto_ver);

	rk_crypto_disable_clk(dev);

	return 0;
}

static const struct rk_crypto_soc_data soc_data_base = {
	.crypto_ver = {
		.aes_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CTS_FLAG |
			    CRYPTO_CTR_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_XTS_FLAG |
			    CRYPTO_CCM_FLAG |
			    CRYPTO_GCM_FLAG |
			    CRYPTO_CMAC_FLAG |
			    CRYPTO_CBCMAC_FLAG,
		.des_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_TDES_FLAG,
		.sm4_ver  = 0x00000000,
		.hash_ver = CRYPTO_HASH_SHA1_FLAG |
			    CRYPTO_HASH_SHA224_FLAG |
			    CRYPTO_HASH_SHA256_FLAG |
			    CRYPTO_HASH_SHA384_FLAG |
			    CRYPTO_HASH_SHA512_224_FLAG |
			    CRYPTO_HASH_SHA512_256_FLAG |
			    CRYPTO_HASH_SHA512_FLAG |
			    CRYPTO_HASH_MD5_FLAG,
		.hmac_ver = CRYPTO_HMAC_SHA1_FLAG |
			    CRYPTO_HMAC_SHA256_FLAG |
			    CRYPTO_HMAC_SHA512_FLAG |
			    CRYPTO_HMAC_MD5_FLAG,
		.pka_ver  = 0x01000000,
	},
};

static const struct rk_crypto_soc_data soc_data_base_sm = {
	.crypto_ver = {
		.aes_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CTS_FLAG |
			    CRYPTO_CTR_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_XTS_FLAG |
			    CRYPTO_CCM_FLAG |
			    CRYPTO_GCM_FLAG |
			    CRYPTO_CMAC_FLAG |
			    CRYPTO_CBCMAC_FLAG,
		.des_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_TDES_FLAG,
		.sm4_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CTS_FLAG |
			    CRYPTO_CTR_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_XTS_FLAG |
			    CRYPTO_CCM_FLAG |
			    CRYPTO_GCM_FLAG |
			    CRYPTO_CMAC_FLAG |
			    CRYPTO_CBCMAC_FLAG,
		.hash_ver = CRYPTO_HASH_SHA1_FLAG |
			    CRYPTO_HASH_SHA224_FLAG |
			    CRYPTO_HASH_SHA256_FLAG |
			    CRYPTO_HASH_SHA384_FLAG |
			    CRYPTO_HASH_SHA512_224_FLAG |
			    CRYPTO_HASH_SHA512_256_FLAG |
			    CRYPTO_HASH_SHA512_FLAG |
			    CRYPTO_HASH_MD5_FLAG |
			    CRYPTO_HASH_SM3_FLAG,
		.hmac_ver = CRYPTO_HMAC_SHA1_FLAG |
			    CRYPTO_HMAC_SHA256_FLAG |
			    CRYPTO_HMAC_SHA512_FLAG |
			    CRYPTO_HMAC_MD5_FLAG |
			    CRYPTO_HMAC_SM3_FLAG,
		.pka_ver  = 0x01000000,
	},
};

static const struct rk_crypto_soc_data soc_data_rk1808 = {
		.crypto_ver = {
		.aes_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CTS_FLAG |
			    CRYPTO_CTR_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_XTS_FLAG |
			    CRYPTO_CCM_FLAG |
			    CRYPTO_GCM_FLAG |
			    CRYPTO_CMAC_FLAG |
			    CRYPTO_CBCMAC_FLAG,
		.des_ver  = CRYPTO_ECB_FLAG |
			    CRYPTO_CBC_FLAG |
			    CRYPTO_CFB_FLAG |
			    CRYPTO_OFB_FLAG |
			    CRYPTO_TDES_FLAG,
		.sm4_ver  = 0x00000000,
		.hash_ver = CRYPTO_HASH_SHA1_FLAG |
			    CRYPTO_HASH_SHA224_FLAG |
			    CRYPTO_HASH_SHA256_FLAG |
			    CRYPTO_HASH_MD5_FLAG,
		.hmac_ver = CRYPTO_HMAC_SHA1_FLAG |
			    CRYPTO_HMAC_SHA256_FLAG |
			    CRYPTO_HMAC_MD5_FLAG |
			    CRYPTO_HMAC_SM3_FLAG,
		.pka_ver  = 0x01000000,
	},
};

static const struct rk_crypto_soc_data soc_data_cryptov3 = {
	.dynamic_ver_init = crypto_v3_dynamic_ver_init,
};

static const struct udevice_id rockchip_crypto_ids[] = {
	{
		.compatible = "rockchip,px30-crypto",
		.data = (ulong)&soc_data_base
	},
	{
		.compatible = "rockchip,rk1808-crypto",
		.data = (ulong)&soc_data_rk1808
	},
	{
		.compatible = "rockchip,rk3308-crypto",
		.data = (ulong)&soc_data_base
	},
	{
		.compatible = "rockchip,rv1126-crypto",
		.data = (ulong)&soc_data_base_sm
	},
	{
		.compatible = "rockchip,rk3568-crypto",
		.data = (ulong)&soc_data_base_sm
	},
	{
		.compatible = "rockchip,rk3588-crypto",
		.data = (ulong)&soc_data_base_sm
	},
	{
		.compatible = "rockchip,crypto-v3",
		.data = (ulong)&soc_data_cryptov3
	},
	{
		.compatible = "rockchip,crypto-v4",
		.data = (ulong)&soc_data_cryptov3 /* reuse crypto v3 config */
	},
	{ }
};

static struct crypto_impl rk_crypto_v2_hash_impl = {
	.name        = "hash_"CRYPTO_DRIVER_NAME,
	.type        = CRYPTO_TYPE_HASH,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = hash_check_valid,

	.hash.hash_init    = rk_hash_init,
	.hash.hash_update  = rk_hash_update,
	.hash.hash_finish  = rk_hash_finish,
};

#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)
static u8 g_key_chn;

static const u32 rk_mode2bc_mode[CIPHER_MODE_NUM] = {
	[CIPHER_MODE_ECB] = CRYPTO_BC_ECB,
	[CIPHER_MODE_CBC] = CRYPTO_BC_CBC,
	[CIPHER_MODE_CTS] = CRYPTO_BC_CTS,
	[CIPHER_MODE_CTR] = CRYPTO_BC_CTR,
	[CIPHER_MODE_CFB] = CRYPTO_BC_CFB,
	[CIPHER_MODE_OFB] = CRYPTO_BC_OFB,
	[CIPHER_MODE_XTS] = CRYPTO_BC_XTS,
	[CIPHER_MODE_CCM] = CRYPTO_BC_CCM,
	[CIPHER_MODE_GCM] = CRYPTO_BC_GCM,
	[CIPHER_MODE_CMAC] = CRYPTO_BC_CMAC,
	[CIPHER_MODE_CBC_MAC] = CRYPTO_BC_CBC_MAC,
};

static inline void set_pc_len_reg(u32 chn, u64 pc_len)
{
	u32 chn_base = CRYPTO_CH0_PC_LEN_0 + chn * 0x08;

	crypto_write(pc_len & 0xffffffff, chn_base);
	crypto_write(pc_len >> 32, chn_base + 4);
}

static inline void set_aad_len_reg(u32 chn, u64 pc_len)
{
	u32 chn_base = CRYPTO_CH0_AAD_LEN_0 + chn * 0x08;

	crypto_write(pc_len & 0xffffffff, chn_base);
	crypto_write(pc_len >> 32, chn_base + 4);
}

static inline bool is_des_mode(u32 rk_mode)
{
	return (rk_mode == CIPHER_MODE_ECB ||
		rk_mode == CIPHER_MODE_CBC ||
		rk_mode == CIPHER_MODE_CFB ||
		rk_mode == CIPHER_MODE_OFB);
}

static void dump_crypto_state(struct crypto_lli_desc *desc,
			      u32 tmp, u32 expt_int,
			      const u8 *in, const u8 *out,
			      u32 len, int ret)
{
	IMSG("%s\n", ret == -ETIME ? "timeout" : "dismatch");

	IMSG("CRYPTO_DMA_INT_ST = %08x, expect_int = %08x\n",
	     tmp, expt_int);
	IMSG("data desc		= %p\n", desc);
	IMSG("\taddr_in		= [%08x <=> %08x]\n",
	     desc->src_addr, (u32)virt_to_phys(in));
	IMSG("\taddr_out	= [%08x <=> %08x]\n",
	     desc->dst_addr, (u32)virt_to_phys(out));
	IMSG("\tsrc_len		= [%08x <=> %08x]\n",
	     desc->src_len, (u32)len);
	IMSG("\tdst_len		= %08x\n", desc->dst_len);
	IMSG("\tdma_ctl		= %08x\n", desc->dma_ctrl);
	IMSG("\tuser_define	= %08x\n", desc->user_define);

	IMSG("\n\nDMA CRYPTO_DMA_LLI_ADDR status = %08x\n",
	     crypto_read(CRYPTO_DMA_LLI_ADDR));
	IMSG("DMA CRYPTO_DMA_ST status = %08x\n",
	     crypto_read(CRYPTO_DMA_ST));
	IMSG("DMA CRYPTO_DMA_STATE status = %08x\n",
	     crypto_read(CRYPTO_DMA_STATE));
	IMSG("DMA CRYPTO_DMA_LLI_RADDR status = %08x\n",
	     crypto_read(CRYPTO_DMA_LLI_RADDR));
	IMSG("DMA CRYPTO_DMA_SRC_RADDR status = %08x\n",
	     crypto_read(CRYPTO_DMA_SRC_RADDR));
	IMSG("DMA CRYPTO_DMA_DST_RADDR status = %08x\n",
	     crypto_read(CRYPTO_DMA_DST_RADDR));
	IMSG("DMA CRYPTO_CIPHER_ST status = %08x\n",
	     crypto_read(CRYPTO_CIPHER_ST));
	IMSG("DMA CRYPTO_CIPHER_STATE status = %08x\n",
	     crypto_read(CRYPTO_CIPHER_STATE));
	IMSG("DMA CRYPTO_TAG_VALID status = %08x\n",
	     crypto_read(CRYPTO_TAG_VALID));
	IMSG("LOCKSTEP status = %08x\n\n",
	     crypto_read(0x618));

	IMSG("dst %dbyte not transferred\n",
	     desc->dst_addr + desc->dst_len -
	     crypto_read(CRYPTO_DMA_DST_RADDR));
}

static int ccm128_set_iv_reg(u32 chn, const u8 *nonce, u32 nlen)
{
	u8 iv_buf[AES_BLOCK_SIZE];
	u32 L;

	memset(iv_buf, 0x00, sizeof(iv_buf));

	L = 15 - nlen;
	iv_buf[0] = ((u8)(L - 1) & 7);

	/* the L parameter */
	L = iv_buf[0] & 7;

	/* nonce is too short */
	if (nlen < (14 - L))
		return -EINVAL;

	/* clear aad flag */
	iv_buf[0] &= ~0x40;
	memcpy(&iv_buf[1], nonce, 14 - L);

	set_iv_reg(chn, iv_buf, AES_BLOCK_SIZE);

	return 0;
}

static void ccm_aad_padding(u32 aad_len, u8 *padding, u32 *padding_size)
{
	u32 i;

	if (aad_len == 0) {
		*padding_size = 0;
		return;
	}

	i = aad_len < (0x10000 - 0x100) ? 2 : 6;

	if (i == 2) {
		padding[0] = (u8)(aad_len >> 8);
		padding[1] = (u8)aad_len;
	} else {
		padding[0] = 0xFF;
		padding[1] = 0xFE;
		padding[2] = (u8)(aad_len >> 24);
		padding[3] = (u8)(aad_len >> 16);
		padding[4] = (u8)(aad_len >> 8);
	}

	*padding_size = i;
}

static int ccm_compose_aad_iv(u8 *aad_iv, u32 data_len, u32 aad_len, u32 tag_size)
{
	aad_iv[0] |= ((u8)(((tag_size - 2) / 2) & 7) << 3);

	aad_iv[12] = (u8)(data_len >> 24);
	aad_iv[13] = (u8)(data_len >> 16);
	aad_iv[14] = (u8)(data_len >> 8);
	aad_iv[15] = (u8)data_len;

	if (aad_len)
		aad_iv[0] |= 0x40;	//set aad flag

	return 0;
}

static int hw_cipher_init(u32 chn, const u8 *key, const u8 *twk_key,
			  u32 key_len, const u8 *iv, u32 iv_len,
			  u32 algo, u32 mode, bool enc)
{
	u32 rk_mode = RK_GET_RK_MODE(mode);
	u32 key_chn_sel = chn;
	u32 reg_ctrl = 0;
	bool use_otpkey = false;

	if (!key && key_len)
		use_otpkey = true;

	IMSG("%s: key addr is %p, key_len is %d, iv addr is %p",
	     __func__, key, key_len, iv);
	if (rk_mode >= CIPHER_MODE_NUM)
		return -EINVAL;

	switch (algo) {
	case CIPHER_ALGO_DES:
		if (key_len > DES_BLOCK_SIZE)
			reg_ctrl |= CRYPTO_BC_TDES;
		else
			reg_ctrl |= CRYPTO_BC_DES;
		break;
	case CIPHER_ALGO_AES:
		reg_ctrl |= CRYPTO_BC_AES;
		break;
	case CIPHER_ALGO_SM4:
		reg_ctrl |= CRYPTO_BC_SM4;
		break;
	default:
		return -EINVAL;
	}

	if (algo == CIPHER_ALGO_AES || algo == CIPHER_ALGO_SM4) {
		switch (key_len) {
		case AES_KEYSIZE_128:
			reg_ctrl |= CRYPTO_BC_128_bit_key;
			break;
		case AES_KEYSIZE_192:
			reg_ctrl |= CRYPTO_BC_192_bit_key;
			break;
		case AES_KEYSIZE_256:
			reg_ctrl |= CRYPTO_BC_256_bit_key;
			break;
		default:
			return -EINVAL;
		}
	}

	reg_ctrl |= rk_mode2bc_mode[rk_mode];
	if (!enc)
		reg_ctrl |= CRYPTO_BC_DECRYPT;

	/* write key data to reg */
	if (!use_otpkey) {
		write_key_reg(key_chn_sel, key, key_len);
		crypto_write(CRYPTO_SEL_USER, CRYPTO_KEY_SEL);
	} else {
		crypto_write(CRYPTO_SEL_KEYTABLE, CRYPTO_KEY_SEL);
	}

	/* write twk key for xts mode */
	if (rk_mode == CIPHER_MODE_XTS)
		write_key_reg(key_chn_sel + 4, twk_key, key_len);

	/* set iv reg */
	if (rk_mode == CIPHER_MODE_CCM)
		ccm128_set_iv_reg(chn, iv, iv_len);
	else
		set_iv_reg(chn, iv, iv_len);

	/* din_swap set 1, dout_swap set 1, default 1. */
	crypto_write(0x00030003, CRYPTO_FIFO_CTL);
	crypto_write(0, CRYPTO_DMA_INT_EN);

	crypto_write(reg_ctrl | CRYPTO_WRITE_MASK_ALL, CRYPTO_BC_CTL);

	return 0;
}

static int hw_cipher_crypt(const u8 *in, u8 *out, u64 len,
			   const u8 *aad, u32 aad_len,
			   u8 *tag, u32 tag_len, u32 mode)
{
	struct crypto_lli_desc *data_desc = NULL, *aad_desc = NULL;
	u8 *dma_in = NULL, *dma_out = NULL, *aad_tmp = NULL;
	u32 rk_mode = RK_GET_RK_MODE(mode);
	u32 reg_ctrl = 0, tmp_len = 0;
	u32 expt_int = 0, mask = 0;
	u32 key_chn = g_key_chn;
	u32 tmp, dst_len = 0;
	int ret = -1;

	if (rk_mode == CIPHER_MODE_CTS && len <= AES_BLOCK_SIZE) {
		printf("CTS mode length %u < 16Byte\n", (u32)len);
		return -EINVAL;
	}

	tmp_len = (rk_mode == CIPHER_MODE_CTR) ? ROUNDUP(len, AES_BLOCK_SIZE) : len;

	data_desc = align_malloc(sizeof(*data_desc), LLI_ADDR_ALIGN_SIZE);
	if (!data_desc)
		goto exit;

	if (IS_ALIGNED((ulong)in, DATA_ADDR_ALIGN_SIZE) && tmp_len == len)
		dma_in = (void *)in;
	else
		dma_in = align_malloc(tmp_len, DATA_ADDR_ALIGN_SIZE);
	if (!dma_in)
		goto exit;

	if (out) {
		if (IS_ALIGNED((ulong)out, DATA_ADDR_ALIGN_SIZE) &&
		    tmp_len == len)
			dma_out = out;
		else
			dma_out = align_malloc(tmp_len, DATA_ADDR_ALIGN_SIZE);
		if (!dma_out)
			goto exit;
		dst_len = tmp_len;
	}

	memset(data_desc, 0x00, sizeof(*data_desc));
	if (dma_in != in)
		memcpy(dma_in, in, len);

	data_desc->src_addr    = (u32)virt_to_phys(dma_in);
	data_desc->src_len     = tmp_len;
	data_desc->dst_addr    = (u32)virt_to_phys(dma_out);
	data_desc->dst_len     = dst_len;
	data_desc->dma_ctrl    = LLI_DMA_CTRL_LAST;

	if (IS_MAC_MODE(rk_mode)) {
		expt_int = CRYPTO_LIST_DONE_INT_ST;
		data_desc->dma_ctrl |= LLI_DMA_CTRL_LIST_DONE;
	} else {
		expt_int = CRYPTO_DST_ITEM_DONE_INT_ST;
		data_desc->dma_ctrl |= LLI_DMA_CTRL_DST_DONE;
	}

	data_desc->user_define = LLI_USER_CIPHER_START |
				 LLI_USER_STRING_START |
				 LLI_USER_STRING_LAST |
				 (key_chn << 4);
	crypto_write((u32)virt_to_phys(data_desc), CRYPTO_DMA_LLI_ADDR);

	if (rk_mode == CIPHER_MODE_CCM || rk_mode == CIPHER_MODE_GCM) {
		u32 aad_tmp_len = 0;

		aad_desc = align_malloc(sizeof(*aad_desc), LLI_ADDR_ALIGN_SIZE);
		if (!aad_desc)
			goto exit;

		memset(aad_desc, 0x00, sizeof(*aad_desc));
		aad_desc->next_addr = (u32)virt_to_phys(data_desc);
		aad_desc->user_define = LLI_USER_CIPHER_START |
					 LLI_USER_STRING_START |
					 LLI_USER_STRING_LAST |
					 LLI_USER_STRING_AAD |
					 (key_chn << 4);

		if (rk_mode == CIPHER_MODE_CCM) {
			u8 padding[AES_BLOCK_SIZE];
			u32 padding_size = 0;

			memset(padding, 0x00, sizeof(padding));
			ccm_aad_padding(aad_len, padding, &padding_size);

			aad_tmp_len = aad_len + AES_BLOCK_SIZE + padding_size;
			aad_tmp_len = ROUNDUP(aad_tmp_len, AES_BLOCK_SIZE);
			aad_tmp = align_malloc(aad_tmp_len,
					       DATA_ADDR_ALIGN_SIZE);
			if (!aad_tmp)
				goto exit;

			/* clear last block */
			memset(aad_tmp + aad_tmp_len - AES_BLOCK_SIZE,
			       0x00, AES_BLOCK_SIZE);

			/* read iv data from reg */
			get_iv_reg(key_chn, aad_tmp, AES_BLOCK_SIZE);
			ccm_compose_aad_iv(aad_tmp, tmp_len, aad_len, tag_len);
			memcpy(aad_tmp + AES_BLOCK_SIZE, padding, padding_size);

			memcpy(aad_tmp + AES_BLOCK_SIZE + padding_size,
			       aad, aad_len);
		} else {
			aad_tmp_len = aad_len;
			if (IS_ALIGNED((ulong)aad, DATA_ADDR_ALIGN_SIZE)) {
				aad_tmp = (void *)aad;
			} else {
				aad_tmp = align_malloc(aad_tmp_len,
						       DATA_ADDR_ALIGN_SIZE);
				if (!aad_tmp)
					goto exit;

				memcpy(aad_tmp, aad, aad_tmp_len);
			}

			set_aad_len_reg(key_chn, aad_tmp_len);
			set_pc_len_reg(key_chn, tmp_len);
		}

		aad_desc->src_addr = (u32)virt_to_phys(aad_tmp);
		aad_desc->src_len  = aad_tmp_len;

		if (aad_tmp_len) {
			data_desc->user_define = LLI_USER_STRING_START |
						 LLI_USER_STRING_LAST |
						 (key_chn << 4);
			crypto_write((u32)virt_to_phys(aad_desc), CRYPTO_DMA_LLI_ADDR);
			cache_op_inner(DCACHE_AREA_CLEAN, aad_tmp, aad_tmp_len);
			cache_op_inner(DCACHE_AREA_CLEAN, aad_desc, sizeof(*aad_desc));
		}
	}

	cache_op_inner(DCACHE_AREA_CLEAN, data_desc, sizeof(*data_desc));
	cache_op_inner(DCACHE_AREA_CLEAN, dma_in, tmp_len);
	cache_op_inner(DCACHE_AREA_INVALIDATE, dma_out, tmp_len);

	/* din_swap set 1, dout_swap set 1, default 1. */
	crypto_write(0x00030003, CRYPTO_FIFO_CTL);
	crypto_write(0, CRYPTO_DMA_INT_EN);

	reg_ctrl = crypto_read(CRYPTO_BC_CTL) | CRYPTO_BC_ENABLE;
	crypto_write(reg_ctrl | CRYPTO_WRITE_MASK_ALL, CRYPTO_BC_CTL);
	crypto_write(0x00010001, CRYPTO_DMA_CTL);//start

	mask = ~(mask | CRYPTO_SYNC_LOCKSTEP_INT_ST);

	/* wait calc ok */
	ret = RK_POLL_TIMEOUT(!(crypto_read(CRYPTO_DMA_INT_ST) & mask),
			      RK_CRYPTO_TIMEOUT);
	tmp = crypto_read(CRYPTO_DMA_INT_ST);
	crypto_write(tmp, CRYPTO_DMA_INT_ST);

	if ((tmp & mask) == expt_int) {
		if (out && out != dma_out)
			memcpy(out, dma_out, len);

		if (IS_NEED_TAG(rk_mode)) {
			ret = WAIT_TAG_VALID(key_chn, RK_CRYPTO_TIMEOUT);
			get_tag_from_reg(key_chn, tag, AES_BLOCK_SIZE);
		}
	} else {
		dump_crypto_state(data_desc, tmp, expt_int, in, out, len, ret);
		ret = -1;
	}

exit:
	crypto_write(0xffff0000, CRYPTO_BC_CTL);//bc_ctl disable
	align_free(data_desc);
	align_free(aad_desc);
	if (dma_in != in)
		align_free(dma_in);
	if (out && dma_out != out)
		align_free(dma_out);
	if (aad && aad != aad_tmp)
		align_free(aad_tmp);

	return ret;
}

static int hw_aes_init(u32 chn, const u8 *key, const u8 *twk_key, u32 key_len,
		       const u8 *iv, u32 iv_len, u32 mode, bool enc)
{
	u32 rk_mode = RK_GET_RK_MODE(mode);

	if (rk_mode > CIPHER_MODE_XTS)
		return -EINVAL;

	if (iv_len > AES_BLOCK_SIZE)
		return -EINVAL;

	if (IS_NEED_IV(rk_mode)) {
		if (!iv || iv_len != AES_BLOCK_SIZE)
			return -EINVAL;
	} else {
		iv_len = 0;
	}

	if (rk_mode == CIPHER_MODE_XTS) {
		if (key_len != AES_KEYSIZE_128 && key_len != AES_KEYSIZE_256)
			return -EINVAL;

		if (!key || !twk_key)
			return -EINVAL;
	} else {
		if (key_len != AES_KEYSIZE_128 &&
		    key_len != AES_KEYSIZE_192 &&
		    key_len != AES_KEYSIZE_256)
			return -EINVAL;
	}

	return hw_cipher_init(chn, key, twk_key, key_len, iv, iv_len,
			      CIPHER_ALGO_AES, mode, enc);
}

static int hw_sm4_init(u32  chn, const u8 *key, const u8 *twk_key, u32 key_len,
		       const u8 *iv, u32 iv_len, u32 mode, bool enc)
{
	u32 rk_mode = RK_GET_RK_MODE(mode);

	if (rk_mode > CIPHER_MODE_XTS)
		return -EINVAL;

	if (iv_len > SM4_BLOCK_SIZE || key_len != SM4_KEYSIZE)
		return -EINVAL;

	if (IS_NEED_IV(rk_mode)) {
		if (!iv || iv_len != SM4_BLOCK_SIZE)
			return -EINVAL;
	} else {
		iv_len = 0;
	}

	if (rk_mode == CIPHER_MODE_XTS) {
		if (!key || !twk_key)
			return -EINVAL;
	}

	return hw_cipher_init(chn, key, twk_key, key_len, iv, iv_len,
			      CIPHER_ALGO_SM4, mode, enc);
}

int rk_crypto_des(struct udevice *dev, u32 mode, const u8 *key, u32 key_len,
		  const u8 *iv, const u8 *in, u8 *out, u32 len, bool enc)
{
	u32 rk_mode = RK_GET_RK_MODE(mode);
	u8 tmp_key[24];
	int ret;

	if (!is_des_mode(rk_mode))
		return -EINVAL;

	if (key_len == DES_BLOCK_SIZE || key_len == 3 * DES_BLOCK_SIZE) {
		memcpy(tmp_key, key, key_len);
	} else if (key_len == 2 * DES_BLOCK_SIZE) {
		memcpy(tmp_key, key, 16);
		memcpy(tmp_key + 16, key, 8);
		key_len = 3 * DES_BLOCK_SIZE;
	} else {
		return -EINVAL;
	}

	ret = hw_cipher_init(0, tmp_key, NULL, key_len, iv, DES_BLOCK_SIZE,
			     CIPHER_ALGO_DES, mode, enc);
	if (ret)
		goto exit;

	ret = hw_cipher_crypt(in, out, len, NULL, 0,
			      NULL, 0, mode);

exit:
	return ret;
}

int rk_crypto_aes(struct udevice *dev, u32 mode,
		  const u8 *key, const u8 *twk_key, u32 key_len,
		  const u8 *iv, u32 iv_len,
		  const u8 *in, u8 *out, u32 len, bool enc)
{
	int ret;

	/* RV1126/RV1109 do not support aes-192 */
#if defined(CONFIG_ROCKCHIP_RV1126)
	if (key_len == AES_KEYSIZE_192)
		return -EINVAL;
#endif

	ret = hw_aes_init(0, key, twk_key, key_len, iv, iv_len, mode, enc);
	if (ret)
		return ret;

	return hw_cipher_crypt(in, out, len, NULL, 0,
			       NULL, 0, mode);
}

int rk_crypto_sm4(struct udevice *dev, u32 mode,
		  const u8 *key, const u8 *twk_key, u32 key_len,
		  const u8 *iv, u32 iv_len,
		  const u8 *in, u8 *out, u32 len, bool enc)
{
	int ret;

	ret = hw_sm4_init(0, key, twk_key, key_len, iv, iv_len, mode, enc);
	if (ret)
		return ret;

	return hw_cipher_crypt(in, out, len, NULL, 0, NULL, 0, mode);
}

static bool cipher_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = NULL;
	u32 version;

	const u32 cipher_bitmap[CIPHER_MODE_NUM] = {
		[CIPHER_MODE_ECB]     = CRYPTO_ECB_FLAG,
		[CIPHER_MODE_CBC]     = CRYPTO_CBC_FLAG,
		[CIPHER_MODE_CTS]     = CRYPTO_CTS_FLAG,
		[CIPHER_MODE_CTR]     = CRYPTO_CTR_FLAG,
		[CIPHER_MODE_CFB]     = CRYPTO_CFB_FLAG,
		[CIPHER_MODE_OFB]     = CRYPTO_OFB_FLAG,
		[CIPHER_MODE_XTS]     = CRYPTO_XTS_FLAG,
		[CIPHER_MODE_CCM]     = CRYPTO_CCM_FLAG,
		[CIPHER_MODE_GCM]     = CRYPTO_GCM_FLAG,
		[CIPHER_MODE_CMAC]    = CRYPTO_CMAC_FLAG,
		[CIPHER_MODE_CBC_MAC] = CRYPTO_CBCMAC_FLAG,
	};

	if (!dev)
		return false;

	priv = dev_get_priv(dev);
	if (!priv)
		return false;

	if (!priv->enabled)
		return false;

	if (mode >= CIPHER_MODE_NUM && mode != CRYPTO_MODE_NONE)
		return false;

	if (algo >= CIPHER_ALGO_NUM)
		return false;

	switch (algo) {
	case CIPHER_ALGO_DES:
		version = priv->soc_data->crypto_ver.des_ver;
		break;
	case CIPHER_ALGO_AES:
		version = priv->soc_data->crypto_ver.aes_ver;
		break;
	case CIPHER_ALGO_SM4:
		version = priv->soc_data->crypto_ver.sm4_ver;
		break;
	default:
		return false;
	}

	if (mode == CRYPTO_MODE_NONE && version)
		return true;

	return !!(cipher_bitmap[mode] & version);
}

int rockchip_crypto_cipher(struct udevice *dev, cipher_context *ctx,
			   const u8 *in, u8 *out, u32 len, bool enc)
{
	int ret;

	rk_crypto_enable_clk(dev);

	switch (ctx->algo) {
	case CIPHER_ALGO_DES:
		ret = rk_crypto_des(dev, ctx->mode, ctx->key, ctx->key_len,
				    ctx->iv, in, out, len, enc);
		break;
	case CIPHER_ALGO_AES:
		ret = rk_crypto_aes(dev, ctx->mode,
				    ctx->key, ctx->twk_key, ctx->key_len,
				    ctx->iv, ctx->iv_len, in, out, len, enc);
		break;
	case CIPHER_ALGO_SM4:
		ret = rk_crypto_sm4(dev, ctx->mode,
				    ctx->key, ctx->twk_key, ctx->key_len,
				    ctx->iv, ctx->iv_len, in, out, len, enc);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	rk_crypto_disable_clk(dev);

	return ret;
}

int rk_crypto_mac(struct udevice *dev, u32 algo, u32 mode,
		  const u8 *key, u32 key_len,
		  const u8 *in, u32 len, u8 *tag)
{
	u32 rk_mode = RK_GET_RK_MODE(mode);
	int ret;

	if (!IS_MAC_MODE(rk_mode))
		return -EINVAL;

	if (algo != CIPHER_ALGO_AES && algo != CIPHER_ALGO_SM4)
		return -EINVAL;

	/* RV1126/RV1109 do not support aes-192 */
#if defined(CONFIG_ROCKCHIP_RV1126)
	if (algo == CIPHER_ALGO_AES && key_len == AES_KEYSIZE_192)
		return -EINVAL;
#endif

	ret = hw_cipher_init(g_key_chn, key, NULL, key_len, NULL, 0,
			     algo, mode, true);
	if (ret)
		return ret;

	return hw_cipher_crypt(in, NULL, len, NULL, 0,
			       tag, AES_BLOCK_SIZE, mode);
}

int rockchip_crypto_mac(struct udevice *dev, cipher_context *ctx,
			const u8 *in, u32 len, u8 *tag)
{
	int ret = 0;

	rk_crypto_enable_clk(dev);

	ret = rk_crypto_mac(dev, ctx->algo, ctx->mode,
			    ctx->key, ctx->key_len, in, len, tag);

	rk_crypto_disable_clk(dev);

	return ret;
}

int rk_crypto_ae(struct udevice *dev, u32 algo, u32 mode,
		 const u8 *key, u32 key_len, const u8 *nonce, u32 nonce_len,
		 const u8 *in, u32 len, const u8 *aad, u32 aad_len,
		 u8 *out, u8 *tag)
{
	u32 rk_mode = RK_GET_RK_MODE(mode);
	int ret;

	if (!IS_AE_MODE(rk_mode))
		return -EINVAL;

	if (len == 0)
		return -EINVAL;

	if (algo != CIPHER_ALGO_AES && algo != CIPHER_ALGO_SM4)
		return -EINVAL;

	/* RV1126/RV1109 do not support aes-192 */
#if defined(CONFIG_ROCKCHIP_RV1126)
	if (algo == CIPHER_ALGO_AES && key_len == AES_KEYSIZE_192)
		return -EINVAL;
#endif

	ret = hw_cipher_init(g_key_chn, key, NULL, key_len, nonce, nonce_len,
			     algo, mode, true);
	if (ret)
		return ret;

	return hw_cipher_crypt(in, out, len, aad, aad_len,
			       tag, AES_BLOCK_SIZE, mode);
}

int rockchip_crypto_ae(struct udevice *dev, cipher_context *ctx,
		       const u8 *in, u32 len, const u8 *aad, u32 aad_len,
		       u8 *out, u8 *tag)
{
	int ret = 0;

	rk_crypto_enable_clk(dev);

	ret = rk_crypto_ae(dev, ctx->algo, ctx->mode, ctx->key, ctx->key_len,
			   ctx->iv, ctx->iv_len, in, len,
			   aad, aad_len, out, tag);

	rk_crypto_disable_clk(dev);

	return ret;
}

#if CONFIG_IS_ENABLED(DM_KEYLAD)
int rockchip_crypto_fw_cipher(struct udevice *dev, cipher_fw_context *ctx,
			      const u8 *in, u8 *out, u32 len, bool enc)
{
	int ret;

	rk_crypto_enable_clk(dev);

	switch (ctx->algo) {
	case CIPHER_ALGO_DES:
		ret = rk_crypto_des(dev, ctx->mode, NULL, ctx->key_len,
				    ctx->iv, in, out, len, enc);
		break;
	case CIPHER_ALGO_AES:
		ret = rk_crypto_aes(dev, ctx->mode, NULL, NULL, ctx->key_len,
				    ctx->iv, ctx->iv_len, in, out, len, enc);
		break;
	case CIPHER_ALGO_SM4:
		ret = rk_crypto_sm4(dev, ctx->mode, NULL, NULL, ctx->key_len,
				    ctx->iv, ctx->iv_len, in, out, len, enc);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	rk_crypto_disable_clk(dev);

	return ret;
}

static ulong rockchip_crypto_keytable_addr(struct udevice *dev)
{
	return CRYPTO_S_BY_KEYLAD_BASE + CRYPTO_CH0_KEY_0;
}
#endif

static bool rockchip_crypto_is_secure(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);

	return priv->secure;
}

static struct crypto_impl rk_crypto_v2_cipher_impl = {
	.name        = "cipher_"CRYPTO_DRIVER_NAME,
	.type        = CRYPTO_TYPE_CIPHER,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = cipher_check_valid,

	.cipher.cipher_crypt = rockchip_crypto_cipher,
	.cipher.cipher_mac   = rockchip_crypto_mac,
	.cipher.cipher_ae    = rockchip_crypto_ae,
	.cipher.is_secure     = rockchip_crypto_is_secure,

#if CONFIG_IS_ENABLED(DM_KEYLAD)
	.cipher.cipher_fw_crypt = rockchip_crypto_fw_cipher,
	.cipher.keytable_addr  = rockchip_crypto_keytable_addr,
#endif

};
#endif

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

static bool rk_mod_exp_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = NULL;

	if (!dev)
		return false;

	priv = dev_get_priv(dev);
	if (!priv)
		return false;

	if (!priv->enabled)
		return false;

	if (mode != CRYPTO_MODE_NONE)
		return false;

	if (algo != ASYM_ALGO_RSA)
		return false;

	return true;
}

static struct crypto_impl rk_mod_exp_impl = {
	.name        = "mod_exp_"CRYPTO_DRIVER_NAME,
	.type        = CRYPTO_TYPE_ASYM,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = rk_mod_exp_check_valid,

	.asym.algo   = ASYM_ALGO_RSA,
	.asym.rsa.mod_exp = rk_mod_exp,
};

#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_EC)

static int rk_ecdsa_verify(struct udevice *dev, const struct ecdsa_public_key *pubkey,
			   const void *hash, size_t hash_len,
			   const void *signature, size_t sig_len)
{
	struct rk_ecp_point point_P, point_sign;
	u32  n_words;
	int ret;

	if (!pubkey || !hash || !signature)
		return -EINVAL;

	if (!pubkey->curve_name || !pubkey->x || !pubkey->y)
		return -EINVAL;

	n_words = BITS2WORD(pubkey->size_bits);

	memset(&point_P, 0, sizeof(point_P));
	memset(&point_sign, 0, sizeof(point_sign));

	ret = rk_mpa_alloc(&point_P.x, (void *)pubkey->x, n_words);
	ret |= rk_mpa_alloc(&point_P.y, (void *)pubkey->y, n_words);
	if (ret)
		goto exit;

	ret = rk_mpa_alloc(&point_sign.x, (void *)signature, n_words);
	ret |= rk_mpa_alloc(&point_sign.y,
			    (void *)signature + WORD2BYTE(n_words), n_words);
	if (ret)
		goto exit;

	rk_crypto_enable_clk(dev);
	ret = rockchip_ecc_verify(pubkey->curve_name, (void *)hash, hash_len, &point_P, &point_sign);
	rk_crypto_disable_clk(dev);
exit:
	rk_mpa_free(&point_P.x);
	rk_mpa_free(&point_P.y);
	rk_mpa_free(&point_sign.x);
	rk_mpa_free(&point_sign.y);

	return ret;
}

static bool rk_ecdsa_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = NULL;

	if (!dev)
		return false;

	priv = dev_get_priv(dev);
	if (!priv)
		return false;

	if (!priv->enabled)
		return false;

	if (mode != CRYPTO_MODE_NONE)
		return false;

	if (algo != ASYM_ALGO_ECC)
		return false;

	return rk_is_ec_supported();
}

static struct crypto_impl rk_ecdsa_impl = {
	.name	     = "ecdsa_verify_"CRYPTO_DRIVER_NAME,
	.type        = CRYPTO_TYPE_ASYM,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = rk_ecdsa_check_valid,

	.asym.algo   = ASYM_ALGO_ECC,
	.asym.ecc.verify = rk_ecdsa_verify,
};

#endif

static int rockchip_crypto_bind(struct udevice *dev)
{
	int ret;
	bool secure;
#if !defined(CONFIG_SPL_BUILD)
	bool secure_sel = false;
#else
	bool secure_sel = true;
#endif

	secure = dev_read_bool(dev, "secure");

	if (secure != secure_sel)
		return 0;

	rk_crypto_v2_hash_impl.dev = dev;

	ret = crypto_impl_register(&rk_crypto_v2_hash_impl);
	if (ret) {
		printf("crypto_impl_register rk_crypto_v2_hash_impl failed.\n");
		goto exit;
	}

#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)

	rk_crypto_v2_cipher_impl.dev = dev;

	ret = crypto_impl_register(&rk_crypto_v2_cipher_impl);
	if (ret) {
		printf("crypto_impl_register rk_crypto_v2_cipher_impl failed.\n");
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

#if CONFIG_IS_ENABLED(ROCKCHIP_EC)

	rk_ecdsa_impl.dev = dev;

	ret = crypto_impl_register(&rk_ecdsa_impl);
	if (ret) {
		printf("crypto_impl_register rk_ecdsa_impl failed.\n");
		goto exit;
	}
#endif

exit:
	return ret;
}

U_BOOT_DRIVER(rk_crypto_v2) = {
	.name       = CRYPTO_DRIVER_NAME,
	.id         = UCLASS_MISC,
	.of_match   = rockchip_crypto_ids,
	.bind       = rockchip_crypto_bind,
	.probe      = rockchip_crypto_probe,
	.of_to_plat = rockchip_crypto_of_to_plat,
	.priv_auto  = sizeof(struct rockchip_crypto_priv),
	.plat_auto  = sizeof(struct rockchip_crypto_plat),
};
