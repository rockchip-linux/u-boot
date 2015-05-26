/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_IRQS_H
#define __RKXX_IRQS_H

#include <common.h>


/* gic v2 register offset */

/* Distributor interface definitions */
#define GICD_CTLR		0x0000
#define GICD_TYPER		0x0004
#define GICD_IGROUPR		0x0080
#define GICD_ISENABLER		0x0100
#define GICD_ICENABLER		0x0180
#define GICD_ISPENDR		0x0200
#define GICD_ICPENDR		0x0280
#define GICD_ISACTIVER		0x0300
#define GICD_ICACTIVER		0x0380
#define GICD_IPRIORITYR		0x0400
#define GICD_ITARGETSR		0x0800
#define GICD_ICFGR		0x0C00
#define GICD_SGIR		0x0F00
#define GICD_CPENDSGIR		0x0F10
#define GICD_SPENDSGIR		0x0F20


/* Physical CPU Interface registers */
#define GICC_CTLR		0x0000
#define GICC_PMR		0x0004
#define GICC_BPR		0x0008
#define GICC_IAR		0x000C
#define GICC_EOIR		0x0010
#define GICC_RPR		0x0014
#define GICC_HPPIR		0x0018
#define GICC_AHPPIR		0x0028
#define GICC_IIDR		0x00FC
#define GICC_DIR		0x1000
#define GICC_PRIODROP           GICC_EOIR


/*
 * RKXX irq
 *
 */
#if defined(CONFIG_RKCHIP_RK3368)
	#include "irqs-rk3368.h"
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
