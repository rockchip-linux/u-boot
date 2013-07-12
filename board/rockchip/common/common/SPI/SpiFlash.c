/*
********************************************************************************
*                   Copyright (c) 2008,CHENFEN
*                         All rights reserved.
*
* File Name：  SpiFlash.c
* 
* Description: 
*
* History:      <author>          <time>        <version>       
*             chenfen          2008-11-10          1.0
*    desc:    ORG.
********************************************************************************
*/

#define     IN_SPIFLASH
#include	"../../armlinux/config.h"
#include    "SpiFlash.h"
#include    "spi.h"
//#include    "gpio.h"


#ifdef   OTP_DATA_ENABLE
uint32 OTPFlashRead(uint32 addr, void *pData, uint32 len) 
{
    uint32 ret;
    ret = OtpDataLoad(0,addr>>9,pData,(len+511)>>9);
    return ret;
}

uint32 OTPFlashWrite(uint32 addr, void *pData, uint32 len) 
{
    uint32 ret;
    ret = OtpDataStore(0,addr>>9,pData,(len+511)>>9);
    ret = OtpDataStore(1,addr>>9,pData,(len+511)>>9);
    return ret;
}
#endif

#ifdef      DRIVERS_SPI
#define     SpiCsPin  GPIOPortB_Pin4

uint32 IsSpiFlashInit = -1;
uint32 Spi_BytePerSec = 4096;
uint32 Spi_BytePerPage = 256;
uint32 Spi_BytePerBlk = 1024*64;
uint32 Spi_TotBytePerChip = 1024*1024;
uint32 Spi_sectorErserCmd ;
uint32 Spi_sectorReadCmd ;
uint32 Spi_sectorWriteCmd ;
uint32 Spi_AddrBytes = 3;
uint32 Spi_ChipId = -1;
uint8 SPiFlashId[6];

#define SpiCs()
#define SpiDeCs()

/*
--------------------------------------------------------------------------------
  Function name : SPIWaitBusy
  Author        : chenfen
  Description   : 
                  
  Input         : 
  Return        : 

  History:     <author>         <time>         <version>       
             chenfen        2009/1/10         Ver1.0
  desc:         ORG
--------------------------------------------------------------------------------
*/
uint32 SPIWaitTFEmpty(void)
{
    uint8 status;
    uint32 i;
    uint32 result;
    pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR;
    for (i=0; i<50000; i++)
    {
        if((g_spiReg->SPIM_SR & TRANSMIT_FIFO_EMPTY) == TRANSMIT_FIFO_EMPTY)
        {
            result= OK;
            break;
        }
        result=ERROR;
    }
    return result;
}

uint32 SPIWaitNotBusy(void)
{
    uint8 status;
    uint32 i;
    uint32 result;
    pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR;
    for (i=0; i<50000; i++)
    {
        if((g_spiReg->SPIM_SR & SPI_BUSY_FLAG) != SPI_BUSY_FLAG)
        {
            result= OK;
            break;
        }
        result=ERROR;
    }
    return result;
}

uint32 SPIWriteWaitBusy(void)
{
    uint32 result;
    SPIWaitTFEmpty();
    result = SPIWaitNotBusy();
    return result;
}

