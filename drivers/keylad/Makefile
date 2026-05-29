#
# Copyright (c) 2025 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier:	GPL-2.0+

obj-$(CONFIG_$(SPL_TPL_)DM_KEYLAD) += keylad.o

keylad-objs := keylad-uclass.o

ifeq ($(CONFIG_ROCKCHIP_RK3588),y)
keylad-objs += rk_keylad_rk3588.o
else
keylad-objs += rk_keylad.o
endif
