/*   Copyright (C) 2007 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File :  hw_common.h
Desc :  IO方式操作寄存器的宏定义

Author : huangxinyu
Date :       2007-05-30
Notes :

$Log: hw_common.h,v $
Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:04  Administrator
*** empty log message ***

Revision 1.4  2007/10/15 09:04:01  Huangxinyu
根据RK27提交修改driver

Revision 1.3  2007/10/08 02:38:41  Lingzhaojun
添加版本自动注释脚本


*********************************************************************/
#ifndef _HW_COMMON_H
#define _HW_COMMON_H

#include "typedef.h"

#define BIT0     1<<0
#define BIT1     1<<1
#define BIT2     1<<2
#define BIT3     1<<3
#define BIT4     1<<4
#define BIT5     1<<5
#define BIT6     1<<6
#define BIT7     1<<7
#define BIT8     1<<8
#define BIT9     1<<9
#define BIT10    1<<10
#define BIT11    1<<11
#define BIT12    1<<12
#define BIT13    1<<13
#define BIT14    1<<14
#define BIT15    1<<15
#define BIT16    1<<16
#define BIT17    1<<17
#define BIT18    1<<18
#define BIT19    1<<19
#define BIT20    1<<20
#define BIT21    1<<21
#define BIT22    1<<22
#define BIT23    1<<23
#define BIT24    1<<24
#define BIT25    1<<25
#define BIT26    1<<26
#define BIT27    1<<27
#define BIT28    1<<28
#define BIT29    1<<29
#define BIT30    1<<30
#define BIT31    1<<31

#ifndef FALSE
#define     FALSE   0
#endif

#ifndef TRUE
#define     TRUE    (!FALSE)
#endif

#ifndef NULL
#define NULL    0
#endif

//#define  RKabs(x)             ((x)>0?(x):-(x))
#define  hwMAX(a,b)           (int)(((int)a>=(int)b)?a:b)
#define  HwMIN(a,b)           (int)(((int)a<=(int)b)?a:b)
#define  BITMASK(nbits)     ((1<<nbits)-1)
#ifndef ARRSIZE /* 取得数组的元素个数 */
#define ARRSIZE( array ) ( sizeof(array)/sizeof(array[0]) )
#endif

#define ReadReg8(addr)                      (*(REG8 *)(addr))
//#define ReadReg8(addr)                     (DummyWriteReg =0xfc,(*(REG8 *)(addr)))
#define WriteReg8(addr, data)               (*(REG8  *)(addr) = data)
#define SetRegBits8(addr, databits)         WriteReg8(addr, ReadReg8(addr)|(databits))
#define ClrRegBits8(addr, databits)         WriteReg8(addr, ReadReg8(addr)&~(databits))
#define SetRegBit8(addr,bit)             WriteReg8(addr,(ReadReg8(addr)|(1<<bit)))
#define ClrRegBit8(addr,bit)             WriteReg8(addr,(ReadReg8(addr)&(~(1<<bit))))
#define GetRegBit8(addr,bit)             (ReadReg8(addr)&(1<<bit))
#define MaskRegBits8(addr, y, z)            WriteReg8(addr, (ReadReg8(addr)&~(y))|(z))

#define ReadReg16(addr)                     (*(REG16 *)(addr))
//#define ReadReg16(addr)                     (DummyWriteReg =0xfc,(*(REG16 *)(addr)))
#define WriteReg16(addr, data)              (*(REG16 *)(addr) = data)
#define SetRegBits16(addr, databit)         WriteReg16(addr, ReadReg16(addr)|(databit))
#define ClrRegBits16(addr, databit)         WriteReg16(addr, ReadReg16(addr)&~(databit))
#define SetRegBit16(addr,bit)             WriteReg16(addr,(ReadReg16(addr)|(1<<bit)))
#define ClrRegBit16(addr,bit)             WriteReg16(addr,(ReadReg16(addr)&(~(1<<bit))))
#define GetRegBit16(addr,bit)             (ReadReg16(addr)&(1<<bit))
#define MaskRegBits16(addr, y, z)           WriteReg16(addr, (ReadReg16(addr)&~(y))|(z))


#define ReadReg32(addr)                     (*(REG32 *)(addr))
#define WriteReg32(addr, data)              (*(REG32 *)(addr) = data)
#define SetRegBits32(addr, databit)         WriteReg32(addr, ReadReg32(addr)|(databit))
#define ClrRegBits32(addr, databit)         WriteReg32(addr, ReadReg32(addr)&~(databit))
#define SetRegBit32(addr,bit)             WriteReg32(addr,(ReadReg32(addr)|(1<<bit)))
#define ClrRegBit32(addr,bit)             WriteReg32(addr,(ReadReg32(addr)&(~(1<<bit))))
#define GetRegBit32(addr,bit)             (ReadReg32(addr)&(1<<bit))
#define MaskRegBits32(addr, y, z)           WriteReg32(addr, (ReadReg32(addr)&~(y))|(z))
typedef union
{
    uint32 A;
    uint32* B;
    void* C;
}VariableParam;


/**************************************************************************
* 函数名称:  delay_nops
* 函数描述:  软件延时
* 入口参数:  计数值
* 出口参数:  无
* 返回值:       无
* 注释:
***************************************************************************/
__inline void delay_nops(UINT32 count)
{
    while (--count) {}
}


#endif /* _HW_COMMON_H */
