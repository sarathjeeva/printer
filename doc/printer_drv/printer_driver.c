#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/broadcom/timer.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <mach/misc.h>
#include <asm/uaccess.h>

#include "printer_hal.h"
#include "printer_driver.h"

//#define DEBUG_TIME 
#ifdef DEBUG_TIME
#include "ustimer.h"

unsigned int test_heat[10000], test_motor[10000];
unsigned int heati = 0, heatj = 0, motori = 0, motorj = 0;
unsigned long long htime1 = 0, htime2 = 0, mtime1 = 0, mtime2 = 0;
unsigned int heat_flag = 1, motor_flag = 1;
#endif

int STB_PWM_CTL = 0;
extern char *env_mach;
extern char *env_pn;
extern char *env_hw;
extern struct mutex spi3_lock;
/* used to protect the printer when debugging */
#define STB_OPEN

void make_dot_tbl(void);
int PrnTemperature(void);

struct bcm5892_timer *heat_timer = NULL;
struct bcm5892_timer *motor_timer = NULL;

/*for motor_timer_hander response time*/
struct timeval motor_tv1,motor_tv2;
unsigned int motor_handler_flag = 1;
unsigned int motor_response_time;

#define PRESTEP_NUM		26
#define MAX_FEEDPAPER_STEP	(MAX_DOT_LINE * 2 + 1000)
#define MODE_SLOW_START		1
#define MODE_SLOW_STOP		2

/* maximum speed */
static unsigned short HEAT_TIME;
static unsigned char k_AdjTime, k_stepmode;
/* pointer for speed acceleration */
static const unsigned short *k_PreStepTime;
static unsigned short k_maxstep;

/* new struct for temperature check */
typedef struct {
	short PreADCValue;
	short NowADCValue;
	short NowTemperature;
	short ConvertFlag;
} TemperatureUint;

static TemperatureUint HeadTemperature = { 0, 0, 0, 0, };

/* struct for temperature and ADC value */
typedef struct {
	short Temperature;
	short ADValue;
} ADConvT;

static const ADConvT *k_ADConvTList;
static unsigned short k_ADConvNum;

static unsigned short k_StepTime;	/* slow time for motor speed step */

/* for printer status */
volatile unsigned char k_PrnStatus;		/* print status */
volatile unsigned char k_heating;		/* heating flag */
volatile unsigned char k_PaperLack;		/* lack paper flag */
static volatile unsigned int k_StepCount;	/* step flag */

/* maximum STB times of one line */
#define MAX_HEAT_BLOCK		(384 / BLACK_HEADER_DOT)

/* struct for print */
typedef struct {
	short DataBlock;		/* how many block was waiting */
	short Temperature;		/* the temperature of printer */
	unsigned int StepTime;		/* the step time for this time */
	/* store the heat time for each STB */
	unsigned char SendDot[MAX_HEAT_BLOCK][48];
	/* store the total heat time */
	unsigned short DotHeatTime[MAX_HEAT_BLOCK];
	unsigned char NxtLiFstDot[48];	/* the next line heat time */
} HeatUnit;

static HeatUnit k_HeatUnit[2];
/* one for heat and one for calculate */
static HeatUnit *uhptr, *chptr;
static char k_StepGo;
static unsigned int k_HeatTime;		/* the slow time of heat */
static unsigned short k_BlockTime;	/* the slow time of one block */
static unsigned short k_HeatBlock;	/* the current heating block */
static unsigned int k_LastStepTime;	/* last step time */
/* the buf between bitmap and HeatUnit */
static unsigned char k_next_SendDot[MAX_HEAT_BLOCK][48], k_next_block;
/* for motor control */
volatile int k_StepPaper, k_feedcounter;
volatile int k_BeginDot, k_BeginDotLine;
volatile int k_MotorStep, k_StepIntCnt;

/* for dot processing */
volatile int k_CurDotLine, k_CurPrnLine;	/* for print control */
unsigned char k_DotBuf[MAX_DOT_LINE + 1][48];	/* printer buf (bit map) */

#ifdef AUTO_ADJ_DOT
static unsigned short k_LastDotNum;
#endif

/* temperature conversion for PRT printer */
static const ADConvT k_ADConvTListPRT[] = {
	{-20, 1002}, {-18, 999}, {-16, 996}, {-14, 993}, {-12, 989},
	{-10, 985}, {-8, 981}, {-6, 976}, {-4, 971}, {-2, 965},
	{0, 959}, {2, 953}, {4, 946}, {6, 937}, {8, 929},
	{10, 920}, {12, 911}, {14, 900}, {16, 891}, {18, 878},
	{20, 867}, {22, 855}, {24, 841}, {26, 828}, {28, 813},
	{30, 798}, {32, 784}, {34, 768}, {36, 751}, {38, 735},
	{40, 717}, {42, 699}, {44, 681}, {46, 664}, {48, 645},
	{50, 626}, {52, 608}, {54, 590}, {56, 572}, {58, 554},
	{60, 535}, {62, 517}, {64, 499}, {66, 482}, {68, 464},
	{70, 447}, {72, 430}, {74, 414}, {76, 398}, {78, 382},
	{80, 367},
};

/* temperature conversion for JG printer */
static const ADConvT k_ADConvTListLTPJ245G[] = {
	{-20, 991}, {-18, 987}, {-16, 983}, {-14, 978}, {-12, 973},
	{-10, 967}, {-8, 961}, {-6, 955}, {-4, 949}, {-2, 940},
	{0, 930}, {2, 920}, {4, 912}, {6, 901}, {8, 889},
	{10, 878}, {12, 866}, {14, 852}, {16, 838}, {18, 824},
	{20, 808}, {22, 792}, {24, 775}, {26, 757}, {28, 730},
	{30, 723}, {32, 703}, {34, 686}, {36, 668}, {38, 648},
	{40, 628}, {42, 608}, {44, 588}, {46, 568}, {48, 548},
	{50, 528}, {52, 508}, {54, 491}, {56, 471}, {58, 452},
	{60, 435}, {62, 417}, {64, 400}, {66, 382}, {68, 364},
	{70, 349}, {72, 335}, {74, 323}, {76, 303}, {78, 295},
	{80, 281},
};

