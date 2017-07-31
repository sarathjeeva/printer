#ifndef __PRINTER_TEMP_H__
#define __PRINTER_TEMP_H__


void prtAdcTempStart(void);
void prtAdcTempStop(void);
// 获取当前的加热头温度(摄氏度)
int prtGetCurTemperature(void);

#endif


