

#ifndef _CHIP_DEPEND_H
#define _CHIP_DEPEND_H
#include "configs/rkchip.h"
#include "gpio_oper.h"
#define RK2928G_CHIP_TAG 0x1
#define RK2928L_CHIP_TAG 0x2
#define RK2926_CHIP_TAG  0x0
extern uint8    ChipType;
typedef volatile unsigned int       REG32;
#define RKLD_APB_FREQ           (50*1000) //LOADER 的 APB频率， khz 
#define APB0_TIMER_BASE         TIMER0_BASE_ADDR
#define RKLD_HWTM1_CON          ((REG32*)(APB0_TIMER_BASE+0X0008)) //config 寄存器 
#define RKLD_HWTM1_LR           ((REG32*)(APB0_TIMER_BASE+0X0000)) // 初始计数 寄存器.
#define RKLD_HWTM1_CVR          ((REG32*)(APB0_TIMER_BASE+0X0004)) // 初始计数 寄存器.
#define KRTIMELoaderCount       (uint32)RKLD_APB_FREQ*80*1000      /* 0xee6b2800 */

#define LOADER_FLAG_REG_L         ((REG32*)(GRF_BASE+0x1D8)) // IMEM 最后面4个byte
#define LOADER_FLAG_REG_H         ((REG32*)(GRF_BASE+0x1DC)) // IMEM 最后面4个byte

//定义Loader启动异常类型
#define SYS_LOADER_ERR_FLAG      0X1888AAFF //2928只有16bit了 0X1888AAFF 

//extern void ModifyUsbVidPid(USB_DEVICE_DESCRIPTOR * pDeviceDescr);
extern bool UsbPhyReset(void);
extern void FlashCsInit(void);

#endif
