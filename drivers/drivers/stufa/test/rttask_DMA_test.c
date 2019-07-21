/*
    TODO List:
    - Check tables
    - change variable names in more intutive 
    - load/unload/load not restart pulsing
    - scatter gahter list with correct interrupt sequence
        -> use ...prep-slave-sg
    - pass interrupt in bcm2835-dma.c to Ipipe
 */

#include <stdarg.h>

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <cobalt/kernel/rtdm/driver.h>


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

/* Min step duration 1.9us (round to 2us) transformed in HZ
to compare FREQ_PWM */
#define DRV8825_MIN_STEP    500000              

#define BCM2708_PERI_BASE   0x20000000
#define PWM_BASE		    ( BCM2708_PERI_BASE + 0x20C000 )
#define BCM2835_PWM_FIFO    0x18
#define PWM_FIFO_LEN	    0x4

#define GPIO_DIR	        23				// PIN16 -> GPIO23


static struct pwm_device *pwm; 
static struct dma_chan *chan;

struct steptable {
            int                     direction;
            unsigned int            speedstart;
            unsigned int            speedend;
            unsigned int            duration;
            unsigned int            stepincrementation;
            unsigned int            totalstep;
            unsigned int            buffersize;
            unsigned int            *steptable;
};

struct dmamotortable{
    struct  pwm_device              *pwm;
    struct  dma_chan                *chan;

    struct  steptable               table;
            dma_addr_t              addr;

    struct  scatterlist             sgl;
            unsigned int            sgl_nents;

    enum    dma_ctrl_flags          flags;
    struct  dma_async_tx_descriptor *desc;
            dma_cookie_t            cookie;

	        dma_async_tx_callback   callback;
	        void                    *callback_param;
    
    struct  dmamotortable           *next;    
};
// typedef struct dmamotortable dmamotortable_t;

static struct dmamotortable acceleration, accomp, vconst, deceleration, deccomp;


