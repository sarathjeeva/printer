
/** Global includes */
#include <config.h>
#include <errors.h>
/** Other includes */
#include <cobra_defines.h>
#include <cobra_macros.h>
#include <cobra_functions.h>
#include <mml_gcr.h>
#include <mml_intc.h>
#include <mml_tmr.h>
#include <mml.h>
#include <mml_uart_regs.h>

#include "printer_timer.h"

#include "menu/menu.h"

#define prt_dbg					//uart_print
#define SC_Return(_x_)	do { if(_x_){sc_dbg("<line:%d> %s <Err:%d>\r\n", __LINE__, __FUNCTION__, _x_);} return (_x_); } while(0)

mml_tmr_config_t prt_motor_timer_cfg;
mml_tmr_config_t prt_heat_timer_cfg;


#define PRINTER_TIMER_MOTOR		( MML_TMR_DEV1 )
#define PRINTER_TIMER_HEAT		( MML_TMR_DEV2 )

#define PRINTER_TIMER_FCK		( MML_TMR_DEV3 )

typedef void (*PrnTimerHandler_t)(void);

PrnTimerHandler_t gpPrtTimerMotorHandler = NULL;
PrnTimerHandler_t gpPrtTimerHeatHandler = NULL;

static void prtTimerMotorHandler(void)
{	
	mml_tmr_interrupt_clear(PRINTER_TIMER_MOTOR);
	mml_tmr_disable(PRINTER_TIMER_MOTOR);
	
	if(gpPrtTimerMotorHandler != NULL)
		gpPrtTimerMotorHandler();		
	
	//prt_dbg("--------> prtTimerMotorHandler in...\r\n");
}

static void prtTimerHeatHandler(void)
{
	mml_tmr_interrupt_clear(PRINTER_TIMER_HEAT);
	mml_tmr_disable(PRINTER_TIMER_HEAT);

	if(gpPrtTimerHeatHandler != NULL)
		gpPrtTimerHeatHandler();

	//prt_dbg("--------> prtTimerHeatHandler in...\r\n");
}

void prtTimerHandlerRegister(int user_id, void (*handler)(void))
{
	user_id = !!user_id;
	
	if(user_id == PRINTER_TIMER_USER_ID_MOTOR)
		gpPrtTimerMotorHandler = handler;
	else
		gpPrtTimerHeatHandler = handler;
}

void prtTimerInit(int user_id)
{
	int result;
	mml_tmr_config_t timer_conf;
	unsigned int freq;
	mml_tmr_id_t tmr_id;

	user_id = !!user_id;
	
	timer_conf.count	     = 1;
	timer_conf.pwm_value = 0;
	timer_conf.clock	     = MML_TMR_PRES_DIV_4;
	timer_conf.mode 	     = MML_TMR_MODE_ONE_SHOT;
	timer_conf.polarity  = MML_TMR_POLARITY_LOW;
	timer_conf.timeout	 = 0xFFFFFFFF;

	if(user_id == PRINTER_TIMER_USER_ID_MOTOR)
	{
		timer_conf.handler = prtTimerMotorHandler;
		tmr_id             = PRINTER_TIMER_MOTOR;
		memcpy(&prt_motor_timer_cfg, &timer_conf, sizeof(prt_motor_timer_cfg));
	}
	else
	{
		timer_conf.handler = prtTimerHeatHandler;
		tmr_id             = PRINTER_TIMER_HEAT;
		memcpy(&prt_heat_timer_cfg, &timer_conf, sizeof(prt_heat_timer_cfg));
	}
	
	result = mml_tmr_init(tmr_id, &timer_conf);
}
void prtTimerStart(int user_id, int us)
{
	unsigned int cmp;
	unsigned int tmr_id;
	mml_tmr_config_t *cfg;
	
	user_id = !!user_id;
	if(user_id == PRINTER_TIMER_USER_ID_MOTOR)
	{
		cfg = &prt_motor_timer_cfg;
		tmr_id = PRINTER_TIMER_MOTOR;
	}
	else
	{
		cfg = &prt_heat_timer_cfg;
		tmr_id = PRINTER_TIMER_HEAT;
	}
	
	cfg->timeout = us;
	mml_tmr_calc_compare(cfg, &cmp);
	mml_tmr_reload_cnt_cmp(tmr_id, 1, cmp);
	mml_tmr_interrupt_enable(tmr_id);
	mml_tmr_enable(tmr_id);	
}

