/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "../config.h"

DECLARE_GLOBAL_DATA_PTR;

extern  uint32   RSA_KEY_TAG;
extern  uint32   RSA_KEY_LENGTH;
extern  uint32   RSA_KEY_DATA;
static	uint16   *RSK_KEY;

extern void P_RC4(unsigned char * buf, unsigned short len);


static bool SecureNSModeVerifyLoader(RK28BOOT_HEAD *hdr)
{
#define RSA_KEY_OFFSET 0x10//according to dumped data, the key is here.
#define RSA_KEY_LEN    0x102//258, public key's length
	char buf[RK_BLK_SIZE];

	memcpy(buf, (void *)hdr + hdr->uiFlashBootOffset, RK_BLK_SIZE);
	P_RC4((unsigned char *)buf, RK_BLK_SIZE);

	if (buf[RSA_KEY_OFFSET] != 0 || buf[RSA_KEY_OFFSET + 1] != 4) {
		PRINT_I("Unsigned loader!\n");
	}
	if (gDrmKeyInfo.publicKeyLen == 0) {
		PRINT_I("NS Mode allow flash unsigned loader.\n");
		return true;
	}

#if 0
	printf("dump new loader's key:\n");
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 16; j++) {
			printf("%02x", buf[RSA_KEY_OFFSET + i * 16 + j]);
		}
		printf("\n");
	}
#endif

	return !memcmp(buf + RSA_KEY_OFFSET, gDrmKeyInfo.publicKey, RSA_KEY_LEN);
}


static bool SecureNSModeSignCheck(uint8 * rsaHash, uint8 *Hash, uint8 length)
{
	uint8  decodedHash[40];
    
	if(0 == rsaDecodeHash(decodedHash, rsaHash, (uint8*)RSK_KEY, length)) {
		if(0 == memcmp(Hash, decodedHash, 20)) {
			PRINT_I("Sign OK\n");
			return true;
		}
	}

	return false;
}


static bool SecureNSModeUbootImageShaCheck(second_loader_hdr *hdr)
{
	uint8_t *sha;
	SHA_CTX ctx;
	int size = SHA_DIGEST_SIZE > hdr->hash_len ? hdr->hash_len : SHA_DIGEST_SIZE;

	SHA_init(&ctx);
	SHA_update(&ctx, (void *)hdr + sizeof(second_loader_hdr), hdr->loader_load_size);
	SHA_update(&ctx, &hdr->loader_load_addr, sizeof(hdr->loader_load_addr));
	SHA_update(&ctx, &hdr->loader_load_size, sizeof(hdr->loader_load_size));
	SHA_update(&ctx, &hdr->hash_len, sizeof(hdr->hash_len));
	sha = SHA_final(&ctx);

#if 0
	int i = 0;
	printf("\nreal sha:\n");
	for (i = 0;i < size;i++) {
		printf("%02x", (char)sha[i]);
	}
	printf("\nsha from image header:\n");
	for (i = 0;i < size;i++) {
		printf("%02x", ((char*)hdr->hash)[i]);
	}
	printf("\n");
#endif

	return !memcmp(hdr->hash, sha, size);
}


