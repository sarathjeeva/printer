 #ifndef __PRN_IO_H
 #define __PRN_IO_H

#include <mach/gpio.h>

#define BIT0 1 << 0
#define BIT1 1 << 1
#define BIT2 1 << 2
#define BIT3 1 << 3
#define BIT4 1 << 4
#define BIT5 1 << 5
#define BIT6 1 << 6
#define BIT7 1 << 7
#define BIT8 1 << 8
#define BIT13 1 << 13
#define BIT15 1 << 15
#define BIT30 1 << 30
#define BIT31 1 << 31

#define ON 1
#define OFF 0

#define HIGH  1
#define LOW  0

// 允许步进细分

//#undef  ENABLE_MOTOR_SUBDIVISION    
#define ENABLE_MOTOR_SUBDIVISION    



#define MIN_Rtmp	(1024*12/(12+10))

#define MAX_Rtmp	(1024*27/(27+10))

#define CHK_Rtmp	(1024*16/(16+10))



#define TYPE_PRN_UNKNOW 0xff

#define TYPE_PRN_APS    1

#define TYPE_PRN_PRT	2



//最大加热点精确到位
//#define BIT_SEL
//打印过程中根据温度调整加热时间
#define TEMPERATURE_ADJ 

//自动调节最多加热点
//#define AUTO_ADJ_DOT 
//使用自动调节加热点时候,当上一行点超过这个数值时候,需要降低加热点
#define MAX_LINE_DOT (384-192)
//打印黑块时自动调整
#define BLACK_ADJ
//连续4行黑块
#define MAX_BLACK_LINE 4
//调整后打印黑块时最大加热点
#define BLACK_HEADER_DOT 48
//电信验证用
//#define BLACK_HEADER_DOT 16

// 每次最多加热点数
#define SEL_HEADER_DOT		64
//电信验证用
//#define SEL_HEADER_DOT		32


// 第一级保护温度,在此温度下软件控制停止打印

#define TEMPERATURE_ALLOW    65    

// 第二级保护温度,在此温度下硬件控制停止打印

#define TEMPERATURE_FORBID   70      


//温度控制ADC通道

#define ADC_TEMP_CHANNEL 1

/* printer */
#ifdef PRN_IO_REQUEST
static inline void pull_pin(int gpio, int high) 
 {
	//gpio_direction_output(gpio, high);
	__gpio_set_value(gpio,high);
 } 
#else
 //hupw:20120911 
static inline void pull_pin(char *name, int high) 
 {
    int gpio = get_pax_gpio_num(name);
    gpio_request(gpio, name);
	gpio_direction_output(gpio, high);
 } 
#endif
  
#ifdef PRN_IO_REQUEST
#define enable_STB0()	 pull_pin(pprn->prnIO.outp_PRT_STB, 0)
#define disable_STB0() pull_pin(pprn->prnIO.outp_PRT_STB, 1)
#else
#define enable_STB0()	 pull_pin(PIN_PRT_STB14, 0)
#define disable_STB0() pull_pin(PIN_PRT_STB14, 1)
#endif

#define enable_STB1()	 /*pull_pin(PIN_PRT_STB23, 0)*/
#define disable_STB1() /*pull_pin(PIN_PRT_STB23, 1)*/
  
#define enable_STB2()	 /*pull_pin(PIN_PRT_STB56, 0)*/
#define disable_STB2() /*pull_pin(PIN_PRT_STB56, 1)*/

#define enable_STB()	\
 do {\
  enable_STB0();\
  enable_STB1();\
  enable_STB2();\
} while(0) 

#define disable_STB()	\
do {\
  disable_STB0();\
  disable_STB1();\
  disable_STB2();\
} while(0) 


#define PRN_SPI_BAUD        5000000

//SPI通讯控制

#define spi_snd_ch(a) 		spi_write_data(spi_printer, a) //SPI0_DAT_REG = ((uint)a<<8)

#define wait_spi_snd_over()	//while((SPI0_STA_REG & 0x02))

//SPI数据锁存控制
#ifdef PRN_IO_REQUEST
#define enable_LAT()	 pull_pin(pprn->prnIO.outp_PRT_LAT, 0)   // LAT low active 
#define disable_LAT() pull_pin(pprn->prnIO.outp_PRT_LAT, 1)    
#else
#define enable_LAT()	 pull_pin(PIN_PRT_LAT, 0)   // LAT low active 
#define disable_LAT() pull_pin(PIN_PRT_LAT, 1)    
#endif

//电源电压控制
#ifdef PRN_IO_REQUEST
#define enable_pwr_t()	 pull_pin(pprn->prnIO.outp_PRT_7VON,0)
#define disable_pwr_t() pull_pin(pprn->prnIO.outp_PRT_7VON,1)
#else
#define enable_pwr_t()	 pull_pin(PIN_PRT_7VON,0)
#define disable_pwr_t() pull_pin(PIN_PRT_7VON,1)
#endif

