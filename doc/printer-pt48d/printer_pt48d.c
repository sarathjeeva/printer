
#include <mml.h>
#include <mml_gpio.h>
#include <mml_spi.h>

#include "printer_dev.h"
#include "printer_volt.h"
#include "printer_temp.h"

// for test
#include "menu/menu.h"

#define prt_dbg			//uart_print
#define SC_Return(_x_)	do { if(_x_){sc_dbg("<line:%d> %s <Err:%d>\r\n", __LINE__, __FUNCTION__, _x_);} return (_x_); } while(0)

int adcGetVal(int channel);


// GPIO
#define PRT_MTA_DEV_REG_BASE	( MML_GPIO1_IOBASE )
#define PRT_MTA_PORT			( MML_GPIO_DEV1 )
#define PRT_MTA_PIN				( 22 )

#define PRT_nMTA_DEV_REG_BASE	( MML_GPIO1_IOBASE )
#define PRT_nMTA_PORT			( MML_GPIO_DEV1 )
#define PRT_nMTA_PIN			( 23 )

#define PRT_MTB_DEV_REG_BASE	( MML_GPIO1_IOBASE )
#define PRT_MTB_PORT			( MML_GPIO_DEV1 )
#define PRT_MTB_PIN				( 25 )

#define PRT_nMTB_DEV_REG_BASE	( MML_GPIO1_IOBASE )
#define PRT_nMTB_PORT			( MML_GPIO_DEV1 )
#define PRT_nMTB_PIN			( 24 )

#define PRT_LATCH_PORT	( MML_GPIO_DEV0 )
#define PRT_LATCH_PIN	( 30 )

#define PRT_STB_PORT	( MML_GPIO_DEV1 )
#define PRT_STB_PIN		( 8 )

#define PRT_POWER_PORT	( MML_GPIO_DEV1 )
#define PRT_POWER_PIN	( 30 )

#define PRT_VDD_PORT	( MML_GPIO_DEV1 )
#define PRT_VDD_PIN		( 31 )

#define PRT_PAPER_PORT	( MML_GPIO_DEV1 )
#define PRT_PAPER_PIN	( 7 )

// SPI
#define PRT_SPI_DEV		( MML_SPI_DEV1 )

#if 0
#define MTA_H()	mml_gpio_write_bit_pattern(PRT_MTA_PORT, PRT_MTA_PIN, 1, 1)
#define MTA_L()	mml_gpio_write_bit_pattern(PRT_MTA_PORT, PRT_MTA_PIN, 1, 0)
#define nMTA_H()	mml_gpio_write_bit_pattern(PRT_nMTA_PORT, PRT_nMTA_PIN, 1, 1)
#define nMTA_L()	mml_gpio_write_bit_pattern(PRT_nMTA_PORT, PRT_nMTA_PIN, 1, 0)
#define MTB_H()	mml_gpio_write_bit_pattern(PRT_MTB_PORT, PRT_MTB_PIN, 1, 1)
#define MTB_L()	mml_gpio_write_bit_pattern(PRT_MTB_PORT, PRT_MTB_PIN, 1, 0)
#define nMTB_H()	mml_gpio_write_bit_pattern(PRT_nMTB_PORT, PRT_nMTB_PIN, 1, 1)
#define nMTB_L()	mml_gpio_write_bit_pattern(PRT_nMTB_PORT, PRT_nMTB_PIN, 1, 0)
#endif
#define MTA_H()			*(volatile unsigned int *)(PRT_MTA_DEV_REG_BASE+0x18) |= ( 1 << PRT_MTA_PIN )
#define MTA_L()			*(volatile unsigned int *)(PRT_MTA_DEV_REG_BASE+0x18) &= ~( 1 << PRT_MTA_PIN )

#define nMTA_H()		*(volatile unsigned int *)(PRT_nMTA_DEV_REG_BASE+0x18) |= ( 1 << PRT_nMTA_PIN )
#define nMTA_L()		*(volatile unsigned int *)(PRT_nMTA_DEV_REG_BASE+0x18) &= ~( 1 << PRT_nMTA_PIN )

