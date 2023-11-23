# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2013 Samsung Electronics Co., Ltd.
#	http://www.samsung.com

obj-$(CONFIG_EXYNOS_ACE_SHA)	+= ace_sha.o
obj-$(CONFIG_CRYPTO_MANAGER) += crypto_manager.o
ifdef CONFIG_DM_HASH
obj-$(CONFIG_CRYPTO_MANAGER) += crypto_hash.o
endif
obj-$(CONFIG_CRYPTO_MANAGER) += crypto_rsa.o
obj-y += rsa_mod_exp/
obj-y += fsl/
obj-y += hash/
obj-y += aspeed/
obj-y += nuvoton/
obj-y += rockchip/
