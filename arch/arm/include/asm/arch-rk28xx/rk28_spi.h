/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  spi.h
Desc    :  定义spi的主从寄存器结构体\寄存器位的宏定义\接口函数

Author  : lhh
Date    : 2008-12-16
Modified:
Revision:           1.00
*********************************************************************/
#ifdef DRIVERS_SPI
#ifndef _DRIVER_SPI_H_
#define _DRIVER_SPI_H_

//SPIM_CTRLR0  SPIS_CTRLR0
#define NORMAL_MODE_OPERATION       (0)
#define TEST_MODE_OPERATION         (1<<11)
#define TRANSMIT_RECEIVE            (0)
#define TRANSMIT_ONLY               (1<<8)
#define RECEIVE_ONLY                (2<<8)
#define SERIAL_CLOCK_POLARITY_LOW   (0)
#define SERIAL_CLOCK_POLARITY_HIGH  (1<<7)
#define SERIAL_CLOCK_PHASE_MIDDLE   (0)
#define SERIAL_CLOCK_PHASE_START    (1<<6)
#define MOTOROLA_SPI                (0)
#define TEXAS_INSTRUMENTS_SSP       (1<<4)
#define NATIONAL_SEMI_MICROWIRE     (2<<4)

///SPIM_SR  SPIS_SR
#define TRANSMIT_FIFO_NOT_FULL      (1<<1)
#define RECEIVE_FIFO_NOT_EMPTY      (1<<3)
#define SPI_BUSY_FLAG               (1)


//SPIM_DMACR SPIS_DMACR
#define TRANSMIT_DMA_ENABLE         (1<<1)
#define RECEIVE_DMA_ENABLE          (1)


//SPI MASTER Registers
typedef volatile struct tagSPI_MASTER_STRUCT
{
    uint32 SPIM_CTRLR0;
    uint32 SPIM_CTRLR1;
    uint32 SPIM_SPIENR;
    uint32 SPIM_MWCR;
    uint32 SPIM_SER;
    uint32 SPIM_BAUDR;
    uint32 SPIM_TXFTLR;
    uint32 SPIM_RXFTLR;
    uint32 SPIM_TXFLR;
    uint32 SPIM_RXFLR;
    uint32 SPIM_SR;
    uint32 SPIM_IMR;
    uint32 SPIM_ISR;
    uint32 SPIM_RISR;
    uint32 SPIM_TXOICR;
    uint32 SPIM_RXOICR;
    uint32 SPIM_RXUICR;
    uint32 SPIM_MSTICR;
    uint32 SPIM_ICR;
    uint32 SPIM_DMACR;
    uint32 SPIM_DMATDLR;
    uint32 SPIM_DMARDLR;
    uint32 SPIM_IDR;
    uint32 SPIM_COMP_VERSION;
    uint32 SPIM_DR0;
    uint32 SPIM_DR1;
    uint32 SPIM_DR2;
    uint32 SPIM_DR3;
    uint32 SPIM_DR4;
    uint32 SPIM_DR5;
    uint32 SPIM_DR6;
    uint32 SPIM_DR7;
    uint32 SPIM_DR8;
    uint32 SPIM_DR9;
    uint32 SPIM_DR10;
    uint32 SPIM_DR11;
    uint32 SPIM_DR12;
    uint32 SPIM_DR13;
    uint32 SPIM_DR14;
    uint32 SPIM_DR15;
}SPIM_REG,*pSPIM_REG;

//SPI SLAVE Registers
typedef volatile struct tagSPI_SLAVE_STRUCT
{
    uint32 SPIS_CTRLR0;
    uint32 SPIS_CTRLR1;
    uint32 SPIS_SPIENR;
    uint32 SPIS_MWCR;
    uint32 RESERVED1;
    uint32 RESERVED2;
    uint32 SPIS_TXFTLR;
    uint32 SPIS_RXFTLR;
    uint32 SPIS_TXFLR;
    uint32 SPIS_RXFLR;
    uint32 SPIS_SR;
    uint32 SPIS_IMR;
    uint32 SPIS_ISR;
    uint32 SPIS_RISR;
    uint32 SPIS_TXOICR;
    uint32 SPIS_RXOICR;
    uint32 SPIS_RXUICR;
    uint32 SPIS_MSTICR;
    uint32 SPIS_ICR;
    uint32 SPIS_DMACR;
    uint32 SPIS_DMATDLR;
    uint32 SPIS_DMARDLR;
    uint32 SPIS_IDR;
    uint32 SPIS_COMP_VERSION;
    uint32 SPIS_DR0;
    uint32 SPIS_DR1;
    uint32 SPIS_DR2;
    uint32 SPIS_DR3;
    uint32 SPIS_DR4;
    uint32 SPIS_DR5;
    uint32 SPIS_DR6;
    uint32 SPIS_DR7;
    uint32 SPIS_DR8;
    uint32 SPIS_DR9;
    uint32 SPIS_DR10;
    uint32 SPIS_DR11;
    uint32 SPIS_DR12;
    uint32 SPIS_DR13;
    uint32 SPIS_DR14;
    uint32 SPIS_DR15;
}SPIS_REG,*pSPIS_REG;

typedef enum
{
    SPI_TRANSMIT = 0,
    SPI_RECEIVE
}SPI_DMA_MODE;

void SPIMSetDma(SPI_DMA_MODE dmaMode, uint8 enableOrDis);
int8 SPIMInit(uint16 baudRate,  uint8 slaveNumb, uint8 transferMode, uint8 serialClockPhase);
void SPIMDeinit(void);
int8 SPIMRead(void *pdata, uint8 dataWidth, uint32 length);
int8 SPIMWrite(void *pdata, uint8 dataWidth, uint32 length);
void SPISSetDma(SPI_DMA_MODE dmaMode, uint8 enableOrDis);
int8 SPISInit(uint8 transferMode, uint8 serialClockPhase);
void SPISDeinit(void);
int8 SPISRead(void *pdata, uint8 dataWidth, uint32 length);
int8 SPISWrite(void *pdata, uint8 dataWidth, uint32 length);


#endif
#endif
