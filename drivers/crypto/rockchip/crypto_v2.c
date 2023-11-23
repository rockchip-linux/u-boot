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
#include <rockchip/crypto_hash_cache.h>
#include <rockchip/crypto_v2.h>
#include <rockchip/crypto_v2_pka.h>

//#define DEBUG

#define CRYPTO_MD5		BIT(0)
#define CRYPTO_SHA1		BIT(1)
#define CRYPTO_SHA256		BIT(2)
#define CRYPTO_SHA512		BIT(3)
#define CRYPTO_SM3		BIT(4)
#define CRYPTO_SHA384		BIT(5)

#define CRYPTO_RSA512		BIT(10)
#define CRYPTO_RSA1024		BIT(11)
#define CRYPTO_RSA2048		BIT(12)
#define CRYPTO_RSA3072		BIT(13)
#define CRYPTO_RSA4096		BIT(14)

#define CRYPTO_DES		BIT(20)
#define CRYPTO_AES		BIT(21)
#define CRYPTO_SM4		BIT(22)

#define CRYPTO_HMAC_MD5		BIT(25)
#define CRYPTO_HMAC_SHA1 	BIT(26)
#define CRYPTO_HMAC_SHA256	BIT(27)
#define CRYPTO_HMAC_SHA512	BIT(28)
#define CRYPTO_HMAC_SM3		BIT(29)

#ifdef DEBUG
#define DMSG(format, ...) printf("[%s, %05d]-trace: " format "\n", \
				 __func__, __LINE__, ##__VA_ARGS__)
#else
#define DMSG(format, ...)
#endif

#define	RK_HASH_CTX_MAGIC	0x1A1A1A1A
#define CRYPTO_V2_DRIVER_NAME	"rk_crypto_v2"

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

#define IS_NEED_IV(rk_mode) ((rk_mode) != RK_MODE_ECB && \
			     (rk_mode) != RK_MODE_CMAC && \
			     (rk_mode) != RK_MODE_CBC_MAC)

#define IS_NEED_TAG(rk_mode) ((rk_mode) == RK_MODE_CMAC || \
			      (rk_mode) == RK_MODE_CBC_MAC || \
			      (rk_mode) == RK_MODE_CCM || \
			      (rk_mode) == RK_MODE_GCM)

#define IS_MAC_MODE(rk_mode) ((rk_mode) == RK_MODE_CMAC || \
			      (rk_mode) == RK_MODE_CBC_MAC)

#define IS_AE_MODE(rk_mode) ((rk_mode) == RK_MODE_CCM || \
			     (rk_mode) == RK_MODE_GCM)

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

struct rk_crypto_soc_data {
	u32 capability;
	u32 (*dynamic_cap)(void);
};

struct rockchip_crypto_plat {
	void __iomem	*base;
	u32				*clocks;
	u32				*frequencies;
	u32				nclocks;
	u32				freq_nclocks;
};

struct rockchip_crypto_priv {
	u32				length;
	struct rk_hash_ctx		*hw_ctx;
	struct rk_crypto_soc_data	*soc_data;
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
	return rk_crypto_set_clk(dev, 1);
}

