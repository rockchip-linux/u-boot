/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_SDPlatAdapt.c
//描述:RK28 SD/MMC driver Platform adaptation implement file
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_SDPlatAdapt.c,v $
Revision 1.5  2011/04/01 08:32:04  Administrator
解决emmc 初始化多次问题

Revision 1.4  2011/03/29 09:24:55  Administrator
*** empty log message ***

Revision 1.3  2011/03/08 08:37:28  Administrator
本地升级支持
解决emmc 初始化多次问题
解决delay延时不准的问题

Revision 1.2  2011/01/26 09:37:39  Administrator
*** empty log message ***

Revision 1.1  2011/01/18 07:20:31  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 11:55:40  Administrator
*** empty log message ***

Revision 1.1.1.1  2010/05/17 03:44:52  hds
20100517 黄德胜提交初始版本

Revision 1.1.1.1  2010/03/06 05:22:59  zjd
2010.3.6由黄德胜提交初始版本

Revision 1.1.1.1  2009/12/15 01:46:31  zjd
20091215 杨永忠提交初始版本

Revision 1.2  2009/10/31 02:21:17  hcy
hcy 09-10-31 解决用控制器做卡检测时，不支持热拔插

Revision 1.6  2009/10/13 08:08:28  hcy
hcy 09-10-13 SD卡驱动更新，改成3种卡检测方式，优化代码提高性能

Revision 1.2  2009/08/18 09:42:06  YYZ
no message

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

Revision 1.4  2009/05/07 12:12:07  hcy
hcy 使用小卡座，卡热拔插，很容易出现读不到文件的问题

Revision 1.3  2009/04/29 10:35:48  fzf
去除 SD POWER IO 的设置！

Revision 1.2  2009/04/02 03:18:08  hcy
卡检测和电源宏改到Hw_define.h中

Revision 1.1.1.1  2009/03/16 01:34:07  zjd
20090316 邓训金提供初始SDK版本

Revision 1.4  2009/03/13 01:44:44  hcy
卡检测和卡电源管理改成GPIO方式

Revision 1.3  2009/03/07 07:30:18  yk
(yk)更新SCU模块各频率设置，完成所有函数及代码，更新初始化设置，
更新遥控器代码，删除FPGA_BOARD宏。
(hcy)SDRAM驱动改成28的

Revision 1.2  2009/03/05 12:37:16  hxy
添加CVS版本自动注释

****************************************************************/
#include    "sdmmc_config.h"

#if (eMMC_PROJECT_LINUX) 
#include <linux/dma-mapping.h>
#include <asm/dma.h>
#endif

#ifdef DRIVERS_SDMMC

/****************************************************************/
//函数名:SDPAM_FlushCache
//描述:清除cache
//参数说明:adr      输入参数     需要清除的起始地址
//         size     输入参数     需要清除的大小，单位字节
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
void   SDPAM_FlushCache(void *adr, uint32 size)
{
    //MMFlushCache(BOTHCACHE, CACREGION, adr, size);
	flush_cache(adr,size);
}

/****************************************************************/
//函数名:SDPAM_CleanCache
//描述:清理cache
//参数说明:adr      输入参数     需要清理的起始地址
//         size     输入参数     需要清理的大小，单位字节
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
void   SDPAM_CleanCache(void *adr, uint32 size)
{
    //MMCleanDCache(CACREGION, adr, size);
}

/****************************************************************/
//函数名:SDPAM_GetAHBFreq
//描述:得到当前AHB总线频率
//参数说明:
//返回值:返回当前AHB总线频率，单位KHz
//相关全局变量:
//注意:
/****************************************************************/
uint32 SDPAM_GetAHBFreq(void)
{
#if SDMMC_NO_PLATFORM
    return 25000;
#else
    return GetMmcCLK();//PLLGetAHBFreq();
#endif
}

