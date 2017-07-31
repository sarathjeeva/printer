#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <linux/spi/spi.h> 

#include <linux/delay.h>
#include <linux/time.h>
#include <linux/hrtimer.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
#include <linux/mx51_pax_io.h>
#include "prn_drv.h"
#include "prn_io.h"

//#define DEBUG_MOTOR
//#define DEBUG_US

#define STEP_MOTOR_HEIGHT	2
#ifdef ENABLE_MOTOR_SUBDIVISION    
#define PRN_NULL_LINES 1
#else
#define PRN_NULL_LINES 0
#endif

#define DEF_HEAT_TIME 1000  /* us 2.5ms/lines */

static const unsigned char s_kdotnum[256]=
{
0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
};

//#define K_ADJTIME_TEMP 46  // must >= 46
//#define K_ADJTIME_TEMP 62  // must >= 46
#define K_ADJTIME_TEMP 68  // must >= 46

static char		k_StepGo;
static int         k_MotorStep;

//static unsigned char  k_AdjTime;
static unsigned int     k_HeatTime;         // 定义加热标准时间
static unsigned short 	    k_BlockTime;		//定义单个块加热标准时间


//For dot processing
static  int            k_feedcounter;
static int            k_CurDotLine,k_CurPrnLine;              // 打印缓冲区行控制变量
static unsigned char    k_DotBuf[MAX_DOT_LINE+2][PRN_LBYTES];           // 打印缓冲区
static  unsigned char   k_next_SendDot[MAX_HEAT_BLOCK+1][PRN_LBYTES];
static unsigned short  k_next_block; //下一行的点

static int s_STB, hr_s_Motor;

#undef DEBUG_HEAT
//#define DEBUG_HEAT
#ifdef DEBUG_HEAT
	#define FLAG_0SEG(x)	(x)			
	#define FLAG_1SEG(x)	(x<<4)			
	#define FLAG_2SEG(x)	(x<<8)				
	#define FLAG_3SEG(x)	(x<<12)				
	#define FLAG_4SEG(x)	(x<<16)				
	#define FLAG_5SEG(x)	(x<<20)				
	#define FLAG_6SEG(x)	(x<<24)					
	#define FLAG_7SEG(x)	(x<<28)		
#define DEBUG_HEAT_LINE 1024
static unsigned int heat_times;
static unsigned long long heat_time_start;
static unsigned long heating[DEBUG_HEAT_LINE];
static unsigned short heating_flag[DEBUG_HEAT_LINE];
#endif

#undef DEBUG_FLAG
//#define DEBUG_FLAG
#ifdef DEBUG_FLAG
	#define FLAG_0SEG(x)	(x)			
	#define FLAG_1SEG(x)	(x<<4)			
	#define FLAG_2SEG(x)	(x<<8)				
	#define FLAG_3SEG(x)	(x<<12)				
	#define FLAG_4SEG(x)	(x<<16)				
	#define FLAG_5SEG(x)	(x<<20)				
	#define FLAG_6SEG(x)	(x<<24)					
	#define FLAG_7SEG(x)	(x<<28)		
static unsigned long FLAG[MAX_DOT_LINE+2];
#endif
//handle
extern int DelayMs(int Ms);
extern int pmic_prt_temp(void);
extern int pmic_prt_vol(void);

/*--------------------- Programm -----------------------------------*/

static inline int have_paper(void)
{
#ifdef PRN_IO_REQUEST
    return gpio_get_value(pprn->prnIO.inp_PRT_POUT)?0:1;  //high level interrupt
#else
    int gpio = get_pax_gpio_num(PIN_PRT_POUT);
    gpio_request(gpio, PIN_PRT_POUT);
    return gpio_get_value(gpio)?0:1;  //high level interrupt
#endif    
}

int  get_status_paper(void){ return have_paper();}


 int get_status_temperature(void)
{
#ifdef PRN_IO_REQUEST
    return gpio_get_value(pprn->prnIO.inp_PRT_TEMP)?1:0;  //high level interrupt
#else
    int gpio = get_pax_gpio_num(PIN_PRT_TEMP);
    gpio_request(gpio, PIN_PRT_TEMP);
    return gpio_get_value(gpio)?1:0;  //high level interrupt
#endif
}



void Thermal_TypeSel(unsigned char tp)
{
//		k_AdjTime=62;
}



// check the paper ,return PRN_PAPEROUT = no paper, PRN_OK = ok
int s_CheckPaper(void)
{
	if(have_paper())
	{
		return PRN_OK;
	}
	return PRN_PAPEROUT;
}
// Latch信号,低有效
void s_PrnLatch(unsigned char status)
{
    if(status == LOW)
    {
        enable_LAT();
    }
    else
    {
        disable_LAT();
    }
}

static inline void s_PrnSTB(unsigned char status)
{
#ifdef DEBUG_HEAT
//	{
				/* Follow the token with the time */
//				unsigned long flags;				
				int this_cpu;				
				unsigned long long t;
//				unsigned long nanosec_rem;

//				raw_local_irq_save(flags);
//	}
#endif
	
    if(status == ON)
    {
#if 0 //def DEBUG_PRN
	printk("STB on at:%lx \n", jiffies);
#endif

#ifdef DEBUG_HEAT
	if(heat_times<DEBUG_HEAT_LINE)
	{
				this_cpu = smp_processor_id();
//				raw_local_irq_restore(flags);

				t = cpu_clock(this_cpu);
//				nanosec_rem = do_div(t, 1000000000);

		heat_time_start=t;	
//		heat_times++;
	}		
#endif

        enable_STB();
    }
    else
    {
#if 0 //def DEBUG_PRN
	printk("STB off at:%lx \n", jiffies);
#endif
        disable_STB();
#ifdef DEBUG_HEAT
	if(heat_times<DEBUG_HEAT_LINE)
	{
	 if(heat_time_start==0){
	// 	printk("@heatdebug_closed!@\n");
		return;
	 }

	 				this_cpu = smp_processor_id();
//				raw_local_irq_restore(flags);

				t = cpu_clock(this_cpu);
//				nanosec_rem = do_div(t, 1000000000);

		t=t-heat_time_start;
		 heating[heat_times]=do_div(t, 1000000000);
		 heat_times++;
		 heat_time_start=0;
		heating_flag[heat_times]=0;
	}
#endif
    }
}

void s_PrnMTDEF(unsigned char status)
{
    if(status == ON)
    {
#if 0 //def DEBUG_PRN
	printk("MTDEF on at:%lx \n", jiffies);
#endif
        set_MTDEF();
    }
    else
    {
#if 0 //def DEBUG_PRN
	printk("MTDEF off at:%lx \n", jiffies);
#endif    
    clr_MTDEF();   // sleep pull up 
    }
}
/*
static inline int enque_heat_latch_data(unsigned char *buf)
{
	s_LatchData(buf);
	return 0;
}
*/
static inline int enque_motor_latch_data(unsigned char *buf)
{
	s_LatchData(buf);
	return 0;
}

static inline void Latch_data(void)
{
	s_PrnLatch(LOW);   //LOW
    udelay(2);
 	s_PrnLatch(HIGH); // HIGH  
  }

// 电源开关控制
void s_PrnPower(unsigned char status)
{
//     unsigned int i = 0;
    
    if(status == ON)
    {
//    	disable_LAT();
        enable_pwr_33();
        enable_pwr_t();
//        enable_pwr_5v();
    }
    else
    {
        disable_pwr_t();
    //    disable_pwr_5v();
    //    for (i = 0; i < 0x1000;i++);
        disable_pwr_33();
//        disable_LAT();
    }
}
#ifdef DEBUG_PRN	
int wl_debug_steps = 0;
#endif


