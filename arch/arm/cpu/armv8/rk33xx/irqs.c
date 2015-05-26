/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKIRQ_VERSION		"1.2"


/* irq handler struct and hander table */
struct s_irq_handler {
	void (*m_func)(void *data);
};

static struct s_irq_handler g_irq_handler[NR_IRQS_MAXNUM];


/* general interrupt server handler for gpio chip */
static inline void generic_handle_irq(int irq, void *data)
{
	/* if g_irq_handler[irq].m_func == -1, gpio pin irq has no server handler */ 
	if ((g_irq_handler[irq].m_func != NULL) && (g_irq_handler[irq].m_func != (interrupt_handler_t *)-1)) {
		g_irq_handler[irq].m_func(data);
	}
}


/* general irq chip */
#include "irqs-gic.c"

/* gpio irq chip */
#ifdef CONFIG_RK_GPIO
#include "irqs-gpio.c"
#endif


/* interrupt server handler */
static inline void irq_handler(void)
{
	uint32 nintid = gic_irq_getid();

	/* here we use gic id checking, not include gpio pin irq */
	if (nintid < NR_GIC_IRQS) {
		if (g_irq_handler[nintid].m_func != NULL) {
			g_irq_handler[nintid].m_func((void *)(unsigned long)nintid);
		}
	}
	
	gic_irq_finish_server(nintid); 
}


/* irq interrupt handler init */
static inline int irq_init(void)
{
	int i;

	/*
	 * After relocation done, bss data initialized.
	 * So irq_init shoule be initialized after bss data initialized.
	 */
	if (!(gd->flags & GD_FLG_RELOC)) {
		debug("irq_init: interrupt should be initialized after relocation and bss data done.\n");
		return -1;
	}

	debug("rk irq version: %s\n", RKIRQ_VERSION);

	if (gd->flags & GD_FLG_IRQINIT) {
		debug("irq_init: irq has been initialized.\n");
		return 0;
	}
	gd->flags |= GD_FLG_IRQINIT;

	debug("irq_init: irq initialized.\n");
	for (i = 0; i < NR_IRQS_MAXNUM; i++) {
		g_irq_handler[i].m_func = NULL;
	}

	gic_get_cpumask();

	/* gic irq init */
#if !defined(CONFIG_ARM64)
	/* armv8 gic init has init when lowlevel_init done */
	gic_irq_init();
#endif

	/* gpio irq init */
#ifdef CONFIG_RK_GPIO
	gpio_irq_init();
#endif

	return 0;
}


#ifdef CONFIG_IMPRECISE_ABORTS_CHECK
void enable_imprecise_aborts(void)
{
	/* enable imprecise aborts */
	asm volatile("cpsie a");
}
#endif


/* enable irq handler */
int irq_handler_enable(int irq)
{
	if (irq >= NR_IRQS_MAXNUM) {
		return (-1);
	}

	debug("irq_handler_enable: irq = %d.\n", irq);
	if (irq < NR_GIC_IRQS) {
		gic_irq_chip.irq_enable(irq);
	} else {
#ifdef CONFIG_RK_GPIO
		gpio_irq_chip.irq_enable(irq);
#endif
	}

	return (0);
}


/* disable irq handler */
int irq_handler_disable(int irq)
{
	if (irq >= NR_IRQS_MAXNUM) {
		return (-1);
	}

	debug("irq_handler_disable: irq = %d.\n", irq);
	if (irq < NR_GIC_IRQS) {
		gic_irq_chip.irq_disable(irq);
	} else {
#ifdef CONFIG_RK_GPIO
		gpio_irq_chip.irq_disable(irq);
#endif
	}
	
	return (0);
}


/**
 *	irq_set_type - set the irq trigger type for an irq
 *	@irq:	irq number
 *	@type:	IRQ_TYPE_{LEVEL,EDGE}_* value - see asm/arch/irq.h
 */
int irq_set_irq_type(int irq, unsigned int type)
{
	if (irq >= NR_IRQS_MAXNUM) {
		return (-1);
	}

	debug("irq_set_irq_type: irq = %d.\n", irq);
	if (irq < NR_GIC_IRQS) {
		gic_irq_chip.irq_set_type(irq, type);
	} else {
#ifdef CONFIG_RK_GPIO
		gpio_irq_chip.irq_set_type(irq, type);
#endif
	}

	return (0);
}


/* irq interrupt install handle */
void irq_install_handler(int irq, interrupt_handler_t *handler, void *data)
{
	if (irq >= NR_IRQS_MAXNUM || !handler) {
		debug("irq_install_handle error: irq = %d, handler = 0x%p, data = 0x%p.\n", irq, handler, data);
		return ;
	}

	debug("irq_install_handler: irq = %d, handler = 0x%p, data = 0x%p.\n", irq, handler, data);

	if (g_irq_handler[irq].m_func != handler) {
		g_irq_handler[irq].m_func = handler;
	}
}


/* interrupt uninstall handler */
void irq_uninstall_handler(int irq)
{
	if (irq >= NR_IRQS_MAXNUM) {
		return ;
	}

	debug("irq_uninstall_handler: irq = %d.\n", irq);
	g_irq_handler[irq].m_func = NULL;
}


int interrupt_init(void)
{
	return arch_interrupt_init();
}

void enable_interrupts(void)
{
	asm volatile("msr	daifclr, #0x03");
}

int disable_interrupts(void)
{
	asm volatile("msr	daifset, #0x03");
	return 1;
}


void do_irq (struct pt_regs *pt_regs, unsigned int esr)
{
	irq_handler();
}


int arch_interrupt_init (void)
{
	return irq_init();
}
