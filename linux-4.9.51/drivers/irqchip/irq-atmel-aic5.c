/*
 * Atmel AT91 AIC5 (Advanced Interrupt Controller) driver
 *
 *  Copyright (C) 2004 SAN People
 *  Copyright (C) 2004 ATMEL
 *  Copyright (C) Rick Bronson
 *  Copyright (C) 2014 Free Electrons
 *
 *  Author: Boris BREZILLON <boris.brezillon@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/bitmap.h>
#include <linux/types.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <asm/exception.h>
#include <asm/mach/irq.h>

#include "irq-atmel-aic-common.h"

/* Number of irq lines managed by AIC */
#define NR_AIC5_IRQS	128

#define AT91_AIC5_SSR		0x0
#define AT91_AIC5_INTSEL_MSK	(0x7f << 0)

#define AT91_AIC5_SMR			0x4

#define AT91_AIC5_SVR			0x8
#define AT91_AIC5_IVR			0x10
#define AT91_AIC5_FVR			0x14
#define AT91_AIC5_ISR			0x18

#define AT91_AIC5_IPR0			0x20
#define AT91_AIC5_IPR1			0x24
#define AT91_AIC5_IPR2			0x28
#define AT91_AIC5_IPR3			0x2c
#define AT91_AIC5_IMR			0x30
#define AT91_AIC5_CISR			0x34

#define AT91_AIC5_IECR			0x40
#define AT91_AIC5_IDCR			0x44
#define AT91_AIC5_ICCR			0x48
#define AT91_AIC5_ISCR			0x4c
#define AT91_AIC5_EOICR			0x38
#define AT91_AIC5_SPU			0x3c
#define AT91_AIC5_DCR			0x6c

#define AT91_AIC5_FFER			0x50
#define AT91_AIC5_FFDR			0x54
#define AT91_AIC5_FFSR			0x58

static struct irq_domain *aic5_domain;

static asmlinkage void __exception_irq_entry
aic5_handle(struct pt_regs *regs)
{
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(aic5_domain, 0);
	u32 irqnr;
	u32 irqstat;

	irqnr = irq_reg_readl(bgc, AT91_AIC5_IVR);
	irqstat = irq_reg_readl(bgc, AT91_AIC5_ISR);

	if (!irqstat)
		irq_reg_writel(bgc, 0, AT91_AIC5_EOICR);
	else
		ipipe_handle_domain_irq(aic5_domain, irqnr, regs);
}

static void aic5_mask(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	unsigned long flags;

	/*
	 * Disable interrupt on AIC5. We always take the lock of the
	 * first irq chip as all chips share the same registers.
	 */
	flags = irq_gc_lock(bgc);
	ipipe_lock_irq(d->irq);
	irq_reg_writel(gc, d->hwirq, AT91_AIC5_SSR);
	irq_reg_writel(gc, 1, AT91_AIC5_IDCR);
	gc->mask_cache &= ~d->mask;
	irq_gc_unlock(bgc, flags);
}

static void aic5_unmask(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	unsigned long flags;

	/*
	 * Enable interrupt on AIC5. We always take the lock of the
	 * first irq chip as all chips share the same registers.
	 */
	flags = irq_gc_lock(bgc);
	irq_reg_writel(gc, d->hwirq, AT91_AIC5_SSR);
	irq_reg_writel(gc, 1, AT91_AIC5_IECR);
	gc->mask_cache |= d->mask;
	ipipe_unlock_irq(d->irq);
	irq_gc_unlock(bgc, flags);
}

#ifdef CONFIG_IPIPE
int at91_gpio_enable_irqdesc(struct ipipe_domain *ipd, unsigned irq);
int at91_gpio_disable_irqdesc(struct ipipe_domain *ipd, unsigned irq);
void at91_gpio_mute(void);
void at91_gpio_unmute(void);

static void aic5_hold(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_domain_chip_generic *dgc = domain->gc;
	struct irq_chip_generic *gc = dgc->gc[0];

	irq_reg_writel(gc, d->hwirq, AT91_AIC5_SSR);
	irq_reg_writel(gc, 1, AT91_AIC5_IDCR);
	irq_reg_writel(gc, 0, AT91_AIC5_EOICR);
}

static void aic5_release(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_domain_chip_generic *dgc = domain->gc;
	struct irq_chip_generic *gc = dgc->gc[0];
	unsigned long flags;

	flags = irq_gc_lock(gc);
	irq_reg_writel(gc, d->hwirq, AT91_AIC5_SSR);
	irq_reg_writel(gc, 1, AT91_AIC5_IECR);
	irq_gc_unlock(gc, flags);
}

static void at91_enable_irqdesc(struct ipipe_domain *ipd, unsigned irq)
{
	at91_gpio_enable_irqdesc(ipd, irq);
}

static void at91_disable_irqdesc(struct ipipe_domain *ipd, unsigned irq)
{
	at91_gpio_disable_irqdesc(ipd, irq);
}

