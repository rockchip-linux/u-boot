/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:  flash.C
Author:     XUESHAN LIN
Created:    1st Dec 2008
Modified:
Revision:   1.00
********************************************************************************
********************************************************************************/
#include	"sdmmc_config.h"

#ifdef  DRIVERS_SDMMC
uint32 gUseEmmc;
#define NO_RESET 0
#define NEED_RESET 1
uint32 BootCapSize;
uint32 UserCapSize;
#define EMMC_BOOT_PART_SIZE         1024
#define SD_CARD_BOOT_PART_OFFSET    64
#define EMMC_CARD_ID                2
#define SD_CARD_FW_PART_OFFSET      8192
#define SD_CARD_SYS_PART_OFFSET     8064

typedef struct SDCardInfoTag
{
    uint16 Valid;
    uint16 AccessPart;
    uint32 FwPartOffset;
    uint32 UserPartOffset;
    uint32 UserPartSize;
    uint32 BootCapSize;
    uint32 UserCapSize;
}SD_Card_Info;

SD_Card_Info gSdCardInfoTbl[3];

void SdmmcSDMInit()
{
//    emmcGpioInit();
//    eMMC_changemode(0);
//    SDM_Init();
//    EmmcPowerEn(1);
    ftl_memset(gSdCardInfoTbl,0,sizeof(gSdCardInfoTbl));
}

void emmc_dev_reset(void)
{
    EmmcPowerEn(0);
#if(PALTFORM==RK29XX)
    DRVDelayMs(50);
#else
    DRVDelayMs(5);
#endif		
    EmmcPowerEn(1);
    DRVDelayMs(1);
}


uint32 SdmmcReinit(uint32 ChipSel)
{
    int32  ret1 = SDM_SUCCESS;
    uint32 ioctlParam[5] = {0,0,0,0,0};
    uint32 retry = 2;
    if(ChipSel == 2)
    {
        eMMC_changemode(0);
        EmmcPowerEn(1);
    }
    SDM_Init(ChipSel);
    sdmmcGpioInit(ChipSel);   //放在sdm init之后，避免rst输出低电平
EMMC_INIT_retry:    
    eMMC_SetDataHigh();
    ioctlParam[0] = ChipSel;
    ret1 = SDM_IOCtrl(SDM_IOCTRL_REGISTER_CARD, ioctlParam);
    if(ret1 != SDM_SUCCESS && retry > 0)
    {
        retry--; 
        if(ChipSel == 2)
        {
            PRINT_E("emmc reset for reinit\n");
            emmc_dev_reset();
        }
        goto EMMC_INIT_retry;
    }
    PRINT_E("SdmmcInit=%x %x\n",ChipSel,ret1);
    if(ret1 == SDM_SUCCESS)  //卡能识别
    {
        SDM_Open(ChipSel);
    }
    return ret1;
}
//#define FTL_DEBUG
#ifdef FTL_DEBUG
uint16 pwrite[32768];
uint16 pread[32768];
#define TestFileLen  1024*1024*1024 //1GB
#define TestSector   64 
#define TestStartSector   0x200000 //1GB
void FTLTest(uint8 ChipSel)
{
	uint16 i,j, loop = 0;
	uint32 block;
    uint32 TestEndLBA;
    uint32 TestLBA = 0;
    uint16 TestSecCount = 1;
    uint16 printFlag;
    uint32 start,end;
    uint32 TestCountNUM=0;

    TestEndLBA = TestStartSector + 2048*32;
    for (i=0; i<32768; i++)
        pwrite[i]=i;
    if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
        EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_DATA_PART);

    start=get_timer(0);
    TestCountNUM=TestStartSector;
    for(i=0;i<=TestFileLen/512/64;i++ ){
        pwrite[0]=i;
    	SDM_Write(ChipSel,TestCountNUM,64,pwrite);
	TestCountNUM+=64;
    }
    end=get_timer(0);
    printf("----->write start=%x end=%x time=%d",start,end,start>end?start-end:end-start);
    start=get_timer(0);
    TestCountNUM=TestStartSector;
    for(i=0;i<=TestFileLen/512/64;i++ ){
    	SDM_Read(ChipSel,TestCountNUM,64,pread);
	TestCountNUM+=64;
    }
    end=get_timer(0);
    printf("----->read start=%x end=%x time=%d",start,end,start>end?start-end:end-start);