#if(PALTFORM==RK28XX)
uint8 SPISendCmd(uint32 *pCmd, uint32 cmdLen, uint8 *pData, uint32 dataLen, uint32 dummy, uint32 mode)
{
    uint8 result=ERROR;
    uint32 timeout;
    uint32 i=0;
    uint8 *pdataRead;

    pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR;
    pdataRead = (uint8 *) pData;
    if(dataLen)
    {
        g_spiReg->SPIM_CTRLR1 = dataLen -1;
    }
    g_spiReg->SPIM_CTRLR0 = (mode | SPIM_CTRLR0_CONFIG0); //TRANSMIT_RECEIVE 8bit data frame size, CPOL=1,CPHA=1 mode |
    SpiCs();
    g_spiReg->SPIM_SPIENR = 1;	
    for(i=0; i<cmdLen; i++)
    {
        g_spiReg->SPIM_DR0 = pCmd[i];
    }
    g_spiReg->SPIM_SER = 1;

    for (i=0; i<500000; i++)
    {
        if((g_spiReg->SPIM_SR & TRANSMIT_FIFO_EMPTY) == TRANSMIT_FIFO_EMPTY)
        {	        	
            break;
        }
    }

    while(dataLen)
    {
        for(timeout=0; timeout<2000; timeout++)  //现在SPI Flash速度是500K，发一个数据回来一个数据，因此需要经过8bit的时间，也就是16us，timeout比16us大就行
        {
	        if ((g_spiReg->SPIM_SR & RECEIVE_FIFO_NOT_EMPTY_MASK) == RECEIVE_FIFO_NOT_EMPTY)
	        {                

	            *pdataRead++ = (g_spiReg->SPIM_DR0) & 0xFF;
	            dataLen--;
	            if(dataLen==0)
	            {
	                result=OK;
	                break;
	            }
	        }
		}
        if(timeout == 2000)
        {
            result = ERROR;
            break;
        }
    }    
    for (i=0; i<500000; i++)
    {	       
        if((g_spiReg->SPIM_SR & SPI_BUSY_FLAG) != SPI_BUSY_FLAG)
        {
            result= OK;
            break;
        }
        result=ERROR;
    }
    g_spiReg->SPIM_SER = 0;
    g_spiReg->SPIM_SPIENR = 0;
    SpiDeCs();
    DRVDelayUs(1); 
    return result;
}
#else
uint8 SPISendCmd(uint32 *pCmd, uint32 cmdLen, uint8 *pData, uint32 dataLen, uint32 dummy, uint32 mode)
{
    uint32 result=OK;
    uint32 timeout = 0;
    uint32 tmp;
    uint32 len;
    uint32 i;
    
    pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR;
    g_spiReg->SPIM_CTRLR0 = (TRANSMIT_RECEIVE | SPIM_CTRLR0_CONFIG0); // 8bit data frame size, CPOL=1,CPHA=1
    len = 0;
    //dataLen += dummy;
    g_spiReg->SPIM_SPIENR = 1;
    g_spiReg->SPIM_SER = 1;
    for(i=0; i<cmdLen; i++)
    {
        g_spiReg->SPIM_TXDR[0] = pCmd[i];
    }
    SPIWaitTFEmpty();
    SPIWaitNotBusy(); 
    if(dataLen)
    {
        g_spiReg->SPIM_SPIENR = 0;
        g_spiReg->SPIM_CTRLR0 = (RECEIVE_ONLY | SPIM_CTRLR0_CONFIG0); // 8bit data frame size, CPOL=1,CPHA=1
        g_spiReg->SPIM_CTRLR1 = dataLen-1;
        g_spiReg->SPIM_SPIENR = 1;
        result=OK;
        while(len < dataLen)
        {
            if ((g_spiReg->SPIM_SR & RECEIVE_FIFO_NOT_EMPTY_MASK) == RECEIVE_FIFO_NOT_EMPTY)
            {
                timeout = 0;
                /*if((g_spiReg->SPIM_SR & RECEIVE_FIFO_FULL) == RECEIVE_FIFO_FULL)
                {
                    for(i=0;i<32;i++)
                    {
                        *pData++ = (uint8)((g_spiReg->SPIM_RXDR[0]) & 0xFF);
                    }
                    len+=32;
                }
                else
                {*/
                    *pData++ = (uint8)((g_spiReg->SPIM_RXDR[0]) & 0xFF);
                    len++;
                //}
            }
            else
            {
                if(timeout++ >= 8000)
                {
                    result = ERROR;
                    break;
                }
            }
        }
    }
    g_spiReg->SPIM_SPIENR = 0;
    g_spiReg->SPIM_SER = 0;
    Delay100cyc(18);     //大于100ns  7.2us*18=130us
    return result;
}

#endif

uint32 SpiFlashWaitBusy(void)
{
	uint32 cmd[1];
	uint8 status=0xff;
	uint32 i;
	for (i=0; i<5000000; i++)
	{
		DRVDelayUs(1);
		cmd[0] = 0x05;
		SPISendCmd(cmd, 1, &status, 1,0,SPIM_E2PROM_READ);   
		if ((status & 0x01) == 0)		
		    return OK;
	}
	return ERROR;
}