void prtTimerStop(int user_id)
{
	unsigned int tmr_id;
	
	user_id = !!user_id;

	if(user_id == PRINTER_TIMER_USER_ID_MOTOR)
	{
		tmr_id = PRINTER_TIMER_MOTOR;
	}
	else
	{
		tmr_id = PRINTER_TIMER_HEAT;
	}

	mml_tmr_interrupt_disable(tmr_id);
	mml_tmr_interrupt_clear(tmr_id);
	mml_tmr_disable(tmr_id);
}

void prtTimerFckHandler(void)
{
	static int fck = 0;
	
	mml_tmr_interrupt_clear(PRINTER_TIMER_FCK);
	
	//prt_dbg("<%d> prtTimerFckHandler...in\r\n", fck++);
}

void prtTimerFckInit(void)
{
	int result;
	mml_tmr_config_t timer_conf;
	unsigned int freq;
	mml_tmr_id_t tmr_id;

	
	tmr_id			     = PRINTER_TIMER_FCK;
	
	timer_conf.count		 = 1;
	timer_conf.pwm_value = 0;
	timer_conf.clock		 = MML_TMR_PRES_DIV_1;
	timer_conf.mode 		 = MML_TMR_MODE_CONTINUOUS;
	timer_conf.polarity  = MML_TMR_POLARITY_LOW;
	timer_conf.timeout	 = 0xFFFFFFFF;

	timer_conf.handler = prtTimerFckHandler;
	
	result = mml_tmr_init(tmr_id, &timer_conf);

	//log_tmr_reg(PRINTER_TIMER_FCK);
	
	mml_tmr_interrupt_enable(tmr_id);
	mml_tmr_enable(tmr_id);	
}

void prtTimerFckStop(void)
{
	mml_tmr_id_t tmr_id;
	
	tmr_id			     = PRINTER_TIMER_FCK;
	mml_tmr_interrupt_disable(tmr_id);
	mml_tmr_interrupt_clear(tmr_id);
	mml_tmr_disable(tmr_id);
}



// ---- test -----------
void prtTimerInit_test(void)
{
	int user_id;

	if(xxx_input_int("timer user id: ", &user_id) < 0)
		return ;
	
	//gpio_test_pin_set_val(0);
	
	prtTimerInit(user_id);

	uart_print("prtTimerInit(%d) ok!!\r\n", user_id);
}
void prtTimerStart_test(void)
{
	int user_id;
	int us;
	
	if(xxx_input_int("timer user id: ", &user_id) < 0)
		return ;
	if(xxx_input_int("timer check timeout(us): ", &us) < 0)
		return ;
	
	//gpio_test_pin_set_val(1);
	prtTimerStart(user_id, us);

	//log_tmr_reg(PRINTER_TIMER_MOTOR);

	uart_print("prtTimerStart(%d,%d) ok!!\r\n", user_id, us);
}
void prtTimerStop_test(void)
{
	int user_id;
	
	if(xxx_input_int("timer user id: ", &user_id) < 0)
		return ;
	
	//gpio_test_pin_set_val(0);
	prtTimerStop(user_id);
	
	uart_print("prtTimerStop ok!!\r\n");
}
void prtTimerStartStop_test(void)
{
	int user_id;
	int us;
	
	if(xxx_input_int("timer user id: ", &user_id) < 0)
		return ;
	if(xxx_input_int("timer check timeout(us): ", &us) < 0)
		return ;
	
	//gpio_test_pin_set_val(1);
	prtTimerStart(user_id, us);
	uart_print("prtTimerStart(%d,%d) ok!!\r\n", user_id, us);

	prtTimerStop(user_id);
	uart_print("prtTimerStop ok!!\r\n");
}

extern void prt48dMotorStep_test(void);

void prtTimerHandlerRegister_test(void)
{
	prtTimerHandlerRegister(PRINTER_TIMER_USER_ID_MOTOR, prt48dMotorStep_test);
	
	uart_print("prtTimerHandlerRegister ok!!\r\n");
}

void prtTimerFckInit_test(void)
{
	prtTimerFckInit();
}


void prt_timer_test(void)
{	
	ST_MENU_OPT tMenuOpt[] = {
		" prt_timer_test ", NULL,
		"prtTimerInit_test", prtTimerInit_test,
		"prtTimerStart_test", prtTimerStart_test,
		"prtTimerStop_test", prtTimerStop_test,
		"prtTimerStartStop_test", prtTimerStartStop_test,
		"prtTimerHandlerRegister_test", prtTimerHandlerRegister_test,
		"prtTimerFckInit_test", prtTimerFckInit_test,
	};

	xxx_MenuSelect(tMenuOpt, sizeof(tMenuOpt)/sizeof(tMenuOpt[0]));
}



