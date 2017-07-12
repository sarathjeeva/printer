/*  
 *  Copyright (c) 2017-5-25
 *  
 *  step motor header file  
 */

#ifndef MOTOR_H
#define MOTOR_H

#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>

typedef struct _step_motor_info
{
        u32 openCnt;
        u32 pulse;
        u32 phase_a;
        u32 phase_na;
        u32 phase_b;
        u32 phase_nb;
        u32 direction_left;
        u32 direction_right;
        u32 irqCntLeft;
        u32 irqCntRight;
        spinlock_t lock;
        u32 stepnum;

        int dpwr_en;
        int ppwr_en;
        int mctl_a;
        int mctl_na;
        int mctl_b;
        int mctl_nb;
        int prn_strobe;
        int prn_mosi;
        int prn_clk;
        int prn_latch;

        u32 mt_phase;

        struct class *prn_class;
        struct device *prn_device;

        dev_t devno;
        unsigned int major;
        struct cdev cdev;

        struct spi_device *prn_spidev;
        u8 *prn_buf;

}step_motor_info;

#endif // MOTOR_H