#define MTB_H()			*(volatile unsigned int *)(PRT_MTB_DEV_REG_BASE+0x18) |= ( 1 << PRT_MTB_PIN )
#define MTB_L()			*(volatile unsigned int *)(PRT_MTB_DEV_REG_BASE+0x18) &= ~( 1 << PRT_MTB_PIN )

#define nMTB_H()		*(volatile unsigned int *)(PRT_nMTB_DEV_REG_BASE+0x18) |= ( 1 << PRT_nMTB_PIN )
#define nMTB_L()		*(volatile unsigned int *)(PRT_nMTB_DEV_REG_BASE+0x18) &= ~( 1 << PRT_nMTB_PIN )

#if 0
#define MOTOR_PHASE_1A_HIGH()	MTA_H() //MTB_H()
#define MOTOR_PHASE_1A_LOW()	MTA_L() //MTB_L()

#define MOTOR_PHASE_2A_HIGH()	MTB_H()
#define MOTOR_PHASE_2A_LOW()	MTB_L()

#define MOTOR_PHASE_1B_HIGH()	nMTA_H()
#define MOTOR_PHASE_1B_LOW()	nMTA_L()

#define MOTOR_PHASE_2B_HIGH()	nMTB_H()
#define MOTOR_PHASE_2B_LOW()	nMTB_L()
#endif

#if 0
#define MOTOR_PHASE_1A_HIGH()	MTB_H() //MTB_H()
#define MOTOR_PHASE_1A_LOW()	MTB_L() //MTB_L()

#define MOTOR_PHASE_2A_HIGH()	MTA_H()
#define MOTOR_PHASE_2A_LOW()	MTA_L()

#define MOTOR_PHASE_1B_HIGH()	nMTB_H() //nMTB_H()
#define MOTOR_PHASE_1B_LOW()	nMTB_L() //nMTB_L()

#define MOTOR_PHASE_2B_HIGH()	nMTA_H()
#define MOTOR_PHASE_2B_LOW()	nMTA_L()
#endif

#define PRT_PWR_H()		*(volatile unsigned int *)(MML_GPIO1_IOBASE+0x18) |= ( 1 << 30 ) 
#define PRT_PWR_L()		*(volatile unsigned int *)(MML_GPIO1_IOBASE+0x18) &= ~( 1 << 30 )

#define PRT_VDD_H()		*(volatile unsigned int *)(MML_GPIO1_IOBASE+0x18) |= ( 1 << 31 ) 
#define PRT_VDD_L()		*(volatile unsigned int *)(MML_GPIO1_IOBASE+0x18) &= ~( 1 << 31 )

#define PRT_STB_H()		*(volatile unsigned int *)(MML_GPIO1_IOBASE+0x18) |= ( 1 << 8 ) 
#define PRT_STB_L()		*(volatile unsigned int *)(MML_GPIO1_IOBASE+0x18) &= ~( 1 << 8 )

#define PRT_LATCH_H()	*(volatile unsigned int *)(MML_GPIO0_IOBASE+0x18) |= ( 1 << 30 ) 
#define PRT_LATCH_L()	*(volatile unsigned int *)(MML_GPIO0_IOBASE+0x18) &= ~( 1 << 30 )


#if 1
static short pt48dStepTab[] = {
	2890, 1786, 1381, 1157, 1014, 914, 838, 777, 728,
	 687,  651,  621,  600,  572, 552, 533, 516, 500,
};
#endif

#if 0
static unsigned short pt48dStepTab[] = {
	2614, 2500, 2426, 2272, 2144, 2035, 1941, 1859, 1786, 1721, 1663, 1610, 1561, 1517, 1477,
	1439, 1404, 1372, 1342, 1313, 1287, 1261, 1238, 1215, 1194, 1174, 1155, 1136, 1119, 1102,
	1086, 1071, 1056, 1042, 1029, 1016, 1003,  991,  979,  968,  957,  947,  936,  927,  917,
	 908,  899,  890,  882,  873,  865,  857,  850,  842,  835,  828,  821,  815,  808,  802,  
	 796,  789,  784,  778,  772,  766,  761,  756,  750,  745,  740,  735,  731,  726,  721,  
	 717,  712,  708,  704,  699,  695,  691,  687,  683,  679,  675,  672,  668,  664,  661,
	 657,  654,  651,  647,  644,  641,  637,  634,  631,  628,  625,  622,  619,  616,  614,
	 611,  608,  605,  603,  600,  597,  595,  592,  590,  587,  585,  582,  580,  577,  575,
	 573,  570,  568,  558,  548,  538,  529,  520,  512,  504,  497,  489,  482,  476,  469,
	 463,  457,  452,  446,
};
#endif

