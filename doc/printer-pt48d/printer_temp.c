
#include "../adc/adc.h"
#include "printer_temp.h"

#include "menu/menu.h"

#define prt_dbg			//uart_print
#define SC_Return(_x_)	do { if(_x_){sc_dbg("<line:%d> %s <Err:%d>\r\n", __LINE__, __FUNCTION__, _x_);} return (_x_); } while(0)

#define PRT_TEMP_ADC_CHANNEL		( ADC_CHANNEL_1 )
#define PRT_TEMP_ADC_SAMPLE_CNT		( 10 )

static unsigned int gTempAdcValTab[PRT_TEMP_ADC_SAMPLE_CNT];
static unsigned int gTempAdcValCounter;

typedef struct
{
    short Temperature;
    short ADValue;
}ADConvT;

static ADConvT k_ADConvTListPrtPT48d[]={	
//{-40, 1015},	
//{-35, 1012},
//{-30, 1009},
//{-25, 1004},
{-20, 998},
{-15, 991}, 
{-10, 982}, 
{-5, 970},
{0, 956},
{5, 939},
{10, 918}, 
{15, 894}, 
{20, 866}, 
{25, 834},
{26, 828},
{27, 822},
{28, 816},
{29, 808},
{30, 799},
{31, 790},
{32, 782},
{33, 774},
{34, 768},
{35, 760},
{36, 753},
{37, 743},
{38, 734},
{39, 725},
{40, 717},
{41, 709},
{42, 700},
{43, 694},
{44, 684},
{45, 674},
{46, 664},
{47, 654},
{48, 644},
{49, 634},
{50, 628},
{51, 618},
{52, 608},
{53, 599},
{54, 590},
{55, 580},
{56, 571},
{57, 562},
{58, 550},
{59, 542},
{60, 533},
{61, 522},
{62, 513},
{63, 504},
{64, 496},
{65, 488},
{66, 478},
{67, 468},
{68, 458},
{69, 448},
{70, 444},
{75, 402},
{80, 363},
};


static void prtAdcTempReadCallback(unsigned int val)
{
	gTempAdcValTab[gTempAdcValCounter%PRT_TEMP_ADC_SAMPLE_CNT] = val;
	gTempAdcValCounter++;
}

void prtAdcTempStart(void)
{
	adcReadCallbackReg(PRT_TEMP_ADC_CHANNEL, prtAdcTempReadCallback);
	adcStart(PRT_TEMP_ADC_CHANNEL);
}

void prtAdcTempStop(void)
{
	adcStop(PRT_TEMP_ADC_CHANNEL);
	gTempAdcValCounter = 0;
}

// 获取当前的加热头温度(摄氏度)
int prtGetCurTemperature(void)
{
	unsigned int sample_max = 0;
	unsigned int sample_min = ~0;
	unsigned int cnt, sample, sample_sum = 0;
	int i;

	adcStop(PRT_TEMP_ADC_CHANNEL);
	prt_dbg("gTempAdcValCounter: %d\r\n", gTempAdcValCounter);
	cnt = gTempAdcValCounter > PRT_TEMP_ADC_SAMPLE_CNT ? PRT_TEMP_ADC_SAMPLE_CNT : gTempAdcValCounter;
	for(i = 0; i < cnt; i++)
	{
		sample = gTempAdcValTab[i];
		if(sample < sample_min)
			sample_min = sample;
		if(sample > sample_max)
			sample_max = sample;
		sample_sum += sample;
	}
	adcStart(PRT_TEMP_ADC_CHANNEL);

	if(cnt < 3) // unlikely
	{
		// 采样数据不够，那就当它是25度吧~~~~(>_<)~~~~
		prt_dbg("what the fck!!!!\r\n");
		return 25;
	}
		
	sample_sum -= ( sample_min + sample_max );
	cnt        -= 2;
	prt_dbg("sample_sum: %d, cnt: %d\r\n", sample_sum, cnt);
	sample      = sample_sum / cnt;
	prt_dbg("sample raw: %d\r\n", sample);
	//sample = sample * 3300 / 1024;
	//prt_dbg("sample volt: %d\r\n", sample);
	
	// TODO: need help!!! sample --> temperature
	for(i = 0; i < sizeof(k_ADConvTListPrtPT48d)/sizeof(k_ADConvTListPrtPT48d[0]); i++)
	{
		if(sample > k_ADConvTListPrtPT48d[i].ADValue)
			break ;
	}
	if( i >= sizeof(k_ADConvTListPrtPT48d)/sizeof(k_ADConvTListPrtPT48d[0]) )
		sample = k_ADConvTListPrtPT48d[i-1].Temperature;
	else
		sample = k_ADConvTListPrtPT48d[i].Temperature;
	
	return sample;
}

// for test
void prtTempAdcStart_test(void)
{
	prtAdcTempStart();
	uart_print("prtAdcTempStart ok!!\r\n");
}

void prtTempAdcStop_test(void)
{
	prtAdcTempStop();
	uart_print("prtAdcTempStop ok!!\r\n");
}

void prtGetCurTemperature_test(void)
{
	unsigned int volt;

	volt = prtGetCurTemperature();
	
	uart_print("prtGetCurTemperature, temp: %d\r\n", volt);
}

void prt_temp_test(void)
{
	ST_MENU_OPT tMenuOpt[] = {
		" prt_volt_test ", NULL,
		"prtTempAdcStart_test", prtTempAdcStart_test,
		"prtTempAdcStop_test", prtTempAdcStop_test,
		"prtGetCurTemperature_test", prtGetCurTemperature_test,
 	};

	xxx_MenuSelect(tMenuOpt, sizeof(tMenuOpt)/sizeof(tMenuOpt[0]));
}


