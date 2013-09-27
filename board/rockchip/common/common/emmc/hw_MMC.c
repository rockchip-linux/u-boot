/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_MMC.c
//描述:MMC protocol implement
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_MMC.c,v $
Revision 1.3  2011/03/30 02:33:29  Administrator
1、支持三星8GB EMMC
2、支持emmc boot 1和boot 2都写入IDBLOCK数据

Revision 1.2  2011/01/21 10:12:56  Administrator
支持EMMC
优化buffer效率

Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:02  Administrator
*** empty log message ***

Revision 1.1.1.1  2010/05/17 03:44:52  hds
20100517 黄德胜提交初始版本

Revision 1.1.1.1  2010/03/06 05:22:59  zjd
2010.3.6由黄德胜提交初始版本

Revision 1.1.1.1  2009/12/15 01:46:31  zjd
20091215 杨永忠提交初始版本

Revision 1.2  2009/10/13 06:30:47  hcy
hcy 09-10-13 SD卡驱动更新，现有3种卡检测方式，优化了代码提高性能

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

Revision 1.2  2009/04/02 03:17:16  hcy
增加卡处理的timeout操作

Revision 1.1.1.1  2009/03/16 01:34:07  zjd
20090316 邓训金提供初始SDK版本

Revision 1.3  2009/03/11 03:33:38  hcy
驱动调整

Revision 1.2  2009/03/05 12:37:15  hxy
添加CVS版本自动注释

****************************************************************/
#include "sdmmc_config.h"

#define MMC_Debug 0