// TODO step must be modifiable
// TODO filler case stepTarg <= FIFOREG_LENGTH
static unsigned int stepTable_creation(int dir, unsigned int stepPs0, unsigned int stepPs1, unsigned int time, unsigned int incr, unsigned int *stepext, unsigned int **table)
{
    int i, j;
    // int stepIncr = stepPs0 < stepPs1 ? ((stepPs1 * incr) / 100) + (((stepPs1 * incr) % 100) != 0) : ((stepPs0 * incr) / 100) + (((stepPs0 * incr) % 100) != 0);
    int stepIncr = incr;
    int stepInit = stepPs0;
    int stepTarg = stepPs1;

    int pulseCount = 0;

    unsigned int bufferLength = 0;
    unsigned int *buffer = NULL;

    unsigned int step = *stepext;

    unsigned int uStepPoint = 1;
    unsigned int *aStepPoint = (unsigned int *) kmalloc(uStepPoint * sizeof(unsigned int), GFP_KERNEL);

    int truncLength = 0;
    unsigned int truncSend = 0x0;


    // --- Input Controller Section
    if(stepPs0 == 0 && stepPs1 == 0) {
        return -1;
    }

    if((stepPs0 == stepPs1) && (time == 0) && (step == 0)) {
        return -1;
    }

    if((stepPs0 != stepPs1) && incr == 0) {
        return -1;
    }

    if((time > 0) && (incr > 0)) {
        return -1;
    }

    if((time > 0) && (step > 0)) {
        return -1;
    }

    if((incr > 0) && (step > 0)) {
        return -1;
    }
    // ---

    // --- V Const Section
    if(stepPs0 == stepPs1) {
        // --- Calculate Buffer Size
        if(step == 0)
            *stepext = stepTarg * time;

        if(time > 0) {
            if((FREQ_PWM / stepTarg) <= FIFOREG_LENGTH) {
                if((FREQ_PWM * time) % FIFOREG_LENGTH)
                    bufferLength = ((FREQ_PWM * time) / FIFOREG_LENGTH) + 2;
                else
                    bufferLength = ((FREQ_PWM * time) / FIFOREG_LENGTH) + 1;
            }
            else {
                if((FREQ_PWM * time) % FIFOREG_LENGTH)
                    bufferLength = ((FREQ_PWM * time) / FIFOREG_LENGTH) + 1;
                else
                    bufferLength = ((FREQ_PWM * time) / FIFOREG_LENGTH);
            }
        }

        if(step > 0) {
            if((FREQ_PWM / stepTarg) <= FIFOREG_LENGTH) {
                if(((FREQ_PWM / stepTarg) * step) % FIFOREG_LENGTH)
                    bufferLength = (((FREQ_PWM / stepTarg) * step) / FIFOREG_LENGTH) + 2;
                else
                    bufferLength = (((FREQ_PWM / stepTarg) * step) / FIFOREG_LENGTH) + 1;
            }
            else {
                if(((FREQ_PWM / stepTarg) * step) % FIFOREG_LENGTH)
                    bufferLength = (((FREQ_PWM / stepTarg) * step) / FIFOREG_LENGTH) + 1;
                else
                    bufferLength = (((FREQ_PWM / stepTarg) * step) / FIFOREG_LENGTH);
            }
        }
        // ---
	    
        // --- Create Buffer
        buffer = (unsigned int *) kzalloc(bufferLength * sizeof(unsigned int), GFP_KERNEL);      // Memory set to zero
        if(buffer == NULL)
            return -1;
        // ---

        // --- Filler V const
        if((FREQ_PWM / stepTarg) <= FIFOREG_LENGTH) {
            // for(i = 0; i < (bufferLength - 1); i++) {
            //     buffer[i] = 0x80000000;
            // }

            // buffer[bufferLength - 1] = 0x00000000;
        }
        else {
            if(time > 0) {
                j = 0;
                truncLength = (FREQ_PWM / stepTarg);
                for(i = 0; i < (bufferLength); i++) {
                    if(truncLength == (FREQ_PWM / stepTarg)) {
                        truncSend = 0x80000000;
                    }
                    else {
                        truncSend = 0x00000000;
                    }
                    
                    if(truncLength < FIFOREG_LENGTH) {
                        truncSend = 0x80000000 >> truncLength;
                    }

                    buffer[i] = truncSend;

                    if((truncLength - FIFOREG_LENGTH) >= 0){
                        truncLength -= FIFOREG_LENGTH;
                    }
                    else {
                        j++;
                        truncLength = (FREQ_PWM / stepTarg) - (FIFOREG_LENGTH - truncLength);
                    }
                }
            }

            if(step > 0) {
                j = 0;
                truncLength = (FREQ_PWM / stepTarg);
                for(i = 0; i < (bufferLength); i++) {
                    if(truncLength == (FREQ_PWM / stepTarg)) {
                        truncSend = 0x80000000;
                    }
                    else {
                        truncSend = 0x00000000;
                    }
                    
                    if(truncLength < FIFOREG_LENGTH) {
                        truncSend = 0x80000000 >> truncLength;
                    }

                    buffer[i] = truncSend;

                    if((truncLength - FIFOREG_LENGTH) >= 0){
                        truncLength -= FIFOREG_LENGTH;
                    }
                    else {
                        j++;
                        truncLength = (FREQ_PWM / stepTarg) - (FIFOREG_LENGTH - truncLength);
                    }

                    if(j == step)
                        break;
                }
            }
        }
        // ---
    }
    // --- V Variable Section
    else  {
        // --- Calculate Buffer Size
        uStepPoint = stepPs0 < stepPs1 ? (stepPs1 - stepPs0) / incr : (stepPs0 - stepPs1) / incr;
        aStepPoint = (unsigned int *) krealloc(aStepPoint, uStepPoint * sizeof(unsigned int), GFP_KERNEL);

        if(dir == -1) {
            stepIncr = -incr;
            stepInit = stepPs1;
            stepTarg = stepPs0;
        }

        j = 0;
        for(i = stepInit + stepIncr; dir == 1 ? i < stepTarg : i > stepTarg; i = i + stepIncr ) {
            pulseCount = pulseCount + (FREQ_PWM / i);

            aStepPoint[j] = (FREQ_PWM / i);
            j++;
        }

        *stepext = j;

        if((FREQ_PWM / stepTarg) <= FIFOREG_LENGTH) {
            if(pulseCount % FIFOREG_LENGTH)
                bufferLength = (pulseCount / FIFOREG_LENGTH) + 2;
            else
                bufferLength = (pulseCount / FIFOREG_LENGTH) + 1;
        }
        else {
            if(pulseCount % FIFOREG_LENGTH)
                bufferLength = (pulseCount / FIFOREG_LENGTH) + 1;
            else
                bufferLength = (pulseCount / FIFOREG_LENGTH);
        }
        // ---
        
        // --- Create Buffer
        buffer = (unsigned int *) kzalloc(bufferLength * sizeof(unsigned int), GFP_KERNEL);      // Memory set to zero
        if(buffer == NULL)
            return -1;
        // ---

        // Filler V Variable
        if((FREQ_PWM / stepTarg) <= FIFOREG_LENGTH) {
            // for(i = 0; i < (bufferLength - 1); i++) {
            //     buffer[i] = 0x80000000;
            // }

            // buffer[bufferLength - 1] = 0x00000000;
        }
        else {
            j = 0;
            truncLength = (int) aStepPoint[j];
            for(i = 0; i < (bufferLength); i++) {
                if(truncLength == aStepPoint[j]) {
                    truncSend = 0x80000000;
                }
                else {
                    truncSend = 0x00000000;
                }
                
                if(truncLength < FIFOREG_LENGTH) {
                    truncSend = 0x80000000 >> truncLength;
                }

                buffer[i] = truncSend;

                if((truncLength - FIFOREG_LENGTH) >= 0){
                    truncLength -= FIFOREG_LENGTH;
                }
                else {
                    j++;
                    truncLength = aStepPoint[j] - (FIFOREG_LENGTH - truncLength);
                }
            }
        }
        // ---
    }

    // Transmit Buffer Pointer
    *table = buffer;

    // CLear Memory
    kfree(aStepPoint);

    // Return Buffer Size
    return bufferLength;
}


