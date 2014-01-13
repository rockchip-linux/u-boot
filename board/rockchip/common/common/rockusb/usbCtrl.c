/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    USRCTRL.C
Author:         XUESHAN LIN
Created:        1st Dec 2008
Modified:
Revision:       1.00
********************************************************************************
********************************************************************************/
#define     IN_USBCTRL
#define	    IN_ROCKUSB
#include    "../../armlinux/config.h"
#include 	"USB20.h"		//USB部分总头文件
#define     ENUM_EN

volatile uint8           UsbConnected;
volatile uint8           UsbBusReset;
volatile uint8           ControlStage;
volatile uint8           Ep0PktSize;
volatile uint16          BulkEpSize;

CONTROL_XFER    ControlData;
ALIGN(8)        uint8 Ep0Buf[64];

#define DWC_OTG_HOST_PORT_REGS_OFFSET 0x440

int dwc_otg_check_dpdm(void)
{
    volatile unsigned int * otg_dctl;
    volatile unsigned int * otg_gotgctl;
    volatile unsigned int * otg_hprt0;
    int bus_status = 0;
    char *OtgReg = USB_OTG_BASE_ADDR_VA;

    g_cruReg->CRU_SOFTRST_CON[4] = ((7<<5)<<16)|(7<<5);    // otg0 phy clkgate
    udelay(3);
    g_cruReg->CRU_SOFTRST_CON[4] = ((7<<5)<<16)|(0<<5);    // otg0 phy clkgate
    mdelay(50);
    g_cruReg->CRU_CLKGATE_CON[1] = ((1<<5)<<16);    // otg0 phy clkgate
    g_cruReg->CRU_CLKGATE_CON[5] = ((1<<13)<<16);   // otg0 hclk clkgate
    g_cruReg->CRU_CLKGATE_CON[4] = ((3<<5)<<16);    // hclk usb clkgate
   #if (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
   if(ChipType == CONFIG_RK3026){
        g_grfReg->GRF_UOC0_CON0 = ((0x01<<0)<<16);
    }
   #else
    if(ChipType == CONFIG_RK3066)
    {
        g_grfReg->GRF_UOC0_CON[2] = ((0x01<<2)<<16);    // exit suspend.
        mdelay(105);
        // printf("regbase %p 0x%x, otg_phy_con%p, 0x%x\n",
        //      OtgReg, *(OtgReg), &g_grfReg->GRF_UOC0_CON[2], g_grfReg->GRF_UOC0_CON[2]); 
    }else {
        g_3188_grfReg->GRF_UOC0_CON[2] = ((0x01<<2)<<16);    // exit suspend.
        mdelay(105);
        // printf("regbase %p 0x%x, otg_phy_con%p, 0x%x\n",
        //     OtgReg, *(OtgReg), &g_3188_grfReg->GRF_UOC0_CON[2], g_3188_grfReg->GRF_UOC0_CON[2]);
     }
   #endif
    otg_dctl = (unsigned int * )(OtgReg+0x804);

    otg_gotgctl = (unsigned int * )(OtgReg);

    otg_hprt0 = (unsigned int * )(OtgReg + DWC_OTG_HOST_PORT_REGS_OFFSET);

    if(*otg_gotgctl &(1<<19)){
        bus_status = 1;
        *otg_dctl &= ~2;
        mdelay(50);    // delay about 10ms
    // check dp,dm
        printf("%s otg_dctl=0x%x,otg_hprt0 = 0x%x\n",__func__,*otg_dctl,*otg_hprt0);
        if((*otg_hprt0 & 0xc00)==0xc00)
            bus_status = 2;
	*otg_dctl |= 2;
    }
   // printf("%s %d \n",__func__,bus_status);

    return bus_status;
}

/**************************************************************************
函数描述:获取VBUS状态
入口参数:
出口参数:0－没有检测到VBUS, 1－有检测到VBUS
调用函数:
***************************************************************************/
uint32 GetVbus(void)
{
    uint32 vbus = 1;
//#if(PALTFORM != RK28XX && PALTFORM != RK29XX )
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR_VA;
    vbus=(OtgReg->Core.gotgctl >> 19) & 0x01;
    DRVDelayUs(1);
    vbus=(OtgReg->Core.gotgctl >> 19) & 0x01;
//#endif 
    return (vbus);     //vbus状态
}


