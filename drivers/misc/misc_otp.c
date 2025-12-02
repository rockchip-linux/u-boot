// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <dm/uclass.h>
#include <misc.h>

struct udevice *misc_otp_get_device(u32 capability)
{
	return misc_get_device_by_capability(capability);
}

int misc_otp_read(struct udevice *dev, int offset, void *buf, int size)
{
	return misc_read(dev, offset, buf, size);
}

int misc_otp_write(struct udevice *dev, int offset, const void *buf, int size)
{
	return misc_write(dev, offset, (void *)buf, size);
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
			printf("OTP: The zone is partly written.\n");
			ret = -EACCES;
			goto out;
		}
		if (read_buf[i] == write_buf[i])
			written_size++;
		else
			break;
	}

	if (size == written_size) {
		printf("OTP: The secure region has been written.\n");
		ret = -EIO;
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