static int table_prep (struct steptable *table, int direction, unsigned int speedstart, unsigned int speedend, unsigned int duration, unsigned int step, unsigned int incrementation)
{
    int err = 0;
    
    table->direction = direction;
    table->speedstart = speedstart;
    table->speedend = speedend;
    table->duration = duration;
    table->stepincrementation = incrementation;
    table->totalstep = step;
    table->steptable = NULL;

    table->buffersize = stepTable_creation(table->direction, table->speedstart, table->speedend, table->duration, table->stepincrementation, &table->totalstep, &table->steptable);

    if(table->buffersize < 0)
        err = -1;

    udelay(10);

    return err;
}


static int sgl_create(struct dma_chan *_chan, struct scatterlist *sg, unsigned int sg_nents, ...)
{
    int err = 0;
    int i;
    struct steptable table;
    va_list arg_list;

    if(_chan == NULL)
        return -1;

	sg_init_table(sg, sg_nents);

    va_start(arg_list, sg_nents);

	for (i = 0; i < sg_nents; i++) {
        table = va_arg(arg_list, struct steptable);

        sg_dma_address(sg) = dma_map_single_attrs(_chan->device->dev, table.steptable, table.buffersize, DMA_TO_DEVICE, 0);

        sg_dma_len(sg) = table.buffersize;

        sg = sg_next(sg);
    }

    va_end(arg_list);

    return err;
}


static int pwm_init (struct pwm_device **__pwm)
{
    int err = 0;

    struct pwm_device *_pwm;

    _pwm = pwm_request(0, "bcm2835-pwm");

    err = pwm_clear_fifo(_pwm);
    if(err)
        printk(KERN_ERR "Failed to Clear Fifo\n");

    err = pwm_serialiser(_pwm, true, false, true);
    if(err)
        printk(KERN_ERR "Failed to set Serialiser\n");
    
    *__pwm = _pwm;

    return err;
}


static int pwm_start (struct pwm_device *_pwm)
{
    int err = 0;

    // struct pwm_device *pwm = _pwm;

    if(_pwm == NULL)
        return -1;
    
    err = pwm_set_dma(_pwm, true, 10, 5);
    if(err)
        printk(KERN_ERR "Failed to set PWM DMA\n");

    err = pwm_enable(_pwm);
    if(err)
        printk(KERN_ERR "Failed to enable PWM\n");

    return err;
}


static int pwm_pause (struct pwm_device *_pwm)
{
    int err = 0;

    // struct pwm_device *pwm = _pwm;

    if(_pwm == NULL)
        return -1;
    
    err = pwm_set_dma(_pwm, false, 10, 5);
    if(err)
        printk(KERN_ERR "Failed to set PWM DMA\n");

    pwm_disable(_pwm);

    err = pwm_clear_status(_pwm);
    if(err)
        printk(KERN_ERR "Failed to Clear Status\n");

    return err;
}


