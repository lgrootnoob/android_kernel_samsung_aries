/* linux/drivers/media/video/samsung/tv20/cec.c
 *
 * cec interface file for Samsung TVOut driver (only s5pv210)
 *
 * Copyright (c) 2010 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>

#include "s5p_tv.h"
#include "cec.h"

/*#define CECDEBUG*/
#ifdef CECDEBUG
#define CECIFPRINTK(fmt, args...) \
	printk(KERN_INFO "\t[CEC_IF] %s: " fmt, __func__ , ## args)
#else
#define CECIFPRINTK(fmt, args...)
#endif

static struct cec_rx_struct cec_rx_struct;
static struct cec_tx_struct cec_tx_struct;

static bool hdmi_on;

/**
 * Change CEC Tx state to state
 * @param state [in] new CEC Tx state.
 */
void __s5p_cec_set_tx_state(enum cec_state state)
{
	atomic_set(&cec_tx_struct.state, state);
}

/**
 * Change CEC Rx state to @c state.
 * @param state [in] new CEC Rx state.
 */
void __s5p_cec_set_rx_state(enum cec_state state)
{
	atomic_set(&cec_rx_struct.state, state);
}


int s5p_cec_open(struct inode *inode, struct file *file)
{
	s5p_tv_clk_gate(true);

	hdmi_on = true;

	__s5p_cec_reset();

	__s5p_cec_set_divider();

	__s5p_cec_threshold();

	__s5p_cec_unmask_tx_interrupts();

	__s5p_cec_set_rx_state(STATE_RX);
	__s5p_cec_unmask_rx_interrupts();
	__s5p_cec_enable_rx();

	return 0;
}

int s5p_cec_release(struct inode *inode, struct file *file)
{
	s5p_tv_clk_gate(false);

	hdmi_on = false;

	__s5p_cec_mask_tx_interrupts();
	__s5p_cec_mask_rx_interrupts();


	return 0;
}

ssize_t s5p_cec_read(struct file *file, char __user *buffer, size_t count,
	loff_t *ppos)
{
	ssize_t retval;

	if (wait_event_interruptible(cec_rx_struct.waitq,
		atomic_read(&cec_rx_struct.state)
		== STATE_DONE)) {
		return -ERESTARTSYS;
	}

	spin_lock_irq(&cec_rx_struct.lock);

	if (cec_rx_struct.size > count) {
		spin_unlock_irq(&cec_rx_struct.lock);
		return -1;
	}

	if (copy_to_user(buffer, cec_rx_struct.buffer, cec_rx_struct.size)) {
		spin_unlock_irq(&cec_rx_struct.lock);
		printk(KERN_ERR " copy_to_user() failed!\n");
		return -EFAULT;
	}

	retval = cec_rx_struct.size;

	__s5p_cec_set_rx_state(STATE_RX);
	spin_unlock_irq(&cec_rx_struct.lock);

	return retval;
}

ssize_t s5p_cec_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	char *data;

	/* check data size */

	if (count > CEC_TX_BUFF_SIZE || count == 0)
		return -1;

	data = kmalloc(count, GFP_KERNEL);

	if (!data) {
		printk(KERN_ERR " kmalloc() failed!\n");
		return -1;
	}

	if (copy_from_user(data, buffer, count)) {
		printk(KERN_ERR " copy_from_user() failed!\n");
		kfree(data);
		return -EFAULT;
	}

	__s5p_cec_copy_packet(data, count);

	kfree(data);

	/* wait for interrupt */
	if (wait_event_interruptible(cec_tx_struct.waitq,
		atomic_read(&cec_tx_struct.state)
		!= STATE_TX)) {
		return -ERESTARTSYS;
	}

	if (atomic_read(&cec_tx_struct.state) == STATE_ERROR)
		return -1;

	return count;
}

static long s5p_cec_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	u32 laddr;

	switch (cmd) {

	case CEC_IOC_SETLADDR:
		CECIFPRINTK("ioctl(CEC_IOC_SETLADDR)\n");

		if (get_user(laddr, (u32 __user *) arg))
			return -EFAULT;

		CECIFPRINTK("logical address = 0x%02x\n", laddr);

		__s5p_cec_set_addr(laddr);

		break;

	default:
		return -EINVAL;
	}

	return 0;
}

u32 s5p_cec_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &cec_rx_struct.waitq, wait);

	if (atomic_read(&cec_rx_struct.state) == STATE_DONE)
		return POLLIN | POLLRDNORM;

	return 0;
}

static const struct file_operations cec_fops = {
	.owner   = THIS_MODULE,
	.open    = s5p_cec_open,
	.release = s5p_cec_release,
	.read    = s5p_cec_read,
	.write   = s5p_cec_write,
	.poll    = s5p_cec_poll,
	.unlocked_ioctl = s5p_cec_ioctl,
};

static struct miscdevice cec_misc_device = {
	.minor = CEC_MINOR,
	.name  = "CEC",
	.fops  = &cec_fops,
};


/**
 * @brief CEC interrupt handler
 *
 * Handles interrupt requests from CEC hardware. \n
 * Action depends on current state of CEC hardware.
 */
