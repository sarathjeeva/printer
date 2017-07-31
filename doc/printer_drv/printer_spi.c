/*****************************************************************************
* Copyright 2009 - 2011 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/

#include <linux/spi/spi.h>
#include <mach/reg_gpio.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include "../spi/pl022-spi.h"
#include "printer_spi.h"

#define CPSR_MAX 254  /* prescale divisor */
#define CPSR_MIN 2    /* prescale divisor min (must be divisible by 2) */
#define SCR_MAX 255   /* other divisor */
#define SCR_MIN 0

#define SPI_INPUT_CLOCK (66000000)  /* better if we can get this from the board */
#define MAX_CALC_SPEED_HZ (SPI_INPUT_CLOCK / (CPSR_MIN * (1 + SCR_MIN)))
#define MIN_CALC_SPEED_HZ (SPI_INPUT_CLOCK / (254 * 256))
#define MAX_SUPPORTED_SPEED_HZ (12000000)

#define MAX_SPEED_HZ ( (MAX_SUPPORTED_SPEED_HZ < MAX_CALC_SPEED_HZ) ? MAX_SUPPORTED_SPEED_HZ : MAX_CALC_SPEED_HZ)
#define MIN_SPEED_HZ (MIN_CALC_SPEED_HZ)

#define SPI_TXFIFO_SIZE (8)
#define SPI_RXFIFO_SIZE (16)
#define TEST_BUF_SIZE 4200

#define PRINTK (!debug_on) ?: printk
#define PROVIDE_LOOPBACK_TEST 0

/* ================================================================================================ */
/*  Forward Definitions (required for SPI and AMBA structs & vars) */
/* ================================================================================================ */

#if PROVIDE_LOOPBACK_TEST
static void pl022_spi_loopback_test (struct device *pDev, int startpos);
#endif


#define FIRST_NONZERO(a,b,c) ({ typeof(a) _a = a; typeof(b) _b = b; typeof(c) _c = c; _a ? _a : (_b ? _b : _c); })

static int debug_on = 0;


/* ================================================================================================
 *  SPI Master Device Configuration/State
 * ================================================================================================
 */
struct pl022_master_device_data {
	struct work_struct work;

	struct workqueue_struct *wq;
	struct list_head msg_queue;
	spinlock_t lock;      /* lock for queue access */
	spinlock_t fifolock;  /* lock on fifo access */
	struct completion isr_completion;

	int polled;

	struct spi_transfer *cur_txfr;
	unsigned cur_txfr_numwords_left_tx;
	unsigned cur_txfr_numwords_left_rx;
	unsigned cur_txfr_tx_idx;
	unsigned cur_txfr_rx_idx;

	struct spi_device *last_spi_device;  /* for debug and test */
	unsigned errors;
};

struct pl022_platform_data {
	void   __iomem	  *base;	       /* PL022 SPI peripheral register base */
	int		  cs_gpio_num;
	int		  bus_num;
	unsigned	  is_enabled;
	unsigned          enable_dma;
	int               dma_tx_id;
	int               dma_rx_id;
};

static void config_hardware(void __iomem *base, unsigned speed, uint8_t mode, int data_size)
{
	 /* half_divisor = clock / (2*speed), rounded up: */
	unsigned half_divisor = (SPI_INPUT_CLOCK + (speed * 2 - 1)) / (speed*2);
	unsigned best_err = half_divisor;
	unsigned best_scr = SCR_MAX;
	unsigned best_half_cpsr = CPSR_MAX/2;
	unsigned scr, half_cpsr, err;

	unsigned polarity = (mode & SPI_CPOL);
	unsigned phase = (mode & SPI_CPHA);


	/* Loop over possible SCR values, calculating the appropriate CPSR and finding the best match
	 * For any SPI speeds above 260KHz, the first iteration will be it, and it will stop.
	 * The loop is left in for completeness */
	PRINTK(KERN_INFO "Setting up PL022 for: %dHz, mode %d, %d bits (target %d)\n",
	       speed, mode, data_size, half_divisor);

	for (scr = SCR_MIN; scr <= SCR_MAX; ++scr) {
		/* find the right cpsr (rounding up) for the given scr */
		half_cpsr = ((half_divisor + scr) / (1+scr));

		if (half_cpsr < CPSR_MIN/2)
			half_cpsr = CPSR_MIN/2;
		if (half_cpsr > CPSR_MAX/2)
			continue;

		err = ((1+scr) * half_cpsr) - half_divisor;

		if (err < best_err) {
			best_err = err;
			best_scr = scr;
			best_half_cpsr = half_cpsr;
			if (err == 0)
				break;
		}
	}

	PRINTK(KERN_INFO "Actual clock rate: %dHz\n", SPI_INPUT_CLOCK / (2 * best_half_cpsr * (1+best_scr)));

	PRINTK(KERN_INFO "Setting PL022 config: %08x %08x %08x\n",
		PL022_SCR_MAP(best_scr) | PL022_SPH_MAP(phase) | PL022_SPO_MAP(polarity) |
		PL022_FRF_SPI | PL022_DSS_MAP(data_size), 2, best_half_cpsr * 2);

	/* Set CR0 params */
	PL022_WRITE_REG(PL022_SCR_MAP(best_scr) | PL022_SPH_MAP(phase) | PL022_SPO_MAP(polarity) |
			PL022_FRF_SPI | PL022_DSS_MAP(data_size), base, PL022_CR0);

	/* Set prescale divisor */
	PL022_WRITE_REG(best_half_cpsr * 2, base, PL022_CPSR);

}