#if 0
    TestCountNUM=TestStartSector;
    for(i=0;i<=TestFileLen/512/64;i++ ){
    	SDM_Read(ChipSel,TestCountNUM,64,pread);
	TestCountNUM+=64;
	for (j=0; j<64*256; j++)
	{
		if(j==0){
			if (pread[0] != i)
			{
				PRINT_E("write not match:row=%x, num=%x, write=%x, read=%x\n", TestCountNUM, j,i, pread[0]);
				break;
			}
		}else if (pwrite[j] != pread[j])
		{
			PRINT_E("write not match:row=%x, num=%x, write=%x, read=%x\n", TestLBA, j, pwrite[j], pread[j]);
			break;
		}
	}
    }
#endif
    PRINT_E("--------trans end---------\n", TestLBA, j, pwrite[j], pread[j]);



#if 0
    for(loop = 0;loop<10;loop ++)
    {
        PRINT_E("---------Test loop = %d---------\n",loop);
        PRINT_E("---------Test ftl write---------\n");
        TestSecCount = 1;
        PRINT_E("TestEndLBA = %x\n",TestEndLBA);
        PRINT_E("TestLBA = %x\n",TestLBA);
    	for (TestLBA=TestStartSector + loop; (TestLBA + TestSecCount) <TestEndLBA;)//SysAreaBlock
    	{
            PRINT_E("SDM_Write = %x\n",TestLBA);
            SDM_Write(ChipSel,TestLBA,TestSecCount,pwrite);
            PRINT_E("SDM_Read = %x\n",TestLBA);
            SDM_Read(ChipSel,TestLBA,TestSecCount,pread);
            printFlag = TestLBA&0x7FF;
            if(printFlag < TestSecCount)
            PRINT_E("TestLBA = %x\n",TestLBA);
           
            for (j=0; j<TestSecCount*256; j++)
            {
                if (pwrite[j] != pread[j])
                {
                    PRINT_E("write not match:row=%x, num=%x, write=%x, read=%x\n", TestLBA, j, pwrite[j], pread[j]);
                    break;
                }
            }
            TestLBA+=TestSecCount;
            TestSecCount++;
            if(TestSecCount > TestSector)
                TestSecCount = 1;
    	}
        PRINT_E("---------Test ftl check---------\n");

        TestSecCount = 1;
    	for (TestLBA=TestStartSector + loop; (TestLBA + TestSecCount) <TestEndLBA;)//SysAreaBlock
    	{
            SDM_Read(ChipSel,TestLBA,TestSecCount,pread);
            printFlag = TestLBA&0x7FF;
            if(printFlag < TestSecCount)
                PRINT_E("TestLBA = %x\n",TestLBA);
           

            for (j=0; j<TestSecCount*256; j++)
            {
                if (pwrite[j] != pread[j])
                {
                    PRINT_E("check not match:row=%x, num=%x, write=%x, read=%x\n", TestLBA, j, pwrite[j], pread[j]);
                    break;
                }
            }
            TestLBA+=TestSecCount;
            TestSecCount++;
            if(TestSecCount > TestSector)
                TestSecCount = 1;
    	}
    }
    PRINT_E("---------Test end---------\n");
#endif
}
#endif
uint32 emmcdebug_on = 1;

