
#include "config.h"

/***************************************************************************
函数描述:
调用函数:
***************************************************************************/
void * ftl_memset(void *s, int c, unsigned int  n)
{
    return ((void*)memset(s, c, n));
}

/***************************************************************************
函数描述:
调用函数:
***************************************************************************/
void* ftl_memcpy(void* pvTo, const void* pvForm, unsigned int  size)
{
    if(((int)pvTo & 0x3)||((int)pvForm & 0x3))
    {
        int i;
        char * pTo = (char *)pvTo;
        char * pForm = (char *)pvForm;
        for(i=0;i<size;i++)
        {
          *pTo++ = *pForm++;
        }
        return 0;
    }
    return ((void*)memcpy(pvTo, pvForm, size));
}

/***************************************************************************
函数描述:
调用函数:
***************************************************************************/
int ftl_memcmp(void *str1, void *str2, unsigned int count)
{
    return (memcmp(str1, str2, count));
}

#if 0
void BIN2HEX(uint32 data, uint8 * p_hexstr)
{
    uint32 index   = 0;
    uint8 int_val;
    uint32 hex_len = 8;
    uint8 * hexTbl="0123456789ABCDEF";
    for (index = 0; index < 8; index++)
    {
        int_val = data>>(7-index) &0xF;
        *p_hexstr++ = hexTbl[int_val];
    }
}
#endif


