#
# (C) Copyright 2008 - 2015 Rockchip Electronics
# Peter, Software Engineering, <superpeter.cai@gmail.com>.
#
# SPDX-License-Identifier:	GPL-2.0+
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
obj-$(CONFIG_RK_UMS_BOOT_EN) += mediaboot/UMSBoot.o
ifdef CONFIG_SECOND_LEVEL_BOOTLOADER
obj-y += mediaboot/nandflash_boot.o
endif
ifdef CONFIG_SECURE_RSA_KEY_IN_RAM
obj-$(CONFIG_SECUREBOOT_CRYPTO) += SecureBoot/crypto.o
else
obj-$(CONFIG_SECUREBOOT_CRYPTO) += SecureBoot/crypto.o SecureBoot/efuse.o
endif
obj-y += SecureBoot/SecureBoot.o
obj-y += SecureBoot/SecureVerify.o
obj-y += storage/storage.o


