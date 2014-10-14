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
****************************************************************/
#include "sdmmc_config.h"

//#define CONFIG_EMMC_PARTITION

#define EMMC_GPP1_SIZE          (0)
#define EMMC_GPP2_SIZE          (0)
#define EMMC_GPP3_SIZE          (0)
#define EMMC_GPP4_SIZE          (0)
#define EMMC_ENH_AREA_SIZE      (8*1024*1024)
#define EMMC_ENH_AREA_ADDR      (0)
#define SDM_SWITCH_ERROR        (0x1 << 23)             //the Device did not switch to the expected mode as requested by the SWITCH command 


#define MMC_Debug 0

#if MMC_Debug
static int MMC_debug = 5;
#define eMMC_printk(n, format, arg...) \
	if (n <= MMC_debug) {	 \
		PRINT_E(format,##arg); \
	}
#else
#define eMMC_printk(n, arg...)
static const int MMC_debug = 0;
#endif



#ifdef DRIVERS_SDMMC

uint32          uncachebuf[128];

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

#ifdef CONFIG_EMMC_PARTITION

/*
Name:       _MMCSwitchCmd
Desc:       
Param:      
Return:     
Global: 
Note:   
Author: 
Log:
*/
static int32 _MMCSwitchCmd(pSDM_CARD_INFO_T pCard, uint32  CmdArg)
{
    int32  ret = SDM_SUCCESS;
    uint32 status = 0;
    
    ret = SDC_SendCommand(pCard->cardId, \
                     (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
                     CmdArg, \
                     &status);

    if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) != 0x0))
    {
        ret = SDM_SWITCH_ERROR;
    }
    
    ret = SDC_SendCommand(pCard->cardId, \
                    (SD_SEND_STATUS | SD_NODATA_OP | SD_RSP_R1 | NO_WAIT_PREV), \
                    (pCard->rca << 16), \
                    &status);
    
    if ((SDC_SUCCESS == ret) && ((status & (0x1 << 7)) != 0x0))
    {
        ret = SDM_SWITCH_ERROR;
    }

    return ret;
}

/*
Name:       _MMCSetPartition
Desc:       
Param:      
Return:     
Global: 
Note:   
Author: 
Log:
*/
static int32 _MMCSetPatition(pSDM_CARD_INFO_T pCard, uint8* pExtCSD)
{   
    int32  ret = SDM_SUCCESS;
    uint8  value, EnhanceAttr;
    uint32 WPG_Sizes, MAX_ENH_SIZE_MULT;
    uint32 i, j;
    uint32 PartSize[] = {EMMC_ENH_AREA_SIZE, EMMC_GPP1_SIZE,EMMC_GPP2_SIZE,EMMC_GPP3_SIZE,EMMC_GPP4_SIZE};
    uint32 PartSizeMult[5]; 

    i = EMMC_ENH_AREA_SIZE + EMMC_GPP1_SIZE + EMMC_GPP2_SIZE + EMMC_GPP3_SIZE + EMMC_GPP4_SIZE;
    /*check need config*/
    if (i == 0)
        return SDM_SUCCESS;
    
    /*Device supports partitioning features and set ENH attribute to partions*/
    if ((pExtCSD[160] & 0x3) != 0x3) 
    {
        return SDM_SUCCESS;//SDM_FUNC_NOT_SUPPORT
    }

    /*check partitioning configuration has completed*/
    if (pExtCSD[155] & 0x1)
    {
        return SDM_SUCCESS;
    }

    /*HC_ERASE_GRP_SIZE [224], HC_WP_GRP_SIZE [221]. 
    If the ENABLE bit in ERASE_GROUP_DEF is set to HIGH:
    Write protect group size = 512KB * HC_ERASE_GRP_SIZE * HC_WP_GRP_SIZE
    */
    WPG_Sizes = (512*1024) * pExtCSD[224] * pExtCSD[221];
    MAX_ENH_SIZE_MULT = (pExtCSD[159]<<16)|(pExtCSD[158]<<8)|pExtCSD[157];

    EnhanceAttr = 0; 
    for (i=0, j=0; i<5; i++)
    {
        PartSizeMult[i] = (PartSize[i] + WPG_Sizes - 1)/WPG_Sizes;
        j += PartSizeMult[i];
        if (PartSize[i])
            EnhanceAttr |= (1<<i);
    }
    if (j > MAX_ENH_SIZE_MULT) //j is TotalSizeMult
    {
        return SDM_PARAM_ERROR;
    }
    
    /*ERASE_GROUP_DEF [175] select high capacity erase unit size Bit defaults to “0” on power on.
     If the partition parameters are sent to a device by CMD6 before setting ERASE_GROUP_DEF bit, 
     the slave shows SWITCH_ERROR.
    */
    if ((pExtCSD[175] & 0x1) == 0)
        return SDM_SWITCH_ERROR;

    /*CMD6 to set: Number and size of general purpose partitions*/
    for (i=0; i<5; i++)
    {
        if (PartSizeMult[i])
        {
            uint8 GP_SIZE_REG[3];
            
            GP_SIZE_REG[0] = 140 + i*3;
            GP_SIZE_REG[1] = 141 + i*3;
            GP_SIZE_REG[2] = 142 + i*3;
            for (j=0; j<3; j++)
            {
                value = (PartSizeMult[i]>>(j*8)) & 0xFF;
                ret = _MMCSwitchCmd(pCard, ((0x3 << 24) | (GP_SIZE_REG[j] << 16) | (value << 8)));
                if (SDC_SUCCESS != ret)
                    return ret;
            }
        }
    }

    /*set Enhanced User Data Area start address*/
    if (PartSizeMult[0])
    {
        for (i=0; i<4; i++)
        {
            value = (EMMC_ENH_AREA_ADDR >>(i*8)) & 0xFF;
            ret = _MMCSwitchCmd(pCard, ((0x3 << 24) | ((136+i) << 16) | (value << 8)));
            if (SDC_SUCCESS != ret)
                return ret;
        }
    }

    /*sets enhanced attributes in general-purpose partitions*/
    ret = _MMCSwitchCmd(pCard, ((0x1 << 24) | (156 << 16) | (EnhanceAttr << 8)));
    if (SDC_SUCCESS != ret)
        return ret;

    /*PARTITIONING_SETTING_COMPLETED (to notify the device that the host has 
    completed partitioning configuration)*/
    value = 1;
    ret = _MMCSwitchCmd(pCard, ((0x1 << 24) | (155 << 16) | (value << 8)));
    if (SDC_SUCCESS != ret)
        return ret;

   /* do Power-cycle*/
   
    while(1);

    return ret;
}
#endif


