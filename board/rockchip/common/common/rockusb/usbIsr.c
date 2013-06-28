/********************************************************************************
*********************************************************************************
                        COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
                                --  ALL RIGHTS RESERVED  --

File Name:      ISR.C
Author:         XUESHAN LIN
Created:        1st Dec 2008
Modified:
Revision:       1.00
********************************************************************************
********************************************************************************/
#define     IN_ISR
#include    "../../armlinux/config.h"
#include 	"USB20.h"		//USB部分总头文件

extern void  CacheFlushBoth(void);
/**************************************************************************
USB中断服务子程序
***************************************************************************/
void UsbIsr(void)
{
    uint32 IntFlag;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    IntFlag=OtgReg->Core.gintsts & OtgReg->Core.gintmsk;
    if (IntFlag == 0)
        return;

    if(IntFlag & (1<<4))       //receive FIFO non-enpty
    {
        OtgReg->Core.gintmsk &= ~(1<<4);
        RxFifoNonEmpty();
        OtgReg->Core.gintmsk |= 1<<4;
    }
    if(IntFlag & (1<<5))    //xfer FIFO enpty
    {
        OtgReg->Core.gintmsk &= ~(1<<5);
        OtgReg->Device.dtknqr4_fifoemptymsk=0;
    }
    if(IntFlag & (1<<10))       //early suspend
    {
        OtgReg->Core.gintsts=1<<10;
    }
    if(IntFlag & (1<<11))       //suspend
    {
        OtgReg->Core.gintsts=1<<11;
    }
    if(IntFlag & (1<<12))  //USB总线复位
    {
        BusReset();
        OtgReg->Core.gintsts=1<<12;
    }
    if(IntFlag & (1<<13))  //Enumeration done
    {
        EnumDone();
        OtgReg->Core.gintsts=1<<13;
    }
    if(IntFlag & (1<<18))       //IN中断
    {
        InIntr();
    }
    if(IntFlag & (1<<19))       //OUT中断
    {
        OutIntr();
    }
    
    if(IntFlag & (1<<30))  //USB VBUS中断
    {
        OtgReg->Core.gintsts=1<<30;
    }
    if(IntFlag & (1ul<<31))     //resume
    {
        OtgReg->Core.gintsts=1ul<<31;
    }
    if(IntFlag & ((1<<22)|(1<<6)|(1<<7)|(1<<17)))
    {
        OtgReg->Core.gintsts=IntFlag & ((1<<22)|(1<<6)|(1<<7)|(1<<17));
    }
}


/**************************************************************************
总线复位处理子程序
***************************************************************************/
void BusReset(void)
{
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;
    SecureBootLock = SecureBootLock_backup; //恢复lock
    UsbBusReset++;
    OtgReg->Device.dcfg &= ~0x07f0;                 //reset device addr
    ControlStage=STAGE_IDLE;
    OtgReg->Device.dctl &= ~0x01;      //Clear the Remote Wakeup Signalling
}


/**************************************************************************
枚举完成子程序
***************************************************************************/
void EnumDone(void)
{
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    BulkEpSize=FS_BULK_TX_SIZE;
    switch ((OtgReg->Device.dsts>>1) & 0x03)
    {
        case 0:         //High speed, PHY clock @30MHz or 60MHz
            BulkEpSize=HS_BULK_TX_SIZE;
        case 1:         //Full speed, PHY clock @30MHz or 60MHz
        case 3:         //Full speed, PHY clock @48MHz
            OtgReg->Device.InEp[0].DiEpCtl &= ~0x03;   //64bytes MPS
            Ep0PktSize=EP0_PACKET_SIZE_HS;
            break;
        case 2:
        default:
            OtgReg->Device.InEp[0].DiEpCtl |= 0x03;   //8bytes MPS
            Ep0PktSize=EP0_PACKET_SIZE_FS;
            break;
    }
    OtgReg->Device.dctl |= 1<<8;               //clear global IN NAK
    usbCmd.Precmd = 0;
    usbCmd.cmd = 0;
    usbCmd.LUN = 0;
    usbCmd.LBA = 0;
    usbCmd.len = 0;
    usbCmd.xferBuf = 0;
    usbCmd.xferLBA = 0;
    usbCmd.xferLen = 0;
    usbCmd.preLen = 0;
    usbCmd.preLBA = 0;
    usbCmd.preBuf = 0;
    usbCmd.pppBufId = 0;
    ReadEndpoint0(Ep0PktSize, Ep0Buf);
    //ReadBulkEndpoint(BulkEpSize, DataBuf);
    ReadBulkEndpoint(31, (uint8*)&gCBW);
    OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = (1ul<<28) | (1<<15)|(2<<18)|(BULK_IN_EP<<22);
    FWCmdPhase=K_CommandPhase;
    FW_DataLenCnt=0;
    FW_WR_Mode = FW_WR_MODE_LBA;
    UsbConnected = 1;
}


/**************************************************************************
Receive FIFO non-enpty
***************************************************************************/
void RxFifoNonEmpty(void)
{

}


