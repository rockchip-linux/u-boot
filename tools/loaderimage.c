/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "compiler.h"
#include <version.h>
#include <config.h>
#include <sha.h>
#include <u-boot/sha256.h>
#include <u-boot/crc.h>

extern uint32_t crc32_rk (uint32_t, const unsigned char *, uint32_t);

#define OPT_PACK		"--pack"
#define OPT_UNPACK		"--unpack"
#define OPT_UBOOT		"--uboot"
#define OPT_TRUSTOS		"--trustos"
#define OPT_SIZE		"--size"

/* pack or unpack */
#define MODE_PACK		0
#define MODE_UNPACK		1

/* image type */
#define IMAGE_UBOOT		0
#define IMAGE_TRUST		1

/* magic and hash size */
#define LOADER_MAGIC_SIZE	16
#define LOADER_HASH_SIZE	32

/* uboot image config */
#define UBOOT_NAME		"uboot"
#ifdef CONFIG_RK_NVME_BOOT_EN
#define UBOOT_NUM		2
#define UBOOT_MAX_SIZE		512 * 1024
#else
#define UBOOT_NUM		4
#define UBOOT_MAX_SIZE		1024 * 1024
#endif
#define UBOOT_VERSION_STRING	U_BOOT_VERSION " (" U_BOOT_DATE " - " \
				U_BOOT_TIME ")" CONFIG_IDENT_STRING

#define RK_UBOOT_MAGIC		"LOADER  "
#define RK_UBOOT_RUNNING_ADDR	CONFIG_SYS_TEXT_BASE

/* trust image config */
#define TRUST_NAME		"trustos"
#define TRUST_NUM		4
#define TRUST_MAX_SIZE		1024 * 1024
#define TRUST_VERSION_STRING	"Trust os"

#define RK_TRUST_MAGIC		"TOS     "
#define RK_TRUST_RUNNING_ADDR	(CONFIG_RAM_PHY_START + SZ_128M + SZ_4M)

typedef struct tag_second_loader_hdr {
	uint8_t magic[LOADER_MAGIC_SIZE];	/* magic */

	uint32_t loader_load_addr;		/* physical load addr */
	uint32_t loader_load_size;		/* size in bytes */
	uint32_t crc32;				/* crc32 */
	uint32_t hash_len;			/* 20 or 32 , 0 is no hash*/
	uint8_t hash[LOADER_HASH_SIZE];		/* sha */

	uint8_t reserved[1024-32-32];
	uint32_t signTag;			/* 0x4E474953 */
	uint32_t signlen;			/* maybe 128 or 256 */
	uint8_t rsaHash[256];			/* maybe 128 or 256, using max size 256 */
	uint8_t reserved2[2048-1024-256-8];
} second_loader_hdr;


void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [--pack|--unpack] [--uboot|--trustos] file_in file_out [load_addr]\n", prog);
}

unsigned int str2hex(char *str)
{
	int i=0;
	unsigned int value = 0;

	if(*str == '0' && ( *(str+1) == 'x' || *(str+1) == 'X' ) )
		str += 2;
	if( *str == 'x' || *str == 'X' )
		str += 1;

	for(i=0; *str!='\0'; i++,++str)
	{
		if(*str>='0' && *str<='9')
			value = value*16+*str-'0';
		else if(*str>='a' && *str<='f')
			value = value*16+*str-'a'+10;
		else if(*str>='A' && *str<='F')
			value = value*16+*str-'A'+10;
		else
			break;
	}
	return value;
}

