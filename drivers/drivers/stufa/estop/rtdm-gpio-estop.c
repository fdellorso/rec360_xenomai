#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <cobalt/kernel/rtdm/driver.h>

#define GPIO_ESTOP1	20				// PIN38 -> GPIO20
#define GPIO_ESTOP2 21				// PIN40 -> GPIO21
static rtdm_irq_t irq_rtdm_estop;

#define PERIOD_NSEC	5000000			// 5ms
static rtdm_timer_t timer_estop;

#define FAKE_EVENT_LIMIT 100
static int fake_event_count = 0;

static void timer_handler(rtdm_timer_t *timer)
{
	int value1 = 0;
	int value2 = 0;

	// rtdm_timer_stop (&timer_estop);

	value1 = gpio_get_value(GPIO_ESTOP1);
	value2 = gpio_get_value(GPIO_ESTOP2);

	if(value1 == 1 && value2 == 0) {
		printk(KERN_INFO "[xenomai] Emergency Stop Activated...\n");
	}
	if(value1 == 0 && value2 == 1) {
		printk(KERN_INFO "[xenomai] Emergency Stop Deactivated...\n");
	}
	if(value1 == value2) {
		fake_event_count++;
	}
	if(fake_event_count >= FAKE_EVENT_LIMIT) {
		// TODO Send Warning message to User
		fake_event_count = 0;
	}

	// printk(KERN_INFO "[xenomai] EStop1: /%d\tEStop2: /%d\n", value1, value2);

	if((rtdm_irq_enable(&irq_rtdm_estop)) != 0) {
		printk(KERN_ERR "ERROR in rtdm_irq_enable()\n");
	}
}

static int estop_handler(rtdm_irq_t *irq)
{
	if((rtdm_irq_disable(&irq_rtdm_estop)) != 0) {
		printk(KERN_ERR "ERROR in rtdm_irq_disable()\n");
		return RTDM_IRQ_HANDLED;
	}

	// printk(KERN_INFO "[xenomai] Emergency Stop Timer Start...\n");
	if ((rtdm_timer_start (&timer_estop, PERIOD_NSEC, 0, RTDM_TIMERMODE_RELATIVE)) !=0) {
		printk(KERN_ERR "ERROR in rtdm_timer_start()\n");
    	rtdm_timer_destroy(&timer_estop);
	}
	
	return RTDM_IRQ_HANDLED;
}

static int __init estop_init (void)
{
	int ret = 0;
	int irq;

	printk(KERN_INFO "Initializing Emergency Stop RTDM...\n");

	irq = gpio_to_irq(GPIO_ESTOP1);


	if ((ret = rtdm_timer_init(&timer_estop, timer_handler, "timer_estop")) != 0) {
		printk(KERN_ERR "ERROR in rtdm_timer_init()\n");
        return ret;
	}
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

	if ((ret = rtdm_irq_request(&irq_rtdm_estop, irq, estop_handler, RTDM_IRQTYPE_EDGE,
								THIS_MODULE->name, NULL)) != 0) {
		printk(KERN_ERR "ERROR in rtdm_irq_request()\n");
		gpio_free(GPIO_ESTOP1);
		gpio_free(GPIO_ESTOP2);
		return ret;
	}

	return 0;
}

static void __exit estop_exit (void)
{
	printk(KERN_INFO "Deinitializing Emergency Stop RTDM...\n");

	rtdm_timer_destroy(&timer_estop);
	rtdm_irq_free(&irq_rtdm_estop);
	gpio_free(GPIO_ESTOP1);
	gpio_free(GPIO_ESTOP2);
}

module_init(estop_init);
module_exit(estop_exit);
MODULE_LICENSE("GPL");