/****************************************************************/
//函数名:MMC_SwitchFunction
//描述:读取EXT_CSD寄存器，根据卡是否支持宽数据线，改变数据线的宽度
//     以及根据卡是否支持高速模式，切换到高速模式
//参数说明:pCardInfo     输入参数  指向卡信息的指针
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
static void _MMC_SwitchFunction(pSDM_CARD_INFO_T pCard)
{
    HOST_BUS_WIDTH_E wide = BUS_WIDTH_INVALID;
    uint32         *pbuf; //为了使用DMA， 改为使用全局的空间
    uint32           value = 0;
    uint32           status = 0;
    int32            ret = SDM_SUCCESS;
    uint8           *pDataBuf;
    uint32           retry_time = 0;

    if(pCard->specVer < MMC_SPEC_VER_40)
    {
        return;
    }
	
    pbuf = uncachebuf;

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
        //rknand_print_hex("exd", pbuf, 1, 512);
        pCard->bootSize = pDataBuf[226]*256;  // *128K
        value = ((pDataBuf[215] << 24) | (pDataBuf[214] << 16) | (pDataBuf[213] << 8) | pDataBuf[212]);//[215--212]  sector count
        if(pCard->bootSize == 0 && value == 0 && retry_time == 0)
        {// acsip 发现读取exd时读到的数据都是0，重新读一次 
            retry_time++;
            continue;
        }
        if(value)
        {
            pCard->capability = value;
        }
		if(pCard->bootSize == 0)
        {
            pCard->bootSize = 1024;
        }
        
		if(pDataBuf[167] != 0x1F && (pDataBuf[166]&0x5))
		{
            //PRINT_E("WR_REL_SET is %x %x\n",pDataBuf[167],pDataBuf[166]); 
			SDC_SendCommand(pCard->cardId, \
								 (MMC4_SWITCH_FUNC | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), \
								 ((0x3 << 24) | (167<< 16) | (0x1F << 8)), \
								 &status);
		}
        #ifdef CONFIG_EMMC_PARTITION
        /*After every power up, when host uses a device in which partition(s) are configured, it must set the 
        ERASE_GROUP_DEF bit to high before issuing read, write, erase and write protect commands, because 
        this bit is reset after power up.*/
        value = 1; //setting ERASE_GROUP_DEF bit
        ret = _MMCSwitchCmd(pCard, ((0x1 << 24) | (175 << 16) | (value << 8)));
        if (SDC_SUCCESS != ret)
        {
            //PRINT_E("Set ERASE_GROUP_DEF bit ERR\n");
            return;
        }
        pDataBuf[175] |= 0x1;
        #ifdef _USB_PLUG_
        ret = _MMCSetPatition(pCard, pDataBuf);
        if (SDC_SUCCESS != ret)
        {
            //PRINT_E("SetPatition ERR: 0x%x\n", ret);
            //return;
        }
        #endif 
        #endif

        gEmmcBootPart = (pDataBuf[179]>>3)&0x3;
        if(gEmmcBootPart == 0)
        {
            gEmmcBootPart = 1;
        }   
        #ifdef _USB_PLUG_
        PRINT_E("mmc Ext_csd, ret=%x ,\n Ext[226]=%x, bootSize=%x, \n \
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
        break;
    }while(1);
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
int32  MMC_SetBootBusWidth(void *pCardInfo, uint32 enable, HOST_BUS_WIDTH_E width)
{
    pSDM_CARD_INFO_T pCard;
    uint32           status = 0;
    int32            ret = SDM_SUCCESS;
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
int32 MMC_SwitchBoot(void *pCardInfo, uint32 enable, uint32 partition)
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
                //    gSDMDriver[pCard->cardId].step = 0x41;
                //    gSDMDriver[pCard->cardId].error = status;
                    ret = SDM_UNKNOWABLECARD;
                    break;
                }
            }
        }
        else if (SDC_RESP_TIMEOUT == ret)
        {
            // MMC can not perform data transfer in the specified voltage range,
            // so it discard themselves and go into "Inactive State"
            //if (TRUE == SDC_IsCardPresence(pCard->cardId))
            //{
               // gSDMDriver[pCard->cardId].step = 0x42;
               // gSDMDriver[pCard->cardId].error = ret;
                ret = SDM_VOLTAGE_NOT_SUPPORT;
                break;
            /*}
            else
            {
                gSDMDriver[pCard->cardId].step = 0x43;
                gSDMDriver[pCard->cardId].error = ret;
                ret = SDM_CARD_NOTPRESENT;
                break;
            }*/
        }
        else
        {
          //  gSDMDriver[pCard->cardId].step = 0x44;
         //   gSDMDriver[pCard->cardId].error = ret;
            /* error occured */
            break;
        }
    }
    if (ret != SDC_SUCCESS)
    {
       // gSDMDriver[pCard->cardId].step = 0x45;
       // gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    //长时间busy
    if (((FOD_FREQ*1000)/(48+2)) == i)
    {
      //  gSDMDriver[pCard->cardId].step = 0x46;
      //  gSDMDriver[pCard->cardId].error = ret;
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
      //  gSDMDriver[pCard->cardId].step = 0x47;
      //  gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    //decode CID
    _MMC_DecodeCID(longResp, pCard);

    //generate a RCA
    rca = _GenerateRCA();
    ret = _Identify_SendCmd(pCard->cardId, (MMC_SET_RELATIVE_ADDR | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV), (rca << 16), &status, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
      //  gSDMDriver[pCard->cardId].step = 0x48;
      //  gSDMDriver[pCard->cardId].error = ret;
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
      //  gSDMDriver[pCard->cardId].step = 0x49;
     //   gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    //decode CSD
    _MMC_DecodeCSD(longResp, pCard);

    pCard->tran_speed = (pCard->tran_speed > MMC_FPP_FREQ) ? MMC_FPP_FREQ : (pCard->tran_speed);
    ret = SDC_UpdateCardFreq(pCard->cardId, pCard->tran_speed);
    if (SDC_SUCCESS != ret)
    {
     //   gSDMDriver[pCard->cardId].step = 0x4A;
      //  gSDMDriver[pCard->cardId].error = ret;
        return;
    }
    /**************************************************/
    // 让卡进入Transfer State
    /**************************************************/
    ret = _Identify_SendCmd(pCard->cardId, (SD_SELECT_DESELECT_CARD | SD_NODATA_OP | SD_RSP_R1B | WAIT_PREV), (rca << 16), &status, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
     //   gSDMDriver[pCard->cardId].step = 0x4B;
     //   gSDMDriver[pCard->cardId].error = ret;
        return;
    }

    /* 协议规定不管是SD1.X或者SD2.0或者SDHC都必须支持block大小为512, 而且我们一般也只用512，因此这里直接设为512 */
    ret = _Identify_SendCmd(pCard->cardId, (SD_SET_BLOCKLEN | SD_NODATA_OP | SD_RSP_R1 | WAIT_PREV), 512, &status, 0, 0, NULL);
    if (SDC_SUCCESS != ret)
    {
     //   gSDMDriver[pCard->cardId].step = 0x4C;
     //   gSDMDriver[pCard->cardId].error = ret;
        return;
    }

    eMMC_printk(5, "%s  %d    MMC Init over, then begin to run SwitchFunction \n",__FILE__,__LINE__);

    pCard->WriteProt = FALSE;  //MMC卡都没有写保护
    //卡输入开启密码在这里做
    /*if (status & CARD_IS_LOCKED)
    {
        pCard->bPassword = TRUE;
        PRINT_E("CARD_IS_LOCKED %x",ret); 
    }
    else*/
    {
        pCard->bPassword = FALSE;
        _MMC_SwitchFunction(pCard);
    }
    
    pCard->type |= type;
    return;
}

#endif //end of #ifdef DRIVERS_SDMMC
