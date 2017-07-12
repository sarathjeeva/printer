/*  
 *  Copyright (c) 2017-5-25
 *  
 *  step motor support  
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

#include "motor.h"

#define PRINTER_NAME "printer-pt48d"
#if 0
#define DRIVER_VERSION "2017.5.25"
#define DRIVER_AUTHOR "19zn"
#define DRIVER_DESC "StepMotor Driver"
#define DRIVER_LICENSE "GPL"
#endif

#define DEVICE_NAME "motor"
#define MOTOR_MAJOR 177

#define MOTOR_IOC_MAGIC 'm'  
#define MOTOR_IOCGETDATA    _IOR(MOTOR_IOC_MAGIC,1,int)  
#define MOTOR_IOCSETDATA    _IOW(MOTOR_IOC_MAGIC,2,int)  
#define MOTOR_IOCSETLEFT    _IOW(MOTOR_IOC_MAGIC,3,int)  
#define MOTOR_IOCSETRIGHT   _IOW(MOTOR_IOC_MAGIC,4,int)  
#define MOTOR_IOC_MAXNR     4

#define PRN_TOTAL_BYTE	48
#define PRN_OUT_BYTE	4

#undef PRNSPI

#define DEBUG 1

#ifdef DEBUG
#define DBG(fmt, args...) \
	do{ \
		printk(KERN_ERR fmt, ##args); \
	}while(0)
#else
#define DBG(fmt, args...)
#endif

step_motor_info *motor_data;

struct spi_device *prn_spidev;

u8 wbuf[48];

static void mt_a(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->mctl_a, 0);
	}else{
		rc = gpio_direction_output(motor_data->mctl_a, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio A, rc = %d\n", rc);
	}
}

static void mt_na(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->mctl_na, 0);
	}else{
		rc = gpio_direction_output(motor_data->mctl_na, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio NA, rc = %d\n", rc);
	}
}

static void mt_b(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->mctl_b, 0);
	}else{
		rc = gpio_direction_output(motor_data->mctl_b, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio B, rc = %d\n", rc);
	}
}

static void mt_nb(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->mctl_nb, 0);
	}else{
		rc = gpio_direction_output(motor_data->mctl_nb, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio NB, rc = %d\n", rc);
	}
}

static void mt_mosi(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->prn_mosi, 0);
	}else{
		rc = gpio_direction_output(motor_data->prn_mosi, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio mosi, rc = %d\n", rc);
	}
}

static void mt_clk(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->prn_clk, 0);
	}else{
		rc = gpio_direction_output(motor_data->prn_clk, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio CLK, rc = %d\n", rc);
	}
}

static void mt_latch(int32_t set)
{
	int32_t rc = 0;

	if(set == 0){
		rc = gpio_direction_output(motor_data->prn_latch, 0);
	}else{
		rc = gpio_direction_output(motor_data->prn_latch, 1);
	}

	if(rc){
		DBG(">>> Failed to set gpio LATCH, rc = %d\n", rc);
	}
}

static void mtStep(void)
{
	switch(motor_data->mt_phase % 0x08)
	{
		case 0:
			mt_a(1);
			mt_na(0);
			mt_b(0);
			mt_nb(0);
			break;
		case 1:
			mt_a(1);
			mt_na(0);
			mt_b(0);
			mt_nb(1);
			break;
		case 2:
			mt_a(0);
			mt_na(0);
			mt_b(0);
			mt_nb(1);
			break;
		case 3:
			mt_a(0);
			mt_na(1);
			mt_b(0);
			mt_nb(1);
			break;
		case 4:
			mt_a(0);
			mt_na(1);
			mt_b(0);
			mt_nb(0);
			break;
		case 5:
			mt_a(0);
			mt_na(1);
			mt_b(1);
			mt_nb(0);
			break;
		case 6:
			mt_a(0);
			mt_na(0);
			mt_b(1);
			mt_nb(0);
			break;
		case 7:
			mt_a(1);
			mt_na(0);
			mt_b(1);
			mt_nb(0);
			break;
	}

	motor_data->mt_phase++;
	if(motor_data->mt_phase == 8)
		motor_data->mt_phase = 0;
}

static void mtPower(int ctrl)
{
	int32_t rc = 0;

	if(ctrl == 0)
	{
		rc = gpio_direction_output(motor_data->ppwr_en, 0);
		if(rc){
			DBG(">>> Failed to disable ppwr, rc = %d\n", rc);
		}
	}else{
		rc = gpio_direction_output(motor_data->ppwr_en, 1);
		if(rc){
			DBG(">>> Failed to enable ppwr, rc = %d\n", rc);
		}
	}
}

static void mt_strobe(int ctrl)
{
	int32_t rc = 0;

	if(ctrl == 0)
	{
		rc = gpio_direction_output(motor_data->prn_strobe, 0);
		if(rc){
			DBG(">>> Failed to disable strobe, rc = %d\n", rc);
		}
	}else{
		rc = gpio_direction_output(motor_data->prn_strobe, 1);
		if(rc){
			DBG(">>> Failed to enable strobe, rc = %d\n", rc);
		}
	}
}

void printMotorStep_test(int c)
{
	int i;
	int st = 1;

	if(c <= 0){
		c = 0;
		return;
	}

	for(i = 0, st = 1; i < c; i++)
	{
		mtPower(st);
		mtStep();
		mdelay(3);
		st = (st == 1)? 0:1;
	}

	mtPower(0);
}

static ssize_t motor_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long count = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: count = %ld... \n", __func__, count);

    if(count <= 0) count = 0;

	printMotorStep_test(count);

    return size;
}

static ssize_t motor_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mtPower(0);
	}else{
		mtPower(1);
	}

    return size;
}

#if 0
static ssize_t motor_a_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mtPower(0);
		mdelay(1);
		mt_a(0);
	}else{
		mtPower(1);
		mdelay(1);
		mt_a(1);
	}

    return size;
}

static ssize_t motor_na_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mtPower(0);
		mdelay(1);
		mt_na(0);
	}else{
		mtPower(1);
		mdelay(1);
		mt_na(1);
	}

    return size;
}

static ssize_t motor_b_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mtPower(0);
		mdelay(1);
		mt_b(0);
	}else{
		mtPower(1);
		mdelay(1);
		mt_b(1);
	}

    return size;
}

static ssize_t motor_nb_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mtPower(0);
		mdelay(1);
		mt_nb(0);
	}else{
		mtPower(1);
		mdelay(1);
		mt_nb(1);
	}

    return size;
}
#endif

#ifndef PRNSPI
static ssize_t prn_clk_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mt_clk(0);
	}else{
		mt_clk(1);
	}

    return size;
}

static ssize_t prn_mosi_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mt_mosi(0);
	}else{
		mt_mosi(1);
	}

    return size;
}

static ssize_t motor_strobe_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mtPower(0);
		mdelay(1);
		mt_strobe(0);
	}else{
		mtPower(1);
		mdelay(1);
		mt_strobe(1);
		mdelay(1);
		mt_strobe(0);
	}

    return size;
}

static ssize_t prn_latch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mt_latch(0);
	}else{
		mt_latch(1);
	}

    return size;
}
#endif

static void print_dot(u8 *buf, int num)
{
	int i, j;
	mt_clk(0);
	mt_mosi(0);
	mt_latch(1);
	mt_strobe(0);
	mdelay(1);
	
	mtPower(1);
	mdelay(1);
	
	for(i = 0; i < num; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if((buf[i] << j) & 0x80)
				mt_mosi(1);
			else
				mt_mosi(0);
			udelay(100);
			mt_clk(1);
			udelay(100);
			mtPower(0);
			mt_clk(0);
			udelay(100);
			mtPower(1);
		}
		
	}
	
	mt_mosi(0);
	
	udelay(100);
	mt_latch(0);
	mtPower(0);
	udelay(100);
	mt_latch(1);
	mtPower(1);

	udelay(100);
	
	mt_strobe(1);
	mdelay(1);
	mt_strobe(0);

	mdelay(20);
	mtPower(0);
}

static void print_buf(u8 *buf)
{
	int i,j;
	u8 dot_buf[PRN_TOTAL_BYTE];
	
	for(i = 0; i < PRN_TOTAL_BYTE/PRN_OUT_BYTE; i++)
	{
		memset(dot_buf, 0, PRN_TOTAL_BYTE);
		for(j = 0; j < PRN_OUT_BYTE; j++)
		{
			dot_buf[i * PRN_OUT_BYTE + j] = buf[i * PRN_OUT_BYTE + j];
		}
		print_dot(dot_buf, PRN_TOTAL_BYTE);
	}
	
}

static ssize_t prn_data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	int i;
	unsigned long s = simple_strtoul(buf, &e, 10);

    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		mt_clk(0);
		mt_mosi(0);
		mt_latch(1);
		mt_strobe(0);
	}else{
		for(i = 0; i < 48; i++)
		{
			wbuf[i] = 0xaa;
		}
		print_buf(wbuf);
	}

    return size;
}

#ifdef PRNSPI
#if 1
static void spidev_complete(void *arg)
{
	complete(arg);
}
#endif

static void spidev_write(void)
{
#if 1
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	int i;
	struct spi_transfer	t = {
		.tx_buf		= wbuf,
		.len = 48,
	};
	struct spi_message	m;
	
	for(i = 0; i < 48; i++)
	{
		wbuf[i] = i;
	}

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	m.complete = spidev_complete;
	m.context = &done;

	if(prn_spidev == NULL)
	{
		DBG(">>> [%s]: prn_spidev NULL... \n", __func__);
	}else{
		DBG(">>> [%s]: spi write... \n", __func__);
		status = spi_async_locked(prn_spidev, &m);
	}
	
	if (status == 0) {
		wait_for_completion(&done);
		status = m.status;
		if (status == 0)
			status = m.actual_length;
	}

#else
	u8 rbuf[1];
	
	spi_write_then_read(prn_spidev, wbuf, sizeof(*wbuf),
			rbuf, sizeof(*rbuf));
#endif

}

#else

static void spidev_write(void)
{

}

#endif


static ssize_t prn_spi_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *e;
	unsigned long s = simple_strtoul(buf, &e, 10);
	
	
    DBG(">>> [%s]: s = %ld... \n", __func__, s);

	if(s == 0){
		
	}else{
		spidev_write();
	}

    return size;
}

static struct device_attribute motor_test_attribute = __ATTR(prn_test,0666,NULL, motor_test_store);
static struct device_attribute motor_power_attribute = __ATTR(prn_power,0666,NULL, motor_power_store);
static struct device_attribute motor_data_attribute = __ATTR(prn_data,0666,NULL, prn_data_store);
//static struct device_attribute motor_a_attribute = __ATTR(prn_a,0666,NULL, motor_a_store);
//static struct device_attribute motor_na_attribute = __ATTR(prn_na,0666,NULL, motor_na_store);
//static struct device_attribute motor_b_attribute = __ATTR(prn_b,0666,NULL, motor_b_store);
//static struct device_attribute motor_nb_attribute = __ATTR(prn_nb,0666,NULL, motor_nb_store);

#ifndef PRNSPI
static struct device_attribute motor_clk_attribute = __ATTR(prn_clk,0666,NULL, prn_clk_store);
static struct device_attribute motor_mosi_attribute = __ATTR(prn_mosi,0666,NULL, prn_mosi_store);
static struct device_attribute motor_strobe_attribute = __ATTR(prn_strobe,0666,NULL, motor_strobe_store);
static struct device_attribute motor_latch_attribute = __ATTR(prn_latch,0666,NULL, prn_latch_store);
#endif

static struct device_attribute pt48d_spi_attribute = __ATTR(prn_spi,0666,NULL, prn_spi_store);

static struct attribute *motor_attrs [] =
{
	&motor_test_attribute.attr,
	&motor_power_attribute.attr,
	&motor_data_attribute.attr,
//	&motor_a_attribute.attr,
//	&motor_na_attribute.attr,
//	&motor_b_attribute.attr,
//	&motor_nb_attribute.attr,
#ifndef PRNSPI
	&motor_clk_attribute.attr,
	&motor_mosi_attribute.attr,
	&motor_strobe_attribute.attr,
	&motor_latch_attribute.attr,
#endif
	&pt48d_spi_attribute.attr,
    NULL
};

static struct attribute_group motor_attribute_group = {
	.attrs = motor_attrs,
};

#ifdef PRNSPI
static int pt48d_spi_probe(struct spi_device *spi)
{
	DBG(">>> [%s]: Entering... \n", __func__);
	
	spi->bits_per_word = 8;
	
	prn_spidev = spi;
	
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
#endif



static int32_t stepMotor_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	step_motor_info *info;
	struct device_node *of_node = pdev->dev.of_node;

	DBG(">>> [%s]: Entering... \n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
	{
	    DBG("Alloc GFP_KERNEL memory failed.");
	    return -ENOMEM;
	}

	motor_data = info;

	rc = of_property_read_u32(of_node,
			"prn-steps", &info->stepnum);
	DBG(">>> steps=%d\n", info->stepnum);

	info->ppwr_en = of_get_named_gpio(of_node, "prn-ppwr-en", 0);
	if(info->ppwr_en < 0){
		DBG(">>> Looking up ppwr_en failed.\n");
	}else{
		rc = gpio_request(info->ppwr_en, "PPWR_EN");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->ppwr_en, rc);
			return rc;
		}
		DBG(">>> Request ppwr_en gpio %d success\n", info->ppwr_en);
		rc = gpio_direction_output(info->ppwr_en, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->ppwr_en, rc);
			return rc;
		}else{
			DBG(">>> Set ppwr_en gpio %d success\n", info->ppwr_en);
		}
	}

	info->mctl_a = of_get_named_gpio(of_node, "prn-mctl-a", 0);
	if(info->mctl_a < 0){
		DBG(">>> Looking up mctl_a failed.\n");
	}else{
		rc = gpio_request(info->mctl_a, "MCTL_A");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->mctl_a, rc);
			return rc;
		}
		DBG(">>> Request mctl_a gpio %d success\n", info->mctl_a);
		rc = gpio_direction_output(info->mctl_a, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->mctl_a, rc);
			return rc;
		}else{
			DBG(">>> Set mctl_a gpio %d success\n", info->mctl_a);
		}
	}

	info->mctl_na = of_get_named_gpio(of_node, "prn-mctl-na", 0);
	if(info->mctl_na < 0){
		DBG(">>> Looking up mctl_na failed.\n");
	}else{
		rc = gpio_request(info->mctl_na, "MCTL_NA");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->mctl_na, rc);
			return rc;
		}
		DBG(">>> Request mctl_na gpio %d success\n", info->mctl_na);
		rc = gpio_direction_output(info->mctl_na, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->mctl_na, rc);
			return rc;
		}else{
			DBG(">>> Set mctl_na gpio %d success\n", info->mctl_na);
		}
	}

	info->mctl_b = of_get_named_gpio(of_node, "prn-mctl-b", 0);
	if(info->mctl_b < 0){
		DBG(">>> Looking up mctl_b failed.\n");
	}else{
		rc = gpio_request(info->mctl_b, "MCTL_B");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->mctl_b, rc);
			return rc;
		}
		DBG(">>> Request mctl_b gpio %d success\n", info->mctl_b);
		rc = gpio_direction_output(info->mctl_b, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->mctl_b, rc);
			return rc;
		}else{
			DBG(">>> Set mctl_b gpio %d success\n", info->mctl_b);
		}
	}

	info->mctl_nb = of_get_named_gpio(of_node, "prn-mctl-nb", 0);
	if(info->mctl_nb < 0){
		DBG(">>> Looking up mctl_nb failed.\n");
	}else{
		rc = gpio_request(info->mctl_nb, "MCTL_NB");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->mctl_nb, rc);
			return rc;
		}
		DBG(">>> Request mctl_nb gpio %d success\n", info->mctl_nb);
		rc = gpio_direction_output(info->mctl_nb, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->mctl_nb, rc);
			return rc;
		}else{
			DBG(">>> Set mctl_nb gpio %d success\n", info->mctl_nb);
		}
	}

#ifndef PRNSPI
	info->prn_strobe = of_get_named_gpio(of_node, "prn-strobe", 0);
	if(info->prn_strobe < 0){
		DBG(">>> Looking up prn_strobe failed.\n");
	}else{
		rc = gpio_request(info->prn_strobe, "STROBE");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->prn_strobe, rc);
			return rc;
		}
		DBG(">>> Request prn_strobe gpio %d success\n", info->prn_strobe);
		rc = gpio_direction_output(info->prn_strobe, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->prn_strobe, rc);
			return rc;
		}else{
			DBG(">>> Set prn_strobe gpio %d success\n", info->prn_strobe);
		}
	}

	info->prn_latch = of_get_named_gpio(of_node, "prn-latch", 0);
	if(info->prn_latch < 0){
		DBG(">>> Looking up prn_latch failed.\n");
	}else{
		rc = gpio_request(info->prn_latch, "PRN-LATCH");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->prn_latch, rc);
			return rc;
		}
		DBG(">>> Request prn_latch gpio %d success\n", info->prn_latch);
		rc = gpio_direction_output(info->prn_latch, 1);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->prn_latch, rc);
			return rc;
		}else{
			DBG(">>> Set prn_latch gpio %d success\n", info->prn_latch);
		}
	}

	info->prn_mosi = of_get_named_gpio(of_node, "prn-mosi", 0);
	if(info->prn_mosi < 0){
		DBG(">>> Looking up prn_mosi failed.\n");
	}else{
		rc = gpio_request(info->prn_mosi, "PRN-MOSI");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->prn_mosi, rc);
			return rc;
		}
		DBG(">>> Request prn_mosi gpio %d success\n", info->prn_mosi);
		rc = gpio_direction_output(info->prn_mosi, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->prn_mosi, rc);
			return rc;
		}else{
			DBG(">>> Set prn_mosi gpio %d success\n", info->prn_mosi);
		}
	}

	info->prn_clk = of_get_named_gpio(of_node, "prn-clk", 0);
	if(info->prn_clk < 0){
		DBG(">>> Looking up prn_clk failed.\n");
	}else{
		rc = gpio_request(info->prn_clk, "PRN-CLK");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->prn_clk, rc);
			return rc;
		}
		DBG(">>> Request prn_clk gpio %d success\n", info->prn_clk);
		rc = gpio_direction_output(info->prn_clk, 0);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->prn_clk, rc);
			return rc;
		}else{
			DBG(">>> Set prn_clk gpio %d success\n", info->prn_clk);
		}
	}
#endif

	msleep(50);

	info->dpwr_en = of_get_named_gpio(of_node, "prn-dpwr-en", 0);
	if(info->dpwr_en < 0){
		DBG(">>> Looking up dpwr_en failed.\n");
	}else{
		rc = gpio_request(info->dpwr_en, "DPWR_EN");
		if(rc){
			DBG(">>> Failed to request gpio %d, rc = %d\n", info->dpwr_en, rc);
			return rc;
		}
		DBG(">>> Request dpwr_en gpio %d success\n", info->dpwr_en);
		rc = gpio_direction_output(info->dpwr_en, 1);
		if(rc){
			DBG(">>> Failed to set gpio %d, rc = %d\n", info->dpwr_en, rc);
			return rc;
		}else{
			DBG(">>> Set dpwr_en gpio %d success\n", info->dpwr_en);
		}
	}

	info->mt_phase = 0;

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

	rc = sysfs_create_group(&info->prn_device->kobj, &motor_attribute_group);
	if (rc < 0)
	{
		printk(KERN_ERR ">>> %s:could not create sysfs group for motor\n", __func__);
	}
	
#ifdef PRNSPI
	rc = spi_register_driver(&pt48d_spi_driver);
	if(rc < 0)
	{
		DBG(">>> [%s]: could not register pt48d_spi_driver\n", __func__);
	}else{
		DBG(">>> [%s]: register pt48d_spi_driver success.\n", __func__);
	}
#endif

	return 0;
}

static const struct of_device_id msm_printer_dt_match[] = {
	{.compatible = "printer,pt48d"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_printer_dt_match);

static struct platform_driver msm_printer_pt48d_driver = {
	.driver = {
		.name = PRINTER_NAME, 
		.owner = THIS_MODULE,
		.of_match_table = msm_printer_dt_match,
	},
};

static int stepMotor_init(void)
{
	DBG(">>> [%s]: Entering... \n", __func__);
	return platform_driver_probe(&msm_printer_pt48d_driver,
				stepMotor_probe);
#if 0
	dev_t devno = MKDEV(motor_major, 0);

	if(motor_major)
		ret = register_chrdev_region(devno, 1, "motordev");
	else{
		ret = alloc_chrdev_region(&devno, 0, 1, "motordev");
		motor_major = MAJOR(devno);
	}
	
#endif

	return 0;
}

static void stepMotor_exit(void)
{


}

module_init(stepMotor_init);
module_exit(stepMotor_exit);

//MODULE_LICENSE("GPL v2");
//MODULE_DESCRIPTION("Step motor controller driver");
//MODULE_AUTHOR("shenguangmin <shenguangmin@19zn.cc>");

