#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/amba/bus.h>
#include <linux/pwm.h>
#include <mach/misc.h>
#include <mach/hardware.h>
#include "printer_hal.h"
#include "printer_spi.h"
#include "../spi/pl022-spi.h"

extern int STB_PWM_CTL ;
struct bcm5892_adc_dev *adc_dev;
static struct pwm_device *prn_stb_pwm = NULL;
/**
 * config power pin
 * driver pin and logic are GPE3, high level is power on
 */
int s_PrnPowerConfig(void)
{
	if (gpio_request(PRN_POWER_CONTROL, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_POWER_CONTROL);
		return -EBUSY;
	}

	return 0;
}

/**
 * driver power control
 * void s_PrnPowerDriver(unsigned int mode)
 * @param[in]
 *  mode	MODE_ON
 *		MODE_OFF
 */
void s_PrnPowerDriver(unsigned int mode)
{
	gpio_direction_output(PRN_POWER_CONTROL, mode ? 1 : 0);
}

/**
 * logic power control
 * void s_PrnPowerLogic(unsigned int mode)
 * @param[in]
 *  mode	MODE_ON
 *		MODE_OFF
 */
void s_PrnPowerLogic(unsigned int mode)
{
	gpio_direction_output(PRN_POWER_CONTROL, mode ? 1 : 0);
}

/**
 * config latch and STB pin
 * GPE8 is STB, GEP9 is latch, low level is STB, low level is latch
 * void s_PrnSTBLATConfig(void)
 */
int s_PrnSTBLATConfig(void)
{
	if (STB_PWM_CTL){
		if (gpio_request(PRN_STB_PWM_CONTROL, "printer") < 0) {
			printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
			    __func__, PRN_STB_PWM_CONTROL);
			return -EBUSY;
		}
		prn_stb_pwm = pwm_request(1, "printer_stb_pwm");
	} else {
		if (gpio_request(PRN_STB_CONTROL, "printer") < 0) {
			printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
			    __func__, PRN_STB_CONTROL);
			return -EBUSY;
		}
	}
	if (gpio_request(PRN_LAT_CONTROL, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_LAT_CONTROL);
		return -EBUSY;
	}

	return 0;
}

/**
 * STB control
 * void s_PrnSTB(unsigned int mode)
 * @param[in]
 *  mode	MODE_ON
 *		MODE_OFF
 */
void s_PrnSTB(unsigned int mode)
{
	unsigned int duty,period_ns;
	if (STB_PWM_CTL){
		if (mode){
			duty = mode;
			period_ns = 20 * 1000 * 1000;

			reg_gpio_iotr_set_pin_type(PRN_STB_PWM_CONTROL, 
				GPIO_PIN_TYPE_ALTERNATIVE_FUNC0);
			pwm_config(prn_stb_pwm, duty * 1000, period_ns);
			pwm_enable(prn_stb_pwm);
		} else {
			pwm_disable(prn_stb_pwm);
			reg_gpio_iotr_set_pin_type(PRN_STB_PWM_CONTROL,GPIO_PIN_TYPE_OUTPUT);
			gpio_direction_output(PRN_STB_PWM_CONTROL, 1);
		}
	} else {
		gpio_direction_output(PRN_STB_CONTROL, mode ? 0 : 1);
	}
}

/**
 * Latch control
 * void s_PrnLAT(unsigned int mode)
 * @param[in]
 *  mode	MODE_ON
 *		MODE_OFF
 */
void s_PrnLAT(unsigned int mode)
{
	gpio_direction_output(PRN_LAT_CONTROL, mode ? 0 : 1);
}

/**
 * config step motor control pin
 * GPE4,5,6,7
 * void s_PrnMotorPhaseConfig(void)
 */
