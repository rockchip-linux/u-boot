

#include    "../armlinux/config.h"


extern  uint32   RSA_KEY_TAG;
extern  uint32   RSA_KEY_LENGTH;
extern  uint32   RSA_KEY_DATA;
uint16  * RSK_KEY;

unsigned long rsaDecodeHash(unsigned char *output, unsigned char *input,unsigned char *publicKey,unsigned char inputlen);
uint32  SecureBootEn;
uint32  SecureBootCheckOK;
uint32  SecureBootLock;
uint32  SecureBootLock_backup;

#define RKNAND_SYS_STORGAE_DATA_LEN 504
typedef struct tagRKNAND_SYS_STORGAE
{
    uint32  tag;
    uint32  len;
    uint8   data[RKNAND_SYS_STORGAE_DATA_LEN];
}RKNAND_SYS_STORGAE;

typedef struct tagSYS_DATA_INFO
{
    uint32 systag;          // "SYSD" 0x44535953
    uint32 syslen;          // 504
    uint32 sysdata[(512-8)/4];  //

    uint32 drmtag;          // "DRMK" 0x4B4D5244
    uint32 drmlen;          // 504
    uint32 drmdata[(512-8)/4];  //

    uint32 reserved[(2048-1024)/4];  //保留
}SYS_DATA_INFO, *pSYS_DATA_INFO;


typedef struct tagBOOT_CONFIG_INFO
{
    uint32 bootTag;          // "SYSD" 0x44535953
    uint32 bootLen;          // 504
    uint32 bootMedia;        // 1:flash 2:emmc 4:sdcard0 8:sdcard1
    uint32 BootPart;         // 0 disable , 1~N : part 1~N
    uint32 secureBootEn;     // 0 disable , 1:enable
    uint32 sdPartOffset;     
    uint32 sdSysPartOffset;  
    uint32 sys_reserved[(508-28)/4];
    uint32 hash; // 0 disable , 1:enable
}BOOT_CONFIG_INFO,*pBOOT_CONFIG_INFO;


typedef struct tagDRM_KEY_INFO
{
    uint32 drmtag;           // "DRMK" 0x4B4D5244
    uint32 drmLen;           // 504
    uint32 keyBoxEnable;     // 0:flash 1:emmc 2:sdcard1 3:sdcard2
    uint32 drmKeyLen;        //0 disable , 1~N : part 1~N
    uint32 publicKeyLen;     //0 disable , 1:enable
    uint32 secureBootLock;   //0 disable , 1:lock
    uint32 secureBootLockKey;//加解密是使用
    uint32 reserved0[(0x40-0x1C)/4];
    uint8  drmKey[0x80];      // key data
    uint32 reserved2[(0xFC-0xC0)/4];
    uint8  publicKey[0x104];      // key data
}DRM_KEY_INFO,*pDRM_KEY_INFO;

BOOT_CONFIG_INFO gBootConfig;
DRM_KEY_INFO gDrmKeyInfo;

void RKLockLoader(void)
{
    if((RSK_KEY[0] == 0X400))
    {
        if( gDrmKeyInfo.secureBootLock == 0)
        {
            gDrmKeyInfo.secureBootLock = 1;
            gDrmKeyInfo.secureBootLockKey = 0;
            StorageSysDataStore(1, &gDrmKeyInfo);
        }
    }
}

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
        RkPrintf("unsigned!\n","");
    }
#else
    SecureBootCheckOK = 0;
    RSK_KEY = (uint16*)&RSA_KEY_DATA;
    SecureBootEn = 0; 
#endif
    SecureBootLock = 0;

#ifdef ERASE_DRM_KEY_EN
    if(gSysData[128+4] != 0 )//publicKeyLen
    {
        RkPrintf("erase drm key for debug.!\n","");
        ftl_memset(gSysData,0,2048);
        StorageSysDataStore(1, gSysData);
    }
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
                ftl_memcpy(gDrmKeyInfo.publicKey,RSK_KEY,0x104);
                updataFlag = 1;
                gDrmKeyInfo.drmKeyLen = 0;
                gDrmKeyInfo.keyBoxEnable = 1;
                gDrmKeyInfo.secureBootLockKey = 0;
                memset( gDrmKeyInfo.drmKey , 0 , 0x80);
#ifdef SECURE_BOOT_LOCK
                gDrmKeyInfo.secureBootLock = 1;
