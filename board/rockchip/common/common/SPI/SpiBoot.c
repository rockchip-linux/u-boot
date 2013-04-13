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
#include    "../../armlinux/config.h"
#include    "SpiBoot.h"

#ifdef      DRIVERS_SPI
#define     SPI_BLOCK_SIZE          512
#define     SPI_LBA_PART_OFFSET     1024

void SpiReadFlashInfo(void *buf)
{
    pFLASH_INFO pInfo=(pFLASH_INFO)buf;
    memset((uint8*)buf,0,512);
    pInfo->BlockSize = SPI_BLOCK_SIZE;
    pInfo->ECCBits = 0;
    pInfo->FlashSize = 0x200000; 
    pInfo->PageSize = 4;
    pInfo->AccessTime = 40;
    pInfo->ManufacturerName=0;
    pInfo->FlashMask=1;
}

/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:ChipSel, blkIndex=起始block号,  nblk=blk数 ，mod: 0 为普通擦除； 1为强制擦除
出口参数:0 为没有坏块，1为有坏块
调用函数:无
***************************************************************************/
uint32 SpiBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod)
{
	return OK;
}
#if 0
uint32 SpiBootWritePBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    uint32 *pDataBuf = pbuf;
    uint16 len;
    if(PBA + nSec >= 256) //超出了不写
        return;
    for (len=0; len<nSec; len++)
    {
        ftl_memcpy( Data,pDataBuf+(len)*132, 512);
        SPIFlashWrite(PBA*512, Data, 512);
    }
    return 0; 
}

uint32 SpiBootReadPBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    uint16 len;
    uint32 *pDataBuf = pbuf;
    PBA &= 0xFF;


    memset(SpareBuf,0xFF,SPARE_LEN);
    for (len=0; len<nSec; len++)
    {
        SPIFlashRead(PBA*512, Data, 512);
        ftl_memcpy(pDataBuf+(len)*132, Data, 512);
        ftl_memcpy(pDataBuf+(len)*132+128, SpareBuf, 16);
    }
    return 0;
}
#else
uint32 SpiBootWritePBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    uint32 *pDataBuf = pbuf;
    uint16 len,i;
    if(PBA + nSec >= SPI_BLOCK_SIZE*2) //超出了不写
        return;
    //RkPrintf("SpiBootWritePBA(%x,%x,%x,%x)\n", ChipSel, PBA , pbuf, nSec );
    for (len=0; len<nSec; len+=4)
    {
        for (i=0; i<4; i++)
        {
            //RkPrintf("SPIFlashWrite len %x nSec %x  i %x \n", len,nSec,i);
        	SPIFlashWrite(((len+PBA)*2+i)*512, pDataBuf+(len+i)*132, 512);
        }
    }
    return 0; 
}


uint32 SpiBootReadPBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    uint16 len,i;
    uint32 *pDataBuf = pbuf;
    PBA &= (SPI_BLOCK_SIZE*2-1);

    memset(SpareBuf,0xFF,SPARE_LEN);

    for (len=0; len<nSec; len+=4)
    {
        for (i=0; i<4; i++)
        {
        	SPIFlashRead(((len+PBA)*2+i)*512, pDataBuf+(len+i)*132, 512);//读2K要跳过2K
            ftl_memcpy(pDataBuf+(len+i)*132+128, SpareBuf+(i)*4, 16);
        }
    }
   
    return 0;
}

uint32 SpiBootReadLBA(uint8 ChipSel,  uint32 LBA ,void *pbuf, uint16 nSec)
{
    SPIFlashRead((LBA+SPI_LBA_PART_OFFSET)*512, pbuf,nSec*512);
    return(0);
}

uint32 SpiBootWriteLBA(uint8 ChipSel,  uint32 LBA, void *pbuf , uint16 nSec  ,uint16 mode)
{
    SPIFlashWrite((LBA+SPI_LBA_PART_OFFSET)*512, pbuf,nSec*512);
    return(0);
}
#endif
#endif
