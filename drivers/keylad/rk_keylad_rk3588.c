// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd
 */
#include <clk.h>
#include <clk-uclass.h>
#include <common.h>
#include <dm.h>
#include <keylad.h>
#include <misc.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/hardware.h>

#define KEYLAD_APB_CMD			0x0450
#define REG_APB_CMD_EN			BIT(0)
#define VALUE_APB_CMD_DISABLE		0
#define VALUE_APB_CMD_ENABLE		BIT(0)

#define KEYLAD_APB_PADDR		0x0454
#define KEYLAD_APB_PWDATA		0x0458
#define KEYLAD_APB_PWRITE		0x045C
#define KEYLAD_DATA_CTL			0x0460
#define VALUE_DATA_CTL_EN		BIT(15)

#define KEYLAD_OTP_COPY			0x060C
#define REG_OTP_KEY_COPY_EN		BIT(2)
#define VALUE_OTP_KEY_COPY_DISABLE	0
#define VALUE_OTP_KEY_COPY_ENABLE	BIT(2)

#define KEYLAD_SRC_NUM_SEL		0x0620
#define VALUE_SELECT_OTP		0
#define VALUE_SELECT_TRNG		BIT(0)

#define KEYLAD_SRC_NUM_DONE		0x0624
#define REG_SRC_NUM_DONE		BIT(0)
#define VALUE_NOT_REACH_PORT		0
#define VALUE_REACH_PORT		BIT(0)

#define KEYLAD_KEY_REG_SIZE_BYTES	4
#define KEYLAD_KEY_REG_NUM		32
#define KEYLAD_AREA_NUM			2

#define RK_KEYLAD_TIME_OUT		10000  /* max 10ms */

#define KEYLAD_POLL_TIMEOUT(condition, timeout, ret) do { \
	u32 time_out = timeout; \
	while (condition) { \
		if (time_out-- == 0) { \
			printf("[%s] %d: time out!\n", __func__, __LINE__); \
			ret = -ETIMEDOUT; \
			break; \
		} \
		udelay(1); \
	} \
} while (0)

struct rockchip_keylad_priv {
	fdt_addr_t			reg;
};

fdt_addr_t keylad_base;

static inline u32 keylad_read(u32 offset)
{
	return readl(keylad_base + offset);
}

static inline void keylad_write(u32 offset, u32 val)
{
	writel(val, keylad_base + offset);
}