/* accelerometer for setp motor */
static const unsigned short k_PreStepTimePRT[] = {
	5000, 4875, 3013, 2327, 1953, 1712, 1541, 1412, 1310, 1227, 1158,
	1099, 1048, 1004, 964, 929, 898, 869, 843, 819, 798, 777, 758, 741,
	725, 709, 694,
};

/* config the corresponding printer */
static void Thermal_TypeSel(unsigned char tp)
{
#if 0
	/* config the temperature meter */
	k_ADConvTList = k_ADConvTListPRT;
	k_ADConvNum = sizeof(k_ADConvTListPRT) / sizeof(k_ADConvTListPRT[0]);
#endif
	k_ADConvTList = k_ADConvTListLTPJ245G;
	k_ADConvNum = sizeof(k_ADConvTListLTPJ245G) /
	    sizeof(k_ADConvTListLTPJ245G[0]);
	/* config the accelerometer */
	k_PreStepTime = k_PreStepTimePRT;
	k_maxstep = sizeof(k_PreStepTimePRT) / sizeof(k_PreStepTimePRT[0]);
	k_StepTime = 900;/*1200*/
	HEAT_TIME = 400;/*600*/
	k_AdjTime = 96;
	k_stepmode = MODE_SLOW_START;		/* speed mode */

	for (; k_maxstep > 1; k_maxstep--) {
		if (k_PreStepTime[k_maxstep - 1] >= k_StepTime)
			break;
	}
}

/* read printer type */
static unsigned char read_prn_type(void)
{
	return TYPE_PRN_LTPJ245G;
}

/* check the paper; return PRN_PAPEROUT if no paper, PRN_OK if OK */
static int s_CheckPaper(void)
{
	if (s_PrnHavePaper())
		return PRN_OK;
	return PRN_PAPEROUT;
}

/* latch the data into printer */
static void s_LatchData(unsigned char *buf)
{
	s_PrnLAT(MODE_OFF);

	spi_send_data(buf, 48);

	ndelay(140);			/* >min (120ns) */
	s_PrnLAT(MODE_ON);
	ndelay(140);			/* >min (120ns) */
	s_PrnLAT(MODE_OFF);
}

/* power control */
static void s_PrnPower(unsigned char mode)
{
	if (mode == MODE_ON) {
		/* open the logic power first */
		s_PrnLAT(MODE_OFF);
		s_PrnPowerLogic(MODE_ON);
		msleep(1);
		s_PrnPowerDriver(MODE_ON);
	}
	else {
		/* close the driver power first */
		s_PrnPowerDriver(MODE_OFF);
		mdelay(1);
		s_PrnPowerLogic(MODE_OFF);
		s_PrnLAT(MODE_ON);
	}
}

/* step motor control */
static void s_MotorStep(void)
{

	k_MotorStep += k_StepGo;
	k_MotorStep &= 7;
/*	printk(KERN_INFO"s_MotorStep:k_StepGo = %d,k_MotorStep = %d\n",k_StepGo,k_MotorStep);
*/
	switch (k_MotorStep) {
#ifdef ENABLE_MOTOR_SUBDIVISION
	case 0:
		s_PrnMotorPhase(PHASE_A_H);
		/* s_PrnMotorPhase(PHASE_NA_L); */
		s_PrnMotorPhase(PHASE_B_H);
		/* s_PrnMotorPhase(PHASE_NB_L); */
		break;
	case 1:
		s_PrnMotorPhase(PHASE_A_L);
		/* s_PrnMotorPhase(PHASE_NA_L); */
		s_PrnMotorPhase(PHASE_B_H);
		/* s_PrnMotorPhase(PHASE_NB_L); */
		break;
	case 2:
		/* s_PrnMotorPhase(PHASE_A_L); */
		s_PrnMotorPhase(PHASE_NA_H);
		s_PrnMotorPhase(PHASE_B_H);
		/* s_PrnMotorPhase(PHASE_NB_L); */
		break;
	case 3:
		/* s_PrnMotorPhase(PHASE_A_L); */
		s_PrnMotorPhase(PHASE_NA_H);
		s_PrnMotorPhase(PHASE_B_L);
		/* s_PrnMotorPhase(PHASE_NB_L); */
		break;
	case 4:
		/* s_PrnMotorPhase(PHASE_A_L); */
		s_PrnMotorPhase(PHASE_NA_H);
		/* s_PrnMotorPhase(PHASE_B_L); */
		s_PrnMotorPhase(PHASE_NB_H);
		break;
	case 5:
		/* s_PrnMotorPhase(PHASE_A_L); */
		s_PrnMotorPhase(PHASE_NA_L);
		/* s_PrnMotorPhase(PHASE_B_L); */
		s_PrnMotorPhase(PHASE_NB_H);
		break;
	case 6:
		s_PrnMotorPhase(PHASE_A_H);
		/* s_PrnMotorPhase(PHASE_NA_L); */
		/* s_PrnMotorPhase(PHASE_B_L); */
		s_PrnMotorPhase(PHASE_NB_H);
		break;
	case 7:
		s_PrnMotorPhase(PHASE_A_H);
		/* s_PrnMotorPhase(PHASE_NA_L); */
		/* s_PrnMotorPhase(PHASE_B_L); */
		s_PrnMotorPhase(PHASE_NB_L);
#else
	case 0:
		s_PrnMotorPhase(PHASE_A_L);
		s_PrnMotorPhase(PHASE_NA_L);
		s_PrnMotorPhase(PHASE_B_H);
		s_PrnMotorPhase(PHASE_NB_L);
		break;
	case 1:
		s_PrnMotorPhase(PHASE_A_L);
		s_PrnMotorPhase(PHASE_NA_H);
		s_PrnMotorPhase(PHASE_B_H);
		s_PrnMotorPhase(PHASE_NB_L);
		break;
	case 2:
		s_PrnMotorPhase(PHASE_A_L);
		s_PrnMotorPhase(PHASE_NA_H);
		s_PrnMotorPhase(PHASE_B_L);
		s_PrnMotorPhase(PHASE_NB_L);
		break;
	case 3:
		s_PrnMotorPhase(PHASE_A_L);
		s_PrnMotorPhase(PHASE_NA_H);
		s_PrnMotorPhase(PHASE_B_L);
		s_PrnMotorPhase(PHASE_NB_H);
		break;
	case 4:
		s_PrnMotorPhase(PHASE_A_L);
		s_PrnMotorPhase(PHASE_NA_L);
		s_PrnMotorPhase(PHASE_B_L);
		s_PrnMotorPhase(PHASE_NB_H);
		break;
	case 5:
		s_PrnMotorPhase(PHASE_A_H);
		s_PrnMotorPhase(PHASE_NA_L);
		s_PrnMotorPhase(PHASE_B_L);
		s_PrnMotorPhase(PHASE_NB_H);
		break;
	case 6:
		s_PrnMotorPhase(PHASE_A_H);
		s_PrnMotorPhase(PHASE_NA_L);
		s_PrnMotorPhase(PHASE_B_L);
		s_PrnMotorPhase(PHASE_NB_L);
		break;
	case 7:
		s_PrnMotorPhase(PHASE_A_H);
		s_PrnMotorPhase(PHASE_NA_L);
		s_PrnMotorPhase(PHASE_B_H);
		s_PrnMotorPhase(PHASE_NB_L);
#endif
		break;
	}
}

