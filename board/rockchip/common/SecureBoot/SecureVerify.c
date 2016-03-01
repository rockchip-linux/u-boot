/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "../config.h"
#include <u-boot/sha256.h>

DECLARE_GLOBAL_DATA_PTR;

extern  uint32   RSA_KEY_TAG;
extern  uint32   RSA_KEY_LENGTH;
extern  uint32   RSA_KEY_DATA;
static	uint16   *RSK_KEY;

extern void P_RC4(unsigned char *buf, unsigned short len);


static bool SecureNSModeVerifyLoader(RK28BOOT_HEAD *hdr)
{
#define RSA_KEY_OFFSET 0x10 /* according to dumped data, the key is here. */
#define RSA_KEY_LEN    0x102 /* 258, public key's length */
	char buf[RK_BLK_SIZE];

	memcpy(buf, (void *)hdr + hdr->uiFlashBootOffset, RK_BLK_SIZE);
	P_RC4((unsigned char *)buf, RK_BLK_SIZE);

	if (buf[RSA_KEY_OFFSET] != 0 || buf[RSA_KEY_OFFSET + 1] != 4)
		PRINT_I("Unsigned loader!\n");
	if (gDrmKeyInfo.publicKeyLen == 0) {
		PRINT_I("NS Mode allow flash unsigned loader.\n");
		return true;
	}

#if 0
	printf("dump new loader's key:\n");
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 16; j++)
			printf("%02x", buf[RSA_KEY_OFFSET + i * 16 + j]);
		printf("\n");
	}
#endif

	return !memcmp(buf + RSA_KEY_OFFSET, gDrmKeyInfo.publicKey, RSA_KEY_LEN);
}


static bool SecureNSModeSignCheck(uint8 *rsaHash, uint8 *Hash, uint8 length)
{
	uint8  decodedHash[40];

	if (0 == rsaDecodeHash(decodedHash, rsaHash, (uint8 *)RSK_KEY, length)) {
		if (0 == memcmp(Hash, decodedHash, 20)) {
			PRINT_I("Sign OK\n");
			return true;
		}
	}

	return false;
}


static bool SecureNSModeUbootImageShaCheck(second_loader_hdr *hdr)
{
#ifndef SECUREBOOT_CRYPTO_EN
	uint8_t *sha;
	SHA_CTX ctx;
	int size = (SHA_DIGEST_SIZE > hdr->hash_len) ? hdr->hash_len : SHA_DIGEST_SIZE;

	SHA_init(&ctx);
	SHA_update(&ctx, (void *)hdr + sizeof(second_loader_hdr), hdr->loader_load_size);
	SHA_update(&ctx, &hdr->loader_load_addr, sizeof(hdr->loader_load_addr));
	SHA_update(&ctx, &hdr->loader_load_size, sizeof(hdr->loader_load_size));
	SHA_update(&ctx, &hdr->hash_len, sizeof(hdr->hash_len));
	sha = SHA_final(&ctx);
#else
	uint32 size;
	uint8 *sha;
	uint32 hwDataHash[8];

	size = hdr->loader_load_size + sizeof(hdr->loader_load_size) \
		 + sizeof(hdr->loader_load_addr) + sizeof(hdr->hash_len);

	CryptoSHAInit(size, 160);
	/* rockchip's second level image. */
	CryptoSHAStart((uint32 *)((void *)hdr + sizeof(second_loader_hdr)), hdr->loader_load_size);
	CryptoSHAStart((uint32 *)&hdr->loader_load_addr, sizeof(hdr->loader_load_addr));
	CryptoSHAStart((uint32 *)&hdr->loader_load_size, sizeof(hdr->loader_load_size));
	CryptoSHAStart((uint32 *)&hdr->hash_len, sizeof(hdr->hash_len));

	CryptoSHAEnd(hwDataHash);

	sha = (uint8 *)hwDataHash;
	size = (SHA_DIGEST_SIZE > hdr->hash_len) ? hdr->hash_len : SHA_DIGEST_SIZE;
#endif

#if 0
	int i = 0;
	printf("\nreal sha:\n");
	for (i = 0; i < size; i++)
		printf("%02x", (char)sha[i]);
	printf("\nsha from image header:\n");
	for (i = 0; i < size; i++)
		printf("%02x", ((char *)hdr->hash)[i]);
	printf("\n");
#endif

	return !memcmp(hdr->hash, sha, size);
}


static bool SecureNSModeVerifyUbootImageSign(second_loader_hdr *hdr)
{
	/* verify uboot iamge. */
	if (memcmp(hdr->magic, RK_UBOOT_MAGIC, sizeof(RK_UBOOT_MAGIC)) != 0) {
		PRINT_E("unrecognized image format!\n");
		return false;
	}

	/* check image sha, make sure image is ok. */
	if (!SecureNSModeUbootImageShaCheck(hdr)) {
		PRINT_E("uboot sha mismatch!\n");
		return false;
	}

	/* signed image, check with signature. */
	if (SecureBootEn) {
		if (gDrmKeyInfo.publicKeyLen == 0) { /* check loader publickey */
			PRINT_E("NS Mode allow flash unsigned loader.\n");
			return false;
		}

		if (hdr->signTag != RK_UBOOT_SIGN_TAG) { /* check image sign tag */
			PRINT_E("unsigned image!\n");
			return false;
		}

		/* check rsa sign here. */
		if (SecureNSModeSignCheck(hdr->rsaHash, hdr->hash, hdr->signlen)) {
			return true;
		} else {
			PRINT_E("signature mismatch!\n");
			return false;
		}
	}

	/* secureboot disable */
	return true;
}


