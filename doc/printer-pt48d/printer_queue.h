#ifndef __PRINTER_QUEUE_H__
#define __PRINTER_QUEUE_H__

int prtQueInit(void);
/*
fill dot line data to print buffer
not use in ISR, need cirtical
*/
int prtQueFillBuf(char *buf, int len);
/*
used in ISR
*/
int prtQueGetBufFromISR(char *buf, int len);
/*
NOT USE
*/
int prtQueGetBuf(char *buf, int len);
/*
check how many dot line can be fill
used NOT in ISR
*/
int prtQueCheckCapacity(void);
/*
check how many dot line exist
used only in ISR, needn't critical 
*/
int prtQueCheckLines(void);
/*
reset the prn buffer
*/
int prtQueReset(void);


#endif