uint32  SPIWriteData(uint32 *pCmd, uint32 cmdLen, void *pData, uint32 dataLen)
{
    uint8 result=ERROR;
    uint32 timeout;
    uint32 i=0;
    uint8 *pdataWrite;
    pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR; 
    pdataWrite= (uint8 *) pData;
    g_spiReg->SPIM_CTRLR0 = (TRANSMIT_ONLY | SPIM_CTRLR0_CONFIG0); // 8bit data frame size, CPOL=1,CPHA=1
    SpiCs();
    g_spiReg->SPIM_SPIENR = 1;	
    g_spiReg->SPIM_SER = 1;
    
    for(i=0; i<cmdLen; i++)
    {
        g_spiReg->SPI0_TXDR = pCmd[i];
    }	   	
    timeout = 0;

    while(dataLen)
    {        
        if ((g_spiReg->SPIM_SR & TRANSMIT_FIFO_NOT_FULL_MASK) == TRANSMIT_FIFO_NOT_FULL)
        {
            g_spiReg->SPI0_TXDR = *pdataWrite++;
            dataLen--;  

            if(dataLen==0)
            {
                result=OK;
            }
            timeout = 0;
        }
        else
        {
            timeout++;
            if(timeout > 500000)
            {
                result=ERROR;
                break;
            }
        }
    }
    if(OK!=SPIWriteWaitBusy())
    {
        SpiDeCs();
        g_spiReg->SPIM_SER = 0;
        g_spiReg->SPIM_SPIENR = 0;
        return ERROR;	
    }
    //DRVDelayUs(8);
    SpiDeCs();
    g_spiReg->SPIM_SER = 0;
    g_spiReg->SPIM_SPIENR = 0;
    if (OK!=SpiFlashWaitBusy())
    {
        result=ERROR;
    }
    return result;
}


/*
--------------------------------------------------------------------------------
  Function name : SPIFlashRead
  Author        : chenfen
  Description   : 
                  
  Input         : 
  Return        : 

  History:     <author>         <time>         <version>       
             chenfen        2009/1/10         Ver1.0
  desc:         ORG
  int32 SPIMRead(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length)
--------------------------------------------------------------------------------
*/
uint32 SPIFlashRead(uint32 addr, void *pData, uint32 len) 
{
    uint32 cmd[5];
    uint32 ReadLen;
    uint32 ret = OK;
    SPIFlashInit(SPI_MASTER_BASE_ADDR);
    //RkPrintf("SPIFlashRead(%x,%x,%x)\n", addr , pData, len );
    while (len > 0)
    {
        ReadLen = (len > 32768)? 32768 : len;     
        cmd[0]= Spi_sectorReadCmd;
        if(Spi_AddrBytes==4)
        {
            cmd[1] = (addr>>24 )& 0xff;
            cmd[2] = (addr>>16 )& 0xff;
            cmd[3] = (addr>>8 ) & 0xff;
            cmd[4] = addr & 0xff;
        }
        else
        {
            cmd[1] = (addr>>16 )& 0xff;
            cmd[2] = (addr>>8 ) & 0xff;
            cmd[3] = addr & 0xff;
        }

        ret=SPISendCmd(cmd, Spi_AddrBytes+1, pData, ReadLen,0,SPIM_E2PROM_READ);//

        (uint8 *)pData += ReadLen;
        len -= ReadLen;
        addr += ReadLen;
    }

    return ret;
}

/*
--------------------------------------------------------------------------------
  Function name : uint32 SPIFlashErase(uint32 Type,  uint32 addr)
  Author        : chenfen
  Description   : 
                  
  Input         : 
  Return        : 

  History:     <author>         <time>         <version>       
             chenfen        2009/1/10         Ver1.0
  desc:         ORG
--------------------------------------------------------------------------------
*/
uint32 SPIFlashErase(uint32 Type,  uint32 addr) 
{
    uint32 cmd[5];
    uint32 ret = OK;
    cmd[0] = 0x06;    //write enable
    if(OK != SPISendCmd(cmd, 1, NULL, 0,0,TRANSMIT_ONLY))
        return ERROR;
    
    cmd[0]  = Spi_sectorErserCmd;
    if(Spi_AddrBytes==4)
    {
        cmd[1] = (addr>>24 )& 0xff;
        cmd[2] = (addr>>16 )& 0xff;
        cmd[3] = (addr>>8 ) & 0xff;
        cmd[4] = addr & 0xff;
    }
    else
    {
        cmd[1] = (addr>>16 )& 0xff;
        cmd[2] = (addr>>8 ) & 0xff;
        cmd[3] = addr & 0xff;
    }


    if(OK != SPISendCmd(cmd, Spi_AddrBytes+1, NULL, 0,0,TRANSMIT_ONLY))
        return ERROR;

    if (OK!=SpiFlashWaitBusy())
    {
        ret = ERROR;
    }
    
    cmd[0] = 0x04;    //write disable
    if(OK != SPISendCmd(cmd, 1, NULL, 0,0,TRANSMIT_ONLY))
        return ERROR;

    return OK;
    
}