#if MMC_Debug
static int MMC_debug = 5;
#define eMMC_printk(n, format, arg...) \
	if (n <= MMC_debug) {	 \
		printf(format,##arg); \
	}
#else
#define eMMC_printk(n, arg...)
static const int MMC_debug = 0;
#endif



#ifdef DRIVERS_SDMMC

#if (eMMC_PROJECT_LINUX) 
static unsigned long dma_mem_alloc(int size)
{
	int order = get_order(size);
	return __get_dma_pages(GFP_KERNEL, order);
}
#else
uint32          uncachebuf[128];
#endif
/*******************************
厂商ID表
********************************/
__align(4) 
uint8 EMMC_MIDTbl[]=
{
    0x15,					    //三星SAMSUNG
    0x11,					    //东芝TOSHIBA
    0x90,					    //海力士HYNIX
    0xff,					    //英飞凌INFINEON
    0x13,					    //美光MICRON
    0xff,					    //瑞萨RENESAS
    0xff,					    //意法半导体ST
    0xff,					    //英特尔intel
    0x45,                       //SanDisk
    0x70,                       //Kingston
    0xfe                        //恒忆 Numonyx
};

static uint8 _MMC_MID = 0;

uint8 MMC_GetMID(void)
{
    uint8 i;

    if ((_MMC_MID == 0) ||(_MMC_MID == 0xff))
        return 0xff;
    
    for (i=0; i<sizeof(EMMC_MIDTbl); i++)
    {
        if (_MMC_MID == EMMC_MIDTbl[i])
        {
            return i;
        }
    }
    return 0xff;
}

/****************************************************************/
//函数名:MMC_DecodeCID
//描述:解析读到的CID寄存器，获取需要的信息
//参数说明:pCID           输入参数  指向存放CID信息的指针
//         pCardInfo      输入参数  指向存放卡信息的指针
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
static void _MMC_DecodeCID(uint32 *pCID, pSDM_CARD_INFO_T pCard)
{
    pCard->year   = (uint16)(1997 + ((pCID[0] >> 12) & 0xF));   //[15:12]
    pCard->month  = (uint8)((pCID[0] >> 8) & 0xF);               //[11:8]
    pCard->psn    = ((pCID[1] & 0x0000FFFF) << 16) | ((pCID[0] & 0xFFFF0000) >> 16);  //[47:16]
    pCard->prv    = (uint8)((pCID[1] >> 16) & 0xFF);             //[55:48]
    pCard->pnm[0] = (uint8)(pCID[3] & 0xFF);                     //[103:56]
    pCard->pnm[1] = (uint8)((pCID[2] >> 24) & 0xFF);
    pCard->pnm[2] = (uint8)((pCID[2] >> 16) & 0xFF);
    pCard->pnm[3] = (uint8)((pCID[2] >> 8) & 0xFF);
    pCard->pnm[4] = (uint8)(pCID[2] & 0xFF);
    pCard->pnm[5] = (uint8)((pCID[1] >> 24) & 0xFF);
    pCard->pnm[6] = 0x0; //字符串结束符
    pCard->oid[0] = (uint8)((pCID[3] >> 16) & 0xFF);             //[119:104]
    pCard->oid[1] = (uint8)((pCID[3] >> 8) & 0xFF);
    pCard->oid[2] = 0x0; //字符串结束符
    _MMC_MID = pCard->mid    = (uint8)((pCID[3] >> 24) & 0xFF);             //[127:120]//samsung 0x15

    return;
}

/****************************************************************/
//函数名:MMC_DecodeCSD
//描述:解析读到的CSD寄存器，获取需要的信息
//参数说明:pCSD           输入参数  指向存放CID信息的指针
//         pCardInfo      输入参数  指向存放卡信息的指针
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
static void _MMC_DecodeCSD(uint32 *pCSD, pSDM_CARD_INFO_T pCard)
{
    uint32           c_size = 0;
    uint32           c_size_mult = 0;
    uint32           read_bl_len = 0;
    uint32           transfer_rate_unit[4] = {10, 100, 1000, 10000};
    uint32           time_value[16] = {10/*reserved*/, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};

    c_size      = (pCSD[1] >> 30) | ((pCSD[2] & 0x3FF) << 2);    //[73:62]
    c_size_mult = (pCSD[1] >> 15) & 0x7;                         //[49:47]
    read_bl_len = (pCSD[2] >> 16) & 0xF;                         //[83:80]

    pCard->specVer    = MMC_SPEC_VER_10 + ((pCSD[3] >> 26) & 0xF); //[125:122]
    pCard->capability = (((c_size + 1)*(0x1 << (c_size_mult + 2))*(0x1 << read_bl_len)) >> 9);
    pCard->taac       = (pCSD[3] >> 16) & 0xFF;                  //[119:112]
    pCard->nsac       = (pCSD[3] >> 8) & 0xFF;                   //[111:104]
    pCard->tran_speed = transfer_rate_unit[pCSD[3] & 0x3]*time_value[(pCSD[3] >> 3) & 0x7]; //[103:96]
    pCard->r2w_factor = (pCSD[0] >> 26) & 0x7;                   //[28:26]
    pCard->dsr_imp    = (pCSD[2] >> 12) & 0x1;                   //[76]
    pCard->ccc        = (uint16)((pCSD[2] >> 20) & 0xFFF);       //[95:84]
    return;
}

/****************************************************************/
//函数名:MMC_SwitchFunction
//描述:读取EXT_CSD寄存器，根据卡是否支持宽数据线，改变数据线的宽度
//     以及根据卡是否支持高速模式，切换到高速模式
//参数说明:pCardInfo     输入参数  指向卡信息的指针
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
#if(eMMC_PROJECT_LINUX == 0)
#ifdef RK_SD_BOOT
static int32 _SetBootSize(int32 cardId, uint32 boot_size)
{
    int32            ret = SDM_FALSE;
    uint32           status = 0;

	#if 0
    if (!_IsCardRegistered(cardId, &port))
    {
        return SDM_PARAM_ERROR;
    }
	#endif    
    ret = SDC_SendCommand(cardId, (SD_CMD62 | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), 0xEFAC62EC, &status);
    if (SDC_SUCCESS != ret)
    {
        return ret;
    }

	ret = SDC_SendCommand(cardId, (SD_CMD62 | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), 0x00CBAEA7, &status);
    if (SDC_SUCCESS != ret)
    {
        return ret;
    }
	
    ret = SDC_SendCommand(cardId, (SD_CMD62 | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), boot_size, &status);
    if (SDC_SUCCESS != ret)
    {
        return ret;
    }

	return SDC_WaitCardBusy(cardId);

}
#endif
#endif
static void _MMC_SwitchFunction(pSDM_CARD_INFO_T pCard)
{
    HOST_BUS_WIDTH_E wide = BUS_WIDTH_INVALID;
    uint32         *pbuf; //为了使用DMA， 改为使用全局的空间
    uint32           value = 0;
    uint32           status = 0;
    int32            ret = SDM_SUCCESS;
    uint8           *pDataBuf;
//    int testpos;

    if(pCard->specVer < MMC_SPEC_VER_40)
    {
        return;
    }
	
#if (eMMC_PROJECT_LINUX) 
    pbuf = (uint32 *)dma_mem_alloc(1024);
    if(pbuf == NULL)
    {
        return;
    }
#else
    pbuf = uncachebuf;
#endif
	
    //printk("%s, %s  %d    \n",__FUNCTION__, __FILE__,__LINE__);

    do
    {
        ret = SDC_BusRequest(pCard->cardId, 
                             (MMC4_SEND_EXT_CSD | SD_READ_OP | SD_RSP_R1 | WAIT_PREV), 
                             0, 
                             &status, 
                             512, 
                             512, 
                             pbuf);
                             
        if (SDC_SUCCESS != ret)
        {
            break;
        }
       

        pDataBuf = (uint8 *)pbuf;  
        //mdelay(1000);
       // printk("\n%s  %d   After Ext_csd, ret=%x  \n",__FILE__,__LINE__, ret);
  /*      for(testpos=0;testpos<32;testpos++)
        {                
            printk( "[%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x,\n[%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x, [%3d]=%2x,\n", \
                     testpos*16+0, pDataBuf[testpos*16+0], testpos*16+1,pDataBuf[testpos*16+1], testpos*16+2,pDataBuf[testpos*16+2], testpos*16+3,pDataBuf[testpos*16+3],\
                     testpos*16+4, pDataBuf[testpos*16+4], testpos*16+5,pDataBuf[testpos*16+5], testpos*16+6,pDataBuf[testpos*16+6], testpos*16+7,pDataBuf[testpos*16+7],\
                     testpos*16+8, pDataBuf[testpos*16+8], testpos*16+9,pDataBuf[testpos*16+9], testpos*16+10,pDataBuf[testpos*16+10], testpos*16+11,pDataBuf[testpos*16+11],\
                     testpos*16+12, pDataBuf[testpos*16+12], testpos*16+13,pDataBuf[testpos*16+13], testpos*16+14,pDataBuf[testpos*16+14], testpos*16+15,pDataBuf[testpos*16+15]);
        }
        
*/
        pCard->bootSize = pDataBuf[226]*256;  // *128K
        // printk("\n%s  %d   After Ext_csd,2222222222222222 ret=%x , pCard_addr=%x \n",__FILE__,__LINE__, ret, pCard);
#ifdef RK_SD_BOOT
		if(pCard->bootSize == 0)
        {
            _SetBootSize(pCard->cardId, 1);
            pCard->bootSize = 1024;
        }
#endif
        value = ((pDataBuf[215] << 24) | (pDataBuf[214] << 16) | (pDataBuf[213] << 8) | pDataBuf[212]);//[215--212]  sector count
        if(value)
        {
            pCard->capability = value;
        }
        gEmmcBootPart = (pDataBuf[179]>>3)&0x3;
        if(gEmmcBootPart == 0)
        {
            gEmmcBootPart = 1;
        }   
		
		#ifdef RK_SD_BOOT
        printf("mmc Ext_csd, ret=%x ,\n Ext[226]=%x, bootSize=%x, \n \
                Ext[215]=%x, Ext[214]=%x, Ext[213]=%x, Ext[212]=%x,cap =%x \n",\
                ret, pDataBuf[226], pCard->bootSize, \
                pDataBuf[215],pDataBuf[214],pDataBuf[213],pDataBuf[212], value);
        #endif
        
        //printk("**************************+++++++++++++++++++++\n");
        
        if (pDataBuf[196] & 0x3) //支持高速模式
        {
            ret = SDC_SendCommand(pCard->cardId, \
                                 (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                                 ((0x3 << 24) | (185 << 16) | (0x1 << 8)), \
                                 &status);
            if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) == 0x0))
            {                
               // printk("%s, %s  %d    \n",__FUNCTION__, __FILE__,__LINE__);
                ret = SDC_SendCommand(pCard->cardId, (SD_SEND_STATUS | SD_NODATA_OP | SD_RSP_R1 | NO_WAIT_PREV), (pCard->rca << 16), &status);
                if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) == 0x0))
                {
                    if (pDataBuf[196] & 0x2) // 52M
                    {
                        ret = SDC_UpdateCardFreq(pCard->cardId, MMCHS_52_FPP_FREQ);
                        if (SDC_SUCCESS == ret)
                        {
                            pCard->tran_speed = MMCHS_52_FPP_FREQ;
                            pCard->workMode |= SDM_HIGH_SPEED_MODE;
                        }
                    }
                    else  // 26M
                    {
                        ret = SDC_UpdateCardFreq(pCard->cardId, MMCHS_26_FPP_FREQ);
                        if (SDC_SUCCESS == ret)
                        {
                            pCard->tran_speed = MMCHS_26_FPP_FREQ;
                            pCard->workMode |= SDM_HIGH_SPEED_MODE;
                        }
                    }
                }
            }
        }

        
        eMMC_printk(5, "%s...%d...Begin to get the bus_width.\n", __FUNCTION__, __LINE__);
        //切换高速模式有不成功不直接return，线宽的切换可以继续
        //切换线宽放在高速模式切换之后做，这样可以顺便检查一下在高速模式下用较宽的数据线会不会出错
        ret = SDC_GetHostBusWidth(pCard->cardId, &wide);
        if (SDC_SUCCESS != ret)
        {
            break;
        }
        
        //printk("emmc buswidth=%d\n",wide);
        
        eMMC_printk(5, "%s...%d...Begin to Set the bus_width.\n", __FUNCTION__, __LINE__);
        Assert((wide != BUS_WIDTH_INVALID), "MMC_SwitchFunction:Host bus width error\n", wide);
        if((wide == BUS_WIDTH_INVALID) || (wide == BUS_WIDTH_MAX))
        {
            ret = SDC_SDC_ERROR;
            break;
        }

        if (wide == BUS_WIDTH_8_BIT)
        {
            value = 0x2;
            ret = SDC_SetHostBusWidth(pCard->cardId, wide);
            // printk("%s, %d=====buswidth=%d ", __FUNCTION__, __LINE__, wide);
            if (SDC_SUCCESS != ret)
            {
                break;
            }
            //下面两个命令都不要检查返回值是否成功，因为它们的CRC会错
            pDataBuf[0] = 0x55;
            pDataBuf[1] = 0xAA;
            pDataBuf[2] = 0x55;
            pDataBuf[3] = 0xAA;
            pDataBuf[4] = 0x55;
            pDataBuf[5] = 0xAA;
            pDataBuf[6] = 0x55;
            pDataBuf[7] = 0xAA;
            ret = SDC_BusRequest(pCard->cardId, \
                                (MMC4_BUSTEST_W | SD_WRITE_OP | SD_RSP_R1 | WAIT_PREV), \
                                0, \
                                &status, \
                                8, \
                                8, \
                                pbuf);
            pDataBuf[0] = 0x00;
            pDataBuf[1] = 0x00;
            pDataBuf[2] = 0x00;
            pDataBuf[3] = 0x00;
            pDataBuf[4] = 0x00;
            pDataBuf[5] = 0x00;
            pDataBuf[6] = 0x00;
            pDataBuf[7] = 0x00;
            ret = SDC_BusRequest(pCard->cardId, \
                                (MMC4_BUSTEST_R | SD_READ_OP | SD_RSP_R1 | WAIT_PREV), \
                                0, \
                                &status, \
                                4, \
                                4, \
                                pbuf);
                                
            //printk("wide = 8 pDataBuf=%x %x %x %x",pDataBuf[0],pDataBuf[1],pDataBuf[2],pDataBuf[3]);

            if ((pDataBuf[0] != 0xAA)
            || (pDataBuf[1] != 0x55))
            {
                //SDC_SetHostBusWidth(pCard->cardId, BUS_WIDTH_1_BIT);
                //break;
                wide = BUS_WIDTH_4_BIT;
            }
        }
        
        if (wide == BUS_WIDTH_4_BIT)
        {
            value = 0x1;
            ret = SDC_SetHostBusWidth(pCard->cardId, wide);
            //printk("%s, %d=====buswidth=%d ", __FUNCTION__, __LINE__, wide);
            if (SDC_SUCCESS != ret)
            {
                break;
            }
            //下面两个命令都不要检查返回值是否成功，因为它们的CRC会错
            pDataBuf[0] = 0x5A;
            pDataBuf[1] = 0x5A;
            pDataBuf[2] = 0x5A;
            pDataBuf[3] = 0x5A;
            ret = SDC_BusRequest(pCard->cardId, \
                                 (MMC4_BUSTEST_W | SD_WRITE_OP | SD_RSP_R1 | WAIT_PREV), \
                                 0, \
                                 &status, \
                                 4, \
                                 4, \
                                 pbuf);
            pDataBuf[0] = 0x00;
            pDataBuf[1] = 0x00;
            pDataBuf[2] = 0x00;
            pDataBuf[3] = 0x00;
            ret = SDC_BusRequest(pCard->cardId, \
                                 (MMC4_BUSTEST_R | SD_READ_OP | SD_RSP_R1 | WAIT_PREV), \
                                 0, \
                                 &status, \
                                 4, \
                                 4, \
                                 pbuf);
            //printk("wide = 4 pDataBuf=%x %x %x %x",pDataBuf[0],pDataBuf[1],pDataBuf[2],pDataBuf[3]);
            if (pDataBuf[0] != 0xA5)
            {
                SDC_SetHostBusWidth(pCard->cardId, BUS_WIDTH_1_BIT);
                break;
            }
        }
        else if(wide == BUS_WIDTH_1_BIT)
        {
            break;
        }
        
        ret = SDC_SendCommand(pCard->cardId, \
                             (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                             ((0x3 << 24) | (183 << 16) | (value << 8)), \
                             &status);
        if ((SDC_SUCCESS != ret) || (status & (0x1 << 7)))
        {
            break;
        }
        ret = SDC_SendCommand(pCard->cardId, (SD_SEND_STATUS | SD_NODATA_OP | SD_RSP_R1 | NO_WAIT_PREV), (pCard->rca << 16), &status);
        if ((SDC_SUCCESS != ret) || (status & (0x1 << 7)))
        {
            break;
        }
        pCard->workMode |= SDM_WIDE_BUS_MODE;
    }while(0);

    //SDOAM_Free(pbuf);
    
    return;
}




