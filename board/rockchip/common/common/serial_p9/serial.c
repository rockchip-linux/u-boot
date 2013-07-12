
/*********************************************************************
 INCLUDE FILES	
*********************************************************************/
#include "../typedef.h"
#include "serial.h"
#include "../debug.h"
#include <stdarg.h>

void serial_init (void)
{
	UartInit();         
}

/**************************************************************************
* 函数描述: Debug输出字符
* 入口参数: ch,stream
* 出口参数: 无
* 返回值:   0 -- 输出成功
*           非0 -- 输出失败
* 说明:     C库printf内部调用
***************************************************************************/
int fputc(int ch, void *stream)
{
   if (ch == '\n')
		UartWriteChar('\r');
	UartWriteChar(ch);

   return 0;
}
#if 0
void rknand_print_hex(uint8 *s,void * buf,uint32 width,uint32 len)
{
    uint32 i,j,count;
    char * p8 = (char *) buf;
    uint32 * p32 =(uint32 *) buf;
    j = 0;
    for(i=0;i<len;i++)
    {
        if(j == 0)
        {
            printf("%s 0x%x ",s,&p32[i]);
        }
        if(width == 1)
            printf("%2x ",p32[i]);
        else
            printf("%2x ",p8[i]);
            
        if(++j>=16)
        {
           j=0;
           printf("\n","");
        }
    }
    printf("\n","");
}
#endif
