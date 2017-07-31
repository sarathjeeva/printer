
#include "../adc/adc.h"
#include "printer_volt.h"
#include "menu/menu.h"

#define prt_dbg			//uart_print
#define PRT_Return(_x_)	do { if(_x_){prt_dbg("<line:%d> %s <Err:%d>\r\n", __LINE__, __FUNCTION__, _x_);} return (_x_); } while(0)

// 分压电阻，上面接入电压的一个
#define PRT_VOLT_RA					( 1000 )
// 分压电阻，下面接入地的一个
#define PRT_VOLT_RB					( 470 )
// 选择哪一路ADC
#define PRT_VOLT_ADC_CHANNEL		( ADC_CHANNEL_0 )
// 计算当前电压的历史采样个数
#define PRT_VOLT_ADC_SAMPLE_CNT		( 10 )

static unsigned int gVoltAdcValTab[PRT_VOLT_ADC_SAMPLE_CNT];
static unsigned int gVoltAdcValCounter;

static void prtAdcVoltReadCallback(unsigned int val)
{
	gVoltAdcValTab[gVoltAdcValCounter%PRT_VOLT_ADC_SAMPLE_CNT] = val;
	gVoltAdcValCounter++;
}

void prtAdcVoltStart(void)
{
	adcReadCallbackReg(PRT_VOLT_ADC_CHANNEL, prtAdcVoltReadCallback);
	adcStart(PRT_VOLT_ADC_CHANNEL);
}

void prtAdcVoltStop(void)
{
	adcStop(PRT_VOLT_ADC_CHANNEL);
	gVoltAdcValCounter = 0;
}

// 获取当前的打印电压(毫伏)
unsigned int prtGetCurVoltage(void)
{
	unsigned int sample_max = 0;
	unsigned int sample_min = ~0;
	unsigned int cnt, sample, sample_sum = 0;
	int i;

	adcStop(PRT_VOLT_ADC_CHANNEL);
	prt_dbg("gVoltAdcValCounter: %d\r\n", gVoltAdcValCounter);
	cnt = gVoltAdcValCounter > PRT_VOLT_ADC_SAMPLE_CNT ? PRT_VOLT_ADC_SAMPLE_CNT : gVoltAdcValCounter;
	for(i = 0; i < cnt; i++)
	{
		sample = gVoltAdcValTab[i];
		
		if(sample < sample_min)
			sample_min = sample;
		if(sample > sample_max)
			sample_max = sample;
		sample_sum += sample;
	}
	adcStart(PRT_VOLT_ADC_CHANNEL);

	if(cnt < 3) // unlikely
	{
		// 采样数据不够，就当它是7500mV吧，~~~~(>_<)~~~~
		prt_dbg("what the fck!!!!\r\n");
		return 7500;
	}
		
	sample_sum -= ( sample_min + sample_max );
	cnt -= 2;
	prt_dbg("sample_sum: %d, cnt: %d\r\n", sample_sum, cnt);
	sample = sample_sum / cnt;
	prt_dbg("sample raw: %d\r\n", sample);
	sample = sample * 3300 / 1024;
	prt_dbg("sample volt: %d\r\n", sample);
	sample = sample * ( PRT_VOLT_RA + PRT_VOLT_RB ) / PRT_VOLT_RB;
	prt_dbg("sample complete: %d\r\n", sample);
	
	return sample;
}


// for test
void prtVoltAdcStart_test(void)
{
	prtAdcVoltStart();

	uart_print("prtAdcVoltStart ok!!\r\n");
}

void prtVoltAdcStop_test(void)
{
	prtAdcVoltStop();
	
	uart_print("prtAdcVoltStop ok!!\r\n");
}

void prtGetCurVoltage_test(void)
{
	unsigned int volt;

	volt = prtGetCurVoltage();

	uart_print("prtGetCurVoltage volt: %d mV\r\n", volt);
}

void prt_volt_test(void)
{
	ST_MENU_OPT tMenuOpt[] = {
		" prt_volt_test ", NULL,
		"prtVoltAdcStart_test", prtVoltAdcStart_test,
		"prtVoltAdcStop_test", prtVoltAdcStop_test,
		"prtGetCurVoltage_test", prtGetCurVoltage_test,
 	};

	xxx_MenuSelect(tMenuOpt, sizeof(tMenuOpt)/sizeof(tMenuOpt[0]));
}




