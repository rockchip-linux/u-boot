# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2013 Samsung Electronics Co., Ltd.
#	http://www.samsung.com

obj-$(CONFIG_$(PHASE_)CRYPTO_MANAGER) += crypto_manager.o
ifdef CONFIG_DM_HASH
obj-$(CONFIG_$(PHASE_)CRYPTO_MANAGER) += crypto_hash_glue.o
endif
ifdef CONFIG_DM_CIPHER
obj-$(CONFIG_$(PHASE_)CRYPTO_MANAGER) += crypto_cipher_glue.o
endif
ifdef CONFIG_RSA
obj-$(CONFIG_$(PHASE_)CRYPTO_MANAGER) += crypto_rsa_glue.o
endif
ifdef CONFIG_ECDSA_VERIFY
obj-$(CONFIG_$(PHASE_)CRYPTO_MANAGER) += crypto_ecc_glue.o
endif
obj-$(CONFIG_$(PHASE_)CRYPTO_MANAGER) += rockchip/

obj-y += hash/
obj-y += cipher/
obj-y += rsa_mod_exp/

obj-$(CONFIG_EXYNOS_ACE_SHA)          += ace_sha.o
obj-y += fsl/
obj-y += aspeed/
obj-y += nuvoton/

