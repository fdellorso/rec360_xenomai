#include <linux/module.h>
#include <linux/pwm.h>

// #include <alchemy/task.h>
#include <cobalt/kernel/rtdm/driver.h>

#define TASK_PRIO  99              /* Highest RT priority */
// #define TASK_STKSZ 4096            /* Stack size (in bytes) */

// static          RT_TASK     task_desc;
static          rtdm_task_t task_pwm;
static  struct  pwm_device  *test_pwm;

static void task_body (void *cookie)
{
    unsigned long clock;

    test_pwm = pwm_get(NULL, "pwm");
    clock = pwm_get_clock(test_pwm);

    printk(KERN_INFO "PWM CLOCK %lu\n", clock);
    
    for (;;) {
    /* ... "cookie" should be NULL ... */
    }
}

static int __init test_init (void)
{
    int err = 0;
    /* ... */
    // if (!err)
    //     err = rt_task_create(&task_desc, "MyTaskName", TASK_STKSZ, TASK_PRIO, 0);
    // if (!err)
    //     err = rt_task_start(&task_desc, &task_body, NULL);

    if (!err)
        err = rtdm_task_init(&task_pwm, "RTDM Task for PWM", &task_body, 0, TASK_PRIO, 1000);
    /* ... */

    return err;
}

static void __exit test_exit (void)
{
    // rt_task_delete(&task_desc);
    rtdm_task_destroy(&task_pwm);
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: pwm rdtm-gpio-estop");
