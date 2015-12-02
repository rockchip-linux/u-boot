/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>
#include <config.h>
#include <u-boot/crc.h>

extern uint32_t crc32_rk (uint32_t, const unsigned char *, uint32_t);

#define SZ_4M                           0x00400000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000

void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <image>\n", prog);
}

/*
 * Neutralize little endians.
 */
uint32_t le_uint32(uint32_t x)
{
	uint32_t val;
	uint8_t *p = (uint8_t *)(&x);

	val =  (*p++ & 0xff) << 0;
	val |= (*p++ & 0xff) << 8;
	val |= (*p++ & 0xff) << 16;
	val |= (*p   & 0xff) << 24;

	return val;
}

int main (int argc, char *argv[])
{
	FILE *fp;
	uint32_t blocks = 0;

	fp = fopen(argv[1], "rb");
	if (!fp) {
		perror(argv[1]);
		return -1;
	}

	struct stat sb;
	int ret = stat(argv[1], &sb);
	if (!fp || ret) {
		perror(argv[1]);
		return -1;
	}
	blocks = sb.st_size / RK_BLK_SIZE;
	if (sb.st_size % RK_BLK_SIZE) {
		printf("size should align %d", RK_BLK_SIZE);
		return -1;
	}

	printf("totle blocks:0x%08x\n", blocks);

#ifdef CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH
	uint32_t buf_size = CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH;
#else
	uint32_t buf_size = 16*1024*1024;
#endif
	void* buf = malloc(buf_size);
	uint16_t buf_blocks = buf_size / RK_BLK_SIZE;
	uint32_t offset = 0;
#ifndef CONFIG_QUICK_CHECKSUM
	uint32_t* crc_array = (uint32_t*) malloc(buf_size);
	uint16_t crc_counts = 0;
	uint32_t checksum = 0;
#else
	long long unsigned int checksum = 0;
#endif
	while (blocks > 0) {
		uint16_t read_blocks = blocks > buf_blocks? buf_blocks : blocks;

		if (fread(buf, read_blocks * RK_BLK_SIZE, 1, fp) != 1) {
			printf("read failed, offset:0x%08x, blocks:0x%08x\n",
					offset, read_blocks);
			return -1;
		}
		offset += read_blocks;
		blocks -= read_blocks;
#ifndef CONFIG_QUICK_CHECKSUM
		crc_array[crc_counts] = crc32_rk(0, buf, read_blocks * RK_BLK_SIZE);
		printf("offset:0x%08x, blocks:0x%08x, crc:0x%08lx\n",
				offset, read_blocks, crc_array[crc_counts]);
		crc_counts++;
#else
		int i = 0;
		uint32_t* data = (uint32_t*) buf;
		for (i = 0;i < read_blocks * RK_BLK_SIZE >> 2;i++)
			checksum += le_uint32(data[i]);
		printf("offset:0x%08x, blocks:0x%08x, checksum:0x%016llx\n",
				offset, read_blocks, checksum);
#endif
	}

#ifndef CONFIG_QUICK_CHECKSUM
	//3:compute whole checksum
	checksum = (crc_counts == 1)? crc_array[0] :
		crc32_rk(0, (unsigned char*)crc_array, sizeof(uint32_t) * crc_counts);
	printf("whole checksum:0x%08lx\n", checksum);
	free(crc_array);
#else
	printf("whole checksum:0x%016llx\n", checksum);
#endif
	free(buf);

	fclose (fp);
	return 0;
}