uint32 TimeOutBase;
void UsbHook(void)
{
    uint16 byte;
    int32  temp;
    int32  backup;
    uint32 len;
    uint32 timer;
	uint32 *pBuf;
    uint32 curLen;
    uint32 curLBA;
    uint32 curBuf;
    uint32 power_on = 1;
    uint32 power_on_timeout = 0;
    uint32 preLBA;
    uint32 xferLen;

    usbCmd.cmd = 0;
    TimeOutBase = RkldTimerGetTick();
    powerOn();
    FW_ReIntForUpdate();
	RkPrintf("powerOn,%d\n" , RkldTimerGetTick());
    while(1)
    {
        if (usbCmd.cmd==K_FW_READ_10)
        {
            power_on_timeout=0;
            usbCmd.Precmd = usbCmd.cmd;
            usbCmd.cmd = 0;
            /*while(usbCmd.len == 0)
            {
                PRINT_E("len error\n");
            }*/
            if((usbCmd.len-usbCmd.xferLen)>0x40)
            {
                curLen = 0x40;
            }
            else
            {
                curLen = usbCmd.len -usbCmd.xferLen;
            }

            //use the pre-read data first
            if(0)//(usbCmd.xferLBA == usbCmd.preLBA)&&(usbCmd.preLen >= curLen))
            {
                usbCmd.xferBuf = usbCmd.preBuf;
                curBuf = usbCmd.pppBufId;
                curLen = (curLen>usbCmd.preLen)?usbCmd.preLen:curLen;

                //we have to use all the pre-read data,
                //or non-sequence read from nand flash will be very slow.
                usbCmd.preLBA += curLen;
                usbCmd.preLen -= curLen;
                usbCmd.preBuf += curLen*132;
            }
            else
            {
                curBuf = usbCmd.pppBufId+1;
                usbCmd.xferBuf = bulkBuf[curBuf&0x01];
                //pLun->read(pLun->InDevId, usbCmd.xferLBA, curLen, usbCmd.xferBuf);
                StorageReadPba(usbCmd.xferLBA,usbCmd.xferBuf,curLen);
            }
            xferLen = usbCmd.xferLen;
            len = usbCmd.len;
            
            usbCmd.xferLBA += curLen;
            usbCmd.xferLen += curLen;
            preLBA = usbCmd.xferLBA;
            
            if(usbCmd.len == usbCmd.xferLen)
            {
                //usbCmd.xferLen = 0;
                //usbCmd.xferLBA = 0;
                FWCmdPhase = K_InCSWPhase;
            } 
            WriteBulkEndpoint(curLen*528, usbCmd.xferBuf); 
            //Pre-read
            if(0)//usbCmd.len >= 0x80)
            {
                /*if((usbCmd.len == usbCmd.xferLen)||((usbCmd.len - usbCmd.xferLen)>0x80))
                {
                    curLen = 0x80;
                }
                else
                {
                    curLen = usbCmd.len - usbCmd.xferLen;
                }*/
                //change to another buffer anytime we need to pre-read
                usbCmd.pppBufId = curBuf+1;
                usbCmd.preBuf = bulkBuf[usbCmd.pppBufId&0x01];
                usbCmd.preLBA = preLBA;
                usbCmd.preLen = curLen;
                StorageReadPba(usbCmd.preLBA, usbCmd.preBuf,curLen);
            }

        }
        else if(usbCmd.cmd==K_FW_LBA_READ_10)
        {
            power_on_timeout=0;
            usbCmd.Precmd = usbCmd.cmd;
            usbCmd.cmd = 0;
            //PRINT_E("K_FW_LBA_READ_10 %x %x\n",usbCmd.len,usbCmd.xferLen);
              /*while(usbCmd.len == 0)
              {
                  PRINT_E("read len error\n");
              }*/
              if((usbCmd.len-usbCmd.xferLen)>0x80)
              {
                  curLen = 0x80;
              }
              else
              {
                  curLen = usbCmd.len -usbCmd.xferLen;
              }
              //use the pre-read data first
              if((usbCmd.xferLBA == usbCmd.preLBA)&&(usbCmd.preLen >= curLen))
              {
                  usbCmd.xferBuf = usbCmd.preBuf;
                  curBuf = usbCmd.pppBufId;
                  curLen = (curLen>usbCmd.preLen)?usbCmd.preLen:curLen;
                  //PRINTF("usbCmd.preLBA=%x %x\n",usbCmd.preLBA,curLen);
                  //we have to use all the pre-read data,
                  //or non-sequence read from nand flash will be very slow.
                  usbCmd.preLBA += curLen;
                  usbCmd.preLen -= curLen;
                  usbCmd.preBuf += curLen*128;
              }
              else
              {
                  //PRINTF("xferLBA=%x %x\n",usbCmd.xferLBA,usbCmd.preLBA);
                  curBuf = usbCmd.pppBufId+1;
                  usbCmd.xferBuf = bulkBuf[curBuf&0x01];
                  
                  if(usbCmd.xferLBA >= 0xFFFFFF00)
                  {
                      UsbStorageSysDataLoad(usbCmd.xferLBA -0xFFFFFF00,curLen,usbCmd.xferBuf);
                  }
                  else if(curLBA == 0xFFFFF000)
                  {
                      SecureBootUnlockCheck(pBuf);
                  }
                  else
                  {
                      StorageReadLba(usbCmd.xferLBA,usbCmd.xferBuf,curLen);
                  }
              }

              xferLen = usbCmd.xferLen;
              len = usbCmd.len;
              usbCmd.xferLBA += curLen;
              usbCmd.xferLen += curLen;
              preLBA = usbCmd.xferLBA;
              if(usbCmd.len == usbCmd.xferLen)
              {
                  //usbCmd.xferLen = 0;
                  //usbCmd.xferLBA = 0;
                  FWCmdPhase = K_InCSWPhase;
              } 
              WriteBulkEndpoint(curLen*512, usbCmd.xferBuf); 

              //Pre-read
              if(usbCmd.len >= 0x20)
              {
                  /*if((usbCmd.len == usbCmd.xferLen)||((usbCmd.len - usbCmd.xferLen)>0x80))
                  {
                      curLen = 0x80;
                  }
                  else
                  {
                      curLen = usbCmd.len - usbCmd.xferLen;
                  }*/
                  //change to another buffer anytime we need to pre-read
                  //PRINT_E("K_FW_LBA_READ_10 curLen %d \n",curLen);
                  usbCmd.pppBufId = curBuf+1;
                  usbCmd.preBuf = bulkBuf[usbCmd.pppBufId&0x01];
                  usbCmd.preLBA = preLBA;
                  usbCmd.preLen = curLen;
                  //transfer complete
                  StorageReadLba(usbCmd.preLBA, usbCmd.preBuf,curLen);
              }
        }
#if defined(DRIVERS_SPI) || defined(OTP_DATA_ENABLE)
        else if(usbCmd.cmd==K_FW_SPI_READ_10)
        {
            usbCmd.Precmd = usbCmd.cmd;
            usbCmd.cmd = 0;

              while(usbCmd.len == 0)
              {
                  PRINT_E("read len error\n");
              }
              if((usbCmd.len-usbCmd.xferLen)>0x10000)
              {
                  curLen = 0x10000;
              }
              else
              {
                  curLen = usbCmd.len -usbCmd.xferLen;
              }

              curBuf = usbCmd.pppBufId+1;
              usbCmd.xferBuf = bulkBuf[curBuf&0x01];
              #ifdef OTP_DATA_ENABLE
              OTPFlashRead(usbCmd.xferLBA,usbCmd.xferBuf,curLen);
              #else
              SPIFlashRead(usbCmd.xferLBA,usbCmd.xferBuf,curLen);
			  #endif
              preLBA = usbCmd.xferLBA;
              xferLen = usbCmd.xferLen;
              len = usbCmd.len;
              
              usbCmd.xferLBA += curLen;
              usbCmd.xferLen += curLen;
              
              if(usbCmd.len == usbCmd.xferLen)
              {
                  FWCmdPhase = K_InCSWPhase;
              } 
              WriteBulkEndpoint(curLen, usbCmd.xferBuf); 
        }
#endif
        else if(usbCmd.cmd==K_FW_WRITE_10)
        {
		    power_on_timeout=0;
            ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
            usbCmd.cmd=0;
            pBuf = usbCmd.xferBuf;
            curLBA = usbCmd.xferLBA;
            curLen = usbCmd.xferLen;
            usbCmd.pppBufId ++;
            usbCmd.preLen += (curLen>>9);
            
            CacheFlushDRegion((uint32)pBuf,(uint32)curLen); // spi 比较慢，先写数据,不然数据没有写完，就来读，会出错。
            if(SecureBootLock == 0)
                StorageWritePba(curLBA,pBuf,(curLen/528));
                
            if(usbCmd.preLen == usbCmd.len)
            {
                usbCmd.preLen = 0;
                SendCSW();
            }
            else
            {
                len = ((usbCmd.len-usbCmd.preLen)>0x40)?0x40:(usbCmd.len-usbCmd.preLen);
                usbCmd.xferBuf = bulkBuf[usbCmd.pppBufId&0x01];
                usbCmd.xferLBA += (curLen/528);
                ReadBulkEndpoint(len*528, usbCmd.xferBuf);
            }
        }
        else if(usbCmd.cmd==K_FW_LBA_WRITE_10)
        {
            #if(PALTFORM==RK28XX)
            ISetLoaderFlag(0x8888AAAA);
            #endif
            power_on_timeout=0;
            usbCmd.cmd=0;
            pBuf = usbCmd.xferBuf;
            curLBA = usbCmd.xferLBA;
            curLen = usbCmd.xferLen;
            usbCmd.pppBufId ++;
            usbCmd.preLen += (curLen>>9);
            if(usbCmd.preLen == usbCmd.len)
            {
                usbCmd.preLen = 0;
                SendCSW();
            }
            else
            {
                len = ((usbCmd.len-usbCmd.preLen)>0x80)?0x80:(usbCmd.len-usbCmd.preLen);
                usbCmd.xferBuf = bulkBuf[usbCmd.pppBufId&0x01];
                usbCmd.xferLBA += (curLen>>9);
                ReadBulkEndpoint(len*512, usbCmd.xferBuf);
            }
            CacheFlushDRegion((uint32)pBuf,(uint32)curLen);
            if(curLBA >= 0xFFFFFF00)
            {
                UsbStorageSysDataStore(curLBA -0xFFFFFF00,(curLen>>9),pBuf);
            }
            else if(curLBA == 0xFFFFF000)
            {
                SecureBootUnlock(pBuf);
            }
            else if(SecureBootLock == 0)
            {
                StorageWriteLba(curLBA,pBuf,(curLen>>9),FW_IMG_WR_Mode);
            }
        }
#if defined(DRIVERS_SPI) || defined(OTP_DATA_ENABLE)
        else if(usbCmd.cmd==K_FW_SPI_WRITE_10)
        {
            usbCmd.cmd=0;
            pBuf = usbCmd.xferBuf;
            curLBA = usbCmd.xferLBA;
            curLen = usbCmd.xferLen;
            usbCmd.pppBufId ++;
            usbCmd.preLen += (curLen);
            if(usbCmd.preLen == usbCmd.len)
            {
                usbCmd.preLen = 0;
                SendCSW();
            }
            else
            {
                len = ((usbCmd.len-usbCmd.preLen)>0x10000)?0x10000:(usbCmd.len-usbCmd.preLen);
                usbCmd.xferBuf = bulkBuf[usbCmd.pppBufId&0x01];
                usbCmd.xferLBA += (curLen);
                ReadBulkEndpoint(len, usbCmd.xferBuf);
            }
            CacheFlushDRegion((uint32)pBuf,(uint32)curLen);
            #ifdef OTP_DATA_ENABLE
            OTPFlashWrite(curLBA, pBuf, curLen);
            #else
            SPIFlashWrite(curLBA, pBuf, curLen);
            #endif
        }
 #endif
#ifdef LINUX_LOADER
        else if(FW_SDRAM_ExcuteAddr)
        {
            //extern void UsbBootLinux(uint32 KernelAddr,uint32 Parameter);
            //UsbBootLinux(FW_SDRAM_ExcuteAddr,FW_SDRAM_Parameter);
            FW_SDRAM_ExcuteAddr = 0 ;
        }
        else if(UsbConnected==0)
        {
            if(g_BootRockusb != 2)
            {
        	    if(((GetVbus() == 0)&&(((RkldTimerGetTick() - TimeOutBase > (1500*1000)))))
        	    ||  ((GetVbus() == 1)&&((RkldTimerGetTick() - TimeOutBase > (10*1000*1000))))     )
                {
                    char    recv_cmd[2];
                    recv_cmd[0]=0;
                    g_bootRecovery = TRUE;
    				g_BootRockusb = 0;
                    //powerOn();
                    change_cmd_for_recovery(&gBootInfo , recv_cmd);  
                    RkPrintf (" %d %d\n",TimeOutBase, RkldTimerGetTick());
                    start_linux(&gBootInfo);
                    //如果引导失败，只能通过usb 修复，把UsbConnected标记置1
                    UsbConnected = 1;
                }
            }
            else
            {
                if((RkldTimerGetTick() - TimeOutBase > (3*1000*1000)))
                {
                    TimeOutBase = RkldTimerGetTick(); 
                    PRINT_E("Usb re Boot. %d\n",RkldTimerGetTick());
                    UsbBoot();
                    power_on_timeout++;
                }
            }
        }
        else if(UsbConnected == 1)//&& power_on == 1
        {
            UsbConnected = 2;
            power_on = 2;
	        RkPrintf (" %d UsbConnected\n", RkldTimerGetTick());
            TimeOutBase = RkldTimerGetTick();
        }
        else if(power_on >= 1)
        {
    	    if((RkldTimerGetTick() - TimeOutBase > (3000*1000)))
    	    {
    	        TimeOutBase=RkldTimerGetTick();
                power_on_timeout++;
    	    }
        }
        
        if(power_on_timeout > 40) // 12s 超时 断电
        {
            power_on_timeout = 0;
            powerOff();
            RkPrintf ("power off\n");
        }
        
        if(UsbConnected && UsbBusReset > 5)
        {
            //PRINT_E("Usb re Boot. %d\n",RkldTimerGetTick());
            UsbBoot();
        }
        
        if(FWSetResetFlag==1)
        {
            //RkPrintf ("Switch to MSC!!\n");
            FWSetResetFlag = 0;
            break;
        }
        else if(FWSetResetFlag==2)
        {
            FWSetResetFlag = 0;
            powerOff();
        }
        else if(FWSetResetFlag==3) //reboot 2 maskrom
        {
            ISetLoaderFlag(0xEF08A53C);
            FWSetResetFlag = 0;
            SoftReset();
        }
        else if(FWSetResetFlag==4) //reboot 2 maskrom
        {
            FWSetResetFlag = 0;
            while(GetVbus());
            SoftReset();
        }
        else if(FWSetResetFlag==0xFF)
        {
            FWSetResetFlag = 0;
            SoftReset();
        }
        else if(FWSetResetFlag == 0x10)
        {
            FWSetResetFlag = 0;
            RKLockLoader();
        }
#else
        if(FWSetResetFlag==0xFF)
        {
            FWSetResetFlag = 0;
            SoftReset();
        }
#endif
        SysLowFormatCheck();
    }
    
#ifdef LINUX_LOADER
    Switch2MSC(); //切换到msc 模式拷贝demo文件
#endif
}
/***************************************************************************
函数描述:从USB引导
入口参数:
出口参数:
调用函数:
***************************************************************************/
void UsbBoot(void)
{
#ifdef ENUM_EN
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;
#endif
    /***************************************************************************
    USB初始化
    ***************************************************************************/
    bulkBuf[0]=&usbXferBuf[0];
    bulkBuf[1]=&usbXferBuf[USB_XFER_BUF_SIZE];
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
#ifdef ENUM_EN
    //UsbPhyReset();
    UsbPhyReset();
    OtgReg->Device.dctl |= 0x02;           //soft disconnect
    DRVDelayUs(500*1000);    //delay 500ms
    UdcInit();
    OtgReg->Device.dctl &= ~0x02;          //soft connect
#endif    
    EnableOtgIntr();
}

