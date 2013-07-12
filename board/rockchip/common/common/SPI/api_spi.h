/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  api_spi.h
Desc    :  应用spi的接口参数

Author  : lhh
Date    : 2008-12-6
Modified:
Revision:           1.00
$Log: api_spi.h,v $
Revision 1.1.2.1  2011/01/18 07:24:43  Administrator
*** empty log message ***

Revision 1.1.2.1  2010/10/18 14:07:15  Administrator
*** empty log message ***

Revision 1.1.1.1  2010/06/30 01:13:30  FZF
no message

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

*********************************************************************/
#ifdef DRIVERS_SPI
#ifndef __API_SPI_H__
#define __API_SPI_H__

typedef enum
{
    SPI_TRANSMIT = 0,
    SPI_RECEIVE
}SPI_DMA_MODE;

typedef enum SPIM_ch
{
    SPIM_CH0 = 1,
    SPIM_CH1
}eSPIM_ch_t;

typedef enum SPIM_TRANSFER_MODE
{
    SPIM_TRANSMIT_RECEIVE = 0,
    SPIM_TRANSMIT_ONLY,
    SPIM_RECEIVE_ONLY,
    SPIM_EEPROM_READ
}eSPIM_TRANSFER_MODE_t;

typedef enum SPIM_PHASE
{
    SPIM_MIDDLE_FIRST_DATA = 0,
    SPIM_START_FIRST_DATA
}eSPIM_PHASE_t;

typedef enum SPIM_POLARITY
{
    SPIM_SERIAL_CLOCK_LOW = 0,
    SPIM_SERIAL_CLOCK_HIGH
}eSPIM_POLARITY_t;

typedef enum SPIM_DMA
{
    SPIM_DMA_DISABLE = 0,
    SPIM_DMA_ENABLE
}eSPIM_DMA_t;


typedef enum
{
    DATA_WIDTH4 = 0,
    DATA_WIDTH5,
    DATA_WIDTH6,
    DATA_WIDTH7,
    DATA_WIDTH8,
    DATA_WIDTH9,
    DATA_WIDTH10,
    DATA_WIDTH11,
    DATA_WIDTH12,
    DATA_WIDTH13,
    DATA_WIDTH14,
    DATA_WIDTH15,
    DATA_WIDTH16    
}SPI_DATA_WIDTH; 

#undef  EXT
#ifdef  IN_API_DRIVER_SPI
#define EXT
pFunc g_SPIVectorTX=0;
pFunc g_SPIVectorRX=0;
#else
#define EXT extern    
extern  pFunc g_SPIVectorTX;
extern  pFunc g_SPIVectorRX;
#endif

EXT uint32 g_spimFreq;


void SPIMUpdateAllApbFreq(uint32 APBnKHz);
int32 SPIMWriteBuff(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length);
void SPIMSetDma(SPI_DMA_MODE dmaMode, eSPIM_DMA_t enableOrDis, SPI_DATA_WIDTH dataWidth, uint32 length);
int32 SPIME2promRead(void *pCommand, uint32 commandLength, void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length);
int32 SPIMInit(uint16 baudRate,  eSPIM_ch_t slaveNumb, eSPIM_TRANSFER_MODE_t transferMode, eSPIM_PHASE_t serialClockPhase, eSPIM_POLARITY_t  polarity);
void SPIMDeinit(void);
int32 SPIMRead(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length);
int32 SPIMWrite(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length);
void SPISSetDma(SPI_DMA_MODE dmaMode, uint8 enableOrDis, SPI_DATA_WIDTH dataWidth, uint32 length);
int32 SPISInit(eSPIM_TRANSFER_MODE_t transferMode, eSPIM_PHASE_t serialClockPhase, uint8 spiImr);
void SPISDeinit(void);
int32 SPISRead(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length);
int32 SPISWrite(void *pdata, SPI_DATA_WIDTH dataWidth, uint32 length);
void SPISIrqHander(void);


#endif
#endif