#endif
            }
            else if(memcmp(gDrmKeyInfo.publicKey,RSK_KEY,0x100)!=0)
            {   //如果已经存在公钥，并且公钥被替换了，那么关闭
                if(memcmp(gDrmKeyInfo.publicKey + 4,RSK_KEY,0x100)==0)
                {
                    ftl_memcpy(gDrmKeyInfo.publicKey,RSK_KEY,0x104);
                    updataFlag = 1;
                }
                else
                {
                    gDrmKeyInfo.keyBoxEnable = 0; //暂时不启用这个功能
                    SecureBootEn = 0;
                    RkPrintf("E:pKey!\n","");
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
        RkPrintf("no sys part.\n","");
        SecureBootEn = 0;
    }
    RkPrintf("SecureBootEn = %d %d\n",SecureBootEn,SecureBootLock);
    SecureBootLock_backup = SecureBootLock;
    return ret;
}


uint8 g_secureBootCheckBuf[512];
void SecureBootUnlock(uint8 *pKey)
{
    g_secureBootCheckBuf[0] = 0;
    g_secureBootCheckBuf[256] = 0xFF;
    if(rsaCheckMD5(pKey, pKey+256, (uint8*)gDrmKeyInfo.publicKey , 128) == 0)
    {
        ftl_memcpy(g_secureBootCheckBuf,pKey,512);
        SecureBootLock = 0;
    }
}

void SecureBootUnlockCheck(uint8 *pKey)
{
    ftl_memcpy(pKey,g_secureBootCheckBuf,512);
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
        memset( gDrmKeyInfo.drmKey , 0 , 0x80);
        if(FTL_ERROR == StorageSysDataStore(1, &gDrmKeyInfo))
        {
            ;// TODO:SysDataStore异常处理
        } 
    }
#endif
    return ret;
}

uint32 SecureBootSignCheck(uint8 * rsaHash,uint8 *Hash , uint8 length)
{
    uint32 ret  = FTL_ERROR;
    uint32 i;
    uint8  decodedHash[40];
    
    if(0 == rsaDecodeHash(decodedHash, rsaHash, (uint8*)RSK_KEY ,length))//
    {
        if(0 == memcmp(Hash,decodedHash,20))
        {
            RkPrintf("Sign OK\n","");
            //SecureBootLock = gDrmKeyInfo.secureBootLock;
            ret = FTL_OK;
        }
    }
    //SecureBootLock = 0;
    return ret;
}

void FlashSramLoadStore(void *pBuf,uint32 offset,uint32 dir,uint32 length)
{
    uint8 *pSramAddr = (uint8*)(NANDC_BASE_ADDR + 0x1000);
    
    if (dir == 0)
    {
        ftl_memcpy(pBuf, pSramAddr + offset, length);
    }
    else
    {
        ftl_memcpy(pSramAddr + offset, pBuf, length);
    }
}

uint32 JSHashBase(uint8 * buf,uint32 len,uint32 hash)
{
    uint32 i;
    for(i=0;i<len;i++)
    {
        hash ^= ((hash << 5) + buf[i] + (hash >> 2));
    }
    return hash;
}

uint32 JSHash(uint8 * buf,uint32 len)
{
    return(JSHashBase(buf,len,0x47C6A7E6));
}

uint32 SetSysData2Kernel(uint32 SecureBootFlag)
{
    uint8 tmp_buf[512];
    gBootConfig.secureBootEn = SecureBootFlag;
    gBootConfig.sdPartOffset = StorageGetSDFwOffset();//gSdmmcFwPartOffset;
    gBootConfig.bootMedia = StorageGetBootMedia();//(uint32)gBootMedia;
    gBootConfig.sdSysPartOffset =  StorageGetSDSysOffset();//gSdmmcSysPartOffset;
#ifdef RK_SD_BOOT
    PRINT_E("sdpart %x %x %x\n",gBootConfig.sdPartOffset,gBootConfig.bootMedia,gBootConfig.sdSysPartOffset); 
#endif
    gBootConfig.hash = JSHash((uint8*)&gBootConfig,508);
    //printf("gBootMedia = %x\n",gBootMedia);
    //printf("gSdmmcFwPartOffset = %x\n",gSdmmcFwPartOffset);
    
    FlashSramLoadStore(&gBootConfig, 0,1, 512);
    FlashSramLoadStore(&gDrmKeyInfo, 512,1, 512);
    StorageSysDataLoad(2,tmp_buf); 
    FlashSramLoadStore(tmp_buf, 1024 ,1, 512);          // vonder info
    FlashSramLoadStore(&gIdDataBuf[384], 1536,1, 512);  // idblk sn info
    return 0;
}



