/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "compiler.h"
#include <version.h>
#include <sha.h>
#include <u-boot/crc.h>

extern uint32_t crc32_rk (uint32_t, const unsigned char *, uint32_t);

#define MODE_PACK             0
#define MODE_UNPACK           1
#define UBOOT_NUM             4
#define UBOOT_MAX_SIZE        1024*1024
#define U_BOOT_VERSION_STRING U_BOOT_VERSION " (" U_BOOT_DATE " - " \
	U_BOOT_TIME ")" CONFIG_IDENT_STRING

#define LOADER_MAGIC_SIZE     16
#define LOADER_HASH_SIZE      32
#define CMD_LINE_SIZE         512

#define RK_UBOOT_MAGIC        "LOADER  "
#define RK_UBOOT_SIGN_TAG     0x4E474953
#define RK_UBOOT_SIGN_LEN     256
typedef struct tag_second_loader_hdr
{
	uint8_t magic[LOADER_MAGIC_SIZE];  // "LOADER  "

	uint32_t loader_load_addr;           /* physical load addr ,default is 0x60000000*/
	uint32_t loader_load_size;           /* size in bytes */
	uint32_t crc32;                      /* crc32 */
	uint32_t hash_len;                   /* 20 or 32 , 0 is no hash*/
	uint8_t hash[LOADER_HASH_SIZE];     /* sha */

	uint8_t reserved[1024-32-32];
	uint32_t signTag; /* 0x4E474953 */
	uint32_t signlen; /* maybe 128 or 256 */
	uint8_t rsaHash[256]; /* maybe 128 or 256, using max size 256 */
	uint8_t reserved2[2048-1024-256-8];
}second_loader_hdr;


void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [--pack|--unpack] file\n", prog);
}

int main (int argc, char *argv[])
{
	int	mode, size, i;
	FILE	*fi, *fo;
	second_loader_hdr hdr;
	char *buf = 0;

	if (argc < 4) {
		usage(argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!strcmp(argv[1], "--pack"))
		mode = MODE_PACK;
	else if (!strcmp(argv[1], "--unpack"))
		mode = MODE_UNPACK;
	else {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	fi = fopen(argv[2], "rb");
	if (!fi) {
		perror(argv[2]);
		exit (EXIT_FAILURE);
	}

	fo = fopen(argv[3], "wb");
	if (!fo) {
		perror(argv[3]);
		exit (EXIT_FAILURE);
	}

	buf = calloc(UBOOT_MAX_SIZE, UBOOT_NUM);
	if (!buf) {
		perror(argv[3]);
		exit (EXIT_FAILURE);
	}
	//memset(buf, 0, UBOOT_NUM*UBOOT_MAX_SIZE);
	if(mode == MODE_PACK){
		printf("pack input %s \n", argv[2]);
		fseek( fi, 0, SEEK_END );
		size = ftell(fi);
		fseek( fi, 0, SEEK_SET );
		printf("pack file size:%d \n", size);
		if(size > UBOOT_MAX_SIZE - sizeof(second_loader_hdr)){
			perror(argv[3]);
			exit (EXIT_FAILURE);
		}
		memset(&hdr, 0, sizeof(second_loader_hdr));
		strcpy((char *)hdr.magic, RK_UBOOT_MAGIC);
		hdr.loader_load_addr = CONFIG_SYS_TEXT_BASE;
		hdr.loader_load_size = size;
		if (!fread(buf + sizeof(second_loader_hdr), size, 1, fi)) {
			exit (EXIT_FAILURE);
		}
		hdr.crc32 = crc32_rk(0, (const unsigned char *)buf + sizeof(second_loader_hdr), size);
		printf("crc = 0x%08x\n", hdr.crc32);

		SHA_CTX ctx;
		uint8_t* sha;
		hdr.hash_len = (SHA_DIGEST_SIZE > LOADER_HASH_SIZE) ? LOADER_HASH_SIZE : SHA_DIGEST_SIZE;
		SHA_init(&ctx);
		SHA_update(&ctx, buf + sizeof(second_loader_hdr), size);
		SHA_update(&ctx, &hdr.loader_load_addr, sizeof(hdr.loader_load_addr));
		SHA_update(&ctx, &hdr.loader_load_size, sizeof(hdr.loader_load_size));
		SHA_update(&ctx, &hdr.hash_len, sizeof(hdr.hash_len));
		sha = (uint8_t*)SHA_final(&ctx);
		memcpy(hdr.hash, sha, hdr.hash_len);

		printf("uboot version:%s\n",U_BOOT_VERSION_STRING);
		memcpy(buf, &hdr, sizeof(second_loader_hdr));
		for(i=0; i<UBOOT_NUM; i++){
			fwrite(buf, UBOOT_MAX_SIZE, 1, fo);
		}
		printf("pack %s success! \n", argv[3]);
	}else if (mode == MODE_UNPACK){
		printf("unpack input %s \n", argv[2]);
		memset(&hdr, 0, sizeof(second_loader_hdr));
		if(!fread(&hdr, sizeof(second_loader_hdr), 1, fi)) {
			exit (EXIT_FAILURE);
		}
		if(!fread(buf, hdr.loader_load_size, 1, fi)) {
			exit (EXIT_FAILURE);
		}
		fwrite(buf, hdr.loader_load_size, 1, fo);
		printf("unpack %s success! \n", argv[3]);
	}
	free(buf);
	fclose(fi);
	fclose(fo);

	return 0;
}