/* printer init */
void s_PrnInit(void)
{
	if (((!strcmp("s800", env_mach)) && (!strcmp("03-01", env_hw)))
		|| ((!strcmp("s900", env_mach)) && (!strcmp("02-01", env_hw)))){
		printk(KERN_INFO "env_mach: %s,env_hw: %s\n",env_mach,env_hw);
		STB_PWM_CTL = 0;
	} else {
		printk(KERN_INFO "env_mach: %s,env_hw: %s\n",env_mach,env_hw);
		STB_PWM_CTL = 1;
	}
	/* config the Latch and STB control port */
	s_PrnSTBLATConfig();
	s_PrnSTB(MODE_OFF);
	s_PrnLAT(MODE_OFF);

	/* config the step motor phase control port */
	s_PrnMotorPhaseConfig();

	/* config the power control port */
	s_PrnPowerConfig();
	s_PrnPower(MODE_OFF);

	/* config the check paper port */
	s_PrnIoCheckConfig();

	/* config ADC */
	s_PrnADCConfig();

#ifdef DEBUG_TIME
	ustimer_init(600 * 1000 * 1000);
#endif

	/* config spi */
	s_SPIConfig();

	/* init var */
	k_MotorStep = 0;
	k_heating = 0;
	k_StepCount = 0;
	k_BeginDot = 0;
	k_BeginDotLine = 0;
	k_PaperLack = 0;
#ifdef AUTO_ADJ_DOT
	k_LastDotNum = 0;
#endif

	/* confign the corresponding printer */
	Thermal_TypeSel(read_prn_type());

	/* set the default gray */
	PrnSetGray(1);

	make_dot_tbl();
	k_StepGo = 1;
}

/* clear the printer heat buf, when stop print it will be used */
static void s_clear_latch_data(void)
{
	unsigned char EmptyBuf[48];

	memset(EmptyBuf, 0, sizeof(EmptyBuf));	/* fill 0 to priner */
	s_LatchData(EmptyBuf);
}

/* stop print */
static void s_StopPrn(unsigned char status)
{
	/* stop timer */
	bcm5892_stop_timer(heat_timer);
	bcm5892_stop_timer(motor_timer);
	s_PrnLAT(MODE_OFF);
	s_PrnSTB(MODE_OFF);
	s_PrnPower(MODE_OFF);
	s_clear_latch_data();

	k_heating = 0;
	k_StepPaper = 0;
	k_PrnStatus = status;

	/* release timer */
	bcm5892_del_timer(heat_timer);
	bcm5892_del_timer(motor_timer);
}

