/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "../config.h"

uint32	SecureMode;
uint32  SecureBootEn;
uint32  SecureBootCheckOK;
uint32  SecureBootLock;
uint32  SecureBootLock_backup;

#ifdef CONFIG_RK_NVME_BOOT_EN
BOOT_CONFIG_INFO gBootConfig __attribute__((aligned(SZ_4K)));
DRM_KEY_INFO gDrmKeyInfo __attribute__((aligned(SZ_4K)));
#else
BOOT_CONFIG_INFO gBootConfig __attribute__((aligned(ARCH_DMA_MINALIGN)));
DRM_KEY_INFO gDrmKeyInfo __attribute__((aligned(ARCH_DMA_MINALIGN)));

#endif

#ifdef ERASE_DRM_KEY_EN
void SecureBootEraseDrmKey(void)
{
#ifdef CONFIG_RK_NVME_BOOT_EN
	ALLOC_ALIGN_BUFFER(u8, buf, 512, SZ_4K);
#else
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, 512);
#endif
	printf("erase drm key for debug!\n");
	memset(buf, 0, 512);
	StorageSysDataStore(1, buf);
}
#endif


uint32 SecureBootCheck(void)
{
	SecureBootEn = 0;
	SecureBootLock = 0;
	SecureBootCheckOK = 0;

#ifdef ERASE_DRM_KEY_EN
	SecureBootEraseDrmKey();
#endif

	SecureModeInit();

	printf("SecureBootEn = %u, SecureBootLock = %u\n", SecureBootEn, SecureBootLock);
	SecureBootLock_backup = SecureBootLock;

	return 0;
}


static uint8 g_secureBootCheckBuf[512];
void SecureBootUnlock(uint8 *pKey)
{
	g_secureBootCheckBuf[0] = 0;
	g_secureBootCheckBuf[256] = 0xFF;

	if (SecureModeRSAKeyCheck(pKey) != 0) {
		ftl_memcpy(g_secureBootCheckBuf, pKey, 512);
		SecureBootLock = 0;
	}
}

void SecureBootUnlockCheck(uint8 *pKey)
{
	ftl_memcpy(pKey, g_secureBootCheckBuf, 512);
}


void SecureBootLockLoader(void)
{
	SecureModeLockLoader();
}


static void FlashSramLoadStore(void *pBuf, uint32 offset, uint32 dir, uint32 length)
{
	uint8 *pSramAddr = (uint8 *)BOOTINFO_RAM_BASE;

	if (dir == 0)
	{
		ftl_memcpy(pBuf, pSramAddr + offset, length);
	}
	else
	{
		ftl_memcpy(pSramAddr + offset, pBuf, length);
	}
}

static uint32 JSHashBase(uint8 * buf, uint32 len, uint32 hash)
{
	uint32 i;

	for(i=0;i<len;i++)
	{
		hash ^= ((hash << 5) + buf[i] + (hash >> 2));
	}

	return hash;
}

static uint32 JSHash(uint8 * buf, uint32 len)
{
	return(JSHashBase(buf, len, 0x47C6A7E6));
}

uint32 SecureBootSetSysData2Kernel(uint32 SecureBootFlag)
{
#ifdef CONFIG_RK_NVME_BOOT_EN
	ALLOC_ALIGN_BUFFER(u8, tmp_buf, 512, SZ_4K);
#else
	ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);
#endif
	gBootConfig.secureBootEn = SecureBootFlag;
	gBootConfig.sdPartOffset = StorageGetSDFwOffset();
	gBootConfig.bootMedia = StorageGetBootMedia();
	gBootConfig.sdSysPartOffset =  StorageGetSDSysOffset();
	gBootConfig.hash = JSHash((uint8*)&gBootConfig, 508);

	StorageSysDataLoad(2, tmp_buf);
/* rk3399 and rk322xh don't save boot info in SRAM for kernel */
#if !defined(CONFIG_RKCHIP_RK3399) && !defined(CONFIG_RKCHIP_RK322XH)
	FlashSramLoadStore(&gBootConfig, 0, 1, 512);
	FlashSramLoadStore(&gDrmKeyInfo, 512, 1, 512);
	FlashSramLoadStore(tmp_buf, 1024, 1, 512);          // vonder info
	FlashSramLoadStore(&gIdDataBuf[384], 1536, 1, 512);  // idblk sn info
#endif

	return 0;
}


uint32 SecureBootSecureDisable(void)
{
	uint32 ret  = FTL_OK;

#ifndef SECURE_BOOT_ENABLE_ALWAY
	if(SecureBootEn)
	{
		if(gBootConfig.bootTag != 0x44535953)
		{
			gBootConfig.bootTag = 0x44535953;
			gBootConfig.bootLen = 504;
			gBootConfig.bootMedia = 0;// TODO: boot 选择
			gBootConfig.BootPart = 0;
		}
		gBootConfig.secureBootEn = 0;
		if(FTL_OK == StorageSysDataStore(0, &gBootConfig))
		{
			SecureBootEn = 0;
		}

		if(gDrmKeyInfo.drmtag != 0x4B4D5244)
		{
			gDrmKeyInfo.drmtag = 0x4B4D5244;
			gDrmKeyInfo.drmLen = 504;
		}
		gDrmKeyInfo.drmKeyLen = 0;
		memset( gDrmKeyInfo.drmKey, 0, 0x80);
		if(FTL_ERROR == StorageSysDataStore(1, &gDrmKeyInfo))
		{
			;// TODO:SysDataStore异常处理
		}
	}
#endif

	return ret;
}


uint32 SecureBootImageCheck(rk_boot_img_hdr *hdr, int unlocked)
{
	return SecureModeBootImageCheck(hdr, unlocked);
}


void SecureBootSecureState2Kernel(uint32 SecureState)
{
	printf("Secure Boot state: %d\n", SecureState);
	if(SecureState == 0) {
		SecureBootSecureDisable();
	}

	SecureBootSetSysData2Kernel(SecureState);
}