//逻辑电压控制脚 
#ifdef PRN_IO_REQUEST
#define enable_pwr_33()	pull_pin(pprn->prnIO.outp_PRT_3VON,1) 
#define disable_pwr_33()	pull_pin(pprn->prnIO.outp_PRT_3VON,0)
#else
#define enable_pwr_33()	pull_pin(PIN_PRT_3VON,1) 
#define disable_pwr_33()	pull_pin(PIN_PRT_3VON,0)
#endif

#define enable_pwr_5v()	 
#define disable_pwr_5v() 

//电机相位控制
#ifdef PRN_IO_REQUEST
#define MOTOR_1A_HIGH() pull_pin(pprn->prnIO.outp_PRT_MTAP,1)
#define MOTOR_1A_LOW() pull_pin(pprn->prnIO.outp_PRT_MTAP,0)

#define MOTOR_2A_HIGH() pull_pin(pprn->prnIO.outp_PRT_MTAN,1)
#define MOTOR_2A_LOW() pull_pin(pprn->prnIO.outp_PRT_MTAN,0)

#define MOTOR_1B_HIGH() pull_pin(pprn->prnIO.outp_PRT_MTBP,1)
#define MOTOR_1B_LOW() pull_pin(pprn->prnIO.outp_PRT_MTBP,0)

#define MOTOR_2B_HIGH() pull_pin(pprn->prnIO.outp_PRT_MTBN,1)
#define MOTOR_2B_LOW() pull_pin(pprn->prnIO.outp_PRT_MTBN,0)
#else
#define MOTOR_1A_HIGH() pull_pin(PIN_PRT_MTAP,1)
#define MOTOR_1A_LOW() pull_pin(PIN_PRT_MTAP,0)

#define MOTOR_2A_HIGH() pull_pin(PIN_PRT_MTAN,1)
#define MOTOR_2A_LOW() pull_pin(PIN_PRT_MTAN,0)

#define MOTOR_1B_HIGH() pull_pin(PIN_PRT_MTBP,1)
#define MOTOR_1B_LOW() pull_pin(PIN_PRT_MTBP,0)

#define MOTOR_2B_HIGH() pull_pin(PIN_PRT_MTBN,1)
#define MOTOR_2B_LOW() pull_pin(PIN_PRT_MTBN,0)
#endif

#define cfg_STEP()	\
do {\
  /*MOTOR_1A_HIGH();*/MOTOR_1A_LOW();\
  /*MOTOR_2A_HIGH();*/MOTOR_2A_LOW();\
  /*MOTOR_1B_HIGH();*/MOTOR_1B_LOW();\
  /*MOTOR_2B_HIGH();*/MOTOR_2B_LOW(); \
} while(0) //GPIO2_EN_SET_REG  = (0xf<<3);GPIO2_OE_SET_REG  = (0xf<<3)

//?? #define set_MTDEF() GPIO2_OUT_CLR_REG =(0x0f<<3);GPIO2_OUT_SET_REG=(0x0a<<3)
#ifdef PRN_IO_REQUEST
#define set_MTDEF() pull_pin(pprn->prnIO.outp_PRT_PDN, 1)

#define clr_MTDEF() pull_pin(pprn->prnIO.outp_PRT_PDN, 0)
#else
#define set_MTDEF() pull_pin(PIN_PRT_PDN, 1)

#define clr_MTDEF() pull_pin(PIN_PRT_PDN, 0)
#endif


//调试口定义

#define s_DebugComPrint s_uart0_printf

#define AdcInit()		



//#define cfg_STB()		do { enable_STB(); disable_STB(); } while (0) //GPIO1_EN_SET_REG  = BIT31;GPIO1_OE_SET_REG  = BIT31

//#define cfg_LAT()		do { disable_LAT(); enable_LAT(); } while (0) //GPIO1_EN_SET_REG  = BIT30;GPIO1_OE_SET_REG  = BIT30


#define cfg_CHKPAPER()	//do {int num = GPIONUM(2, 29); gpio_request(num, "PRN_PAPER"); printer_gpio_output(num, 1); printer_gpio_output(num, 0);} while(0) //GPIO1_EN_SET_REG  = BIT5;GPIO1_OE_CLR_REG  = BIT5

//BIT29为电源 BIT2为3.3v逻辑电压

#define cfg_PWR()		do { enable_pwr_t(); disable_pwr_t(); } while (0) //GPIO1_EN_SET_REG  = BIT2;GPIO1_OE_SET_REG  = BIT2

//#define cfg_spi()		spi_config_printer(spi_printer) //GPIO1_EN_CLR_REG  = (BIT13 | BIT15)


//低功耗口状态配置
#define cfg_idle_port()	





//为保证电机时序需要的调整,单位为us,这个需要精确调试

#define ADJ_MOTOR_PRE 3

#define ADJ_MOTOR_ST1 0

#define ADJ_MOTOR_ST2 0

//#define ADJ_MOTOR_ST2 6



//加热头中断耗时 它包括SPI通许耗时,这个时间建议测试出来

#define HEAD_INT_TIME 100

#endif 

