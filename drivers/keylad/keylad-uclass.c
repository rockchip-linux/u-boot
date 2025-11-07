// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd
 */

#include <keylad.h>

struct udevice *keylad_get_device(void)
{
	const struct dm_keylad_ops *ops;
	struct udevice *dev;
	struct uclass *uc;
	int ret;

	ret = uclass_get(UCLASS_KEYLAD, &uc);
	if (ret)
		return NULL;

	for (uclass_first_device(UCLASS_KEYLAD, &dev);
	     dev;
	     uclass_next_device(&dev)) {
		ops = device_get_ops(dev);
		if (!ops || !ops->transfer_fwkey)
			continue;

		return dev;
	}

	return NULL;
}

int keylad_transfer_fwkey(struct udevice *dev, ulong dst,
			  enum RK_FW_KEYID fw_keyid, u32 keylen)
{
	const struct dm_keylad_ops *ops = device_get_ops(dev);

	if (!ops || !ops->transfer_fwkey)
		return -ENOSYS;

	if (dst == 0)
		return -EINVAL;

	return ops->transfer_fwkey(dev, dst, fw_keyid, keylen);
}

UCLASS_DRIVER(keylad) = {
	.id	= UCLASS_KEYLAD,
	.name	= "keylad",
};
