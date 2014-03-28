#ifndef __MACH_ROCKCHIP_IOMAP_H
#define __MACH_ROCKCHIP_IOMAP_H

#ifndef __ASSEMBLY__
#include <asm/io.h>
#endif

#define RK_IO_ADDRESS(x)                IOMEM(0xFED00000 + x)

#define RK_CRU_VIRT                     RK_IO_ADDRESS(0x00000000)
#define RK_GRF_VIRT                     RK_IO_ADDRESS(0x00010000)
#define RK_SGRF_VIRT                    (RK_GRF_VIRT + 0x1000)
#define RK_PMU_VIRT                     RK_IO_ADDRESS(0x00020000)
#define RK_ROM_VIRT                     RK_IO_ADDRESS(0x00030000)
#define RK_EFUSE_VIRT                   RK_IO_ADDRESS(0x00040000)
#define RK_GPIO_VIRT(n)                 RK_IO_ADDRESS(0x00050000 + (n) * 0x1000)
#define RK_DEBUG_UART_VIRT              RK_IO_ADDRESS(0x00060000)
#define RK_CPU_AXI_BUS_VIRT             RK_IO_ADDRESS(0x00070000)
#define RK_TIMER_VIRT                   RK_IO_ADDRESS(0x00080000)
#define RK_GIC_VIRT                     RK_IO_ADDRESS(0x00090000)
#define RK_BOOTRAM_VIRT                 RK_IO_ADDRESS(0x000a0000)
#define RK_DDR_VIRT                     RK_IO_ADDRESS(0x000d0000)

#define RK3188_CRU_PHYS                 0x20000000
#define RK3188_CRU_SIZE                 SZ_4K
#define RK3188_GRF_PHYS                 0x20008000
#define RK3188_GRF_SIZE                 SZ_4K
#define RK3188_PMU_PHYS                 0x20004000
#define RK3188_PMU_SIZE                 SZ_4K
#define RK3188_ROM_PHYS                 0x10120000
#define RK3188_ROM_SIZE                 SZ_16K
#define RK3188_EFUSE_PHYS               0x20010000
#define RK3188_EFUSE_SIZE               SZ_4K
#define RK3188_GPIO0_PHYS               0x2000a000
#define RK3188_GPIO1_PHYS               0x2003c000
#define RK3188_GPIO2_PHYS               0x2003e000
#define RK3188_GPIO3_PHYS               0x20080000
#define RK3188_GPIO_SIZE                SZ_4K
#define RK3188_CPU_AXI_BUS_PHYS         0x10128000
#define RK3188_CPU_AXI_BUS_SIZE         SZ_32K
#define RK3188_TIMER0_PHYS              0x20038000
#define RK3188_TIMER3_PHYS              0x2000e000
#define RK3188_TIMER_SIZE               SZ_4K
#define RK3188_DDR_PCTL_PHYS            0x20020000
#define RK3188_DDR_PCTL_SIZE            SZ_4K
#define RK3188_DDR_PUBL_PHYS            0x20040000
#define RK3188_DDR_PUBL_SIZE            SZ_4K
#define RK3188_UART0_PHYS               0x10124000
#define RK3188_UART1_PHYS               0x10126000
#define RK3188_UART2_PHYS               0x20064000
#define RK3188_UART3_PHYS               0x20068000
#define RK3188_UART_SIZE                SZ_4K