static int rk_get_fwkey_param(u32 keyid, u32 *offset, u32 *max_len)
{
	switch (keyid) {
	case RK_FW_KEY0:
		*offset  = OTP_FW_ENC_KEY_ADDR;
		*max_len = OTP_FW_ENC_KEY_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rk_get_otpkey_param(u32 keyid, u32 *offset, u32 *max_len)
{
	switch (keyid) {
	case RK_OTP_KEY0:
		*offset  = OTP_OEM_KEY0_ADDR;
		*max_len = OTP_OEM_KEY0_SIZE;
		break;
	case RK_OTP_KEY1:
		*offset  = OTP_OEM_KEY1_ADDR;
		*max_len = OTP_OEM_KEY1_SIZE;
		break;
	case RK_OTP_KEY2:
		*offset  = OTP_OEM_KEY2_ADDR;
		*max_len = OTP_OEM_KEY2_SIZE;
		break;
	case RK_OTP_KEY3:
		*offset  = OTP_OEM_KEY3_ADDR;
		*max_len = OTP_OEM_KEY3_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int rk_otp_s_flush(u32 otp_offset_word, u32 n_words)
{
	struct otp_param param;
	struct udevice *dev;

	dev = misc_otp_get_device(OTP_S);
	if (!dev)
		return -ENODEV;

	memset(&param, 0x00, sizeof(param));

	param.flush_offset = otp_offset_word;
	param.flush_size   = n_words;

	return misc_otp_ioctl(dev, IOCTL_REQ_FLUSH, &param);
}

static int rk_keylad_flush_otp_cache(u32 otp_offset, u32 keylen)
{
	int ret = 0;
	uint32_t otp_offset_word = otp_offset / 4;

	/* clear src num_done */
	keylad_write(KEYLAD_SRC_NUM_DONE, VALUE_REACH_PORT);
	/* select otp as src */
	keylad_write(KEYLAD_SRC_NUM_SEL, VALUE_SELECT_OTP);

	/* flush data from otp */
	ret = rk_otp_s_flush(otp_offset_word, keylen / sizeof(u32));
	if (ret) {
		printf("Failed to flush otp cache, ret: %d\n", ret);
		return ret;
	}

	/* wait for writing to keylad port */
	KEYLAD_POLL_TIMEOUT((keylad_read(KEYLAD_SRC_NUM_DONE) & REG_SRC_NUM_DONE) ==
			    VALUE_NOT_REACH_PORT, RK_KEYLAD_TIME_OUT, ret);

	return ret;
}

static int rk_keylad_send_key(u32 key_reg, u32 n_words, ulong dst_addr)
{
	int ret = 0;

	/* key_reg of 32bits can be 0-31 */
	if ((key_reg + n_words) > KEYLAD_KEY_REG_NUM)
		return -EINVAL;

	for (u32 i = 0; i < n_words; i++) {
		/* set destination addr */
		keylad_write(KEYLAD_APB_PADDR,
			     (dst_addr & 0xffffffff) + (i * KEYLAD_KEY_REG_SIZE_BYTES));
		/* select which word of key table to be sent */
		keylad_write(KEYLAD_APB_PWDATA, key_reg + i);

		keylad_write(KEYLAD_APB_CMD, VALUE_APB_CMD_ENABLE);
		KEYLAD_POLL_TIMEOUT((keylad_read(KEYLAD_APB_CMD) & REG_APB_CMD_EN) ==
				    VALUE_APB_CMD_ENABLE, RK_KEYLAD_TIME_OUT, ret);
	}

	return ret;
}

static int rk_keylad_read_otp_key(u32 otp_offset, u32 keylad_area, u32 keylen)
{
	int ret = 0;
	u32 nbytes = keylen;

	/* keylad_area of 256bits can be 0-1 */
	if (keylad_area >= KEYLAD_AREA_NUM)
		return -EINVAL;

	ret = rk_keylad_flush_otp_cache(otp_offset, nbytes);
	if (ret)
		return ret;

	/* select which 256-bits of key table to be write */
	keylad_write(KEYLAD_OTP_COPY, VALUE_OTP_KEY_COPY_ENABLE | keylad_area);
	/* wait for writing to key table */
	KEYLAD_POLL_TIMEOUT((keylad_read(KEYLAD_OTP_COPY) & REG_OTP_KEY_COPY_EN) ==
			     VALUE_OTP_KEY_COPY_ENABLE, RK_KEYLAD_TIME_OUT, ret);

	return ret;
}

static int rockchip_keylad_transfer_fwkey(struct udevice *dev, ulong dst,
					  u32 fw_keyid, u32 keylen)
{
	int res = 0;
	u32 fw_key_offset;
	u32 max_key_len = 0;

	if (keylen % 4) {
		printf("key_len(%u) must be multiple of 4 error.", keylen);
		return -EINVAL;
	}

	res = rk_get_fwkey_param(fw_keyid, &fw_key_offset, &max_key_len);
	if (res)
		return res;

	if (keylen > max_key_len) {
		printf("key_len(%u) > %u error.", keylen, max_key_len);
		return -EINVAL;
	}

	res = rk_keylad_read_otp_key(fw_key_offset, 0, keylen);
	if (res) {
		printf("Keyladder read otp key err: 0x%x.", res);
		return res;
	}

	res = rk_keylad_send_key(0, keylen / 4, dst);

	if (res) {
		printf("Keyladder transfer key err: 0x%x.", res);
		goto exit;
	}

exit:
	return res;
}

static int rockchip_keylad_transfer_otpkey(struct udevice *dev, ulong dst,
					   enum RK_OTP_KEYID otp_keyid, u32 keylen)
{
	int res = 0;
	u32 otp_offset;
	u32 max_key_len = 0;

	if (keylen % 4) {
		printf("key_len(%u) must be multiple of 4 error.", keylen);
		return -EINVAL;
	}

	res = rk_get_otpkey_param(otp_keyid, &otp_offset, &max_key_len);
	if (res) {
		printf("Invalid otp_keyid(%u) error.", otp_keyid);
		goto exit;
	}

	if (keylen > max_key_len) {
		printf("key_len(%u) > %u error.", keylen, max_key_len);
		res = -EINVAL;
		goto exit;
	}

	res = rk_keylad_read_otp_key(otp_offset, 0, keylen);
	if (res) {
		printf("Keyladder read otp key err: 0x%x.", res);
		goto exit;
	}

	res = rk_keylad_send_key(0, keylen / 4, dst);
	if (res) {
		printf("Keyladder transfer key err: 0x%x.", res);
		goto exit;
	}

exit:
	return res;
}

static const struct dm_keylad_ops rockchip_keylad_ops = {
	.transfer_fwkey  = rockchip_keylad_transfer_fwkey,
	.transfer_otpkey = rockchip_keylad_transfer_otpkey,
};

static int rockchip_keylad_ofdata_to_platdata(struct udevice *dev)
{
	struct rockchip_keylad_priv *priv = dev_get_priv(dev);

	memset(priv, 0x00, sizeof(*priv));

	priv->reg = (fdt_addr_t)dev_read_addr_ptr(dev);
	if (priv->reg == FDT_ADDR_T_NONE)
		return -EINVAL;

	keylad_base = priv->reg;

	return 0;
}

static int rockchip_keylad_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id rockchip_keylad_ids[] = {
	{
		.compatible = "rockchip,keylad",
	},
	{}
};

U_BOOT_DRIVER(rockchip_keylad) = {
	.name		= "rockchip_keylad",
	.id		= UCLASS_KEYLAD,
	.of_match	= rockchip_keylad_ids,
	.ops		= &rockchip_keylad_ops,
	.probe		= rockchip_keylad_probe,
	.ofdata_to_platdata = rockchip_keylad_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct rockchip_keylad_priv),
};