/****************************************************************/
//函数名:SDPAM_SDCClkEnable
//描述:选择是否开启SDMMC控制器的工作时钟
//参数说明:nSDCPort   输入参数   端口号
//         enable     输入参数   是否使能
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
void SDPAM_SDCClkEnable(SDMMC_PORT_E nSDCPort, bool enable)
{
/*    eCLK_GATE gate;

    //gate = (nSDCPort == SDC0) ? CLK_GATE_SDMMC0 : CLK_GATE_SDMMC1;
    if(nSDCPort == SDC0)
    {
        gate = CLK_GATE_SDMMC0;
    }
    else if(nSDCPort == SDC1)
    {
        gate = CLK_GATE_SDMMC1;
    }
    else
    {
        gate = CLK_GATE_EMMC; //eMMC controller
    }
    
    if(enable)
    {
        #if ((SDMMC0_DET_MODE == SD_GPIO_DET) || (SDMMC0_DET_MODE == SD_ALWAYS_PRESENT) )  ///eMMC 怎么处理呢?
        SCUEnableClk(gate);
        #endif
    }
    else
    {
        #if ((SDMMC0_DET_MODE == SD_GPIO_DET) || (SDMMC0_DET_MODE == SD_ALWAYS_PRESENT))
        SCUDisableClk(gate);
        #endif
    }
*/
}

/****************************************************************/
//函数名:SDPAM_SDCReset
//描述:从SCU上复位SDMMC控制器
//参数说明:nSDCPort   输入参数   端口号
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
void SDPAM_SDCReset(SDMMC_PORT_E nSDCPort)
{
    if(nSDCPort == SDC0)
    {
        SDCReset(0);
    }
    else if (nSDCPort == SDC1)
    {
        SDCReset(1);
    }
    else
    {
        SDCReset(2);
    }
}

