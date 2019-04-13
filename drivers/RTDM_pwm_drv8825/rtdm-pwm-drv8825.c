#include <linux/fs.h>
#include <linux/pwm.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <cobalt/kernel/rtdm/driver.h>

// #include <src/bcm2835.h>

// #define DRV8825_DIVISOR		( BCM2835_PWM_CLOCK_DIVIDER_2048 | \
// 							  BCM2835_PWM_CLOCK_DIVIDER_1024 | \
// 							  BCM2835_PWM_CLOCK_DIVIDER_512  | \
// 							  BCM2835_PWM_CLOCK_DIVIDER_128  | \
// 							  BCM2835_PWM_CLOCK_DIVIDER_32   | \
// 							  BCM2835_PWM_CLOCK_DIVIDER_4    | \
// 							  BCM2835_PWM_CLOCK_DIVIDER_2 )

static struct pwm_chip bcm2835pwm;
static struct pwm_state bcm2835pwmstate;

static rtdm_irq_t irq_rtdm_dma;

static int dma_handler(rtdm_irq_t *irq)
{


	return RTDM_IRQ_HANDLED;
}

static int __init drv8825_init (void)
{
	int ret = 0;

	

	return ret;
}

static void __exit drv8825_exit (void)
{

}

module_init(drv8825_init);
module_exit(drv8825_exit);
MODULE_LICENSE("GPL");