static bool SecureNSModeBootImageShaCheck(rk_boot_img_hdr *boothdr)
{
#ifndef SECUREBOOT_CRYPTO_EN
	uint8_t *sha;
	SHA_CTX ctx;
	int size = (SHA_DIGEST_SIZE > sizeof(boothdr->id)) ? sizeof(boothdr->id) : SHA_DIGEST_SIZE;

	SHA_init(&ctx);

	/* Android image */
	SHA_update(&ctx, (void *)(unsigned long)boothdr->kernel_addr, boothdr->kernel_size);
	SHA_update(&ctx, &boothdr->kernel_size, sizeof(boothdr->kernel_size));
	SHA_update(&ctx, (void *)(unsigned long)boothdr->ramdisk_addr, boothdr->ramdisk_size);
	SHA_update(&ctx, &boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	SHA_update(&ctx, (void *)(unsigned long)boothdr->second_addr, boothdr->second_size);
	SHA_update(&ctx, &boothdr->second_size, sizeof(boothdr->second_size));

	/* rockchip's image add information. */
	SHA_update(&ctx, &boothdr->tags_addr, sizeof(boothdr->tags_addr));
	SHA_update(&ctx, &boothdr->page_size, sizeof(boothdr->page_size));
	SHA_update(&ctx, &boothdr->unused, sizeof(boothdr->unused));
	SHA_update(&ctx, &boothdr->name, sizeof(boothdr->name));
	SHA_update(&ctx, &boothdr->cmdline, sizeof(boothdr->cmdline));

	sha = SHA_final(&ctx);
#else
	uint32 size;
	uint8 *sha;
	uint32 hwDataHash[8];

	size = boothdr->kernel_size + sizeof(boothdr->kernel_size) \
		+ boothdr->ramdisk_size + sizeof(boothdr->ramdisk_size) \
		+ boothdr->second_size + sizeof(boothdr->second_size) \
		+ sizeof(boothdr->tags_addr) + sizeof(boothdr->page_size) \
		+ sizeof(boothdr->unused) + sizeof(boothdr->name) + sizeof(boothdr->cmdline);

	CryptoSHAInit(size, 160);

	/* Android image. */
	CryptoSHAStart((uint32 *)(unsigned long)boothdr->kernel_addr, boothdr->kernel_size);
	CryptoSHAStart((uint32 *)&boothdr->kernel_size, sizeof(boothdr->kernel_size));
	CryptoSHAStart((uint32 *)(unsigned long)boothdr->ramdisk_addr, boothdr->ramdisk_size);
	CryptoSHAStart((uint32 *)&boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	CryptoSHAStart((uint32 *)(unsigned long)boothdr->second_addr, boothdr->second_size);
	CryptoSHAStart((uint32 *)&boothdr->second_size, sizeof(boothdr->second_size));

	/* only rockchip's image add. */
	CryptoSHAStart((uint32 *)&boothdr->tags_addr, sizeof(boothdr->tags_addr));
	CryptoSHAStart((uint32 *)&boothdr->page_size, sizeof(boothdr->page_size));
	CryptoSHAStart((uint32 *)&boothdr->unused, sizeof(boothdr->unused));
	CryptoSHAStart((uint32 *)&boothdr->name, sizeof(boothdr->name));
	CryptoSHAStart((uint32 *)&boothdr->cmdline, sizeof(boothdr->cmdline));

	CryptoSHAEnd(hwDataHash);

	sha = (uint8 *)hwDataHash;
	size = SHA_DIGEST_SIZE > sizeof(boothdr->id) ? sizeof(boothdr->id) : SHA_DIGEST_SIZE;
#endif

#if 0
	int i = 0;
	printf("\nreal sha:\n");
	for (i = 0; i < size; i++)
		printf("%02x", (char)sha[i]);
	printf("\nsha from image header:\n");
	for (i = 0; i < size; i++)
		printf("%02x", ((char *)boothdr->id)[i]);
	printf("\n");
#endif

	return !memcmp(boothdr->id, sha, size);
}


static bool SecureNSModeVerifyBootImageSign(rk_boot_img_hdr* boothdr)
{
	/* verify boot/recovery image */
	if (memcmp(boothdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0)
		return false;

	/* check image sha, make sure image is ok. */
	if (!SecureNSModeBootImageShaCheck(boothdr)) {
		PRINT_E("boot or recovery image sha mismatch!\n");
		return false;
	}

	/* signed image, check with signature. */
	if (SecureBootEn) {
		/* check loader publickey */
		if (gDrmKeyInfo.publicKeyLen == 0) {
			PRINT_E("NS Mode allow flash unsigned loader.\n");
			return false;
		}

		/* check image sign tag */
		if (boothdr->signTag != SECURE_BOOT_SIGN_TAG) {
			return false;
		}

		/* check rsa sign here. */
		if (SecureNSModeSignCheck((uint8 *)boothdr->rsaHash, (uint8 *)boothdr->id, boothdr->signlen)) {
			return true;
		} else {
			PRINT_E("signature mismatch!\n");
			return false;
		}
	}

	/* secureboot disable */
	return true;
}


static bool SecureNSModeBootImageCheck(rk_boot_img_hdr *hdr, int unlocked)
{
	rk_boot_img_hdr *boothdr = (rk_boot_img_hdr *)hdr;

	SecureBootCheckOK = 0;

	/* if boot/recovery not include kernel */
	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		/* if soft crc32 checking kernel and boot image enable, it will take time */
#ifdef CONFIG_BOOTRK_RK_IMAGE_CHECK
		uint32 crc32 = 0;

		debug("%s: Kernel image CRC32 check...\n", __func__);
		crc32 = CRC_32CheckBuffer((unsigned char *)(unsigned long)hdr->kernel_addr, hdr->kernel_size + 4);
		if (!crc32) {
			PRINT_E("kernel image CRC32 failed!\n");
			return false;
		}
		debug("Kernel CRC32 check ok.\n");

		debug("%s: Boot image CRC32 check...\n", __func__);
		crc32 = CRC_32CheckBuffer((unsigned char *)(unsigned long)hdr->ramdisk_addr, hdr->ramdisk_size + 4);
		if (!crc32) {
			PRINT_E("Boot image CRC32 failed!\n");
			return false;
		}
		debug("Boot CRC32 check ok.\n");
#endif /* CONFIG_BOOTRK_RK_IMAGE_CHECK */
		return true;
	}

	/* if sha checking boot image, it will take time */
#if defined(CONFIG_BOOTRK_OTA_IMAGE_CHECK) || defined(SECUREBOOT_CRYPTO_EN)
	/* check image sha, make sure image is ok. */
	if (!SecureNSModeBootImageShaCheck(boothdr)) {
		PRINT_E("boot or recovery image sha mismatch!\n");
		return false;
	}
#endif

	/* signed image, check with signature. */
	if (!unlocked && SecureBootEn && (boothdr->signTag == SECURE_BOOT_SIGN_TAG)) {
		if (SecureNSModeSignCheck(boothdr->rsaHash, (uint8 *)boothdr->id, boothdr->signlen)) {
			SecureBootCheckOK = 1;
		} else {
			SecureBootCheckOK = 0;
			PRINT_E("SecureNSModeSignCheck failed\n");
		}
	}

	return true;
}


static void SecureNSModeLockLoader(void)
{
	if (RSK_KEY[0] == 0X400) {
		if (gDrmKeyInfo.secureBootLock == 0) {
			gDrmKeyInfo.secureBootLock = 1;
			gDrmKeyInfo.secureBootLockKey = 0;
			StorageSysDataStore(1, &gDrmKeyInfo);
		}
	}
}


static bool SecureNSModeKeyCheck(uint8 *pKey)
{
	if (rsaCheckMD5(pKey, pKey + 256, (uint8 *)gDrmKeyInfo.publicKey, 128) == 0) {
		return true;
	}

	return false;
}


static void SecureNSModeInit(void)
{
	uint32 updataFlag = 0;

	SecureBootEn = 1;
	SecureBootCheckOK = 0;
	RSK_KEY = (uint16 *)&RSA_KEY_DATA;
#if 0
{
	int i;
	char *p = RSK_KEY;
	printf("RSA_KEY_DATA============================================\n");
	for(i=0;i<32;i++)
	printf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++,*p++);
	printf("RSA_KEY_DATA============================================\n");
}
#endif
	if ((RSK_KEY[0] != 0X400)) {
		SecureBootEn = 0;
		RkPrintf("unsigned!\n");
	}

	SecureBootLock = 0;

	if (StorageSysDataLoad(1, &gDrmKeyInfo) == FTL_OK) {
		updataFlag = 0;
		SecureBootLock = gDrmKeyInfo.secureBootLock;
		if (SecureBootLock != 1)
			SecureBootLock = 0;

		if (gDrmKeyInfo.drmtag != 0x4B4D5244) {
			gDrmKeyInfo.drmtag = 0x4B4D5244;
			gDrmKeyInfo.drmLen = 504;
			gDrmKeyInfo.keyBoxEnable = 0;
			gDrmKeyInfo.drmKeyLen = 0;
			gDrmKeyInfo.publicKeyLen = 0;
			gDrmKeyInfo.secureBootLock = 0;
			gDrmKeyInfo.secureBootLockKey = 0;
			updataFlag = 1;
		}

		if (RSK_KEY[0] == 0X400) {
#ifdef SECURE_BOOT_SET_LOCK_ALWAY
			if (gDrmKeyInfo.secureBootLock == 0) {
				gDrmKeyInfo.secureBootLock = 1;
				gDrmKeyInfo.secureBootLockKey = 0;
				updataFlag = 1;
			}
#endif
			if (gDrmKeyInfo.publicKeyLen == 0) {
				/* 没有公钥，是第一次才开启keyBoxEnable */
				gDrmKeyInfo.publicKeyLen = 0x100;
				ftl_memcpy(gDrmKeyInfo.publicKey, RSK_KEY, 0x104);
				updataFlag = 1;
				gDrmKeyInfo.drmKeyLen = 0;
				gDrmKeyInfo.keyBoxEnable = 1;
				gDrmKeyInfo.secureBootLockKey = 0;
				memset( gDrmKeyInfo.drmKey, 0, 0x80);
#ifdef SECURE_BOOT_LOCK
				gDrmKeyInfo.secureBootLock = 1;
#endif
			} else if (memcmp(gDrmKeyInfo.publicKey, RSK_KEY, 0x100) != 0) {
				/* 如果已经存在公钥，并且公钥被替换了，那么关闭 */
				if (memcmp(gDrmKeyInfo.publicKey + 4, RSK_KEY, 0x100) == 0) {
					ftl_memcpy(gDrmKeyInfo.publicKey, RSK_KEY, 0x104);
					updataFlag = 1;
				} else {
					gDrmKeyInfo.keyBoxEnable = 0; /* 暂时不启用这个功能 */
					SecureBootEn = 0;
					RkPrintf("E:pKey!\n");
				}
			}
			else if(SecureBootEn && (gDrmKeyInfo.keyBoxEnable == 0))
			{
				gDrmKeyInfo.keyBoxEnable = 1;
				updataFlag = 1;
			}
		}

		if (updataFlag) {
			updataFlag = 0;
			if (FTL_ERROR == StorageSysDataStore(1, &gDrmKeyInfo)) {
				;/* TODO:SysDataStore异常处理 */
			}
		}
	}

	if (StorageSysDataLoad(0, &gBootConfig) == FTL_OK) {
		updataFlag = 0;
		if (gBootConfig.bootTag != 0x44535953) {
			gBootConfig.bootTag = 0x44535953;
			gBootConfig.bootLen = 504;
			gBootConfig.bootMedia = 0; /* TODO: boot 选择 */
			gBootConfig.BootPart = 0;
			gBootConfig.secureBootEn = 0; /* SecureBootEn, 默认disable */
			updataFlag = 1;
		} else {
#ifndef SECURE_BOOT_ENABLE_ALWAY
			if (gBootConfig.secureBootEn == 0)
				SecureBootEn = 0;
#endif
		}

		if (updataFlag) {
			updataFlag = 0;
			if (FTL_ERROR == StorageSysDataStore(0, &gBootConfig)) {
				;/* TODO:SysDataStore异常处理 */
			}
		}
	} else {
		RkPrintf("no sys part.\n");
		SecureBootEn = 0;
	}
}


#ifdef SECUREBOOT_CRYPTO_EN

static uint32 g_rsa_key_buf[528];

#define SecureSHAInit(MsgLen, hash_bits)	CryptoSHAInit(MsgLen, hash_bits)
#define SecureSHAUpdate(data, DataLen)		CryptoSHAStart(data, DataLen)
#define SecureSHAFinish(result)			CryptoSHAEnd(result)
#define SecureSHACheck(InHash)			CryptoSHACheck(InHash)
#define SecureRSAVerify(pHead, SigOffset)	CryptoRSAVerify(pHead, SigOffset)

#if !defined(CONFIG_SECURE_RSA_KEY_IN_RAM)
static int32 SecureRKModeChkPubkey(uint32 *pKey)
{
	uint32 hash[8];	/* max 256bit */
	uint32 HashOTP[OTP_HASH_LEN / 4];

	/* Get the public key HASH from the Efuse */
	SecureEfuseRead((void *)(unsigned long)SECURE_EFUSE_BASE_ADDR, HashOTP, OTP_HASH_ADDR, OTP_HASH_LEN);

	SecureSHAInit(PUBLIC_KEY_LEN, 256);
	SecureSHAUpdate(pKey, PUBLIC_KEY_LEN);
	SecureSHAFinish(hash);

	/* compare the result with Efuse data */
	return memcmp((uint8 *)&hash, (uint8 *)&HashOTP, OTP_HASH_LEN); /* audi, audis only check 64bit */
}


static int32 SecureRKModeGetRSAKey(void)
{
	int i = 0;

	if (g_rsa_key_buf[0] == 0xb86d753c) {
		for (i = 0; i < 4; i++) {
			P_RC4((uint8 *)(g_rsa_key_buf + 132 * i), 512);
			if (i > 0)
				memcpy(g_rsa_key_buf + 128 * i, g_rsa_key_buf + 132 * i, 512);
		}
	} else if (g_rsa_key_buf[0] == 0x4B415352) {
		for (i = 0; i < 4; i++) {
			if (i > 0)
				memcpy(g_rsa_key_buf + 128 * i, g_rsa_key_buf + 132 * i, 512);
		}
	} else {
		return -1;
	}

#if 0
{
	int j = 0, k = 0;
	char *buf = (char *)g_rsa_key_buf;

	printf("dump new loader's key:\n");
	for (j = 0; j < 2048 / 16; j++) {
		for (k = 0; k < 16; k++)
			printf("%02x", buf[j * 16 + k]);
		printf("\n");
	}
}
#endif

	return 0;
}

#else

static int32 SecureRKModeGetRSAKey(void)
{
	BOOT_HEADER *pRSAKey_head = (BOOT_HEADER *)CONFIG_SECURE_RSA_KEY_ADDR;

	if (pRSAKey_head->tag != 0x4B415352) {
		return -1;
	}

	PRINT_E("Secure Boot find rsa key in ram.\n");

	memset((void *)g_rsa_key_buf, 0, sizeof(g_rsa_key_buf));
	memcpy((void *)g_rsa_key_buf, (void *)CONFIG_SECURE_RSA_KEY_ADDR, sizeof(BOOT_HEADER));

#if 0
{
	int j = 0, k = 0;
	char *buf = (char *)g_rsa_key_buf;

	printf("dump new loader's key:\n");
	for (j = 0; j < 2048 / 16; j++) {
		for (k = 0; k < 16; k++)
			printf("%02x", buf[j * 16 + k]);
		printf("\n");
	}
}
#endif

	return 0;
}

static int32 SecureRKModeChkPubkey(uint32 *pKey)
{
	BOOT_HEADER *pkeyHead = (BOOT_HEADER *)g_rsa_key_buf;
	uint32 size = sizeof(pkeyHead->RSA_N) + sizeof(pkeyHead->RSA_E) + sizeof(pkeyHead->RSA_C);

	/* compare rsa key */
	return memcmp((uint8 *)&pKey, (uint8 *)&pkeyHead->RSA_N, size);
}
#endif /* CONFIG_SECURE_RSA_KEY_IN_RAM */


static bool SecureRKModeVerifyLoader(RK28BOOT_HEAD *hdr)
{
	char keybuf[RK_BLK_SIZE * 4];
	BOOT_HEADER *pKeyHead = (BOOT_HEADER *)keybuf;
	int i;

	debug("Loader Head: hdr = 0x%x, flash data offset = 0x%x\n", \
			(uint32)(unsigned long)hdr, hdr->uiFlashDataOffset);

	memcpy(keybuf, (void *)hdr + hdr->uiFlashDataOffset, RK_BLK_SIZE * 4);
	for (i = 0; i < 4; i++)
		P_RC4((unsigned char *)keybuf + RK_BLK_SIZE * i, RK_BLK_SIZE);
#if 0
{
	int j = 0, k = 0;
	char *buf = (char *)keybuf;

	printf("dump new loader's key:\n");
	for (j = 0; j < (2048 / 16); j++) {
		for (k = 0; k < 16; k++)
			printf("%02x", buf[j * 16 + k]);
		printf("\n");
	}
}
#endif

	/* check new loader key */
	if ((pKeyHead->tag == 0x4B415352) && (SecureRKModeChkPubkey(pKeyHead->RSA_N) == 0))
		return true;

	return false;
}


static bool SecureRKModeVerifyUbootImage(second_loader_hdr *uboothdr)
{
	int i;
	uint32 size;
	uint8 *dataHash;
	uint32 rsaResult[8];
	uint32 hwDataHash[8];
	uint8 *rsahash = (uint8 *)rsaResult;
	BOOT_HEADER *pkeyHead = (BOOT_HEADER *)g_rsa_key_buf;

#if 0
{
	int k = 0;
	char *buf = uboothdr->hash;

	printf("Second Loader Head: magic = %s, sign tag = 0x%x, hash len = %d\n",
		uboothdr->magic, uboothdr->signTag, uboothdr->hash_len);

	printf("Second Loader Head: hash data:\n");
	for (k = 0; k < uboothdr->hash_len; k++)
		printf("%02x", buf[k]);
	printf("\n");

	printf("Loader addr = 0x%x, size = 0x%x\n", uboothdr->loader_load_addr, uboothdr->loader_load_size);
}
#endif

	/* verify uboot iamge. */
	if (memcmp(uboothdr->magic, RK_UBOOT_MAGIC, sizeof(RK_UBOOT_MAGIC)) != 0)
		return false;

	if ((uboothdr->signTag != RK_UBOOT_SIGN_TAG) \
			&& (uboothdr->hash_len != 20) && (uboothdr->hash_len != 32))
		return false;

#if 0
{
	if (uboothdr->hash_len == 20) {
		SHA_CTX ctx;

		SHA_init(&ctx);
		SHA_update(&ctx, (void *)uboothdr + sizeof(second_loader_hdr), uboothdr->loader_load_size);
		SHA_update(&ctx, &uboothdr->loader_load_addr, sizeof(uboothdr->loader_load_addr));
		SHA_update(&ctx, &uboothdr->loader_load_size, sizeof(uboothdr->loader_load_size));
		SHA_update(&ctx, &uboothdr->hash_len, sizeof(uboothdr->hash_len));
		dataHash = SHA_final(&ctx);
	} else {
		sha256_context ctx;

		sha256_starts(&ctx);
		sha256_update(&ctx, (void *)(unsigned long)uboothdr + sizeof(second_loader_hdr), uboothdr->loader_load_size);
		sha256_update(&ctx, (void *)(unsigned long)&uboothdr->loader_load_addr, sizeof(uboothdr->loader_load_addr));
		sha256_update(&ctx, (void *)(unsigned long)&uboothdr->loader_load_size, sizeof(uboothdr->loader_load_size));
		sha256_update(&ctx, (void *)(unsigned long)&uboothdr->hash_len, sizeof(uboothdr->hash_len));
		sha256_finish(&ctx, (void *)(unsigned long)hwDataHash);
		dataHash = (uint8 *)hwDataHash;
	}

	int k = 0;
	char *buf = (char *)(unsigned long)dataHash;

	printf("Soft Calc Image hash data:\n");
	for (k = 0; k < uboothdr->hash_len; k++)
		printf("%02x", buf[k]);
	printf("\n");
}
#endif

	size = uboothdr->loader_load_size + sizeof(uboothdr->loader_load_size) \
		 + sizeof(uboothdr->loader_load_addr) + sizeof(uboothdr->hash_len);

	CryptoRSAInit((uint32 *)(uboothdr->rsaHash), pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
	if (uboothdr->hash_len == 20) {
		CryptoSHAInit(size, 160);
	} else {
		CryptoSHAInit(size, 256);
		CryptoSHAInputByteSwap(1);
	}

	/* rockchip's second level image. */
	CryptoSHAStart((uint32 *)((void *)uboothdr + sizeof(second_loader_hdr)), uboothdr->loader_load_size);
	CryptoSHAStart((uint32 *)&uboothdr->loader_load_addr, sizeof(uboothdr->loader_load_addr));
	CryptoSHAStart((uint32 *)&uboothdr->loader_load_size, sizeof(uboothdr->loader_load_size));
	CryptoSHAStart((uint32 *)&uboothdr->hash_len, sizeof(uboothdr->hash_len));

	CryptoSHAEnd(hwDataHash);
	CryptoRSAEnd(rsaResult);

	dataHash = (uint8 *)hwDataHash;

#if 0
{
	int k = 0;
	uint8 *buf = dataHash;

	printf("Crypto Calc Image hash data:\n");
	for (k = 0; k < uboothdr->hash_len; k++)
		printf("%02x", buf[k]);
	printf("\n");
}
#endif

	/* check the hash of image */
	if (memcmp(uboothdr->hash, hwDataHash, uboothdr->hash_len) != 0)
		return false;

	/* check the sign of hash */
	for (i = 0; i < uboothdr->hash_len; i++)
		if (rsahash[uboothdr->hash_len - 1 - i] != dataHash[i])
			return false;

	return true;
}


static bool SecureRKModeVerifyBootImage(rk_boot_img_hdr *boothdr)
{
	int i;
	uint32 size;
	uint8 *dataHash;
	uint32 rsaResult[8];
	uint32 hwDataHash[8];
	uint8 *rsahash = (uint8 *)rsaResult;
	BOOT_HEADER *pkeyHead = (BOOT_HEADER *)g_rsa_key_buf;

#if 0
{
	int k = 0;

	printf("Boot Image Head: magic = %s, sign tag = 0x%x, sha flag = %d\n",
			boothdr->magic, boothdr->signTag, boothdr->sha_flag);

	printf("Boot Image Head: hash data:\n");
	if (boothdr->sha_flag == 256) {
		char *buf = (char *)(unsigned long)boothdr->sha;

		for (k = 0; k < 32; k++)
			printf("%02x", buf[k]);
		printf("\n");
	} else {
		char *buf = (char *)(unsigned long)boothdr->id;

		for (k = 0; k < sizeof(boothdr->id); k++)
			printf("%02x", buf[k]);
		printf("\n");
	}
	printf("kernel addr = 0x%x, size = 0x%x\n", boothdr->kernel_addr, boothdr->kernel_size);
	printf("ramdisk addr = 0x%x, size = 0x%x\n", boothdr->ramdisk_addr, boothdr->ramdisk_size);
	printf("second addr = 0x%x, size = 0x%x\n", boothdr->second_addr, boothdr->second_size);
}
#endif

	/* verify boot/recovery image */
	if (memcmp(boothdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0)
		return false;

	if (boothdr->signTag != SECURE_BOOT_SIGN_TAG)
		return false;

#if 0
{
	if (boothdr->sha_flag == 256) {
		sha256_context ctx;

		sha256_starts(&ctx);
		/* Android image */
		sha256_update(&ctx, (void *)(unsigned long)boothdr->kernel_addr, boothdr->kernel_size);
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->kernel_size, sizeof(boothdr->kernel_size));
		sha256_update(&ctx, (void *)(unsigned long)boothdr->ramdisk_addr, boothdr->ramdisk_size);
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
		sha256_update(&ctx, (void *)(unsigned long)boothdr->second_addr, boothdr->second_size);
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->second_size, sizeof(boothdr->second_size));

		/* rockchip's image add information. */
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->tags_addr, sizeof(boothdr->tags_addr));
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->page_size, sizeof(boothdr->page_size));
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->unused, sizeof(boothdr->unused));
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->name, sizeof(boothdr->name));
		sha256_update(&ctx, (void *)(unsigned long)&boothdr->cmdline, sizeof(boothdr->cmdline));
		sha256_finish(&ctx, (void *)(unsigned long)hwDataHash);

		dataHash = (uint8 *)hwDataHash;
	} else {
		SHA_CTX ctx;

		SHA_init(&ctx);

		/* Android image */
		SHA_update(&ctx, (void *)boothdr->kernel_addr, boothdr->kernel_size);
		SHA_update(&ctx, &boothdr->kernel_size, sizeof(boothdr->kernel_size));
		SHA_update(&ctx, (void *)boothdr->ramdisk_addr, boothdr->ramdisk_size);
		SHA_update(&ctx, &boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
		SHA_update(&ctx, (void *)boothdr->second_addr, boothdr->second_size);
		SHA_update(&ctx, &boothdr->second_size, sizeof(boothdr->second_size));

		/* rockchip's image add information. */
		SHA_update(&ctx, &boothdr->tags_addr, sizeof(boothdr->tags_addr));
		SHA_update(&ctx, &boothdr->page_size, sizeof(boothdr->page_size));
		SHA_update(&ctx, &boothdr->unused, sizeof(boothdr->unused));
		SHA_update(&ctx, &boothdr->name, sizeof(boothdr->name));
		SHA_update(&ctx, &boothdr->cmdline, sizeof(boothdr->cmdline));

		dataHash = SHA_final(&ctx);
	}

	int k = 0;
	uint8 *buf = dataHash;

	printf("Soft Calc Image hash data:\n");
	for (k = 0; k < sizeof(boothdr->id); k++)
		printf("%02x", buf[k]);
	printf("\n");
}
#endif

	size = boothdr->kernel_size + sizeof(boothdr->kernel_size) \
		+ boothdr->ramdisk_size + sizeof(boothdr->ramdisk_size) \
		+ boothdr->second_size + sizeof(boothdr->second_size) \
		+ sizeof(boothdr->tags_addr) + sizeof(boothdr->page_size) \
		+ sizeof(boothdr->unused) + sizeof(boothdr->name) + sizeof(boothdr->cmdline);

	if (boothdr->sha_flag == 256) {
		CryptoRSAInit((uint32 *)(boothdr->rsaHash2), pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
		CryptoSHAInit(size, 256);
		CryptoSHAInputByteSwap(1);
	} else {
		CryptoRSAInit((uint32 *)(boothdr->rsaHash), pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
		CryptoSHAInit(size, 160);
	}

	/* Android image. */
	CryptoSHAStart((uint32 *)(unsigned long)boothdr->kernel_addr, boothdr->kernel_size);
	CryptoSHAStart((uint32 *)&boothdr->kernel_size, sizeof(boothdr->kernel_size));
	CryptoSHAStart((uint32 *)(unsigned long)boothdr->ramdisk_addr, boothdr->ramdisk_size);
	CryptoSHAStart((uint32 *)&boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	CryptoSHAStart((uint32 *)(unsigned long)boothdr->second_addr, boothdr->second_size);
	CryptoSHAStart((uint32 *)&boothdr->second_size, sizeof(boothdr->second_size));

	/* only rockchip's image add. */
	CryptoSHAStart((uint32 *)&boothdr->tags_addr, sizeof(boothdr->tags_addr));
	CryptoSHAStart((uint32 *)&boothdr->page_size, sizeof(boothdr->page_size));
	CryptoSHAStart((uint32 *)&boothdr->unused, sizeof(boothdr->unused));
	CryptoSHAStart((uint32 *)&boothdr->name, sizeof(boothdr->name));
	CryptoSHAStart((uint32 *)&boothdr->cmdline, sizeof(boothdr->cmdline));

	CryptoSHAEnd(hwDataHash);
	CryptoRSAEnd(rsaResult);

	dataHash = (uint8 *)hwDataHash;

#if 0
{
	int k = 0;
	uint8 *buf = dataHash;

	printf("Crypto Calc Image hash data:\n");
	if (boothdr->sha_flag == 256) {
		for (k = 0; k < 32; k++)
			printf("%02x", buf[k]);
		printf("\n");
	} else {
		for (k = 0; k < sizeof(boothdr->id); k++)
			printf("%02x", buf[k]);
		printf("\n");
	}
}
#endif

	/* check the hash of image */
	if (boothdr->sha_flag == 256) {
		if (memcmp(boothdr->sha, hwDataHash, 32) != 0)
			return false;

		/* check the sign of hash */
		for (i = 0; i < 32; i++)
			if (rsahash[31 - i] != dataHash[i])
				return false;
	} else {
		if (memcmp(boothdr->id, hwDataHash, sizeof(boothdr->id)) != 0)
			return false;

		/* check the sign of hash */
		for (i = 0; i < 20; i++)
			if (rsahash[19 - i] != dataHash[i])
				return false;
	}

	return true;
}


static bool SecureRKModeBootImageCheck(rk_boot_img_hdr *boothdr, int unlocked)
{
	SecureBootCheckOK = 0;

	if (SecureRKModeVerifyBootImage(boothdr)) {
		SecureBootCheckOK = 1;
		return true;
	} else {
		SecureBootCheckOK = 0;
		PRINT_E("SecureRKModeSignCheck failed\n");
		return false;
	}
}


static void SecureRKModeLockLoader(void)
{
	if (gDrmKeyInfo.secureBootLock == 0) {
		gDrmKeyInfo.secureBootLock = 1;
		gDrmKeyInfo.secureBootLockKey = 0;
		StorageSysDataStore(1, &gDrmKeyInfo);
	}
}


static bool SecureRKModeKeyCheck(uint8 *pKey)
{
	int i;
	uint8 swap;
	uint32 rsaResult[8];
	uint8 *rsahash = (uint8 *)rsaResult;
	BOOT_HEADER *pkeyHead = (BOOT_HEADER *)g_rsa_key_buf;

#if 0
{
	int j = 0, k = 0;
	char *buf = (char *)pKey;

	printf("dump usb key:\n");
	for (j = 0; j < 512 / 16; j++) {
		for (k = 0; k < 16; k++)
			printf("%02x", buf[j * 16 + k]);
		printf("\n");
	}
}
#endif

#if 0
	printf("source: \n");
	for (i = 0; i < 32; i++)
		printf("%02x", pKey[256+i]);
	printf("\n");
#endif

	/* swap */
	for(i = 0; i < 256 / 2; i++) {
		swap = pKey[i];
		pKey[i] = pKey[256 - i - 1];
		pKey[256 - i - 1] = swap;
	}

	CryptoRSAInit((uint32 *)pKey, pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
	CryptoRSAEnd(rsaResult);

#if 0
	printf("calc result: \n");
	for (i = 0; i < 32; i++)
		printf("%02x", rsahash[31-i]);
	printf("\n");
#endif

	/* swap */
	for (i = 0; i < 32 / 2; i++) {
		swap = rsahash[i];
		rsahash[i] = rsahash[31 - i];
		rsahash[31 - i] = swap;
	}

	/* check the hash of image */
	if (memcmp(pKey + 256, rsaResult, sizeof(rsaResult)) == 0) {
		memset(pKey, 0, 256);
		memcpy(pKey, rsaResult, sizeof(rsaResult));

		return true;
	}

	return false;
}


static uint32 SecureRKModeRSAKeyInit(uint32 *secure)
{
	uint32 SecureEn = 0;

#if defined(CONFIG_SECURE_RSA_KEY_IN_RAM)
	/* Get RSAKey in sdram which miniloader offer */
	if (SecureRKModeGetRSAKey() == 0)
		SecureEn = 1;

	*secure = SecureEn;
#else
	BOOT_HEADER *pHead = (BOOT_HEADER *)g_rsa_key_buf;
	uint32 i = 0;
	int32 ret;

#if defined(CONFIG_RKEFUSE_V1)
	/* efuse v1 read char unit */
	uint8 flag = 0;
	SecureEfuseRead((void *)(unsigned long)SECURE_EFUSE_BASE_ADDR, &flag, 0X1F, 1);
	if (0xFF == flag)
		SecureEn = 1;
#elif defined(CONFIG_RKEFUSE_V2)
	/* efuse v2 read word unit */
	uint32 flag = 0;
	SecureEfuseRead((void *)(unsigned long)SECURE_EFUSE_BASE_ADDR, &flag, 0X00, 4);
	if (flag & 0x01)
		SecureEn = 1;
#endif
	*secure = SecureEn;
	if (SecureEn != 0) {
		StorageReadFlashInfo((uint8 *)&g_FlashInfo);
		if (StorageGetBootMedia() == BOOT_FROM_FLASH)
			i = 2;
		else
			i = 0;
		for (; i < 16; i++) {
			PRINT_I("SecureRKModeRSAKeyInit %x\n", i * g_FlashInfo.BlockSize + 4);
			ret = StorageReadPba(i * g_FlashInfo.BlockSize + 4, g_rsa_key_buf, 4);
			if (ret == FTL_OK)
				if (SecureRKModeGetRSAKey() == 0)
					if ((pHead->tag == 0x4B415352) && (SecureRKModeChkPubkey(pHead->RSA_N) == 0))
						break;
		}
		/* check key error */
		if (i >= 16) {
			PRINT_E("SecureRKMode RSAKey Init error!\n");
			return ERROR;
		}
	}
#endif /* CONFIG_SECURE_RSA_KEY_IN_RAM */

	return OK;
}


static uint32 SecureRKModeInit(void)
{
	uint32 secure = 0;

	/* crypto init */
	CryptoInit();

	/* check secure flag */
	if (SecureRKModeRSAKeyInit(&secure) == ERROR)
		return ERROR;

	if (secure != 0) {
		SecureMode = SBOOT_MODE_RK;
		PRINT_E("Secure Boot Mode: 0x%x\n", SecureMode);

		/* set SecureBoot enable and lock flag */
		SecureBootEn = 1;
		SecureBootLock = 1;

		/* config drm information */
		if (StorageSysDataLoad(1, &gDrmKeyInfo) == FTL_OK) {
			if ((gDrmKeyInfo.drmtag != 0x4B4D5244) || (gDrmKeyInfo.publicKeyLen == 0)) {
				gDrmKeyInfo.drmtag = 0x4B4D5244;
				gDrmKeyInfo.drmLen = 504;
				gDrmKeyInfo.publicKeyLen = 0x200;
				gDrmKeyInfo.keyBoxEnable = 1;
				gDrmKeyInfo.drmKeyLen = 0;
				gDrmKeyInfo.secureBootLock = 1;
				gDrmKeyInfo.secureBootLockKey = 0;

				StorageSysDataStore(1, &gDrmKeyInfo);
			}
		}
	}

	return OK;
}
#endif /* SECUREBOOT_CRYPTO_EN */


bool SecureModeVerifyLoader(RK28BOOT_HEAD *hdr)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK)
		return SecureRKModeVerifyLoader(hdr);
#endif

	return SecureNSModeVerifyLoader(hdr);
}


bool SecureModeVerifyUbootImage(second_loader_hdr *pHead)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK)
		return SecureRKModeVerifyUbootImage(pHead);
