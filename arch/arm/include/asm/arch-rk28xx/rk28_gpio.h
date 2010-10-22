/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  gpio.h
Desc    :  定义gpio的寄存器结构体\寄存器位的宏定义\接口函数

Author  : lhh
Date    : 2008-11-20
Modified:
Revision:           1.00
*********************************************************************/
#ifdef DRIVERS_GPIO
#ifndef _DRIVER_GPIO_H_
#define _DRIVER_GPIO_H_

/*********************************************************************
 ENUMERATIONS AND STRUCTURES
*********************************************************************/
typedef enum eGPIOPinLevel
{
    GPIO_LOW=0,
    GPIO_HIGH
}eGPIOPinLevel_t;

typedef enum eGPIOPinDirection
{
    GPIO_IN=0,
    GPIO_OUT
}eGPIOPinDirection_t;

// Constants for GPIO ports
typedef enum eGPIOPORT
{
    GPIOPortA=0,
    GPIOPortB,
    GPIOPortC,
    GPIOPortD,
    GPIOPORTLast
}eGPIOPORT_t;

typedef enum GPIOPullType {
    GPIONormal,
    GPIOPullDown,
    GPIOPullUp,
    GPIONOInit
}eGPIOPullType_t;

typedef enum eGPIOPinNum
{
    GPIO0PortA_Pin0=0,
    GPIO0PortA_Pin1,
    GPIO0PortA_Pin2,
    GPIO0PortA_Pin3,
    GPIO0PortA_Pin4,
    GPIO0PortA_Pin5,
    GPIO0PortA_Pin6,
    GPIO0PortA_Pin7,
    GPIO0PortB_Pin0,
    GPIO0PortB_Pin1,
    GPIO0PortB_Pin2,
    GPIO0PortB_Pin3,
    GPIO0PortB_Pin4,
    GPIO0PortB_Pin5,
    GPIO0PortB_Pin6,
    GPIO0PortB_Pin7,
    GPIO0PortC_Pin0,
    GPIO0PortC_Pin1,
    GPIO0PortC_Pin2,
    GPIO0PortC_Pin3,
    GPIO0PortC_Pin4,
    GPIO0PortC_Pin5,
    GPIO0PortC_Pin6,
    GPIO0PortC_Pin7,
    GPIO0PortD_Pin0,
    GPIO0PortD_Pin1,
    GPIO0PortD_Pin2,
    GPIO0PortD_Pin3,
    GPIO0PortD_Pin4,
    GPIO0PortD_Pin5,
    GPIO0PortD_Pin6,
    GPIO0PortD_Pin7,
    GPIO1PortA_Pin0,
    GPIO1PortA_Pin1,
    GPIO1PortA_Pin2,
    GPIO1PortA_Pin3,
    GPIO1PortA_Pin4,
    GPIO1PortA_Pin5,
    GPIO1PortA_Pin6,
    GPIO1PortA_Pin7,
    GPIO1PortB_Pin0,
    GPIO1PortB_Pin1,
    GPIO1PortB_Pin2,
    GPIO1PortB_Pin3,
    GPIO1PortB_Pin4,
    GPIO1PortB_Pin5,
    GPIO1PortB_Pin6,
    GPIO1PortB_Pin7,
    GPIO1PortC_Pin0,
    GPIO1PortC_Pin1,
    GPIO1PortC_Pin2,
    GPIO1PortC_Pin3,
    GPIO1PortC_Pin4,
    GPIO1PortC_Pin5,
    GPIO1PortC_Pin6,
    GPIO1PortC_Pin7,
    GPIO1PortD_Pin0,
    GPIO1PortD_Pin1,
    GPIO1PortD_Pin2,
    GPIO1PortD_Pin3,
    GPIO1PortD_Pin4,
    GPIO1PortD_Pin5,
    GPIO1PortD_Pin6,
    GPIO1PortD_Pin7,
    GPIOPinNumLast        // for init config cycle num
}eGPIOPinNum_t;

//GPIO Registers
typedef volatile struct tagGPIO_STRUCT
{
    uint32 GPIO_SWPORTA_DR;
    uint32 GPIO_SWPORTA_DDR;
    uint32 RESERVED1;
    uint32 GPIO_SWPORTB_DR;
    uint32 GPIO_SWPORTB_DDR;
    uint32 RESERVED2;
    uint32 GPIO_SWPORTC_DR;
    uint32 GPIO_SWPORTC_DDR;
    uint32 RESERVED3;
    uint32 GPIO_SWPORTD_DR;
    uint32 GPIO_SWPORTD_DDR;
    uint32 RESERVED4;
    uint32 GPIO_INTEN;
    uint32 GPIO_INTMASK;
    uint32 GPIO_INTTYPE_LEVEL;
    uint32 GPIO_INT_POLARITY;
    uint32 GPIO_INT_STATUS;
    uint32 GPIO_INT_RAWSTATUS;
    uint32 GPIO_DEBOUNCE;
    uint32 GPIO_PORTS_EOI;
    uint32 GPIO_EXT_PORTA;
    uint32 GPIO_EXT_PORTB;
    uint32 GPIO_EXT_PORTC;
    uint32 GPIO_EXT_PORTD;
    uint32 GPIO_LS_SYNC;
}GPIO_REG,*pGPIO_REG;

int8 GPIOSetPinDirection(eGPIOPinNum_t GPIOPinNum, eGPIOPinDirection_t direction);
int8 GPIOGetPinDirection(eGPIOPinNum_t GPIOPinNum);
int8 GPIOSetPinLevel(eGPIOPinNum_t GPIOPinNum, eGPIOPinLevel_t level);
int8 GPIOGetPinLevel(eGPIOPinNum_t GPIOPinNum);
int8 GPIOEnableIntr(eGPIOPinNum_t GPIOPinNum);
int8 GPIODisableIntr(eGPIOPinNum_t GPIOPinNum);
int8 GPIOClearIntr(eGPIOPinNum_t GPIOPinNum);
int8 GPIOInmarkIntr(eGPIOPinNum_t GPIOPinNum);
int8 GPIOClrearInmarkIntr(eGPIOPinNum_t GPIOPinNum);
int8 GPIOEnabledDebounce(eGPIOPinNum_t GPIOPinNum);
int8 GPIODisabledDebounce(eGPIOPinNum_t GPIOPinNum);
int8 GPIOIntLevel(eGPIOPinNum_t GPIOPinNum);
int8 GPIOIntEdge(eGPIOPinNum_t GPIOPinNum);
int8 GPIOIntPolarityActiveLow(eGPIOPinNum_t GPIOPinNum);
int8 GPIOIntPolarityActiveHigh(eGPIOPinNum_t GPIOPinNum);
int8 GPIOPullUpDown(eGPIOPinNum_t GPIOPinNum, eGPIOPullType_t GPIOPullUpDown);

void GPIO0IntHander(void);
void GPIO1IntHander(void);

#undef  EXT
#ifdef  IN_DRIVER_GPIO
#define EXT
#else
#define EXT     extern
#endif

EXT pFunc g_gpioVectorTable0[8]; 
EXT pFunc g_gpioVectorTable1[8]; 

#endif
#endif