#if 0
static short pt48dStepTab[] = {
	3321, 3214, 3102, 2895, 2803, 2762, 2314, 2028,
	1928, 1828, 1728, 1675, 1635, 1595, 1565, 1543, 
	1523, 1503, 1486, 1466, 1446, 1426, 1406, 1384,
	1364, 1344, 1322, 1302, 1282, 1262, 
};
#endif

static volatile unsigned int prt_phase = 0;

// 
static void pt48dConfig(void)
{
	int result;
    mml_gpio_config_t config;
	mml_spi_params_t spi_master = { .baudrate = 4000000,
									.ssel = 0x00,	/* 置1位*/
									.word_size = 8,
									.mode = MML_SPI_MODE_MASTER,
									.wor = MML_SPI_WOR_NOT_OPEN_DRAIN,
									.clk_pol = MML_SPI_SCLK_LOW,
									.phase = MML_SPI_PHASE_LOW,
									.brg_irq = MML_SPI_BRG_IRQ_DISABLE,
									.ssv = MML_SPI_SSV_LOW,
									.ssio = MML_SPI_SSIO_OUTPUT,
									.tlj = MML_SPI_TLJ_DIRECT,
									.dma_tx.active = MML_SPI_DMA_DISABLE,
									.dma_rx.active = MML_SPI_DMA_DISABLE };

	config.gpio_direction     = MML_GPIO_DIR_OUT;
	config.gpio_function      = MML_GPIO_NORMAL_FUNCTION;
	config.gpio_intr_mode     = MML_GPIO_NORMAL_FUNCTION;
	config.gpio_intr_polarity = MML_GPIO_INT_POL_RAISING;
	config.gpio_pad_config    = MML_GPIO_PAD_NORMAL;

	result = mml_gpio_init(PRT_VDD_PORT, PRT_VDD_PIN, 1, config);
	prt_dbg("gpio_init, PRT_VDD_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_POWER_PORT, PRT_POWER_PIN, 1, config);
	prt_dbg("gpio_init, PRT_POWER_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_STB_PORT, PRT_STB_PIN, 1, config);
	prt_dbg("gpio_init, PRT_STB_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_LATCH_PORT, PRT_LATCH_PIN, 1, config);
	prt_dbg("gpio_init, PRT_LATCH_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_MTA_PORT, PRT_MTA_PIN, 1, config);
	prt_dbg("gpio_init, PRT_MTA_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_nMTA_PORT, PRT_nMTA_PIN, 1, config);
	prt_dbg("gpio_init, PRT_nMTA_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_MTB_PORT, PRT_MTB_PIN, 1, config);
	prt_dbg("gpio_init, PRT_MTB_PORT: %d\r\n", result);
	result = mml_gpio_init(PRT_nMTB_PORT, PRT_nMTB_PIN, 1, config);
	prt_dbg("gpio_init, PRT_nMTB_PORT: %d\r\n", result);
	
	config.gpio_direction     = MML_GPIO_DIR_IN;
	config.gpio_function      = MML_GPIO_NORMAL_FUNCTION;
	config.gpio_intr_mode     = MML_GPIO_NORMAL_FUNCTION;
	config.gpio_intr_polarity = MML_GPIO_INT_POL_RAISING;
	config.gpio_pad_config    = MML_GPIO_PAD_NORMAL;
	result = mml_gpio_init(PRT_PAPER_PORT, PRT_PAPER_PIN, 1, config);
	prt_dbg("gpio_init, PRT_PAPER_PORT: %d\r\n", result);
	
	result = mml_spi_init(PRT_SPI_DEV, &spi_master);
	prt_dbg("mml_spi_init, PRT_PAPER_PORT: %d\r\n", result);
	result = mml_spi_ioctl(PRT_SPI_DEV, MML_SPI_IOCTL_ENABLE, NULL);
	prt_dbg("mml_spi_ioctl, MML_SPI_IOCTL_ENABLE: %d\r\n", result);

	PRT_PWR_L();
	PRT_STB_H();
	PRT_LATCH_H();
	PRT_VDD_H();
	MTA_L();
	nMTA_L();
	MTB_L();
	nMTB_L();
}

static void pt48dPowerCtl(int ctrl)
{
	if(ctrl)
	{
		PRT_PWR_H();
	}
	else
	{
		PRT_PWR_L();
	}
}

static void pt48dVddCtl(int ctrl)
{
	if(ctrl)
	{
		PRT_VDD_H();
	}
	else
	{
		PRT_VDD_L();
	}
}

static void pt48LatchCtrl( int ctrl )
{
	if(ctrl)
	{
		PRT_LATCH_H();
	}
	else
	{
		PRT_LATCH_L();
	}

}

/*
A1	A2	B1	B2
0	0	0	1
1	0	0	1
1	0	0	0
1	0	1	0
0	0	1	0
0	1	1	0
0	1	0	0
0	1	0	1
*/
static void pt48dStep(void)
{	
#if 0
	//uart_printk("phase:%d phase&0x07:%d\r\n", prt_phase, prt_phase & 0x07);
	switch (prt_phase & 0x07)
	{
		#if 0
		// 2016.07.11
        case 0:
            MOTOR_PHASE_1A_HIGH();
            MOTOR_PHASE_1B_LOW();
            MOTOR_PHASE_2A_HIGH();
            MOTOR_PHASE_2B_LOW();
        break;
        case 1:
            MOTOR_PHASE_1A_HIGH();
            MOTOR_PHASE_1B_LOW();
            MOTOR_PHASE_2A_LOW();
            MOTOR_PHASE_2B_LOW();
        break;
        case 2:
            MOTOR_PHASE_1A_HIGH();
            MOTOR_PHASE_1B_LOW();
            MOTOR_PHASE_2A_LOW();
            MOTOR_PHASE_2B_HIGH();
        break;
        case 3:
            MOTOR_PHASE_1A_LOW();
            MOTOR_PHASE_1B_LOW();
            MOTOR_PHASE_2A_LOW();
            MOTOR_PHASE_2B_HIGH();
        break;
        case 4:
            MOTOR_PHASE_1A_LOW();
            MOTOR_PHASE_1B_HIGH();
            MOTOR_PHASE_2A_LOW();
            MOTOR_PHASE_2B_HIGH();
        break;
        case 5:
            MOTOR_PHASE_1A_LOW();
            MOTOR_PHASE_1B_HIGH();
            MOTOR_PHASE_2A_LOW();
            MOTOR_PHASE_2B_LOW();
        break;
        case 6:
            MOTOR_PHASE_1A_LOW();
            MOTOR_PHASE_1B_HIGH();
            MOTOR_PHASE_2A_HIGH();
            MOTOR_PHASE_2B_LOW();
        break;
        case 7:
            MOTOR_PHASE_1A_LOW();
            MOTOR_PHASE_1B_LOW();
            MOTOR_PHASE_2A_HIGH();
            MOTOR_PHASE_2B_LOW();
        break;

		#endif
		
		#if 0
		case 7:
			MOTOR_PHASE_1A_HIGH();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_HIGH();
			MOTOR_PHASE_2B_LOW();
		break;
		case 6:
			MOTOR_PHASE_1A_HIGH();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_LOW();
		break;
		case 5:
			MOTOR_PHASE_1A_HIGH();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_HIGH();
		break;
		case 4:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_HIGH();
		break;
		case 3:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_HIGH();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_HIGH();
		break;
		case 2:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_HIGH();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_LOW();
		break;
		case 1:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_HIGH();
			MOTOR_PHASE_2A_HIGH();
			MOTOR_PHASE_2B_LOW();
		break;
		case 0:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_HIGH();
			MOTOR_PHASE_2B_LOW();
		break;
		#endif

		#if 0
		case 0:
			MOTOR_PHASE_1A_HIGH();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_HIGH();
			MOTOR_PHASE_2B_LOW();
		break;
		case 1:
			MOTOR_PHASE_1A_HIGH();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_LOW();
		break;
		case 2:
			MOTOR_PHASE_1A_HIGH();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_HIGH();
		break;
		case 3:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_HIGH();
		break;
		case 4:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_HIGH();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_HIGH();
		break;
		case 5:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_HIGH();
			MOTOR_PHASE_2A_LOW();
			MOTOR_PHASE_2B_LOW();
		break;
		case 6:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_HIGH();
			MOTOR_PHASE_2A_HIGH();
			MOTOR_PHASE_2B_LOW();
		break;
		case 7:
			MOTOR_PHASE_1A_LOW();
			MOTOR_PHASE_1B_LOW();
			MOTOR_PHASE_2A_HIGH();
			MOTOR_PHASE_2B_LOW();
		break;
		#endif
	 }
#endif
	switch(prt_phase&0x07)
	{
	case 0:
		MTA_H();nMTA_L();MTB_L();nMTB_H();
		break;
	case 1:
		MTA_L();nMTA_L();MTB_L();nMTB_H();
		break;
	case 2:
		MTA_L();nMTA_H();MTB_L();nMTB_H();
		break;
	case 3:
		MTA_L();nMTA_H();MTB_L();nMTB_L();
		break;
	case 4:
		MTA_L();nMTA_H();MTB_H();nMTB_L();
		break;
	case 5:
		MTA_L();nMTA_L();MTB_H();nMTB_L();
		break;
	case 6:
		MTA_H();nMTA_L();MTB_H();nMTB_L();
		break;
	case 7:
	//default:
		MTA_H();nMTA_L();MTB_L();nMTB_L();
		break;		
	}

#if 0
	//uart_print("prt_phase:%d, phase&0x07: %d\r\n", prt_phase, prt_phase&0x07);
	switch(prt_phase&0x07)
	{
	case 7:
		MTA_H();nMTA_L();MTB_L();nMTB_L();
		break;
	case 6:
		MTA_H();nMTA_L();MTB_L();nMTB_H();
		break;
	case 5:
		MTA_L();nMTA_L();MTB_L();nMTB_H();
		break;
	case 4:
		MTA_L();nMTA_H();MTB_L();nMTB_H();
		break;
	case 3:
		MTA_L();nMTA_H();MTB_L();nMTB_L();
		break;
	case 2:
		MTA_L();nMTA_H();MTB_H();nMTB_L();
		break;
	case 1:
		MTA_L();nMTA_L();MTB_H();nMTB_L();
		break;
	case 0:
	default:
		MTA_H();nMTA_L();MTB_H();nMTB_L();
		break;		
	}
#endif	
	prt_phase++;
}

// 1: 开始加热；0:停止加热
static void pt48dStbCtl(int ctrl)
{
	if(!ctrl)
	{
		PRT_STB_H();
	}
	else
	{
		PRT_STB_L();
	}
}

static void pt48dMotorPower(int ctrl)	
{
	if(ctrl==0)
	{
		MTA_L();nMTA_L();MTB_L();nMTB_L();
		prt_phase = 0;
	}
}

static void pt48dLatch(void)
{
	int cnt;

	pt48dStbCtl( 0 );
	PRT_LATCH_L();
	for(cnt=0; cnt<5; cnt++)
		;
	PRT_LATCH_H();
	for(cnt=0; cnt<5; cnt++)
		;
}

static int pt48dGetTemprature(void)
{
	// 打印机温度为30度
	//return 30;
	return prtGetCurTemperature();
}

static unsigned int pt48dGetVoltage(void)
{
	// 打印电压为8000mV
	//return 8000;
	return prtGetCurVoltage();
}

#define BASE_HEAT_TIME	550


typedef struct
{
    unsigned short v;
    unsigned short t;
}T_VoltConvTime;

static T_VoltConvTime gVoltConvTimeTab[] = {
{9400, 400},
{9200, 450},
{9000, 480},
{8800, 500},
{8600, 520},
{8400, 540},
{8200, 580},
{8000, 600},
{7800, 640},
{7600, 680},
{7400, 715},
{7200, 760},
{7000, 800},
{6800, 850},
{6600, 900},
{6400, 960},
};
#if 0
const static unsigned short gVoltBank[] = {
8800, 8600, 8400, 8200, 8000, 7850, 7700, 7550, 
7400, 7250, 7100, 6950, 6800, 6650, 6500, 6300,
6100, 5800,
};
const static unsigned char gVoltFactor[] = {
 210,  200,  185,  170,  155,  145,  135,  125, 
 115,  110,  105,  100,   95,   85,   75,   70, 
  70,   60,   55,   50,
};
#endif
const static unsigned char gTempFactor[] = {
	//25
	100,
	//26 - 30
	100, 100, 100, 100, 100,
	//31 - 35
	 95,  94,  93,	92,  91,
	//36 - 40
	 80,  80,  76,	76,  76,
	//41 - 45
	 73,  73,  70,	70,  70,
	//46 - 50
	 67,  67,  67,	67,  67,
	//51 - 55
	 64,  64,  63,	63,  63,	
	//56 - 60
	 61,  61,  59,	59,  59,
	//61 - 65
	 57,  57,  55,	55,  55,	 
	//66 - 70
	 50,  50,  50,	50,  50,
	//71 - 75
	 50,  50,  50,	50,  50,
};

static unsigned int prtCalcHeatTimer(int gray)
{
	int i, cnt;
	int tm;
	int volt, temp;

	volt = pt48dGetVoltage();
	temp = pt48dGetTemprature();

	//prt_dbg_add("Volt", volt);
	//prt_dbg_add("Temp", temp);
#if 0
	for(i=0; i<sizeof(gVoltBank)/sizeof(gVoltBank[0]); i++)
	{
		if(volt > gVoltBank[i])
			break;
	}
	cnt = sizeof(gVoltFactor)/sizeof(gVoltFactor[0]);
	tm = gVoltFactor[cnt-i-1] * BASE_HEAT_TIME / 100;
#endif
	cnt = sizeof(gVoltConvTimeTab) / sizeof(gVoltConvTimeTab[0]);
	for( i = 0; i < cnt; i++ )
	{
		if(volt > gVoltConvTimeTab[i].v )
			break;
	}
	if( i >= cnt )
	{
		i = cnt - 1;
	}
	tm = gVoltConvTimeTab[i].t;

	//prt_dbg_add("TM1", tm);
	if(temp >= (25))
	{
		if(temp <= (70)) // 25~70度的时候，随温度的变高，加热时间递减一点
		{
			tm = (tm * gTempFactor[(temp-(25))]) / 100;
		}
		else
		{
			tm = (tm * 50) / 100;
		}
	}
	else // 低于25度，加热时间递增一点
	{
		temp = 25 - temp;
		tm=tm+tm*temp/62;
	}

	// 根据外面设置的灰度值调整一下
	if( gray >= 50 && gray <= 250) 
	{
		tm = tm * gray / 100;
	}

	//prt_dbg_add("TM2", tm);

	return tm;
}


static unsigned int pt48dGetHeatTime(int gray)
{
	// TODO: Need help!!! How to calculate???
	return prtCalcHeatTimer(gray);
}

static void pt48dLoad(unsigned char *s)
{
	int ret;

	ret = mml_spi_transmit(PRT_SPI_DEV, s, PRT_BYTES_PER_LINE);

	//uart_print("mml_spi_transmit, ret: %d\r\n", ret);
	//uart_print_hex("recv data::\r\n", s, PRT_BYTES_PER_LINE);
}


// 0:缺纸, 1:有纸
static int pt48dCheckPaper(void)
{	
	unsigned int data;
	
	mml_gpio_read_bit_pattern(PRT_PAPER_PORT, PRT_PAPER_PIN, 1, &data);
	
	return data;
}

static void pt48dInit(void)
{
	pt48dPowerCtl(0);
	pt48dVddCtl(1);
	pt48dStbCtl(0);
	pt48dMotorPower(0);
	//prtAdcVoltStart();
	//prtAdcTempStart();
	prt_phase = 0;
}

T_PrinterDriver tDrvPt48d = {
	.init = pt48dInit,
	.config = pt48dConfig,
	.powerControl = pt48dPowerCtl,
	.step = pt48dStep,
	.stbControl = pt48dStbCtl,
	.motorPower = pt48dMotorPower,
	.latch = pt48dLatch,
	.load = pt48dLoad,
	.checkPaper = pt48dCheckPaper,
	.getTemprature = pt48dGetTemprature,
	.getHeatTime = pt48dGetHeatTime,
	.getVoltage = pt48dGetVoltage,
	.damageTemprature = 75,
	.maxTemprature = 68,
	.minVoltage = 3300,
	.maxVoltage = 12500,
	.normalHeatDot = 48,
	.normalAdjHeatDot = 192,
	.blackAdjHeatDot = 96,
	.blackAdjLine = 4,
	.waittime = 0,
	.stepTab = pt48dStepTab,
	.stepTabCnt = sizeof(pt48dStepTab)/sizeof(pt48dStepTab[0]),
	.preStepCnt = 48,
	.minStepTime = 446,	
	.mask = /*MASK_PRT_BLACK_ADJ|MASK_PRT_DOT_ADJ|*/MASK_PRT_HEAT_TWINCE,
};


// ------------------- for test ----------------------
void prt48dMotorStep_test(void)
{
	static int fck_cnt = 0;
	int step_us;

	pt48dStep();

	if(prt_phase >= sizeof(pt48dStepTab)/sizeof(pt48dStepTab[0]))
		step_us = pt48dStepTab[sizeof(pt48dStepTab)/sizeof(pt48dStepTab[0])-1];
	else
		step_us = pt48dStepTab[prt_phase];
	
	prtTimerStart(0, step_us);

	//if(fck_cnt++ > 2000)
	//{
	//	prtTimerStop(0);
	//	fck_cnt = 0;
	//}
}



void pt48dConfig_test(void)
{
	pt48dConfig();
	uart_print("pt48dConfig ok!!\r\n");
}

void pt48dInit_test(void)
{
	pt48dInit();
	uart_print("pt48dInit ok!!\r\n");
}

void pt48dPowerCtl_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dPowerCtl(0/1):", &ctl) < 0)
		return ;

	pt48dPowerCtl(ctl);

	uart_print("pt48dPowerCtl(%d) ok!!\r\n", ctl);
}

void pt48dVddCtl_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dVddCtl(0/1):", &ctl) < 0)
		return ;

	pt48dVddCtl(ctl);

	uart_print("pt48dVddCtl(%d) ok!!\r\n", ctl);
}

void pt48dStbCtl_test(void)
{
	int i, cnt, us;
	int ctl;
	
	if(xxx_input_int("pt48dStbCtl, cnt:", &cnt) < 0)
		return ;

	if(cnt > 100)
		return ;
	
	ctl = 1;
	us = 1500;
	for(i = 0; i < 1; i++)
	{
		//uart_print("heat<%d> : %d\r\n", i, us);
		pt48dStbCtl(ctl);
		DelayUs(us);
		ctl ^= 1;
	}
	
	pt48dStbCtl(0);
	
	uart_print("pt48dStbCtl(%d) ok!!\r\n", cnt);
}

void pt48dStep_test(void)
{
	int i, step;
	int us;
	
	if(xxx_input_int("pt48dStep, steps:", &step) < 0)
		return ;
	
	for(i = 0; i < step; i++)
	{
		
		if(i < sizeof(pt48dStepTab)/sizeof(pt48dStepTab[0]))
			us = pt48dStepTab[i];
		else
			us = pt48dStepTab[sizeof(pt48dStepTab)/sizeof(pt48dStepTab[0])-1];

		
		pt48dStep();

		DelayUs(us);

		//uart_print("pt48dStep(%d): %dus\r\n", i, us);
	}

	pt48dMotorPower(0);
	uart_print("pt48dStep(%d) ok!!\r\n", step);
}

void pt48dMTA_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dMTA(0/1):", &ctl) < 0)
		return ;

	if(ctl)
		MTA_H();
	else
		MTA_L();
	
	uart_print("pt48dMTA(%d)\r\n", ctl);
}
void pt48dnMTA_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dnMTA(0/1):", &ctl) < 0)
		return ;
	
	if(ctl)
		nMTA_H();
	else
		nMTA_L();
	
	uart_print("pt48dnMTA(%d)\r\n", ctl);
}
void pt48dMTB_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dMTB(0/1):", &ctl) < 0)
		return ;
	
	if(ctl)
		MTB_H();
	else
		MTB_L();
	
	uart_print("pt48dMTB(%d)\r\n", ctl);
}
void pt48dnMTB_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dnMTB(0/1):", &ctl) < 0)
		return ;
	
	if(ctl)
		nMTB_H();
	else
		nMTB_L();
	
	uart_print("pt48dnMTB(%d)\r\n", ctl);
}

