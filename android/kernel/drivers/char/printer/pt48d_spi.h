/*  
 *  Copyright (c) 2017-7-7
 *  
 *  pt48d_spi.h  
 */

#ifndef PT48D_SPI_H
#define PT48D_SPI_H

#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>

typedef struct _pt48d_spi_data
{
        spinlock_t lock;

        struct class *prn_class;
        struct device *prn_device;

        dev_t devno;
        unsigned int major;
        struct cdev cdev;

        struct spi_device *prn_spidev;
        u8 *prn_buf;

}pt48d_spi_data;



#endif // PT48D_SPI_H


