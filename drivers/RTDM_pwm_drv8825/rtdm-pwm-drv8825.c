#include <linux/fs.h>
#include <linux/pwm.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <cobalt/kernel/rtdm/driver.h>

static struct pwm_device drv8825;

static struct device pwm1;

static void drv8825_handler(rtdm_timer_t *timer)
{

}

static int __init drv8825_init (void)
{
	int ret = 0;

	pwm_config(&drv8825, 10, 10);


	return ret;
}

static void __exit drv8825_exit (void)
{

}

module_init(drv8825_init);
module_exit(drv8825_exit);
MODULE_LICENSE("GPL");