//hupw:20120912 add for motor control       //////////////////////////////////////
static inline void __DRV8833_motorStep8_inc(int phase)
{
//	clr_MTDEF();   // sleep pull down 

  switch(phase & 0x07)
    {
        case 7:  //STEP1
		//	MOTOR_1A_HIGH();
		//	MOTOR_2A_LOW();
			MOTOR_1B_LOW();
		//	MOTOR_2B_LOW();
        break;
        case 6:  //STEP2
      		MOTOR_1A_HIGH();
		//	MOTOR_2A_LOW();
		//	MOTOR_1B_HIGH();
		//	MOTOR_2B_LOW();
        break;
        case 5: //STEP3
      	//	MOTOR_1A_LOW();
			MOTOR_2A_LOW();
		//	MOTOR_1B_HIGH();
		//	MOTOR_2B_LOW();
        break;
        case 4: //STEP4
      	//	MOTOR_1A_LOW();
		//	MOTOR_2A_HIGH();
			MOTOR_1B_HIGH();
		//	MOTOR_2B_LOW();
        break;
           case 3:  //STEP5
      	//	MOTOR_1A_LOW();
		//	MOTOR_2A_HIGH();
		//	MOTOR_1B_LOW();
			MOTOR_2B_LOW();
        break;
        case 2:  //STEP6
      	//	MOTOR_1A_LOW();
			MOTOR_2A_HIGH();
		//	MOTOR_1B_LOW();
		//	MOTOR_2B_HIGH();
        break;
        case 1: //STEP7
      		MOTOR_1A_LOW();
		//	MOTOR_2A_LOW();
		//	MOTOR_1B_LOW();
		//	MOTOR_2B_HIGH();
        break;
        case 0: //STEP8
 //     		MOTOR_1A_HIGH();
//			MOTOR_2A_LOW();
//			MOTOR_1B_LOW();
			MOTOR_2B_HIGH();
        break;     
    }
 //   set_MTDEF();   // sleep pull up 
}

static inline void __DRV8833_motorStep8_dec(int phase)
{
//	clr_MTDEF();   // sleep pull down 

  switch(phase & 0x07)
    {
        case 7:  //STEP1
		//	MOTOR_1A_HIGH();
		//	MOTOR_2A_LOW();
		//	MOTOR_1B_LOW();
			MOTOR_2B_LOW();
        break;
        case 6:  //STEP2
      	//	MOTOR_1A_HIGH();
		//	MOTOR_2A_LOW();
			MOTOR_1B_HIGH();
		//	MOTOR_2B_LOW();
        break;
        case 5: //STEP3
      		MOTOR_1A_LOW();
		//	MOTOR_2A_LOW();
		//	MOTOR_1B_HIGH();
		//	MOTOR_2B_LOW();
        break;
        case 4: //STEP4
      	//	MOTOR_1A_LOW();
			MOTOR_2A_HIGH();
		//	MOTOR_1B_HIGH();
		//	MOTOR_2B_LOW();
        break;
           case 3:  //STEP5
      	//	MOTOR_1A_LOW();
		//	MOTOR_2A_HIGH();
			MOTOR_1B_LOW();
		//	MOTOR_2B_LOW();
        break;
        case 2:  //STEP6
      	//	MOTOR_1A_LOW();
		//	MOTOR_2A_HIGH();
		//	MOTOR_1B_LOW();
			MOTOR_2B_HIGH();
        break;
        case 1: //STEP7
      	//	MOTOR_1A_LOW();
			MOTOR_2A_LOW();
		//	MOTOR_1B_LOW();
		//	MOTOR_2B_HIGH();
        break;
        case 0: //STEP8
      		MOTOR_1A_HIGH();
//			MOTOR_2A_LOW();
//			MOTOR_1B_LOW();
//			MOTOR_2B_HIGH();
        break;     
    }
 //   set_MTDEF();   // sleep pull up 
}
static void __DRV8833_motorStep4(int phase)
{
//	clr_MTDEF();   // sleep pull down 

  switch(phase & 0x03)
    {
        case 0:  //STEP1
          MOTOR_1A_HIGH();
			MOTOR_2A_LOW();
			MOTOR_1B_HIGH();
			MOTOR_2B_LOW();
        break;
        case 1:  //STEP2
           MOTOR_1A_HIGH();
			MOTOR_2A_LOW();
			MOTOR_1B_LOW();
			MOTOR_2B_HIGH();
        break;
        case 2: //STEP3
           MOTOR_1A_LOW();
			MOTOR_2A_HIGH();
			MOTOR_1B_LOW();
			MOTOR_2B_HIGH();
        break;
        case 3: //STEP4
         	MOTOR_1A_LOW();
			MOTOR_2A_HIGH();
			MOTOR_1B_HIGH();
			MOTOR_2B_LOW();   
        break;
    }

 //   set_MTDEF();   // sleep pull up 
}

/////////////////////////////////////////////////////
static void inline s_MotorStep(unsigned char status)
{
    if(status)
    {
#if 0 //def DEBUG_PRN	
	printk("motor %d\n", wl_debug_steps++);
#endif

		k_MotorStep+=k_StepGo;

#ifdef ENABLE_MOTOR_SUBDIVISION    
      k_MotorStep&=7;

	if(k_StepGo>0)
      	__DRV8833_motorStep8_inc(k_MotorStep);
	else
		__DRV8833_motorStep8_dec(k_MotorStep);
#else
        k_MotorStep&=3;
        __DRV8833_motorStep4(k_MotorStep);
#endif
    }
    else
    {
#ifdef DEBUG_PRN	
		 wl_debug_steps=0;
#endif    
		s_PrnMTDEF(ON);
		
/*
	step the motor to 0
*/
#ifdef ENABLE_MOTOR_SUBDIVISION    
		k_MotorStep=0;
		{
     		MOTOR_1A_HIGH();
			MOTOR_2A_LOW();
			MOTOR_1B_LOW();
			MOTOR_2B_HIGH();
		}
#endif			
    }
}
/*---------------------------------------------------*/
static struct hrtimer prn_stb_timer, prn_motor_timer;
static spinlock_t prn_stb_lock, prn_motor_lock;

static int stb_get_time(void)
{
//   if (hrtimer_active(&prn_stb_timer))
	if( hrtimer_is_queued(&prn_stb_timer))
   {
      ktime_t r = hrtimer_get_remaining(&prn_stb_timer);
      return (r.tv.nsec+999) / 1000;
   }
   else{
	 	//printk("un_stb\n");
      return 0;
   }
}

static int motor_get_time(void)
{
  // if (hrtimer_active(&prn_motor_timer))
  	if( hrtimer_is_queued(&prn_motor_timer))
   {
     ktime_t r = hrtimer_get_remaining(&prn_motor_timer);
      return (r.tv.nsec+999) / 1000;
   }
   else{
	 	//printk("un_motor\n");	 	
      return 0;
   }
}


static void motor_enable_us(int value)
{
   unsigned long flags;
	
   spin_lock_irqsave(&prn_motor_lock, flags);
   hrtimer_cancel(&prn_motor_timer);
	 
  	if(value>0)
  	{
   		value = (value > 20000 ? 20000 : value);
   		hrtimer_start(&prn_motor_timer, ktime_set(0, (value) * 1000), HRTIMER_MODE_REL);
  		if (!hrtimer_active(&prn_motor_timer))
  		{
  			printk("Can't active motor timer!\n");
  		}
			
	 	s_MotorStep(1);	  			
		hr_s_Motor=ON;			
  	}
	else
		hr_s_Motor=OFF;			
		
   spin_unlock_irqrestore(&prn_motor_lock, flags);
}


static void stb_enable_us(int value)
{
   unsigned long flags;

   spin_lock_irqsave(&prn_stb_lock, flags);
   hrtimer_cancel(&prn_stb_timer);

   if (value <= 0)
   {
		s_PrnSTB(OFF);			
		s_STB=OFF;
   }
   else
   {
   
      value = (value > 5000 ? 5000 : value);
      hrtimer_start(&prn_stb_timer, ktime_set(0, (value) * 1000), HRTIMER_MODE_REL);
  		if (!hrtimer_active(&prn_stb_timer))
  		{
  			printk("Can't active stb timer!\n");
  		}
			
		s_PrnSTB(ON);					
		s_STB=ON;					
   }

   spin_unlock_irqrestore(&prn_stb_lock, flags);
}

static enum hrtimer_restart prn_motor_func(struct hrtimer *timer)
{	
	if(hr_s_Motor==ON)							
		hr_s_Motor=OFF;

#ifdef DEBUG_HEAT
		heating_flag[heat_times]|=FLAG_0SEG(2);
#endif			
	
   return HRTIMER_NORESTART;
}

static enum hrtimer_restart prn_stb_func(struct hrtimer *timer)
{	
	if(s_STB==ON){		
		s_PrnSTB(OFF);			
		s_STB=OFF;
	}	
#ifdef DEBUG_HEAT
		heating_flag[heat_times]|=FLAG_0SEG(1);
#endif			
   return HRTIMER_NORESTART;
}

