#
# SPDX-License-Identifier: GPL-2.0
#
#  Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
#

crypto_common_rsa := crypto_v2_pka.o crypto_v2_util.o crypto_mpa.o
crypto_common_ec  := crypto_ecc.o crypto_v2_util.o crypto_mpa.o

obj-$(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_V1) += crypto_v1.o crypto_hash_cache.o
obj-$(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_V2) += crypto_v2.o crypto_hash_cache.o
obj-$(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_CE) += crypto_ce.o rkce_core.o

ifneq ($(filter y,$(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_V2) $(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_CE))$(CONFIG_$(SPL_TPL_)ROCKCHIP_RSA),)
obj-y += $(crypto_common_rsa)
endif

ifneq ($(filter y,$(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_V2) $(CONFIG_$(SPL_TPL_)ROCKCHIP_CRYPTO_CE))$(CONFIG_$(SPL_TPL_)ROCKCHIP_EC),)
obj-y += $(crypto_common_ec)
endif