/****************************************************************/
//函数名:SDPAM_SetMmcClkDiv
//描述:设置SCU上mmc_clk_div的分频值
//参数说明:nSDCPort   输入参数   端口号
//         div        输入参数   分频值
//返回值:返回当前AHB总线频率，单位KHz
//相关全局变量:
//注意:
/****************************************************************/
void   SDPAM_SetMmcClkDiv(SDMMC_PORT_E nSDCPort, uint32 div)
{
#if SDMMC_NO_PLATFORM
    return;
#else
    if(nSDCPort == SDC0)
    {
        SCUSelSDClk(0, div);
    }
    else if (nSDCPort == SDC1)
    {
        SCUSelSDClk(1, div);
    }
    else
    {
        SCUSelSDClk(2, div);
    }
#endif
}
#if EN_SD_DMA
/****************************************************************/
//函数名:SDPAM_DMAStart
//描述:配置一个DMA传输
//参数说明:nSDCPort   输入参数   需要数据传输的端口号
//         dstAddr    输入参数   目标地址
//         srcAddr    输入参数   源地址
//         size       输入参数   数据长度，单位字节
//         rw         输入参数   表示数据是要从卡读出还是写到卡，1:写到卡，0:从卡读出
//         CallBack   输入参数   DMA传输完的回调函数
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_DMAStart(SDMMC_PORT_E nSDCPort, uint32 dstAddr, uint32 srcAddr, uint32 size, bool rw, pFunc CallBack)
{
#if SDMMC_NO_PLATFORM
    return TRUE;
#else

#if eMMC_PROJECT_LINUX
    uint32 mode;
    int ret;
    
    uint8 *buf1;

    if(nSDCPort == SDC0)
    {
        if(rw)
        {
            mode = RK29_DMASRC_MEM;
        }
        else
        {
            mode = RK29_DMASRC_HW;
        }
    }
    else if(nSDCPort == SDC1)
    {
        if(rw)
        {
            mode = RK29_DMASRC_MEM;
        }
        else
        {
            mode = RK29_DMASRC_HW;
        }
    }
    else
    {
        if(rw)
        {
            mode = RK29_DMASRC_MEM;
        }
        else
        {
            mode = RK29_DMASRC_HW;
        }
    }

    eMMC_printk(5, "%s..%d   Before call dma_map_single.......\n",__FUNCTION__, __LINE__);
    if(rw)
    {
        eMMC_host->dmabuf= dma_map_single(NULL, (void*)srcAddr,size<<2, DMA_TO_DEVICE);
        if( eMMC_host->dmabuf <= 0)
        {
            printk("%s..%d   run dma_map_single fail!!!!\n",__FUNCTION__, __LINE__);
            BUG_ON(1);
            return FALSE;
        }
        
    }
    else
    {
         eMMC_host->dmabuf = dma_map_single(NULL, (void*)dstAddr, size<<2, DMA_FROM_DEVICE);
        if( eMMC_host->dmabuf <= 0)
        {
            printk("%s..%d   run dma_map_single fail!!!!\n",__FUNCTION__, __LINE__);
            BUG_ON(1);
            return FALSE;
        }
              
    }

    eMMC_host->dmalen = (size<<2);
    
    eMMC_printk(5,"%s..%d   After  call dma_map_single,  dmalen=%x, dma_addr=%x, dmabuf=%x.......\n",__FUNCTION__, __LINE__, eMMC_host->dmalen, eMMC_host->dma_addr, eMMC_host->dmabuf);
    rk29_dma_devconfig(eMMC_host->dma_chn, mode, (unsigned long )(eMMC_host->dma_addr));

    
    eMMC_printk(5,"%s..%d   Before  call rk29_dma_enqueue.......\n",__FUNCTION__, __LINE__);
    if(rw)
    {        
        
         ret = rk29_dma_enqueue(eMMC_host->dma_chn, (void *)eMMC_host,  eMMC_host->dmabuf,size<<2);
         buf1 = (uint8 *)srcAddr;

         //dma_unmap_single(NULL,   eMMC_host->dmabuf, size<<9, DMA_TO_DEVICE);

    }
    else
    {
        eMMC_printk(3,"%s...%d....=====  use DMA for read===========\n",__FUNCTION__,__LINE__, ret);
          
         ret = rk29_dma_enqueue(eMMC_host->dma_chn, (void *)eMMC_host,  eMMC_host->dmabuf, size<<2);
         buf1 = (uint8 *)dstAddr;
         
         //dma_unmap_single(NULL,   eMMC_host->dmabuf, size<<9, DMA_FROM_DEVICE);

    }
    
    eMMC_printk(3,"%s...%d.....After rk29_dma_enqueue, ret =%x \n",__FUNCTION__,__LINE__, ret);
    
    eMMC_printk(3,"%s...%d.....==============DMA config ================\n",__FUNCTION__,__LINE__);
    eMMC_printk(3, " use dma;  dma_chn=%d,  dma-addr=%x,  \n ",eMMC_host->dma_chn,eMMC_host->dma_addr);
    eMMC_printk(3, " Originbufaddr=%x, DMAbuf=%x,  size=%x, direction=%d (0--RK29_DMASRC_MEM;1--RK29_DMASRC_HW) \n ", buf1,  eMMC_host->dmabuf, size<<2, mode);
    eMMC_printk(3,"===========================================================================\n");

    if(ret<0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }


#else    
    eDMA_MODE       mode;

    if(nSDCPort == SDC0)
    {
        if(rw)
        {
            mode = DMA_PERI_SDMMC_TX;
        }
        else
        {
            mode = DMA_PERI_SDMMC_RX;
        }
    }
    else if(nSDCPort == SDC1)
    {
        if(rw)
        {
            mode = DMA_PERI_SDIO_TX;
        }
        else
        {
            mode = DMA_PERI_SDIO_RX;
        }
    }
    else
    {
        if(rw)
        {
            mode = DMA_PERI_EMMC_TX;
        }
        else
        {
            mode = DMA_PERI_EMMC_RX;
        }
    }
//传入的dma size是以word为单位的，需要将其转为bytes为单位的长度
    if(DMAOK == DMAStart(dstAddr, srcAddr, size<<2, mode, CallBack))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
#endif

#endif    
}

