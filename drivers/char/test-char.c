#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/fcntl.h>
#include <linux/percpu.h>
#include <linux/slab.h>

#include <linux/fips.h>
#include <linux/wait.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>


MODULE_AUTHOR("Yoichi Yuasa <yuasa@linux-mips.org>");
MODULE_DESCRIPTION("TANBAC TB0219 base board driver");
MODULE_LICENSE("GPL");

static int major;	/* default is dynamic major device number */
module_param(major, int, 0);
MODULE_PARM_DESC(major, "Major device number");

static DECLARE_WAIT_QUEUE_HEAD(read_wait);

static int write_count = 0;

static ssize_t tanbac_tb0219_read(struct file *file, char __user *buf, size_t len,
                                  loff_t *ppos)
{
	unsigned int minor;
	char value = 'o';

	minor = iminor(file->f_path.dentry->d_inode);
	switch (minor) {
	case 0:
		break;
	case 16 ... 23:
		break;
	case 32 ... 39:
		break;
	case 48 ... 55:
		break;
	default:
		return -EBADF;
	}

	if (len <= 0)
		return -EFAULT;

	printk("test: Wait for some event\n");
	wait_event_interruptible(read_wait, write_count > 0);
	printk("test: wakeup from event\n");

	write_count = 0;
	if (put_user(value, buf))
		return -EFAULT;

	return 1;
}

static ssize_t tanbac_tb0219_write(struct file *file, const char __user *data,
                                   size_t len, loff_t *ppos)
{
	unsigned int minor;
	/* tb0219_type_t type; */
	size_t i;
	int retval = 0;
	char c;

	minor = iminor(file->f_path.dentry->d_inode);
	switch (minor) {
	case 0:
		/* type = TYPE_LED; */
		break;
	case 32 ... 39:
		/* type = TYPE_GPIO_OUTPUT; */
		break;
	default:
		return -EBADF;
	}

	for (i = 0; i < len; i++) {
		if (get_user(c, data + i))
			return -EFAULT;

		/* switch (type) { */
		/* case TYPE_LED: */
		/* 	retval = set_led(c); */
		/* 	break; */
		/* case TYPE_GPIO_OUTPUT: */
		/* 	retval = set_gpio_output_pin(minor - 32, c); */
		/* 	break; */
		/* } */
		printk("%c", c);

		if (retval < 0)
			break;
	}
	write_count = 1;
	wake_up_interruptible(&read_wait);


	printk("\n");

	return i;
}

static int tanbac_tb0219_open(struct inode *inode, struct file *file)
{
	unsigned int minor;

	minor = iminor(inode);
	switch (minor) {
	case 0:
	case 16 ... 23:
	case 32 ... 39:
	case 48 ... 55:
		return nonseekable_open(inode, file);
	default:
		break;
	}

	return -EBADF;
}

static int tanbac_tb0219_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations tb0219_fops = {
	.owner		= THIS_MODULE,
	.read		= tanbac_tb0219_read,
	.write		= tanbac_tb0219_write,
	.open		= tanbac_tb0219_open,
	.release	= tanbac_tb0219_release,
	.llseek		= no_llseek,
};

static int __devinit tb0219_probe(struct platform_device *dev)
{
	int retval;

	/* if (request_mem_region(TB0219_START, TB0219_SIZE, "TB0219") == NULL) */
	/* 	return -EBUSY; */

	/* tb0219_base = ioremap(TB0219_START, TB0219_SIZE); */
	/* if (tb0219_base == NULL) { */
	/* 	release_mem_region(TB0219_START, TB0219_SIZE); */
	/* 	return -ENOMEM; */
	/* } */

	retval = register_chrdev(major, "TB0219", &tb0219_fops);
	if (retval < 0) {
		/* iounmap(tb0219_base); */
		/* tb0219_base = NULL; */
		/* release_mem_region(TB0219_START, TB0219_SIZE); */
		return retval;
	}

	/* old_machine_restart = _machine_restart; */
	/* _machine_restart = tb0219_restart; */

	/* tb0219_pci_irq_init(); */

	if (major == 0) {
		major = retval;
		printk(KERN_INFO "TB0219: major number %d\n", major);
	}

	return 0;
}

static int __devexit tb0219_remove(struct platform_device *dev)
{
	/* _machine_restart = old_machine_restart; */

	/* iounmap(tb0219_base); */
	/* tb0219_base = NULL; */

	/* release_mem_region(TB0219_START, TB0219_SIZE); */

	return 0;
}

static struct platform_device *tb0219_platform_device;

static struct platform_driver tb0219_device_driver = {
	.probe		= tb0219_probe,
	.remove		= __devexit_p(tb0219_remove),
	.driver		= {
		.name	= "TB0219",
		.owner	= THIS_MODULE,
	},
};

static int __init tanbac_tb0219_init(void)
{
	int retval;

	tb0219_platform_device = platform_device_alloc("TB0219", -1);
	if (!tb0219_platform_device)
		return -ENOMEM;

	retval = platform_device_add(tb0219_platform_device);
	if (retval < 0) {
		platform_device_put(tb0219_platform_device);
		return retval;
	}

	retval = platform_driver_register(&tb0219_device_driver);
	if (retval < 0)
		platform_device_unregister(tb0219_platform_device);

	return retval;
}

static void __exit tanbac_tb0219_exit(void)
{
	platform_driver_unregister(&tb0219_device_driver);
	platform_device_unregister(tb0219_platform_device);
}

module_init(tanbac_tb0219_init);
module_exit(tanbac_tb0219_exit);