uint32 SdmmcInit(uint32 ChipSel)
{
    int32  ret1 = SDM_SUCCESS;
    int32 ret;
    int count =0;
    uint32 ioctlParam[5] = {0,0,0,0,0};
    ret1 = SdmmcReinit(ChipSel);
    if(ret1 == SDM_SUCCESS)  //卡能识别
    {
        gSdCardInfoTbl[ChipSel].AccessPart = EMMC_BOOT_PART;
        ioctlParam[0] = ChipSel;
        ioctlParam[1] = 0;  //capbility
        ret1 = SDM_IOCtrl(SDM_IOCTR_GET_BOOT_CAPABILITY, ioctlParam);
        gSdCardInfoTbl[ChipSel].BootCapSize = ioctlParam[1];
      	//PRINT_E("BootCapSize=%lx\n", gSdCardInfoTbl[ChipSel].BootCapSize);
      	
      	if(gSdCardInfoTbl[ChipSel].BootCapSize != 0)
        	gSdCardInfoTbl[ChipSel].BootCapSize = EMMC_BOOT_PART_SIZE;

        ioctlParam[0] = ChipSel;
        ioctlParam[1] = 0;  //capbility
        ret1 = SDM_IOCtrl(SDM_IOCTR_GET_CAPABILITY, ioctlParam);
        gSdCardInfoTbl[ChipSel].UserCapSize  = ioctlParam[1];
        //PRINT_E("UserCapSize=%lx\n", gSdCardInfoTbl[ChipSel].UserCapSize);
        if(gSdCardInfoTbl[ChipSel].BootCapSize)
        {
            gSdCardInfoTbl[ChipSel].AccessPart = gEmmcBootPart;
            //boot partition1
            ioctlParam[0] = ChipSel;
            ioctlParam[1] = gEmmcBootPart;  //boot partition1.
            ret1 = SDM_IOCtrl(SDM_IOCTR_ACCESS_BOOT_PARTITION, ioctlParam);
            if(ret1 == SDM_SUCCESS)
            {
            }
        	SDM_Read(ChipSel,0,4,gIdDataBuf); // id blk data
            EmmcSetBootPart(ChipSel,gEmmcBootPart,EMMC_DATA_PART);
        	SDM_Read(ChipSel,SD_CARD_BOOT_PART_OFFSET,4,Data); // id blk data
            if(Data[0] == 0xFCDC8C3B )
                gSdCardInfoTbl[ChipSel].FwPartOffset = SD_CARD_FW_PART_OFFSET; 
        }
        else
        {
        	ret1 = SDM_Read(ChipSel,SD_CARD_BOOT_PART_OFFSET,4,gIdDataBuf); // id blk data
        	#ifdef RK_SD_BOOT
            PRINT_E("gIdDataBuf[0]=%lx ret1 = %x\n", gIdDataBuf[0],ret1);
            #endif
            if(gIdDataBuf[0] == 0xFCDC8C3B )
                gSdCardInfoTbl[ChipSel].FwPartOffset = SD_CARD_FW_PART_OFFSET; 
            //check sd0 boot
        }
      	PRINT_E("FwPartOffset=%lx , %x\n", gSdCardInfoTbl[ChipSel].FwPartOffset,BootCapSize);
        //PRINT_E("SdmmcInit OK!!");
        gSdCardInfoTbl[ChipSel].Valid = 1;
        if( gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
        {
            SDM_Read(ChipSel,SD_CARD_SYS_PART_OFFSET,4,gSysData );
        }      
#ifdef FTL_DEBUG
        FTLTest(ChipSel);
#endif
        return OK;
    }
    return ERROR;
}

uint32 SdmmcDeInit(void)
{
    int32  ret = SDM_SUCCESS;
    if(gSdCardInfoTbl[EMMC_CARD_ID].BootCapSize)
    {
        uint32 ioctlParam[5] = {0,0,0,0,0};
        gEmmcBootPart = EMMC_BOOT_PART;
        gSdCardInfoTbl[EMMC_CARD_ID].AccessPart = EMMC_BOOT_PART;
        ioctlParam[0] = EMMC_CARD_ID;
        ioctlParam[1] = EMMC_BOOT_PART;
        SDM_IOCtrl(SDM_IOCTR_ACCESS_BOOT_PARTITION, ioctlParam);
        eMMC_Switch_ToMaskRom();
        emmc_dev_reset();
    }
    return ret;
}

uint32 EmmcSetBootPart(uint32 ChipSel,uint32 BootPart,uint32 AccessPart)
{
    //写第二份数据，从第一个boot区引导
    if(gEmmcBootPart != BootPart || gSdCardInfoTbl[ChipSel].AccessPart != AccessPart)
    {
        uint32 ioctlParam[5] = {0,0,0,0,0};
        gEmmcBootPart = BootPart;
        gSdCardInfoTbl[ChipSel].AccessPart = AccessPart;
        ioctlParam[0] = ChipSel;
        ioctlParam[1] = AccessPart;
        SDM_IOCtrl(SDM_IOCTR_ACCESS_BOOT_PARTITION, ioctlParam);
    }   
}

uint32 SdmmcBootWritePBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    uint32 i;
    uint16 len;
    uint8 pageSizeRaw;
    uint8 pageSizeLimit;
    uint32 BlockOffset;
    uint16 PageOffset;
    uint32 *pDataBuf = pbuf;
    pageSizeLimit = DATA_LEN/128;
    if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
    {
        memset(SpareBuf,0xFF,SPARE_LEN);
        if(PBA + nSec >= EMMC_BOOT_PART_SIZE*2) //超出了不写
            return 0;

        if(PBA + nSec >= EMMC_BOOT_PART_SIZE) //超出了不写
        {
            //写第二份数据，从第一个boot区引导
            EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_BOOT_PART2);
            PBA &= (EMMC_BOOT_PART_SIZE-1);
        }
        else
        {
            //写第一份数据，从第二个boot区引导
            EmmcSetBootPart(ChipSel,EMMC_BOOT_PART2,EMMC_BOOT_PART);
        }
    }
    else
    {
        if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
        {
            if(PBA + nSec >= EMMC_BOOT_PART_SIZE*5) //超出了不写
                return 0;
            PBA = PBA + SD_CARD_BOOT_PART_OFFSET;
        }
        else
        {
            return -1;//异常
        }
    }

    for (len=0; len<nSec; len+=pageSizeLimit)
    {
        for (i=0; i<(MIN(nSec,pageSizeLimit)); i++)
        {
            ftl_memcpy(Data+i*128, pDataBuf+(len+i)*132, 512);
            ftl_memcpy(SpareBuf+i*4, pDataBuf+(len+i)*132+128, 16);
        }
        SDM_Write(ChipSel,PBA,(MIN(nSec,pageSizeLimit)),Data);
    }
    
    return 0;
}

