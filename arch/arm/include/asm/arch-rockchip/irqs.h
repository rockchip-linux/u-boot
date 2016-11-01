/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_IRQS_H
#define __RKXX_IRQS_H

#include <common.h>

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
#define GIC_DIST_SOFTINT		0xf00


/* INTC Registers */
typedef volatile struct tagGICD_REG {
	uint32_t icddcr;      	//0x000
	uint32_t icdictr;    	//0x004
	uint32_t icdiidr;    	//0x008
	uint32_t reserved0[29];
	uint32_t icdisr[4];	//0x080
	uint32_t reserved1[28];
	uint32_t icdiser[4];	//0x100
	uint32_t reserved2[28];
	uint32_t icdicer[4];	//0x180
	uint32_t reserved3[28];
	uint32_t icdispr[4];	//0x200
	uint32_t reserved4[28];
	uint32_t icdicpr[4];	//0x280
	uint32_t reserved5[28];
	uint32_t icdiabr[4];	//0x300
	uint32_t reserved6[60];
	uint32_t icdipr_sgi[4];	//0x400
	uint32_t icdipr_ppi[4];	//0x410
	uint32_t icdipr_spi[18];	//0x420
	uint32_t reserved7[230];
	uint32_t itargetsr[255];	//0x800
	uint32_t reserved9[1];
	uint32_t icdicfr[7];	//0xc00
	uint32_t reserved8[185];
	uint32_t icdsgir;		//0xf00
} GICD_REG, *pGICD_REG;


typedef volatile struct tagGICC_REG {
	uint32_t iccicr;		//0x00
	uint32_t iccpmr;		//0x04
	uint32_t iccbpr;		//0x08
	uint32_t icciar;		//0x0c
	uint32_t icceoir;		//0x10
	uint32_t iccrpr;		//0x14
	uint32_t icchpir;		//0x18
	uint32_t iccabpr;		//0x1c
	uint32_t reserved0[55];
	uint32_t icciidr;		//0xfc
} GICC_REG, *pGICC_REG;

/* define for gic, from RKIO_CORE_GIC_PHYS */
#define RKIO_GICC_PHYS			0xFFC02000 /* GIC CPU */
#define RKIO_GICD_PHYS			0xFFC01000 /* GIC DIST */

#define g_gicdReg       ((pGICD_REG)RKIO_GICD_PHYS)
#define g_giccReg       ((pGICC_REG)RKIO_GICC_PHYS)


/*
 * RKXX irq
 *
 */
#if defined(CONFIG_ROCKCHIP_RK3288)
	#include "irqs-rk3288.h"
#elif defined(CONFIG_RKCHIP_RK3036)
	#include "irqs-rk3036.h"
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#include "irqs-rk312X.h"
#elif defined(CONFIG_RKCHIP_RK322X)
	#include "irqs-rk322x.h"
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