static void at91_mute_pic(void)
{
	at91_gpio_mute();
}

static void at91_unmute_pic(void)
{
	at91_gpio_unmute();
}

static void at91_pic_muter_register(void)
{
	struct ipipe_mach_pic_muter at91_pic_muter = {
		.enable_irqdesc = at91_enable_irqdesc,
		.disable_irqdesc = at91_disable_irqdesc,
		.mute = at91_mute_pic,
		.unmute = at91_unmute_pic,
	};

	ipipe_pic_muter_register(&at91_pic_muter);
}
#endif

static int aic5_retrigger(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	unsigned long flags;

	/* Enable interrupt on AIC5 */
	flags = irq_gc_lock(bgc);
	irq_reg_writel(bgc, d->hwirq, AT91_AIC5_SSR);
	irq_reg_writel(bgc, 1, AT91_AIC5_ISCR);
	irq_gc_unlock(bgc, flags);

	return 0;
}

static int aic5_set_type(struct irq_data *d, unsigned type)
{
	struct irq_domain *domain = d->domain;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	unsigned long flags;
	unsigned int smr;
	int ret;

	flags = irq_gc_lock(bgc);
	irq_reg_writel(bgc, d->hwirq, AT91_AIC5_SSR);
	smr = irq_reg_readl(bgc, AT91_AIC5_SMR);
	ret = aic_common_set_type(d, type, &smr);
	if (!ret)
		irq_reg_writel(bgc, smr, AT91_AIC5_SMR);
	irq_gc_unlock(bgc, flags);

	return ret;
}

#ifdef CONFIG_PM
static void aic5_suspend(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_domain_chip_generic *dgc = domain->gc;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	unsigned long flags;
	int i;
	u32 mask;

	flags = irq_gc_lock(bgc);
	for (i = 0; i < dgc->irqs_per_chip; i++) {
		mask = 1 << i;
		if ((mask & gc->mask_cache) == (mask & gc->wake_active))
			continue;

		irq_reg_writel(bgc, i + gc->irq_base, AT91_AIC5_SSR);
		if (mask & gc->wake_active)
			irq_reg_writel(bgc, 1, AT91_AIC5_IECR);
		else
			irq_reg_writel(bgc, 1, AT91_AIC5_IDCR);
	}
	irq_gc_unlock(bgc, flags);
}

static void aic5_resume(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_domain_chip_generic *dgc = domain->gc;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	unsigned long flags;
	int i;
	u32 mask;

	flags = irq_gc_lock(bgc);
	for (i = 0; i < dgc->irqs_per_chip; i++) {
		mask = 1 << i;
		if ((mask & gc->mask_cache) == (mask & gc->wake_active))
			continue;

		irq_reg_writel(bgc, i + gc->irq_base, AT91_AIC5_SSR);
		if (mask & gc->mask_cache)
			irq_reg_writel(bgc, 1, AT91_AIC5_IECR);
		else
			irq_reg_writel(bgc, 1, AT91_AIC5_IDCR);
	}
	irq_gc_unlock(bgc, flags);
}

static void aic5_pm_shutdown(struct irq_data *d)
{
	struct irq_domain *domain = d->domain;
	struct irq_domain_chip_generic *dgc = domain->gc;
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(domain, 0);
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	unsigned long flags;
	int i;

	flags = irq_gc_lock(bgc);
	for (i = 0; i < dgc->irqs_per_chip; i++) {
		irq_reg_writel(bgc, i + gc->irq_base, AT91_AIC5_SSR);
		irq_reg_writel(bgc, 1, AT91_AIC5_IDCR);
		irq_reg_writel(bgc, 1, AT91_AIC5_ICCR);
	}
	irq_gc_unlock(bgc, flags);
}
#else
#define aic5_suspend		NULL
#define aic5_resume		NULL
#define aic5_pm_shutdown	NULL
#endif /* CONFIG_PM */

static void __init aic5_hw_init(struct irq_domain *domain)
{
	struct irq_chip_generic *gc = irq_get_domain_generic_chip(domain, 0);
	int i;

	/*
	 * Perform 8 End Of Interrupt Command to make sure AIC
	 * will not Lock out nIRQ
	 */
	for (i = 0; i < 8; i++)
		irq_reg_writel(gc, 0, AT91_AIC5_EOICR);

	/*
	 * Spurious Interrupt ID in Spurious Vector Register.
	 * When there is no current interrupt, the IRQ Vector Register
	 * reads the value stored in AIC_SPU
	 */
	irq_reg_writel(gc, 0xffffffff, AT91_AIC5_SPU);

	/* No debugging in AIC: Debug (Protect) Control Register */
	irq_reg_writel(gc, 0, AT91_AIC5_DCR);

	/* Disable and clear all interrupts initially */
	for (i = 0; i < domain->revmap_size; i++) {
		irq_reg_writel(gc, i, AT91_AIC5_SSR);
		irq_reg_writel(gc, i, AT91_AIC5_SVR);
		irq_reg_writel(gc, 1, AT91_AIC5_IDCR);
		irq_reg_writel(gc, 1, AT91_AIC5_ICCR);
	}
}

