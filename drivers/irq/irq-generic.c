/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <dm.h>
#include <asm/io.h>
#include <asm/u-boot-arm.h>
#include <asm/ptrace.h>
#include <irq-generic.h>
#include "irq-internal.h"

DECLARE_GLOBAL_DATA_PTR;

/* Individual interrupt handler for shared IRQ support */
struct irq_handler {
	interrupt_handler_t *handle_irq;
	void *data;
	struct list_head node;
	u32 count;
};

struct irq_desc {
	struct list_head handlers;	/* List of irq_handler for shared IRQ */
	u32 flag;
	u32 count;
	int handler_count;		/* Number of handlers in the list */
	bool is_shared;			/* True if this IRQ has multiple handlers */
};

struct irqchip_desc {
	struct irq_chip *gic;
	struct irq_chip *gpio;
	struct irq_chip *virq;

	int suspend_irq[PLATFORM_SUSPEND_MAX_IRQ];
	int suspend_num;
};

static struct irq_desc irq_desc[PLATFORM_MAX_IRQ];
static struct irqchip_desc irqchip;
static bool intr_setup;

int bad_irq(int irq)
{
	if (!intr_setup) {
		IRQ_W("Interrupt framework is not setup\n");
		return -EINVAL;
	}

	if (irq < PLATFORM_MAX_IRQ) {
		if (list_empty(&irq_desc[irq].handlers))
			return -EINVAL;
	} else {
		if (bad_virq(irq)) {
			IRQ_E("Unknown virq: %d\n", irq);
			return -EINVAL;
		}
	}

	return 0;
}

/* general interrupt handler for gpio chip */
void __generic_gpio_handle_irq(int irq)
{
	struct irq_handler *handler;
	struct list_head *node;

	if (bad_irq(irq))
		return;

	if (irq < PLATFORM_GIC_MAX_IRQ) {
		IRQ_W("IRQ %d: Invalid GPIO irq\n", irq);
		return;
	}

	irq_desc[irq].count++;
	/* Iterate through all registered handlers (shared IRQ support) */
	list_for_each(node, &irq_desc[irq].handlers) {
		handler = list_entry(node, struct irq_handler, node);
		if (handler->handle_irq) {
			handler->count++;
			handler->handle_irq(irq, handler->data);
		}
	}
}

void __do_generic_irq_handler(void)
{
	struct irq_handler *handler;
	struct list_head *node;
	u32 irq;

	assert(irqchip.gic->irq_get);
	assert(irqchip.gic->irq_eoi);

	irq = irqchip.gic->irq_get();

	if (irq < PLATFORM_GIC_MAX_IRQ) {
		irq_desc[irq].count++;
		/* Iterate through all registered handlers (shared IRQ support) */
		list_for_each(node, &irq_desc[irq].handlers) {
			handler = list_entry(node, struct irq_handler, node);
			if (handler->handle_irq) {
				handler->count++;
				handler->handle_irq(irq, handler->data);
			}
		}
	}

	irqchip.gic->irq_eoi(irq);
}

int irq_is_busy(int irq)
{
	return (irq >= 0 && !list_empty(&irq_desc[irq].handlers)) ? -EBUSY : 0;
}

static int bad_irq_chip(struct irq_chip *chip)
{
	return (!chip->name || !chip->irq_init || !chip->irq_enable ||
		!chip->irq_disable) ? -EINVAL : 0;
}

