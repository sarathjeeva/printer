
#include "printer_dbg.h"
#include "menu/menu.h"

#define prt_dbg_out		//uart_printk

#define PRT_DBG_SIZE	( 10 )

struct prt_dbg_tag {
	char tag[8];
	unsigned int val;
} prt_dbg_buf[PRT_DBG_SIZE];

unsigned int prt_dbg_len;

void prt_dbg_add(char *tag, unsigned int val)
{
	int i;

	for(i = 0; i < strlen(tag) && i < sizeof(prt_dbg_buf[0].tag)-1; i++)
	{
		prt_dbg_buf[prt_dbg_len].tag[i] = tag[i];
	}
	prt_dbg_buf[prt_dbg_len].tag[i] = '\0';
	prt_dbg_buf[prt_dbg_len].val = val;
	
	prt_dbg_len++;
	if(prt_dbg_len >= sizeof(prt_dbg_buf)/sizeof(prt_dbg_buf[0]))
	{
		prt_dbg_len = sizeof(prt_dbg_buf)/sizeof(prt_dbg_buf[0]) - 1;
	}
}

void prt_dbg_show(void)
{
	int i;

	for(i = 0; i < prt_dbg_len; i++)
	{
		prt_dbg_out("<%04d> tag: %8s, val: %d\r\n", i, prt_dbg_buf[i].tag, prt_dbg_buf[i].val);
	}
}

void prt_dbg_init(void)
{
	memset(prt_dbg_buf, 0x00, sizeof(prt_dbg_buf));
	prt_dbg_len = 0;
}

void prt_dbg_reset(void)
{
	prt_dbg_len = 0;
}

void prt_dbg_tag_test(void)
{	
	ST_MENU_OPT tMenuOpt[] = {
		" prt_dbg_tag_test ", NULL,
		"prt_dbg_show", prt_dbg_show,
		"prt_dbg_init", prt_dbg_init,
	};

	xxx_MenuSelect(tMenuOpt, sizeof(tMenuOpt)/sizeof(tMenuOpt[0]));
}


