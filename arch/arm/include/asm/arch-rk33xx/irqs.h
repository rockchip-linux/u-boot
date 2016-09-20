/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_IRQS_H
#define __RKXX_IRQS_H

#include <common.h>
#include <asm/gic.h>


/*
 * RKXX irq
 *
 */
#if defined(CONFIG_RKCHIP_RK3368)
	#include "irqs-rk3368.h"
#elif defined(CONFIG_RKCHIP_RK3366)
	#include "irqs-rk3366.h"
#elif defined(CONFIG_RKCHIP_RK3399)
	#include "irqs-rk3399.h"
#elif defined(CONFIG_RKCHIP_RK322XH)
	#include "irqs-rk322xh.h"
#else
	#error "PLS config irqs-rkxx.h!"
#endif

/*
 * IRQ line status.
 *
 *
 * IRQ_TYPE_NONE		- default, unspecified type
 * IRQ_TYPE_EDGE_RISING		- rising edge triggered
 * IRQ_TYPE_EDGE_FALLING	- falling edge triggered
 * IRQ_TYPE_EDGE_BOTH		- rising and falling edge triggered
 * IRQ_TYPE_LEVEL_HIGH		- high level triggered
 * IRQ_TYPE_LEVEL_LOW		- low level triggered
 * IRQ_TYPE_LEVEL_MASK		- Mask to filter out the level bits
 * IRQ_TYPE_SENSE_MASK		- Mask for all the above bits
 */
enum {
	IRQ_TYPE_NONE		= 0x00000000,
	IRQ_TYPE_EDGE_RISING	= 0x00000001,
	IRQ_TYPE_EDGE_FALLING	= 0x00000002,
	IRQ_TYPE_EDGE_BOTH	= (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
	IRQ_TYPE_LEVEL_HIGH	= 0x00000004,
	IRQ_TYPE_LEVEL_LOW	= 0x00000008,
	IRQ_TYPE_LEVEL_MASK	= (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH),
	IRQ_TYPE_SENSE_MASK	= 0x0000000f,
};

/**
 * struct irq_chip - hardware interrupt chip descriptor
 *
 * @name:		name for /proc/interrupts
 * @irq_enable:		enable the interrupt (defaults to chip->unmask if NULL)
 * @irq_disable:	disable the interrupt
 * @irq_ack:		start of a new interrupt
 * @irq_eoi:		end of interrupt
 * @irq_set_type:	set the flow type (IRQ_TYPE_LEVEL/etc.) of an IRQ
 *
 * @release:		release function solely used by UML
 */
struct irq_chip {
	const char	*name;

	int		(*irq_enable)(int irq);
	int		(*irq_disable)(int irq);

	void		(*irq_ack)(int irq);
	void		(*irq_eoi)(int irq);

	int		(*irq_set_type)(int irq, unsigned int flow_type);
};


#ifdef CONFIG_IMPRECISE_ABORTS_CHECK
void enable_imprecise_aborts(void);
#endif


/* irq_install_handler() has been define in include/common.h */
void irq_install_handler(int irq, interrupt_handler_t *handler, void *data);
void irq_uninstall_handler(int irq);
int irq_set_irq_type(int irq, unsigned int type);
int irq_handler_enable(int irq);
int irq_handler_disable(int irq);


#endif /* __RKXX_IRQS_H */
