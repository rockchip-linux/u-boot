

#define     PALTFORM    RK30XX

#define     RK_USB_EN
//#define     RK_FLASH_BOOT_EN
//#define   RK_SPI_BOOT_EN
//#define     RK_SDMMC_BOOT_EN
//#define   DRIVERS_EFUSE
#define     SDMMC_SDC_ID        2
#define     EMMC_BOOT_ENABLE    1
//#define     L2CACHE_ENABLE

#define     RK_MODIFY_USB_VIDPID_EN     1 

//RK30 实测是 一个循环是17ns @ 600Mhz 2级优化
#define     CPU_DELAY_US(n)  volatile  uint32 i = n>>4;while (i--);

#define     L2CACHE_WAY_SIZE    2 //0b000:16KB,0b001:16KB,0b010:32KB,0b011:64KB,0b100:128KB,0b101:256KB,0b110:512KB
#define     L2CACHE_WAY_NUM     1 //0  8-way , 1  16-way

#define     SD_FIFO_OFFSET    0x200
#define     FIFO_DEPTH        (0x100)       //FIFO depth = 256 word


