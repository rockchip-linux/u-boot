/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>


typedef enum INT_TRIG {
	INT_LEVEL_TRIG,
	INT_EDGE_TRIG
} eINT_TRIG;

typedef enum INT_SECURE {
	INT_SECURE,
	INT_NOSECURE
} eINT_SECURE;

typedef enum INT_SIGTYPE {
	INT_SIGTYPE_IRQ,
	INT_SIGTYPE_FIQ
} eINT_SIGTYPE;


static inline void int_set_prio_filt(uint32 nprio)
{
	g_giccReg->iccpmr = (nprio&0xff);
}

static inline void int_enable_distributor(void)
{
	g_gicdReg->icddcr = 0x01;
}

static inline void int_disable_distributor(void)
{
	g_gicdReg->icddcr = 0x00;
}

static inline void int_enable_secure_signal(void)
{
	g_giccReg->iccicr |= 0x01;
}

static inline void int_disable_secure_signal(void)
{
	g_giccReg->iccicr &= (~0x01);
}

static inline void int_enable_nosecure_signal(void)
{
	g_giccReg->iccicr |= 0x02;
}

static inline void int_disable_nosecure_signal(void)
{
	g_giccReg->iccicr &= (~0x02);
}


static int gic_irq_set_trig(int irq, eINT_TRIG ntrig)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS)
		return -1;

	M = irq / 16;
	N = irq % 16;

	if (ntrig == INT_LEVEL_TRIG)
		g_gicdReg->icdicfr[M] &= (~(1 << (2 * N + 1)));
	else
		g_gicdReg->icdicfr[M] |= (1 << (2 * N + 1));

	return 0;
}


/* irq set pending */
__maybe_unused static int gic_irq_set_pending(int irq)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS)
		return -1;

	M = irq / 32;
	N = irq % 32;
	g_gicdReg->icdispr[M] = (0x1 << N);

	return 0;
}


/* irq clear pending */
__maybe_unused static int gic_irq_clear_pending(int irq)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS)
		return -1;

	M = irq / 32;
	N = irq % 32;

	g_gicdReg->icdicpr[M] = (0x1 << N);

	return 0;
}


__maybe_unused static int gic_irq_set_secure(int irq, eINT_SECURE nsecure)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS)
		return -1;

	M = irq / 32;
	N = irq % 32;
	g_gicdReg->icdiser[M] |= nsecure << N;

	return 0;
}


static uint8 g_gic_cpumask = 1;
static uint32 gic_get_cpumask(void)
{
	uint32 mask = 0, i;

	for (i = mask = 0; i < 32; i += 4) {
		mask = g_gicdReg->itargetsr[i];
		mask |= mask >> 16;
		mask |= mask >> 8;
		if (mask)
			break;
	}

	if (!mask)
		printf("GIC CPU mask not found.\n");

	debug("GIC CPU mask = 0x%08x\n", mask);

	return mask;
}


/* enable irq handler */
static int gic_handler_enable(int irq)
{
	uint32 shift = (irq % 4) * 8;
	uint32 offset = (irq / 4);
	uint32 M, N;

	if (irq >= NR_GIC_IRQS)
		return -1;

	M = irq / 32;
	N = irq % 32;
	g_giccReg->iccicr &= (~0x08);
	g_gicdReg->icdiser[M] = (0x1 << N);
	g_gicdReg->itargetsr[offset] &= ~(0xFF << shift);
	g_gicdReg->itargetsr[offset] |= (g_gic_cpumask << shift);

	return 0;
}


/* disable irq handler */
static int gic_handler_disable(int irq)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS)
		return -1;

	M = irq / 32;
	N = irq % 32;
	g_gicdReg->icdicer[M] = (0x1<<N);

	return 0;
}

/**
 *	irq_set_type - set the irq trigger type for an irq
 *	@irq:	irq number
 *	@type:	IRQ_TYPE_{LEVEL,EDGE}_* value - see asm/arch/irq.h
 */
static int gic_set_irq_type(int irq, unsigned int type)
{
	unsigned int int_type;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_EDGE_FALLING:
		int_type = INT_EDGE_TRIG;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_LEVEL_LOW:
		int_type = INT_LEVEL_TRIG;
		break;
	default:
		return -1;
	}

	gic_irq_set_trig(irq, int_type);

	return 0;
}


/**
 * gic interrupt init
 */
static void gic_irq_init(void)
{
	debug("gic_irq_init.\n");

	/* end of interrupt */
	g_giccReg->icceoir = NR_GIC_IRQS;
	/* disable signalling the interrupt */
	g_giccReg->iccicr = 0x00;
	g_gicdReg->icddcr = 0x00;

	g_gicdReg->icdicer[0] = 0xFFFFFFFF;
	g_gicdReg->icdicer[1] = 0xFFFFFFFF;
	g_gicdReg->icdicer[2] = 0xFFFFFFFF;
	g_gicdReg->icdicer[3] = 0xFFFFFFFF;
	g_gicdReg->icdicfr[3] &= (~(1<<1));

	int_set_prio_filt(0xff);
	int_enable_secure_signal();
	int_enable_nosecure_signal();
	int_enable_distributor();

	g_gic_cpumask = gic_get_cpumask();
	printf("GIC CPU mask = 0x%08x\n", g_gic_cpumask);
}

static struct irq_chip gic_irq_chip = {
	.name			= (const char *)"gic",

	.irq_disable		= gic_handler_disable,
	.irq_enable		= gic_handler_enable,

	.irq_set_type		= gic_set_irq_type,
};