static irqreturn_t s5p_cec_irq_handler(int irq, void *dev_id)
{

	u32 status = 0;

	/* read flag register */


	/* is this our interrupt? */
/*
	if (!(flag & (1 << HDMI_IRQ_CEC))) {
		return IRQ_NONE;
	}
*/
	status = __s5p_cec_get_status();

	if (status & CEC_STATUS_TX_DONE) {
		if (status & CEC_STATUS_TX_ERROR) {
			CECIFPRINTK(" CEC_STATUS_TX_ERROR!\n");
			__s5p_cec_set_tx_state(STATE_ERROR);
		} else {
			CECIFPRINTK(" CEC_STATUS_TX_DONE!\n");
			__s5p_cec_set_tx_state(STATE_DONE);
		}

		__s5p_clr_pending_tx();


		wake_up_interruptible(&cec_tx_struct.waitq);
	}

	if (status & CEC_STATUS_RX_DONE) {
		if (status & CEC_STATUS_RX_ERROR) {
			CECIFPRINTK(" CEC_STATUS_RX_ERROR!\n");
			__s5p_cec_rx_reset();

		} else {
			u32 size;

			CECIFPRINTK(" CEC_STATUS_RX_DONE!\n");

			/* copy data from internal buffer */
			size = status >> 24;

			spin_lock(&cec_rx_struct.lock);

			__s5p_cec_get_rx_buf(size, cec_rx_struct.buffer);

			cec_rx_struct.size = size;

			__s5p_cec_set_rx_state(STATE_DONE);

			spin_unlock(&cec_rx_struct.lock);

			__s5p_cec_enable_rx();
		}

		/* clear interrupt pending bit */
		__s5p_clr_pending_rx();


		wake_up_interruptible(&cec_rx_struct.waitq);
	}

	return IRQ_HANDLED;
}

static int __init s5p_cec_probe(struct platform_device *pdev)
{
	u8 *buffer;
	int irq_num;
	int ret;

	s3c_gpio_cfgpin(S5PV210_GPH1(4), S3C_GPIO_SFN(0x4));
	s3c_gpio_setpull(S5PV210_GPH1(4), S3C_GPIO_PULL_NONE);

	/* get ioremap addr */
	__s5p_cec_probe(pdev);

	if (misc_register(&cec_misc_device)) {
		printk(KERN_WARNING " Couldn't register device 10, %d.\n",
			CEC_MINOR);
		return -EBUSY;
	}

	irq_num = platform_get_irq(pdev, 0);
	if (irq_num < 0) {
		printk(KERN_ERR  "failed to get %s irq resource\n", "cec");
		ret = -ENOENT;
		return ret;
	}

	ret = request_threaded_irq(irq_num, NULL,
		s5p_cec_irq_handler, IRQF_DISABLED,
		"s5p_cec_irq", pdev);

	if (ret != 0) {
		printk(KERN_ERR  "failed to install %s irq (%d)\n", "cec", ret);
		return ret;
	}


	init_waitqueue_head(&cec_rx_struct.waitq);
	spin_lock_init(&cec_rx_struct.lock);
	init_waitqueue_head(&cec_tx_struct.waitq);

	buffer = kmalloc(CEC_TX_BUFF_SIZE, GFP_KERNEL);

	if (!buffer) {
		printk(KERN_ERR " kmalloc() failed!\n");
		misc_deregister(&cec_misc_device);
		return -EIO;
	}

	cec_rx_struct.buffer = buffer;

	cec_rx_struct.size   = 0;

	return 0;
}

/*
 *  Remove
 */
static int s5p_cec_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
/*
 *  Suspend
 */
int s5p_cec_suspend(struct device* dev)
{
	if (hdmi_on)
		s5p_tv_clk_gate(false);

	return 0;
}

/*
 *  Resume
 */
int s5p_cec_resume(struct device* dev)
{
	if (hdmi_on)
		s5p_tv_clk_gate(true);

	return 0;
}

static const struct dev_pm_ops s5p_cec_pm_ops = {
	.suspend = s5p_cec_suspend,
	.resume = s5p_cec_resume,
};
#endif

static struct platform_driver s5p_cec_driver = {
	.probe		= s5p_cec_probe,
	.remove		= s5p_cec_remove,
	.driver		= {
		.name	= "s5p-cec",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm     = &s5p_cec_pm_ops,
#endif
	},
};

static char banner[] __initdata =
	"S5P CEC Driver, (c) 2010 Samsung Electronics\n";

int __init s5p_cec_init(void)
{
	int ret;

	printk(banner);

	ret = platform_driver_register(&s5p_cec_driver);

	if (ret) {
		printk(KERN_ERR "Platform Device Register Failed %d\n", ret);
		return -1;
	}

	return 0;
}

static void __exit s5p_cec_exit(void)
{
	kfree(cec_rx_struct.buffer);

	platform_driver_unregister(&s5p_cec_driver);

}

module_init(s5p_cec_init);
module_exit(s5p_cec_exit);

MODULE_AUTHOR("SangPil Moon");
MODULE_DESCRIPTION("SS5PC11X CEC driver");
MODULE_LICENSE("GPL");