static int __do_arch_irq_init(void)
{
	int ret = -EINVAL;

	/* After relocation done, bss data intr_setup */
	if (!(gd->flags & GD_FLG_RELOC)) {
		IRQ_W("Interrupt framework should initialize after reloc\n");
		return -EINVAL;
	}

	/*
	 * We set true before arch_gpio_irq_init() to avoid fail when
	 * request irq for gpio banks.
	 */
	intr_setup = true;
	memset(irq_desc, 0, sizeof(irq_desc));

	/* Initialize handler list heads for shared IRQ support */
	for (int i = 0; i < PLATFORM_MAX_IRQ; i++)
		INIT_LIST_HEAD(&irq_desc[i].handlers);

	irqchip.gic = arch_gic_get_irqchip();
	if (bad_irq_chip(irqchip.gic)) {
		IRQ_E("Bad gic irqchip\n");
		goto out;
	}

	irqchip.gpio = arch_gpio_get_irqchip();
	if (bad_irq_chip(irqchip.gpio)) {
		IRQ_E("Bad gpio irqchip\n");
		goto out;
	}

	irqchip.virq = arch_virq_get_irqchip();
	if (bad_irq_chip(irqchip.virq)) {
		IRQ_E("Bad virq irqchip\n");
		goto out;
	}

	ret = irqchip.gic->irq_init();
	if (ret) {
		IRQ_E("GIC Interrupt setup failed, ret=%d\n", ret);
		goto out;
	}

	ret = irqchip.gpio->irq_init();
	if (ret) {
		IRQ_E("GPIO Interrupt setup failed, ret=%d\n", ret);
		goto out;
	}

	ret = irqchip.virq->irq_init();
	if (ret) {
		IRQ_E("VIRQ Interrupt setup failed, ret=%d\n", ret);
		goto out;
	}

	return 0;

out:
	intr_setup = false;

	return ret;
}

int irq_handler_enable(int irq)
{
	int ret;

	if (bad_irq(irq))
		return -EINVAL;

	if (irq < PLATFORM_GIC_MAX_IRQ)
		ret = irqchip.gic->irq_enable(irq);
	else if (irq < PLATFORM_GPIO_MAX_IRQ)
		ret = irqchip.gpio->irq_enable(irq);
	else
		ret = irqchip.virq->irq_enable(irq);

	if (!ret && irq < PLATFORM_MAX_IRQ)
		irq_desc[irq].flag |= IRQ_FLG_ENABLE;

	return ret;
}

int irq_handler_disable(int irq)
{
	int ret;

	if (bad_irq(irq))
		return -EINVAL;

	if (irq < PLATFORM_GIC_MAX_IRQ)
		ret = irqchip.gic->irq_disable(irq);
	else if (irq < PLATFORM_GPIO_MAX_IRQ)
		ret = irqchip.gpio->irq_disable(irq);
	else
		ret = irqchip.virq->irq_disable(irq);

	if (!ret && irq < PLATFORM_MAX_IRQ)
		irq_desc[irq].flag &= ~IRQ_FLG_ENABLE;

	return ret;
}

int irq_set_irq_type(int irq, unsigned int type)
{
	if (bad_irq(irq))
		return -EINVAL;

	if (irq < PLATFORM_GIC_MAX_IRQ)
		return irqchip.gic->irq_set_type(irq, type);
	else if (irq < PLATFORM_GPIO_MAX_IRQ)
		return irqchip.gpio->irq_set_type(irq, type);
	else
		return -ENOSYS;
}

int irq_revert_irq_type(int irq)
{
	if (bad_irq(irq))
		return -EINVAL;

	if (irq < PLATFORM_GIC_MAX_IRQ)
		return 0;
	else if (irq < PLATFORM_GPIO_MAX_IRQ)
		return irqchip.gpio->irq_revert_type(irq);
	else
		return -ENOSYS;
}

int irq_get_gpio_level(int irq)
{
	if (bad_irq(irq))
		return -EINVAL;

	if (irq < PLATFORM_GIC_MAX_IRQ)
		return 0;
	else if (irq < PLATFORM_GPIO_MAX_IRQ)
		return irqchip.gpio->irq_get_gpio_level(irq);
	else
		return -ENOSYS;
}

/* Internal function with shared flag */
static void irq_install_handler_with_flags(int irq, interrupt_handler_t *handler,
					   void *data, u32 irq_flags)
{
	struct irq_handler *irq_handler;

	if (!intr_setup) {
		IRQ_W("Interrupt framework is not intr_setup\n");
		return;
	}

	if (!handler)
		return;

	if (irq < PLATFORM_MAX_IRQ) {
		/* Exclusive mode: check if handler already exists */
		if (!(irq_flags & IRQF_SHARED) &&
		    !list_empty(&irq_desc[irq].handlers)) {
			IRQ_E("IRQ %d already has a handler\n", irq);
			return;
		}

		/* Allocate new irq_handler structure */
		irq_handler = (struct irq_handler *)malloc(sizeof(*irq_handler));
		if (!irq_handler) {
			IRQ_E("Failed to allocate memory for irq handler\n");
			return;
		}

		irq_handler->handle_irq = handler;
		irq_handler->data = data;
		irq_handler->count = 0;

		/* Add to the handler list */
		list_add_tail(&irq_handler->node, &irq_desc[irq].handlers);
		irq_desc[irq].handler_count++;
		if (irq_desc[irq].handler_count > 1)
			irq_desc[irq].is_shared = true;
	} else {
		/* VIRQ doesn't support shared mode */
		if (irq_flags & IRQF_SHARED) {
			IRQ_E("VIRQ %d doesn't support shared handlers\n", irq);
			return;
		}
		virq_install_handler(irq, handler, data);
	}
}

