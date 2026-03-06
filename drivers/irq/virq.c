/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/u-boot-arm.h>
#include <irq-generic.h>
#include "irq-internal.h"

DECLARE_GLOBAL_DATA_PTR;

static LIST_HEAD(virq_desc_head);
static u32 virq_id = PLATFORM_MAX_IRQ;

static u32 virq_id_alloc(void)
{
	return ++virq_id;
}

struct virq_data {
	int irq;
	u32 flag;
	u32 count;

	/* VIRQ doesn't need shared support - one handler per virq */
	interrupt_handler_t *handle_irq;
	void *data;
};

/* The structure to maintail the irqchip and child virqs */
struct virq_desc {
	struct virq_chip *chip;		/* irq chip */
	struct virq_data *virqs;	/* child irq data list */
	struct udevice *parent;		/* parent device */
	int pirq;			/* parent irq */
	int use_count;			/* enable count */
	int irq_base;			/* child irq base */
	int irq_end;			/* child irq end */
	uint reg_stride;
	uint unalign_reg_idx;
	uint unalign_reg_stride;
	uint *status_buf;

	struct list_head node;
};

static struct virq_desc *find_virq_desc(int irq)
{
	struct virq_desc *desc;
	struct list_head *node;

	list_for_each(node, &virq_desc_head) {
		desc = list_entry(node, struct virq_desc, node);
		if (irq >= desc->irq_base && irq <= desc->irq_end)
			return desc;
	}

	return NULL;
}

int virq_to_irq(struct virq_chip *chip, int virq)
{
	struct virq_desc *desc;
	struct list_head *node;
	int irq;

	if (!chip)
		return -EINVAL;

	list_for_each(node, &virq_desc_head) {
		desc = list_entry(node, struct virq_desc, node);
		if (desc->chip == chip) {
			irq = desc->irq_base + virq;
			if (irq >= desc->irq_base && irq <= desc->irq_end)
				return irq;
		}
	}

	return -ENOENT;
}

int bad_virq(int irq)
{
	return !find_virq_desc(irq);
}

void virqs_show(int pirq)
{
	struct virq_data *vdata;
	struct virq_desc *desc;
	struct udevice *dev;
	struct list_head *desc_node;
	int num;
	int i;

	/* Iterate through ALL virq_desc that share this parent IRQ */
	list_for_each(desc_node, &virq_desc_head) {
		desc = list_entry(desc_node, struct virq_desc, node);

		/* Skip if this desc doesn't match our parent IRQ */
		if (desc->pirq != pirq)
			continue;

		vdata = desc->virqs;
		num = desc->irq_end - desc->irq_base + 1;

		for (i = 0; i < num; i++) {
			/* Skip if no handler registered */
			if (!vdata[i].handle_irq)
				continue;

			dev = (struct udevice *)vdata[i].data;
			printf(" %3d  %-12s  %c   0x%016lx  %-13s  %-16s  %6u  (parent: %d)\n",
			       vdata[i].irq, "VIRQ",
			       vdata[i].flag & IRQ_FLG_ENABLE ? 'Y' : 'N',
			       (ulong)vdata[i].handle_irq,
			       (dev && dev->driver) ? dev->driver->name : "N/A",
			       dev ? dev->name : "N/A",
			       vdata[i].count, pirq);
		}
	}
}

int virq_install_handler(int irq, interrupt_handler_t *handler, void *data)
{
	struct virq_desc *desc;
	int virq;

	if (!handler)
		return -EINVAL;

	desc = find_virq_desc(irq);
	if (!desc)
		return -ENOENT;

	virq = irq - desc->irq_base;

	/* VIRQ doesn't support shared handlers - direct assignment */
	if (desc->virqs[virq].handle_irq)
		return -EBUSY;

	desc->virqs[virq].handle_irq = handler;
	desc->virqs[virq].data = data;
	desc->virqs[virq].count = 0;

	return 0;
}