/****************************************************************/
//函数名:MMC_AccessBootPartition
//描述:Access boot partition or user area
//参数说明:pCard 输入参数  卡信息的指针
//         enable     输入参数  使能
//         partition  输入参数  具体操作哪个boot partition
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
int32  MMC_AccessBootPartition(void *pCardInfo, uint32 partition)
{
    pSDM_CARD_INFO_T pCard;
    uint32           status = 0;
    int32            ret = SDM_SUCCESS;
//    uint8            tmp;
    uint8            value=0;

    if (partition > 7)
    {
        return SDM_PARAM_ERROR;
    }

    value = (gEmmcBootPart << 3)|partition;
    
    pCard = (pSDM_CARD_INFO_T)pCardInfo;
    ret = SDC_SendCommand(pCard->cardId, \
                         (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                         ((0x3 << 24) | (179 << 16) | (value << 8)), \
                         &status);
    if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) != 0x0))
    {
        ret = SDM_FALSE;
    }
#ifdef RK_SD_BOOT
    SDC_SendCommand(pCard->cardId, \
                         (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                         ((0x3 << 24) | (162 << 16) | (1 << 8)), \
                         &status);
#endif
    return ret;
}




/****************************************************************/
//函数名:MMC_AccessBootPartition
//描述:Access boot partition or user area
//参数说明:pCard 输入参数  卡信息的指针
//         enable     输入参数  使能
//         partition  输入参数  具体操作哪个boot partition
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
int32  MMC_SetBootBusWidth(void *pCardInfo, bool enable, HOST_BUS_WIDTH_E width)
{
    pSDM_CARD_INFO_T pCard;
    uint32           status = 0;
    int32            ret = SDM_SUCCESS;
//    uint8            tmp;
    uint8            value;

    if(enable)
    {
        value = 0x4;  //Retain boot bus width after boot operation
    }
    else
    {
        value = 0;   //Reset bus width to x1 after boot operation (default)
    }

    switch (width)
    {
        case BUS_WIDTH_1_BIT:
            //value = (value | 0x0);
            break;
        case BUS_WIDTH_4_BIT:
            value = (value | 0x1);
            break;
        case BUS_WIDTH_8_BIT: 
            value = (value | 0x2);
            break;
        default:
            break;
    }
       
    pCard = (pSDM_CARD_INFO_T)pCardInfo;
    ret = SDC_SendCommand(pCard->cardId, \
                         (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                         ((0x3 << 24) | (177 << 16) | (value << 8)), \
                         &status);
    if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) != 0x0))
    {
        ret = SDM_FALSE;
    }
    return ret;
}