void irq_install_handler(int irq, interrupt_handler_t *handler, void *data)
{
	irq_install_handler_with_flags(irq, handler, data, IRQF_NONE);
}

void irq_install_handler_flags(int irq, interrupt_handler_t *handler,
			       void *data, u32 irq_flags)
{
	irq_install_handler_with_flags(irq, handler, data, irq_flags);
}

void irq_free_handler(int irq)
{
	struct irq_handler *irq_handler;
	struct list_head *node, *tmp;

	if (irq_handler_disable(irq))
		return;

	if (irq < PLATFORM_MAX_IRQ) {
		/* Free all handlers in the list */
		list_for_each_safe(node, tmp, &irq_desc[irq].handlers) {
			irq_handler = list_entry(node, struct irq_handler, node);
			list_del_init(&irq_handler->node);
			free(irq_handler);
		}
		irq_desc[irq].handler_count = 0;
		irq_desc[irq].is_shared = false;
	} else {
		virq_free_handler(irq);
	}
}

int irq_handler_enable_suspend_only(int irq)
{
	if (bad_irq(irq))
		return -EINVAL;

	if (irqchip.suspend_num >= PLATFORM_SUSPEND_MAX_IRQ) {
		printf("Over max count(%d) of suspend irq\n",
		       PLATFORM_SUSPEND_MAX_IRQ);
		return -EPERM;
	}

	irqchip.suspend_irq[irqchip.suspend_num++] = irq;
	return 0;
}

int irqs_suspend(void)
{
	int i;

	for (i = 0; i < irqchip.suspend_num; i++)
		irq_handler_enable(irqchip.suspend_irq[i]);

	return irqchip.gic->irq_suspend();
}

int irqs_resume(void)
{
	int i;

	for (i = 0; i < irqchip.suspend_num; i++)
		irq_handler_disable(irqchip.suspend_irq[i]);

	return irqchip.gic->irq_resume();
}

void do_irq(struct pt_regs *pt_regs)
{
#ifdef CONFIG_IRQ_TIMER_DUMP
	printf("\n>>> Rockchp Debugger:\n");
	show_regs(pt_regs);
#endif

	__do_generic_irq_handler();
}

int arch_interrupt_init(void)
{
#ifndef CONFIG_ARM64
	unsigned long cpsr __maybe_unused;

	/* stack has been reserved in: arch_reserve_stacks() */
	IRQ_STACK_START = gd->irq_sp;
	IRQ_STACK_START_IN = gd->irq_sp;

	__asm__ __volatile__("mrs %0, cpsr\n"
			     : "=r" (cpsr)
			     :
			     : "memory");

	__asm__ __volatile__("msr cpsr_c, %0\n"
			     "mov sp, %1\n"
			     :
			     : "r" (IRQ_MODE | I_BIT |
				    F_BIT | (cpsr & ~FIQ_MODE)),
			       "r" (IRQ_STACK_START)
			     : "memory");

	__asm__ __volatile__("msr cpsr_c, %0"
			     :
			     : "r" (cpsr)
			     : "memory");
#endif
	return __do_arch_irq_init();
}

int interrupt_init(void)
{
	int err;

	err = arch_interrupt_init();
	if (!err) {
		enable_interrupts();
#ifdef CONFIG_IRQ_TIMER_DUMP
		irq_timeout_stackdump();
#endif
		printf("IRQ: Enabled\n");
	}

	return err;
}

void enable_interrupts(void)
{
	local_irq_enable();
}

