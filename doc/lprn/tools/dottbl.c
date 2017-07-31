#include <stdio.h>
#include <stdlib.h>

/*---------------------------------------------*/
static unsigned char s_kdotnum[256];
void make_dot_tbl(void)
{
	int ii,jj,nn;
	unsigned char tch;
	for(ii=0;ii<256;ii++)
	{
		tch=ii;
		for(jj=nn=0;(jj<8)&&(tch);jj++)
		{
			if(tch&1) nn++;
			tch>>=1;
		}
		s_kdotnum[ii]=nn;
	}
}

void main(void)
{
	int i,j;
	make_dot_tbl();
	for(i=0;i<16;i++)
	{
		printf("  ");
		for(j=0;j<16;j++){
			printf("%d,", s_kdotnum[i*16+j]);	
		}
		printf("\n");
	}
}
/*-------------------------------------------------------------*/
/*--- end                                       randtable.c ---*/
/*-------------------------------------------------------------*/