static int rk_crypto_disable_clk(struct udevice *dev)
{
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

static bool hash_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	struct rockchip_crypto_priv *priv = NULL;

	const u32 hash_bitmap[HASH_ALGO_NUM] = {
		[HASH_ALGO_MD5]    = CRYPTO_MD5,
		[HASH_ALGO_SHA1]   = CRYPTO_SHA1,
		[HASH_ALGO_SHA256] = CRYPTO_SHA256,
		[HASH_ALGO_SHA384] = CRYPTO_SHA384,
		[HASH_ALGO_SHA512] = CRYPTO_SHA512,

	};

	if (!dev)
		return false;

	priv = dev_get_priv(dev);
	if (!priv)
		return false;

	if (mode != CRYPTO_MODE_NONE || algo >= HASH_ALGO_NUM)
		return false;

	return !!(hash_bitmap[algo] & priv->soc_data->capability);
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

static u32 crypto_v3_dynamic_cap(void)
{
	u32 capability = 0;
	u32 ver_reg, i;
	struct cap_map {
		u32 ver_offset;
		u32 mask;
		u32 cap_bit;
	};
	const struct cap_map cap_tbl[] = {
	{CRYPTO_HASH_VERSION, CRYPTO_HASH_MD5_FLAG,    CRYPTO_MD5},
	{CRYPTO_HASH_VERSION, CRYPTO_HASH_SHA1_FLAG,   CRYPTO_SHA1},
	{CRYPTO_HASH_VERSION, CRYPTO_HASH_SHA256_FLAG, CRYPTO_SHA256},
	{CRYPTO_HASH_VERSION, CRYPTO_HASH_SHA512_FLAG, CRYPTO_SHA512},
	{CRYPTO_HASH_VERSION, CRYPTO_HASH_SM3_FLAG,    CRYPTO_SM3},

	{CRYPTO_HMAC_VERSION, CRYPTO_HMAC_MD5_FLAG,    CRYPTO_HMAC_MD5},
	{CRYPTO_HMAC_VERSION, CRYPTO_HMAC_SHA1_FLAG,   CRYPTO_HMAC_SHA1},
	{CRYPTO_HMAC_VERSION, CRYPTO_HMAC_SHA256_FLAG, CRYPTO_HMAC_SHA256},
	{CRYPTO_HMAC_VERSION, CRYPTO_HMAC_SHA512_FLAG, CRYPTO_HMAC_SHA512},
	{CRYPTO_HMAC_VERSION, CRYPTO_HMAC_SM3_FLAG,    CRYPTO_HMAC_SM3},

	{CRYPTO_AES_VERSION,  CRYPTO_AES256_FLAG,      CRYPTO_AES},
	{CRYPTO_DES_VERSION,  CRYPTO_TDES_FLAG,        CRYPTO_DES},
	{CRYPTO_SM4_VERSION,  CRYPTO_ECB_FLAG,         CRYPTO_SM4},
	};

	/* rsa */
	capability = CRYPTO_RSA512 |
		     CRYPTO_RSA1024 |
		     CRYPTO_RSA2048 |
		     CRYPTO_RSA3072 |
		     CRYPTO_RSA4096;

	for (i = 0; i < ARRAY_SIZE(cap_tbl); i++) {
		ver_reg = crypto_read(cap_tbl[i].ver_offset);

		if ((ver_reg & cap_tbl[i].mask) == cap_tbl[i].mask)
			capability |= cap_tbl[i].cap_bit;
	}

	return capability;
}

static u32 crypto_adjust_capability(u32 capability)
{
	u32 mask = 0;

#if !(CONFIG_IS_ENABLED(ROCKCHIP_CIPHER))
	mask |= (CRYPTO_DES | CRYPTO_AES | CRYPTO_SM4);
#endif

#if !(CONFIG_IS_ENABLED(ROCKCHIP_HMAC))
	mask |= (CRYPTO_HMAC_MD5 | CRYPTO_HMAC_SHA1 | CRYPTO_HMAC_SHA256 |
			 CRYPTO_HMAC_SHA512 | CRYPTO_HMAC_SM3);
#endif

#if !(CONFIG_IS_ENABLED(ROCKCHIP_RSA))
	mask |= (CRYPTO_RSA512 | CRYPTO_RSA1024 | CRYPTO_RSA2048 |
			 CRYPTO_RSA3072 | CRYPTO_RSA4096);
#endif

	return capability & (~mask);
}

static int rockchip_crypto_probe(struct udevice *dev)
{
	struct rockchip_crypto_priv *priv = dev_get_priv(dev);
	struct rk_crypto_soc_data *sdata;
	int ret = 0;

	sdata = (struct rk_crypto_soc_data *)dev_get_driver_data(dev);

	if (sdata->dynamic_cap)
		sdata->capability = sdata->dynamic_cap();
	else
		sdata->capability = crypto_adjust_capability(sdata->capability);

	priv->soc_data = sdata;

	priv->hw_ctx = memalign(LLI_ADDR_ALIGN_SIZE,
				sizeof(struct rk_hash_ctx));
	if (!priv->hw_ctx)
		return -ENOMEM;

	ret = rk_crypto_clk_init(dev);
	if (ret)
		return ret;

	rk_crypto_enable_clk(dev);

	hw_crypto_reset();

	rk_crypto_disable_clk(dev);

	return 0;
}

static const struct rk_crypto_soc_data soc_data_base = {
	.capability =
		      CRYPTO_MD5 |
		      CRYPTO_SHA1 |
		      CRYPTO_SHA256 |
		      CRYPTO_SHA384 |
		      CRYPTO_SHA512 |
		      CRYPTO_HMAC_MD5 |
		      CRYPTO_HMAC_SHA1 |
		      CRYPTO_HMAC_SHA256 |
		      CRYPTO_HMAC_SHA512 |
		      CRYPTO_RSA512 |
		      CRYPTO_RSA1024 |
		      CRYPTO_RSA2048 |
		      CRYPTO_RSA3072 |
		      CRYPTO_RSA4096 |
		      CRYPTO_DES |
		      CRYPTO_AES,
};

static const struct rk_crypto_soc_data soc_data_base_sm = {
	.capability = CRYPTO_MD5 |
		      CRYPTO_SHA1 |
		      CRYPTO_SHA256 |
		      CRYPTO_SHA384 |
		      CRYPTO_SHA512 |
		      CRYPTO_SM3 |
		      CRYPTO_HMAC_MD5 |
		      CRYPTO_HMAC_SHA1 |
		      CRYPTO_HMAC_SHA256 |
		      CRYPTO_HMAC_SHA512 |
		      CRYPTO_HMAC_SM3 |
		      CRYPTO_RSA512 |
		      CRYPTO_RSA1024 |
		      CRYPTO_RSA2048 |
		      CRYPTO_RSA3072 |
		      CRYPTO_RSA4096 |
		      CRYPTO_DES |
		      CRYPTO_AES |
		      CRYPTO_SM4,
};

static const struct rk_crypto_soc_data soc_data_rk1808 = {
	.capability = CRYPTO_MD5 |
		      CRYPTO_SHA1 |
		      CRYPTO_SHA256 |
		      CRYPTO_HMAC_MD5 |
		      CRYPTO_HMAC_SHA1 |
		      CRYPTO_HMAC_SHA256 |
		      CRYPTO_RSA512 |
		      CRYPTO_RSA1024 |
		      CRYPTO_RSA2048 |
		      CRYPTO_RSA3072 |
		      CRYPTO_RSA4096,
};

static const struct rk_crypto_soc_data soc_data_cryptov3 = {
	.capability  = 0,
	.dynamic_cap = crypto_v3_dynamic_cap,
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
	.type        = CRYPTO_TYPE_HASH,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = hash_check_valid,

	.hash.hash_init    = rk_hash_init,
	.hash.hash_update  = rk_hash_update,
	.hash.hash_finish  = rk_hash_finish,
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

static bool rk_mod_exp_check_valid(struct udevice *dev, u32 algo, u32 mode)
{
	if (!dev)
		return false;

	if (mode != CRYPTO_MODE_NONE)
		return false;

	if (algo != ASYM_ALGO_RSA)
		return false;

	return true;
}

static struct crypto_impl rk_mod_exp_impl = {
	.type        = CRYPTO_TYPE_ASYM,
	.uclass_id   = UCLASS_MISC,
	.priority    = CRYPTO_PRIORITY_HW,
	.check_valid = rk_mod_exp_check_valid,

	.asym.rsa.mod_exp = rk_mod_exp,
};

#endif

static int rockchip_crypto_bind(struct udevice *dev)
{
	int ret;

	rk_crypto_v2_hash_impl.dev = dev;

	ret = crypto_impl_register(&rk_crypto_v2_hash_impl);
	if (ret) {
		printf("crypto_impl_register rk_crypto_v2_hash_impl failed.\n");
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

U_BOOT_DRIVER(rk_crypto_v2) = {
	.name       = CRYPTO_V2_DRIVER_NAME,
	.id         = UCLASS_MISC,
	.of_match   = rockchip_crypto_ids,
	.bind       = rockchip_crypto_bind,
	.probe      = rockchip_crypto_probe,
	.of_to_plat = rockchip_crypto_of_to_plat,
	.priv_auto  = sizeof(struct rockchip_crypto_priv),
	.plat_auto  = sizeof(struct rockchip_crypto_plat),
};
