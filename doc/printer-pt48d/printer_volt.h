#ifndef __PRINTER_VOLT_H__
#define __PRINTER_VOLT_H__

void prtAdcVoltStart(void);
void prtAdcVoltStop(void);
// 获取当前的打印电压(毫伏)
unsigned int prtGetCurVoltage(void);


#endif