uint32 SdmmcBootReadPBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    uint32 i;
    uint32 ret;
    uint16 len;
    uint8 pageSizeRaw;
    uint8 pageSizeLimit;
    uint32 BlockOffset;
    uint16 PageOffset;
    uint16 PageAlignOffset;
    uint16 idblk_flag = 0;
    uint16 read_len;
    uint32 *pDataBuf = pbuf;

    pageSizeLimit = DATA_LEN/128;
    memset(SpareBuf,0xFF,SPARE_LEN);

    if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
    {
        if(PBA + nSec >= EMMC_BOOT_PART_SIZE*2) //超出了只读第一份
            PBA &= (EMMC_BOOT_PART_SIZE-1);
    
        if(PBA + nSec >= EMMC_BOOT_PART_SIZE) 
        {
            //读第二份数据，从第一个boot区引导
            EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_BOOT_PART2);
            PBA &= (EMMC_BOOT_PART_SIZE-1);
        }
        else
        {
            //读第一份数据 ，从第一个boot区引导
            EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_BOOT_PART);
        }
    }
    else
    {
        if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
        {
            if(PBA + nSec >= EMMC_BOOT_PART_SIZE*5) //超出了不写
                PBA &= (EMMC_BOOT_PART_SIZE-1);
            PBA = PBA + SD_CARD_BOOT_PART_OFFSET;
        }
        else
        {
            return -1;//异常
        }
    }
        
    for (len=0; len<nSec; len+=pageSizeLimit)
    {
        SDM_Read(ChipSel,PBA,(MIN(nSec,pageSizeLimit)),Data);
        for (i=0; i<(MIN(nSec,pageSizeLimit)); i++)
        {
            ftl_memcpy(pDataBuf+(len+i)*132, Data+(i)*128, 512);
            ftl_memcpy(pDataBuf+(len+i)*132+128, SpareBuf+(i)*4, 16);
        }
    }

    return 0;
}

