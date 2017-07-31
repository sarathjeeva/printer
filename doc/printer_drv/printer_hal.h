#pragma once
#ifndef _PRINTER_HAL_H_
#define _PRINTER_HAL_H_

#include <mach/reg_gpio.h>

#define MODE_OFF		0
#define MODE_ON			1

/***** printer pins *****/
#define PRN_POWER_CONTROL	(HW_GPIO3_PIN_MAX + 3)
#define PRN_STB_CONTROL		(HW_GPIO3_PIN_MAX + 8)
#define PRN_STB_PWM_CONTROL	(HW_GPIO2_PIN_MAX + 1)
#define PRN_LAT_CONTROL		(HW_GPIO3_PIN_MAX + 9)
#define PRN_PHASE_A_CONTROL	(HW_GPIO3_PIN_MAX + 4)
#define PRN_PHASE_NA_CONTROL	(HW_GPIO3_PIN_MAX + 5)
#define PRN_PHASE_B_CONTROL	(HW_GPIO3_PIN_MAX + 6)
#define PRN_PHASE_NB_CONTROL	(HW_GPIO3_PIN_MAX + 7)
#define PRN_CHECK_PAPER		(HW_GPIO0_PIN_MAX + 6)

/***** step motor phase defines *****/
#define PHASE_A_H		0
#define PHASE_A_L		1
#define PHASE_NA_H		2
#define PHASE_NA_L		3
#define PHASE_B_H		4
#define PHASE_B_L		5
#define PHASE_NB_H		6
#define PHASE_NB_L		7

/***** printer type defines *****/
#define TYPE_PRN_UNKNOWN	0xff
#define TYPE_PRN_APS		1
#define TYPE_PRN_PRT		2
#define TYPE_PRN_MT183		3
#define TYPE_PRN_LTPJ245G	4

/***** print control *****/
/* control switch, bit control */
/* #define BIT_SEL */

/* control switch, whether the heat time depends on temperature */
#define TEMPERATURE_ADJ

/* control switch, auto adjust the max heat dot number */
/* #define AUTO_ADJ_DOT */
#define MAX_LINE_DOT		(384 - 192)

/* control switch, auto adjust max heat dot number for printing black block */
#define BLACK_ADJ
#define MAX_BLACK_LINE		4
/* the max heat dot number after adjust */
#define BLACK_HEADER_DOT	48

/* max heat dot number */
#define SEL_HEADER_DOT		64

/* over heat protection level1 */
#define TEMPERATURE_LEVEL1	65

/* over heat protection level2 */
#define TEMPERATURE_LEVEL2	70

/* the ADC channel for printer */
#define ADC_TEMP_CHANNEL	3
/* the ADC channel for voltage */
#define ADC_VOLT_CHANNEL	4
#define ADC_MAX_VALUE_VOLT	760

/* control switch, how to check paper */
/* #define LACK_PAPER_CHECK_IO */
/* #define LACK_PAPER_CHECK_ADC_SCAN */
#define LACK_PAPER_CHECK_ADC_NOSCAN

/* the ADC channel for lack of paper */
#define ADC_LACKPAPER_CHANNEL	2

/* the spi baud of printer */
#define PRN_SPI_BAUD		5000000

/* used to adjust the step time */
#define ADJ_MOTOR_PRE		3
#define ADJ_MOTOR_ST1		11
#define ADJ_MOTOR_ST2		0

/* waste time in the timer handler */
#define HEAD_INT_TIME		250

/*****times of read ADC******/
#define ADC_READ_TIMES		100

/***** output functions *****/
int s_PrnPowerConfig(void);
void s_PrnPowerDriver(unsigned int mode);
void s_PrnPowerLogic(unsigned int mode);

int s_PrnSTBLATConfig(void);
void s_PrnSTB(unsigned int mode);
void s_PrnLAT(unsigned int mode);

int s_PrnMotorPhaseConfig(void);
void s_PrnMotorPhase(unsigned int PhaseCtrl);

int s_PrnIoCheckConfig(void);
int s_PrnHavePaper(void);
int s_PrnVoltageCheck(void);

int s_PrnADCConfig(void);
int s_PrnTempAdc(int status);

void s_SPIConfig(void);
void spi_send_data(unsigned char *buf, int len);

void s_PrnGPIOFree(void);

void printer_idle_suspend(void);
void printer_idle_resume(void);

#endif /* _PRINTER_HAL_H_ */