uint32 Chiperase(void)
{
    uint32 cmd[4]; 
    uint32 ret;
    cmd[0] = 0x06;    //write enable
    SPISendCmd(cmd, 1, NULL, 0,0,TRANSMIT_ONLY); 	
    cmd[0] = 0xc7;
    SPISendCmd(cmd, 1, NULL, 0, 0, TRANSMIT_ONLY);
    if (OK!=SpiFlashWaitBusy())
    {
        ret=ERROR;
    }
    cmd[0] = 0x04;    //write disable
    SPISendCmd(cmd, 1, NULL, 0,0,TRANSMIT_ONLY);
}



/*
--------------------------------------------------------------------------------
  Function name : SPIFlashReadID
  Author        : chenfen
  Description   : 
                  
  Input         : 
  Return        : 

  History:     <author>         <time>         <version>       
             chenfen        2009/1/10         Ver1.0
  desc:         ORG
--------------------------------------------------------------------------------
*/
void SPIFlashReadID(uint8 ChipSel,void * buf)
{
    uint8 * pdata = buf;
    uint32 cmd[4]; 
    cmd[0] = READ_AD;
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    
   // pdata[3] = 'S';
   // pdata[4] = 'P';

#if(PALTFORM==RK28XX)
    SPISendCmd(cmd, 4, pdata, 3, 0, SPIM_E2PROM_READ);
#else
    SPISendCmd(cmd, 1, pdata, 5, 0, SPIM_E2PROM_READ);
    //RkPrintf("Spi_ChipId = %x %x %x %x %x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4]);
#endif
}

void SPIFlashProbe(uint32 id)
{
    Spi_BytePerPage = 256;
    Spi_BytePerSec = 4096;
    Spi_BytePerBlk = 1024*64;
    Spi_TotBytePerChip = 1024*1024;
    Spi_sectorErserCmd = 0x20;
    Spi_sectorReadCmd = READ_DATA;
    Spi_sectorWriteCmd = BYTE_WRITE ;

    if((id&0xFFFF00) == 0x1C2000)
    {
        Spi_sectorErserCmd = 0xD8;
    }

    if((id&0xFF0000) == 0x010000)
    {
	    pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR ;
        Spi_sectorErserCmd = 0xDC; //D8
        Spi_BytePerSec = 1024*64;
        if(SPiFlashId[4] == 0)
            Spi_BytePerSec = 1024*256;
    	g_spiReg->SPIM_BAUDR = 2; //75/2 = 38Mhz
        Spi_AddrBytes = 4;
        //Spi_BytePerPage = 512;
        Spi_sectorReadCmd = BYTE4_READ;
        Spi_sectorWriteCmd = BYTE4_WRITE ;
    }
}
extern  void spiflashtest();
/*
--------------------------------------------------------------------------------
  Function name : SPIFlashInit
  Author        : chenfen
  Description   : 
                  
  Input         : 
  Return        : 

  History:     <author>         <time>         <version>       
             chenfen        2009/1/10         Ver1.0
  desc:         ORG
--------------------------------------------------------------------------------
*/
uint32 SPIFlashInit(uint32 chipSel)
{
	uint32 i;
	uint32 ret;
	pSPIM_REG g_spiReg = (pSPIM_REG) SPI_MASTER_BASE_ADDR ;
    if(IsSpiFlashInit == -1 || (Spi_BytePerPage & 127 !=0))
    {
#if(PALTFORM==RK28XX)
    	g_spiReg->SPIM_BAUDR = 24; 
#else
    	g_spiReg->SPIM_BAUDR = 6; 
#endif
        g_spiReg->SPIM_CTRLR0 = (SPIM_E2PROM_READ | SPIM_CTRLR0_CONFIG0); // 8bit data frame size, CPOL=1,CPHA=1
        SpiGpioInit();
        IsSpiFlashInit = 1;
        SPIFlashReadID(0,SPiFlashId);
        Spi_ChipId = SPiFlashId[0]<<16 | SPiFlashId[1]<<8 | SPiFlashId[2];
        RkPrintf("Spi_ChipId = %x\n",Spi_ChipId);
        SPIFlashProbe(Spi_ChipId);
        if(Spi_ChipId == 0x0 || Spi_ChipId == 0xFFFFFF)
            return -1;
            
        //TODO:校验SPI FLASH
        if(SPIFlashRead(0, gIdDataBuf, 2048)==0)
        {
            pIDSEC0 idSec0;
            idSec0=(pIDSEC0)(gIdDataBuf+0);
            if (idSec0->magic==0xfcdc8c3b)
            {
                return 0;
            }
        }
        return -1;
    }
    return 0;
}