static int aic5_irq_domain_xlate(struct irq_domain *d,
				 struct device_node *ctrlr,
				 const u32 *intspec, unsigned int intsize,
				 irq_hw_number_t *out_hwirq,
				 unsigned int *out_type)
{
	struct irq_chip_generic *bgc = irq_get_domain_generic_chip(d, 0);
	unsigned long flags;
	unsigned smr;
	int ret;

	if (!bgc)
		return -EINVAL;

	ret = aic_common_irq_domain_xlate(d, ctrlr, intspec, intsize,
					  out_hwirq, out_type);
	if (ret)
		return ret;

	irq_gc_lock_irqsave(bgc, flags);
	irq_reg_writel(bgc, *out_hwirq, AT91_AIC5_SSR);
	smr = irq_reg_readl(bgc, AT91_AIC5_SMR);
	aic_common_set_priority(intspec[2], &smr);
	irq_reg_writel(bgc, smr, AT91_AIC5_SMR);
	irq_gc_unlock_irqrestore(bgc, flags);

	return ret;
}

static const struct irq_domain_ops aic5_irq_ops = {
	.map	= irq_map_generic_chip,
	.xlate	= aic5_irq_domain_xlate,
};

static void __init sama5d3_aic_irq_fixup(struct device_node *root)
{
	aic_common_rtc_irq_fixup(root);
}

static const struct of_device_id aic5_irq_fixups[] __initconst = {
	{ .compatible = "atmel,sama5d3", .data = sama5d3_aic_irq_fixup },
	{ .compatible = "atmel,sama5d4", .data = sama5d3_aic_irq_fixup },
	{ /* sentinel */ },
};

static int __init aic5_of_init(struct device_node *node,
			       struct device_node *parent,
			       int nirqs)
{
	struct irq_chip_generic *gc;
	struct irq_domain *domain;
	int nchips;
	int i;

	if (nirqs > NR_AIC5_IRQS)
		return -EINVAL;

	if (aic5_domain)
		return -EEXIST;

	domain = aic_common_of_init(node, &aic5_irq_ops, "atmel-aic5",
				    nirqs, aic5_irq_fixups);
	if (IS_ERR(domain))
		return PTR_ERR(domain);

	aic5_domain = domain;
	nchips = aic5_domain->revmap_size / 32;
	for (i = 0; i < nchips; i++) {
		gc = irq_get_domain_generic_chip(domain, i * 32);

		gc->chip_types[0].regs.eoi = AT91_AIC5_EOICR;
		gc->chip_types[0].chip.irq_mask = aic5_mask;
		gc->chip_types[0].chip.irq_unmask = aic5_unmask;
		gc->chip_types[0].chip.irq_retrigger = aic5_retrigger;
		gc->chip_types[0].chip.irq_set_type = aic5_set_type;
		gc->chip_types[0].chip.irq_suspend = aic5_suspend;
		gc->chip_types[0].chip.irq_resume = aic5_resume;
		gc->chip_types[0].chip.irq_pm_shutdown = aic5_pm_shutdown;
#ifdef CONFIG_IPIPE
		gc->chip_types[0].chip.irq_hold	= aic5_hold;
		gc->chip_types[0].chip.irq_release = aic5_release;
#endif
	}

	aic5_hw_init(domain);
	set_handle_irq(aic5_handle);

#ifdef CONFIG_IPIPE
	at91_pic_muter_register();
#endif

	return 0;
}

#define NR_SAMA5D2_IRQS		77

static int __init sama5d2_aic5_of_init(struct device_node *node,
				       struct device_node *parent)
{
	return aic5_of_init(node, parent, NR_SAMA5D2_IRQS);
}
IRQCHIP_DECLARE(sama5d2_aic5, "atmel,sama5d2-aic", sama5d2_aic5_of_init);

#define NR_SAMA5D3_IRQS		48

static int __init sama5d3_aic5_of_init(struct device_node *node,
				       struct device_node *parent)
{
	return aic5_of_init(node, parent, NR_SAMA5D3_IRQS);
}
IRQCHIP_DECLARE(sama5d3_aic5, "atmel,sama5d3-aic", sama5d3_aic5_of_init);

#define NR_SAMA5D4_IRQS		68

static int __init sama5d4_aic5_of_init(struct device_node *node,
				       struct device_node *parent)
{
	return aic5_of_init(node, parent, NR_SAMA5D4_IRQS);
}
IRQCHIP_DECLARE(sama5d4_aic5, "atmel,sama5d4-aic", sama5d4_aic5_of_init);