void virq_free_handler(int irq)
{
	struct virq_desc *desc;
	int virq;

	desc = find_virq_desc(irq);
	if (!desc)
		return;

	virq = irq - desc->irq_base;

	/* VIRQ doesn't support shared handlers - direct clear */
	desc->virqs[virq].handle_irq = NULL;
	desc->virqs[virq].data = NULL;
}

static uint reg_base_get(struct virq_desc *desc, uint reg_base, int idx)
{
	int reg_addr;

	if (idx <= desc->unalign_reg_idx) {
		reg_addr = reg_base + (idx * desc->unalign_reg_stride);
	} else {
		reg_addr = reg_base +
			   (desc->unalign_reg_idx * desc->unalign_reg_stride);
		reg_addr += (idx - desc->unalign_reg_idx) * desc->reg_stride;
	}

	return reg_addr;
}

void virq_chip_generic_handler(int pirq, void *pdata)
{
	struct virq_chip *chip;
	struct virq_desc *desc;
	struct virq_data *vdata;
	struct udevice *parent;
	struct list_head *desc_node;
	uint status_reg;
	int irq;
	int ret;
	int i;

	/*
	 * Iterate through ALL virq_desc that share this parent IRQ.
	 * This handles the case where multiple PMICs share the same GPIO.
	 */
	list_for_each(desc_node, &virq_desc_head) {
		desc = list_entry(desc_node, struct virq_desc, node);

		/* Skip if this desc doesn't match our parent IRQ */
		if (desc->pirq != pirq)
			continue;

		chip = desc->chip;
		vdata = desc->virqs;
		parent = desc->parent;

		if (!chip || !vdata || !parent)
			continue;

		/* Read all status register for this device */
		for (i = 0; i < chip->num_regs; i++) {
			int val;

			status_reg = reg_base_get(desc, chip->status_base, i);
			val = chip->read(parent, status_reg);
			if (val < 0) {
				printf("%s: Read status register 0x%x failed, ret=%d\n",
				       __func__, status_reg, val);
			}
			desc->status_buf[i] = (uint)val;
		}

		/* Handle all virq - VIRQ doesn't support shared handlers */
		for (i = 0; i < chip->num_irqs; i++) {
			if (desc->status_buf[chip->irqs[i].reg_offset] &
			    chip->irqs[i].mask) {
				irq = vdata[i].irq;

				if (vdata[i].handle_irq) {
					vdata[i].count++;
					vdata[i].handle_irq(irq, vdata[i].data);
				}
			}
		}

		/* Clear all status register for this device */
		for (i = 0; i < chip->num_regs; i++) {
			status_reg = reg_base_get(desc, chip->status_base, i);
			ret = chip->write(parent, status_reg, ~0U);
			if (ret)
				printf("%s: Clear status register 0x%x failed, ret=%d\n",
				       __func__, status_reg, ret);
		}
	}
}