/****************************************************************/
//函数名:MMC_SwitchBoot
//描述:切换boot partition或者user area
//参数说明:pCard 输入参数  卡信息的指针
//         enable     输入参数  使能
//         partition  输入参数  具体操作哪个boot partition
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
int32 MMC_SwitchBoot(void *pCardInfo, bool enable, uint32 partition)
{
    pSDM_CARD_INFO_T pCard;
    uint32           status = 0;
    int32            ret = SDM_SUCCESS;
    uint8            tmp;
    uint8            value=0;

    if (partition > 7)
    {
        return SDM_PARAM_ERROR;
    }
    tmp = partition;
    if(enable)
    {
        value = tmp | (tmp << 3);
    }
    else
    {
        value = (gEmmcBootPart << 3)|tmp;
    }

    pCard = (pSDM_CARD_INFO_T)pCardInfo;
    ret = SDC_SendCommand(pCard->cardId, \
                         (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                         ((0x3 << 24) | (179 << 16) | (value << 8)), \
                         &status);
    if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) != 0x0))
    {
        ret = SDM_FALSE;
    }
    return ret;
}
/****************************************************************/
//函数名:MMC_Init
//描述:MMC卡的初始化
//参数说明:pCardInfo 输入参数  卡信息的指针
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
void   MMC_Init(void *pCardInfo)
{
    pSDM_CARD_INFO_T pCard;
    uint32           type = UNKNOW_CARD;
    uint32           longResp[4];
    uint32           status = 0;
    uint16           rca = 0;
    uint32           i = 0;
    int32            ret = SDC_SUCCESS;
    
    pCard = (pSDM_CARD_INFO_T)pCardInfo;
    /**************************************************/
    // 让卡进入Ready State
    /**************************************************/
    for (i=0; i<((FOD_FREQ*1000)/(48+2)); i++)
    {
        ret = _Identify_SendCmd(pCard->cardId, (MMC_SEND_OP_COND | SD_NODATA_OP | SD_RSP_R3 | WAIT_PREV), 0x40ff8000, &status, 0, 0, NULL);
        if (SDC_SUCCESS == ret)
        {
            if (status & 0x80000000)
            {
                if ((0x80ff8000 == status) || (0x80ff8080 == status))
                {
                    type = MMC;
                    break;
                }
                else if((0xc0ff8000 == status) || (0xc0ff8080 == status))
                {
                    type = eMMC2G;
                    break;
                }
                else
                {
                    gSDMDriver[pCard->cardId].step = 0x41;
                    gSDMDriver[pCard->cardId].error = status;
                    ret = SDM_UNKNOWABLECARD;
                    break;
                }
            }
        }
        else if (SDC_RESP_TIMEOUT == ret)
        {
            // MMC can not perform data transfer in the specified voltage range,
            // so it discard themselves and go into "Inactive State"
            if (TRUE == SDC_IsCardPresence(pCard->cardId))
            {
                gSDMDriver[pCard->cardId].step = 0x42;
                gSDMDriver[pCard->cardId].error = ret;
                ret = SDM_VOLTAGE_NOT_SUPPORT;
                break;
            }
            else
            {
                gSDMDriver[pCard->cardId].step = 0x43;
                gSDMDriver[pCard->cardId].error = ret;
                ret = SDM_CARD_NOTPRESENT;
                break;
            }
        }
        else
        {
            gSDMDriver[pCard->cardId].step = 0x44;
            gSDMDriver[pCard->cardId].error = ret;
            /* error occured */
            break;
        }
    }
    if (ret != SDC_SUCCESS)
    {
        gSDMDriver[pCard->cardId].step = 0x45;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    //长时间busy
    if (((FOD_FREQ*1000)/(48+2)) == i)
    {
        gSDMDriver[pCard->cardId].step = 0x46;
        gSDMDriver[pCard->cardId].error = ret;
        ret = SDM_VOLTAGE_NOT_SUPPORT;
        return;
    }
    /**************************************************/
    // 让卡进入Stand-by State
    /**************************************************/
    longResp[0] = 0;
    longResp[1] = 0;
    longResp[2] = 0;
    longResp[3] = 0;
    ret = _Identify_SendCmd(pCard->cardId, (SD_ALL_SEND_CID | SD_NODATA_OP | SD_RSP_R2 | WAIT_PREV), 0, longResp, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
        gSDMDriver[pCard->cardId].step = 0x47;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    //decode CID
    _MMC_DecodeCID(longResp, pCard);

    //generate a RCA
    rca = _GenerateRCA();
    ret = _Identify_SendCmd(pCard->cardId, (MMC_SET_RELATIVE_ADDR | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV), (rca << 16), &status, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
        gSDMDriver[pCard->cardId].step = 0x48;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    pCard->rca = rca;

    longResp[0] = 0;
    longResp[1] = 0;
    longResp[2] = 0;
    longResp[3] = 0;
    ret = _Identify_SendCmd(pCard->cardId, (SD_SEND_CSD | SD_NODATA_OP | SD_RSP_R2 | WAIT_PREV), (rca << 16), longResp, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
        gSDMDriver[pCard->cardId].step = 0x49;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    //decode CSD
    _MMC_DecodeCSD(longResp, pCard);

    pCard->tran_speed = (pCard->tran_speed > MMC_FPP_FREQ) ? MMC_FPP_FREQ : (pCard->tran_speed);
    ret = SDC_UpdateCardFreq(pCard->cardId, pCard->tran_speed);
    if (SDC_SUCCESS != ret)
    {
        gSDMDriver[pCard->cardId].step = 0x4A;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    /**************************************************/
    // 让卡进入Transfer State
    /**************************************************/
    ret = _Identify_SendCmd(pCard->cardId, (SD_SELECT_DESELECT_CARD | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), (rca << 16), &status, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
        gSDMDriver[pCard->cardId].step = 0x4B;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }

    /* 协议规定不管是SD1.X或者SD2.0或者SDHC都必须支持block大小为512, 而且我们一般也只用512，因此这里直接设为512 */
    ret = _Identify_SendCmd(pCard->cardId, (SD_SET_BLOCKLEN | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV), 512, &status, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
        gSDMDriver[pCard->cardId].step = 0x4C;
        gSDMDriver[pCard->cardId].error = ret;
        return;
    }

    eMMC_printk(5, "%s  %d    MMC Init over, then begin to run SwitchFunction \n",__FILE__,__LINE__);

    pCard->WriteProt = FALSE;  //MMC卡都没有写保护
    //卡输入开启密码在这里做
    if (status & CARD_IS_LOCKED)
    {
        pCard->bPassword = TRUE;
    }
    else
    {
        pCard->bPassword = FALSE;
        _MMC_SwitchFunction(pCard);
    }
    
    pCard->type |= type;
    return;
}

#endif //end of #ifdef DRIVERS_SDMMC
