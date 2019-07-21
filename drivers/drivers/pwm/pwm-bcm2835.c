/*
 * Copyright 2014 Bart Tanghe <bart.tanghe@thomasmore.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/pwm_dev.h>

#define PWM_CONTROL				0x000
#define PWM_STATUS				0x004
#define PWM_DMACONF				0x008
#define PWM_FIFO				0x018
#define PWM_CONTROL_SHIFT(x)	((x) * 8)
#define PWM_CONTROL_MASK		0xFF
#define PWM_MODE				0x80		/* set timer in PWM mode */
#define PWM_ENABLE				(1 << 0)
#define PWM_SERIALISER			(1 << 1)
#define PWM_REPEATFIFO			(1 << 2)
#define PWM_SILENCE				(1 << 3)
#define PWM_POLARITY			(1 << 4)
#define PWM_USEFIFO				(1 << 5)
#define PWM_CLEARFIFO			(1 << 6)
#define PWM_MARKSPACE			(1 << 7)
#define DMA_ENABLE				(1 << 31)

#define PERIOD(x)				(((x) * 0x10) + 0x10)
#define DUTY(x)					(((x) * 0x10) + 0x14)

#define MIN_PERIOD				108		/* 9.2 MHz max. PWM clock */

#define CONST_DELAY				1000

extern const char *__clk_get_name(const struct clk *);

struct bcm2835_pwm {
	struct pwm_chip chip;
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
};

static inline struct bcm2835_pwm *to_bcm2835_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct bcm2835_pwm, chip);
}

static int bcm2835_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);
	value &= ~(PWM_CONTROL_MASK << PWM_CONTROL_SHIFT(pwm->hwpwm));
	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);

	return 0;
}

static void bcm2835_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);
	value &= ~(PWM_CONTROL_MASK << PWM_CONTROL_SHIFT(pwm->hwpwm));
	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);
}

static int bcm2835_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			      int duty_ns, int period_ns)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	unsigned long rate = clk_get_rate(pc->clk);
	unsigned long scaler;
	// u32 value;

	if (!rate) {
		dev_err(pc->dev, "failed to get clock rate\n");
		return -EINVAL;
	}

	scaler = NSEC_PER_SEC / rate;

	if (period_ns <= MIN_PERIOD) {
		dev_err(pc->dev, "period %d not supported, minimum %d\n",
			period_ns, MIN_PERIOD);
		return -EINVAL;
	}

	// value = readl(pc->base + PWM_CONTROL);
	// value |= (PWM_MODE << PWM_CONTROL_SHIFT(pwm->hwpwm));
	// writel(value, pc->base + PWM_CONTROL);
	// udelay(10);

	// writel(duty_ns / scaler, pc->base + DUTY(pwm->hwpwm));
	// writel(period_ns / scaler, pc->base + PERIOD(pwm->hwpwm));

	// writel(duty_ns, pc->base + DUTY(pwm->hwpwm));
	// udelay(10);
	// writel(period_ns, pc->base + PERIOD(pwm->hwpwm));
	// udelay(10);

	return 0;
}

static int bcm2835_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);
	value |= PWM_ENABLE << PWM_CONTROL_SHIFT(pwm->hwpwm);
	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);

	return 0;
}

static void bcm2835_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);
	value &= ~(PWM_ENABLE << PWM_CONTROL_SHIFT(pwm->hwpwm));
	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);
}

static int bcm2835_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm,
								enum pwm_polarity polarity)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);

	if (polarity == PWM_POLARITY_NORMAL)
		value &= ~(PWM_POLARITY << PWM_CONTROL_SHIFT(pwm->hwpwm));
	else
		value |= PWM_POLARITY << PWM_CONTROL_SHIFT(pwm->hwpwm);

	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);

	return 0;
}

static unsigned long bcm2835_get_clock(struct pwm_chip *chip)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	unsigned long rate = clk_get_rate(pc->clk);

	// struct clk *parent_clk = clk_get_parent(pc->clk);
	// printk(KERN_INFO "Clock name: %s\tParent name: %s\n", __clk_get_name(pc->clk), __clk_get_name(parent_clk));

	if (!rate) {
		dev_err(pc->dev, "failed to get clock rate\n");
		return -EINVAL;
	}
	
	return rate;
}

