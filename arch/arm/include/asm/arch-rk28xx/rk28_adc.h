/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  adc.h
Desc    :  定义adc的寄存器结构体\寄存器位的宏定义\接口函数

Author  : lhh
Date    : 2008-11-20
Modified:
Revision:           1.00
$Log: adc.h,v $
*********************************************************************/
#ifdef DRIVERS_ADC
#ifndef _DRIVER_ADC_H_
#define _DRIVER_ADC_H_

// define the ADC convert rate ,  1000 =  1000KHz 
//#define ADC_Frequence   (1000)   


//ADC_STAS
#define ADC_STOP         (0)
//ADC_CTRL
#define ADC_ENABLED_INT (1<<5) 
#define ADC_DISABLED_INT (0) 
#define ADC_POWER_ON    (1<<3)
#define ADC_POWER_OFF    (0)
#define ADC_START       (1<<4)


//ADC Registers
typedef volatile struct tagADC_STRUCT
{
    uint32 ADC_DATA;
    uint32 ADC_STAS;
    uint32 ADC_CTRL;
}ADC_REG,*pADC_REG;


#define ADC_Frequence   (1000)   

typedef enum Adc_channel
{
    Adc_channel0=0,
    Adc_channel1,
    Adc_channel2,
    Adc_channel3,
    Adc_channel_max
}ADC_channel_t;

int32 ADCInit(void);
int32 ADCStart(uint8 ch);
int32 ADCUpdataApbFreq(uint32 APBnKHz);
int32 ADCReadData(void);
void ADCDeinit(void);
void RockAdcScanning(void);

#endif
#endif