#********************************************************************************
#       COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
#           --  ALL RIGHTS RESERVED  --
#File Name:
#Author:
#Created:
#Modified:
#Revision:       1.00
#********************************************************************************

#obj-y += armlinux/armlinux.o
obj-y += armlinux/bootloader.o
obj-y += armlinux/enter_recovery.o
#obj-y += armlinux/Mainloop.o
obj-y += armlinux/mtdpart.o
obj-y += armlinux/parameter.o
#obj-y += armlinux/session.o
#obj-y += armlinux/rk28_kld.o
#obj-y += common/A8MMU/CacheCleanFlushRegion.o
#obj-y += common/A8MMU/CacheCleanFlushWay.o
#obj-y += common/A8MMU/CacheCleanFlushWhole.o
#obj-y += common/A8MMU/CacheControl.o
#obj-y += common/A8MMU/CacheLock.o
obj-y += common/A8MMU/hw_mm.o
#obj-y += common/A8MMU/MMU.o
#obj-y += common/A8MMU/MMUControl.o
#obj-y += common/A8MMU/MMUOperateTLB.o
#obj-y += common/A8MMU/pl310_l2cc.o
#obj-y += common/A8MMU/RK30CacheCleanFlushWhole.o
#obj-y += common/crc/ldcrc.o
#obj-y += common/crc/ldcrctable.o
obj-y += common/crc/rsa.o
obj-y += common/crc/sha.o
obj-y += common/emmc/hw_MMC.o
obj-y += common/emmc/hw_SD.o
obj-y += common/emmc/hw_SDController.o
obj-y += common/emmc/hw_SDM.o
obj-y += common/emmc/hw_SDOsAdapt.o
obj-y += common/emmc/hw_SDPlatAdapt.o
obj-y += common/emmc/sdmmcBoot.o
#obj-y += common/fat/chkdsk.o
#obj-y += common/fat/dir.o
#obj-y += common/fat/Drive.o
#obj-y += common/fat/fat.o
#obj-y += common/fat/FDT.o
#obj-y += common/fat/file.o
obj-y += common/rc4/rc4.o
obj-y += common/rc4/rc4_enc.o
obj-y += common/rockusb/Chap_9.o
obj-y += common/rockusb/D12ci.o
obj-y += common/rockusb/rockusb.o
obj-y += common/rockusb/usbCtrl.o
obj-y += common/rockusb/usbIsr.o
#obj-y += common/serial_p9/serial.o
#obj-y += common/SPI/SpiBoot.o
#obj-y += common/SPI/SpiFlash.o
obj-y += common/boot.o
obj-y += common/GetIdBlockInfo.o
#obj-y += common/loaderTag.o
obj-y += common/std_commod.o
obj-y += common/storage.o
#obj-y += platform/platform.o


obj-$(CONFIG_RK_3026_CHIP) += platform/rk2928/chipDepend.o
obj-$(CONFIG_RK_30XX_CHIP) += platform/rk30/chipDepend.o
obj-y += platform/rk30/gpio_oper.o
obj-y += platform/rk30/Isr.o
##obj-y += platform/rk30/regdump.o
##obj-y += platform/rk30/romloader.o
##obj-y += platform/rk30/start.o
obj-y += platform/rk30/timer.o
##obj-y += platform/rk30/uart.o
##obj-y += dummy.o
