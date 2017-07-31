
#ifndef PRINTER_T
#define PRINTER_T

//#include "ZA9L.H"
//#include "ZA9L_MacroDef.h"


#define MAXPRNBUFFER	500000
#define ONELINEMARK		1
#define ONELINEBYTES	49
#define MAX_PRNLINES	MAXPRNBUFFER/ONELINEBYTES

#ifndef     ON
#define     ON          1
#endif

#ifndef     OFF
#define     OFF         0
#endif

#ifndef     LF
#define     LF          0x0a
#endif
#ifndef     FF
#define     FF          0x0c
#endif

/*******  Write Printer  ******************************************/
#define POWERCTRL		0x01		    //打印机上电控制
#define STROBE135		0x02		    //加热块1,3,5
#define STROBE246		0x03		    //加热块2,4,6
#define STROBEALL		0x04
#define LATCH_DATA		0x05		    //锁存数据
#define STEPCTRL		0x06		    //电机驱动
#define STEPAB			0x07            //AB相位
#define STEP_1PHASE		0x08            //关相位
#define ADC_CONV		0x09            //AD转换
#define STROBE1         0x0A            //热块 1
#define STROBE23        0x0B            //热块 2,3
#define STROBE4         0x0C            //热块 4
#define STROBE56        0x0D            //热块 5,6

/******   error no   **********************************************/
#define PRN_OK                  0x00
#define PRN_BUSY				0x01
#define PRN_PAPEROUT			0x02
#define PRN_WRONG_PACKAGE		0x03
#define PRN_FAULT				0x04
#define PRN_TOOHEAT				0x08
#define PRN_UNFINISHED			0xf0
#define PRN_NOFONTLIB			0xfc
#define PRN_OUTOFMEMORY			0xfe

#endif //PRINTER_T


