#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <cobalt/kernel/rtdm/driver.h>

// PIN38 -> GPIO20
#define GPIO_ESTOP1	20
// PIN40 -> GPIO21
#define GPIO_ESTOP2 21

static rtdm_irq_t irq_rtdm;

static int handler(rtdm_irq_t *irq)
{
    	static int value1 = 0;
		static int value2 = 0;
    	value1 = gpio_get_value(GPIO_ESTOP1);
		value2 = gpio_get_value(GPIO_ESTOP2);
		if(value1 == value2 && value1 == 0) {
    		printk(KERN_INFO "[xenomai] Emergency Stop Activated...\n");
		}
		if(value1 == value2 && value1 == 1) {
			printk(KERN_INFO "[xenomai] Emergency Stop Deactivated...\n");
		}
    	return RTDM_IRQ_HANDLED;
}

static int __init example_init (void)
{
	int ret = 0;
	int irq;
	printk(KERN_INFO "Initializing Emergency Stop RTDM...\n");

	irq = gpio_to_irq(GPIO_ESTOP1);

	if ((ret = gpio_request(GPIO_ESTOP1, THIS_MODULE->name)) != 0) {
		printk(KERN_ERR "ERROR in gpio_request()\n");
		return ret;
	}
	if ((ret = gpio_request(GPIO_ESTOP2, THIS_MODULE->name)) != 0) {
		printk(KERN_ERR "ERROR in gpio_request()\n");
		return ret;
	}
	if ((ret = gpio_direction_input(GPIO_ESTOP1)) != 0) {
		printk(KERN_ERR "ERROR in gpio_direction_input()\n");
    	gpio_free(GPIO_ESTOP1);
		return ret;
	}
	if ((ret = gpio_direction_input(GPIO_ESTOP2)) != 0) {
		printk(KERN_ERR "ERROR in gpio_direction_input()\n");
		gpio_free(GPIO_ESTOP1);
		gpio_free(GPIO_ESTOP2);
		return ret;
	}

	irq_set_irq_type(irq, IRQ_TYPE_EDGE_BOTH);

	if ((ret = rtdm_irq_request(&irq_rtdm, irq, handler, RTDM_IRQTYPE_EDGE,
								THIS_MODULE->name, NULL)) != 0) {
		printk(KERN_ERR "ERROR in rtdm_irq_request()\n");
		gpio_free(GPIO_ESTOP1);
		gpio_free(GPIO_ESTOP2);
		return ret;
	}

	return 0;
}

static void __exit example_exit (void)
{
	rtdm_irq_free(&irq_rtdm);
	gpio_free(GPIO_ESTOP1);
	gpio_free(GPIO_ESTOP2);
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
