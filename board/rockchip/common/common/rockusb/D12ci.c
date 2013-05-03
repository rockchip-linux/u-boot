/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    D12CI.C
Author:         XUESHAN LIN
Created:        1st Dec 2008
Modified:
Revision:       1.00
********************************************************************************
********************************************************************************/
#define     IN_D12CI
#include    "../../armlinux/config.h"
#include 	"USB20.h"		//USB部分总头文件

extern void DRVDelayUs(uint32 us);
//#define FORCE_FS

/**************************************************************************
USB控制器寄存器初始化
***************************************************************************/
void UdcInit(void)
{
    uint32 count;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;
    
    UsbConnected = 0;
    UsbBusReset = 0;

    //wait AHB Master idle
    for (count=0; count<10000; count++)
    {
        if ((OtgReg->Core.grstctl & (1ul<<31))!=0)
            break;
    }
    //core soft reset
    OtgReg->Core.grstctl|=1<<0;               //Core soft reset
    for (count=0; count<10000; count++)
    {
        if ((OtgReg->Core.grstctl & (1<<0))==0)
            break;
    }

    OtgReg->ClkGate.PCGCR=0x00;             //Restart the Phy Clock
    OtgReg->Device.dcfg &= ~0x03;                   //Enable HS
#ifdef FORCE_FS    
    OtgReg->Device.dcfg |= 0x01;                    //Force FS
#endif    
    OtgReg->Device.dcfg &= ~0x07f0;                 //reset device addr
    OtgReg->Core.grstctl|=(0x10<<6) | (1<<5);       //Flush all Txfifo
    for (count=0; count<10000; count++)
    {
        if ((OtgReg->Core.grstctl & (1<<5))==0)
            break;
    }
    OtgReg->Core.grstctl|=1<<4;                     //Flush all Rxfifo
    for (count=0; count<10000; count++)
    {
        if ((OtgReg->Core.grstctl & (1<<4))==0)
            break;
    }
    OtgReg->Core.grstctl|=1<<3;                     //Flush IN token lenarning queue
#if(PALTFORM != RK28XX)
    OtgReg->Core.grxfsiz = 0x00000210;
    OtgReg->Core.gnptxfsiz = 0x00100210;
    OtgReg->Core.dptxfsiz_dieptxf[0] = 0x01000220;
    OtgReg->Core.dptxfsiz_dieptxf[1] = 0x00100320;
#endif
    OtgReg->Device.InEp[0].DiEpCtl=(1<<27)|(1<<30);        //IN0 SetNAK & endpoint disable
    OtgReg->Device.InEp[0].DiEpTSiz=0;
    OtgReg->Device.InEp[0].DiEpDma=0;
    OtgReg->Device.InEp[0].DiEpInt=0xff;

    OtgReg->Device.OutEp[0].DoEpCtl=(1<<27)|(1<<30);        //OUT0 SetNAK & endpoint disable
    OtgReg->Device.OutEp[0].DoEpTSiz=0;
    OtgReg->Device.OutEp[0].DoEpDma=0;
    OtgReg->Device.OutEp[0].DoEpInt=0xff;

    OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = 1<<28;
    OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl=(1ul<<31)|(1<<28)|(1<<26)|(2<<22)|(2<<18)|(1<<15)|0x200;
    OtgReg->Device.OutEp[BULK_OUT_EP].DoEpInt=0xff;

    OtgReg->Device.diepmsk=0x2f;                   //device IN interrutp mask
    OtgReg->Device.doepmsk=0x0f;                   //device OUT interrutp mask
    OtgReg->Device.daint=0xffffffff;               //clear all pending intrrupt
    OtgReg->Device.daintmsk=0x00010001 | ((1<<BULK_IN_EP) | ((1<<BULK_OUT_EP)<<16));    //device all ep interrtup mask(IN0 & OUT0)
    OtgReg->Core.gintsts=0xffffffff;
    OtgReg->Core.gotgint=0xffffffff;
    OtgReg->Core.gintmsk=(1<<4)|/*(1<<5)|*/(1<<10)|(1<<11)|(1<<12)|(1<<13)|(1<<18)|(1<<19)|(1ul<<30)|(1ul<<31);
    OtgReg->Core.gahbcfg |= 0x01;             //Global interrupt mask
    OtgReg->Core.gahbcfg |= 1<<5;
#if(PALTFORM != RK28XX)
    OtgReg->Core.gahbcfg |= 7<<1;
#endif
    OtgReg->Core.gintmsk&=~(1<<4);
}


