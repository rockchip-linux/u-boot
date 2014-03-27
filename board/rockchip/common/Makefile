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
obj-$(CONFIG_ZYF) += armlinux/bootloader.o
obj-$(CONFIG_ZYF) += armlinux/enter_recovery.o
#obj-y += armlinux/Mainloop.o
obj-$(CONFIG_ZYF) += armlinux/mtdpart.o
obj-$(CONFIG_ZYF) += armlinux/parameter.o
#obj-y += armlinux/session.o
#obj-y += armlinux/rk28_kld.o
#obj-y += common/A8MMU/CacheCleanFlushRegion.o
#obj-y += common/A8MMU/CacheCleanFlushWay.o
#obj-y += common/A8MMU/CacheCleanFlushWhole.o
#obj-y += common/A8MMU/CacheControl.o
#obj-y += common/A8MMU/CacheLock.o
obj-$(CONFIG_ZYF) += common/A8MMU/hw_mm.o
#obj-y += common/A8MMU/MMU.o
#obj-y += common/A8MMU/MMUControl.o
#obj-y += common/A8MMU/MMUOperateTLB.o
#obj-y += common/A8MMU/pl310_l2cc.o
#obj-y += common/A8MMU/RK30CacheCleanFlushWhole.o
#obj-y += common/crc/ldcrc.o
#obj-y += common/crc/ldcrctable.o
obj-$(CONFIG_ZYF) += common/crc/rsa.o
obj-$(CONFIG_ZYF) += common/crc/sha.o
obj-$(CONFIG_ZYF) += common/emmc/hw_MMC.o
obj-$(CONFIG_ZYF) += common/emmc/hw_SD.o
obj-$(CONFIG_ZYF) += common/emmc/hw_SDController.o
obj-$(CONFIG_ZYF) += common/emmc/hw_SDM.o
obj-$(CONFIG_ZYF) += common/emmc/hw_SDOsAdapt.o
obj-$(CONFIG_ZYF) += common/emmc/hw_SDPlatAdapt.o
obj-$(CONFIG_ZYF) += common/emmc/sdmmcBoot.o
#obj-y += common/fat/chkdsk.o
#obj-y += common/fat/dir.o
#obj-y += common/fat/Drive.o
#obj-y += common/fat/fat.o
#obj-y += common/fat/FDT.o
#obj-y += common/fat/file.o
obj-$(CONFIG_ZYF) += common/rc4/rc4.o
obj-$(CONFIG_ZYF) += common/rc4/rc4_enc.o
obj-$(CONFIG_ZYF) += common/rockusb/Chap_9.o
obj-$(CONFIG_ZYF) += common/rockusb/D12ci.o
obj-$(CONFIG_ZYF) += common/rockusb/rockusb.o
obj-$(CONFIG_ZYF) += common/rockusb/usbCtrl.o
obj-$(CONFIG_ZYF) += common/rockusb/usbIsr.o
#obj-y += common/serial_p9/serial.o
#obj-y += common/SPI/SpiBoot.o
#obj-y += common/SPI/SpiFlash.o
obj-$(CONFIG_ZYF) += common/boot.o
obj-$(CONFIG_ZYF) += common/GetIdBlockInfo.o
#obj-y += common/loaderTag.o
obj-$(CONFIG_ZYF) += common/std_commod.o
obj-$(CONFIG_ZYF) += common/storage.o
#obj-y += platform/platform.o


obj-$(CONFIG_RK_3026_CHIP) += platform/rk2928/chipDepend.o
obj-$(CONFIG_RK_30XX_CHIP) += platform/rk30/chipDepend.o
obj-$(CONFIG_ZYF) += platform/rk30/gpio_oper.o
obj-$(CONFIG_ZYF) += platform/rk30/Isr.o
##obj-y += platform/rk30/regdump.o
##obj-y += platform/rk30/romloader.o
##obj-y += platform/rk30/start.o
#obj-$(CONFIG_ZYF) += platform/rk30/timer.o
##obj-y += platform/rk30/uart.o
##obj-y += dummy.o
