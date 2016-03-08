/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_USE_IRQ

#define RKIRQ_VERSION		"1.1"


/* irq handler struct and hander table */
struct s_irq_handler {
	void (*m_func)(void *data);
};

static struct s_irq_handler g_irq_handler[NR_IRQS];


/* general interrupt server handler for gpio chip */
static inline void generic_handle_irq(int irq, void *data)
{
	/* m_func == -1 for gpio pin irq has no server handler */
	if ((g_irq_handler[irq].m_func != NULL)
			&& (g_irq_handler[irq].m_func != (interrupt_handler_t *)-1))
		g_irq_handler[irq].m_func(data);
}


/* general irq chip */
#include "irqs-gic.c"

/* gpio irq chip */
#ifdef CONFIG_RK_GPIO
#include "irqs-gpio.c"
#endif


/* get interrupt id */
static inline uint32 gic_irq_getid(void)
{
	return g_giccReg->icciar & 0x3ff; /* bit9 - bit0*/
}

/* finish interrupt server */
static inline void gic_irq_finish_server(uint32 nintid)
{
	g_giccReg->icceoir = nintid;
}


/* interrupt server handler */
static inline void irq_handler(void)
{
	uint32 nintid = gic_irq_getid();

	/* here we use gic id checking, not include gpio pin irq */
	if (nintid < NR_GIC_IRQS)
		if (g_irq_handler[nintid].m_func != NULL) \
			g_irq_handler[nintid].m_func((void *)(unsigned long)nintid);

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
		debug("interrupt should be initialized after relocation and bss data done.\n");
		return -1;
	}

	if (gd->flags & GD_FLG_IRQINIT) {
		debug("rk irq has been initialized.\n");
		return 0;
	}
	gd->flags |= GD_FLG_IRQINIT;

	debug("rk irq version: %s, initialized.\n", RKIRQ_VERSION);
	for (i = 0; i < NR_IRQS; i++)
		g_irq_handler[i].m_func = NULL;

	/* gic irq init */
	gic_irq_init();

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
	if (irq >= NR_IRQS)
		return -1;

	if (irq < NR_GIC_IRQS)
		gic_irq_chip.irq_enable(irq);
#ifdef CONFIG_RK_GPIO
	else
		gpio_irq_chip.irq_enable(irq);
#endif

	return 0;
}


/* disable irq handler */
int irq_handler_disable(int irq)
{
	if (irq >= NR_IRQS)
		return -1;

	if (irq < NR_GIC_IRQS)
		gic_irq_chip.irq_disable(irq);
#ifdef CONFIG_RK_GPIO
	else
		gpio_irq_chip.irq_disable(irq);
#endif

	return 0;
}


/**
 *	irq_set_type - set the irq trigger type for an irq
 *	@irq:	irq number
 *	@type:	IRQ_TYPE_{LEVEL,EDGE}_* value - see asm/arch/irq.h
 */
int irq_set_irq_type(int irq, unsigned int type)
{
	if (irq >= NR_IRQS)
		return -1;

	if (irq < NR_GIC_IRQS)
		gic_irq_chip.irq_set_type(irq, type);
#ifdef CONFIG_RK_GPIO
	else
		gpio_irq_chip.irq_set_type(irq, type);
#endif

	return 0;
}


/* irq interrupt install handle */
void irq_install_handler(int irq, interrupt_handler_t *handler, void *data)
{
	if (irq >= NR_IRQS || !handler) {
		printf("error: irq = %d, handler = 0x%p, data = 0x%p.\n", irq, handler, data);
		return;
	}

	if (g_irq_handler[irq].m_func != handler)
		g_irq_handler[irq].m_func = handler;
}


/* interrupt uninstall handler */
void irq_uninstall_handler(int irq)
{
	if (irq >= NR_IRQS)
		return;

	g_irq_handler[irq].m_func = NULL;
}


void do_irq(struct pt_regs *pt_regs)
{
	irq_handler();
}


int arch_interrupt_init(void)
{
	return irq_init();
}

#else

int irq_handler_enable(int irq)
{
	return -1;
}

int irq_handler_disable(int irq)
{
	return -1;
}

int irq_set_irq_type(int irq, unsigned int type)
{
	return -1;
}

void irq_install_handler(int irq, interrupt_handler_t *handler, void *data)
{

}

void irq_uninstall_handler(int irq)
{

}

int arch_interrupt_init(void)
{
	return -1;
}

#endif /* CONFIG_USE_IRQ */
