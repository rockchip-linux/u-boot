#define     IN_CRC32
#include    "../../armlinux/config.h"

//#pragma arm section code = "LOADER2"

/******************************************************************************/
#define CRC16_CCITT         0x1021		// CRC 多项表达式.
#define CRC32_CRC32         0x04C10DB7
////////////////////////////////////////////////////////////////////////////////////////
//#include "crctable.c"
/******************************************************************************/
/******************************************************************************/
// 注意：因最高位一定为"1"，故略去 
//const INT16U cnCRC_16 = 0x8005; 
// CRC-16 = X16 + X15 + X2 + X0 
//const INT16U cnCRC_CCITT = 0x1021; 
// CRC-CCITT = X16 + X12 + X5 + X0，据说这个 16 位 CRC 多项式比上一个要好 

//const INT32U cnCRC_32 = 0x04C10DB7; 
// CRC-32 = X32 + X26 + X23 + X22 + X16 + X11 + X10 + X8 + X7 + X5 + X4 + X2 + X1 + X0 

//unsigned long Table_CRC[256]; // CRC 表 

extern unsigned long gTable_Crc32[256];

// 计算 32 位 CRC-32 值 
unsigned long CRC_32( unsigned char * aData, unsigned long aSize ) 
{ 
    unsigned long i; 
    unsigned long nAccum = 0; 
    //unsigned long startTime;
    //unsigned long endTime;
    //startTime = RkldTimerGetTick();
    for ( i = 0; i < aSize; i++ ) 
        nAccum = ( nAccum << 8 ) ^ gTable_Crc32[( nAccum >> 24 ) ^ *aData++]; 
    //endTime = RkldTimerGetTick();
    //printf ("CRC_32 times  %d ,aSize = %x\n", endTime - startTime,aSize);
#if 0
    startTime = RkldTimerGetTick();
    ftl_memcpy((void*)0x64000000,(void*)0x63000000,0x1000000);
    endTime = RkldTimerGetTick();
    printf ("memcpy  times  %d ,aSize = %x\n", endTime - startTime,aSize);
    
    startTime = RkldTimerGetTick();
    ftl_memcpy((void*)0x64000000,(void*)0x62000000,0x1000000);
    endTime = RkldTimerGetTick();
    printf ("memcpy times  %d ,aSize = %x\n", endTime - startTime,aSize);

{
    uint32 *pfrom =  (uint32 *)0x62000000;
    uint32 *pto =  (uint32 *)0x64000000;
    startTime = RkldTimerGetTick();
    for(i=0;i<0x1000000/4;i++)
    {
        *pto++ = *pfrom++ ;
    }
    endTime = RkldTimerGetTick();
    printf ("memcpy times  %d ,aSize = %x\n", endTime - startTime,aSize);

    pfrom =  (uint32 *)0x63000000;
    pto =  (uint32 *)0x64000000;
    startTime = RkldTimerGetTick();
    for(i=0;i<0x1000000/4;i++)
    {
        *pto++ = *pfrom++ ;
    }
    endTime = RkldTimerGetTick();
    printf ("memcpy times  %d ,aSize = %x\n", endTime - startTime,aSize);
        
}
#endif
    return nAccum; 
} 


// 检查 BUFFER CRC 校验是否 有错误.假设 最后 4 个 BYTE 是 CRC32 的校验值.
// CMY:
// 若CRC校验失败，返回0
// 若CRC校验成功，返回CRC值
uint32 CRC_32CheckBuffer( unsigned char * aData, unsigned long aSize )
{
    uint32 crc = 0;
	int i=0;
    //return 1;
    if( aSize <= 4 )
    {
        return 0;
    }
    aSize -= 4;

	// CMY:考虑到4Bytes对齐
	for(i=3; i>=0; i--)
		crc = (crc<<8)+(*(aData+aSize+i));

    if( CRC_32( aData , aSize ) == crc )
        return crc;
	
    return 0;
}

/////////////////////////////////////////////////////////////
//#pragma arm section code