int disable_interrupts(void)
{
	int flags;

	local_irq_save(flags);
	return flags;
}

static int do_dump_irqs(struct cmd_tbl *cmdtp, int flag,
			int argc, char * const argv[])
{
	struct udevice *dev;
	struct irq_handler *handler;
	struct list_head *node;
	char *drv_name;
	int pirq;
	int total_irqs = 0, enabled_irqs = 0, shared_irqs = 0;
	u64 total_count = 0;

	/* Print summary header */
	printf("\n");
	printf("================================================================================\n");
	printf("                           IRQ Summary Statistics\n");
	printf("\n");

	for (pirq = 0; pirq < PLATFORM_MAX_IRQ; pirq++) {
		if (!list_empty(&irq_desc[pirq].handlers)) {
			total_irqs++;
			if (irq_desc[pirq].flag & IRQ_FLG_ENABLE)
				enabled_irqs++;
			if (irq_desc[pirq].is_shared)
				shared_irqs++;
			total_count += irq_desc[pirq].count;
		}
	}

	printf(" Total IRQs: %d     Enabled: %d     Shared: %d     Total Events: %llu\n",
	       total_irqs, enabled_irqs, shared_irqs, total_count);
	printf("================================================================================\n");
	printf("\n");

	/* Print IRQ table header */
	printf(" IRQ  Type          En  Handler             Driver         DevName              Count   Share\n");
	printf("--------------------------------------------------------------------------------------------\n");

	for (pirq = 0; pirq < PLATFORM_MAX_IRQ; pirq++) {
		if (list_empty(&irq_desc[pirq].handlers))
			continue;

		list_for_each(node, &irq_desc[pirq].handlers) {
			handler = list_entry(node, struct irq_handler, node);
			if (!handler->handle_irq)
				continue;

			dev = (struct udevice *)handler->data;
			if (dev && strstr(dev->name, "gpio"))
				drv_name = "IRQ";
			else if (dev && dev->driver)
				drv_name = dev->driver->name;
			else
				drv_name = "N/A";

			/* Determine IRQ type string */
			const char *type_str;

			if (pirq < PLATFORM_GIC_MAX_IRQ)
				type_str = "GIC ";
			else if (pirq < PLATFORM_GPIO_MAX_IRQ)
				type_str = "GPIO";
			else
				type_str = "VIRQ";

			/* Print GPIO bank info for GPIO IRQs */
			if (pirq >= PLATFORM_GIC_MAX_IRQ && pirq < PLATFORM_GPIO_MAX_IRQ) {
				/* Reverse conversion: IRQ -> GPIO bank:offset */
				int bank = (pirq - PIN_BASE) / GPIO_BANK_PINS;
				int offset = (pirq - PIN_BASE) % GPIO_BANK_PINS;
				char type_buf[20];

				/* offset 0-7 -> A0-A7, 8-15 -> B0-B7, 16-23 -> C0-C7, 24-31 -> D0-D7 */
				snprintf(type_buf, sizeof(type_buf), "GPIO%d_%c%d",
					 bank, 'A' + (offset / 8), offset % 8);
				printf(" %3d  %-12s  %c   0x%016lx  %-13s  %-16s  %6u  %5d\n",
				       pirq, type_buf,
				       irq_desc[pirq].flag & IRQ_FLG_ENABLE ? 'Y' : 'N',
				       (ulong)handler->handle_irq, drv_name,
				       dev ? dev->name : "N/A",
				       handler->count, irq_desc[pirq].is_shared);
			} else {
				printf(" %3d  %-12s  %c   0x%016lx  %-13s  %-16s  %6u  %5d\n",
				       pirq, type_str,
				       irq_desc[pirq].flag & IRQ_FLG_ENABLE ? 'Y' : 'N',
				       (ulong)handler->handle_irq, drv_name,
				       dev ? dev->name : "N/A",
				       handler->count, irq_desc[pirq].is_shared);
			}
		}

		virqs_show(pirq);
	}

	if (irqchip.gic->irq_reg_dump) {
		printf("\n\n");
		irqchip.gic->irq_reg_dump();
	}

	return 0;
}

U_BOOT_CMD(
	dump_irqs, 1, 1, do_dump_irqs, "Dump IRQs", ""
);