int s_PrnMotorPhaseConfig(void)
{
	if (gpio_request(PRN_PHASE_A_CONTROL, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_PHASE_A_CONTROL);
		return -EBUSY;
	}

	if (gpio_request(PRN_PHASE_NA_CONTROL, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_PHASE_NA_CONTROL);
		return -EBUSY;
	}

	if (gpio_request(PRN_PHASE_B_CONTROL, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_PHASE_B_CONTROL);
		return -EBUSY;
	}

	if (gpio_request(PRN_PHASE_NB_CONTROL, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_PHASE_NB_CONTROL);
		return -EBUSY;
	}

	return 0;
}

/**
 * control motor step
 * void s_PrnMotorPhase(unsigned int PhaseCtrl)
 * @param[in]
 * PhaseCtrl
 *	PHASE_A_H
 *	PHASE_A_L
 *	PHASE_NA_H
 *	PHASE_NA_L
 *	PHASE_B_H
 *	PHASE_B_L
 *	PHASE_NB_H
 */
void s_PrnMotorPhase(unsigned int PhaseCtrl)
{
	switch (PhaseCtrl) {
	case PHASE_A_H:
		gpio_direction_output(PRN_PHASE_A_CONTROL, 1);
		break;

	case PHASE_A_L:
		gpio_direction_output(PRN_PHASE_A_CONTROL, 0);
		break;

	case PHASE_NA_H:
		gpio_direction_output(PRN_PHASE_NA_CONTROL, 1);
		break;

	case PHASE_NA_L:
		gpio_direction_output(PRN_PHASE_NA_CONTROL, 0);
		break;

	case PHASE_B_H:
		gpio_direction_output(PRN_PHASE_B_CONTROL, 1);
		break;

	case PHASE_B_L:
		gpio_direction_output(PRN_PHASE_B_CONTROL, 0);
		break;

	case PHASE_NB_H:
		gpio_direction_output(PRN_PHASE_NB_CONTROL, 1);
		break;

	case PHASE_NB_L:
		gpio_direction_output(PRN_PHASE_NB_CONTROL, 0);
		break;

	default:
		break;
	}
}

/**
 * config lack paper pin
 * GPB6
 * void s_PrnIoCheckConfig(void)
 */
int s_PrnIoCheckConfig(void)
{
#ifdef LACK_PAPER_CHECK_IO
	if (gpio_request(PRN_CHECK_PAPER, "printer") < 0) {
		printk(KERN_ERR "%s(): Unable to request GPIO pin %d\n",
		    __func__, PRN_CHECK_PAPER);
		return -EBUSY;
	}
#endif
	return 0;
}

/**
 * voltage check
 * int s_PrnVoltageCheck(void)
 * @retval
 *	1:	voltage > 9.5V
 *	0:	normal
 */
int s_PrnVoltageCheck(void)
{
	int adc_value;
	int i = 0;
	bcm5892_adc_prefetch(adc_dev, ADC_VOLT_CHANNEL);
	do {
		adc_value = bcm5892_adc_fetch(adc_dev);
		if (i++ >= ADC_READ_TIMES){
			i = 0;
			break;
		}
	} while (adc_value < 0);
	if (adc_value >= ADC_MAX_VALUE_VOLT) 
		return 1;
	else
		return 0;
}
/**
 * lack paper check
 * int s_PrnHavePaper(void)
 * @retval
 *	1:	has paper
 *	0:	no paper
 */
int s_PrnHavePaper(void)
{
#ifdef LACK_PAPER_CHECK_IO
	return (!(gpio_get_value(PRN_CHECK_PAPER)));
#else
	int adc_value;
	int i = 0;
	
	bcm5892_adc_prefetch(adc_dev, ADC_LACKPAPER_CHANNEL);
	do {
		adc_value = bcm5892_adc_fetch(adc_dev);
		if (i++ >= ADC_READ_TIMES){
			i = 0;
			break;
		}
	} while (adc_value < 0);

	return (adc_value < 70);
#endif
}

/**
 * ADC init, used to get the struct about ADC from linux
 * int s_PrnADCConfig(void)
 */
int s_PrnADCConfig(void)
{
	struct amba_device pdev;

	adc_dev = bcm5892_adc_register(&pdev);

	return 0;
}

/*
 * status:
 *	0: get value
 *	1: start convert
 *	other: convert and get value
 */