void  _init_prn_timer(void)
{
   hrtimer_init(&prn_stb_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   prn_stb_timer.function = prn_stb_func;
   hrtimer_init(&prn_motor_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   prn_motor_timer.function = prn_motor_func;	 
}

//==============================================================================
static unsigned char __CheckStatus(void)
{
	int ii;
	unsigned char status=STATUS_IDLE;

	if(s_CheckPaper() == PRN_PAPEROUT)	
	{
		ii=0;
		while(1)
		{
			DelayMs(2);
			if(s_CheckPaper()!=PRN_PAPEROUT)
			{
				break;
			}
			if(ii++>10)
			{
#ifdef DEBUG_PRN					
				printk("No paper!\n");
#endif
				status=STATUS_SUSPEND_NOPAPER;
				goto ret_CheckStatus;
			}
		}
	}

    if(s_ReadHeadTemp() >= LIMIT_TEMP)      
    {
#ifdef DEBUG_PRN			
		printk("Too heat!\n");		
#endif
		status=STATUS_SUSPEND_HOT;
		goto ret_CheckStatus;
    }

	ii=pmic_prt_vol();
	if(ii>=LIMIT_RESTART_VOL)
			pprn->low_level_retry=0;
	
	if(ii<=LIMIT_VOL||	pprn->low_level_retry>=LOW_LEVEL_RETRY)
	{
#ifdef DEBUG_PRN			
		printk("Voltage low level:%d\n", ii);		
#endif
		status=STATUS_SUSPEND_LOWLEVEL;
	}

ret_CheckStatus:
	return status;
}

void s_PrnInit(void)
{
    // 配置Strobe信号
    s_PrnSTB(OFF);
	
	//数据送出关闭
	s_PrnLatch(HIGH);

	//打印机相位配置
	cfg_STEP();
//	s_MotorStep(0);
	s_PrnMTDEF(OFF);

	//关打印机电源
	cfg_PWR();
    s_PrnPower(OFF);

	//配置缺纸检测
	cfg_CHKPAPER();

	//SPI配置
	//cfg_spi();
	//低功耗口配置
	cfg_idle_port();
	
    AdcInit();
    
/////////////////////////////////////////////////
	pprn->low_level_retry=0;

    //Init var
    k_MotorStep    = 0;
	k_StepGo=1;	
    k_feedcounter   = 0;

	enable_pwr_33();
	DelayMs(10);
	PrnStatus(__CheckStatus());
	disable_pwr_33();
		
    Thermal_TypeSel(TYPE_PRN_PRT);
}

static void PrnStop(unsigned char status)
{
    //Stop timer
	s_PrnMTDEF(OFF);
    s_PrnLatch(HIGH);
    s_PrnSTB(OFF);
    s_PrnPower(OFF);
	extern void pmic_stop_printing();
	pmic_stop_printing();

}

static void s_StartMotor(void)
{    
	//disable_LAT();
	s_PrnLatch(HIGH);
	s_PrnSTB(OFF);
    s_PrnPower(ON);
}



static unsigned char PrnStart(void)
{
	unsigned char status;
/* 加入检测是否有纸 */

	extern void pmic_start_printing();
	pmic_start_printing();

	enable_pwr_33();
	DelayMs(10);
	status=__CheckStatus();
	if(status!=STATUS_IDLE){
		disable_pwr_33();
		return status;
	}
		
    // 启动打印
#ifdef DEBUG_PRN	    
    printk("Start motor\n");
#endif
    s_StartMotor();
   
    return STATUS_BUSY;
}

int s_ReadHeadTemp(void)
{
	return pmic_prt_temp();
}

int s_ReadVoltage(void)
{
	return pmic_prt_vol();
}
/*
	根据上一次打印时检测的温度，
	调整下一次的加热时间
	返回值: ms
*/
static inline unsigned short get_block_time(void)
{
	int temp_in,temp;
	int rsl;
#if 0
//	temp_in=s_ReadHeadTemp();
//	if(pprn)
//		pprn->temperature =temp_in;
	
	rsl=k_HeatTime;
/*	
	if(temp_in > 25)
	{
		temp = temp_in - 25;
		rsl=k_HeatTime-k_HeatTime*temp/k_AdjTime;
	}
	else
	{
		temp = 25 - temp_in;
		rsl=k_HeatTime+k_HeatTime*temp/k_AdjTime;
	}
*/	
	k_BlockTime=rsl;	
#else
	temp_in=pprn->temperature;
	
	rsl=k_HeatTime;

	if(temp_in > 25)
	{
		temp = temp_in - 25;
		rsl=k_HeatTime-k_HeatTime*temp/K_ADJTIME_TEMP;
	}
	else
	{
		temp = 25 - temp_in;
		rsl=k_HeatTime+k_HeatTime*temp/K_ADJTIME_TEMP;
	}

	k_BlockTime=rsl;	
#endif

	return rsl;
}


/**
*Description:
*
*   Split one print line to many heat line.
*	the number of heat line is calculated with MAX_HEAT_DOTS(div)
*		
*Paramter: 
*
*	blk: the heat lines' data
*	tPtr: the print line data
*
*return:
*	the number of heat lines
*
*notice:
*  hupw : 20121204 add following feature
*	1. if the temperate is large than 60C , use the the max div to heat
*	2. if the voltage is less then 3.5v, use the max div to heat			
*/
//static unsigned char SplitBits8[]={	0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

static unsigned short linePixels(unsigned char *tPtr)
{
	int jj;
	unsigned short dots=0;
	for(jj=0;jj<PRN_LBYTES;jj++){
			dots=dots+(unsigned short)s_kdotnum[*tPtr];			
			tPtr++;
	}
	
	return dots;
}

static unsigned short split_block8(unsigned char *blk, unsigned char *tPtr)
{
	int jj;
	unsigned char tch;

	//memset(&blk[0],0,8*PRN_LBYTES);				
	for(jj=0;jj<PRN_LBYTES;jj++)
	{
		tch=*tPtr;
		tPtr++;
		blk[jj]=tch&0x02;
		blk[jj+PRN_LBYTES]=tch&0x20;
		blk[jj+2*PRN_LBYTES]=tch&0x08;
		blk[jj+3*PRN_LBYTES]=tch&0x80;
		blk[jj+4*PRN_LBYTES]=tch&0x01;
		blk[jj+5*PRN_LBYTES]=tch&0x10;
		blk[jj+6*PRN_LBYTES]=tch&0x04;
		blk[jj+7*PRN_LBYTES]=tch&0x40;		
	}
		
	return 8;		
}

//static unsigned char SplitBits8[]={0x80,0x40, 0x20,0x10,0x08,0x04,0x02,0x01};
static unsigned char SplitBits4[]={0x22,0x88, 0x11,0x44};
static unsigned short split_block4(unsigned char *blk, unsigned char *tPtr0, int div)
{
	unsigned char tch;
	unsigned short  bn;
	unsigned short dots;
	unsigned char *tPtr;
	
	int ii,jj, max_dot;

	dots=bn=0;
	
	max_dot=MAX_HEAT_DOTS(div);
	
	memset(&blk[0],0,PRN_LBYTES);

	for(ii=0;ii<4;ii++){		
		tPtr=tPtr0;
		for(jj=0;jj<PRN_LBYTES;jj++)
		{
			tch=*tPtr;
			tPtr++;
			tch=(tch&SplitBits4[ii]);
			if(tch==0) continue;
						
			blk[bn*PRN_LBYTES+jj]|=tch;
		
			dots=dots+(unsigned short)s_kdotnum[tch];
			if(dots>=max_dot)
			{
				bn++;	
				if(bn>MAX_HEAT_BLOCK){
					printk("Split4 err:div(%d) max(%d) %d, %d\n",div,max_dot, ii,jj);
					return (MAX_HEAT_BLOCK);
				}
				dots=0;
				memset(&blk[bn*PRN_LBYTES],0,PRN_LBYTES);			
			}			
		}
	}	

	if(dots) bn++;	
	return bn;		
}


static unsigned short split_block(unsigned char *blk,unsigned char *tPtr)
{
	int div;

	div = pprn->block_div;
	if(pprn->temperature>=LIMIT_TEMP)
	{
		printk("Temperature limited!\n");
	//	div=MAX_SPLIT_DIV;
	}
	else if(pprn->low_level_retry>=LOW_LEVEL_RETRY)
	{
		printk("Voltage limited!\n");
		div=MAX_SPLIT_DIV;
	}

	if(div==3){
		int jjj;

		jjj=linePixels(tPtr);
		if(jjj>=48*6) //black block
			return split_block8(blk, tPtr);
	}
	
#if 0 //def DEBUG_PRN	
	printk("max_dots %d\n", max_dot);
#endif		

#if 1
	return split_block4(blk,tPtr,div)	;
#else
	{
	unsigned char tch/*,nch,tb*/;
	unsigned short bn/*,bt*/;
	unsigned short dots;
	int max_dot;
	int jj;

	max_dot=MAX_HEAT_DOTS(div);

	memset(&blk[0],0,PRN_LBYTES);
	dots=0;bn=0;
	for(jj=0;jj<PRN_LBYTES;jj++)
	{
		tch=*tPtr;
		tPtr++;
		if(tch==0) continue;


		blk[bn*PRN_LBYTES+jj]=tch;
		
		dots=dots+(unsigned short)s_kdotnum[tch];
		if(dots>=max_dot)
		{
			bn++;							
			dots=0;
			memset(&blk[bn*PRN_LBYTES],0,PRN_LBYTES);			
		}
	}

	if(dots) bn++;
	return bn;
	}
#endif	
}

/*
	hupw:20130912 
	Max speed : 70mm/s = 70x(1/0.125) lines/s = 560 lines/s
	Step speed : 1000 000/560 = 1785 us/lines = 446 us/steps	
*/
#define STEP_DIVS 50 //130
static const  int     MotorStepTimes[STEP_DIVS] ={
    (2523),    
    (2007),    
    (1685),    
    (1464),     
    (1302),    
    (1177),     
    (1078),    
    ( 997),
    ( 929),     
    ( 872),    //10 
    ( 823),   
     ( 780),
     (742),
     ( 708),
     ( 687),
     ( 668),
     ( 651),
     ( 635),
     ( 620),
     ( 606),//20
     ( 593),
     ( 581),
     ( 569),
     ( 558),
     ( 548),
     ( 538),
     ( 529),
     ( 520),
     ( 512),
     ( 504), // 30
     ( 497),
     ( 489),
     ( 482),
     ( 476),
     ( 469),
     ( 463),
     ( 457),
     ( 452),
     ( 446),
     ( 441),  //  40
     ( 436),
     ( 431),
     ( 426),
     ( 422),
     ( 417),
     ( 413),
     ( 409),
     ( 405),
     ( 401),
     ( 397),  // 50
 /*   ( 393),
     ( 390),
     ( 386),
     ( 383),
     ( 379),
     ( 376),
     ( 373),
     ( 370),
     ( 367),
     ( 364), //  60
     ( 361),
     ( 358),
     ( 355),
     ( 353),
     ( 350),
     ( 348),
     ( 345),
     ( 343),
     ( 340),
     ( 338),  //  70
     ( 336),
     ( 333),
     ( 331),
     ( 329),
     ( 327),
     ( 325),
     ( 323),
     ( 321),
     ( 319),
     ( 317),  //  80
     ( 315),
     ( 313),
     ( 311),
     ( 309),
     ( 308),
     ( 306),
     ( 304),
     ( 303),
     ( 301),
     ( 299), //  90
     ( 298),
     ( 296),
     ( 295),
     ( 293),
     ( 291),
     ( 290),
     ( 289),
     ( 287),
     ( 286),
     ( 284), //  100 
     ( 283),
     ( 282),
     ( 280),
     ( 279),
     ( 278),
     ( 276),
     ( 275),
     ( 274),
     ( 273),
     ( 271), // 110
     ( 270),
     ( 269),
     ( 268),
     ( 267),
     ( 266),
     ( 264),
     ( 263),
     ( 262),
     ( 261),
     ( 260), //  120
     ( 259),
     ( 258),
     ( 257),
     ( 256),
     ( 255),
     ( 254),
     ( 253),
     ( 252),
     ( 251),
     ( 250), //  130 
*/     
};
#define MOTO_LINE_STEPS	4

static const  int   sum_motor_step_data[STEP_DIVS-4] ={
 7679,
  6458,
  5628,
  5021,
  4554,
  4181,
  3876,
  3621,
  3404,
  3217,  //10
  3053,
  2917,
  2805,
  2714,
  2641,
  2574,
  2512,
  2454,
  2400,
  2349,  //20 
  2301,
  2256,
  2213,
  2173,
  2135,
  2099,
  2065,
  2033,
  2002,
  1972, //30
  1944,
  1916,
  1890,
  1865,
  1841,
  1818,
  1796,
  1775,
  1754,
  1734,  //40
  1715,
  1696,
  1678,
  1661,
  1644,
  1628,
  1612,
/*  1596,
  1581,
  1566,
  1552,
  1538,
  1524,
  1511,
  1498,
  1486,
  1474,
  1462,
  1450,
  1438,
  1427,
  1416,
  1406,
  1396,
  1386,
  1376,
  1366,
  1357,
  1347,
  1338,
  1329,
  1320,
  1312,
  1304,
  1296,
  1288,
  1280,
  1272,
  1264,
  1256,
  1248,
  1241,
  1234,
  1227,
  1221,
  1214,
  1207,
  1201,
  1194,
  1188,
  1182,
  1175,
  1169,
  1163,
  1157,
  1152,
  1146,
  1140,
  1135,
  1129,
  1124,
  1119,
  1113,
  1108,
  1103,
  1098,
  1093,
  1088,
  1083,
  1078,
  1074,
  1070,
  1065,
  1060,
  1055,
  1050,
  1046,
  1042,
  1038,
  1034,
  1030,
  1026,
  1022,
  1018,
  1014,
  1010,
*/  
};
/*
static inline int  sum_motor_step(int x) {
	return (MotorStepTimes[x]+MotorStepTimes[x+1]+MotorStepTimes[x+2]+MotorStepTimes[x+3]);
}
*/
#define sum_motor_step(x) (sum_motor_step_data[x])

static int motor_delay[MOTO_LINE_STEPS];


static inline void set_motor_delay(int start)
{
	motor_delay[0]=MotorStepTimes[start];
	motor_delay[1]=MotorStepTimes[start+1];
	motor_delay[2]=MotorStepTimes[start+2];
	motor_delay[3]=MotorStepTimes[start+3];
}

static inline int sum_motor_delay(void)
{
	return (motor_delay[0]+motor_delay[1]+motor_delay[2]+motor_delay[3]);
}
		
static int	motor_step_udelay_fast(int us)
{
#ifdef ENABLE_MOTOR_SUBDIVISION
#define STEP_DIVDOR 85

#define STEP_1 100
#define STEP_2 (STEP_1*STEP_DIVDOR/100)
#define STEP_3 (STEP_2*STEP_DIVDOR/100)
#define STEP_4 (STEP_3*STEP_DIVDOR/100)

#define STEP_ALL (STEP_1+STEP_2+STEP_3+STEP_4)

	int i=-1;
	static int last_us=-1;
	static int last_i=-1;

	/*sum_motor_step(0)=7679 */
	if(us>sum_motor_step(0) 
		||sum_motor_delay()>sum_motor_step(0))  // heat slowly
	{
		motor_delay[0]=(us*STEP_1)/STEP_ALL;
		if(motor_delay[0]<(motor_delay[3]*STEP_2/100))   // last step is slowly 
			motor_delay[0]=(motor_delay[3]*STEP_2/100);
				
		motor_delay[1]=motor_delay[0]*STEP_DIVDOR/100;
		motor_delay[2]=motor_delay[1]*STEP_DIVDOR/100;
		motor_delay[3]=motor_delay[2]*STEP_DIVDOR/100;
		last_us=last_i=-1;		
		goto ret_ok;
	}

	if(!motor_delay[0])   //first start
	{
		set_motor_delay(0);
		i=0;
		last_us=last_i=-1;
		goto ret_ok;
	}

	if(last_us==us)
	{ 
		i=last_i; goto ret_ok;
	}
	

	for(i=0;i<STEP_DIVS-4;i++)
	{
		if(us>sum_motor_step(i)){
			i--;	
			break;
		}
		
		if(MotorStepTimes[i]<motor_delay[3])
			break;
	}

	set_motor_delay(i);

	if(last_i==i)
		last_us=us;
	else 
		last_us=-1;
	
	last_i=i;

ret_ok:	
#ifdef DEBUG_PRN
//	printk("motor(%d, %d):%d,%d,%d,%d\n",pprn->temperature, us, motor_delay[0],motor_delay[1],motor_delay[2],motor_delay[3]);
#endif
	
	return i;
#endif	
}

static int last_temperature=0;	
static inline int check_temperature(void)
{
	int err=0;
	int temp;
	if(pprn){
		temp=s_ReadHeadTemp();
		if(last_temperature!=temp)
		{
			last_temperature=temp;
			goto ret_check_temperature;
		}
		
		pprn->temperature=last_temperature;
		
        if(pprn->temperature >= LIMIT_TEMP)  //  超过允许打印上限
        {
        	printk("Hot!\n");
			PrnStatus(STATUS_SUSPEND_HOT);				
			err=1;
			s_PrnSTB(OFF);			
        }		
	}	

ret_check_temperature:
	return err;
}

/**
Description:
	check the voltage during heat	
	
*/
static inline int check_voltage(void)
{
	int voltage, err=0;
	if(pprn){
		voltage=pmic_prt_vol();
        if(voltage <= VOLTAGE_FORBID)  
        {
        	printk("Voltage low level:%d\n", voltage);
			PrnStatus(STATUS_SUSPEND_LOWLEVEL);				
			err=1;
			s_PrnSTB(OFF);			
        }	
		else if(voltage<=LIMIT_VOL_STB
			/*&&pprn->low_level_retry<(LOW_LEVEL_RETRY*2)*/
		)
		{
			pprn->low_level_retry++;
#ifdef DEBUG_PRN			
			if(pprn->low_level_retry==LOW_LEVEL_RETRY)
				printk("Voltage limit level\n");
#endif			
		}
/*
		else if(pprn->low_level_retry>0)
			pprn->low_level_retry--;
*/			
	}	

	return err;
}

static inline int prn_check_cmd_status(void)
{
	int err=0;
	unsigned short state;
	
		state=PrnState(-1);	
		if(state==STATE_STOP){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			err=1;
			//PrnClrDotBuf();
			PrnStatus(STATUS_IDLE);
			goto Ret_check_err;
		}
		else if(state==STATE_SUSPEND){			
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			err=1;
			PrnStatus(STATUS_SUSPEND);
			goto Ret_check_err;
		}

Ret_check_err:			
	return err;			
}		


static inline int prn_check_err_status(void)
{
	int err=0;
	unsigned short state;
	
		state=PrnState(-1);	
		if(state==STATE_STOP){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			err=1;
			//PrnClrDotBuf();
			PrnStatus(STATUS_IDLE);
			goto Ret_check_err;
		}
		else if(state==STATE_SUSPEND){			
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			err=1;
			PrnStatus(STATUS_SUSPEND);
			goto Ret_check_err;
		}

	  //  判断是否缺纸,如果缺纸则退出
		if(s_CheckPaper() == PRN_PAPEROUT)	
		{
			err=1;
			printk("No paper!!!\n");			
			PrnStatus(STATUS_SUSPEND_NOPAPER);
			goto Ret_check_err;
		}

		if(get_status_temperature())
		{
			printk("Temperatue sense high, too hot!\n");
			PrnStatus(STATUS_SUSPEND_HOT);				
			err=1;
		}	

Ret_check_err:			
	return err;			
}		

static void s_PrnGraphic(void)
{
//#define LAT_DATA_US	20
//#define TEMP_ADC_US 20
#define TASK_check_err_status 	(1<<1)
#define TASK_split_block				(1<<2)
#define TASK_calc_motor				(1<<3)
#define TASK_latch_data  				(1<<4)

#define PRE_STEP_MOTOR		8

#ifdef DEBUG_PRN
	int prn_lines=0;
	unsigned long start_jiffes;
#endif

	unsigned short task;

//	unsigned short state;
	int k_next_block0;
	
	unsigned long flags;
	int step_motor,err;
	int m_udelays,h_udelays, h_udelays0 /*, h_every_motor*/;
	int m_lines;		
	int k_CurSplitLine;
#if PRE_STEP_MOTOR
	int pre_step_motor=PRE_STEP_MOTOR;
#endif
	
//	unsigned char empty_buf[PRN_LBYTES];

	if(PrnStatus(-1)==STATUS_OUTOFMEMORY)return;
	if(PrnIsBufEmpty()){PrnStatus(STATUS_IDLE); return;}

	err=PrnStart();
	PrnStatus(err);	
	if(err!=STATUS_BUSY)return;
	
	err=0;
	k_MotorStep=0;
	k_StepGo=1;	
	cfg_STEP();
	s_MotorStep(0);	
	udelay(200);
#ifdef DEBUG_HEAT
	 heat_times=0;
	heat_time_start=0;
//static unsigned long heating[MAX_DOT_LINE];
#endif

	s_STB=OFF;
	hr_s_Motor=OFF;

	task=0;	
	
//	max_delay=0;
//	min_delay=1000000;
//	memset(empty_buf,0,PRN_LBYTES);
//   	enque_motor_latch_data(empty_buf);		
#if PRE_STEP_MOTOR
//	memset((unsigned char *)k_next_SendDot,0,(PRE_STEP_MOTOR*PRN_LBYTES));
#endif

	pprn->transfered==XFER_IDLE;	

	motor_delay[0]=motor_delay[1]=motor_delay[2]=motor_delay[3]=0;
	
#ifdef DEBUG_PRN				
	start_jiffes=jiffies;
#endif

#ifdef DEBUG_FLAG
		 FLAG[0]=0;
#endif

	k_CurSplitLine=k_CurPrnLine;
	if(pprn->temperature=s_ReadHeadTemp());
	err|=prn_check_err_status();

//	pprn->ttl_prn_lines=0;
	while(!PrnIsBufPrintted()&&(err==0))
	{	
	/*
		Stop/Suspend command recieved, exit...
	*/
	
#ifdef DEBUG_PRN				
	prn_lines++;
#endif		

#ifdef DEBUG_FLAG
		 FLAG[prn_lines]=0;
#endif

#if PRE_STEP_MOTOR
	if(pre_step_motor>0)
	{
	 	pre_step_motor--;
		if(pre_step_motor>0)
			task|=TASK_split_block;
		else
			task=0;
		
		k_next_block=0;
	}
	else
		k_CurPrnLine++;
#endif


	if(!(task&TASK_split_block))
	{
		//k_CurPrnLine=(k_CurPrnLine+1)%MAX_DOT_LINE;
		k_next_block=split_block((unsigned char *)k_next_SendDot,&k_DotBuf[k_CurSplitLine][0]);		
		k_CurSplitLine=(k_CurSplitLine+1);
#ifdef DEBUG_FLAG
		 FLAG[prn_lines-1]|=FLAG_1SEG(1);
#endif		
		if(k_next_block>0){
			enque_motor_latch_data(&k_next_SendDot[0][0]);	
#ifdef DEBUG_FLAG
		 FLAG[prn_lines-1]|=FLAG_3SEG(1);
#endif						
		}		
	}
	
#if 0 //def DEBUG_PRN			
	printk("split lines: %d\n", k_next_block);	
//   printk("STB delay(%d, %d)\n", min_delay,max_delay);	
#endif	 

	if(!(task&TASK_calc_motor))
	{
		h_udelays0=get_block_time();   // do not adjustment	
		motor_step_udelay_fast(h_udelays0*k_next_block);
#ifdef DEBUG_FLAG
		 FLAG[prn_lines-1]|=FLAG_2SEG(1);
#endif			
	}

	task=0;
	m_lines=0;	
	k_next_block0=k_next_block;

/*
	Motor step 1 line
*/		
	 for(step_motor=0;step_motor<MOTO_LINE_STEPS;step_motor++)
	 {
		m_udelays=motor_delay[step_motor];
	 //	s_MotorStep(1);	  
		motor_enable_us(m_udelays);
	 		
		do{			
			if(s_STB==ON){
				h_udelays=stb_get_time();		
				if(h_udelays<=0){
				   spin_lock_irqsave(&prn_stb_lock, flags);
					s_PrnSTB(OFF);			
					s_STB=OFF;
				   spin_unlock_irqrestore(&prn_stb_lock, flags);
				//	while(s_STB==ON);  //
				//	break;
				}
			}
			else
				h_udelays=0;
			
			m_udelays=motor_get_time();			
			
			if(m_udelays<=0){
				if((step_motor==MOTO_LINE_STEPS-1)&&m_lines<k_next_block0){
					goto retry_next_heat;  //not finished
				}else
					break;
			}
			/*
				少于5us
			*/
			//if(m_udelays<5)continue;  
			
	 	//h_udelays=(k_next_block0-m_lines+MOTO_LINE_STEPS-step_motor-1)/(MOTO_LINE_STEPS-step_motor);

		//if(h_udelays>0) h_udelays-=30;  // adjust 
		retry_next_heat:
		//for(ii=0;ii<h_udelays;ii++)
		{						
//			while(s_STB==ON);

			/*
				判断是否有加热块需要传送和加热
			*/							
	 		if(h_udelays<=0&&m_lines<k_next_block0)
			{
			/*
				if we reach here, STB must should be off
			*/
				if(s_STB==ON){
						printk("Heating error!\n");						
				}

				if(pprn->transfered==XFER_IDLE){
					enque_motor_latch_data(&k_next_SendDot[m_lines][0]);		
					#ifdef DEBUG_FLAG
		 			FLAG[prn_lines-1]|=FLAG_4SEG(1);  //Urgly 
					#endif							
				}
				
				while(pprn->transfered==XFER_BUSY);

				if(pprn->transfered==XFER_ERR)
				{
					printk("Tranfer error!\n");
				}
				
				if(pprn->transfered==XFER_FINISHED)	{
					pprn->transfered=XFER_IDLE;
					Latch_data();				
				}
				
				stb_enable_us(h_udelays0);
				m_lines++;			

				err|=prn_check_cmd_status();
				if(err)break;						
				err|=check_voltage();	
				if(err)break;						

				if(m_lines<k_next_block0){
					if(pprn->transfered==XFER_IDLE){
						enque_motor_latch_data(&k_next_SendDot[m_lines][0]);		
						#ifdef DEBUG_FLAG
		 				FLAG[prn_lines-1]|=FLAG_3SEG(8);
						#endif	
					}
					continue;
			 	}
			}
			else
				err|=prn_check_cmd_status();	
		}

		if(err)break;				
		
			/**
				所有数据已经发送完毕，可以计算下一次的数据
			*/

#if PRE_STEP_MOTOR
		if(pre_step_motor<=0){
#endif		
			if((!(task&TASK_split_block))&&m_lines>=k_next_block0)
			{
				//k_CurPrnLine=(k_CurPrnLine+1)%MAX_DOT_LINE;
				k_next_block=split_block((unsigned char *)k_next_SendDot,&k_DotBuf[k_CurSplitLine][0]);
				k_CurSplitLine=(k_CurSplitLine+1);
				task|=TASK_split_block;
			#ifdef DEBUG_FLAG
		 		FLAG[prn_lines]|=FLAG_1SEG(4);
			#endif
				continue;
			}

			/**
				下一次的数据已经计算出来了，可以计算下一次的加热时间
			*/

			if((!(task&TASK_calc_motor))&&(task&TASK_split_block))
			{
				h_udelays0=get_block_time();   // do not adjustment	
				motor_step_udelay_fast(h_udelays0*k_next_block);
				task|=TASK_calc_motor;
				#ifdef DEBUG_FLAG
		 		FLAG[prn_lines]|=FLAG_2SEG(4);
				#endif			
				continue;
			}			

			//if(m_udelays<50)continue;  					
			if((task&TASK_calc_motor)&&(task&TASK_split_block))
			{
				if(k_next_block>0&&pprn->transfered==XFER_IDLE){
					enque_motor_latch_data(&k_next_SendDot[0][0]);	
				#ifdef DEBUG_FLAG					
					FLAG[prn_lines]|=FLAG_3SEG(4);
				#endif
				}
			}
#if PRE_STEP_MOTOR
		}
#endif		
			
		/*	
			if(!(task&TASK_check_err_status)){
				err|=prn_check_err_status();
				task|=TASK_check_err_status;
			#ifdef DEBUG_FLAG
		 		FLAG[prn_lines-1]|=FLAG_0SEG(1);
			#endif				
			}						
		*/	
		}while(1/*hr_s_Motor==ON*/);
 	 }
	
	if(s_STB==ON){		
		do{	h_udelays=stb_get_time();}while(h_udelays>0);
	   spin_lock_irqsave(&prn_stb_lock, flags);		
		s_PrnSTB(OFF);			
		s_STB=OFF;
	   spin_unlock_irqrestore(&prn_stb_lock, flags);		
	}


	err|=check_temperature();		
	if(!(task&TASK_check_err_status)){
		err|=prn_check_err_status();
		task|=TASK_check_err_status;
		#ifdef DEBUG_FLAG
 		FLAG[prn_lines-1]|=FLAG_0SEG(2);
		#endif						
	}
	
#ifdef DEBUG_PRN			
	if(m_lines<k_next_block0)
		printk("lines lost: %d, %d\n", m_lines, k_next_block0);	
#endif	 
	// enque_motor_latch_data(&empty_buf[0]);								
  }
 
//	s_PrnSTB(OFF);								
//	enque_motor_latch_data(&empty_buf[0]);												
	
	motor_enable_us(0);
	stb_enable_us(0);

	if(!err&&PrnIsBufPrintted())
	{
		//PrnClrDotBuf();
		PrnStatus(STATUS_IDLE);
	}
	
  cfg_STEP();
  PrnStop(err);
	
#ifdef DEBUG_PRN			
	if(jiffies!=start_jiffes)
		printk("Print : %d lines in %d x10ms, speed(%d mm/s)\n", prn_lines,(int)(jiffies-start_jiffes),(prn_lines*125/(int)(jiffies-start_jiffes))/10);	
	else
		printk("Print : %d lines in %d x10ms\n", prn_lines,(int)(jiffies-start_jiffes));	
//   printk("STB delay(%d, %d)\n", min_delay,max_delay);	
#endif	 
#ifdef DEBUG_HEAT
	for(	h_udelays=0;h_udelays< heat_times;h_udelays++){
			printk("heat times(%d): flag(0x%x) %lu us\n",h_udelays, heating_flag[h_udelays],heating[h_udelays]/1000);	
	}
//static unsigned long heating[MAX_DOT_LINE];
#endif

#ifdef DEBUG_FLAG
	for(	h_udelays=0;h_udelays< prn_lines;h_udelays++)
			printk("Flags(%d): 0x%x \n",h_udelays,  FLAG[h_udelays]);	
#endif

}
#if 0
static void s_ManualFeedPaper(void)
{
	unsigned short state;
	int step_motor,err;
	int m_udelays;
//	int m_lines=0;

	if(k_feedcounter==0)return;
	
	if(k_feedcounter>0)
		k_StepGo=1;	
	else{
		k_feedcounter=-k_feedcounter;
		k_StepGo=-1;
	}
	
	k_MotorStep=0;
//	pprn->ttl_prn_lines=0;
	err=PrnStart();
	PrnStatus(err);	
	if(err!=STATUS_BUSY)return;
	
	err=0;
	cfg_STEP();
	s_MotorStep(0);	
	udelay(300);
	
	motor_delay[0]=motor_delay[1]=motor_delay[2]=motor_delay[3]=0;

	while(k_feedcounter&&!err)
	{	
//		pprn->ttl_prn_lines++;
	
		k_feedcounter--;
	/*
		Stop/Suspend command recieved, exit...
	*/	
		state=PrnState(-1);	
		if(state==STATE_STOP||state==STATE_SUSPEND){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			break;
		}
/*		else if(state==STATE_SUSPEND){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			break;
		}
*/		
	 for(step_motor=0;step_motor<MOTO_LINE_STEPS;step_motor++)
	 {
	  //  判断是否缺纸,如果缺纸则退出
      	if(s_CheckPaper() == PRN_PAPEROUT)	
		{
			err=1;
			PrnStatus(STATUS_SUSPEND_NOPAPER);
			break;
		}

		m_udelays=motor_delay[step_motor];
				
	 	s_MotorStep(1);	   
		
		if(step_motor==0){
			motor_step_udelay_fast(0);
			m_udelays=motor_delay[step_motor];							
		}
		
		udelay(m_udelays);		
 	 }
  }

 if(!err)	
	PrnStatus(STATUS_IDLE);

  cfg_STEP();
  PrnStop(err);
}
#endif
#if 0
int	motor_step_udelay(int m_lines,  int us)
{
#define TTL_PRE_STEPS	18

static const  int     PreStepTime[TTL_PRE_STEPS] ={
	 2890, 1786, 1381, 1157, 1014, 914, 
	 838, 777, 728,687, 651, 621, 
	 596, 572,552, 533, 516, 500
};

	int delay;
#ifdef ENABLE_MOTOR_SUBDIVISION    
	m_lines = (m_lines>>1);
#endif	

	if(m_lines>TTL_PRE_STEPS-1)delay=PreStepTime[TTL_PRE_STEPS-1];
	else delay = PreStepTime[m_lines];

	delay-=us;
	//if(delay)udelay(delay);
	return delay;
}
static void s_PrnGraphic_SLOW(void)
{
//#define LAT_DATA_US	20
//#define TEMP_ADC_US 20
#ifdef DEBUG_PRN
	int max_delay,min_delay;
#endif
	unsigned short state;
	int i,step_motor,err;
	int m_udelays,h_udelays;
	int m_lines=0;
	unsigned char empty_buf[PRN_LBYTES];

	if(PrnIsBufEmpty()){PrnStatus(STATUS_IDLE); return;}

	err=PrnStart();
	PrnStatus(err);	
	if(err!=STATUS_BUSY)return;
	
	err=0;
	k_MotorStep=0;
	k_StepGo=1;	
	cfg_STEP();
	s_MotorStep(0);	
	udelay(300);

	
	max_delay=0;
	min_delay=1000000;
	memset(empty_buf,0,PRN_LBYTES);
   	enque_motor_latch_data(empty_buf);		
/*
	for(m_lines=0;m_lines<36;m_lines++)
	{
	 	s_MotorStep(1);
		m_udelays=motor_step_udelay(m_lines,0);		
		udelay(m_udelays);
	}
*/
//	pprn->ttl_prn_lines=0;
	while(!PrnIsBufEmpty()&&(err==0))
	{	
	/*
		Stop/Suspend command recieved, exit...
	*/	
		state=PrnState(-1);	
		if(state==STATE_STOP){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			PrnClrDotBuf();
			PrnStatus(STATUS_IDLE);
			break;
		}
		else if(state==STATE_SUSPEND){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			PrnStatus(STATUS_SUSPEND);
			break;
		}

		
		k_CurPrnLine=(k_CurPrnLine+1)%MAX_DOT_LINE;
		k_next_block=split_block((unsigned char *)k_next_SendDot,k_DotBuf[k_CurPrnLine]);
#if 0 //def DEBUG_PRN			
	printk("split lines: %d\n", k_next_block);	
//   printk("STB delay(%d, %d)\n", min_delay,max_delay);	
#endif	 

//	pprn->ttl_prn_lines++;

	 for(step_motor=0;step_motor<STEP_MOTOR_HEIGHT;step_motor++)
	 {
	  //  判断是否缺纸,如果缺纸则退出
      	if(s_CheckPaper() == PRN_PAPEROUT)	
		{
			err=1;
			PrnStatus(STATUS_SUSPEND_NOPAPER);
			break;
		}
				
	 	s_MotorStep(1);	   m_lines++;
		m_udelays=motor_step_udelay(m_lines,0);		 					
		//udelay(m_udelays);					
		for(i=0;i<k_next_block;i++)
		{
	    	enque_motor_latch_data(&k_next_SendDot[i][0]);		
			if(get_status_temperature())
			{
				printk("Too hot!\n");
				PrnStatus(STATUS_SUSPEND_HOT);				
				err=1;
				break;
			}	
	 		h_udelays=get_block_time();					
#if 0
            //  判断当前缓冲采样的温度是否过温,如果过温则退出
            if(chptr->Temperature >= TEMPERATURE_FORBID)  // 超过允许打印上限
            {
                s_StopPrn(PRN_TOOHEAT);
				  printk("too hot\n");
                return;
            }

			if(m_udelays>k_BlockTime)
#endif							
			s_PrnSTB(ON);

//			h_udelays+=(300-i*100);

#ifdef DEBUG_PRN			
			max_delay=max(max_delay,h_udelays);
			min_delay=min(h_udelays,min_delay);			
#endif			
			if(h_udelays){
				udelay(h_udelays);
				if(m_udelays>0)	m_udelays-=h_udelays;				
			}
			s_PrnSTB(OFF);
		}
		
		if(m_udelays>0)	udelay(m_udelays);		
			
    	enque_motor_latch_data(&empty_buf[0]);					
		//	m_udelays+=(k_BlockTime+LAT_DATA_US+TEMP_ADC_US);
		for(i=0;i<PRN_NULL_LINES;i++)
		{
      		if(s_CheckPaper() == PRN_PAPEROUT)
			{
				err=PRN_PAPEROUT;
				PrnStatus(STATUS_SUSPEND_NOPAPER);				
				break;
			}								
			m_lines++;
			s_MotorStep(1);
			m_udelays=motor_step_udelay(m_lines,0);		 								
			udelay(m_udelays);			
		}
 	 }
  }
#if 0
	state=PrnState(-1);	
	if(state==STATE_BUSY)
	{
		if(PrnIsBufEmpty()){PrnClrDotBuf();PrnState(STATE_IDLE);}
		else
		{
			/*if not finised???
			*/
		#ifdef DEBUG_PRN			
		printk("Print is not finished: %d\n", state);	
		#endif
			queue_work(pprn->motor_workqueue, &(pprn->motor_work));
		}
	}
	else if(state==CMD_STOP){PrnClrDotBuf();PrnState(STATE_IDLE);}
#endif

	if(!err&&PrnIsBufEmpty())
	{
		PrnClrDotBuf();
		PrnStatus(STATUS_IDLE);
	}
	
  cfg_STEP();
  PrnStop(err);
	
#ifdef DEBUG_PRN			
//	printk("Print lines: %d\n", pprn->ttl_prn_lines);	
   printk("STB delay(%d, %d)\n", min_delay,max_delay);	
#endif	 
}

static void s_ManualFeedPaper(void)
{
	unsigned short state;
	int i,step_motor,err;
	int m_udelays;
	int m_lines=0;

	if(k_feedcounter==0)return;
	
	if(k_feedcounter>0)
		k_StepGo=1;	
	else{
		k_feedcounter=-k_feedcounter;
		k_StepGo=-1;
	}
	
	k_MotorStep=0;
//	pprn->ttl_prn_lines=0;
	err=PrnStart();
	PrnStatus(err);	
	if(err!=STATUS_BUSY)return;
	
	err=0;
	cfg_STEP();
	s_MotorStep(0);	
	udelay(300);
	
/*
	for(m_lines=0;m_lines<36;m_lines++)
	{
	 	s_MotorStep(1);
		m_udelays=motor_step_udelay(m_lines,0);		
		udelay(m_udelays);
	}
*/
	while(k_feedcounter&&!err)
	{	
//		pprn->ttl_prn_lines++;
	
		k_feedcounter--;
	/*
		Stop/Suspend command recieved, exit...
	*/	
		state=PrnState(-1);	
		if(state==STATE_STOP||state==STATE_SUSPEND){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			break;
		}
/*		else if(state==STATE_SUSPEND){
#ifdef DEBUG_PRN			
	printk("printer state: %d\n", state);	
#endif
			break;
		}
*/		
	 for(step_motor=0;step_motor<STEP_MOTOR_HEIGHT;step_motor++)
	 {
	  //  判断是否缺纸,如果缺纸则退出
      	if(s_CheckPaper() == PRN_PAPEROUT)	
		{
			err=1;
			PrnStatus(STATUS_SUSPEND_NOPAPER);
			break;
		}
				
	 	s_MotorStep(1);	   m_lines++;
		m_udelays=motor_step_udelay(m_lines,0);		 					
		if(m_udelays>0)	udelay(m_udelays);		
			
		for(i=0;i<PRN_NULL_LINES;i++)
		{
      		if(s_CheckPaper() == PRN_PAPEROUT)
			{
				err=PRN_PAPEROUT;
				PrnStatus(STATUS_SUSPEND_NOPAPER);				
				break;
			}		
			m_lines++;
			s_MotorStep(1);
			m_udelays=motor_step_udelay(m_lines,0);		 								
			udelay(m_udelays);			
		}
 	 }
  }

 if(!err)	
	PrnStatus(STATUS_IDLE);

  cfg_STEP();
  PrnStop(err);
}
#endif

void s_PrnLooper(struct work_struct *work)
{
	struct sched_param sch_param = {.sched_priority = 1};

	sched_setscheduler(current, SCHED_RR, &sch_param);
	
/*if(pprn->wr_tag==FEEDBACK)s_ManualFeedPaper();
	else
*/	
		s_PrnGraphic();

	wake_up_interruptible(&pprn->wq);
}



int PrnGoPix(short pix)
{
#if 0
//	int ii;
	if(pix==0) return 0;
	while(k_PrnStatus==PRN_BUSY)
	{
		
	}
	s_PrnPower(ON);
	k_PrnStatus=PRN_BUSY;
	if(pix>0) k_StepGo=1;
	else
	{
		k_StepGo=-1;
		pix=0-pix;
	}
	k_StepPaper=pix*2+1;
	s_StartMotor();
	while(k_PrnStatus)
	{
		
	}
	s_StopPrn(0);
#endif		
	return 0;	
}

int PrnState(int newstate)
{
	unsigned short oldstate;

//	spin_lock(&pprn->state_lock);
	
	oldstate= pprn->state;
	if(newstate>STATE_MIN&&newstate<STATE_MAX)
		pprn->state=newstate;
	
//	spin_unlock(&pprn->state_lock);
	return oldstate;
}

int  PrnStatus(int newstatus)
{
	unsigned short oldstatus;

	mutex_lock(&pprn->mutex);
	
	oldstatus= pprn->status;
	if(newstatus>=STATUS_MIN&&newstatus<STATUS_MAX)
		pprn->status=newstatus;
	
	mutex_unlock(&pprn->mutex);
	return oldstatus;
}

int PrnTemperature(void)
{
    return(s_ReadHeadTemp());
}


void PrnSetGray(int Level)
{
	pprn->gray_level=Level;
	
    if(Level == 0)
    {
        k_HeatTime = DEF_HEAT_TIME * 7 / 10;
    }
    if(Level >= 1&&Level<5)
    {
        k_HeatTime = Level*DEF_HEAT_TIME;
    }
    else if((Level>=50) && (Level<=500))    //  按照百分比设置加热浓度
    {
        k_HeatTime = DEF_HEAT_TIME * Level / 100;
    }

#ifdef DEBUG_PRN
	printk("HeadTime:%d\n", k_HeatTime);
#endif				
}


void s_Motor4Step(int n)
{
#if 0
 #else
 	int i = 0;
   s_PrnMTDEF(ON);   // sleep pull up 
	for (i = 0; i < n; i++)
	{
	  __DRV8833_motorStep4(i);
		DelayMs(1);
	}
    s_PrnMTDEF(OFF);   // sleep pull up 
 #endif
}


void s_Motor8Step(int n)
{
#if 0
#else
  	int i = 0;
   s_PrnMTDEF(ON);   // sleep pull up 
	for (i = 0; i < n; i++)
	{
	  __DRV8833_motorStep8_inc(i);
		DelayMs(1);
	}
    s_PrnMTDEF(OFF);   // sleep pull up 
 #endif
}

void ResetPrnPtr(void)
{
	k_CurPrnLine= 0;
}

int PrnIsBufPrintted(void)
{
	return (k_CurPrnLine>=k_CurDotLine)?1:0;
}

	
void PrnClrDotBuf(void)
{
    k_CurDotLine=k_CurPrnLine= 0;
	//pprn->ttl_prn_lines=0;
    memset(k_DotBuf,0,sizeof(k_DotBuf));
	if(PrnStatus(-1)==STATUS_OUTOFMEMORY)
		PrnStatus(STATUS_IDLE);	
}

int PrnIsBufEmpty(void)
{
	//return (k_CurDotLine==k_CurPrnLine)?1:0;
		return k_CurDotLine<=0?1:0;
}

static void *And_memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--){
		*tmp |= *s;
		tmp++ ;
		s++;
	}
	
	return dest;
}