/**************************************************************************
SETUP中断
***************************************************************************/
void Setup(void)
{
    //CacheFlushBoth();
    CacheFlushDRegion((uint32)Ep0Buf,(uint32)64);
    ftl_memcpy(&ControlData.DeviceRequest, Ep0Buf, 8);
    ControlHandler();                      //调用请求处理子程序
}


/***************************************************************************
请求处理子程序
***************************************************************************/
void ControlHandler(void)
{
    uint8 type, req;

    type = ControlData.DeviceRequest.bmRequestType & USB_REQUEST_TYPE_MASK;
    req = ControlData.DeviceRequest.bRequest & USB_REQUEST_MASK;

    if (type == USB_STANDARD_REQUEST)
    {
        switch (req)
        {
            case 5:
                set_address();
                break;
            case 6:
                get_descriptor();
                break;
            case 9:
                UsbConnected = 1;
                set_configuration();
                break;
            default:
                stall_ep0();
                break;
        }
    }
    else
        stall_ep0();
}


/**************************************************************************
EP0 OUT packet处理
***************************************************************************/
void Ep0OutPacket(uint16 len)
{
//    len=len;
#ifdef DEBUG_FLASH 
    int i;
    RkPrintf("Ep0Buf,tick=%d\n" , RkldTimerGetTick());
    for(i=0;i<len;i++)
        RkPrintf("0x%x " , Ep0Buf[i]);
    RkPrintf("\n");
#endif
}

/**************************************************************************
端点0OUT中断
***************************************************************************/
void OutIntr(void)
{
    uint32 i;
    uint32 ch;
    uint32 event;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    ch=(OtgReg->Device.daint & OtgReg->Device.daintmsk) >> 16;   //在ROM里只有EP0中断
    for (i=0; i<3; i++)
    {
        if ((1<<i) & ch)
        {
            event=OtgReg->Device.OutEp[i].DoEpInt & OtgReg->Device.doepmsk;
            if ((event & 0x01) != 0)        //Transfer complete
            {
                OtgReg->Device.OutEp[i].DoEpInt=0x01;
                if (i==0)
                {
                    uint32 len;
                    len=Ep0PktSize-(OtgReg->Device.OutEp[0].DoEpTSiz&0x7f);
                    if (len>0)
                        Ep0OutPacket(len);
                    ReadEndpoint0(Ep0PktSize, Ep0Buf);
                }
                else
                {
                    uint32 len;
                    len=0x20000-(OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz&0x1ffff);
                    if (len>0)
                    {
                        FWOutPacket(len);
                    }
                }
            }
            if ((event & 0x02) != 0)        //Endpoint disable
            {
                OtgReg->Device.OutEp[i].DoEpInt=0x02;
            }
            if ((event & 0x04) != 0)        //AHB Error
            {
                OtgReg->Device.OutEp[i].DoEpInt=0x04;
            }
            if ((event & 0x08) != 0)        //Setup Phase Done (contorl EPs)
            {
                OtgReg->Device.OutEp[i].DoEpInt=0x08;
                Setup();
                ReadEndpoint0(Ep0PktSize, Ep0Buf);
            }
        }
    }
}

/**************************************************************************
端点0 IN中断
***************************************************************************/
void InIntr(void)
{
    uint32 i;
    uint32 ch;
    uint32 event;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    ch=(OtgReg->Device.daint & OtgReg->Device.daintmsk) & 0xffff;   //在ROM里只有EP0中断
    for (i=0; i<3; i++)
    {
        if ((1<<i) & ch)
        {
            event=OtgReg->Device.diepmsk | ((OtgReg->Device.dtknqr4_fifoemptymsk & 0x01)<<7);    //<<7是因为msk是保留的
            event &= OtgReg->Device.InEp[i].DiEpInt;
            if ((event & 0x01) != 0)        //Transfer complete
            {
                OtgReg->Device.InEp[i].DiEpInt=0x01;
                if (i != 0)
                    FWInPacket();
            }
            if ((event & 0x02) != 0)        //Endpoint disable
            {
                OtgReg->Device.InEp[i].DiEpInt=0x02;
            }
            if ((event & 0x04) != 0)        //AHB Error
            {
                OtgReg->Device.InEp[i].DiEpInt=0x04;
            }
            if ((event & 0x08) != 0)        //TimeOUT Handshake (non-ISOC IN EPs)
            {
                OtgReg->Device.InEp[i].DiEpInt=0x08;
            }
            if ((event & 0x20) != 0)        //IN Token Received with EP mismatch
            {
                OtgReg->Device.InEp[i].DiEpInt=0x20;
            }
            if ((event & 0x80) != 0)        //Transmit FIFO empty
            {
                OtgReg->Device.InEp[i].DiEpInt=0x10;
            }
            if ((event & 0x100) != 0)       //Buffer Not Available
            {
                OtgReg->Device.InEp[i].DiEpInt=0x100;
            }
        }
    }
}