int s_PrnTempAdc(int status)
{
	int adc_value = 0;
	int i = 0;
	switch (status) {
	case 0:
		do {
			adc_value = bcm5892_adc_fetch(adc_dev);
			if (i++ >= ADC_READ_TIMES){
				i = 0;
				break;
			}
		} while (adc_value < 0);
		break;
	case 1:
		bcm5892_adc_prefetch(adc_dev, ADC_TEMP_CHANNEL);
		break;
	default:
		/*
		 * XXX: bcm5892_adc_read() may sleep, it cannot be used in
		 * an interrupt handler.
		 */
#if 0
		adc_value = bcm5892_adc_read(adc_dev, ADC_TEMP_CHANNEL);
#endif
		bcm5892_adc_prefetch(adc_dev, ADC_TEMP_CHANNEL);
		do {
			adc_value = bcm5892_adc_fetch(adc_dev);
			if (i++ >= ADC_READ_TIMES){
				i = 0;
				break;
			}
		} while (adc_value < 0);
		break;
	}

	return adc_value;
}

void s_PrnGPIOFree(void)
{
	gpio_free(PRN_POWER_CONTROL);
	gpio_free(PRN_STB_CONTROL);
	gpio_free(PRN_LAT_CONTROL);
	gpio_free(PRN_PHASE_A_CONTROL);
	gpio_free(PRN_PHASE_NA_CONTROL);
	gpio_free(PRN_PHASE_B_CONTROL);
	gpio_free(PRN_PHASE_NB_CONTROL);
	gpio_free(PRN_CHECK_PAPER);
}

/****************************** SPI ******************************/
#define SPI_MODE_MASK	(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH | \
			 SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP | \
			 SPI_NO_CS | SPI_READY)

struct spi_device *spi;
struct spi_transfer tx;

void spi_send_data(unsigned char *buf, int len)
{
	struct spi_transfer *p_tx = &tx;

	p_tx->tx_buf = buf;
	p_tx->len = len;

	spi_send_poll(spi, p_tx);
}

static void spi_init_speed(void)
{
	int retval;
	u32 save;

	spi = spidev_get(3);
	save = spi->max_speed_hz;
	spi->max_speed_hz = PRN_SPI_BAUD;
	retval = spi_setup(spi);
	if (retval < 0) {
		spi->max_speed_hz = save;
	}
	else {
		printk(KERN_INFO "%d Hz (max)\n", spi->max_speed_hz);
	}
}

static void spi_init_mode(void)
{
	int retval;

	u32 tmp = SPI_MODE_0;
	u8 save;

	spi = spidev_get(3);
	save = spi->mode;
	if (tmp & ~SPI_MODE_MASK) {
		retval = -EINVAL;
	}

	tmp |= spi->mode & ~SPI_MODE_MASK;
	spi->mode = (u8)tmp;
	retval = spi_setup(spi);
	if (retval < 0) {
		spi->mode = save;
	}
	else {
		printk(KERN_INFO "spi mode %02x\n", tmp);
	}
}

void s_SPIConfig(void)
{
	spi_init_speed();
	spi_init_mode();
}

/******************* Idle Mode Control ********************/
void printer_idle_suspend(void)
{
	gpio_direction_output(PRN_POWER_CONTROL, 0);
	gpio_direction_output(PRN_STB_CONTROL, 1);
	gpio_direction_output(PRN_LAT_CONTROL, 1);
	gpio_direction_output(PRN_PHASE_A_CONTROL, 0);
	gpio_direction_output(PRN_PHASE_NA_CONTROL, 0);
	gpio_direction_output(PRN_PHASE_B_CONTROL, 0);
	gpio_direction_output(PRN_PHASE_NB_CONTROL, 0);
#ifdef LACK_PAPER_CHECK_IO
	gpio_direction_output(PRN_CHECK_PAPER, 0);
#endif
}

void printer_idle_resume(void)
{
#ifdef LACK_PAPER_CHECK_IO
	gpio_direction_input(PRN_CHECK_PAPER);
#endif
}

