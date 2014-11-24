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

uint32	SecureMode;
uint32  SecureBootEn;
uint32  SecureBootCheckOK;
uint32  SecureBootLock;
uint32  SecureBootLock_backup;

BOOT_CONFIG_INFO gBootConfig __attribute__((aligned(ARCH_DMA_MINALIGN)));
DRM_KEY_INFO gDrmKeyInfo __attribute__((aligned(ARCH_DMA_MINALIGN)));


#ifdef ERASE_DRM_KEY_EN
void SecureBootEraseDrmKey(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, 512);

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

	printf("SecureBootEn = %lx, SecureBootLock = %lx\n", SecureBootEn, SecureBootLock);
	SecureBootLock_backup = SecureBootLock;
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
	uint8 *pSramAddr = (uint8 *)(NANDC_BASE_ADDR + 0x1000);

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
	ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);

	gBootConfig.secureBootEn = SecureBootFlag;
	gBootConfig.sdPartOffset = StorageGetSDFwOffset();
	gBootConfig.bootMedia = StorageGetBootMedia();
	gBootConfig.sdSysPartOffset =  StorageGetSDSysOffset();
	gBootConfig.hash = JSHash((uint8*)&gBootConfig, 508);

	StorageSysDataLoad(2, tmp_buf);
	FlashSramLoadStore(&gBootConfig, 0, 1, 512);
	FlashSramLoadStore(&gDrmKeyInfo, 512, 1, 512);
	FlashSramLoadStore(tmp_buf, 1024, 1, 512);          // vonder info
	FlashSramLoadStore(&gIdDataBuf[384], 1536, 1, 512);  // idblk sn info

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


uint32 SecureBootImageSecureCheck(rk_boot_img_hdr *hdr, int unlocked)
{
	return SecureModeBootImageSecureCheck(hdr, unlocked);
}


void SecureBootSecureState2Kernel(uint32 SecureState)
{
	printf("Secure Boot state: %d\n", SecureState);
	if(SecureState == 0) {
		SecureBootSecureDisable();
	}

	SecureBootSetSysData2Kernel(SecureState);
}