/****************************************************************/
//函数名:SDPAM_DMAStop
//描述:停止一个已经配置过的DMA传输
//参数说明:nSDCPort   输入参数   需要停止的端口号
//         rw         输入参数   表示停止的数据是要从卡读出的操作还是写到卡的操作，1:写到卡，0:从卡读出
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_DMAStop(SDMMC_PORT_E nSDCPort, bool rw)
{
#if SDMMC_NO_PLATFORM
    return TRUE;
#else
#if eMMC_PROJECT_LINUX
    uint32 mode;
    int ret;

    if(nSDCPort == SDC0)
    {
        if(rw)
        {
            mode = RK29_DMASRC_MEM;
        }
        else
        {
            mode = RK29_DMASRC_HW;
        }
    }
    else if(nSDCPort == SDC1)
    {
        if(rw)
        {
            mode = RK29_DMASRC_MEM;
        }
        else
        {
            mode = RK29_DMASRC_HW;
        }
    }
    else
    {
        if(rw)
        {
            mode = RK29_DMASRC_MEM;
        }
        else
        {
            mode = RK29_DMASRC_HW;
        }
    }
    
    //printk("%s..%d  ======= Stop the DMA; begin to call dma_unmap_single()=================\n",__FUNCTION__, __LINE__);
    if(rw)
    {
        
        dma_unmap_single(NULL,  eMMC_host->dmabuf, eMMC_host->dmalen, DMA_TO_DEVICE);
    }
    else
    {
        dma_unmap_single(NULL,  eMMC_host->dmabuf, eMMC_host->dmalen, DMA_FROM_DEVICE);
    }
   
    //printk("%s..%d  ======= Stop the DMA; begin to call rk29_dma_ctrl()=================\n",__FUNCTION__, __LINE__);
    ret = rk29_dma_ctrl(eMMC_host->dma_chn,RK29_DMAOP_STOP);
    if(ret<0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
       
    
#else
    eDMA_MODE       mode;
    if(nSDCPort == SDC0)
    {
        if(rw)
        {
            mode = DMA_PERI_SDMMC_TX;
        }
        else
        {
            mode = DMA_PERI_SDMMC_RX;
        }
    }
    else if(nSDCPort == SDC1)
    {
        if(rw)
        {
            mode = DMA_PERI_SDIO_TX;
        }
        else
        {
            mode = DMA_PERI_SDIO_RX;
        }
    }
    else
    {
        if(rw)
        {
            mode = DMA_PERI_EMMC_TX;
        }
        else
        {
            mode = DMA_PERI_EMMC_RX;
        }
    }
#endif

    return TRUE;
#endif    
}
#endif
/****************************************************************/
//函数名:SDPAM_INTCRegISR
//描述:向中断控制器注册某个端口的中断服务线程
//参数说明:nSDCPort   输入参数   需要注册的端口号
//         Routine    输入参数   服务线程
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_INTCRegISR(SDMMC_PORT_E nSDCPort, pFunc Routine)
{
#if SDMMC_NO_PLATFORM
    return TRUE;
#else
    return TRUE;
#endif
}

/****************************************************************/
//函数名:SDPAM_INTCEnableIRQ
//描述:使能中断控制器上某端口的中断
//参数说明:nSDCPort   输入参数   需要使能的端口号
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_INTCEnableIRQ(SDMMC_PORT_E nSDCPort)
{
#if eMMC_PROJECT_LINUX
    return TRUE;
#else
    uint32 ret = 0;
    
    if(nSDCPort == SDC0)
    {
        ret = IRQEnable(INT_SDMMC);//ret = IRQEnable(IRQ_SDMMC0);
    }
    else if(nSDCPort == SDC1)
    {
        ret = IRQEnable(INT_SDIO);//ret = IRQEnable(IRQ_SDMMC1);
    }
    else
    {
        ret = IRQEnable(INT_eMMC);
    }

    if(ret == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif
}

#if 0
void IOMUXSetSDMMC2(eIOMUX_SDMMC type)
{    
    int value;
    pGRF_REG reg=(pGRF_REG)RK29_GRF_REG_BASE;
   
    switch(type)
    {
        case IOMUX_SDMMC_1BIT:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0x0fL<<28);   //data4/data5
            value &= ~(0x3fL<<22);   //data3/data2/data1            
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            value |= (0x15L<<16);
            reg->GRF_GPIO3L_IOMUX = value;
            
            value = reg->GRF_GPIO3H_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            reg->GRF_GPIO3H_IOMUX = value;            
            break;
        case IOMUX_SDMMC_4BIT:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0xFFFL<<16);
            value |= (0x555L<<16);//clk cmd data0~3
            reg->GRF_GPIO3L_IOMUX = value;
            value = reg->GRF_GPIO3H_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            reg->GRF_GPIO3H_IOMUX = value;   
            break;
        case IOMUX_SDMMC_8BIT:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0xFFFFL<<16);
            value |= (0x5555L<<16);//clk cmd data0~5
            reg->GRF_GPIO3L_IOMUX = value;
            value = reg->GRF_GPIO3H_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            value |= (0x5L<<0);
            reg->GRF_GPIO3H_IOMUX = value;  
            break;
        case IOMUX_SDMMC_OTHER:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0x0fL<<28);   //data4/data5
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            reg->GRF_GPIO3L_IOMUX = value;
            
            value = reg->GRF_GPIO3H_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            reg->GRF_GPIO3H_IOMUX = value;  

            break;
        default:
            break;        
    }
}


