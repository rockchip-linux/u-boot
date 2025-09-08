# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2020 Philippe Reynes <philippe.reynes@softathome.com>

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_BUTTON) += button-uclass.o
obj-$(CONFIG_BUTTON_ADC) += button-adc.o
obj-$(CONFIG_BUTTON_GPIO) += button-gpio.o
obj-$(CONFIG_BUTTON_QCOM_PMIC) += button-qcom-pmic.o
obj-$(CONFIG_BUTTON_RK8XX) += button-rk8xx.o
else
obj-$(CONFIG_SPL_BUTTON_ADC) += button-adc-spl.o
endif