/***** get the printer temperature when printing
           input parameter:
                    0: start convert
                    1: get the ADC value and start convert again
                    2: get the ADC value and compare to the last ADC value

                 0xff: get the temperature directly
                other: get the ADC valure
******/
static int s_ReadHeadTemp(int step)
{
	int i, nValue, nPreValue, diff;

	switch (step) {
#if defined(LACK_PAPER_CHECK_IO) || defined(LACK_PAPER_CHECK_ADC_NOSCAN)
	case 0:
		s_PrnTempAdc(1);	/* start convert */
		break;

	case 1:
		HeadTemperature.PreADCValue = s_PrnTempAdc(0);	/* read ADC */
		s_PrnTempAdc(1);	/* start convert again */
		break;

	case 2:
		HeadTemperature.NowADCValue = s_PrnTempAdc(0);
		diff = (HeadTemperature.NowADCValue >
		    HeadTemperature.PreADCValue) ? (HeadTemperature.
		    NowADCValue -
		    HeadTemperature.PreADCValue) : (HeadTemperature.
		    PreADCValue - HeadTemperature.NowADCValue);
		if (diff > 8) {
			HeadTemperature.PreADCValue = \
			    HeadTemperature.NowADCValue;
			s_PrnTempAdc(1);
			/* third convert flag */
			HeadTemperature.ConvertFlag = 1;
		}
		else {
			/* convert to temperature */
			for (i = 0; i < k_ADConvNum - 1; i++) {
				if (HeadTemperature.NowADCValue >
				    k_ADConvTList[i].ADValue)
					break;
			}

			HeadTemperature.NowTemperature =
			    k_ADConvTList[i].Temperature;

			/* don't need to third convert */
			HeadTemperature.ConvertFlag = 0;
#if 0
			printk("temperature = %d\n",
			    k_ADConvTList[i].Temperature);
#endif
			return (HeadTemperature.NowTemperature);
		}
		break;

	case 3:
		if (HeadTemperature.ConvertFlag == 1) {
			HeadTemperature.ConvertFlag = 0;
			HeadTemperature.NowADCValue = s_PrnTempAdc(0);

			/* convert to temperature */
			for (i = 0; i < k_ADConvNum - 1; i++) {
				if (HeadTemperature.NowADCValue >
				    k_ADConvTList[i].ADValue)
					break;
			}

			HeadTemperature.NowTemperature =
			    k_ADConvTList[i].Temperature;
#if 0
			printk("temperature = %d\n",
			    k_ADConvTList[i].Temperature);
#endif
			return (HeadTemperature.NowTemperature);
		}
		break;
#endif

#ifdef LACK_PAPER_CHECK_ADC_SCAN
	case 1:
		s_PrnTempAdc(1);
		/* read ADC first time */
		HeadTemperature.PreADCValue = s_PrnTempAdc(0);
		break;

	case 2:
		s_PrnTempAdc(1);
		HeadTemperature.NowADCValue = s_PrnTempAdc(0);
		diff = (HeadTemperature.NowADCValue >
		    HeadTemperature.PreADCValue) ?
		    (HeadTemperature.NowADCValue -
		    HeadTemperature.PreADCValue) :
		    (HeadTemperature.PreADCValue -
		    HeadTemperature.NowADCValue);
		if (diff > 4) {
			HeadTemperature.PreADCValue =
			    HeadTemperature.NowADCValue;
			/* third convert flag */
			HeadTemperature.ConvertFlag = 1;
		}
		else {
			/* convert to temperature */
			for (i = 0; i < k_ADConvNum - 1; i++) {
				if (HeadTemperature.NowADCValue >
				    k_ADConvTList[i].ADValue)
					break;
			}

			HeadTemperature.NowTemperature =
			    k_ADConvTList[i].Temperature;
			/* don't need to third convert */
			HeadTemperature.ConvertFlag = 0;

			return (HeadTemperature.NowTemperature);
		}
		break;

	case 3:
		if (HeadTemperature.ConvertFlag == 1) {
			HeadTemperature.ConvertFlag = 0;
			s_PrnTempAdc(1);
			HeadTemperature.NowADCValue = s_PrnTempAdc(0);
			/* convert to temperature */
			for (i = 0; i < k_ADConvNum - 1; i++) {
				if (HeadTemperature.NowADCValue >
				    k_ADConvTList[i].ADValue)
					break;
			}

			HeadTemperature.NowTemperature =
			    k_ADConvTList[i].Temperature;

			return (HeadTemperature.NowTemperature);
		}
		break;
#endif

	case 0xff:
		nPreValue = 0x8000;
		for (i = 0; i < 3; i++) {
			nValue = s_PrnTempAdc(2);
			diff = (nValue > nPreValue) ? (nValue - nPreValue) :
			    (nPreValue - nValue);
			if (diff < 8)
				break;
			nPreValue = nValue;
		}

		/* convert to temperature */
		for (i = 0; i < k_ADConvNum - 1; i++) {
			if (nValue > k_ADConvTList[i].ADValue)
				break;
		}

		HeadTemperature.NowTemperature = k_ADConvTList[i].Temperature;
		HeadTemperature.ConvertFlag = 0;	/* return to 0 */

		return (HeadTemperature.NowTemperature);
		break;

	default:
		break;
	}
	return 0;
}

/* get the temperature and calculate the heat time */
static void get_block_time(void)
{
	int temp_in, temp;
	int rsl;

	temp_in = HeadTemperature.NowTemperature;	/* s_ReadHeadTemp(); */
	chptr->Temperature = temp_in;
	if (temp_in > 25) {
		temp = temp_in - 25;
		rsl = k_HeatTime - k_HeatTime * temp / k_AdjTime;
	}
	else {
		temp = 25 - temp_in;
		rsl = k_HeatTime + k_HeatTime * temp / k_AdjTime;
	}
	k_BlockTime = rsl;
}

#define MAX_MOTOR_STEP k_maxstep
static unsigned char s_kdotnum[256];
void make_dot_tbl(void)
{
	int ii, jj, nn;
	unsigned char tch;

	for (ii = 0; ii < 256; ii++) {
		tch = ii;
		for (jj = nn = 0; (jj < 8) && (tch); jj++) {
			if (tch & 1) {
				nn++;
			}
			tch >>= 1;
		}
		s_kdotnum[ii] = nn;
	}
}

static unsigned int get_dot(unsigned char *dot, int len)
{
	unsigned int rsl;

	rsl = 0;
	while (len--) {
		rsl += s_kdotnum[*dot++];
	}
	return rsl;
}

static unsigned short dot_time(unsigned int dotnum)
{
	unsigned int rsl;

	rsl = dotnum / SEL_HEADER_DOT;
	if (dotnum % SEL_HEADER_DOT) {
		rsl++;
	}
	rsl *= (k_BlockTime + HEAD_INT_TIME);
	return rsl;
}

static unsigned short next_step_tbl[100], p_last_step, p_stop_step, p_last_time;

static void create_next_step_tbl(void)
{
	int ii, jj;

	p_last_step = 0;
	get_block_time();
	p_stop_step = MAX_MOTOR_STEP;
	for (ii = 0; ii < MAX_MOTOR_STEP; ii++) {
		if (ii < k_CurDotLine) {
			unsigned int dnum;
			dnum = get_dot(k_DotBuf[ii], 48);
			next_step_tbl[ii] = dot_time(dnum);
		}
		else {
			next_step_tbl[ii] = k_PreStepTime[0];
		}
		for (jj = k_maxstep; jj; jj--) {
			if (k_PreStepTime[jj - 1] > next_step_tbl[ii])
				break;
		}
		jj += ii;
		if (jj < p_stop_step) {
			p_stop_step = (jj);
		}
	}
	p_last_time = k_PreStepTime[p_stop_step];
}