/**************************************************************************
读取端点数据
***************************************************************************/
void ReadEndpoint0(uint16 len, void *buf)
{
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    CacheFlushDRegion((uint32)buf,(uint32)len);
    OtgReg->Device.OutEp[0].DoEpDma=(uint32)buf;
    OtgReg->Device.OutEp[0].DoEpTSiz=Ep0PktSize | (1<<19);
    OtgReg->Device.OutEp[0].DoEpCtl = (1ul<<15) | (1ul<<26) | (1ul<<31);     //Active ep, Clr Nak, endpoint enable
}


/**************************************************************************
写端点
***************************************************************************/
void WriteEndpoint0(uint16 len, void* buf)
{
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    if (((OtgReg->Core.gnptxsts & 0xffff) >= (len+3)/4) && (((OtgReg->Core.gnptxsts>>16) & 0xff)>0))
    {
        OtgReg->Device.InEp[0].DiEpTSiz=len | (1<<19);
        OtgReg->Device.InEp[0].DiEpDma=(uint32)buf;
        OtgReg->Device.InEp[0].DiEpCtl = (1ul<<26) | (1ul<<31);     //endpoint enable
    }
}

/**************************************************************************
读取端点数据
***************************************************************************/
void ReadBulkEndpoint(uint32 len, void *buf)
{
    uint32 regBak;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;
    
    CacheFlushDRegion((uint32)buf,(uint32)len);
    //RkPrintf("ReadBulkEndpoint %d\n",len);
    OtgReg->Device.OutEp[BULK_OUT_EP].DoEpDma=(uint32)buf;
   // OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz=BulkEpSize | (1<<19);
    OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz=0x20000 | (((len+BulkEpSize-1)/BulkEpSize)<<19);
    regBak=OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl;
   //OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl |= (1ul<<15) | (1ul<<26) | (1ul<<31) | BulkEpSize; //Active ep, Clr Nak, endpoint enable
   OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl = (regBak&0xFFFFF800) | (1ul<<15) | (1ul<<19) | (1ul<<26) | (1ul<<31) | BulkEpSize; //Active ep, Clr Nak, endpoint enable
}

/**************************************************************************
写端点
***************************************************************************/
void WriteBulkEndpoint(uint32 len, void* buf)
{
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;
    uint32 regBak;
    //RkPrintf("WriteBulkEndpoint,DTXFSTS=%d\n" , (OtgReg->Device.InEp[BULK_IN_EP].DTXFSTS & 0xffff));
    //RkPrintf("WriteBulkEndpoint %d\n",len);
//    if ((OtgReg->Device.InEp[BULK_IN_EP].DTXFSTS & 0xffff) >= (len+3)/4)
    {
        OtgReg->Device.InEp[BULK_IN_EP].DiEpTSiz=len | (((len+BulkEpSize-1)/BulkEpSize)<<19);
        OtgReg->Device.InEp[BULK_IN_EP].DiEpDma=(uint32)buf;
        regBak=((OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl & (1<<16))==0)?(1<<28):(1<<29);
        regBak |= (1<<15)|(2<<18)|(BULK_IN_EP<<22)|BulkEpSize; //endpoint enable
        regBak |= (1ul<<26)|(1ul<<31);
        OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = regBak;
    }
}


