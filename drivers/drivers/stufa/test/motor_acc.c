#include <linux/module.h>

#define MOTOR_STEP      200
#define PWM_CLOCK       19200000
#define FIFOREG_LENGTH  32

#define LOOP_TIME       25
#define PLANET_REDUC    1
#define DRIVER_USTEP    16
#define PWM_DIVISOR     1250

#define STEP_LOOP       ( MOTOR_STEP * PLANET_REDUC * DRIVER_USTEP )
#define STEP_PS         ( STEP_LOOP / LOOP_TIME )
#define FREQ_PWM        ( PWM_CLOCK / PWM_DIVISOR )

static unsigned int calculate_buffer(unsigned int stepPs0, unsigned int stepPs1, unsigned int time, unsigned int incr)
{
    int i;
    int stepIncr = stepPs0 < stepPs1 ? stepPs1 * incr / 100 : stepPs0 * incr / 100;
    int stepTarg = stepPs0 < stepPs1 ? stepPs1 : stepPs0;
    int stepCount = 0;

    if(stepPs0 == 0 && stepPs1 == 0) {
        return 0;
    }

    if((stepPs0 == 0 || stepPs1 == 0) && time == 0) {
        return 0;
    }

    if(stepPs0 == stepPs1) {
        return FREQ_PWM * time / (FIFOREG_LENGTH / 2);
    }
    else  {
        for(i = stepIncr; i < stepTarg; i + stepIncr ) {
            stepCount = FREQ_PWM / i;
        }

        return stepCount / (FIFOREG_LENGTH / 2);
    }
}