int PrnFillDotBuf(unsigned char *buf, int line)
{
	int i;// ,mline;

	if(PrnStatus(-1)==STATUS_OUTOFMEMORY) return 0;
	
	for (i = 0; i < line; i++)
	{	

		//mline= (k_CurDotLine+1)%MAX_DOT_LINE;
		//if(mline==k_CurPrnLine) { 
		//	mline= (k_CurDotLine+1);
		//	if(mline>MAX_DOT_LINE) { 		
		//		printk("printer buffer overflow!\n");
		//		break;			
		//	}

		//	k_CurDotLine=mline;

		if(k_CurDotLine>=MAX_DOT_LINE) { 				
			printk("printer buffer overflow!\n");
			PrnStatus(STATUS_OUTOFMEMORY);
			break;			
		}
		
		if(buf!=NULL){
			And_memcpy(&k_DotBuf[k_CurDotLine][0], buf, PRN_LBYTES);
			buf+=PRN_LBYTES;			
#if 0 //def DEBUG_PRN
	printk("line:%d, ASCII:%2x,%2x,%2x,%2x\n",k_CurDotLine,
		k_DotBuf[k_CurDotLine][0],k_DotBuf[k_CurDotLine][1],k_DotBuf[k_CurDotLine][2],k_DotBuf[k_CurDotLine][3]);
#endif
	//	pprn->ttl_prn_lines++;
		}
		//else
		//	memset(&k_DotBuf[k_CurDotLine][0],0x00,PRN_LBYTES);		

		k_CurDotLine++;
	}

#ifdef DEBUG_PRN	
//	printk("lines: %d\n", k_CurDotLine);
#endif

	return i;
}

