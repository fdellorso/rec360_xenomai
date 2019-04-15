#include <linux/fs.h>
#include <linux/dma.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <cobalt/kernel/rtdm/driver.h>


static rtdm_irq_t irq_rtdm_dma_pwm;

static int dma_pwm_handler(rtdm_irq_t *irq)
{


	return RTDM_IRQ_HANDLED;
}

static int __init dma_pwm_init (void)
{
	int ret = 0;

	

	return ret;
}

static void __exit dma_pwm_exit (void)
{

}

module_init(dma_pwm_init);
module_exit(dma_pwm_exit);
MODULE_LICENSE("GPL");
