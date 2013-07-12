#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)librockchip.o

#COBJS-y += armlinux/armlinux.o
COBJS-y += armlinux/bootloader.o
COBJS-y += armlinux/enter_recovery.o
#COBJS-y += armlinux/Mainloop.o
COBJS-y += armlinux/mtdpart.o
COBJS-y += armlinux/parameter.o
#COBJS-y += armlinux/session.o
#COBJS-y += armlinux/rk28_kld.o
#SOBJS-y += common/A8MMU/CacheCleanFlushRegion.o
#SOBJS-y += common/A8MMU/CacheCleanFlushWay.o
#SOBJS-y += common/A8MMU/CacheCleanFlushWhole.o
#SOBJS-y += common/A8MMU/CacheControl.o
#SOBJS-y += common/A8MMU/CacheLock.o
COBJS-y += common/A8MMU/hw_mm.o
#COBJS-y += common/A8MMU/MMU.o
#SOBJS-y += common/A8MMU/MMUControl.o
#SOBJS-y += common/A8MMU/MMUOperateTLB.o
#COBJS-y += common/A8MMU/pl310_l2cc.o
#SOBJS-y += common/A8MMU/RK30CacheCleanFlushWhole.o
#COBJS-y += common/crc/ldcrc.o
#COBJS-y += common/crc/ldcrctable.o
COBJS-y += common/crc/rsa.o
#COBJS-y += common/crc/sha.o
COBJS-y += common/emmc/hw_MMC.o
COBJS-y += common/emmc/hw_SD.o
COBJS-y += common/emmc/hw_SDController.o
COBJS-y += common/emmc/hw_SDM.o
COBJS-y += common/emmc/hw_SDOsAdapt.o
COBJS-y += common/emmc/hw_SDPlatAdapt.o
COBJS-y += common/emmc/sdmmcBoot.o
#COBJS-y += common/fat/chkdsk.o
#COBJS-y += common/fat/dir.o
#COBJS-y += common/fat/Drive.o
#COBJS-y += common/fat/fat.o
#COBJS-y += common/fat/FDT.o
#COBJS-y += common/fat/file.o
COBJS-y += common/rc4/rc4.o
COBJS-y += common/rc4/rc4_enc.o
COBJS-y += common/rockusb/Chap_9.o
COBJS-y += common/rockusb/D12ci.o
COBJS-y += common/rockusb/rockusb.o
COBJS-y += common/rockusb/usbCtrl.o
COBJS-y += common/rockusb/usbIsr.o
#COBJS-y += common/serial_p9/serial.o
#COBJS-y += common/SPI/SpiBoot.o
#COBJS-y += common/SPI/SpiFlash.o
COBJS-y += common/boot.o
COBJS-y += common/GetIdBlockInfo.o
#SOBJS-y += common/loaderTag.o
COBJS-y += common/std_commod.o
COBJS-y += common/storage.o
#COBJS-y += platform/platform.o
COBJS-y += platform/rk30/chipDepend.o
COBJS-y += platform/rk30/gpio_oper.o
COBJS-y += platform/rk30/Isr.o
#COBJS-y += platform/rk30/regdump.o
#SOBJS-y += platform/rk30/romloader.o
#SOBJS-y += platform/rk30/start.o
COBJS-y += platform/rk30/timer.o
#COBJS-y += platform/rk30/uart.o
#COBJS-y += dummy.o


COBJS	:= $(COBJS-y)
#SOBJS	:= $(SOBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS) $(SOBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
