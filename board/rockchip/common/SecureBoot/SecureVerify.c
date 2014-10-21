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
	if (!memcmp(hdr->magic, RK_UBOOT_MAGIC, sizeof(RK_UBOOT_MAGIC))) {
		if (hdr->signTag == RK_UBOOT_SIGN_TAG) {
			/* signed image, check with signature. */
			/* check sha here. */
			if (!SecureNSModeUbootImageShaCheck(hdr)) {
				PRINT_E("sha mismatch!\n");
				goto fail;
			}
			if (!SecureBootEn) {
				PRINT_E("loader sign mismatch, not allowed to flash!\n");
				goto fail;
			}
			/* check rsa sign here. */
			if (SecureNSModeSignCheck(hdr->rsaHash, hdr->hash, hdr->signlen)) {
				return true;
			} else {
				PRINT_E("signature mismatch!\n");
				goto fail;
			}
		} else {
			PRINT_E("unsigned image!\n");
			goto fail;
		}
	} else {
		PRINT_E("unrecognized image format!\n");
		goto fail;
	}

fail:
	return false;
}


static bool SecureNSModeBootImageShaCheck(rk_boot_img_hdr *boothdr)
{
	uint8_t *sha;
	SHA_CTX ctx;
	int size = SHA_DIGEST_SIZE > sizeof(boothdr->id) ? sizeof(boothdr->id) : SHA_DIGEST_SIZE;

	void *kernel_data = (void*)boothdr + boothdr->page_size;
	void *ramdisk_data = kernel_data + ALIGN(boothdr->kernel_size, boothdr->page_size);
	void *second_data = 0;
	if (boothdr->second_size) {
		second_data = kernel_data + ALIGN(boothdr->ramdisk_size, boothdr->page_size);
	}

	SHA_init(&ctx);

	/* Android image */
	SHA_update(&ctx, kernel_data, boothdr->kernel_size);
	SHA_update(&ctx, &boothdr->kernel_size, sizeof(boothdr->kernel_size));
	SHA_update(&ctx, ramdisk_data, boothdr->ramdisk_size);
	SHA_update(&ctx, &boothdr->ramdisk_size, sizeof(boothdr->ramdisk_size));
	SHA_update(&ctx, second_data, boothdr->second_size);
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
	if (!memcmp(boothdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		if (boothdr->signTag == SECURE_BOOT_SIGN_TAG) {
			/* signed image, check with signature. */
			/* check sha here. */
			if (!SecureNSModeBootImageShaCheck(boothdr)) {
				PRINT_E("sha mismatch!\n");
				goto fail;
			}
			if (!SecureBootEn) {
				PRINT_E("loader sign mismatch, not allowed to flash!\n");
				goto fail;
			}
			/* check rsa sign here. */
			if(SecureNSModeSignCheck((uint8 *)boothdr->rsaHash, (uint8 *)boothdr->id,
						boothdr->signlen)) {
				return true;
			} else {
				PRINT_E("signature mismatch!\n");
				goto fail;
			}
		} else {
			PRINT_E("unsigned image!\n");
			goto fail;
		}
	} else {
		PRINT_E("unrecognized image format!\n");
		goto fail;
	}

fail:
	return false;
}


static bool SecureNSModeBootImageSecureCheck(rk_boot_img_hdr *hdr, int unlocked)
{
	rk_boot_img_hdr *boothdr = (rk_boot_img_hdr *)hdr;

	SecureBootCheckOK = 0;

	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
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


bool SecureModeVerifyLoader(RK28BOOT_HEAD *hdr)
{
	return SecureNSModeVerifyLoader(hdr);
}


bool SecureModeVerifyUbootImage(second_loader_hdr *pHead)
{
	return SecureNSModeVerifyUbootImageSign(pHead);
}


bool SecureModeVerifyBootImage(rk_boot_img_hdr* boothdr)
{
	return SecureNSModeVerifyBootImageSign(boothdr);
}

bool SecureModeBootImageSecureCheck(rk_boot_img_hdr *hdr, int unlocked)
{
	return SecureNSModeBootImageSecureCheck(hdr, unlocked);
}

bool SecureModeRSAKeyCheck(uint8 *pKey)
{
	return SecureNSModeKeyCheck(pKey);
}


void SecureModeLockLoader(void)
{
	return SecureNSModeLockLoader();
}


uint32 SecureModeInit(void)
{
	SecureMode = SBOOT_MODE_NS;

	if (SecureMode == SBOOT_MODE_NS) {
		SecureNSModeInit();
	}

	return FTL_OK;
}