void SPIFLSAH_EraseAuto(uint32 addr)
{
    uint32 BytePerSec = Spi_BytePerSec;
    if((Spi_ChipId&0xFFFF00) == 0x1C2000)
    {
        if(addr < 0x2000)
        {
            BytePerSec = 0x1000;
        }
        else if(addr < 0x4000)
        {
            BytePerSec = 0x2000;
        }
        else if(addr < 0x8000)
        {
            BytePerSec = 0x4000;
        }
        else 
        {
            BytePerSec = 0x8000;
            if(Spi_ChipId >= 0x1C2012)
            {
                if(addr >= 0x10000)
                {
                    BytePerSec = 0x10000;
                }
            }
        }
    }
    if((addr & (BytePerSec - 1)) == 0)
    {
        SPIFlashErase(BytePerSec,addr);
    }
}


/*
--------------------------------------------------------------------------------
  Function name : SPIFlashWrite
  Author        : chenfen
  Description   : 
                  
  Input         : 
  Return        : 

  History:     <author>         <time>         <version>       
             chenfen        2009/1/10         Ver1.0
  desc:         ORG
   SPIMWrite(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length)
--------------------------------------------------------------------------------
*/
uint32 SPIFlashWrite(uint32 addr, void *pData, uint32 len) 
{
    uint32 cmd[8];
    uint32 writeLen;
    uint32 ret=ERROR;
    SPIFlashInit(SPI_MASTER_BASE_ADDR);
    writeLen = Spi_BytePerPage - (addr % Spi_BytePerPage);
    writeLen = (len > writeLen)? writeLen : len;
    
    while (len > 0)      
    {
	    if (OK!=SpiFlashWaitBusy())
	    {
	        ret=ERROR;
	    }      
        SPIFLSAH_EraseAuto(addr);
	    if (OK!=SpiFlashWaitBusy())
	    {
	        ret=ERROR;
	    }      

        cmd[0] = WRITE_ENABLE;    //write enable
        SPISendCmd(cmd, 1, NULL, 0,0,TRANSMIT_ONLY); 

        cmd[0] = Spi_sectorWriteCmd;
        if(Spi_AddrBytes==4)
        {
            cmd[1] = (addr>>24 )& 0xff;
            cmd[2] = (addr>>16 )& 0xff;
            cmd[3] = (addr>>8 ) & 0xff;
            cmd[4] = addr & 0xff;
        }
        else
        {
            cmd[1] = (addr>>16 )& 0xff;
            cmd[2] = (addr>>8 ) & 0xff;
            cmd[3] = addr & 0xff;
        }
        
        if(OK!=SPIWriteData(cmd, Spi_AddrBytes+1, (uint8*)pData, writeLen))
        {
            ret=ERROR;
            break;
        }	
        (uint8 *)pData +=writeLen;
        addr = (addr+Spi_BytePerPage) & (~(Spi_BytePerPage-1));      
        len -= writeLen;
        writeLen = (len > Spi_BytePerPage)? Spi_BytePerPage : len;

    }
    if (OK!=SpiFlashWaitBusy())
    {
        ret=ERROR;
    }      

    cmd[0] = WRITE_DISABLE;    //write disable
    SPISendCmd(cmd, 1, NULL, 0,0,TRANSMIT_ONLY);
    return ret;
}
#endif
#if 0
#define TEST_LEN 128
uint32 write[TEST_LEN];
uint32 read[TEST_LEN];
void spiflashtest()
{
    uint32 i;
    for(i=0;i<TEST_LEN;i++)
    {
        write[i] = i;
        read[i] = i+1;
    }
    SPIFlashWrite(0, write, TEST_LEN*4);
    SPIFlashRead(0, read,TEST_LEN*4);
    for(i=0;i<TEST_LEN;i++)
    {
        if(write[i]!= read[i])
            while(1);
    }
}
#endif