uint32 SdmmcBootReadLBA(uint8 ChipSel,uint32 LBA , void *pbuf , uint16 nSec)
{
    uint32 iret = FTL_OK;
    if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
    {
        EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_DATA_PART);
    }
    else
    {
        if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
        {
            ;//LBA = LBA + SD_CARD_FW_PART_OFFSET;
        }
        else
        {
            return -1;//异常
        }
    }
    iret = SDM_Read(ChipSel,LBA + gSdCardInfoTbl[ChipSel].FwPartOffset,nSec,pbuf);
    if(iret!=FTL_OK)
        RkPrintf("SDM_Read FLT_ERROR,LBA=%x,iret = %x\n" , LBA,iret);
    return (iret);
}

uint32 SdmmcBootWriteLBA(uint8 ChipSel,uint32 LBA,  void *pbuf ,uint16 nSec  ,uint16 mode)
{
    uint32 iret = FTL_OK;
    if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
    {
        EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_DATA_PART);
    }
    else
    {
        if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
        {
            ;//LBA = LBA + SD_CARD_FW_PART_OFFSET;
        }
        else
        {
            return -1;//异常
        }
    }
    iret = SDM_Write(ChipSel,LBA  + gSdCardInfoTbl[ChipSel].FwPartOffset,nSec,pbuf);
    if(iret!=FTL_OK)
        RkPrintf("SDM_Write FLT_ERROR,LBA=%x,iret = %x\n" , LBA,iret);
    return iret;
}


void SdmmcReadID(uint8 ChipSel, void *buf)
{
    uint8 * pbuf = buf;
    pbuf[0] = 'E';
    pbuf[1] = 'M';
    pbuf[2] = 'M';
    pbuf[3] = 'C';
    pbuf[4] = ' ';
}

void SdmmcReadFlashInfo(void *buf)
{
    pFLASH_INFO pInfo=(pFLASH_INFO)buf;
    ftl_memset((uint8*)buf,0,512);
    pInfo->BlockSize = EMMC_BOOT_PART_SIZE;
    pInfo->ECCBits = 0;
    pInfo->FlashSize = gSdCardInfoTbl[EMMC_CARD_ID].UserCapSize; 
    pInfo->PageSize = 4;
    pInfo->AccessTime = 40;
    pInfo->ManufacturerName=0;
    pInfo->FlashMask = 0;
    pInfo->FlashMask=1;
}

uint32 SdmmcGetCapacity(uint8 ChipSel)
{
    return gSdCardInfoTbl[ChipSel].UserCapSize - gSdCardInfoTbl[ChipSel].FwPartOffset;
}

uint32 SdmmcSysDataLoad(uint8 ChipSel, uint32 Index,void *Buf)
{
    uint32 ret = FTL_ERROR;
#if(PALTFORM!=RK28XX)
    if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
    {
        int32_t tmpBuf[512 / sizeof(int32_t)];
        if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
        {
            EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_DATA_PART);
        }
        ret = SDM_Read(ChipSel,SD_CARD_SYS_PART_OFFSET + Index,1,&tmpBuf);
        ftl_memcpy(Buf, &tmpBuf, 512);
        if (Index <= 3)
            ftl_memcpy(&gSysData[Index*128], &tmpBuf, 512);
        //PRINT_E("SdmmcSysDataLoad , %x, %x ret=%x\n",ChipSel,Index,ret);
    }
