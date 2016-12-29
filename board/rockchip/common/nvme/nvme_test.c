/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "pcie.h"
#include "nvme.h"

#define SECTOR_NUM 4096

int nvme_test(void)
{
	int err, i;
	u64 data = 1;
	uint64_t ts;
	bool should_retry = false;
	int time_elapsed;

	__aligned(512) u8 *rbuf;
	__aligned(512) u8 *wbuf;

	rbuf = (u8 *)(gd->arch.ddr_end - CONFIG_RK_LCD_SIZE - SZ_16M);
	wbuf = (u8 *)(gd->arch.ddr_end - CONFIG_RK_LCD_SIZE - SZ_32M);

	printf("start nvme test, sector number = %d\n", SECTOR_NUM);

	err = nvme_init(0);
	if (err) {
		printf("nvmee_test: fail to init nvme\n");
		return err;
	}

	err = nvme_get_capacity(0);
	if (err < 0) {
		printf("nvme_test: fail to get capacity \n");
		return err;
	}
	printf("nvme_test: SSD capaacity is 0x%x sector\n", err);

retry:
	memset(wbuf, data, sizeof(u8) * 512 * SECTOR_NUM);
	memset(rbuf, 0x0, sizeof(u8) * 512 * SECTOR_NUM);

	ts = get_timer(0);
	err = nvme_write(0, 0, wbuf, SECTOR_NUM, 0);
	if (err) {
		printf("fail to write nvme\n");
		return err;
	}

	time_elapsed = get_timer(0) - ts;
	printf("nvme write speed is %d MB/s\n",
	       (SECTOR_NUM >> 1) / time_elapsed);

	ts = get_timer(0);
	err = nvme_read(0, 0, rbuf, SECTOR_NUM);
	if (err) {
		printf("fail to read nvme\n");
		return err;
	}

	time_elapsed = get_timer(0) - ts;
	printf("nvme read speed is %d MB/s\n",
	       (SECTOR_NUM >> 1) / time_elapsed);

	for (i = 0; i < 512 * SECTOR_NUM; i++) {
		if (*(rbuf + i) != *(wbuf + i)) {
			printf("compare err\n");
			err = -EINVAL;
			break;
		}
	}

	if (err)
		printf("nvme_test failed, test number = %lld\n", data);
	else
		printf("nvme_test successfully, test number = %lld\n", data);

	data++;

	if (should_retry)
		goto retry;

	printf("finish nvme test\n");

	return nvme_flush();
}
