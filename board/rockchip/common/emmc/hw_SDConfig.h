/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SD_CONFIG_H_
#define _SD_CONFIG_H_

/********************定义******************************/
/* 卡检测方式定义 */
#define SD_CONTROLLER_DET     (1 << 0) /* 使用SDMMC控制器自带的卡检测脚 */
#define SD_GPIO_DET           (1 << 1) /* 使用外部GPIO做卡检测 */
#define SD_ALWAYS_PRESENT     (1 << 2) /* 卡总是存在，不会拔插 */

#define EMMC_DATA_PART        (0)
#define EMMC_BOOT_PART        (1)
#define EMMC_BOOT_PART2       (2)

#define SD_CARD_Support       (1)


/*****************可配置参数**************************/
#define SDMMC0_BUS_WIDTH      (4) /* 硬件布板为SDMMC0控制器留的数据线宽度，1:一根数据线，4:四根数据线，8:八根数据线，其他值不支持 */
#define SDMMC0_DET_MODE       SD_CONTROLLER_DET /* 卡检测方式选择，从SD_CONTROLLER_DET、SD_GPIO_DET、SD_ALWAYS_PRESENT中选择 */
#define SDMMC0_EN_POWER_CTL   (0) /* 是否对SD卡电源进行控制，1:要控制SD卡电源，0:不对SD卡电源进行控制 */

#define SDMMC1_BUS_WIDTH      (4) /* 硬件布板为SDMMC1控制器留的数据线宽度，1:一根数据线，4:四根数据线，其他值不支持，SDMMC1控制器不支持8根数据线 */
#define SDMMC1_DET_MODE       SD_GPIO_DET /* 卡检测方式选择，从SD_CONTROLLER_DET、SD_GPIO_DET、SD_ALWAYS_PRESENT中选择 */
#define SDMMC1_EN_POWER_CTL   (0) /* 是否对SD卡电源进行控制，1:要控制SD卡电源，0:不对SD卡电源进行控制 */

#define SDMMC2_BUS_WIDTH      (8) /* 硬件布板为SDMMC1控制器留的数据线宽度，1:一根数据线，4:四根数据线，其他值不支持，SDMMC1控制器不支持8根数据线 */
#define SDMMC2_DET_MODE       SD_ALWAYS_PRESENT /* 卡检测方式选择，从SD_CONTROLLER_DET、SD_GPIO_DET、SD_ALWAYS_PRESENT中选择 */
#define SDMMC2_EN_POWER_CTL   (0) /* 是否对SD卡电源进行控制，1:要控制SD卡电源，0:不对SD卡电源进行控制 */


/* dmac config */
#if defined(CONFIG_RK_MMC_DMA)

/* external mac */
#if defined(CONFIG_RK_MMC_EDMAC) && defined(CONFIG_RK_PL330_DMAC)
#define EN_SD_DMA             (1) /* 是否用DMA来进行数据传输，1:DMA方式，0:中断方式 */
#define EN_SD_INT             (1) /* 是否采用中断发生来查询一些SDMMC控制器的重要位，1:用中断方式，0:用轮询方式，目前就算用轮询方式，卡检测还是设成中断的 */
#else
#define EN_SD_DMA             (0)
#define EN_SD_INT             (0) /* 是否采用中断发生来查询一些SDMMC控制器的重要位，1:用中断方式，0:用轮询方式，目前就算用轮询方式，卡检测还是设成中断的 */
#endif

/* internal dmac */
#ifdef CONFIG_RK_MMC_IDMAC
#define EN_SDC_INTERAL_DMA    (1)
#define EN_SD_DATA_TRAN_INT   (0)
#else
#define EN_SDC_INTERAL_DMA    (0)
#define EN_SD_DATA_TRAN_INT   (0)
#endif

#else

#define EN_SD_DMA             (0)
#define EN_SDC_INTERAL_DMA    (0)

#endif /* CONFIG_RK_MMC_DMA */

#if defined(CONFIG_RK_MMC_DDR_MODE)
#define EN_EMMC_DDR_MODE      (1)
#else
#define EN_EMMC_DDR_MODE      (0)
#endif /* CONFIG_RK_MMC_DDR_MODE */

#define EN_SD_PRINTF          (0)      /* 是否允许SD驱动内部调试信息打印，1:开启打印，0:关闭打印 */
#define DEBOUNCE_TIME         (25)     /* 卡拔插的消抖动时间,单位ms */

#define FOD_FREQ              (200)    /* 卡识别阶段使用的频率,单位KHz,协议规定最大400KHz */
/* 卡正常工作的最低频率为FREQ_HCLK_MAX / 8 */
#define SD_FPP_FREQ           (24000)  /* 标准SD卡正常工作频率，单位KHz，协议规定最大25MHz */
#define SDHC_FPP_FREQ         (40000)  /* SDHC卡在高速模式下的工作频率，单位KHz，协议规定最大50MHz */
#define MMC_FPP_FREQ          (18000)  /* 标准MMC卡正常工作频率，单位KHz，协议规定最大20MHz */
#define MMCHS_26_FPP_FREQ     (25000)  /* 高速模式只支持最大26M的HS-MMC卡，在高速模式下的工作频率，单位KHz，协议规定最大26MHz */

#if (EN_SD_DMA) || (EN_SDC_INTERAL_DMA)
#define MMCHS_52_FPP_FREQ     (50000)  /* 高速模式能支持最大52M的HS-MMC卡，在高速模式下的工作频率，单位KHz，协议规定最大52MHz */
#else
#define MMCHS_52_FPP_FREQ     (40000)  /* 高速模式能支持最大52M的HS-MMC卡，在高速模式下的工作频率，单位KHz，协议规定最大52MHz */
#endif

#if (!EN_SD_PRINTF)
#define SDOAM_Printf(...)
#else
#define SDOAM_Printf printf
#endif

#endif /* end of #ifndef _SD_CONFIG_H_ */
#endif /* end of #ifdef DRIVERS_SDMMC */
