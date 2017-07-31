
#ifndef __PRINTER_DEV_H__
#define __PRINTER_DEV_H__

#ifndef NULL
#define NULL		(void *) (0)
#endif

typedef enum {
	PRT_ERR_START = -90000,
	PRT_ERR_NOT_INITED, // -89999
	PRT_ERR_PARAM, // -89998
	PRT_ERR_NULL, // -89997
	PRT_ERR_NODATA, // -89996
	PRT_ERR_NOPAPER, // -89995
	PRT_ERR_OVERHEAT, // -89994
	PRT_ERR_LOW_VOLT, // -89993
	PRT_ERR_HIGH_VOLT,  // -89992
	PRT_ERR_BUF_FULL, // -89991
	PRT_ERR_DIRTY_DATA, // -89990
	PRT_ERR_BUSY, // -89989
	PRT_ERR_CONTROL, // -89988
	PRT_ERR_DAMAGE,  // -89987

    PRT_OK = 0,                           /**< Operation succeed */
} PRT_Return_t;


#define PRT_QUEUE_BUFFER_LINES			( 200 )

#define PRT_DOTS_PER_LINE				( 384 )
#define PRT_BYTES_PER_LINE				( 48 )
#define PRT_BLOCK_PER_LINE				( 8 )
#define PRT_STEPS_PER_LINE				( 4 )

#define MASK_PRT_BLACK_ADJ				(1<<0)		//black adjust
#define MASK_PRT_DOT_ADJ				(1<<1)		//dot auto adjust
#define MASK_PRT_HEAT_TWINCE			(1<<2)		//heat twince per dot
#define MASK_PRT_CUT_SUPPORT			(1<<3)		//support cut paper
#define MASK_PRT_STEP_FULL				(1<<4)		//step2-1
#define MASK_PRT_ROLLER_CHECK			(1<<5)		//support check roller


#define PRT_BYTES_PER_LINE				( 48 )

#define MOTOR		0
#define HEAT		1

#define BLACK_LINE_UPPER			192
#define BLACK_LINE_LOWER			128

#define DOTS_PER_LINE				384
#define BYTES_PER_LINE				48
#define BLOCK_PER_LINE				8

typedef struct{
	short wForwardPre;
	short wForward;
	short wReverseMax;
	short wReverseMin;
	short wReverseHoming;
}T_CutterParam;

typedef struct{
	void (*init)(void);
	void (*config)(void);
	void (*powerControl)(int);
	void (*step)(void);
	//void (*cutterStep)(int dir);
	void (*stbControl)(int);
	void (*motorPower)(int);
	//void (*cutterPower)(int);
	void (*latch)(void);
	void (*load)(char *);
	int (*checkPaper)(void); // 0: have no paper, 1:have paper
	//int (*checkCutter)(void);
	//int (*checkRoller)(void);
	int (*getTemprature)(void);
	int (*getHeatTime)(int);
	unsigned int (*getVoltage)(void);
	unsigned short *stepTab;
	short *cutterStepTab;
	short stepTabCnt;
	short cutterStepTabCnt;
	short preStepCnt;
	short minStepTime;		//this can used to limit the speed
	short minVoltage;
	short maxVoltage;
	short damageTemprature;
	short maxTemprature;
	short normalHeatDot;
	short normalAdjHeatDot;
	short blackAdjHeatDot;
	short blackAdjLine;
	short waittime;
	T_CutterParam *cutParam;
	void (*setCutter)(int type);
	int mask;
	
}T_PrinterDriver;

extern T_PrinterDriver tDrvPt48d;

int prtDevInit(unsigned int pre_step);
int prtDevFill(unsigned char *data, unsigned int len);
int prtDevSetGray(int gray);
int prtDevStep(unsigned int step);
int prtDevStart(unsigned int finish_step);
int prtDevGetStatus(void);
int prtDevStop(int err);
int prtDevPrintSalesSlip(unsigned int pre_step, unsigned int finish_step);
int prtDevCheckCapacity(void);
void prtDevSetFillFinish(void);
void prtDevClrFillFinish(void);


#endif


