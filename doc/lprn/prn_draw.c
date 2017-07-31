#include <linux/string.h>
#include <linux/font.h>

#include "prn_drv.h"
#include "prn_draw.h"
#include "prn_io.h"

#define MAX_FONT_WIDTH      32
#define MAX_FONT_HEIGHT     32

//static int      k_CharSpace,k_LineSpace;
//static int   	k_LeftIndent,k_RightIndent;                // 行列间距      
static int      k_CurDotCol;
static int 		k_CurHeight;   // 当前列,左边界,当前字体高度变量
static unsigned char    k_ExBuf[2*MAX_FONT_HEIGHT][PRN_LBYTES];

//printer API
unsigned char PrnInit(void)
{
//    k_CharSpace     = 0;
//    k_LineSpace     = 0;
		
    k_CurDotCol     = 0;
    k_CurHeight     = 0;
//    k_LeftIndent    = 0;
    memset(k_ExBuf,0,sizeof(k_ExBuf));

#ifdef CONFIG_FONT_8x16
    PrnFontSet("VGA8x16");
#endif
    return 0;
}

void PrnFontSet(char *name)
{
	struct font_desc *ft;
	
	ft=(struct font_desc *)find_font((const char *)name);
	if(ft!=NULL&&pprn)
	{
		pprn->ft = ft;
		k_CurHeight = ft->height;
#ifdef DEBUG_PRN
	printk("Set font: %s, idx(%d), Size(%d,%d)\n ", ft->name, ft->idx,ft->width, ft->height);
#endif
	}
}
#if 0
// 设置行列间距
void PrnSpaceSet(unsigned char x, unsigned char y)
{
    k_CharSpace = x;
    k_LineSpace = y;
}
// 设置字符间距
void  PrnSetSpCh(unsigned char x)
{
    k_CharSpace = x;
}
// 设置行间距
void  PrnSetSpLi(unsigned char y)
{
    k_LineSpace = y;
}
void PrnLeftIndent(unsigned short x)
{
    if(x > 300)     // 336
        x = 300;    // 336
    if(k_LeftIndent != k_CurDotCol) s_NewLine();
    k_LeftIndent = (int)x;
    k_CurDotCol  = (int)x;
}
#endif
//  调用s_NewLine函数请注意判断是否可以成功填充到打印缓冲
static int s_NewLine(void)
{
    int i;

#if 0 
    if(k_CurDotCol == k_LeftIndent)
    {
#ifdef DEBUG_PRN
	printk("newline16 \n");
#endif    	
        k_CurHeight = 16;
#if 0        
        if(k_CurDotLine+16+k_LineSpace > MAX_DOT_LINE)
        {
//            k_PrnStatus  = PRN_OUTOFMEMORY;
//            k_CurDotLine = MAX_DOT_LINE;
            return -1;
        }
        k_CurDotLine += (16+k_LineSpace);
        return 0;
#endif				
    }
#else
	if(k_CurHeight<=0||k_CurHeight>=2*MAX_FONT_HEIGHT) 
	{
		printk("Font height(%d) overflow!\n", k_CurHeight);
		return (0);
	}
#endif

#ifdef DEBUG_PRN
/*	for(i=0;i<k_CurHeight;i++)
	printk("Fill ASCII:%2x,%2x,%2x,%2x\n",
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][0],
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][1],
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][2],
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][3]);*/
#endif		

	i=PrnFillDotBuf((unsigned char *)&k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight][0], k_CurHeight);
//	if(i<=0)return i;

	/*Fill Space line*/		
//	i=PrnFillDotBuf((unsigned char *)NULL, k_LineSpace);		
//	if(i<=0)return i;
   
    memset(k_ExBuf,0,sizeof(k_ExBuf));		
//    k_CurDotCol   = k_LeftIndent;
    k_CurDotCol   = 0;
//    k_CurHeight   = 0;		

	if(i>0)i=0;

    return i;
}

static inline void _GetFont(unsigned char *buf, struct font_desc *ft,  unsigned short  code)
{
	unsigned int offset;
	int size = (ft->width*ft->height)/8;
	
	offset=	(unsigned int)(code);
	offset =offset * size;
	
   memcpy(buf, ft->data+offset, size);
}

static int s_ProcChar(unsigned char *str)
{
/* hupw:201210
	Just for Byte pixel alignment 
*/
    int i,j;
    int Width,Height;
    unsigned char *tDotPtr,*Font;
    unsigned char tmpBuf[(MAX_FONT_WIDTH*MAX_FONT_HEIGHT)/8+1];

	if(pprn==NULL||pprn->ft==NULL) 
	{
		printk("printer not set!\n");
		return (0);
	}
	
	Width=pprn->ft->width;
	Height=pprn->ft->height;
	if(Width<0||Width>MAX_FONT_WIDTH
		||Height<0||Height>MAX_FONT_HEIGHT)
	{
		printk("Font size(%d,%d) overflow!\n", Width,Height);
		return (0);
	}
	
  	_GetFont(tmpBuf,pprn->ft,(unsigned short)(*str));

    if(k_CurDotCol+Width > PRN_LDOTS)
    {
        if(s_NewLine()) return (0);
    }

	if(k_CurHeight < Height) k_CurHeight = Height;

    tDotPtr   = &k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight][k_CurDotCol/8];
    Font      = tmpBuf;

    for(i=0; i<Height; i++)
    {
    	tDotPtr   = &k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][k_CurDotCol/8];
        // 填充点阵
        for(j=0; j<Width; j+=8)
        {
            *tDotPtr |= *Font;									
			  tDotPtr++;
			  Font++;
        }
    }
		
//  	 k_CurDotCol += (Width+k_CharSpace);
  	 k_CurDotCol += (Width);
	return 1;

#if 0 //def DEBUG_PRN
	for(i=0;i<k_CurHeight;i++)
	printk("Fill ASCII:%2x,%2x,%2x,%2x\n",
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][0],
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][1],
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][2],
		k_ExBuf[2*MAX_FONT_HEIGHT-k_CurHeight+i][3]);
#endif		
}

unsigned int s_PrnStr(unsigned char *str)
{
    int i,CodeBytes;
    
    i = 0;
    while(1)
    {
       if(!str[i]){
		/*	if(k_CurDotCol!=k_LeftIndent)s_NewLine();*/
			if(k_CurDotCol>0)s_NewLine();
			break;
        }

		 if(str[i]==0x0d&&str[i+1]=='\n'	)i++;					 
        if(str[i] == '\n')
        {
            if(s_NewLine()) break;
            i++;
        }		
        else
        {
         	CodeBytes=s_ProcChar(str+i);
			if(CodeBytes<=0)break;			 
          i += CodeBytes;
        }
    }
		
    return i;
}


void  PrnDrawClrBuf(void)
{
    k_CurDotCol     = 0;
//    k_CurHeight     = 0;
    memset(k_ExBuf,0,sizeof(k_ExBuf));
}


