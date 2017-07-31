#ifndef _PRINTER_DRIVER_H
#define _PRINTER_DRIVER_H

#define ENABLE_MOTOR_SUBDIVISION

#define MAX_DOT_LINE		5000

/* error codes */
#define PRN_OK			0x00
#define PRN_BUSY		0x01
#define PRN_PAPEROUT		0x02
#define PRN_OVERHEAT		0x03
#define PRN_OUTOFMEMORY		0x04
#define PRN_OVERVOLTAGE		0x05
#define PRN_SET_TIME_WRONG	0x10

void s_PrnInit(void);
unsigned char PrnStart(int line);
void PrnSetGray(unsigned char Level);
unsigned char PrnStatus(void);
unsigned long GetDotBuf(const char __user *from, int len);
void FillDotBuf(int offset, int len);

#endif /* _PRINTER_DRIVER_H */