static void toggle_cs(struct spi_device *spi_dev, int *current_cs) {
	struct pl022_platform_data *pPlatform = spi_dev->master->dev.parent->platform_data;

	reg_gpio_set_pin(pPlatform->cs_gpio_num, !*current_cs);
	reg_gpio_iotr_set_pin_type(pPlatform->cs_gpio_num , GPIO_PIN_TYPE_OUTPUT);

	*current_cs = !*current_cs;
}

static void spi3_load_txfifo(int num_to_tx, struct pl022_platform_data *pPlatform,
					   struct pl022_master_device_data *master_data)
{
	struct spi_transfer *txfr = master_data->cur_txfr;

	const uint8_t *tx_buf_8 = txfr->tx_buf;
	const uint16_t *tx_buf_16 = txfr->tx_buf;

    int i;
	
	if (num_to_tx > master_data->cur_txfr_numwords_left_tx)
		num_to_tx = master_data->cur_txfr_numwords_left_tx;
	
	master_data->cur_txfr->bits_per_word = 8;

	if (master_data->cur_txfr->bits_per_word <= 8) {
		/* bytes or smaller */
		for (i = 0; i < num_to_tx; ++i) {
			/* wait until txfifo is not full */
			while ((PL022_REG(pPlatform->base, PL022_SR) & PL022_SR_TNF) == 0);
			PL022_WRITE_REG(tx_buf_8 ? tx_buf_8[master_data->cur_txfr_tx_idx++] :
					0, pPlatform->base, PL022_DR);
		}
	} else {
		/* bigger words */
		for (i = 0; i < num_to_tx; ++i) {
			/* wait until txfifo is not full */
			while ((PL022_REG(pPlatform->base, PL022_SR) & PL022_SR_TNF) == 0);

			PL022_WRITE_REG(tx_buf_16 ? tx_buf_16[master_data->cur_txfr_tx_idx++] :
					0, pPlatform->base, PL022_DR);
		}
	}

	master_data->cur_txfr_numwords_left_tx -= num_to_tx;

	if (tx_buf_8) {
		PRINTK(KERN_INFO "Tx:");
		for (i = num_to_tx; i > 0; --i)
			PRINTK(" %02x", tx_buf_8[master_data->cur_txfr_tx_idx - i]);
		PRINTK("\n");
	} else {
		PRINTK(KERN_INFO "Tx blank(%d)\n", num_to_tx);
	}
}


int spi_send_poll(struct spi_device *spi_dev, struct spi_transfer *txfr) {
	struct pl022_platform_data *pPlatform = spi_dev->master->dev.parent->platform_data;
	struct pl022_master_device_data *master_data = dev_get_drvdata(&spi_dev->master->dev);

	unsigned speed = 5000000;
	unsigned bits_per_word = 8;
	int num_this_time = SPI_TXFIFO_SIZE;
    spi_dev->mode = 0;

	config_hardware(pPlatform->base, speed, spi_dev->mode, bits_per_word);

	master_data->cur_txfr = txfr;
	master_data->cur_txfr_numwords_left_tx = (bits_per_word <= 8) ? txfr->len : (txfr->len / 2);
	master_data->cur_txfr_numwords_left_rx = master_data->cur_txfr_numwords_left_tx;
	master_data->cur_txfr_tx_idx = 0;
	master_data->cur_txfr_rx_idx = 0;

	while (master_data->cur_txfr_numwords_left_tx)
	{
	     spi3_load_txfifo(num_this_time, pPlatform, master_data);
	}
	
	while ((PL022_REG(pPlatform->base, PL022_SR) & PL022_SR_TFE) == 0);//wait for the FIFO is empty
	//while ((PL022_REG(pPlatform->base, PL022_SR) & PL022_SR_BSY) == 1);
	
	return 0;
}



