 #ifndef __PRN_DRAW_H
 #define __PRN_DRAW_H
 
 
//#include "font.h"


#define SMALL_FONT          0
#define BIG_FONT            1

#define PAGE_LINES          200


//printer API
unsigned char PrnInit(void);

//void PrnDoubleWidth(int AscDoubleWidth, int LocalDoubleWidth);

//void PrnDoubleHeight(int AscDoubleHeight, int LocalDoubleHeight);
//void PrnAttrSet(int Reverse);
#if 0
void PrnFontSet(uchar Ascii, uchar Locale);
#else
void PrnFontSet(char *name);
#endif
// 设置行列间距
void PrnSpaceSet(unsigned char x, unsigned char y);

// 设置字符间距
void  PrnSetSpCh(unsigned char x);
// 设置行间距
void  PrnSetSpLi(unsigned char y);

void PrnLeftIndent(unsigned short x);


void PrnStep(short pixel);

unsigned int s_PrnStr(unsigned char *str);
void  PrnDrawClrBuf(void);
#endif

