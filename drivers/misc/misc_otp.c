// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <dm/uclass.h>
#include <misc.h>
#if CONFIG_IS_ENABLED(DM_HWSPINLOCK)
#include <hwspinlock.h>
#include <hwspinlock-uclass.h>

#define MISC_OTP_HW_SPINLOCK_TIMEOUT	2000000
#endif

struct udevice *misc_otp_get_device(u32 capability)
{
	return misc_get_device_by_capability(capability);
}

int misc_otp_read(struct udevice *dev, int offset, void *buf, int size)
{
	int ret;

#if CONFIG_IS_ENABLED(DM_HWSPINLOCK)
	struct hwspinlock otp_lock;

	ret = hwspin_lock_get_id_by_index(dev, 0, &otp_lock);
	if (ret) {
		printf("%s get hwspin id failed, ret=%d\n", dev->name, ret);
		return ret;
	}
	ret = hwspin_trylock_timeout(&otp_lock, MISC_OTP_HW_SPINLOCK_TIMEOUT);
	if (ret) {
		printf("%s try hwspin lock failed, ret=%d\n", dev->name, ret);
		return ret;
	}
#endif
	ret = misc_read(dev, offset, buf, size);
	if (ret)
		printf("%s read misc failed, ret=%d\n", dev->name, ret);

#if CONFIG_IS_ENABLED(DM_HWSPINLOCK)
	hwspin_unlock(&otp_lock);
#endif

	return ret;
}

int misc_otp_write(struct udevice *dev, int offset, const void *buf, int size)
{
	int ret;

#if CONFIG_IS_ENABLED(DM_HWSPINLOCK)
	struct hwspinlock otp_lock;

	ret = hwspin_lock_get_id_by_index(dev, 0, &otp_lock);
	if (ret) {
		printf("%s get hwspin id failed, ret=%d\n", dev->name, ret);
		return ret;
	}
	ret = hwspin_trylock_timeout(&otp_lock, MISC_OTP_HW_SPINLOCK_TIMEOUT);
	if (ret) {
		printf("%s try hwspin lock failed, ret=%d\n", dev->name, ret);
		return ret;
	}
#endif
	ret = misc_write(dev, offset, (void *)buf, size);
	if (ret)
		printf("%s write misc failed, ret=%d\n", dev->name, ret);

#if CONFIG_IS_ENABLED(DM_HWSPINLOCK)
	hwspin_unlock(&otp_lock);
#endif

	return ret;
}

int misc_otp_ioctl(struct udevice *dev, unsigned long request, void *buf)
{
	return misc_ioctl(dev, request, buf);
}

int misc_otp_write_verify(struct udevice *dev, int offset, const uint8_t *write_buf, int size)
{
	uint8_t verify_buf[size];
	uint8_t read_buf[size];
	int i, written_size = 0, ret = 0;

	memset(verify_buf, 0, sizeof(verify_buf));
	memset(read_buf, 0, sizeof(read_buf));

	ret = misc_otp_read(dev, offset, &read_buf, size);
	if (ret) {
		printf("OTP: misc_otp_read fail, ret=%d\n", ret);
		goto out;
	}

	for (i = 0; i < size; i++) {
		/* Already 1 value in otp can't be written to 0. */
		if (read_buf[i] & ~write_buf[i]) {
			printf("OTP: The zone is partly written and value is different.\n");
			ret = -EACCES;
			goto out;
		}
		if (read_buf[i] == write_buf[i])
			written_size++;
		else
			break;
	}

	/* No need to return error code, because expected value has already been written. */
	if (size == written_size) {
		printf("OTP: The secure region has already written by the same value.\n");
		goto out;
	}

	ret = misc_otp_write(dev, (offset + written_size), &write_buf[written_size],
			     (size - written_size));
	if (ret) {
		printf("OTP: misc_otp_write fail, ret=%d\n", ret);
		goto out;
	}

	ret = misc_otp_read(dev, (offset + written_size), verify_buf, (size - written_size));
	if (ret) {
		printf("OTP: misc_otp_read(verify) fail, ret=%d\n", ret);
		goto out;
	}

	for (i = 0; i < (size - written_size); i++) {
		if ((write_buf[i + written_size] | read_buf[i + written_size]) != verify_buf[i]) {
			ret = -EIO;
			printf("OTP: Actual value(%u) is different from expected value(%u).\n",
			       verify_buf[i], (write_buf[i + written_size] | read_buf[i + written_size]));
			goto out;
		}
	}

out:
	return ret;
}
