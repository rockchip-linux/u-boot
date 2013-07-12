/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  spi.h
Desc    :  定义spi的主从寄存器结构体\寄存器位的宏定义\接口函数

Author  : lhh
Date    : 2008-12-16
Modified:
Revision:           1.00
$Log: spi.h,v $
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
#ifndef _DRIVER_SPI_H_
#define _DRIVER_SPI_H_

//SPIM_DMACR SPIS_DMACR
#define TRANSMIT_DMA_ENABLE         (1<<1)
#define RECEIVE_DMA_ENABLE          (1)

#if(PALTFORM==RK28XX)

//SPIM_CTRLR0  SPIS_CTRLR0
#define TRANSMIT_RECEIVE            (0)
#define TRANSMIT_ONLY               (1<<8)
#define RECEIVE_ONLY                (2<<8)
#define SPIM_E2PROM_READ            (3<<8)

#define SPIM_CTRLR0_CONFIG0          0xC7
#define SPIM_CTRLR0_CONFIG1          0xD8


///SPIM_SR  SPIS_SR
#define RECEIVE_FIFO_FULL           (1<<4)
#define RECEIVE_FIFO_NOT_EMPTY_MASK (1<<3)
#define RECEIVE_FIFO_NOT_EMPTY      (1<<3)
#define TRANSMIT_FIFO_EMPTY         (1<<2)

#define TRANSMIT_FIFO_NOT_FULL_MASK (1<<1)
#define TRANSMIT_FIFO_NOT_FULL      (1<<1)
#define SPI_BUSY_FLAG               (1)


#define SPI0_RXDR    SPIM_DR0
#define SPI0_TXDR    SPIM_DR0       

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


#else//if(PALTFORM==RK29XX)

#define TRANSMIT_RECEIVE            (0<<18)
#define TRANSMIT_ONLY               (1<<18)
#define RECEIVE_ONLY                (2<<18)
#define SPIM_E2PROM_READ            TRANSMIT_RECEIVE

//SPIM_CTRLR0  SPIS_CTRLR0

#define TRANSMIT_RECEIVE            (0<<18)
#define TRANSMIT_ONLY               (1<<18)
#define RECEIVE_ONLY                (2<<18)

#define SPIM_CTRLR0_CONFIG0         0x24C1  
#define SPIM_CTRLR0_CONFIG1         0x24C1 // need  test


///SPIM_SR  SPIS_SR
#define RECEIVE_FIFO_FULL           (1<<4)
#define RECEIVE_FIFO_NOT_EMPTY_MASK (1<<3)
#define RECEIVE_FIFO_NOT_EMPTY      (0<<3)
#define TRANSMIT_FIFO_EMPTY         (1<<2)
#define TRANSMIT_FIFO_NOT_FULL_MASK (1<<1)
#define TRANSMIT_FIFO_NOT_FULL      (0<<1)
#define SPI_BUSY_FLAG               (1)

#define SPI0_RXDR    SPIM_RXDR[0]
#define SPI0_TXDR    SPIM_TXDR[0] 

//SPI MASTER Registers
typedef volatile struct tagSPI_MASTER_STRUCT
{
    uint32 SPIM_CTRLR0;
    uint32 SPIM_CTRLR1;
    uint32 SPIM_SPIENR;
    uint32 SPIM_SER;
    uint32 SPIM_BAUDR; //0x10
    uint32 SPIM_TXFTLR;
    uint32 SPIM_RXFTLR;
    uint32 SPIM_TXFLR;
    uint32 SPIM_RXFLR; //0x20
    uint32 SPIM_SR;
    uint32 SPIM_IPR; // 29 add
    uint32 SPIM_IMR;
    uint32 SPIM_ISR; //0x30
    uint32 SPIM_RISR;
    uint32 SPIM_ICR;
    uint32 SPIM_DMACR;
    uint32 SPIM_DMATDLR; //0x40
    uint32 SPIM_DMARDLR;
    uint32 reserved[(0x400-0x48)/4];
    uint32 SPIM_TXDR[0x100];
    uint32 SPIM_RXDR[0x100];
}SPIM_REG,*pSPIM_REG;

#endif

#endif
#endif