void pt48dMotorPower_test(void)
{
	int ctl;
	if(xxx_input_int("pt48dMotorPower(0/1):", &ctl) < 0)
		return ;

	pt48dMotorPower(ctl);

	uart_print("pt48dPowerCtl(%d)\r\n", ctl);
}

void pt48dLatch_test(void)
{
	pt48dLatch();
	uart_print("pt48dLatch ok!!\r\n");
}

void pt48dLoad_test(void)
{
	unsigned char data[48];

	memset(data, 0x01, sizeof(data));

	uart_print("pt48dLoad ...\r\n");
	pt48dLoad(data);
	uart_print("pt48dLoad ok!!\r\n");
}

void pt48dLoadAndLatch_test(void)
{
	int i;
	unsigned char data[48];

	memset(data, 0x00, sizeof(data));

	uart_print("pt48dLoad ...\r\n");
	pt48dLatch();
	pt48dLoad(data);
	uart_print("pt48dLoad ok!!\r\n");
	pt48dLatch();
	uart_print("pt48dLatch ok!!\r\n");
	
	DelayMs(10);
	
	//memset(data, 0xff, sizeof(data));
	for(i = 0; i < 8; i++)
		data[i] = 0xFF;
	
	memset(data, 0xf0, sizeof(data));
	
	uart_print("pt48dLoad ...\r\n");
	pt48dLatch();
	pt48dLoad(data);
	uart_print("pt48dLoad ok!!\r\n");
	pt48dLatch();
	uart_print("pt48dLatch ok!!\r\n");

}