#endif
    return ret;
}

uint32 SdmmcSysDataStore(uint8 ChipSel, uint32 Index,void *Buf)
{
    uint32 ret = FTL_ERROR;
#if(PALTFORM!=RK28XX)
    if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
    {
        int32_t tmpBuf[512 / sizeof(int32_t)];
        if(gSdCardInfoTbl[ChipSel].BootCapSize > 0)
        {
            EmmcSetBootPart(ChipSel,EMMC_BOOT_PART,EMMC_DATA_PART);
        }
        ftl_memcpy(&tmpBuf,Buf, 512);
        ret = SDM_Write(ChipSel,SD_CARD_SYS_PART_OFFSET  + Index,1,&tmpBuf);
        //PRINT_E("SdmmcSysDataStore , %x, %x ret=%x\n",ChipSel,Index,ret);
        if (Index <= 3)
            ftl_memcpy(&gSysData[Index*128],&tmpBuf, 512);
    }
#endif
    return ret;
}

uint32 SdmmcGetFwOffset(uint8 ChipSel)
{
    uint32 offset = 0;
    if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
    {
        offset = SD_CARD_FW_PART_OFFSET;
    }
    return offset;
}

uint32 SdmmcGetSysOffset(uint8 ChipSel)
{
    uint32 offset = 0;
    if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
    {
        offset = SD_CARD_SYS_PART_OFFSET;
    }
    return offset;
}


/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:ChipSel, blkIndex=起始block号,  nblk=blk数 ，mod: 0 为普通擦除； 1为强制擦除
出口参数:0 为没有坏块，1为有坏块
调用函数:无
***************************************************************************/
uint32 SdmmcBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod)
{
	return OK;
}

#define IDblockDataSize  (EMMC_BOOT_PART_SIZE*512)
uint32 *IDBlockData0;//[IDblockDataSize/4];
uint32 *IDBlockData1;//[IDblockDataSize/4];
void EmmcIdBlockRecover(uint8 part)
{
    //前一次升级没有完成，引导区设置为boot2区，boot1区可能数据不正确
    if(gEmmcBootPart != part && gSdCardInfoTbl[EMMC_CARD_ID].BootCapSize > 0) 
    {  
        uint32 ioctlParam[5] = {0,0,0,0,0};
        IDBlockData0 = &usbXferBuf[0];
        IDBlockData1 = &usbXferBuf[IDblockDataSize/4];

        ioctlParam[0] = EMMC_CARD_ID;
        ioctlParam[1] = gEmmcBootPart;
        gSdCardInfoTbl[EMMC_CARD_ID].AccessPart = gEmmcBootPart;
        SDM_IOCtrl(SDM_IOCTR_ACCESS_BOOT_PARTITION, ioctlParam);
        if(SDM_Read(EMMC_CARD_ID,0,EMMC_BOOT_PART_SIZE,IDBlockData1) != SDM_SUCCESS)
        {
            return ;
        }
        
        ioctlParam[0] = EMMC_CARD_ID;
        ioctlParam[1] = part;
        gSdCardInfoTbl[EMMC_CARD_ID].AccessPart  = part;
        SDM_IOCtrl(SDM_IOCTR_ACCESS_BOOT_PARTITION, ioctlParam);
        SDM_Read(EMMC_CARD_ID,0,EMMC_BOOT_PART_SIZE,IDBlockData0);
        if(memcmp(IDBlockData1,IDBlockData0,IDblockDataSize) == 0)
        {
            gEmmcBootPart = EMMC_BOOT_PART; //the same ,change to boot1
            return;
        }        

        //copy data form boot2 t0 boot1 
        if(SDM_Write(EMMC_CARD_ID,0,EMMC_BOOT_PART_SIZE,IDBlockData1) != SDM_SUCCESS)
        {
            return;
        }
        //check data
        SDM_Read(EMMC_CARD_ID,0,EMMC_BOOT_PART_SIZE,IDBlockData0);
        if(memcmp(IDBlockData1,IDBlockData0,IDblockDataSize) == 0)
        {
            gEmmcBootPart = EMMC_BOOT_PART; //the same ,change to boot1
            return;
        } 
    }
}

