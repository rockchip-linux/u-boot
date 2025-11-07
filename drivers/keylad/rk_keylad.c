// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <clk.h>
#include <keylad.h>
#include <dm.h>
#include <misc.h>
#include <asm/io.h>
#include <clk-uclass.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clock.h>

#define KEYLAD_APB_CMD			0x0450
#define REG_APB_CMD_EN			BIT(0)
#define VALUE_APB_CMD_DISABLE		0
#define VALUE_APB_CMD_ENABLE		BIT(0)

#define KEYLAD_APB_PADDR		0x0454
#define KEYLAD_APB_PWDATA		0x0458
#define KEYLAD_APB_PWRITE		0x045C
#define KEYLAD_DATA_CTL			0x0460
#define VALUE_DATA_CTL_EN		BIT(15)

#define KEYLAD_KEY_SEL			0x0610
#define VALUE_KEY_SEL_OUTER_KEY		0x00000000

#define KEYLAD_LOCKSTEP_FLAG		0x0618
#define KEYLAD_LOCKSTEP_EN		0x061C

#define KEY_LADDER_OTP_KEY_REQ		0x0640
#define KL_OTP_KEY_REQ_DST_ADDR(addr)	((addr) & 0x3) // 256bit algin address
#define KL_OTP_KEY_REQ_BYTE_SWAP	BIT(4)
#define KL_OTP_KEY_REQ_WORD_SWAP	BIT(5)
#define KL_OTP_KEY_REQ_EN		BIT(8)
#define KL_OTP_KEY_ECC_ST		BIT(12)
#define KL_OTP_KEY_REQ_SRC_ADDR(addr)	(((addr) & 0xffff) << 16)// byte address, dword align

#define KEY_LADDER_KEY_LEN		0x0648
#define KL_KEY_LEN(len)			((len) & 0x3f)

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
	char				*clocks;
	u32				nclocks;
};

fdt_addr_t keylad_base;

static int rk_keylad_do_enable_clk(struct udevice *dev, int enable)
{
	struct rockchip_keylad_priv *priv = dev_get_priv(dev);
	struct clk clk;
	int i, ret;

	for (i = 0; i < priv->nclocks; i++) {
		ret = clk_get_by_index(dev, i, &clk);
		if (ret < 0) {
			printf("Keylad failed to get clk index %d, ret=%d\n", i, ret);
			return ret;
		}

		if (enable)
			ret = clk_enable(&clk);
		else
			ret = clk_disable(&clk);

		if (ret < 0 && ret != -ENOSYS) {
			printf("Keylad failed to enable(%d) clk(%ld): ret=%d\n",
			       enable, clk.id, ret);
			return ret;
		}
	}

	return 0;
}

static int rk_keylad_enable_clk(struct udevice *dev)
{
	return rk_keylad_do_enable_clk(dev, 1);
}

static int rk_keylad_disable_clk(struct udevice *dev)
{
	return rk_keylad_do_enable_clk(dev, 0);
}

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

static int rk_otp_keylad_read_init(void)
{
	struct udevice *dev;

	dev = misc_otp_get_device(OTP_S);
	if (!dev)
		return -ENODEV;

	misc_otp_ioctl(dev, IOCTL_REQ_START, NULL);
	misc_otp_ioctl(dev, IOCTL_REQ_KEYLAD_INIT, NULL);

	return 0;
}

static int rk_otp_keylad_read_deinit(void)
{
	struct udevice *dev;

	dev = misc_otp_get_device(OTP_S);
	if (!dev)
		return -ENODEV;

	misc_otp_ioctl(dev, IOCTL_REQ_KEYLAD_DEINIT, NULL);
	misc_otp_ioctl(dev, IOCTL_REQ_STOP, NULL);

	return 0;
}

static int rk_keylad_read_otp_key(u32 otp_offset, u32 keylad_area, u32 keylen)
{
	int ret = 0;
	u32 val = 0;
	u32 nbytes = keylen;

	/* keylad_area of 256bits can be 0-1 */
	if (keylad_area >= KEYLAD_AREA_NUM)
		return -EINVAL;

	ret = rk_otp_keylad_read_init();
	if (ret) {
		printf("Keyladder read otp key init err: 0x%x.", ret);
		return ret;
	}

	/* src use byte address, dst use keytable block address */
	val = KL_OTP_KEY_REQ_SRC_ADDR(otp_offset / 2) |
	      KL_OTP_KEY_REQ_DST_ADDR(keylad_area) |
	      KL_OTP_KEY_REQ_BYTE_SWAP |
	      KL_OTP_KEY_REQ_EN;

	keylad_write(KEYLAD_KEY_SEL, VALUE_KEY_SEL_OUTER_KEY);

	keylad_write(KEY_LADDER_KEY_LEN, KL_KEY_LEN(nbytes));

	keylad_write(KEY_LADDER_OTP_KEY_REQ, val);

	KEYLAD_POLL_TIMEOUT(keylad_read(KEY_LADDER_OTP_KEY_REQ) & KL_OTP_KEY_REQ_EN,
			    RK_KEYLAD_TIME_OUT, ret);

	val = keylad_read(KEY_LADDER_OTP_KEY_REQ);
	if (val & KL_OTP_KEY_ECC_ST) {
		printf("KEYLAD transfer OTP key ECC check error!");
		ret = -EIO;
	}

	rk_otp_keylad_read_deinit();

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

	rk_keylad_enable_clk(dev);

	res = rk_keylad_read_otp_key(fw_key_offset, 0, keylen);
	if (res) {
		printf("Keyladder read otp key err: 0x%x.", res);
		return res;
	}

	res = rk_keylad_send_key(0, keylen / 4, dst);

	rk_keylad_disable_clk(dev);

	if (res) {
		printf("Keyladder transfer key err: 0x%x.", res);
		goto exit;
	}

exit:
	return res;
}

static const struct dm_keylad_ops rockchip_keylad_ops = {
	.transfer_fwkey   = rockchip_keylad_transfer_fwkey,
};

static int rockchip_keylad_ofdata_to_platdata(struct udevice *dev)
{
	struct rockchip_keylad_priv *priv = dev_get_priv(dev);
	int len = 0;
	int ret = 0;

	memset(priv, 0x00, sizeof(*priv));

	priv->reg = (fdt_addr_t)dev_read_addr_ptr(dev);
	if (priv->reg == FDT_ADDR_T_NONE)
		return -EINVAL;

	keylad_base = priv->reg;

	/* if there is no clocks in dts, just skip it */
	if (!dev_read_prop(dev, "clocks", &len)) {
		printf("Keylad \"clocks\" property not set.\n");
		return 0;
	}

	priv->clocks = malloc(len);
	if (!priv->clocks)
		return -ENOMEM;

	priv->nclocks = len / (2 * sizeof(u32));
	if (dev_read_u32_array(dev, "clocks", (u32 *)priv->clocks,
			       priv->nclocks)) {
		printf("Keylad can't read \"clocks\" property.\n");
		ret = -EINVAL;
		goto exit;
	}

	return 0;
exit:
	if (priv->clocks)
		free(priv->clocks);

	return ret;
}

static int rockchip_keylad_probe(struct udevice *dev)
{
	rk_keylad_disable_clk(dev);

	return 0;
}

static const struct udevice_id rockchip_keylad_ids[] = {
	{
		.compatible = "rockchip,keylad",
	},
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
