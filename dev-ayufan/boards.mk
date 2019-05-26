ifeq (rock64,$(BOARD_TARGET))

UBOOT_DEFCONFIG ?= rock64-rk3328_defconfig
UBOOT_TPL ?= tmp/rkbin/rk33/rk3328_ddr_786MHz_v1.13.bin
BL31 ?= tmp/rkbin/rk33/rk3328_bl31_v1.39.bin
BOARD_CHIP ?= rk3328
ifneq (,$(FLASH_SPI))
LOADER_BIN ?= tmp/rkbin/rk33/rk3328_loader_v1.08.244_for_spi_nor_build_Aug_7_2017.bin
else
LOADER_BIN ?= tmp/rkbin/rk33/rk3328_loader_ddr333_v1.08.244.bin
endif
IMAGES ?= flash-spi erase-spi
LOADERS ?= rksd_loader

else ifeq (rockpro64,$(BOARD_TARGET))

UBOOT_DEFCONFIG ?= rockpro64-rk3399_defconfig
UBOOT_TPL ?= tmp/rkbin/rk33/rk3399_ddr_933MHz_v1.19.bin
BL31 ?= tmp/rkbin/rk33/rk3399_bl31_v1.25.elf
BOARD_CHIP ?= rk3399
LOADER_BIN ?= tmp/rkbin/rk33/rk3399_loader_v1.10.112_support_1CS.bin
USE_SEPARATE_SPIFLASH ?= true
IMAGES ?= flash-spi erase-spi
LOADERS ?= rksd_loader rkspi_loader

else ifeq (rockpi4b,$(BOARD_TARGET))

UBOOT_DEFCONFIG ?= rockpi4b-rk3399_defconfig
UBOOT_TPL ?= tmp/rkbin/rk33/rk3399_ddr_933MHz_v1.19.bin
BL31 ?= tmp/rkbin/rk33/rk3399_bl31_v1.25.elf
BOARD_CHIP ?= rk3399
LOADER_BIN ?= tmp/rkbin/rk33/rk3399_loader_v1.10.112_support_1CS.bin
USE_SEPARATE_SPIFLASH ?= true
IMAGES ?= flash-spi erase-spi
LOADERS ?= rksd_loader rkspi_loader

else ifeq (pinebookpro,$(BOARD_TARGET))

UBOOT_DEFCONFIG ?= pinebook_pro-rk3399_defconfig
UBOOT_TPL ?= tmp/rkbin/rk33/rk3399_ddr_933MHz_v1.19.bin
BL31 ?= tmp/rkbin/rk33/rk3399_bl31_v1.25.elf
BOARD_CHIP ?= rk3399
LOADER_BIN ?= tmp/rkbin/rk33/rk3399_loader_v1.10.112_support_1CS.bin
USE_SEPARATE_SPIFLASH ?= true
IMAGES ?= flash-spi erase-spi
LOADERS ?= rksd_loader rkspi_loader

else
$(error Unsupported BOARD_TARGET)
endif