void pt48dGetHeatTime_test(void)
{
	int temp;

	temp = pt48dGetHeatTime(0);

	uart_print("pt48dGetHeatTime, temp: %d\r\n", temp);
}

void pt48dGetTemprature_test(void)
{
	int temp;

	temp = pt48dGetTemprature();

	uart_print("pt48dGetTemprature, temp: %d\r\n", temp);
}

void pt48dGetVoltage_test(void)
{
	int volt;

	volt = pt48dGetVoltage();

	uart_print("pt48dGetVoltage, volt: %d\r\n", volt);
}

void pt48dCheckPaper_test(void)
{
	int ret;

	ret = pt48dCheckPaper();

	uart_print("pt48dCheckPaper, ret: %d\r\n", ret);
}

void pt48d_io_test(void)
{	
	ST_MENU_OPT tMenuOpt[] = {
		" pt48d_io_test ", NULL,
		"pt48dConfig_test", pt48dConfig_test,
		"pt48dInit_test", pt48dInit_test,
		"pt48dVddCtl_test", pt48dVddCtl_test,
		"pt48dPowerCtl_test", pt48dPowerCtl_test,

		"pt48dLoadAndLatch_test", pt48dLoadAndLatch_test,
		"pt48dStbCtl_test", pt48dStbCtl_test,
		
		
		"pt48dStep_test", pt48dStep_test,

		"pt48dGetTemprature_test", pt48dGetTemprature_test,
		"pt48dCheckPaper_test", pt48dCheckPaper_test,
		"pt48dGetVoltage_test", pt48dGetVoltage_test,
		
		"pt48dMTA_test", pt48dMTA_test,
		"pt48dnMTA_test", pt48dnMTA_test,
		"pt48dMTB_test", pt48dMTB_test,
		"pt48dnMTB_test", pt48dnMTB_test,
		
		//"pt48dMotorPower_test", pt48dMotorPower_test,
		//"pt48dLatch_test",pt48dLatch_test,
		//"pt48dLoad_test",pt48dLoad_test,
	};

	xxx_MenuSelect(tMenuOpt, sizeof(tMenuOpt)/sizeof(tMenuOpt[0]));
}


