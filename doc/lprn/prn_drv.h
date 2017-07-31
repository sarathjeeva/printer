 #ifndef __PRN_DRV_H
 #define __PRN_DRV_H

 #include <linux/types.h>
#include <linux/list.h>
#include <linux/sysfs.h>
#include <linux/compiler.h>
#include <linux/spinlock.h>
#include <linux/kref.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <asm/atomic.h>
#include <linux/font.h>
#include <linux/spi/spi.h> 
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/module.h>

#include "printer.h"
//#include "prn_io.h"
#define PRN_IO_REQUEST 


#define DEBUG_PRN

#define PRESTEP_NUM 	26	

#define PRN_DOT_ES	125  /*(0.125mm)*/

#define PRN_LDOTS  384   /*Every line dots*/
#define PRN_LBYTES	(PRN_LDOTS>>3)   /*Every line dot bytes*/

#define PLOFF(x)  (x*PRN_LBYTES)

#define MAX_DOT_LINE      (5000)  // (4096)

#define MODE_SLOW_START 1
#define MODE_SLOW_STOP	2

/*define the split block as:
	2^0 = 1   not split, heat total line once
	2^1 = 2   split to 2 block, 192 dot once block
	2^2 = 4   split to 4 block, 96 dot once block	
	2^3 = 8   split to 8 block, 48 dot once block		
	2^4 = 16   split to 16 block, 24 dot once block		
	2^5 = 32   split to 32 block, 12 dot once block				
*/
#define MAX_SPLIT_DIV		5
#define MAX_HEAT_BLOCK      (1<<MAX_SPLIT_DIV)   
#define MAX_HEAT_DOTS(div) 	(PRN_LDOTS>>div)

/************   打印缓冲结构             **************************/
typedef struct
{
    short Temperature;
    short ADValue;
}ADConvT;
	
/*------------------------------------------------*/
enum prn_tag{
	MIN_TAGS=0,
//	SYSDEF_TEXT=MIN_TAGS,
	VGA8x16_TEXT=MIN_TAGS,
	VGA8x8_TEXT,			
	GRAPHY,				
	FEEDBACK,					
	MAX_TAGS
};

enum prn_state{
	STATE_MIN=0,		
	STATE_START,		
	STATE_STOP,
	STATE_SUSPEND,
	STATE_RESUME,		
	STATE_RESET,	
	STATE_MAX					
};

enum prn_status{
	STATUS_MIN = 0,
	STATUS_IDLE=STATUS_MIN,				
	STATUS_BUSY,
	STATUS_SUSPEND,	
	STATUS_SUSPEND_NOPAPER,
	STATUS_SUSPEND_HOT,	
	STATUS_NOPAPER,
	STATUS_SUSPEND_LOWLEVEL,		
	STATUS_OUTOFMEMORY,
	STATUS_MAX
};

#define isBusyStatus(st) 	(st>=STATUS_BUSY&&st<=STATUS_SUSPEND_HOT)

enum transfer_status{
	XFER_IDLE=0,
	XFER_BUSY,
	XFER_FINISHED,
	XFER_ERR,
	XFER_STATUS
};

struct prn_context{
	struct list_head list;
	enum prn_tag tag;
//	unsigned char name[MAX_FONT_NAME+1];
	long ppos;
	long	size;
	void *data;
};

#ifdef PRN_IO_REQUEST
struct PrnIOStruct{
	int outp_PRT_LAT;

	int outp_PRT_STB; 

	int  outp_PRT_MTAP; 
	int  outp_PRT_MTAN; 
	int  outp_PRT_MTBP;
	int  outp_PRT_MTBN;

	int  outp_PRT_7VON;
	int  outp_PRT_3VON ;
	int  outp_PRT_PDN ;

	int  inp_PRT_TEMP;
	int  inp_PRT_POUT;
};

//extern PPrinter pprn;
#endif

typedef struct PrnStruct{
	int open;
/*
	struct list_head context;
	spinlock_t list_lock;
*/	
	unsigned short wr_tag;	  /*Current printer tag*/
	unsigned short gray_level;
	unsigned short block_div;			
	unsigned short state;
	unsigned short status;
	unsigned short temperature;
	int low_level_retry;
	int ttl_prn_lines;	
//	spinlock_t state_lock;	/* spin lock */
	struct font_desc *ft;	

#ifdef PRN_IO_REQUEST
	struct PrnIOStruct prnIO;
#endif

	int transfered;
   struct spi_message m;
   struct spi_transfer t;

	 
	struct device *dev;	
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
   struct work_struct motor_work;   /* the work queue */
   struct workqueue_struct *motor_workqueue;       /* touch work queue handle */
	wait_queue_head_t 	wq;	/* wait queue for write */	 
	struct mutex		mutex;	/* mutex protecting status */	
}Printer, *PPrinter;

extern PPrinter pprn;
/*-----------------------------------------------*/
// 电源开关控制
void s_PrnMTDEF(unsigned char status);
//void s_PrnSTB(unsigned char status);
void s_PrnLatch(unsigned char status);
void s_PrnPower(unsigned char status);

void PrnSetGray(int Level);



void s_LatchData(unsigned char *buf);
void s_clear_latch_data(void);




//void s_MotorStep(unsigned char status);

void s_PrnInit(void);

int s_CheckPaper(void);	
int s_ReadHeadTemp(void);
int s_ReadVoltage(void);


int PrnState(int newstate);
int PrnStatus(int newstatus);

int PrnTemperature(void);
int PrnGetDotLine(void);

int  get_status_paper(void);
 int get_status_temperature(void);
 	
void s_Motor4Step(int n);
void s_Motor8Step(int n);

int PrnIsBufEmpty(void);
int PrnIsBufPrintted(void);
void ResetPrnPtr(void);
void PrnClrDotBuf(void);
int PrnFillDotBuf(unsigned char *buf, int count);
/* get the print data form userspace */
int PrnGetDotBuf(const char __user *from, int len);
int PrnGetFeedback(const char __user *from, int len);

void s_PrnLooper(struct work_struct *work);
#define MOTOR_POLL_DELAY			10 /* ms delay between samples */

#define LOW_LEVEL_RETRY 5
/*
#define LIMIT_VOL  (3600)
#define LIMIT_VOL_STB  (LIMIT_VOL-200)
#define VOLTAGE_FORBID (LIMIT_VOL-300)   
#define LIMIT_RESTART_VOL (LIMIT_VOL+200)
*/
extern int pmic_current_battery_warning(void);
#define LIMIT_VOL  (-1)
#define LIMIT_VOL_STB  (-2)
#define VOLTAGE_FORBID (-3)   
#define LIMIT_RESTART_VOL (0)



#define LIMIT_TEMP (TEMPERATURE_FORBID-5)
#define LIMIT_RESTART_TEMP (TEMPERATURE_FORBID-25)
#endif

