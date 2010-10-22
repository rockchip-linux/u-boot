/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	grf.c
Desc 	:	GPIO功能选择IOMUX，保证每个PIN脚只有一个函数入口可以设置 
Author 	:  	yangkai
Date 	:	2009-01-05
Notes 	:

********************************************************************/
#define IN_GRF
#include  <asm/arch/rk28_drivers.h>
#ifdef DRIVERS_GRF

/*----------------------------------------------------------------------
Name	: IOMUXSetI2C0
Desc	: I2C0相关pin脚设置
Params	: type:IOMUX_I2C0--设置成I2C使用
			   IOMUX_I2C0_GPIO--设置成GPIO
Return	: 
Notes	:
----------------------------------------------------------------------*/
void IOMUXSetI2C0(eIOMUX_I2C0 type)
{
	if(type)
	{
		g_grfReg->IOMUX_A_CON &= ~IOMUXA_GPIO1_A45;
	}
	else
	{
		g_grfReg->IOMUX_A_CON |= IOMUXA_GPIO1_A45;
	}
}

/*----------------------------------------------------------------------
Name	: IOMUXSetI2C1
Desc	: I2C0相关pin脚设置
Params	: type:IOMUX_I2C1_GPIO--设置成GPIO
			   IOMUX_I2C1_UART1--设置成UART1信号线
			   IOMUX_I2C1	  --设置成I2C信号线
Return	: 
Notes	:
----------------------------------------------------------------------*/
void IOMUXSetI2C1(eIOMUX_I2C1 type)
{
	g_grfReg = (pGRF_REG)REG_FILE_BASE_ADDR;
	uint32 config = g_grfReg->IOMUX_A_CON;
	config &= ~(0x3<<28);
	config |= (type<<28);
	g_grfReg->IOMUX_A_CON = config;	
}

/*----------------------------------------------------------------------
Name	: GRFInit
Desc	: GRF模块初始化
Params  : 
Return  : 
Notes   : 系统开机时初始化IOMUX
----------------------------------------------------------------------*/
void GRFInit(void)
{
    g_grfReg = (pGRF_REG)REG_FILE_BASE_ADDR;
    
}

/*----------------------------------------------------------------------
Name	: IOMUXSetSPI0
Desc	: 设置SPI1相关管脚
Params  : type:IOMUX_SPI0_GPIO  --设置成GPIO
               IOMUX_SPI0_CSN0  --设置成SPI1+CSN0信号线
               IOMUX_SPI0_CSN1  --设置成SPI1+CSN0信号线
               IOMUX_SPI0_SDMMC0--设置相关管脚为SDMMC0信号线
               IOMUX_SPI0_SDMMC1--设置相关管脚为SDMMC1信号线
Return  : 
Notes   :
----------------------------------------------------------------------*/
void IOMUXSetSPI0(eIOMUX_SPI0 type)
{
    uint32 config = g_grfReg->IOMUX_A_CON;
    switch(type)
    {
        case IOMUX_SPI0_CSN0:
            config &= ~0x3000f;
            config |= 0x04;/////////
            break;
        case IOMUX_SPI0_CSN1:
            config &= ~0x3000f;
            config |= 0x10004;
            break;
        case IOMUX_SPI0_SDMMC0:
            config &= ~0x000f;
            config |= 0x0a;
            break;
        case IOMUX_SPI0_SDMMC1:
            config &= ~0x30000;
            config |= 0x20000;
            break;
        case IOMUX_SPI0_GPIO:
            config &= ~0x3000f;
            break;
        default:
            break;
    }
    g_grfReg->IOMUX_A_CON = config;
}

#endif