/* calculate the current step time */
static unsigned short get_now_time(unsigned short mt)
{
	int ii, jj, kk, mm;
	unsigned int dnum;
	unsigned short nextlasttime, mintime;

	/* which step to slow down */
	ii = p_last_step;
	next_step_tbl[ii] = mt;
	mm = MAX_MOTOR_STEP;
	for (jj = 0; jj < MAX_MOTOR_STEP; jj++) {
		if (next_step_tbl[ii] >= k_PreStepTime[jj]) {
			kk = jj;
			while (kk) {
				if (next_step_tbl[ii] <= k_PreStepTime[kk])
					break;
				kk--;
			}
			if (kk) {
				if (mm > (kk + jj)) {
					mm = kk + jj;
				}
			}
			else {
				/* stop step number */
				mm = jj;
				break;
			}
		}
		ii++;
		ii %= MAX_MOTOR_STEP;
	}

	if (mm < MAX_MOTOR_STEP) {
		mintime = k_PreStepTime[mm];
	}
	else {
		mintime = k_PreStepTime[MAX_MOTOR_STEP - 1];
	}
	if (mt < mintime) {
		mt = mintime;
	}
	ii = k_CurPrnLine + MAX_MOTOR_STEP - 1;
	if (ii < k_CurDotLine) {
		dnum = get_dot(k_DotBuf[ii], 48);
		nextlasttime = dot_time(dnum);
	}
	else {
		nextlasttime = k_PreStepTime[0];
	}
	next_step_tbl[p_last_step] = nextlasttime;
	p_last_step++;
	p_last_step %= MAX_MOTOR_STEP;

	if (mt < p_last_time) {
		for (ii = 0; ii < MAX_MOTOR_STEP; ii++) {
			if (p_last_time > k_PreStepTime[ii])
				break;
		}
		if (ii >= MAX_MOTOR_STEP) {
			ii--;
		}
		if (mt < k_PreStepTime[ii]) {
			mt = k_PreStepTime[ii];
		}
	}
	p_last_time = mt;
	return mt;
}

static unsigned short get_pretime(unsigned short sn)
{
	unsigned short nn;

	if (k_stepmode == MODE_SLOW_STOP) {
		p_stop_step = k_maxstep;
	}
	else {
		p_stop_step = k_maxstep;
	}

	nn = sn;
	if ((PRESTEP_NUM - sn + p_stop_step) < nn) {
		nn = (PRESTEP_NUM - sn + p_stop_step);
	}
	p_last_time = k_PreStepTime[nn];
	if (p_last_time < k_StepTime) {
		p_last_time = k_StepTime;
	}
	k_LastStepTime = p_last_time;

	return p_last_time;
}

/* calculate the current step time */
static unsigned int get_now_steptime(unsigned int nowtm, unsigned int lasttm)
{
	int ii, tt;
	unsigned int mindt;

	/* return if the current speed is slower than the last */
	if (nowtm >= lasttm)
		return nowtm;
	/* else the speed should be controlled */
	tt = k_maxstep;
	for (ii = 0; ii < tt; ii++) {
		if (k_PreStepTime[ii] < lasttm)	/* find the suitable speed */
			break;
	}

	if (ii < (tt - 1)) {
		mindt = k_PreStepTime[ii];
	}
	else {
		mindt = k_PreStepTime[tt - 1];
	}

	if (nowtm < lasttm && nowtm < mindt) {
		nowtm = mindt;			/* limit the speed */
	}

	if (nowtm < k_StepTime) {
		nowtm = k_StepTime;
	}

	return nowtm;
}

static void s_SetPrnTime(HeatUnit *hdptr)
{
	int ii, allt = 0;
	/* set the heat time to each block */
	for (ii = 0; ii < hdptr->DataBlock; ii++) {
		hdptr->DotHeatTime[ii] = k_BlockTime;
		allt += hdptr->DotHeatTime[ii];
	}

	if (allt) {
		allt += ii * HEAD_INT_TIME;
	}
	hdptr->StepTime = (allt) / 2;
	if (k_stepmode == MODE_SLOW_STOP) {
		/* slow start slow stop */
		hdptr->StepTime = get_now_time(hdptr->StepTime);
	}
	else if (k_stepmode == MODE_SLOW_START) {
		/* slow start */
		hdptr->StepTime = get_now_steptime(hdptr->StepTime,
		    k_LastStepTime);
	}
	else {
		/* don't control the speed */
		ii = PRESTEP_NUM + k_CurPrnLine * 2;
		if (ii < k_maxstep) {
			if (hdptr->StepTime < k_PreStepTime[ii]) {
				hdptr->StepTime = k_PreStepTime[ii];
			}
		}
		if (hdptr->StepTime < k_StepTime) {
			hdptr->StepTime = k_StepTime;
		}
	}
	k_LastStepTime = hdptr->StepTime;
}

#ifdef BLACK_ADJ
static short k_blkln = 0;
#endif

const unsigned char cbtn[] = {
	0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
};

static unsigned short split_block(unsigned char blk[MAX_HEAT_BLOCK][48],
    unsigned char *tPtr)
{
	unsigned short jj, dn, bn;
	unsigned char tch;
	unsigned short max_dot;
#ifdef BIT_SEL
	unsigned short bt;
	unsigned char nch, tb;
#endif
	unsigned short lndot;

#ifdef AUTO_ADJ_DOT
	if (k_LastDotNum > MAX_LINE_DOT) {
		max_dot = SEL_HEADER_DOT;
	}
	else {
		max_dot = 2 * SEL_HEADER_DOT;
	}
	k_LastDotNum = 0;
#else
	max_dot = SEL_HEADER_DOT;
#endif
#ifdef BLACK_ADJ
	if (k_blkln >= MAX_BLACK_LINE) {
		k_blkln = MAX_BLACK_LINE;
		max_dot = BLACK_HEADER_DOT;
	}
	lndot = 0;
#endif
	memset(blk, 0, 48 * MAX_HEAT_BLOCK);
	for (jj = bn = dn = 0; jj < 48; jj++) {
		tch = *tPtr++;
		if (tch == 0)
			continue;
#ifdef AUTO_ADJ_DOT
		k_LastDotNum += s_kdotnum[tch];
#endif
#ifdef BLACK_ADJ
		/* calculate the black dot of one line */
		lndot += s_kdotnum[tch];
#endif

		blk[bn][jj] = tch;
#ifdef BIT_SEL
		bt = jj << 3;
		for (; tch != 0; ) {
			if (tch & 0x80) {
				if (++dn == max_dot) {
					/* accurate control */
					tb = bt & 7;
					if (tb != 7) {
						nch = blk[bn][jj];
						blk[bn][jj] = nch & cbtn[tb];
						blk[bn + 1][jj] =
						    nch & (~cbtn[tb]);
					}
					bn++;
					dn = 0;
				}
			}
			bt++;
			tch = (tch << 1) & 0xff;
		}
#else
		dn += s_kdotnum[tch];
		/* switch to other block if reached the max heat dot num */
		if (dn >= max_dot) {
			bn++;
			dn = 0;
		}
#endif
	}
	if (dn) {
		bn++;
	}
#ifdef BLACK_ADJ
	if (lndot >= 192) {
		k_blkln++;
	}
	else if (lndot >= 128) {
		/* XXX? */
	}
	else if (k_blkln) {
		k_blkln--;
	}
#endif
	return bn;
}

