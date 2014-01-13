

#ifndef _CHIP_DEPEND_H
#define _CHIP_DEPEND_H

#include "gpio_oper.h"
#include "configs/rkchip.h"
#define     RK3068_POP_CHIP_TAG      0xF
#define     RK3068_CHIP_TAG          0x5
#define     RK3066_CHIP_TAG          0x0
#define     RK3000_CHIP_TAG          0xC


extern uint8    ChipType;

//typedef volatile unsigned int       REG32;
#define RKLD_APB_FREQ           (50*1000) //LOADER 的 APB频率， khz 
#define APB0_TIMER_BASE         TIMER0_BASE_ADDR
#define RKLD_HWTM1_CON          ((REG32*)(APB0_TIMER_BASE+0X0008)) //config 寄存器 
#define RKLD_HWTM1_LR           ((REG32*)(APB0_TIMER_BASE+0X0000)) // 初始计数 寄存器.
#define RKLD_HWTM1_CVR          ((REG32*)(APB0_TIMER_BASE+0X0004)) // 初始计数 寄存器.
#define KRTIMELoaderCount       (uint32)RKLD_APB_FREQ*80*1000      /* 0xee6b2800 */

//#define LOADER_FLAG_REG         ((REG32*)(GRF_BASE+0x1C8)) //GRF_OS_REG0
#define LOADER_FLAG_REG         ((REG32*)(PMU_BASE_ADDR+0x40)) //PMU_OS_REG0
#define LOADER_MODE_REG         ((REG32*)(PMU_BASE_ADDR+0x44)) //PMU_OS_REG4

//定义Loader启动异常类型
#define SYS_LOADER_ERR_FLAG      0X1888AAFF 

//extern void ModifyUsbVidPid(USB_DEVICE_DESCRIPTOR * pDeviceDescr);
extern bool UsbPhyReset(void);
extern void FlashCsInit(void);

#endif
