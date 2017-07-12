/*  
 *  Copyright (c) 2017-7-7
 *  
 *  printer spi driver  
 */

#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include "pt48d_spi.h"

#define PRINTER_NAME "pt48d-spi"

#if 0
#define DRIVER_VERSION "2017.5.25"
#define DRIVER_AUTHOR "19zn"
#define DRIVER_DESC "StepMotor Driver"
#define DRIVER_LICENSE "GPL"
#endif

#define MOTOR_IOC_MAGIC 'm'  
#define MOTOR_IOCGETDATA    _IOR(MOTOR_IOC_MAGIC,1,int)  
#define MOTOR_IOCSETDATA    _IOW(MOTOR_IOC_MAGIC,2,int)  
#define MOTOR_IOCSETLEFT    _IOW(MOTOR_IOC_MAGIC,3,int)  
#define MOTOR_IOCSETRIGHT   _IOW(MOTOR_IOC_MAGIC,4,int)  
#define MOTOR_IOC_MAXNR     4

#define DEBUG 1

#ifdef DEBUG
#define DBG(fmt, args...) \
	do{ \
		printk(KERN_ERR fmt, ##args); \
	}while(0)
#else
#define DBG(fmt, args...)
#endif

pt48d_spi_data *spi_data;


static void spidev_complete(void *arg) 
{ 
    complete(arg);  //调用complete 
	DBG(">>> [%s]: ... \n", __func__);
} 

static ssize_t spidev_sync(pt48d_spi_data *info, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done); 
	int rc;

	message->complete = spidev_complete;
	message->context = &done;


	rc = spi_async(info->prn_spidev, message);
	if(rc == 0){
		wait_for_completion(&done);
		rc = message->status;
		if(rc == 0)
			rc = message->actual_length;
	}


	DBG(">>> [%s]: ... \n", __func__);

	return rc;
}

static ssize_t spidev_sync_write(pt48d_spi_data *info, size_t len)
{
	struct spi_transfer t = {
		.tx_buf = info->prn_buf,
		.len = len,
	};

	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	return spidev_sync(info, &m);
}


static ssize_t prn_spi_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);
	int i;
	
	DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		
	}else{
		for(i = 0; i < 48; i++)
		{
			spi_data->prn_buf[i] = 0x55;
		}

		spidev_sync_write(spi_data, 48);
	}

	return size;
}


static struct device_attribute pt48d_spi_attribute = __ATTR(prn_spi,0666,NULL, prn_spi_store);

static struct attribute *spi_attrs [] =
{
	&pt48d_spi_attribute.attr,
    NULL
};

static struct attribute_group spi_attribute_group = {
	.attrs = spi_attrs,
};



static int pt48d_spi_probe(struct spi_device *spi)
{
	DBG(">>> [%s]: Entering... \n", __func__);
	
	spi->bits_per_word = 8;
	
	spi_data->prn_spidev = spi;

	spi_data->prn_buf = kmalloc(1024, GFP_KERNEL);
	if(!spi_data->prn_buf){
		DBG(">>> [%s]: NO MEM... \n", __func__);
	}

	spi->max_speed_hz = 2000000;
	spi_setup(spi);
	
	DBG(">>> [%s]: mode=%d, max_speed_hz=%d, chip_select=%d \n", __func__, spi->mode, spi->max_speed_hz, spi->chip_select);
	
	return 0;
}

static int pt48d_spi_remove(struct spi_device *spi)
{
	
	return 0;
}

static struct of_device_id pt48d_spi_match_table[] = {
        {
                .compatible = "printer,pt48d-spi",
        },
        {}
};

static struct spi_driver pt48d_spi_driver = {
        .driver = {
                .name = "pt48d_spi",
                .of_match_table = pt48d_spi_match_table,
                .owner = THIS_MODULE,
        },
        .probe = pt48d_spi_probe,
        .remove = pt48d_spi_remove,
};

static int pt48d_spi_init(void)
{
	int32_t rc = 0;
	pt48d_spi_data *info;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
	{
	    DBG("Alloc GFP_KERNEL memory failed.");
	    return -ENOMEM;
	}

	spi_data = info;

	info->prn_class = class_create(THIS_MODULE, "printer");
	if(IS_ERR(info->prn_class)){
		return PTR_ERR(info->prn_class);
	}

	info->prn_device = device_create(info->prn_class, NULL, 0, "%s", "pt48d");
	if (unlikely(IS_ERR(info->prn_device))) {
		rc = PTR_ERR(info->prn_device);
		info->prn_device = NULL;
		DBG(">>> [%s]: could not allocate prn_device\n", __func__);
	}

	dev_set_drvdata(info->prn_device, info);

	rc = sysfs_create_group(&info->prn_device->kobj, &spi_attribute_group);
	if (rc < 0)
	{
		printk(KERN_ERR ">>> %s:could not create sysfs group for spi\n", __func__);
	}
	
	rc = spi_register_driver(&pt48d_spi_driver);
	if(rc < 0)
	{
		DBG(">>> [%s]: could not register pt48d_spi_driver\n", __func__);
	}else{
		DBG(">>> [%s]: register pt48d_spi_driver success.\n", __func__);
	}

	return 0;
}

static void pt48d_spi_exit(void)
{


}

module_init(pt48d_spi_init);
module_exit(pt48d_spi_exit);