/* get the print data form userspace */
int PrnGetDotBuf(const char __user *from, int len)
{
	int i; //, mline;
	int line=(len+PRN_LBYTES-1)/PRN_LBYTES;
	char tmp[PRN_LBYTES];

	if(PrnStatus(-1)==STATUS_OUTOFMEMORY) return 0;
	
	for(i=0;i<line;i++)
	{
	//	mline= (k_CurDotLine+1)%MAX_DOT_LINE;
	//	if(mline==k_CurPrnLine)  /*Full overflow*/
	//	mline= (k_CurDotLine+1);
	//	if(mline>=MAX_DOT_LINE) { 			
	//	{
	//		printk("printer buffer overflow!\n");
	//		break;			
	//	}
		
	//	k_CurDotLine=mline;
		if(k_CurDotLine>=MAX_DOT_LINE) 			
		{
			printk("printer buffer overflow!\n");
			PrnStatus(STATUS_OUTOFMEMORY);
			break;			
		}

	//	if (copy_from_user(&k_DotBuf[k_CurDotLine][0], from, PRN_LBYTES))
		if (copy_from_user(tmp, from, PRN_LBYTES))
			return -EFAULT;

		And_memcpy(&k_DotBuf[k_CurDotLine][0], tmp, PRN_LBYTES);
		k_CurDotLine++;
		
//		pprn->ttl_prn_lines++;
		from+=PRN_LBYTES;
#if 0 //def DEBUG_PRN
		printk("line:%d, ASCII:%2x,%2x,%2x,%2x\n",k_CurDotLine,
		k_DotBuf[k_CurDotLine][0],k_DotBuf[k_CurDotLine][1],k_DotBuf[k_CurDotLine][2],k_DotBuf[k_CurDotLine][3]);
#endif
	}

	return (i*PRN_LBYTES);
}


int PrnGetFeedback(const char __user *from, int len)
{
	if(PrnStatus(-1)==STATUS_OUTOFMEMORY) return 0;
	
	if (copy_from_user(&k_feedcounter, from, sizeof(int)))
		return -EFAULT;		

	k_feedcounter+=k_CurDotLine;
	if(k_feedcounter>MAX_DOT_LINE||k_feedcounter<0){
		PrnStatus(STATUS_OUTOFMEMORY);	
		return -EFAULT;
	}

	k_CurDotLine=k_feedcounter;
	
	return len;
}


int PrnGetDotLine(void)
{
	//return pprn->ttl_prn_lines;
	return k_CurDotLine;
}



