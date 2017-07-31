#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/kit.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <mach/reg_gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/atomic.h>
#include <asm/fiq.h>

#include "printer_linux.h"
#include "printer_hal.h"
#include "printer_driver.h"

/* #define PRINTER_DEBUG */

struct printer_dev_t {
	struct cdev cdev;
	int open;
	unsigned char printer_status;
	spinlock_t printer_lock;	/* spin lock */
};

struct printer_dev_t *printer_devp;

static int printer_major = PRINTER_MAJOR;

static int get_hardware_version(char *version)
{
	if (version == 0)
		return -1;
	strcpy(version, "001");		/* XXX printer constant? */
	return 0;
}

static int get_software_version(char *version)
{
	if (version == 0)
		return -1;
	strcpy(version, "001");		/* XXX printer constant? */
	return 0;
}
static int proc_read(char *page, char **start, off_t off, int count,
    int *eof, void *data)
{
	int len;
	char soft_version[8];
	char hard_version[8];

	if (get_hardware_version(hard_version))
		return -1;
	if (get_software_version(soft_version))
		return -1;

	len = sprintf(page, "SW:%s\n", soft_version);
	len += sprintf(page + len, "HW:%s\n", hard_version);

	return len;
}

/* open the printer */
static int printer_open(struct inode *inode, struct file *file)
{
#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER device open...\n");
#endif
	spin_lock(&printer_devp->printer_lock);
	if (printer_devp->open) {
		spin_unlock(&printer_devp->printer_lock);
		return -EBUSY;
	}
	printer_devp->open = 1;
	spin_unlock(&printer_devp->printer_lock);

	/* if open succeeds, status will be 0 */
	printer_devp->printer_status = 0;

	try_module_get(THIS_MODULE);	/* calculate the used times */

	return 0;
}

/* release the printer */
static int printer_release(struct inode *inode, struct file *file)
{
#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER device release...\n");
#endif
	spin_lock(&printer_devp->printer_lock);
	printer_devp->open = 0;
	spin_unlock(&printer_devp->printer_lock);
	module_put(THIS_MODULE);

	return 0;
}

/* read printer, used to get the printer current status */
static ssize_t printer_read(struct file *filp,	/* see include/linux/fs.h */
    char __user *buffer,	/* buffer to fill with data */
    size_t length,		/* length of the buffer */
    loff_t *offset)
{
#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER device read...\n");
#endif
	printer_devp->printer_status = PrnStatus();

	if (copy_to_user(buffer, &printer_devp->printer_status, 1))
		return -EFAULT;

	return 1;
}

/* write to printer */
static ssize_t printer_write(struct file *file, const char __user *data,
    size_t len, loff_t *ppos)
{
	unsigned char printer_gray;
	size_t data_len;
	int line;		/* the line number need to be print */

#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER device write...\n");
#endif
	data_len = len;

	if (data_len == 0) {
		/* no data */
		printer_devp->printer_status = PRN_OK;
	}
	else if (data_len > MAX_DOT_LINE * 48 + 2) {
		/* data_len is too big */
		printer_devp->printer_status = PRN_OUTOFMEMORY;
		return -ENOMEM;
	}
	else if (data_len <= 2) {
		/* just set gray (data[0] is gray, data[1] is not used) */
		if (copy_from_user(&printer_gray, data, 1))
			return -EFAULT;
		PrnSetGray(printer_gray);
		printer_devp->printer_status = PrnStart(0);
	}
	else {
		if (((data_len - 2) % 48) == 0) {
			line = (data_len - 2) / 48;
		}
		else {
			line = (data_len - 2) / 48 + 1;
			data_len = line * 48 + 2;
		}

		/* data[0] is gray, data[1] is not used */
		if (copy_from_user(&printer_gray, data, 1))
			return -EFAULT;

		PrnSetGray(printer_gray);

		if (GetDotBuf(data + 2, len - 2))
			return -EFAULT;

		if (data_len != len) {
			FillDotBuf(len - 2, data_len - len);
		}

		printer_devp->printer_status = PrnStart(line);
		printk("ret = %d...\n", printer_devp->printer_status);
	}

	return len;
}