/*----------------------------------------------------------------------
Name	: IOMUXSetSDMMC1
Desc	: 设置SDMMC1相关管脚
Params  : type: IOMUX_SDMMC1 设置成SDMMC1信号线
                IOMUX_SDMMC1_OTHER设置成非SDMMC1信号线
Return  : 
Notes   : 默认使用4线，不使用pwr_en, write_prt, detect_n信号
----------------------------------------------------------------------*/
void IOMUXSetSDMMC1(eIOMUX_SDMMC type)
{
    int value;
    pGRF_REG reg=(pGRF_REG)RK29_GRF_REG_BASE;
   
    switch(type)
    {
        case IOMUX_SDMMC_1BIT:
            value = reg->GRF_GPIO1H_IOMUX;
            value &= ~(0x3fL<<8);   //data3/data2/data1            
            value &= ~(0xfL<<4);    //data0/cmd
            value |= (0x5L<<4);
            value &= ~(0x3L<<14);    //clk
            value |= (0x1L<<14);            
            reg->GRF_GPIO1H_IOMUX = value;        
            
            break;
        case IOMUX_SDMMC_4BIT:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value |= (0x15L<<22);
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            value |= (0x15L<<16);
            reg->GRF_GPIO3L_IOMUX = value;

            break;
        case IOMUX_SDMMC_8BIT:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            reg->GRF_GPIO3L_IOMUX = value;


            break;
        case IOMUX_SDMMC_OTHER:
            value = reg->GRF_GPIO3L_IOMUX;
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            reg->GRF_GPIO3L_IOMUX = value;


            break;
        default:
            break;        
    }

}

/*----------------------------------------------------------------------
Name	: IOMUXSetSDMMC0
Desc	: 设置SDMMC0相关管脚
Params  : type: IOMUX_SDMMC0 设置成SDMMC0信号线
                IOMUX_SDMMC0_OTHER设置成非SDMMC0信号线
Return  : 
Notes   : 默认使用4线，不使用pwr_en, write_prt, detect_n信号
----------------------------------------------------------------------*/
void IOMUXSetSDMMC0(eIOMUX_SDMMC type)
{    
    int value;
    pGRF_REG reg=(pGRF_REG)RK29_GRF_REG_BASE;
   
    switch(type)
    {
        case IOMUX_SDMMC_1BIT:
            value = reg->GRF_GPIO1H_IOMUX;
            value &= ~(0x0fL<<28);   //data4/data5
            value &= ~(0x3fL<<22);   //data3/data2/data1            
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            value |= (0x15L<<16);
            reg->GRF_GPIO1H_IOMUX = value;
            
            value = reg->GRF_GPIO2L_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            reg->GRF_GPIO2L_IOMUX = value;            
            
            break;
        case IOMUX_SDMMC_4BIT:
            value = reg->GRF_GPIO1H_IOMUX;
            value &= ~(0x0fL<<28);   //data4/data5
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value |= (0x15L<<22);
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            value |= (0x15L<<16);
            reg->GRF_GPIO1H_IOMUX = value;
            
            value = reg->GRF_GPIO2L_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            reg->GRF_GPIO2L_IOMUX = value;   

            break;
        case IOMUX_SDMMC_8BIT:
            value = reg->GRF_GPIO1H_IOMUX;
            value &= ~(0x0fL<<28);   //data4/data5
            value |= (0x5L<<28);
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value |= (0x15L<<22);
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            value |= (0x15L<<16);
            reg->GRF_GPIO1H_IOMUX = value;
            
            value = reg->GRF_GPIO2L_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            value |= (0x5L<<28);
            reg->GRF_GPIO2L_IOMUX = value;  

            break;
        case IOMUX_SDMMC_OTHER:
            value = reg->GRF_GPIO1H_IOMUX;
            value &= ~(0x0fL<<28);   //data4/data5
            value &= ~(0x3fL<<22);   //data3/data2/data1
            value &= ~(0x3fL<<16);  //data0/cmd/clk
            reg->GRF_GPIO1H_IOMUX = value;
            
            value = reg->GRF_GPIO2L_IOMUX;
            value &= ~(0x0fL<<0);  //data6/data7
            reg->GRF_GPIO2L_IOMUX = value;  

            break;
        default:
            break;        
    }

}
#endif