static int bcm2835_set_clock(struct pwm_chip *chip, struct pwm_device *pwm,
					  		 unsigned long divisor)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	unsigned long rate = clk_get_rate(pc->clk);
	unsigned long new_rate;
	int ret = 0;
	u32 value;

	value = readl(pc->base + PWM_CONTROL);
	if(value & (PWM_ENABLE << PWM_CONTROL_SHIFT(pwm->hwpwm))) {
		dev_err(pc->dev, "PWM Enabled, don't change clock rate during job\n");
		return -EPERM;
	}

	value = readl(pc->base + PWM_DMACONF);
	if(value & DMA_ENABLE) {
		dev_err(pc->dev, "DMA Enabled, don't change clock rate during job\n");
		return -EPERM;
	}

	if (!rate) {
		dev_err(pc->dev, "failed to get clock rate\n");
		return -EINVAL;
	}

	if(divisor < 0 || divisor > 4095) {
		dev_err(pc->dev, "divisor %lu not supported, minimum 0, maximum 4095\n",
			divisor);
		return -EINVAL;
	}

	new_rate = rate / divisor;
	ret = clk_set_rate(pc->clk, new_rate);

	return ret;
}

static int bcm2835_set_dma(struct pwm_chip *chip,
						   bool dma_enable, u8 dma_dreq, u8 dma_panic)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	if((dma_dreq < 0x00 || dma_dreq > 0xFF)) {
		dev_err(pc->dev, "DMA Data Request Threshold %u not supported, minimum 0x00, maximum 0xFF\n",
						(unsigned int)dma_dreq);
		return -EINVAL;
	}

	if((dma_panic < 0x00 || dma_panic > 0xFF)) {
		dev_err(pc->dev, "DMA Panic Threshold %u not supported, minimum 0x00, maximum 0xFF\n",
						(unsigned int)dma_panic);
		return -EINVAL;
	}

	value = readl(pc->base + PWM_DMACONF);
	
	// value |=  ((dma_dreq & 0x000000FF) << 0);
	// value |=  ((dma_panic & 0x000000FF) << 8);

	if(dma_enable == true) value |=  DMA_ENABLE;
	else			   	   value &= ~DMA_ENABLE;

	// printk(KERN_INFO "PWM DMAC: 0x%x\n", value);

	writel(value, pc->base + PWM_DMACONF);
	udelay(CONST_DELAY);

	return 0;
}

static int bcm2835_serialiser_config(struct pwm_chip *chip, struct pwm_device *pwm,
			      					 bool serialiser, bool silence_bit, bool use_fifo)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);

	if(serialiser == true) value |=  (PWM_SERIALISER << PWM_CONTROL_SHIFT(pwm->hwpwm));
	else				   value &= ~(PWM_SERIALISER << PWM_CONTROL_SHIFT(pwm->hwpwm));

	if(silence_bit == true) value |=  (PWM_SILENCE << PWM_CONTROL_SHIFT(pwm->hwpwm));
	else					value &= ~(PWM_SILENCE << PWM_CONTROL_SHIFT(pwm->hwpwm));

	if(use_fifo == true) value |=  (PWM_USEFIFO << PWM_CONTROL_SHIFT(pwm->hwpwm));
	else				 value &= ~(PWM_USEFIFO << PWM_CONTROL_SHIFT(pwm->hwpwm));

	// value |= ~(PWM_REPEATFIFO << PWM_CONTROL_SHIFT(pwm->hwpwm));

	// printk(KERN_INFO "PWM Control Value: 0x%x\n", value);

	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);

	// value = readl(pc->base + PERIOD(pwm->hwpwm));
	// printk(KERN_INFO "PWM Range Value: 0x%x\n", value);

	// value = readl(pc->base + DUTY(pwm->hwpwm));
	// printk(KERN_INFO "PWM Data Value: 0x%x\n", value);

	return 0;
}

static int bcm2835_set_range(struct pwm_chip *chip, struct pwm_device *pwm,
							 u32 value)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);

	if(value < 0) {
		dev_err(pc->dev, "Range Value must be positive\n");
		return -EINVAL;
	}

	writel(value, pc->base + PERIOD(pwm->hwpwm));
	udelay(CONST_DELAY);

	return 0;
}