/* IO control */
static int printer_ioctl(struct inode *inode, struct file *file,
    unsigned int cmd, unsigned long arg)
{
#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER device ioctl...\n");
#endif
	return 0;
}

/* kernel interface */
static struct file_operations printer_fops = {
	.owner = THIS_MODULE,
	.ioctl = printer_ioctl,
	.open = printer_open,
	.write = printer_write,
	.read = printer_read,
	.release = printer_release,
};

static int printer_setup_cdev(struct printer_dev_t *dev, int index)
{
	int err, devno = MKDEV(printer_major, index);	/* generate dev_t */

	cdev_init(&dev->cdev, &printer_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &printer_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding PRINTER%d", err, index);
	}

	return err;
}

static int __init s_printer_init(void)
{
	int result = -1;

#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER driver init...\n");
#endif
	dev_t devno = MKDEV(printer_major, 0);

	if (printer_major) {
		result = register_chrdev_region(devno, 1, "printer");
	}

	if (result < 0) {
		printk(KERN_INFO "Registering char device failed with %d\n",
		    printer_major);
		return result;
	}

	printer_devp = kmalloc(sizeof(struct printer_dev_t), GFP_KERNEL);
	if (!printer_devp) {
		result = -ENOMEM;
		goto fail_malloc;
	}

	memset(printer_devp, 0, sizeof(struct printer_dev_t));

	result = printer_setup_cdev(printer_devp, 0);

	if (result)
		goto fail_addcdev;

	result = proc_create_file("printer", proc_read, NULL);
	if (result < 0) {
		printk(KERN_INFO "printer: couldn't create proc entry\n");
		goto fail_proc;
	}
#ifdef PRINTER_DEBUG
	printk(KERN_INFO "I was assigned major number %d. To talk to\n",
	    printer_major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME,
	    printer_major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");
#endif
	s_PrnInit();

	return 0;
 fail_proc:
 fail_addcdev:
	kfree(printer_devp);
 fail_malloc:
	unregister_chrdev_region(devno, 1);
	return result;
}

static void __exit s_printer_exit(void)
{
#ifdef PRINTER_DEBUG
	printk(KERN_INFO "PAX PRINTER driver exit...\n");
#endif
	cdev_del(&printer_devp->cdev);
	kfree(printer_devp);
	unregister_chrdev_region(MKDEV(printer_major, 0), 1);
	s_PrnGPIOFree();
}

static int __devinit printer_probe(struct platform_device *pdev)
{
	int ret;

	printk(KERN_INFO "into printer_probe...\n");
	ret = s_printer_init();
	return ret;
}

static int __devexit printer_remove(struct platform_device *dev)
{
	printk(KERN_INFO "into printer_remove...\n");
	s_printer_exit();
	return 0;
}

static void printer_shutdown(struct platform_device *dev)
{
	printk(KERN_INFO "into printer_shutdown...\n");
}

static int printer_suspend(struct platform_device *dev, pm_message_t state)
{
	printk(KERN_INFO "into printer_suspend...\n");
	printer_idle_suspend();

	return 0;
}

static int printer_resume(struct platform_device *dev)
{
	printk(KERN_INFO "into printer_resume...\n");
	printer_idle_resume();

	return 0;
}

static struct platform_driver printer_driver = {
	.probe = printer_probe,
	.remove = printer_remove,
	.shutdown = printer_shutdown,
	.suspend = printer_suspend,
	.resume = printer_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "printer",
	},
};

static int __init printer_init(void)
{
	return platform_driver_register(&printer_driver);
}

static void __exit printer_exit(void)
{
	platform_driver_unregister(&printer_driver);
}

module_init(printer_init);
module_exit(printer_exit);

MODULE_AUTHOR(".com");
MODULE_DESCRIPTION("Device Driver");
MODULE_LICENSE("GPL");