#define RK3288_CRU_PHYS                 0xFF760000
#define RK3288_CRU_SIZE                 SZ_4K
#define RK3288_GRF_PHYS                 0xFF770000
#define RK3288_GRF_SIZE                 SZ_4K
#define RK3288_SGRF_PHYS                0xFF740000
#define RK3288_SGRF_SIZE                SZ_4K
#define RK3288_PMU_PHYS                 0xFF730000
#define RK3288_PMU_SIZE                 SZ_4K
#define RK3288_ROM_PHYS                 0xFFFD0000
#define RK3288_ROM_SIZE                 (SZ_16K + SZ_4K)
#define RK3288_EFUSE_PHYS               0xFFB40000
#define RK3288_EFUSE_SIZE               SZ_4K
#define RK3288_GPIO0_PHYS               0xFF750000
#define RK3288_GPIO1_PHYS               0xFF780000
#define RK3288_GPIO2_PHYS               0xFF790000
#define RK3288_GPIO3_PHYS               0xFF7A0000
#define RK3288_GPIO4_PHYS               0xFF7B0000
#define RK3288_GPIO5_PHYS               0xFF7C0000
#define RK3288_GPIO6_PHYS               0xFF7D0000
#define RK3288_GPIO7_PHYS               0xFF7E0000
#define RK3288_GPIO8_PHYS               0xFF7F0000
#define RK3288_GPIO_SIZE                SZ_4K
#define RK3288_SERVICE_CORE_PHYS        0XFFA80000
#define RK3288_SERVICE_CORE_SIZE        SZ_4K
#define RK3288_SERVICE_DMAC_PHYS        0XFFA90000
#define RK3288_SERVICE_DMAC_SIZE        SZ_4K
#define RK3288_SERVICE_GPU_PHYS         0XFFAA0000
#define RK3288_SERVICE_GPU_SIZE         SZ_4K
#define RK3288_SERVICE_PERI_PHYS        0XFFAB0000
#define RK3288_SERVICE_PERI_SIZE        SZ_4K
#define RK3288_SERVICE_BUS_PHYS         0XFFAC0000
#define RK3288_SERVICE_BUS_SIZE         SZ_16K
#define RK3288_SERVICE_VIO_PHYS         0XFFAD0000
#define RK3288_SERVICE_VIO_SIZE         SZ_4K
#define RK3288_SERVICE_VIDEO_PHYS       0XFFAE0000
#define RK3288_SERVICE_VIDEO_SIZE       SZ_4K
#define RK3288_SERVICE_HEVC_PHYS        0XFFAF0000
#define RK3288_SERVICE_HEVC_SIZE        SZ_4K
#define RK3288_TIMER0_PHYS              0xFF6B0000
#define RK3288_TIMER6_PHYS              0xFF810000
#define RK3288_TIMER_SIZE               SZ_4K
#define RK3288_DDR_PCTL0_PHYS           0xFF610000
#define RK3288_DDR_PCTL1_PHYS           0xFF630000
#define RK3288_DDR_PCTL_SIZE            SZ_4K
#define RK3288_DDR_PUBL0_PHYS           0xFF620000
#define RK3288_DDR_PUBL1_PHYS           0xFF640000
#define RK3288_DDR_PUBL_SIZE            SZ_4K
#define RK3288_UART_BT_PHYS             0xFF180000
#define RK3288_UART_BB_PHYS             0xFF190000
#define RK3288_UART_DBG_PHYS            0xFF690000
#define RK3288_UART_GPS_PHYS            0xFF1B0000
#define RK3288_UART_EXP_PHYS            0xFF1C0000
#define RK3288_UART_SIZE                SZ_4K
#define RK3288_GIC_DIST_PHYS            0xFFC01000
#define RK3288_GIC_DIST_SIZE            SZ_4K
#define RK3288_GIC_CPU_PHYS             0xFFC02000
#define RK3288_GIC_CPU_SIZE             SZ_4K
#define RK3288_BOOTRAM_PHYS             0xFF720000
#define RK3288_BOOTRAM_SIZE             SZ_4K
#define RK3288_IMEM_PHYS                0xFF700000
#define RK3288_IMEM_SZIE                0x00018000

#define RK3288_I2C_SENSOR_PHYS          0xFF140000
#define RK3288_I2C_SENSOR_SZIE          SZ_64K
#define RK3288_I2C_CAM_PHYS             0xFF150000
#define RK3288_I2C_CAM_SZIE             SZ_64K
#define RK3288_I2C_TP_PHYS              0xFF160000
#define RK3288_I2C_TP_SZIE              SZ_64K
#define RK3288_I2C_HDMI_PHYS            0xFF170000
#define RK3288_I2C_HDMI_SZIE            SZ_64K
#define RK3288_I2C_PMU_PHYS             0xFF650000
#define RK3288_I2C_PMU_SZIE             SZ_64K
#define RK3288_I2C_AUDIO_PHYS           0xFF660000
#define RK3288_I2C_AUDIO_SZIE           SZ_64K
#define RK3288_NANDC0_PHYS              0xFF400000


#define RK3288_USB_HOST0_EHCI_PHYS      0xFF500000
#define RK3288_USB_HOST0_EHCI_SIZE      SZ_128K
#define RK3288_USB_HOST0_OHCI_PHYS      0xFF520000
#define RK3288_USB_HOST0_OHCI_SIZE      SZ_128K
#define RK3288_USB_HOST0_HOST1_PHYS     0xFF540000
#define RK3288_USB_HOST0_HOST1_SIZE     SZ_256K
#define RK3288_USB_OTG_PHYS             0xFF580000
#define RK3288_USB_OTG_SIZE             SZ_256K
#define RK3288_USB_SHIC_PHYS            0xFF5C0000
#define RK3288_USB_SHIC_SIZE            SZ_256K

#define RK3288_SDMMC_PHY               0xFF0C0000
#define RK3288_SDMMC_SIZE              SZ_64K
#define RK3288_SDIO0_PHY               0xFF0D0000
#define RK3288_SDIO0_SIZE              SZ_64K
#define RK3288_SDIO1_PHY               0xFF0E0000
#define RK3288_SDIO1_SIZE              SZ_64K
#define RK3288_EMMC_PHY                0xFF0F0000
#define RK3288_EMMC_SIZE               SZ_64K
#define RK3288_SAR_ADC_PHY             0xFF100000
#define RK3288_EMSAR_ADCMC_SIZE        SZ_64K




#define PMU_SYS_REG0                   0xFF730094
#define PMU_SYS_REG1                   0xFF730098
#define PMU_SYS_REG2                   0xFF73009c
#define PMU_SYS_REG3                   0xFF7300A0
#define RK3288_BOOTROM_VERSION_ADDR    0xFFFF4FF0    //(320A20131116V100)
#define RK3188_BOOTROM_VERSION_ADDR    0x101227F0
#define PMU_SYS_REG3                   0xFF7300A0
#define PMU_SYS_REG3                   0xFF7300A0
#define PMU_SYS_REG3                   0xFF7300A0

#endif
