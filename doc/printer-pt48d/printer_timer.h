#ifndef __PRINTER_TIMER_H__
#define __PRINTER_TIMER_H__


#define PRINTER_TIMER_USER_ID_MOTOR		( 0 )
#define PRINTER_TIMER_USER_ID_HEAT		( 1 )

void prtTimerHandlerRegister(int user_id, void (*handler)(void));
void prtTimerInit(int user_id);
void prtTimerStart(int user_id, int us);
void prtTimerStop(int user_id);

void prtTimerFckInit(void);
void prtTimerFckHandler(void);
void prtTimerFckStop(void);


#endif


