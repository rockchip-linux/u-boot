/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  i2c.h
Desc    :  定义i2c的寄存器结构体\寄存器位的宏定义\接口函数

Author  : lhh
Date    : 2008-12-04
Modified:
Revision:           1.00
*********************************************************************/
#ifdef DRIVERS_I2C
#ifndef _DRIVER_IIC_H_
#define _DRIVER_IIC_H_

//I2C_IER
#define I2C_ARBITR_LOSE_ENABLE   (1<<7)  //Arbitration lose interrupt

//I2C_ISR
#define I2C_ARBITR_LOSE_STATUS   (1<<7)  //Arbitration lose STATUS
#define I2C_RECE_INT_MACKP       (1<<1) //Master ACK period interrupt status bit
#define I2C_RECE_INT_MACK        (1)     //Master receives ACK interrupt status bit


//I2C_LSR
#define I2C_LSR_RECE_NACK        (1<<1)

//I2C_LCMR
#define I2C_LCMR_RESUME          (1<<2)
#define I2C_LCMR_STOP            (1<<1)
#define I2C_LCMR_START           (1)

//I2C_CONR
#define I2C_CON_ACK              (0)
#define I2C_CON_NACK             (1<<4)
#define I2C_MASTER_TRAN_MODE     (1<<3)
#define I2C_MASTER_RECE_MODE     (0)
#define I2C_MASTER_PORT_ENABLE   (1<<2)
#define I2C_SLAVE_RECE_MODE      (0)
#define I2C_SLAVE_TRAN_MODE      (1<<1)
#define I2C_SLAVE_PORT_ENABLE    (1)


//I2C_OPR
#define SLAVE_7BIT_ADDRESS_MODE  (0)
#define SLAVE_10BIT_ADDRESS_MODE  (1<<8)
#define RESET_I2C_STATUS         (1<<7)
#define I2C_CORE_ENABLE          (1<<6)
#define I2C_CORE_DISABLE         (0)


#define I2C_READ_BIT             (1)
#define I2C_WRITE_BIT            (0)


//I2C Registers
typedef volatile struct tagIIC_STRUCT
{
    uint32 I2C_MTXR;
    uint32 I2C_MRXR;
    uint32 I2C_STXR;
    uint32 I2C_SRXR;
    uint32 I2C_SADDR;
    uint32 I2C_IER;
    uint32 I2C_ISR;
    uint32 I2C_LCMR;
    uint32 I2C_LSR;
    uint32 I2C_CONR;
    uint32 I2C_OPR;
}I2C_REG,*pI2C_REG;

typedef enum I2C_mode
{
    NORMALMODE = 0,
    DIRECTMODE
}eI2C_mode_t;

typedef enum I2C_ch
{
    I2C_CH0,
    I2C_CH1
}eI2C_ch_t;

#undef  EXT
#ifdef  IN_DRIVER_API_I2C
#define EXT
#else
#define EXT     extern
#endif

EXT uint16 g_i2cSpeed;
EXT pI2C_REG g_pI2cReg;
int8 I2CInit(uint8 i2cNumb, uint16 speed);
int8 I2CDeInit(uint8 i2cNumb);
int8 I2CWrite(uint16 I2CSlaveAddr, uint16 regAddr, void *pData, uint16 size, uint8 addressBit, eI2C_mode_t mode);
int8 I2CRead(uint16 I2CSlaveAddr, uint16 regAddr, void *pData, uint16 size, uint8 addressBit, eI2C_mode_t mode);

#endif
#endif


