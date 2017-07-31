
#include "printer_dev.h"
#include "printer_queue.h"
#include "queue.h"

static unsigned char prtPrnBuf[PRT_BYTES_PER_LINE*PRT_QUEUE_BUFFER_LINES+1];
static T_Queue prtPrnQue;

int prtQueInit(void)
{
	QueInit(&prtPrnQue, prtPrnBuf, sizeof(prtPrnBuf));
	return ( PRT_OK );
}

/*
fill dot line data to print buffer
not use in ISR, need cirtical
*/
int prtQueFillBuf(char *buf, int len)
{
	int elen;

//	cpsid();	
	elen = QueCheckCapability(&prtPrnQue);
//	cpsie();
	if (elen < len)
	{
		/*
		log("<%d, %d, %d, %d>(%d, %d, %d)(%d, %d, %d)", elen, len, dBytes, dLine, 
				prtPrnQue.head, prtPrnQue.tail, prtPrnQue.flag, head, tail, flag);
				*/
		return ( PRT_ERR_BUF_FULL );
	}
//	cpsid();
	QuePuts(&prtPrnQue, buf, len);
//	cpsie();
	
	return ( PRT_OK );
}

/*
used in ISR
*/
int prtQueGetBufFromISR(char *buf, int len)
{
	int glen;
	
	glen = QueGets(&prtPrnQue, buf, len);
	return glen;
}

/*
NOT USE
*/
int prtQueGetBuf(char *buf, int len)
{
	int glen;

	if(QueCheckElements(&prtPrnQue) < len)
		return 0;
//	cpsid();
	glen = QueGets(&prtPrnQue, buf, len);
//	cpsie();
	return glen;
}


/*
check how many dot line can be fill
used NOT in ISR
*/
int prtQueCheckCapacity(void)
{
	//int bytes;
	int line;

//	cpsid();
	line = QueCheckCapability(&prtPrnQue) / PRT_BYTES_PER_LINE;
	/*
	line = bytes/PRT_BYTES_PER_LINE;
	dBytes = bytes;
	dLine = line;
	head = prtPrnQue.head;
	tail = prtPrnQue.tail;
	flag = prtPrnQue.flag;
	*/
//	cpsie();
	
	return line;
}

/*
check how many dot line exist
used only in ISR, needn't critical 
*/
int prtQueCheckLines(void)
{
	int line;

	//portENTER_CRITICAL();
	line = QueCheckElements(&prtPrnQue) / PRT_BYTES_PER_LINE;
	//portEXIT_CRITICAL();
	return line;
}

/*
reset the prn buffer
*/
int prtQueReset(void)
{

//	portENTER_CRITICAL();
	QueReset(&prtPrnQue);
//	portEXIT_CRITICAL();
	return ( PRT_OK );
}

int prtQueFckSet(void)
{
	prtPrnQue.head= ( 48 * 1200 );

	return 0;
}

// ---------------- for test ---------------- 
#include "menu/menu.h"

void prt_queue_log_info(void)
{
	uart_print("head: %d\r\n", prtPrnQue.head);
	uart_print("tail: %d\r\n", prtPrnQue.tail);
	uart_print("size: %d\r\n", prtPrnQue.size);
}

void prtPrintQueInit_test(void)
{
	int ret;

	ret = prtQueInit();

	uart_print("prtQueInit, ret:%d\r\n", ret);
}

void prtPrintQueFillBuf_test(void)
{
	unsigned char data[48];
	int i, ret;
	int val, lines;

	if(xxx_input_int("input fill data val::", &val) < 0)
	{
		return ;
	}
	if(xxx_input_int("input fill lines::", &lines) < 0)
	{
		return ;
	}

	memset(data, (unsigned char)val, sizeof(data));

	for(i = 0; i < lines; i++)
	{
		ret = prtQueFillBuf(data, 48);
		if(ret != 0)
		{
			uart_print("<%d> prtQueFillBuf, err: %d\r\n", i, ret);
			break;
		}
	}
	
	uart_print("prtQueFillBuf, OK!!\r\n");
}

void prtPrintQueGetBuf_test(void)
{
	int ret;
	unsigned char data[48];

	ret = prtQueGetBuf(data, sizeof(data));
	if(ret == sizeof(data))
	{
		uart_print_hex("Queue Data::\r\n", data, sizeof(data));
	}
	
	uart_print("prtQueGetBuf, ret:%d\r\n", ret);
}

void prtPrintQueGetCapacity_test(void)
{
	int ret;

	ret = prtQueCheckCapacity();

	uart_print("prtQueCheckCapacity, ret:%d\r\n", ret);
}

void prtPrintQueGetLines_test(void)
{
	int ret;

	ret = prtQueCheckLines();

	uart_print("prtQueCheckLines, ret:%d\r\n", ret);
}

void prtPrintQueReset_test(void)
{
	int ret;

	ret = prtQueReset();

	uart_print("prtQueReset, ret:%d\r\n", ret);
}

void prt_queue_test(void)
{
	ST_MENU_OPT tMenuOpt[] = {
		" prt_queue_test ", NULL,
		"prt_queue_log_info", prt_queue_log_info,
		"prtPrintQueInit_test", prtPrintQueInit_test,
		"prtPrintQueFillBuf_test", prtPrintQueFillBuf_test,
		"prtPrintQueGetBuf_test", prtPrintQueGetBuf_test,
		"prtPrintQueGetCapacity_test", prtPrintQueGetCapacity_test,
		"prtPrintQueGetLines_test", prtPrintQueGetLines_test,
		"prtPrintQueReset_test", prtPrintQueReset_test,
 	};

	xxx_MenuSelect(tMenuOpt, sizeof(tMenuOpt)/sizeof(tMenuOpt[0]));
}


