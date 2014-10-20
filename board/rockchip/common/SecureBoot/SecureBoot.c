/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Configuation settings for the rk3xxx chip platform.
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

extern  uint32   RSA_KEY_TAG;
extern  uint32   RSA_KEY_LENGTH;
extern  uint32   RSA_KEY_DATA;
static	uint16   *RSK_KEY;

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
	uint32 ret  = FTL_OK;
	uint32 updataFlag;
#ifdef SECURE_BOOT_ENABLE
	SecureBootEn = 1;
	SecureBootCheckOK = 0;
	RSK_KEY = (uint16*)&RSA_KEY_DATA;
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
#else
	SecureBootCheckOK = 0;
	RSK_KEY = (uint16*)&RSA_KEY_DATA;
	SecureBootEn = 0; 
#endif
	SecureBootLock = 0;

#ifdef ERASE_DRM_KEY_EN
	SecureBootEraseDrmKey();
#endif

	if(StorageSysDataLoad(1,&gDrmKeyInfo) == FTL_OK)
	{
		updataFlag = 0;
		//if(SecureBootEn)
		SecureBootLock = gDrmKeyInfo.secureBootLock;
		if(SecureBootLock != 1)
			SecureBootLock = 0;

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
        
		if((RSK_KEY[0] == 0X400))
		{
#ifdef SECURE_BOOT_SET_LOCK_ALWAY
			if( gDrmKeyInfo.secureBootLock == 0)
			{
				gDrmKeyInfo.secureBootLock = 1;
				gDrmKeyInfo.secureBootLockKey = 0;
				updataFlag = 1;
			}
#endif
			if(gDrmKeyInfo.publicKeyLen==0)
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
			else if(memcmp(gDrmKeyInfo.publicKey,RSK_KEY,0x100)!=0)
			{   //如果已经存在公钥，并且公钥被替换了，那么关闭
				if(memcmp(gDrmKeyInfo.publicKey + 4, RSK_KEY, 0x100)==0)
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
		/*else  //这种情况应该不发生，没有记录public key的情况下，不应该lock住
		{
			if(gDrmKeyInfo.publicKeyLen==0)
				SecureBootLock = 0;
		}*/
        
		if(updataFlag)
		{
			updataFlag = 0;
			if(FTL_ERROR == StorageSysDataStore(1, &gDrmKeyInfo))
			{
				;// TODO:SysDataStore异常处理
			}
		}
	}
    
	if(StorageSysDataLoad(0,&gBootConfig) == FTL_OK)
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
	printf("SecureBootEn = %lx, SecureBootLock = %lx\n", SecureBootEn, SecureBootLock);
	SecureBootLock_backup = SecureBootLock;
	return ret;
}


static uint8 g_secureBootCheckBuf[512];
void SecureBootUnlock(uint8 *pKey)
{
	g_secureBootCheckBuf[0] = 0;
	g_secureBootCheckBuf[256] = 0xFF;
	if(rsaCheckMD5(pKey, pKey+256, (uint8*)gDrmKeyInfo.publicKey, 128) == 0)
	{
		ftl_memcpy(g_secureBootCheckBuf, pKey, 512);
		SecureBootLock = 0;
	}
}

void SecureBootUnlockCheck(uint8 *pKey)
{
	ftl_memcpy(pKey, g_secureBootCheckBuf, 512);
}

uint32 SecureBootDisable(void)
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

uint32 SecureBootSignCheck(uint8 * rsaHash, uint8 *Hash, uint8 length)
{
	uint32 ret  = FTL_ERROR;
	uint8  decodedHash[40];
    
	if(0 == rsaDecodeHash(decodedHash, rsaHash, (uint8*)RSK_KEY, length))//
	{
		if(0 == memcmp(Hash, decodedHash, 20))
		{
			RkPrintf("Sign OK\n");
			//SecureBootLock = gDrmKeyInfo.secureBootLock;
			ret = FTL_OK;
		}
	}
	//SecureBootLock = 0;
	return ret;
}


void SecureBootLockLoader(void)
{
	if((RSK_KEY[0] == 0X400)) {
		if(gDrmKeyInfo.secureBootLock == 0) {
			gDrmKeyInfo.secureBootLock = 1;
			gDrmKeyInfo.secureBootLockKey = 0;
			StorageSysDataStore(1, &gDrmKeyInfo);
		}
	}
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

uint32 SetSysData2Kernel(uint32 SecureBootFlag)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);

	gBootConfig.secureBootEn = SecureBootFlag;
	gBootConfig.sdPartOffset = StorageGetSDFwOffset();
	gBootConfig.bootMedia = StorageGetBootMedia();
	gBootConfig.sdSysPartOffset =  StorageGetSDSysOffset();
	gBootConfig.hash = JSHash((uint8*)&gBootConfig, 508);

	FlashSramLoadStore(&gBootConfig, 0, 1, 512);
	FlashSramLoadStore(&gDrmKeyInfo, 512, 1, 512);
	StorageSysDataLoad(2, tmp_buf); 
	FlashSramLoadStore(tmp_buf, 1024, 1, 512);          // vonder info
	FlashSramLoadStore(&gIdDataBuf[384], 1536, 1, 512);  // idblk sn info

	return 0;
}