static int dma_init (struct pwm_device *_pwm, struct dma_chan **__chan)
{
    int err = 0;

    const   __be32          *addr;
            dma_addr_t      pwm_reg_base;

    struct dma_slave_config config;

    struct dma_chan *_chan;

    addr = of_get_address(_pwm->chip->dev->of_node, 0, NULL, NULL);
    pwm_reg_base = be32_to_cpup(addr);

    _chan = dma_request_slave_channel(_pwm->chip->dev, "fifo");

    config.direction = DMA_MEM_TO_DEV;
    config.dst_addr = (u32)(pwm_reg_base + BCM2835_PWM_FIFO);
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    config.dst_maxburst = 1;                                   // TODO verify
    err = dmaengine_slave_config(_chan, &config);

    *__chan = _chan;

    return err;
}


static int dma_prep_single (struct dmamotortable *dmamotor)
{
    int err = 0;

    enum dma_ctrl_flags flags;

    struct dma_chan *_chan = dmamotor->chan;
    struct dma_async_tx_descriptor *_desc = dmamotor->desc;

	dmamotor->addr = dma_map_single_attrs(_chan->device->dev, dmamotor->table.steptable, dmamotor->table.buffersize, DMA_TO_DEVICE, 0);

    flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	_desc = dmaengine_prep_slave_single(_chan, dmamotor->addr, dmamotor->table.buffersize, DMA_MEM_TO_DEV, flags);

    if(_desc) {
        _desc->callback = dmamotor->callback;
        _desc->callback_param = dmamotor->callback_param;
        dmamotor->cookie = dmaengine_submit(_desc);
    }
    else {
        err = -1;
    }

    return err;
}


static int dma_prep_sg (struct dmamotortable *dmamotor)
{
    int err = 0;

    enum dma_ctrl_flags flags;

    struct dma_chan *_chan = dmamotor->chan;
    struct dma_async_tx_descriptor *_desc = dmamotor->desc;

    struct scatterlist *sg = &dmamotor->sgl; 
    unsigned int sg_nents = dmamotor->sgl_nents;

    flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
    _desc = dmaengine_prep_slave_sg(_chan, sg, sg_nents, DMA_MEM_TO_DEV, flags);

    if(_desc) {
        _desc->callback = dmamotor->callback;
        _desc->callback_param = &dmamotor->callback_param;
        dmamotor->cookie = dmaengine_submit(_desc);
    }
    else {
        err = -1;
    }

    return err;
}


static int dma_motor_direction (struct steptable *table)
{
    int err = 0;

    if(table == NULL)
        return -1;

    gpio_set_value(GPIO_DIR, table->direction);

    return err;
}


static int dma_issue (struct dmamotortable *dmamotor)
{
    int err = 0;

    struct dma_chan *_chan = dmamotor->chan;

    dma_motor_direction(&dmamotor->table);

    if(_chan) {
        dma_async_issue_pending(_chan);
    }
    else {
        err = -1;
    }

    return err;
}


static          unsigned int    count = 0;
static void dma_table_callback(void *param)
{
    int err = 0;

    struct dmamotortable *dmamotor;

    if(param != NULL)
        dmamotor = (struct dmamotortable *) param;

    if(dmaengine_tx_status(dmamotor->chan, dmamotor->cookie, NULL) == DMA_COMPLETE) {
        count++;
        err = dma_motor_direction(&dmamotor->table);
        printk(KERN_INFO "DMA Transaction complete %u\n", count);
    }
}


static int table_init (void)
{
    int err = 0;

    accomp.pwm = pwm;
    accomp.chan = chan;
    accomp.callback = dma_table_callback;
    accomp.callback_param = &accomp;
    accomp.sgl_nents = 1;
    accomp.next = &acceleration;

    acceleration.pwm = pwm;
    acceleration.chan = chan;
    acceleration.callback = dma_table_callback;
    acceleration.callback_param = &acceleration;
    acceleration.sgl_nents = 1;
    acceleration.next = &vconst;

    vconst.pwm = pwm;
    vconst.chan = chan;
    vconst.callback = dma_table_callback;
    vconst.callback_param = &vconst;
    vconst.sgl_nents = 1;
    vconst.next = &deceleration;

    deceleration.pwm = pwm;
    deceleration.chan = chan;
    deceleration.callback = dma_table_callback;
    deceleration.callback_param = &deceleration;
    deceleration.sgl_nents = 1;
    deceleration.next = &deccomp;

    deccomp.pwm = pwm;
    deccomp.chan = chan;
    deccomp.callback = dma_table_callback;
    deccomp.callback_param = &deccomp;
    deccomp.sgl_nents = 1;
    deccomp.next = NULL;

    return err;
}