/* prepare the next dot line */
static HeatUnit *s_LoadData(HeatUnit *hdptr)
{
	/* print finished? */
	if (k_CurPrnLine >= k_CurDotLine) {
		memset(hdptr->SendDot[0], 0, sizeof(hdptr->SendDot));
		memset(hdptr->NxtLiFstDot, 0, sizeof(hdptr->NxtLiFstDot));
		hdptr->DataBlock = 0;
		return NULL;
	}
	/* fill the heat unit */
	memcpy(hdptr->SendDot[0], k_next_SendDot, sizeof(hdptr->SendDot));
	hdptr->DataBlock = k_next_block;	/* 此块信息之前已经算好 */
	k_CurPrnLine++;

	if (k_CurPrnLine >= k_CurDotLine) {
		memset(k_next_SendDot, 0, sizeof(k_next_SendDot));
		k_next_block = 0;
	}
	else {
		k_next_block = split_block(k_next_SendDot,
		    k_DotBuf[k_CurPrnLine]);
	}

	memcpy(hdptr->NxtLiFstDot, k_next_SendDot[0],
	    sizeof(hdptr->NxtLiFstDot));
	s_SetPrnTime(hdptr);
	return hdptr;
}

/* get the next step time */
static int get_step_time(int start_step, int stop_step)
{
	int rsl;

	if (start_step > k_maxstep) {
		start_step = k_maxstep;
	}
	if (stop_step > k_maxstep) {
		stop_step = k_maxstep;
	}
	if (start_step > stop_step) {
		stop_step = stop_step;
	}
	rsl = k_PreStepTime[stop_step];
	if (rsl < k_StepTime) {
		rsl = k_StepTime;
	}
	return rsl;
}

/* heat timer handler */
static void heat_timer_handler(unsigned long arg)
{
	bcm5892_stop_timer(heat_timer);

#ifdef DEBUG_TIME
	if (heat_flag) {
		htime1 = get_us_clock();
		heat_flag = 0;
	}
	else {
		htime2 = get_us_clock();
		test_heat[heati++] = (unsigned int)(htime2 - htime1);
		htime1 = htime2;
	}
#endif
	k_HeatBlock++;
	if (k_HeatBlock < uhptr->DataBlock) {
		s_LatchData(uhptr->SendDot[k_HeatBlock]);
	}
	else {
		k_HeatBlock = 0;
		/* if current line ended, latch the next dot line data */
		s_LatchData(uhptr->NxtLiFstDot);
		s_PrnSTB(MODE_OFF);
		k_heating = 0;
		return;
	}
#ifdef STB_OPEN
	s_PrnSTB(MODE_OFF);
	udelay(5);
	s_PrnSTB(uhptr->DotHeatTime[k_HeatBlock]);
#endif
	bcm5892_reset_timeout(heat_timer, (uhptr->DotHeatTime[k_HeatBlock]));
}