int main (int argc, char *argv[])
{
	int			mode = -1, image = -1;
	int			max_size, max_num;
	int			size, i;
	uint32_t		loader_addr, in_loader_addr = -1;
	char			*magic, *version, *name;
	FILE			*fi, *fo;
	second_loader_hdr	hdr;
	char 			*buf = 0;
	uint32_t		in_size = 0, in_num = 0;
	char 			*file_in = NULL, *file_out = NULL;

	if (argc < 5) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], OPT_PACK)) {
			mode = MODE_PACK;
		} else if (!strcmp(argv[i], OPT_UNPACK)) {
			mode = MODE_UNPACK;
		} else if (!strcmp(argv[i], OPT_UBOOT)) {
			image = IMAGE_UBOOT;
			file_in = argv[++i];
			file_out = argv[++i];
			/* detect whether loader address is delivered */
			if ((argv[i+1]) && (strncmp(argv[i+1], "--", 2)))
				in_loader_addr = str2hex(argv[++i]);
		} else if (!strcmp(argv[i], OPT_TRUSTOS)) {
			image = IMAGE_TRUST;
			file_in = argv[++i];
			file_out = argv[++i];
			/* detect whether loader address is delivered */
			if ((argv[i+1]) && (strncmp(argv[i+1], "--", 2)))
				in_loader_addr = str2hex(argv[++i]);
		} else if (!strcmp(argv[i], OPT_SIZE)) {
			in_size = strtoul(argv[++i], NULL, 10);
			/* Must be 512kb align due to preloader detects every 512kb */
			if (in_size % 512) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			in_size *= 1024;

			in_num = strtoul(argv[++i], NULL, 10);
		} else {
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (!file_in || !file_out) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* config image information */
	if (image == IMAGE_UBOOT) {
		name = UBOOT_NAME;
		magic = RK_UBOOT_MAGIC;
		version = UBOOT_VERSION_STRING;
		max_size = in_size ? in_size : UBOOT_MAX_SIZE;
		max_num = in_num ? in_num : UBOOT_NUM;
		loader_addr = (in_loader_addr == -1) ?
			RK_UBOOT_RUNNING_ADDR : in_loader_addr;
	} else if (image == IMAGE_TRUST) {
		name = TRUST_NAME;
		magic = RK_TRUST_MAGIC;
		version = TRUST_VERSION_STRING;
		max_size = in_size ? in_size : TRUST_MAX_SIZE;
		max_num = in_num ? in_num : TRUST_NUM;
		loader_addr = (in_loader_addr == -1) ?
			RK_TRUST_RUNNING_ADDR : in_loader_addr;
	} else {
		exit(EXIT_FAILURE);
	}

	/* file in */
	fi = fopen(file_in, "rb");
	if (!fi) {
		perror(file_in);
		exit(EXIT_FAILURE);
	}

	/* file out */
	fo = fopen(file_out, "wb");
	if (!fo) {
		perror(file_out);
		exit (EXIT_FAILURE);
	}

	printf("\n load addr is 0x%x!\n", loader_addr);

	buf = calloc(max_size, max_num);
	if (!buf) {
		perror(file_out);
		exit (EXIT_FAILURE);
	}

	if (mode == MODE_PACK) {
		printf("pack input %s \n", file_in);
		fseek(fi, 0, SEEK_END);
		size = ftell(fi);
		fseek(fi, 0, SEEK_SET);
		printf("pack file size: %d \n", size);
		if (size > max_size - sizeof(second_loader_hdr)) {
			perror(file_out);
			exit (EXIT_FAILURE);
		}
		memset(&hdr, 0, sizeof(second_loader_hdr));
		strcpy((char *)hdr.magic, magic);
		hdr.loader_load_addr = loader_addr;
		if (!fread(buf + sizeof(second_loader_hdr), size, 1, fi))
			exit (EXIT_FAILURE);

		/* Aligned size to 4-byte, Rockchip HW Crypto need 4-byte align */
		size = (((size + 3) >> 2 ) << 2);
		hdr.loader_load_size = size;

		hdr.crc32 = crc32_rk(0, (const unsigned char *)buf + sizeof(second_loader_hdr), size);
		printf("crc = 0x%08x\n", hdr.crc32);

#ifndef CONFIG_SECUREBOOT_SHA256
		SHA_CTX ctx;
		uint8_t *sha;
		hdr.hash_len = (SHA_DIGEST_SIZE > LOADER_HASH_SIZE) ? LOADER_HASH_SIZE : SHA_DIGEST_SIZE;
		SHA_init(&ctx);
		SHA_update(&ctx, buf + sizeof(second_loader_hdr), size);
		SHA_update(&ctx, &hdr.loader_load_addr, sizeof(hdr.loader_load_addr));
		SHA_update(&ctx, &hdr.loader_load_size, sizeof(hdr.loader_load_size));
		SHA_update(&ctx, &hdr.hash_len, sizeof(hdr.hash_len));
		sha = (uint8_t *)SHA_final(&ctx);
		memcpy(hdr.hash, sha, hdr.hash_len);
#else
		sha256_context ctx;
		uint8_t hash[LOADER_HASH_SIZE];

		memset(hash, 0, LOADER_HASH_SIZE);

		hdr.hash_len = 32; /* sha256 */
		sha256_starts(&ctx);
		sha256_update(&ctx, (void *)buf + sizeof(second_loader_hdr), size);
		sha256_update(&ctx, (void *)&hdr.loader_load_addr, sizeof(hdr.loader_load_addr));
		sha256_update(&ctx, (void *)&hdr.loader_load_size, sizeof(hdr.loader_load_size));
		sha256_update(&ctx, (void *)&hdr.hash_len, sizeof(hdr.hash_len));
		sha256_finish(&ctx, hash);
		memcpy(hdr.hash, hash, hdr.hash_len);
#endif /* CONFIG_SECUREBOOT_SHA256 */

		printf("%s version: %s\n", name, version);
		memcpy(buf, &hdr, sizeof(second_loader_hdr));
		for(i = 0; i < max_num; i++)
			fwrite(buf, max_size, 1, fo);

		printf("pack %s success! \n", file_out);
	} else if (mode == MODE_UNPACK) {
		printf("unpack input %s \n", file_in);
		memset(&hdr, 0, sizeof(second_loader_hdr));
		if (!fread(&hdr, sizeof(second_loader_hdr), 1, fi))
			exit (EXIT_FAILURE);

		if (!fread(buf, hdr.loader_load_size, 1, fi))
			exit (EXIT_FAILURE);

		fwrite(buf, hdr.loader_load_size, 1, fo);
		printf("unpack %s success! \n", file_out);
	}

	free(buf);
	fclose(fi);
	fclose(fo);

	return 0;
}