/****************************************************************/
//函数名:SDPAM_IOMUX_SetSDPort
//描述:将IO复用到某个端口，并且该端口的数据线宽度由width指定
//参数说明:nSDCPort   输入参数   端口号
//         width      输入参数   数据线宽度
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_IOMUX_SetSDPort(SDMMC_PORT_E nSDCPort, HOST_BUS_WIDTH_E width)
{
    uint32 Bits = 1;

    if (((nSDCPort == SDC1) && (width == BUS_WIDTH_8_BIT)) || (width == BUS_WIDTH_INVALID))
    {
        return FALSE;
    }
    
    switch(width)
    {
        case BUS_WIDTH_1_BIT:
            Bits = 1;
            break;
        case BUS_WIDTH_4_BIT:
            Bits = 4;
            break;
        case BUS_WIDTH_8_BIT:
            Bits = 8;
            break;
        default:
            return FALSE;
    }
    
    if(nSDCPort == SDC0)
    {
        IOMUXSetSDMMC(0,Bits);
    }
    else if (nSDCPort == SDC1)
    {
        IOMUXSetSDMMC(1,Bits);
    }
    else
    {
        IOMUXSetSDMMC(2,Bits);
    }

    return TRUE;
}

/****************************************************************/
//函数名:SDPAM_IOMUX_PwrEnGPIO
//描述:将端口的电源控制复用为GPIO
//参数说明:nSDCPort   输入参数   端口号
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_IOMUX_PwrEnGPIO(SDMMC_PORT_E nSDCPort)
{
    if(nSDCPort == SDC0)
    {
        #if SDMMC0_EN_POWER_CTL
        IOMUXSetSMCS1(IOMUX_SMCS1_GPIO);
        GPIOSetPinDirection(SDMMC0_POWER_PIN, GPIO_OUT);
        #endif
    }
    else
    {
        #if SDMMC1_EN_POWER_CTL
        //IOMUX先不考虑SDMMC1的
        #endif
    }
    return TRUE;
}