int virq_add_chip(struct udevice *dev, struct virq_chip *chip, int irq)
{
	struct virq_data *vdata;
	struct virq_desc *desc;
	uint *status_buf;
	uint status_reg;
	uint mask_reg;
	int ret;
	int i;

	if (irq < 0)
		return -EINVAL;

	desc = (struct virq_desc *)malloc(sizeof(*desc));
	if (!desc)
		return -ENOMEM;

	vdata = (struct virq_data *)calloc(sizeof(*vdata), chip->num_irqs);
	if (!vdata) {
		ret = -ENOMEM;
		free(desc);
		return ret;
	}

	status_buf = (uint *)calloc(sizeof(*status_buf), chip->num_regs);
	if (!status_buf) {
		ret = -ENOMEM;
		free(vdata);
		free(desc);
		return ret;
	}

	for (i = 0; i < chip->num_irqs; i++) {
		vdata[i].irq = virq_id_alloc();
		vdata[i].flag = 0;
		vdata[i].count = 0;
		vdata[i].handle_irq = NULL;
		vdata[i].data = NULL;
	}

	desc->parent = dev;
	desc->pirq = irq;
	desc->chip = chip;
	desc->virqs = vdata;
	desc->use_count = 0;
	desc->irq_base = vdata[0].irq;
	desc->irq_end = vdata[chip->num_irqs - 1].irq;
	desc->status_buf = status_buf;
	desc->reg_stride = chip->irq_reg_stride ? : 1;
	desc->unalign_reg_stride = chip->irq_unalign_reg_stride ? : 1;
	desc->unalign_reg_idx = chip->irq_unalign_reg_stride ?
					chip->irq_unalign_reg_idx : 0;
	list_add_tail(&desc->node, &virq_desc_head);

	/* Mask all register */
	for (i = 0; i < chip->num_regs; i++) {
		mask_reg = reg_base_get(desc, chip->mask_base, i);
		ret = chip->write(dev, mask_reg, ~0U);
		if (ret)
			printf("%s: Set mask register 0x%x failed, ret=%d\n",
			       __func__, mask_reg, ret);
	}

	/* Clear all status */
	for (i = 0; i < chip->num_regs; i++) {
		status_reg = reg_base_get(desc, chip->status_base, i);
		ret = chip->write(dev, status_reg, ~0U);
		if (ret)
			printf("%s: Clear status register 0x%x failed, ret=%d\n",
			       __func__, status_reg, ret);
	}

	/* Add parent irq into interrupt framework with generic virq handler */
	irq_install_handler_flags(irq, virq_chip_generic_handler, dev, IRQF_SHARED);

	return irq_handler_disable(irq);
}

static int virq_init(void)
{
	INIT_LIST_HEAD(&virq_desc_head);
	return 0;
}

static int __virq_enable(int irq, int enable)
{
	struct virq_chip *chip;
	struct virq_desc *desc;
	uint mask_reg, mask_val;
	uint reg_val;
	int virq;
	int ret;

	desc = find_virq_desc(irq);
	if (!desc) {
		printf("%s: %s Invalid irq %d\n",
		       __func__, enable ? "Enable" : "Disable", irq);
		return -ENOENT;
	}

	chip = desc->chip;
	if (!chip)
		return -ENOENT;

	virq = irq - desc->irq_base;
	mask_val = chip->irqs[virq].mask;
	mask_reg = reg_base_get(desc, chip->mask_base,
				chip->irqs[virq].reg_offset);
	reg_val = chip->read(desc->parent, mask_reg);
	if (enable)
		reg_val &= ~mask_val;
	else
		reg_val |= mask_val;

	ret = chip->write(desc->parent, mask_reg, reg_val);
	if (ret) {
		printf("%s: Clear status register 0x%x failed, ret=%d\n",
		       __func__, mask_reg, ret);
		return ret;
	}

	if (enable)
		desc->virqs[virq].flag |= IRQ_FLG_ENABLE;
	else
		desc->virqs[virq].flag &= ~IRQ_FLG_ENABLE;

	return 0;
}

static int virq_enable(int irq)
{
	struct virq_desc *desc = find_virq_desc(irq);
	int ret;

	if (bad_virq(irq))
		return -EINVAL;

	ret = __virq_enable(irq, 1);
	if (!ret) {
		if (desc->use_count == 0)
			irq_handler_enable(desc->pirq);
		desc->use_count++;
	}

	return ret;
}

static int virq_disable(int irq)
{
	struct virq_desc *desc = find_virq_desc(irq);
	int ret;

	if (bad_virq(irq))
		return -EINVAL;

	ret = __virq_enable(irq, 0);
	if (!ret) {
		if (desc->use_count <= 0)
			return ret;

		if (desc->use_count == 1)
			irq_handler_disable(desc->pirq);
		desc->use_count--;
	}

	return ret;
}

struct irq_chip virq_generic_chip = {
	.name		= "virq-irq-chip",
	.irq_init	= virq_init,
	.irq_enable	= virq_enable,
	.irq_disable	= virq_disable,
};

struct irq_chip *arch_virq_get_irqchip(void)
{
	return &virq_generic_chip;
}
