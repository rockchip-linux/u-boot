/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	intc.c
Desc 	:	定义INTC的寄存器结构体\寄存器位的宏定义
Author 	:  	yangkai
Date 	:	2008-11-05
Notes 	:   
$Log: intc.h,v $
********************************************************************/
#ifdef DRIVERS_INTC
#ifndef _INTC_H
#define _INTC_H

/********************************************************************
**                            宏定义                                *
********************************************************************/
#define PRIO_MAX 0xff  
/********************************************************************
**                          结构定义                                *
********************************************************************/
typedef volatile struct tagGICD_REG
{
uint32 ICDDCR 	      ;      //0x000 
uint32 ICDICTR 	      ;    //0x004   
uint32 ICDIIDR 	      ;    //0x008
uint32 RESERVED0[29]  ; 
uint32 ICDISR[4] 	    ;   //   0x080  
uint32 RESERVED1[28]  ;
uint32 ICDISER[4]   	;     // 0x100 
uint32 RESERVED2[28]  ;   
uint32 ICDICER[4] 	  ;        //0x180   
uint32 RESERVED3[28]  ;
uint32 ICDISPR[4]   	;      //0x200  
uint32 RESERVED4[28]  ;
uint32 ICDICPR[4] 	  ;      //0x280   
uint32 RESERVED5[28]  ;
uint32 ICDIABR[4] 	  ;        //0x300
uint32 RESERVED6[60]  ;
uint32 ICDIPR_SGI[4] 	;       // 0x400
uint32 ICDIPR_PPI[4] 	;         // 0x410 
uint32 ICDIPR_SPI[18] ;	        //0x420
uint32 RESERVED57[486] ;
uint32 ICDICFR[7]     ;        //0xc00
uint32 RESERVED8[185] ;
uint32 ICDSGIR 	      ;        //0xf00 

}GICD_REG, *pGICD_REG;	
typedef volatile struct tagGICC_REG
{
uint32 ICCICR 	     ;         //0x00 
uint32 ICCPMR 	     ;         //0x04 
uint32 ICCBPR 	     ;         //0x08 
uint32 ICCIAR 	     ;         //0x0c 
uint32 ICCEOIR      ;         //0x10 
uint32 ICCRPR 	     ;         //0x14 
uint32 ICCHPIR      ;         //0x18 
uint32 ICCABPR      ;         //0x1c 
uint32 RESERVED0[55];
uint32 ICCIIDR      ;         //0xfc  
}GICC_REG, *pGICC_REG;


typedef enum _IRQ_NUM
{
    INT_SGI0        ,
    INT_SGI1        ,
    INT_SGI2        ,
    INT_SGI3        ,
    INT_SGI4        ,
    INT_SGI5        ,
    INT_SGI6        ,
    INT_SGI7        ,
    INT_SGI8        ,
    INT_SGI9        ,
    INT_SGI10       ,
    INT_SGI11       ,
    INT_SGI12       ,
    INT_SGI13       ,
    INT_SGI14       ,
    INT_SGI15       ,
    INT_PPI0        ,
    INT_PPI1        ,
    INT_PPI2        ,
    INT_PPI3        ,
    INT_PPI4        ,
    INT_PPI5        ,
    INT_PPI6        ,
    INT_PPI7        ,
    INT_PPI8        ,
    INT_PPI9        ,
    INT_PPI10       ,
    INT_PPI11       ,
    INT_PPI12       ,
    INT_PPI13       ,
    INT_PPI14       ,
    INT_PPI15       ,
    
    INT_DMAC0_0     ,
    INT_DMAC0_1     ,
    INT_DMAC0_2     ,
    INT_DMAC0_3     ,
    INT_DMAC2_0     ,
    INT_DMAC2_1     ,
    INT_DMAC2_2     ,
    INT_DMAC2_3     ,
    INT_DMAC2_4     ,
    INT_GPU         ,
    INT_VEPU        ,
    INT_VDPU        ,
    INT_VIP         ,
    INT_LCDC        ,
    INT_IPP         ,
    INT_EBC         ,
    INT_USB_OTG0    ,
    INT_USB_OTG1    ,
    INT_USB_Host    ,
    INT_MAC         ,
    INT_HIF0        ,
    INT_HIF1        ,
    INT_HSADC_TSI   ,
    INT_SDMMC       ,
    INT_SDIO        ,
    INT_eMMC        ,
    INT_SARADC      ,
    INT_NandC       ,
    INT_NandCRDY    ,
    INT_SMC         ,
    INT_PID_FILTER  ,
    INT_I2S_PCM_8CH ,
    INT_I2S_PCM_2CH ,
    INT_SPDIF       ,
    INT_UART0       ,
    INT_UART1       ,
    INT_UART2       ,
    INT_UART3       ,
    INT_SPI0        ,
    INT_SPI1        ,
    INT_I2C0        ,
    INT_I2C1        ,
    INT_I2C2        ,
    INT_I2C3        ,
    INT_TIMER0      ,
    INT_TIMER1      ,
    INT_TIMER2      ,
    INT_TIMER3      ,
    INT_PWM0        ,
    INT_PWM1        ,
    INT_PWM2        ,
    INT_PWM3        ,
    INT_WDT         ,
    INT_RTC         ,
    INT_PMU         ,
    INT_GPIO0       ,
    INT_GPIO1       ,
    INT_GPIO2       ,
    INT_GPIO3       ,
    INT_GPIO4       ,
    INT_GPIO5       ,
    INT_GPIO6       ,
    INT_USB_AHB_ARB ,
    INT_PERI_AHB_ARB,
    INT_A8IRQ0      ,
    INT_A8IRQ1      ,
    INT_A8IRQ2      ,
    INT_A8IRQ3      ,
    INT_MAXNUM      
}eINT_NUM;

typedef enum FIQ_NUM
{
    FIQ_GPIO0 = 0,
    FIQ_SWI,
    FIQ_MAXNUM
}eFIQ_NUM;

typedef struct tagTNUM64
{
 uint32 L32;
 uint32 H32;
}TNUM64;
typedef enum INT_TRIG
{
	INT_LEVEL_TRIG,
	INT_EDGE_TRIG,
}eINT_TRIG;
typedef enum INT_SECURE
{
	INT_SECURE,
	INT_NOSECURE,
}eINT_SECURE;
typedef enum INT_SIGTYPE
{
	INT_SIGTYPE_IRQ,
	INT_SIGTYPE_FIQ,
}eINT_SIGTYPE;


/********************************************************************
**                          变量定义                                *
********************************************************************/
#undef EXT
#ifdef IN_INTC
    #define EXT
#else    
    #define EXT extern
#endif
EXT     pFunc g_irqVectorTable[104];  // IRQ controller interrupt handle vector table
EXT     int g_irqVectorTableParam1[104];
EXT     pFunc g_irqVectorTableParam2[104];
EXT     pFunc g_fiqVectorTable[104];
EXT     int g_fiqVectorTableParam1[104];
EXT     pFunc g_fiqVectorTableParam2[104];

#define     USB_OTG_INT_CH                  (0x30)


//EXT     pINTC_REG g_intcReg;
#define g_gicdReg ((pGICD_REG)GIC_PERI_BASE_ADDR)
#define g_giccReg ((pGICC_REG)GIC_CPU_BASE_ADDR)

/********************************************************************
**                          函数声明                                *
********************************************************************/

/********************************************************************
**                          表格定义                                *
********************************************************************/

#endif
#endif
