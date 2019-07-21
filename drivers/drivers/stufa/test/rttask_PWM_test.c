#include <linux/module.h>
#include <linux/pwm.h>

#include <cobalt/kernel/rtdm/driver.h>

static          rtdm_task_t task_pwm;
static  struct  pwm_device  *test_pwm;

static void task_body (void *cookie)
{
    unsigned long clock;
    int err;

    (void) cookie;

    printk(KERN_INFO "RTDM Task Inited\n");

    test_pwm = pwm_request(0, "bcm2835-pwm");

    clock = pwm_get_clock(test_pwm);
    printk(KERN_INFO "PWM CLOCK %lu\n", clock);

    printk(KERN_INFO "PWM Status: 0x%x\n", pwm_get_status(test_pwm));

    err = pwm_clear_status(test_pwm);
    if(err)
        printk(KERN_ERR "Failed to clear PWM Status\n");

    printk(KERN_INFO "PWM Status: 0x%x\n", pwm_get_status(test_pwm));

    err = pwm_set_fifo(test_pwm, 0x00000001);
    if(err)
        printk(KERN_ERR "Failed to set FIFO Buffer\n");
    err = pwm_set_fifo(test_pwm, 0x00000001);
    if(err)
        printk(KERN_ERR "Failed to set FIFO Buffer\n");
    err = pwm_set_fifo(test_pwm, 0x00000001);
    if(err)
        printk(KERN_ERR "Failed to set FIFO Buffer\n");
    err = pwm_set_fifo(test_pwm, 0x00000001);
    if(err)
        printk(KERN_ERR "Failed to set FIFO Buffer\n");

    printk(KERN_INFO "PWM Status: 0x%x\n", pwm_get_status(test_pwm));

    err = pwm_serialiser(test_pwm, true, false, true);
    if(err)
        printk(KERN_ERR "Failed to set Serialiser\n");

    printk(KERN_INFO "PWM Status: 0x%x\n", pwm_get_status(test_pwm));
    
    err = pwm_enable(test_pwm);
    if(err)
        printk(KERN_ERR "Failed to enable PWM\n");

    printk(KERN_INFO "PWM Status: 0x%x\n", pwm_get_status(test_pwm));

    while(1) {
        rtdm_task_wait_period(NULL);
        printk(KERN_INFO "PWM Status: 0x%x\n", pwm_get_status(test_pwm));
    }
}

static int __init test_init (void)
{
    int err = 0;

    printk(KERN_INFO "Init Test PWM\n");

    if (!err)
        err = rtdm_task_init(&task_pwm, "RTDM Task for PWM", &task_body, NULL, 1, 500000000);
    
    printk(KERN_INFO "ERR: %d\n", err);

    return err;
}

static void __exit test_exit (void)
{
    rtdm_task_destroy(&task_pwm);
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: pwm-bcm2835 rdtm-gpio-estop");
