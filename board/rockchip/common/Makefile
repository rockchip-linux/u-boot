#
# (C) Copyright 2008 - 2014 Rockchip Electronics
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

obj-y += platform/crc.o
obj-y += platform/rsa.o
obj-y += platform/sha.o
obj-y += platform/rc4_enc.o
obj-y += platform/ftl_std.o
obj-y += platform/chipDepend.o
obj-y += rkloader/parameter.o
obj-y += rkloader/idblock.o
obj-y += rkloader/rkloader.o
obj-y += rkloader/rkimage.o
obj-y += rkloader/key.o
obj-$(CONFIG_RK_PWM_REMOTE) += rkloader/pwm_remotectl.o
obj-y += rkboot/fastboot.o
obj-y += emmc/hw_MMC.o
obj-y += emmc/hw_SD.o
obj-y += emmc/hw_SDController.o
obj-y += emmc/hw_SDM.o
obj-y += emmc/hw_SDOsAdapt.o
obj-y += emmc/hw_SDPlatAdapt.o
obj-y += mediaboot/sdmmcBoot.o
ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
obj-y += mediaboot/nandflash_boot.o
endif
obj-$(CONFIG_SECUREBOOT_CRYPTO) += SecureBoot/crypto.o SecureBoot/efuse.o
obj-y += SecureBoot/SecureBoot.o
obj-y += SecureBoot/SecureVerify.o
obj-y += storage/storage.o