#if (SDMMC0_DET_MODE == SD_GPIO_DET)
static void sdmmc0_det_Handler(void)
{
    eGPIOIntType_t intType;
    
    if(SDMMC0_DETECT_ACTIVE_LEVEL == GPIOGetPinLevel(SDMMC0_DETECT_PIN))
    {
        SDOAM_SendMsg(MSG_CARD_INSERT, SDC0);
        intType = ((SDMMC0_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelHigh : GPIOLevelLow);
        GPIOIRQRegISR(SDMMC0_DETECT_PIN, sdmmc0_det_Handler, intType);
    }
    else
    {
        SDOAM_SendMsg(MSG_CARD_REMOVE, SDC0);
        intType = ((SDMMC0_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelLow : GPIOLevelHigh);
        GPIOIRQRegISR(SDMMC0_DETECT_PIN, sdmmc0_det_Handler, intType);
    }
}
#endif

#if (SDMMC1_DET_MODE == SD_GPIO_DET)
#if 0
static void sdmmc1_det_Handler(void)
{
    eGPIOIntType_t intType;
    
    if(SDMMC1_DETECT_ACTIVE_LEVEL == GPIOGetPinLevel(SDMMC1_DETECT_PIN))
    {
        SDOAM_SendMsg(MSG_CARD_INSERT, SDC1);
        intType = ((SDMMC1_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelHigh : GPIOLevelLow);
        GPIOIRQRegISR(SDMMC1_DETECT_PIN, sdmmc1_det_Handler, intType);
    }
    else
    {
        SDOAM_SendMsg(MSG_CARD_REMOVE, SDC1);
        intType = ((SDMMC1_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelLow : GPIOLevelHigh);
        GPIOIRQRegISR(SDMMC1_DETECT_PIN, sdmmc1_det_Handler, intType);
    }
 
}
#endif  
#endif

/****************************************************************/
//函数名:SDPAM_IOMUX_DetGPIO
//描述:将端口的卡检测复用为GPIO
//参数说明:nSDCPort   输入参数   端口号
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
bool   SDPAM_IOMUX_DetGPIO(SDMMC_PORT_E nSDCPort)
{  
    #if 0
    eGPIOIntType_t intType;
    
    if(nSDCPort == SDC0)
    {
        #if (SDMMC0_DET_MODE == SD_GPIO_DET)
        //detect GPIO的IOMUX在这边做，或者在外面做好
        GPIOSetPinDirection(SDMMC0_DETECT_PIN, GPIO_IN);
        if(SDMMC0_DETECT_ACTIVE_LEVEL == GPIOGetPinLevel(SDMMC0_DETECT_PIN))
        {
            intType = ((SDMMC0_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelHigh : GPIOLevelLow);
        }
        else
        {
            intType = ((SDMMC0_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelLow : GPIOLevelHigh);
        }
        GPIOIRQRegISR(SDMMC0_DETECT_PIN, sdmmc0_det_Handler, intType);
        GPIOEnableIntr(SDMMC0_DETECT_PIN);
        #endif
    }
    else
    {
        #if (SDMMC1_DET_MODE == SD_GPIO_DET)
        //detect GPIO的IOMUX在这边做，或者在外面做好
        GPIOSetPinDirection(SDMMC1_DETECT_PIN, GPIO_IN);
        if(SDMMC1_DETECT_ACTIVE_LEVEL == GPIOGetPinLevel(SDMMC1_DETECT_PIN))
        {
            intType = ((SDMMC1_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelHigh : GPIOLevelLow);
        }
        else
        {
            intType = ((SDMMC1_DETECT_ACTIVE_LEVEL == GPIO_LOW) ? GPIOLevelLow : GPIOLevelHigh);
        }
        GPIOIRQRegISR(SDMMC1_DETECT_PIN, sdmmc1_det_Handler, intType);
        GPIOEnableIntr(SDMMC1_DETECT_PIN);
        #endif
    }
    #endif
    
    return TRUE;
}

/****************************************************************/
//函数名:SDPAM_ControlPower
//描述:控制指定端口的card电源开启或关闭
//参数说明:nSDCPort 输入参数   端口号
//         enable   输入参数   1:开启电源，0:关闭电源
//返回值:
//相关全局变量:
//注意:
/****************************************************************/
void SDPAM_ControlPower(SDMMC_PORT_E nSDCPort, bool enable)
{
#if SDMMC0_EN_POWER_CTL
    eGPIOPinLevel_t  level;
#endif
    if(nSDCPort == SDC0)
    {
        #if SDMMC0_EN_POWER_CTL
        level = enable ? SDMMC0_POWER_ACTIVE_LEVEL : ((SDMMC0_POWER_ACTIVE_LEVEL == GPIO_LOW) ? GPIO_HIGH : GPIO_LOW);
        GPIOSetPinLevel(SDMMC0_POWER_PIN, level);
        #endif
    }
    else if(nSDCPort == SDC1)
    {   
        #if SDMMC1_EN_POWER_CTL
        level = enable ? SDMMC0_POWER_ACTIVE_LEVEL : ((SDMMC0_POWER_ACTIVE_LEVEL == GPIO_LOW) ? GPIO_HIGH : GPIO_LOW);
        //IOMUX先不考虑SDMMC1的
        #endif
    }
    else
    {
        #if SDMMC2_EN_POWER_CTL
        //level = enable ? SDMMC0_POWER_ACTIVE_LEVEL : ((SDMMC0_POWER_ACTIVE_LEVEL == GPIO_LOW) ? GPIO_HIGH : GPIO_LOW);
        //GPIOSetPinLevel(SDMMC0_POWER_PIN, level);
        #endif
    }
    
}

/****************************************************************/
//函数名:SDPAM_IsCardPresence
//描述:检查端口上的卡是否在卡槽上
//参数说明:nSDCPort 输入参数   端口号
//返回值: TRUE       卡在卡槽上
//        FALSE      卡不在卡槽上
//相关全局变量:
//注意:
/****************************************************************/
bool SDPAM_IsCardPresence(SDMMC_PORT_E nSDCPort)
{
    if(nSDCPort == SDC0)
    {
        #if (SDMMC0_DET_MODE == SD_GPIO_DET)
        if(SDMMC0_DETECT_ACTIVE_LEVEL == GPIOGetPinLevel(SDMMC0_DETECT_PIN))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
        #else
        return TRUE;
        #endif
    }
    else
    {
        #if (SDMMC1_DET_MODE == SD_GPIO_DET)
        //IOMUX先不考虑SDMMC1的
        return TRUE;
        #else
        return TRUE;
        #endif
    }
}

#endif //end of #ifdef DRIVERS_SDMMC