void SdmmcCheckIdBlock(void)
{
    EmmcIdBlockRecover(EMMC_BOOT_PART2);//固件升级前先检查idblock 两份数据都是正确的。
}

#ifdef RK_SD_BOOT
int sdBootCheckSdCard(uint8 ChipSel)
{
    uint16 buf[256];
    uint32 iret;
    if(gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
    {
        iret = SDM_Read(ChipSel,0,1,buf); // read MBR
        if(buf[255] == 0xAA55 && buf[0] == 0x0 && buf[4] == 0x0 && buf[8] == 0x0 && buf[64] == 0x0 && buf[128]) //检查表的合法性
        {
            gSdCardInfoTbl[ChipSel].UserPartOffset = ((uint32)(buf[0xE4]) << 16 ) | buf[0xE3]; 
            gSdCardInfoTbl[ChipSel].UserPartSize = ((uint32)(buf[0xE6]) << 16 ) | buf[0xE5];
            if(gSdCardInfoTbl[ChipSel].UserPartOffset < gSdCardInfoTbl[ChipSel].FwPartOffset)
                return -2;
            gSdCardInfoTbl[ChipSel].FwPartOffset = SD_CARD_FW_PART_OFFSET;
            gSdCardInfoTbl[ChipSel].UserPartOffset -= SD_CARD_FW_PART_OFFSET;//所有读写数据都会加上SD_CARD_FW_PART_OFFSET，这里要减掉
            return 0;
        }
    }
    return -1;
}

uint32 sdBootGetUserPartOffset(uint8 ChipSel)
{
    return gSdCardInfoTbl[ChipSel].UserPartOffset;
}
#endif

#ifdef RK_SDCARD_BOOT_EN
uint32 BootFromSdCard(uint8 ChipSel)
{
    uint32 ret = -1;
    uint32 iret;
    uint32 buf[128];
    if( gSdCardInfoTbl[ChipSel].FwPartOffset == SD_CARD_FW_PART_OFFSET)
    {
        if(0 == gIdDataBuf[128+104/4]) // sd卡升级
        {
            pIDSEC0 pidb0 = (pIDSEC0)&gIdDataBuf[0];
            uint32 *pLoaderAddr=(uint32 *)0x60000000;
            void (*theLoader)(void) = (void (*)(void))pLoaderAddr;
            uint32  LoaderOffset,i,LoaderSize;
            printf("BootFromSdCard 0\n");
            P_RC4((uint8*)&gIdDataBuf[0],512);
            iret = SDM_Read(ChipSel,SD_CARD_BOOT_PART_OFFSET + 4,1,buf); // read MBR
            P_RC4((uint8*)buf,512);
            if(buf[0] == RKGetDDRTag())
            {
                LoaderOffset = pidb0->BootDataSize + 4;
                LoaderSize = pidb0->bootCodeSize;
                if(LoaderSize < (0x80000/0x200)) // 不能把正在运行的代码冲了
                {
                    //printf("LoaderSize = %d\n" , LoaderSize);
                    iret = SDM_Read(ChipSel,SD_CARD_BOOT_PART_OFFSET + LoaderOffset,LoaderSize,pLoaderAddr); // read MBR
                    //printf("Loader loader OK!\n" );
                    if(iret == 0)
                    {   
                        for(i=0;i<LoaderSize;i++)
                        {
                            P_RC4((uint8*)&pLoaderAddr[i*128],512);
                        }
                        MMUDeinit();
                        printf("boot from sd0\n");
                        theLoader();
                    }                    
                }
            }
        }
        else if(1 == gIdDataBuf[128+104/4])// sd 卡运行
        {
            //check sd 卡
            printf("run on sd0\n");
            ret = 0;//返回0，初始化成功
        }
    }
    return ret;
}
#endif

#endif