#endif

	return SecureNSModeVerifyUbootImageSign(pHead);
}


bool SecureModeVerifyBootImage(rk_boot_img_hdr *boothdr)
{
	/* hdr read from storage, adjust hdr kernel/ramdisk/second address. */
	boothdr->kernel_addr = (uint32_t)((unsigned long)(void *)boothdr + boothdr->page_size);
	boothdr->ramdisk_addr = boothdr->kernel_addr + ALIGN(boothdr->kernel_size, boothdr->page_size);
	if (boothdr->second_size)
		boothdr->second_addr = boothdr->ramdisk_addr + ALIGN(boothdr->ramdisk_size, boothdr->page_size);

#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK)
		return SecureRKModeVerifyBootImage(boothdr);
#endif

	return SecureNSModeVerifyBootImageSign(boothdr);
}


bool SecureModeBootImageCheck(rk_boot_img_hdr *hdr, int unlocked)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK)
		return SecureRKModeBootImageCheck(hdr, unlocked);
#endif

	return SecureNSModeBootImageCheck(hdr, unlocked);
}


bool SecureModeRSAKeyCheck(uint8 *pKey)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK)
		return SecureRKModeKeyCheck(pKey);
#endif

	return SecureNSModeKeyCheck(pKey);
}


void SecureModeLockLoader(void)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK)
		return SecureRKModeLockLoader();
#endif

	return SecureNSModeLockLoader();
}


uint32 SecureModeInit(void)
{
	SecureMode = SBOOT_MODE_NS;

#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureRKModeInit() == ERROR) {
		PRINT_E("SecureRKModeInit error!\n");
		ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
		return FTL_ERROR;
	}
#endif

	if (SecureMode == SBOOT_MODE_NS)
		SecureNSModeInit();

	return FTL_OK;
}
