# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2017 Heinrich Schuchardt

# This file only gets included with CONFIG_EFI_LOADER set, so all
# object inclusion implicitly depends on it

obj-y += efi_uclass.o
ifeq ($(CONFIG_PARTITIONS),y)
obj-y += efi_block_device.o
endif

obj-$(CONFIG_GBL_EFI_BOOT_CONTROL_PROTOCOL) += gbl_efi_boot_control_protocol.o
obj-$(CONFIG_GBL_EFI_AVB_PROTOCOL) += gbl_efi_avb_protocol.o
obj-$(CONFIG_GBL_EFI_FASTBOOT_PROTOCOL) += gbl_efi_fastboot_protocol.o
obj-$(CONFIG_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL) += gbl_efi_fastboot_transport_protocol.o
obj-$(CONFIG_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_DUMMY) += gbl_efi_fastboot_transport_protocol_dummy.o
obj-$(CONFIG_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_SERIAL) += gbl_efi_fastboot_transport_protocol_interactive_serial.o
obj-$(CONFIG_GBL_EFI_OS_CONFIGURATION_PROTOCOL) += gbl_efi_os_configuration_protocol.o
obj-$(CONFIG_EFI_DT_FIXUP_CF) += efi_dt_fixup_cf.o
obj-$(CONFIG_SYSRESET_SBI) += efi_reset_riscv.o
