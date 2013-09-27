/****************************************************************
//    CopyRight(C) 2008 by Rock-Chip Fuzhou
//      All Rights Reserved
//文件名:hw_SDConfig.h
//描述:RK28 SD driver configurable file
//作者:hcy
//创建日期:2008-11-08
//更改记录:
//当前版本:1.00
$Log: hw_SDConfig.h,v $
Revision 1.3  2011/03/30 02:33:29  Administrator
1、支持三星8GB EMMC
2、支持emmc boot 1和boot 2都写入IDBLOCK数据

Revision 1.2  2011/01/21 10:12:56  Administrator
支持EMMC
优化buffer效率

Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:02  Administrator
*** empty log message ***

Revision 1.2  2009/08/18 09:40:09  YYZ
no message

Revision 1.1.1.1  2009/08/18 06:43:27  Administrator
no message

Revision 1.1.1.1  2009/08/14 08:02:01  Administrator
no message

****************************************************************/
#ifdef DRIVERS_SDMMC

#ifndef _SD_CONFIG_H_
#define _SD_CONFIG_H_

/********************定义******************************/
//卡检测方式定义
#define SD_CONTROLLER_DET     (1 << 0) //使用SDMMC控制器自带的卡检测脚
#define SD_GPIO_DET           (1 << 1) //使用外部GPIO做卡检测
#define SD_ALWAYS_PRESENT     (1 << 2) //卡总是存在，不会拔插

#define EMMC_DATA_PART        (0)  // 1
#define EMMC_BOOT_PART        (1)  // 1
#define EMMC_BOOT_PART2       (2)  // 1
#define Only_Controller2_USED (0)      //若设置1，那么只能对SDMMC Controller 2起作用，即对eMMC作用。 不影响其他控制器(0是SDMMC, 1是SDIO)
                                        //若设置0，那么本驱动能同时适用于三个控制器
#define SD_CARD_Support       (1)
#define eMMC_PROJECT_LINUX    (0)


/*****************可配置参数**************************/
#define SDMMC0_USED           (1)      //用于配置硬件上是否使用了SDMMC0端口，1:使用了SDMMC0控制器，0:没有使用
#define SDMMC0_BUS_WIDTH      (4)      //硬件布板为SDMMC0控制器留的数据线宽度，1:一根数据线，4:四根数据线，8:八根数据线，其他值不支持
#define SDMMC0_DET_MODE       SD_CONTROLLER_DET      //卡检测方式选择，从SD_CONTROLLER_DET、SD_GPIO_DET、SD_ALWAYS_PRESENT中选择
#define SDMMC0_EN_POWER_CTL   (0)      //是否对SD卡电源进行控制，1:要控制SD卡电源，0:不对SD卡电源进行控制

#define SDMMC1_USED           (1)      //用于配置硬件上是否使用了SDMMC1端口，1:使用了SDMMC1控制器，0:没有使用
#define SDMMC1_BUS_WIDTH      (4)      //硬件布板为SDMMC1控制器留的数据线宽度，1:一根数据线，4:四根数据线，其他值不支持，SDMMC1控制器不支持8根数据线
#define SDMMC1_DET_MODE       SD_GPIO_DET      //卡检测方式选择，从SD_CONTROLLER_DET、SD_GPIO_DET、SD_ALWAYS_PRESENT中选择
#define SDMMC1_EN_POWER_CTL   (0)      //是否对SD卡电源进行控制，1:要控制SD卡电源，0:不对SD卡电源进行控制

#define SDMMC2_USED           (1)      //用于配置硬件上是否使用了SDMMC1端口，1:使用了SDMMC1控制器，0:没有使用
#define SDMMC2_BUS_WIDTH      (8)       //硬件布板为SDMMC1控制器留的数据线宽度，1:一根数据线，4:四根数据线，其他值不支持，SDMMC1控制器不支持8根数据线
#define SDMMC2_DET_MODE       SD_ALWAYS_PRESENT  //卡检测方式选择，从SD_CONTROLLER_DET、SD_GPIO_DET、SD_ALWAYS_PRESENT中选择
#define SDMMC2_EN_POWER_CTL   (0)      //是否对SD卡电源进行控制，1:要控制SD卡电源，0:不对SD卡电源进行控制

#ifdef SDMMC_USE_DMA
#define EN_SD_DMA             (1)      //是否用DMA来进行数据传输，1:DMA方式，0:中断方式
#else
#define EN_SD_DMA             (0)
#endif

#define EN_SD_INT             (0)      //是否采用中断发生来查询一些SDMMC控制器的重要位，1:用中断方式，0:用轮询方式，目前就算用轮询方式，卡检测还是设成中断的

#define EN_SD_PRINTF          (0)      //是否允许SD驱动内部调试信息打印，1:开启打印，0:关闭打印
#define DEBOUNCE_TIME         (25)     //卡拔插的消抖动时间,单位ms

#define FOD_FREQ              (200)    //卡识别阶段使用的频率,单位KHz,协议规定最大400KHz
//卡正常工作的最低频率为FREQ_HCLK_MAX/8
#define SD_FPP_FREQ           (24000)  //标准SD卡正常工作频率，单位KHz，协议规定最大25MHz
#define SDHC_FPP_FREQ         (40000)  //SDHC卡在高速模式下的工作频率，单位KHz，协议规定最大50MHz
#define MMC_FPP_FREQ          (18000)  //标准MMC卡正常工作频率，单位KHz，协议规定最大20MHz
#define MMCHS_26_FPP_FREQ     (25000)  //高速模式只支持最大26M的HS-MMC卡，在高速模式下的工作频率，单位KHz，协议规定最大26MHz
#define MMCHS_52_FPP_FREQ     (40000)  //高速模式能支持最大52M的HS-MMC卡，在高速模式下的工作频率，单位KHz，协议规定最大52MHz

#if(!EN_SD_PRINTF)
#define SDOAM_Printf(...)
#endif
//#define SDOAM_Printf printf

#endif //end of #ifndef _SD_CONFIG_H_
#endif //end of #ifdef DRIVERS_SDMMC

