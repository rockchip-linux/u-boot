/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>


typedef enum INT_TRIG
{
	INT_LEVEL_TRIG,
	INT_EDGE_TRIG
} eINT_TRIG;

typedef enum INT_SECURE
{
	INT_SECURE,
	INT_NOSECURE
} eINT_SECURE;

typedef enum INT_SIGTYPE
{
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

	if (irq >= NR_GIC_IRQS) {
		return (-1);
	}

	debug("gic_irq_set_trig: irq = %d, ntrig = %d.\n", irq, ntrig);
	M = irq / 16;
	N = irq % 16;

	if(ntrig == INT_LEVEL_TRIG) {
		g_gicdReg->icdicfr[M] &= (~(1<<(2*N+1)));   
	} else {  
		g_gicdReg->icdicfr[M] |= (1<<(2*N+1));
	}

	return 0; 
}

/* irq set pending */
static int gic_irq_set_pending(int irq)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS) {
		return (-1);
	}

	debug("gic_irq_handler_pending: irq = %d.\n", irq);
	M = irq / 32;
	N = irq % 32;
	g_gicdReg->icdispr[M] = (0x1<<N);

	return 0; 
}


/* irq clear pending */
static int gic_irq_clear_pending(int irq)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS) {
		return (-1);
	}

	debug("gic_irq_clear_pending: irq = %d.\n", irq);
	M = irq / 32;
	N = irq % 32;

	g_gicdReg->icdicpr[M] = (0x1<<N);

	return 0; 
}


static int gic_irq_set_secure(int irq, eINT_SECURE nsecure)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS) {
		return (-1);
	}

	debug("gic_irq_set_secure: irq = %d, nsecure = %d.\n", irq, nsecure);
	M = irq / 32;
	N = irq % 32;
	g_gicdReg->icdiser[M] |= (nsecure)<<(N);

	return 0; 
}


/* enable irq handler */
static int gic_handler_enable(int irq)
{
	uint32 shift = (irq % 4) * 8;
	uint32 offset = (irq / 4);
	uint32 M, N;

	if (irq >= NR_GIC_IRQS) {
		return (-1);
	}

	debug("gic_handler_enable: irq = %d.\n", irq);
	M = irq / 32;
	N = irq % 32;
	g_giccReg->iccicr &= (~0x08);
	g_gicdReg->icdiser[M] = (0x1<<N);
	g_gicdReg->itargetsr[offset] |= (1 << shift);

	return (0);
}


/* disable irq handler */
static int gic_handler_disable(int irq)
{
	uint32 M, N;

	if (irq >= NR_GIC_IRQS) {
		return (-1);
	}

	debug("gic_handler_disable: irq = %d.\n", irq);
	M = irq / 32;
	N = irq % 32;
	g_gicdReg->icdicer[M] = (0x1<<N);

	return (0);
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
}

static struct irq_chip gic_irq_chip = {
	.name			= (const char*)"gic",

	.irq_disable		= gic_handler_disable,
	.irq_enable		= gic_handler_enable,

	.irq_set_type		= gic_set_irq_type,
};

