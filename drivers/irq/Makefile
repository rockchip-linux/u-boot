#
# (C) Copyright 2017 Rockchip Electronics Co., Ltd.
#
# SPDX-License-Identifier:     GPL-2.0+
#

obj-y += irq-gic.o irq-generic.o irq-gpio-switch.o virq.o
ifdef CONFIG_IRQ_GPIO_V2
obj-y += irq-gpio-v2.o
else
obj-y += irq-gpio.o
endif

obj-$(CONFIG_IRQ_TIMER_DUMP) += irq-timer.o
