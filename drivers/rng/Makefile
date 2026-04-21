# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2019, Linaro Limited
#

obj-$(CONFIG_$(SPL_TPL_)DM_RNG) += rng-uclass.o
obj-$(CONFIG_$(SPL_TPL_)RNG_ROCKCHIP) += rockchip_rng.o

ifdef CONFIG_LIB_HW_RAND
obj-$(CONFIG_$(SPL_TPL_)RNG_ROCKCHIP) += rockchip_rand.o
endif