/* motor timer handler, used for control the motor step time */
static void motor_timer_handler(unsigned long arg)
{

	bcm5892_stop_timer(motor_timer);

#ifdef DEBUG_TIME
	if (motor_flag) {
		mtime1 = get_us_clock();
		motor_flag = 0;
	}
	else {
		mtime2 = get_us_clock();
		test_motor[motori++] = (unsigned int)(mtime2 - mtime1);
		mtime1 = mtime2;
	}
#endif
	if (motor_handler_flag){
		do_gettimeofday(&motor_tv1);
		motor_handler_flag = 0;
	}
	else {
		do_gettimeofday(&motor_tv2);
		motor_response_time = (unsigned int)((1000000 * (motor_tv2.tv_sec - motor_tv1.tv_sec)
			+ (motor_tv2.tv_usec - motor_tv1.tv_usec)));
		motor_tv1.tv_sec = motor_tv2.tv_sec;
		motor_tv1.tv_usec = motor_tv2.tv_usec;
	}

	k_feedcounter++;
#ifdef ENABLE_MOTOR_SUBDIVISION
	if (k_feedcounter >= 2 * MAX_FEEDPAPER_STEP) {
#else
	if (k_feedcounter >= MAX_FEEDPAPER_STEP) {
#endif
		s_StopPrn(PRN_OK);
		return;
	}

	if (k_StepPaper) {
		k_StepPaper--;
		if (k_StepPaper) {
			int sttm;
			s_MotorStep();
#ifdef ENABLE_MOTOR_SUBDIVISION
			sttm = get_step_time((k_feedcounter - 1) / 2,
			    k_StepPaper) / 2;
#else
			sttm = get_step_time(k_feedcounter - 1, k_StepPaper);
#endif
			bcm5892_reset_timeout(motor_timer, sttm);
		}
		else {
			s_StopPrn(0);
		}
		return;
	}
	/* check for lack of paper */
	if (k_StepIntCnt == 0) {
		if (s_PrnHavePaper()) {
			k_PaperLack = 0;
		}
		else {
			/* 10 timers lackpaper */
			k_PaperLack++;
			if (k_PaperLack > 10) {
				s_StopPrn(PRN_PAPEROUT);
				return;
			}
		}
	}
	/* to decide whether is need to feed paper before heating */
#ifdef ENABLE_MOTOR_SUBDIVISION
	if (k_feedcounter < (2 * PRESTEP_NUM)) {
#else
	if (k_feedcounter < (PRESTEP_NUM)) {
#endif
		int gt;
		/* feed paper before heating */
		s_MotorStep();

#ifdef ENABLE_MOTOR_SUBDIVISION
		gt = get_pretime(k_feedcounter / 2) / 2;
		if (k_feedcounter == 2 * PRESTEP_NUM - 1) {
#else
		gt = get_pretime(k_feedcounter);
		/* last step begin to send data */
		if (k_feedcounter == PRESTEP_NUM - 1) {
#endif

			bcm5892_reset_timeout(motor_timer, gt - ADJ_MOTOR_ST2);
			s_LoadData(chptr);
			s_LatchData(chptr->SendDot[0]);
			k_HeatBlock = 0;
		}
		else {
			bcm5892_reset_timeout(motor_timer, gt - ADJ_MOTOR_PRE);
		}
		return;
	}
#if 0
		/* the time is wong if we come here */
		if ((k_heating) && (k_StepIntCnt == 0)) {
		printk(KERN_INFO"PRN_SET_TIME_WRONG\n");
		s_StopPrn(PRN_SET_TIME_WRONG);
		return;
	}
#endif
	switch (k_StepIntCnt) {
		HeatUnit *tp;
	case 0:
		if (k_heating == 1){
			bcm5892_reset_timeout(motor_timer,500);
			break;
		}
		s_PrnSTB(MODE_OFF);			/* stop heating */
		if (chptr == NULL) {
			s_StopPrn(PRN_OK);
			return;
		}
		/* if the printer is too hot stop printing */
		if (chptr->Temperature >= TEMPERATURE_LEVEL2) {
			printk("is too hot....\n");
			s_StopPrn(PRN_OVERHEAT);
			return;
		}
		/* exchange the heat unit and calculate unit */
		tp = chptr;
		chptr = uhptr;
		uhptr = tp;

		if (motor_response_time > uhptr->StepTime){
			k_LastStepTime = 3013;
		}
		/* first step */
		s_MotorStep();
		k_StepIntCnt++;

#ifdef ENABLE_MOTOR_SUBDIVISION
		bcm5892_reset_timeout(motor_timer,
		    (uhptr->StepTime / 2) - ADJ_MOTOR_ST1);
#else
		bcm5892_reset_timeout(motor_timer,
		    uhptr->StepTime - ADJ_MOTOR_ST1);
#endif
		k_HeatBlock = 0;
		k_heating = 0;
		if (uhptr->DataBlock) {
			k_heating = 1;
#ifdef STB_OPEN
			s_PrnSTB(uhptr->DotHeatTime[0]);		/* begin to STB */
#endif
			bcm5892_reset_timeout(heat_timer,
			    (uhptr->DotHeatTime[0]));
		}
		else {
			/* send the next line data */
			s_LatchData(uhptr->NxtLiFstDot);
		}

/* whether the heat time depends on the temperature */
#ifdef TEMPERATURE_ADJ
		get_block_time();
#else
		chptr->Temperature = HeadTemperature.NowTemperature;
#endif

#if defined(LACK_PAPER_CHECK_IO) || defined(LACK_PAPER_CHECK_ADC_NOSCAN)
		s_ReadHeadTemp(0);
#endif
		break;

		/* the next step */
#ifdef ENABLE_MOTOR_SUBDIVISION
	case 1:
		if (motor_response_time > uhptr->StepTime){
			k_LastStepTime = 3013;
		}
		s_MotorStep();
		bcm5892_reset_timeout(motor_timer,
		    (uhptr->StepTime / 2) - ADJ_MOTOR_ST2);
		s_ReadHeadTemp(k_StepIntCnt);
		k_StepIntCnt++;
		break;
	case 2:
		if (k_heating == 1){
			bcm5892_reset_timeout(motor_timer,500);
			break;
		}
#if 1
		s_PrnSTB(MODE_OFF);			


		if (uhptr->Temperature >= TEMPERATURE_LEVEL2) {
			printk("is too hot....\n");
			s_StopPrn(PRN_OVERHEAT);
			return;
		}
#endif		
		if (motor_response_time > uhptr->StepTime){
			k_LastStepTime = 3013;
		}
		s_MotorStep();
		bcm5892_reset_timeout(motor_timer,
		    (uhptr->StepTime / 2) - ADJ_MOTOR_ST2);
		s_ReadHeadTemp(k_StepIntCnt);
		k_StepIntCnt++;
#if 1	
		k_HeatBlock = 0;
		if ( uhptr->DataBlock) {
			k_heating = 1;
			s_LatchData(uhptr->SendDot[0]);
#ifdef STB_OPEN
			s_PrnSTB(uhptr->DotHeatTime[0]);	
#endif
			bcm5892_reset_timeout(heat_timer,
			    (uhptr->DotHeatTime[0]));
		}
#endif
		break;
#endif
		/* the last step */
	default:
		if (motor_response_time > uhptr->StepTime){
			k_LastStepTime = 3013;
		}
		s_MotorStep();
#ifdef ENABLE_MOTOR_SUBDIVISION
		bcm5892_reset_timeout(motor_timer,
		    (uhptr->StepTime / 2) - ADJ_MOTOR_ST2);
#else
		bcm5892_reset_timeout(motor_timer,
		    uhptr->StepTime - ADJ_MOTOR_ST2);
#endif
		chptr = s_LoadData(chptr);
		s_ReadHeadTemp(k_StepIntCnt);
		k_StepIntCnt = 0;
		break;
	}
}
static int heat_timer_init(void)
{
	struct bcm5892_timer_param timer_param;
	int data = 10;

	timer_param.timeout = 500;
	timer_param.handler = heat_timer_handler;
	timer_param.data = (unsigned long)data;

	heat_timer = bcm5892_get_timer(&timer_param);
	if (heat_timer != NULL) {
#if 0
		printk(KERN_INFO "get timer success...\n");
#endif
		return 0;
	}
	else {
		printk("Not enough timers\n");
		return -1;
	}
}

static int motor_timer_init(void)
{
	struct bcm5892_timer_param timer_param;
	int data = 10;

	timer_param.timeout = 500;
	timer_param.handler = motor_timer_handler;
	timer_param.data = (unsigned long)data;
	motor_timer = bcm5892_get_timer(&timer_param);
	if (motor_timer != NULL) {
#if 0
		printk(KERN_INFO "get timer success...\n");
#endif
		return 0;
	}
	else {
		printk("Not enough timers\n");
		return -1;
	}
}

static void s_StartMotor(void)
{
	unsigned char EmptyBuf[48];
	int ret = 0;

	/* initinalize heat timer and motor timer */
	ret = heat_timer_init();
	if (ret < 0) {
		printk(KERN_INFO "heat_timer init failed...\n");
		return;
	}

	ret = motor_timer_init();
	if (ret < 0) {
		bcm5892_del_timer(heat_timer);	/* should release heat timer */
		printk(KERN_INFO "motor_timer init failed...\n");
		return;
	}

	k_StepIntCnt = 0;
	k_heating = 0;
	k_feedcounter = 0;
	k_CurPrnLine = 0;
	k_BeginDot = 0;
	k_BeginDotLine = 0;
	k_PaperLack = 0;
	k_LastStepTime = k_StepTime;
	s_SPIConfig();
	s_PrnLAT(MODE_OFF);
	s_PrnSTB(MODE_OFF);
	s_PrnPower(MODE_ON);
	msleep(1);
	memset(k_HeatUnit, 0x00, sizeof(k_HeatUnit));
	chptr = &k_HeatUnit[0];
	uhptr = &k_HeatUnit[1];

	if (k_stepmode == MODE_SLOW_STOP) {
		create_next_step_tbl();
	}
	else {
		get_block_time();
	}

	/* calculate the heat block */
	if (k_CurDotLine) {
		k_next_block = split_block(k_next_SendDot, k_DotBuf[0]);
	}

#ifdef BLACK_ADJ
	k_blkln = 0;
#endif

	/* send 0 to the printer before printing */
	memset(EmptyBuf, 0, sizeof(EmptyBuf));
	s_LatchData(EmptyBuf);

	bcm5892_reset_timeout(motor_timer, k_PreStepTime[0]);
}

/* printer start print API */
unsigned char PrnStart(int line)
{
#ifdef DEBUG_TIME
	int jj;
#endif
	k_CurDotLine = line;
	if (k_CurDotLine == 0)
		return PRN_OK;

	mutex_lock(&spi3_lock);
	bcm5892_adc_get(NULL);

	if (s_PrnVoltageCheck()) {
		printk(KERN_INFO"over voltage.\n");
		bcm5892_adc_put(NULL);
		mutex_unlock(&spi3_lock);
		return PRN_OVERVOLTAGE;
	}
	/* before print we should check the temperature and paper first */
	s_PrnPowerLogic(MODE_ON);
	msleep(5);					/* need delay */

	/* check the temperature */
	HeadTemperature.NowTemperature = s_ReadHeadTemp(0xff);
	if (HeadTemperature.NowTemperature >= TEMPERATURE_LEVEL1) {
		s_PrnPowerLogic(MODE_OFF);
		bcm5892_adc_put(NULL);
		mutex_unlock(&spi3_lock);
		return PRN_OVERHEAT;
	}
	/* check paper */
	if (s_CheckPaper() != 0) {
		int ii;
		ii = 0;
		while (1) {
			msleep(2);			/* need delay */
			if (s_CheckPaper() == 0)
				break;
			if (ii++ > 10) {
				s_PrnPowerLogic(MODE_OFF);
				bcm5892_adc_put(NULL);
				mutex_unlock(&spi3_lock);
				return PRN_PAPEROUT;
			}
		}
	}

	k_PrnStatus = PRN_BUSY;
	k_StepPaper = 0;
	k_StepGo = 1;

	/* start print */
	s_StartMotor();

	/* wait until the print finish */
	while (k_PrnStatus == PRN_BUSY) {
		msleep(20);
	}

	bcm5892_adc_put(NULL);
	mutex_unlock(&spi3_lock);

#ifdef DEBUG_TIME

	if (line > 2400) {
		line = 2400;
	}
	printk("motor test time..........\n");
	for (jj = 0; jj < line * 4 + 100; jj++) {
		if ((jj % 4) == 0) {
			printk("          ");
		}
		if ((jj % 12) == 0) {
			printk("\n");
		}
		printk("%d, ", test_motor[jj]);
	}

	printk("\n\nheat test time.........\n");
	for (jj = 0; jj < heati + 10; jj++) {
		if ((jj % 20) == 0) {
			printk("\n");
		}
		printk("%d, ", test_heat[jj]);
	}
	heati = heatj = motori = motorj = 0;
	heat_flag = motor_flag = 1;
	memset(test_heat, 0, sizeof(test_heat));
	memset(test_motor, 0, sizeof(test_motor));
#endif

	return k_PrnStatus;
}

static int CheckPaper(void)
{
	int rsl, ii;

	for (ii = 0; ii < 3; ii++) {
		rsl = s_CheckPaper();
		if (rsl == 0)
			break;
		msleep(3);
	}
	return rsl;
}

int PrnTemperature(void)
{
	return (s_ReadHeadTemp(0xff));
}

void PrnSetGray(unsigned char Level)
{
	switch (Level) {
	case 1:			/* normal heat time */
		k_HeatTime = HEAT_TIME;
		break;
	case 3:			/* double time for double-bonded paper A */
		k_HeatTime = (HEAT_TIME * 3) / 2;
		break;
	case 4:			/* for double-bonded paper B */
		k_HeatTime = HEAT_TIME * 2;
		break;
	default:
#if 0
		if (Level >= 50 && Level <= 500)
			k_HeatTime = HEAT_TIME * Level / 100;
#endif
		k_HeatTime = HEAT_TIME;
		break;
	}
}

/* get the print data form userspace */
unsigned long GetDotBuf(const char __user *from, int len)
{
	if (copy_from_user(k_DotBuf, from, len))
		return -EFAULT;
	return 0;
}

/* fill the buf */
void FillDotBuf(int offset, int len)
{
	memset(&k_DotBuf[0][0] + offset, 0x00, len);
}

/* check printer status */
unsigned char PrnStatus(void)
{
	volatile int PrnTemp, paperstatus;

	switch (k_PrnStatus) {
	case PRN_BUSY:
		break;
	case PRN_OUTOFMEMORY:
		break;
	default:
		k_PrnStatus = PRN_BUSY;
		s_PrnPowerLogic(MODE_ON);
		msleep(5);
		PrnTemp = s_ReadHeadTemp(0xff);
		paperstatus = CheckPaper();
		s_PrnPower(MODE_OFF);
		if (PrnTemp >= TEMPERATURE_LEVEL1) {
			k_PrnStatus = PRN_OVERHEAT;
		}
		else {
			k_PrnStatus = paperstatus;
		}
		break;
	}

	return k_PrnStatus;
}