static bool SecureNSModeVerifyUbootImageSign(second_loader_hdr* hdr)
{
	if (gDrmKeyInfo.publicKeyLen == 0) {
		PRINT_I("NS Mode allow flash unsigned loader.\n");
		return true;
	}

	/* verify uboot iamge. */
	if (memcmp(hdr->magic, RK_UBOOT_MAGIC, sizeof(RK_UBOOT_MAGIC)) != 0) {
		PRINT_E("unrecognized image format!\n");
		return false;
	}

	if (hdr->signTag != RK_UBOOT_SIGN_TAG) {
		PRINT_E("unsigned image!\n");
		return false;
	}

	if (!SecureBootEn) {
		PRINT_E("loader sign mismatch, not allowed to flash!\n");
		return false;
	}

	/* signed image, check with signature. */
	/* check sha here. */
	if (!SecureNSModeUbootImageShaCheck(hdr)) {
		PRINT_E("sha mismatch!\n");
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


static bool SecureNSModeBootImageShaCheck(rk_boot_img_hdr *boothdr)
{
	uint8_t *sha;
	SHA_CTX ctx;
	int size = SHA_DIGEST_SIZE > sizeof(boothdr->id) ? sizeof(boothdr->id) : SHA_DIGEST_SIZE;

	SHA_init(&ctx);

	/* Android image */
	SHA_update(&ctx, boothdr->kernel_addr, boothdr->kernel_size);
	SHA_update(&ctx, &boothdr->kernel_size, sizeof(boothdr->kernel_size));
	SHA_update(&ctx, boothdr->ramdisk_addr, boothdr->ramdisk_size);
	SHA_update(&ctx, &boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	SHA_update(&ctx, boothdr->second_addr, boothdr->second_size);
	SHA_update(&ctx, &boothdr->second_size, sizeof(boothdr->second_size));

	/* rockchip's image add information. */
	SHA_update(&ctx, &boothdr->tags_addr, sizeof(boothdr->tags_addr));
	SHA_update(&ctx, &boothdr->page_size, sizeof(boothdr->page_size));
	SHA_update(&ctx, &boothdr->unused, sizeof(boothdr->unused));
	SHA_update(&ctx, &boothdr->name, sizeof(boothdr->name));
	SHA_update(&ctx, &boothdr->cmdline, sizeof(boothdr->cmdline));

	sha = SHA_final(&ctx);

#if 0
	int i = 0;
	printf("\nreal sha:\n");
	for (i = 0;i < size;i++) {
		printf("%02x", (char)sha[i]);
	}
	printf("\nsha from image header:\n");
	for (i = 0;i < size;i++) {
		printf("%02x", ((char*)boothdr->id)[i]);
	}
	printf("\n");
#endif

	return !memcmp(boothdr->id, sha, size);
}


static bool SecureNSModeVerifyBootImageSign(rk_boot_img_hdr* boothdr)
{
	if (gDrmKeyInfo.publicKeyLen == 0) {
		PRINT_I("NS Mode allow flash unsigned loader.\n");
		return true;
	}

	/* verify boot/recovery image */
	if (memcmp(boothdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		return false;
	}

	if (boothdr->signTag != SECURE_BOOT_SIGN_TAG) {
		return false;
	}

	if (!SecureBootEn) {
		PRINT_E("loader sign mismatch, not allowed to flash!\n");
		return false;
	}

	/* signed image, check with signature. */
	/* check sha here. */
	if (!SecureNSModeBootImageShaCheck(boothdr)) {
		PRINT_E("sha mismatch!\n");
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


static bool SecureNSModeBootImageSecureCheck(rk_boot_img_hdr *hdr, int unlocked)
{
	rk_boot_img_hdr *boothdr = (rk_boot_img_hdr *)hdr;

	SecureBootCheckOK = 0;

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		return false;
	}

	if (!unlocked && SecureBootEn && (boothdr->signTag == SECURE_BOOT_SIGN_TAG))
	{
		if (SecureNSModeSignCheck(boothdr->rsaHash, (uint8 *)boothdr->id, boothdr->signlen))
		{
			SecureBootCheckOK = 1;
			return true;
		}
		else
		{
			SecureBootCheckOK = 0;
			PRINT_E("SecureNSModeSignCheck failed\n");
			return false;
		}
	}

	return false;
}


static void SecureNSModeLockLoader(void)
{
	if (RSK_KEY[0] == 0X400) {
		if(gDrmKeyInfo.secureBootLock == 0) {
			gDrmKeyInfo.secureBootLock = 1;
			gDrmKeyInfo.secureBootLockKey = 0;
			StorageSysDataStore(1, &gDrmKeyInfo);
		}
	}
}


static bool SecureNSModeKeyCheck(uint8 *pKey)
{
	if (rsaCheckMD5(pKey, pKey+256, (uint8 *)gDrmKeyInfo.publicKey, 128) == 0) {
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
	if((RSK_KEY[0] != 0X400))
	{
		SecureBootEn = 0;
		RkPrintf("unsigned!\n");
	}

	SecureBootLock = 0;

	if(StorageSysDataLoad(1, &gDrmKeyInfo) == FTL_OK)
	{
		updataFlag = 0;
		SecureBootLock = gDrmKeyInfo.secureBootLock;
		if(SecureBootLock != 1) {
			SecureBootLock = 0;
		}

		if(gDrmKeyInfo.drmtag != 0x4B4D5244)
		{
			gDrmKeyInfo.drmtag = 0x4B4D5244;
			gDrmKeyInfo.drmLen = 504;
			gDrmKeyInfo.keyBoxEnable = 0;
			gDrmKeyInfo.drmKeyLen = 0;
			gDrmKeyInfo.publicKeyLen = 0;
			gDrmKeyInfo.secureBootLock = 0;
			gDrmKeyInfo.secureBootLockKey = 0;
			updataFlag = 1;
		}

		if(RSK_KEY[0] == 0X400)
		{
#ifdef SECURE_BOOT_SET_LOCK_ALWAY
			if(gDrmKeyInfo.secureBootLock == 0)
			{
				gDrmKeyInfo.secureBootLock = 1;
				gDrmKeyInfo.secureBootLockKey = 0;
				updataFlag = 1;
			}
#endif
			if(gDrmKeyInfo.publicKeyLen == 0)
			{//没有公钥，是第一次才开启keyBoxEnable,
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
			}
			else if(memcmp(gDrmKeyInfo.publicKey, RSK_KEY, 0x100) != 0)
			{   //如果已经存在公钥，并且公钥被替换了，那么关闭
				if(memcmp(gDrmKeyInfo.publicKey + 4, RSK_KEY, 0x100) == 0)
				{
					ftl_memcpy(gDrmKeyInfo.publicKey, RSK_KEY, 0x104);
					updataFlag = 1;
				}
				else
				{
					gDrmKeyInfo.keyBoxEnable = 0; //暂时不启用这个功能
					SecureBootEn = 0;
					RkPrintf("E:pKey!\n");
				}
			}
		}

		if(updataFlag)
		{
			updataFlag = 0;
			if(FTL_ERROR == StorageSysDataStore(1, &gDrmKeyInfo))
			{
				;// TODO:SysDataStore异常处理
			}
		}
	}

	if(StorageSysDataLoad(0, &gBootConfig) == FTL_OK)
	{
		updataFlag = 0;
		if(gBootConfig.bootTag != 0x44535953)
		{
			gBootConfig.bootTag = 0x44535953;
			gBootConfig.bootLen = 504;
			gBootConfig.bootMedia = 0;// TODO: boot 选择
			gBootConfig.BootPart = 0;
			gBootConfig.secureBootEn = 0;//SecureBootEn; 默认disable
			updataFlag = 1;
		}
		else
		{
#ifndef SECURE_BOOT_ENABLE_ALWAY
			if(gBootConfig.secureBootEn == 0)
				SecureBootEn = 0;
#endif
		}

		if(updataFlag)
		{
			updataFlag = 0;
			if(FTL_ERROR == StorageSysDataStore(0, &gBootConfig))
			{
				;// TODO:SysDataStore异常处理
			}
		}
	}
	else
	{
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

static int32 SecureRKModeChkPubkey(uint32 *pKey)
{
	uint32 hash[8];         //max 256bit
	uint32 HashOTP[OTP_HASH_LEN/4];

	/* Get the public key HASH from the Efuse */
	EfuseRead(HashOTP, OTP_HASH_ADDR, OTP_HASH_LEN);

	SecureSHAInit(PUBLIC_KEY_LEN, 256);
	SecureSHAUpdate(pKey, PUBLIC_KEY_LEN);
	SecureSHAFinish(hash);

	// compare the result with Efuse data
	return memcmp((uint8 *)&hash, (uint8 *)&HashOTP, OTP_HASH_LEN);     //audi, audis only check 64bit
}


static int32 SecureRKModeGetRSAKey(void)
{
	int i = 0;

	if (g_rsa_key_buf[0] == 0xb86d753c) {
		for (i = 0; i < 4; i++) {
			P_RC4((uint8 *)(g_rsa_key_buf+132*i), 512);
			if (i > 0) {
				memcpy(g_rsa_key_buf+128*i, g_rsa_key_buf+132*i, 512);
			}
		}
	} else if (g_rsa_key_buf[0] == 0x4B415352) {
		for (i = 0; i < 4; i++) {
			if (i > 0) {
				memcpy(g_rsa_key_buf+128*i, g_rsa_key_buf+132*i, 512);
			}
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
		for (k = 0; k < 16; k++) {
			printf("%02x", buf[j * 16 + k]);
		}
		printf("\n");
	}
}
#endif

	return 0;
}


static bool SecureRKModeVerifyLoader(RK28BOOT_HEAD *hdr)
{
	char keybuf[RK_BLK_SIZE * 4];
	BOOT_HEADER *pKeyHead = (BOOT_HEADER *)keybuf;
	int i;

	debug("Loader Head: hdr = 0x%x, flash data offset = 0x%x\n", hdr, hdr->uiFlashDataOffset);

	memcpy(keybuf, (void *)hdr + hdr->uiFlashDataOffset, RK_BLK_SIZE * 4);
	for (i = 0; i < 4; i++) {
		P_RC4((unsigned char *)keybuf + RK_BLK_SIZE * i, RK_BLK_SIZE);
	}
#if 0
{
	int j = 0, k = 0;
	char *buf = (char *)keybuf;

	printf("dump new loader's key:\n");
	for (j = 0; j < (2048 / 16); j++) {
		for (k = 0; k < 16; k++) {
			printf("%02x", buf[j * 16 + k]);
		}
		printf("\n");
	}
}
#endif

	/* check new loader key */
	if ((pKeyHead->tag == 0x4B415352) && (SecureRKModeChkPubkey(pKeyHead->RSA_N) == 0)) {
		return true;
	}

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
	for (k = 0; k < uboothdr->hash_len; k++) {
		printf("%02x", buf[k]);
	}
	printf("\n");

	printf("Loader addr = 0x%x, size = 0x%x\n", uboothdr->loader_load_addr, uboothdr->loader_load_size);
}
#endif

	/* verify uboot iamge. */
	if (memcmp(uboothdr->magic, RK_UBOOT_MAGIC, sizeof(RK_UBOOT_MAGIC)) != 0) {
		return false;
	}

	if ((uboothdr->signTag != RK_UBOOT_SIGN_TAG) \
			&& (uboothdr->hash_len != 20) && (uboothdr->hash_len != 32)) {
		return false;
	}

#if 0
{
	SHA_CTX ctx;

	SHA_init(&ctx);
	SHA_update(&ctx, (void *)uboothdr + sizeof(second_loader_hdr), uboothdr->loader_load_size);
	SHA_update(&ctx, &uboothdr->loader_load_addr, sizeof(uboothdr->loader_load_addr));
	SHA_update(&ctx, &uboothdr->loader_load_size, sizeof(uboothdr->loader_load_size));
	SHA_update(&ctx, &uboothdr->hash_len, sizeof(uboothdr->hash_len));
	dataHash = SHA_final(&ctx);

	int k = 0;
	char *buf = dataHash;

	printf("Soft Calc Image hash data:\n");
	for (k = 0; k < uboothdr->hash_len; k++) {
		printf("%02x", buf[k]);
	}
	printf("\n");
}
#endif

	size = uboothdr->loader_load_size + sizeof(uboothdr->loader_load_size) \
		 + sizeof(uboothdr->loader_load_addr) + sizeof(uboothdr->hash_len);

	CryptoRSAInit((uint32 *)(uboothdr->rsaHash), pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
	CryptoSHAInit(size, 160);
	/* rockchip's second level image. */
	CryptoSHAStart((uint32 *)((void *)uboothdr + sizeof(second_loader_hdr)), uboothdr->loader_load_size);
	CryptoSHAStart((uint32 *)&uboothdr->loader_load_addr, sizeof(uboothdr->loader_load_addr));
	CryptoSHAStart((uint32 *)&uboothdr->loader_load_size, sizeof(uboothdr->loader_load_size));
	CryptoSHAStart((uint32 *)&uboothdr->hash_len, sizeof(uboothdr->hash_len));

	CryptoSHAEnd(hwDataHash);
	CryptoRSAEnd(rsaResult);

	dataHash = (char *)hwDataHash;

#if 0
{
	int k = 0;
	char *buf = dataHash;

	printf("Crypto Calc Image hash data:\n");
	for (k = 0; k < uboothdr->hash_len; k++) {
		printf("%02x", buf[k]);
	}
	printf("\n");
}
#endif

	/* check the hash of image */
	if (memcmp(uboothdr->hash, hwDataHash, uboothdr->hash_len) != 0) {
		return false;
	}

	/* check the sign of hash */
	for (i = 0; i < 20; i++) {
		if (rsahash[19-i] != dataHash[i]) {
			return false;
		}
	}

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
	char *buf = boothdr->id;

	printf("Boot Image Head: magic = %s, sign tag = 0x%x\n", boothdr->magic, boothdr->signTag);

	printf("Boot Image Head: hash data:\n");
	for (k = 0; k < sizeof(boothdr->id); k++) {
		printf("%02x", buf[k]);
	}
	printf("\n");

	printf("kernel addr = 0x%x, size = 0x%x\n", boothdr->kernel_addr, boothdr->kernel_size);
	printf("ramdisk addr = 0x%x, size = 0x%x\n", boothdr->ramdisk_addr, boothdr->ramdisk_size);
	printf("second addr = 0x%x, size = 0x%x\n", boothdr->second_addr, boothdr->second_size);
}
#endif

	/* verify boot/recovery image */
	if (memcmp(boothdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		return false;
	}

	if (boothdr->signTag != SECURE_BOOT_SIGN_TAG) {
		return false;
	}
#if 0
{
	SHA_CTX ctx;

	SHA_init(&ctx);

	/* Android image */
	SHA_update(&ctx, boothdr->kernel_addr, boothdr->kernel_size);
	SHA_update(&ctx, &boothdr->kernel_size, sizeof(boothdr->kernel_size));
	SHA_update(&ctx, boothdr->ramdisk_addr, boothdr->ramdisk_size);
	SHA_update(&ctx, &boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	SHA_update(&ctx, boothdr->second_addr, boothdr->second_size);
	SHA_update(&ctx, &boothdr->second_size, sizeof(boothdr->second_size));

	/* rockchip's image add information. */
	SHA_update(&ctx, &boothdr->tags_addr, sizeof(boothdr->tags_addr));
	SHA_update(&ctx, &boothdr->page_size, sizeof(boothdr->page_size));
	SHA_update(&ctx, &boothdr->unused, sizeof(boothdr->unused));
	SHA_update(&ctx, &boothdr->name, sizeof(boothdr->name));
	SHA_update(&ctx, &boothdr->cmdline, sizeof(boothdr->cmdline));

	dataHash = SHA_final(&ctx);

	int k = 0;
	char *buf = dataHash;

	printf("Soft Calc Image hash data:\n");
	for (k = 0; k < sizeof(boothdr->id); k++) {
		printf("%02x", buf[k]);
	}
	printf("\n");
}
#endif

	size = boothdr->kernel_size + sizeof(boothdr->kernel_size) \
		+ boothdr->ramdisk_size + sizeof(boothdr->ramdisk_size) \
		+ boothdr->second_size + sizeof(boothdr->second_size) \
		+ sizeof(boothdr->tags_addr) + sizeof(boothdr->page_size) \
		+ sizeof(boothdr->unused) + sizeof(boothdr->name) + sizeof(boothdr->cmdline);

	CryptoRSAInit((uint32*)(boothdr->rsaHash), pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
	CryptoSHAInit(size, 160);

	/* Android image. */
	CryptoSHAStart((uint32 *)boothdr->kernel_addr, boothdr->kernel_size);
	CryptoSHAStart((uint32 *)&boothdr->kernel_size, sizeof(boothdr->kernel_size));
	CryptoSHAStart((uint32 *)boothdr->ramdisk_addr, boothdr->ramdisk_size);
	CryptoSHAStart((uint32 *)&boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	CryptoSHAStart((uint32 *)boothdr->second_addr, boothdr->second_size);
	CryptoSHAStart((uint32 *)&boothdr->second_size, sizeof(boothdr->second_size));

	/* only rockchip's image add. */
	CryptoSHAStart((uint32 *)&boothdr->tags_addr, sizeof(boothdr->tags_addr));
	CryptoSHAStart((uint32 *)&boothdr->page_size, sizeof(boothdr->page_size));
	CryptoSHAStart((uint32 *)&boothdr->unused, sizeof(boothdr->unused));
	CryptoSHAStart((uint32 *)&boothdr->name, sizeof(boothdr->name));
	CryptoSHAStart((uint32 *)&boothdr->cmdline, sizeof(boothdr->cmdline));

	CryptoSHAEnd(hwDataHash);
	CryptoRSAEnd(rsaResult);

	dataHash = (char*)hwDataHash;

#if 0
{
	int k = 0;
	char *buf = dataHash;

	printf("Crypto Calc Image hash data:\n");
	for (k = 0; k < sizeof(boothdr->id); k++) {
		printf("%02x", buf[k]);
	}
	printf("\n");
}
#endif

	/* check the hash of image */
	if (memcmp(boothdr->id, hwDataHash, sizeof(boothdr->id)) != 0) {
		return false;
	}

	/* check the sign of hash */
	for (i = 0; i < 20; i++) {
		if (rsahash[19-i] != dataHash[i]) {
			return false;
		}
	}

	return true;
}


static bool SecureRKModeBootImageSecureCheck(rk_boot_img_hdr *boothdr, int unlocked)
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
		for (k = 0; k < 16; k++) {
			printf("%02x", buf[j * 16 + k]);
		}
		printf("\n");
	}
}
#endif

#if 0
	printf("source: \n");
	for (i = 0; i < 32; i++) {
		printf("%02x", pKey[256+i]);
	}
	printf("\n");
#endif

	/* swap */
	for(i = 0; i< 256 / 2; i++) {
		swap = pKey[i];
		pKey[i] = pKey[256 - i - 1];
		pKey[256 - i - 1] = swap;
	}

	CryptoRSAInit((uint32 *)pKey, pkeyHead->RSA_N, pkeyHead->RSA_E, pkeyHead->RSA_C);
	CryptoRSAEnd(rsaResult);

#if 0
	printf("calc result: \n");
	for (i = 0; i < 32; i++) {
		printf("%02x", rsahash[31-i]);
	}
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

static uint32 SecureRKModeInit(void)
{
	BOOT_HEADER *pHead = (BOOT_HEADER *)g_rsa_key_buf;
	uint32 secure = 0;
	uint32 i = 0;
	int32 ret;

	/* check efuse secure flag */
	secure = 0;
#if defined(CONFIG_RKCHIP_RK3128)
	/* rk3128 efuse read char unit */
	uint8 flag = 0;
	EfuseRead(&flag, 0X1F, 1);
	if (0xFF == flag) {
		secure = 1;
	}
#elif defined(CONFIG_RKCHIP_RK3288)
	/* rk3288 efuse read word unit */
	uint32 flag = 0;
	EfuseRead(&flag, 0X00, 4);
	if (flag & 0x01) {
		secure = 1;
	}
#endif

	if (secure != 0) {
		SecureMode = SBOOT_MODE_RK;
		printf("Secure Boot Mode: 0x%x\n", SecureMode);

		CryptoInit();
		StorageReadFlashInfo((uint8 *)&g_FlashInfo);

		i = 0;
		if (StorageGetBootMedia() == BOOT_FROM_FLASH) {
			i = 2;
		}
		for (; i<16; i++) {
			PRINT_E("SecureInit %x\n", i * g_FlashInfo.BlockSize + 4);
			ret = StorageReadPba(i * g_FlashInfo.BlockSize + 4, g_rsa_key_buf, 4);
			if (ret == FTL_OK) {
				if (SecureRKModeGetRSAKey() == 0) {
					if ((pHead->tag == 0x4B415352) && (SecureRKModeChkPubkey(pHead->RSA_N) == 0)) {
						break;
					}
				}
			}
		}
		/* check key error */
		if (i >= 16) {
			return ERROR;
		}

		/* config drm information */
		SecureBootEn = 1;
		SecureBootLock = 1;

		if (StorageSysDataLoad(1, &gDrmKeyInfo) == FTL_OK) {
			if (gDrmKeyInfo.drmtag != 0x4B4D5244) {
				gDrmKeyInfo.drmtag = 0x4B4D5244;
				gDrmKeyInfo.drmLen = 504;
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
	if (SecureMode == SBOOT_MODE_RK) {
		return SecureRKModeVerifyLoader(hdr);
	}
#endif

	return SecureNSModeVerifyLoader(hdr);
}


bool SecureModeVerifyUbootImage(second_loader_hdr *pHead)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK) {
		return SecureRKModeVerifyUbootImage(pHead);
	}
#endif

	return SecureNSModeVerifyUbootImageSign(pHead);
}


bool SecureModeVerifyBootImage(rk_boot_img_hdr* boothdr)
{
	/* hdr read from storage, adjust hdr kernel/ramdisk/second address. */
	boothdr->kernel_addr = (void *)boothdr + boothdr->page_size;
	boothdr->ramdisk_addr = boothdr->kernel_addr + ALIGN(boothdr->kernel_size, boothdr->page_size);
	if (boothdr->second_size) {
		boothdr->second_addr = boothdr->ramdisk_addr + ALIGN(boothdr->ramdisk_size, boothdr->page_size);
	}

#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK) {
		return SecureRKModeVerifyBootImage(boothdr);
	}
#endif

	return SecureNSModeVerifyBootImageSign(boothdr);
}


bool SecureModeBootImageSecureCheck(rk_boot_img_hdr *hdr, int unlocked)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK) {
		return SecureRKModeBootImageSecureCheck(hdr, unlocked);
	}
#endif

	return SecureNSModeBootImageSecureCheck(hdr, unlocked);
}


bool SecureModeRSAKeyCheck(uint8 *pKey)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK) {
		return SecureRKModeKeyCheck(pKey);
	}
#endif

	return SecureNSModeKeyCheck(pKey);
}


void SecureModeLockLoader(void)
{
#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureMode == SBOOT_MODE_RK) {
		return SecureRKModeLockLoader();
	}
#endif

	return SecureNSModeLockLoader();
}


uint32 SecureModeInit(void)
{
	SecureMode = SBOOT_MODE_NS;

#ifdef SECUREBOOT_CRYPTO_EN
	if (SecureRKModeInit() == ERROR) {
		printf("SecureRKModeInit error!\n");
		ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
		return FTL_ERROR;
	}
#endif

	if (SecureMode == SBOOT_MODE_NS) {
		SecureNSModeInit();
	}

	return FTL_OK;
}