static int bcm2835_set_data(struct pwm_chip *chip, struct pwm_device *pwm,
							u32 value)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);

	if(value < 0) {
		dev_err(pc->dev, "Data Value must be positive\n");
		return -EINVAL;
	}

	writel(value, pc->base + DUTY(pwm->hwpwm));
	udelay(CONST_DELAY);

	return 0;
}

static int bcm2835_set_fifo(struct pwm_chip *chip, u32 value)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);

	if(value < 0) {
		dev_err(pc->dev, "FIFO Value must be positive\n");
		return -EINVAL;
	}

	writel(value, pc->base + PWM_FIFO);
	udelay(CONST_DELAY);

	return 0;
}

static unsigned int bcm2835_get_status(struct pwm_chip *chip)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_STATUS);

	return value;
}

static int bcm2835_clear_status(struct pwm_chip *chip)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = 0x1FC;

	writel(value, pc->base + PWM_STATUS);
	udelay(CONST_DELAY);
	
	return 0;
}

static int bcm2835_clear_fifo(struct pwm_chip *chip)
{
	struct bcm2835_pwm *pc = to_bcm2835_pwm(chip);
	u32 value;

	value = readl(pc->base + PWM_CONTROL);
	value |= PWM_CLEARFIFO;
	writel(value, pc->base + PWM_CONTROL);
	udelay(CONST_DELAY);
	
	return 0;
}

static const struct pwm_ops bcm2835_pwm_ops = {
	.request = bcm2835_pwm_request,
	.free = bcm2835_pwm_free,
	.config = bcm2835_pwm_config,
	.serialiser = bcm2835_serialiser_config,
	.get_clock = bcm2835_get_clock,
	.set_fifo = bcm2835_set_fifo,
	.set_dma = bcm2835_set_dma,
	.get_status = bcm2835_get_status,
	.clear_status = bcm2835_clear_status,
	.clear_fifo = bcm2835_clear_fifo,
	.enable = bcm2835_pwm_enable,
	.disable = bcm2835_pwm_disable,
	.set_polarity = bcm2835_set_polarity,
	.owner = THIS_MODULE,
};

static int bcm2835_pwm_probe(struct platform_device *pdev)
{
	struct bcm2835_pwm *pc;
	struct resource *res;
	int ret;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	pc->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pc->base))
		return PTR_ERR(pc->base);

	pc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pc->clk)) {
		dev_err(&pdev->dev, "clock not found: %ld\n", PTR_ERR(pc->clk));
		return PTR_ERR(pc->clk);
	}

	ret = clk_prepare_enable(pc->clk);
	if (ret)
		return ret;

	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &bcm2835_pwm_ops;
	pc->chip.npwm = 2;
	pc->chip.of_xlate = of_pwm_xlate_with_flags;
	pc->chip.of_pwm_n_cells = 3;

	platform_set_drvdata(pdev, pc);

	ret = pwmchip_add(&pc->chip);
	if (ret < 0)
		goto add_fail;

	return 0;

add_fail:
	clk_disable_unprepare(pc->clk);
	return ret;
}

static int bcm2835_pwm_remove(struct platform_device *pdev)
{
	struct bcm2835_pwm *pc = platform_get_drvdata(pdev);

	clk_disable_unprepare(pc->clk);

	return pwmchip_remove(&pc->chip);
}

static const struct of_device_id bcm2835_pwm_of_match[] = {
	{ .compatible = "brcm,bcm2835-pwm", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bcm2835_pwm_of_match);

static struct platform_driver bcm2835_pwm_driver = {
	.driver = {
		.name = "bcm2835-pwm",
		.of_match_table = bcm2835_pwm_of_match,
	},
	.probe = bcm2835_pwm_probe,
	.remove = bcm2835_pwm_remove,
};
module_platform_driver(bcm2835_pwm_driver);

MODULE_AUTHOR("Bart Tanghe <bart.tanghe@thomasmore.be>");
MODULE_DESCRIPTION("Broadcom BCM2835 PWM driver");
MODULE_LICENSE("GPL v2");