static int gpio_init(unsigned int gpio)
{
    int err = 0;

    if ((err = gpio_request(gpio, THIS_MODULE->name)) != 0) {
		printk(KERN_ERR "ERROR in gpio_request()\n");
		return err;
	}

    if ((err = gpio_direction_output(gpio, 0)) != 0) {
		printk(KERN_ERR "ERROR in gpio_direction_output()\n");
    	gpio_free(GPIO_DIR);
		return err;
	}

    return err;
}


static int dma_motor_oneloop(struct dmamotortable *dmamotor)
{
    int err = 0;
    struct dmamotortable *_dmamotor;

    if(dmamotor == NULL) {
        return -1;
    }
    else {
        _dmamotor = dmamotor;
    }

    while(_dmamotor != NULL) {
        err = dma_prep_single(_dmamotor);
        _dmamotor = _dmamotor->next;
    }

    // err = dma_prep_single(&accomp);
    // err = dma_prep_single(&acceleration);
    // err = dma_prep_single(&vconst);
    // err = dma_prep_single(&deceleration);
    // err = dma_prep_single(&deccomp);

    err = dma_issue(dmamotor);
    err = pwm_start(pwm);

    return err;
}
EXPORT_SYMBOL_GPL(dma_motor_oneloop);


static int __init test_init (void)
{
    int err = 0;

    err = gpio_init(GPIO_DIR);
    err = pwm_init(&pwm);
    err = dma_init(pwm, &chan);
    err = table_init();

    err = table_prep(&acceleration.table, 1, 0, STEP_PS, 0, 0, 8);
    err = table_prep(&accomp.table, -1, STEP_PS/8, STEP_PS/8, 0, acceleration.table.totalstep, 0);
    err = table_prep(&vconst.table, 1, STEP_PS, STEP_PS, LOOP_TIME, 0, 0);
    err = table_prep(&deceleration.table, 1, STEP_PS, 0, 0, 0, 8);
    err = table_prep(&deccomp.table, -1, STEP_PS/8, STEP_PS/8, 0, deceleration.table.totalstep, 0);

    printk(KERN_INFO "Acc Comp - Total Step: %u\tBuffer Size: %u\n", accomp.table.totalstep, accomp.table.buffersize);
    printk(KERN_INFO "Accelera - Total Step: %u\tBuffer Size: %u\n", acceleration.table.totalstep, acceleration.table.buffersize);
    printk(KERN_INFO "V Consta - Total Step: %u\tBuffer Size: %u\n", vconst.table.totalstep, vconst.table.buffersize);
    printk(KERN_INFO "Decelera - Total Step: %u\tBuffer Size: %u\n", deceleration.table.totalstep, deceleration.table.buffersize);
    printk(KERN_INFO "Dec Comp - Total Step: %u\tBuffer Size: %u\n", deccomp.table.totalstep, deccomp.table.buffersize);

    // err = dma_motor_oneloop(&accomp);
    
    return err;
}


static void __exit test_exit (void)
{
    // int i;
    // struct scatterlist *sgl;

    if(pwm != NULL) {
        pwm_disable(pwm);
        pwm_free(pwm);
    }

    if(chan != NULL) {
        dmaengine_terminate_all(chan);

        // if(acceleration.sgl.dma_address) {
        //     for_each_sg(&acceleration.sgl, sgl, acceleration.sgl_nents, i) {
        //         dma_unmap_single_attrs(chan->device->dev, sgl->dma_address, sgl->length, DMA_TO_DEVICE, 0);
        //     }
        // }

        if(accomp.desc != NULL)
            dma_unmap_single_attrs(chan->device->dev, accomp.addr, accomp.table.buffersize, DMA_TO_DEVICE, 0);
        if(acceleration.desc != NULL)
            dma_unmap_single_attrs(chan->device->dev, acceleration.addr, acceleration.table.buffersize, DMA_TO_DEVICE, 0);
        
        dma_release_channel(chan);
        chan = NULL;
    }

    if(acceleration.table.steptable != NULL)
        kfree(acceleration.table.steptable);
    if(accomp.table.steptable != NULL)
        kfree(accomp.table.steptable);

    gpio_free(GPIO_DIR);
}


module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
