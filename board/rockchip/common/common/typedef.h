
#ifndef	__TYPEDEF_H
#define __TYPEDEF_H
#include <common.h>

typedef volatile unsigned int       REG32;
typedef volatile unsigned short  	REG16;
typedef volatile unsigned char  	REG8;

typedef int BOOLEAN;
typedef BOOLEAN BOOL;

#define	FALSE	0
#define TRUE    (!FALSE)

#ifndef NULL
#define	NULL	0
#endif

#define OK                  0
#define ERROR               !0

//typedef char * va_list; 


typedef unsigned long           uint32;
typedef unsigned long			UINT32;
typedef unsigned short			UINT16;
typedef unsigned char 			UINT8;
typedef long					INT32;
typedef short					INT16;
typedef char					INT8;

typedef unsigned char			INT8U;
typedef signed	char   			INT8S;
typedef unsigned short			INT16U;
typedef signed	short  			INT16S;
typedef int 	  				INT32S;
typedef unsigned long   		INT32U;

typedef unsigned long	L32U;
typedef signed	long  L32S;

typedef unsigned char	BYTE;
typedef unsigned long ULONG;



typedef volatile unsigned int  data_t;
typedef volatile unsigned int* addr_t;

typedef 	void (*pFunc)(void);	//定义函数指针, 用于调用绝对地址

typedef		unsigned char		uint8;
typedef		signed char		    int8;
typedef		unsigned short	    uint16;
typedef		signed short	    int16;
typedef		signed long			int32;
typedef		unsigned long long	uint64;
typedef		signed long long	int64;

#define DEFINE__U32

#define ReadReg32(addr)                     (*(volatile uint32 *)(addr))
#define WriteReg32(addr, data)              (*(volatile uint32 *)(addr) = data)
#define MaskRegBits32(addr, y, z)           WriteReg32(addr, (ReadReg32(addr)&~(y))|(z))

#endif  /*__TYPEDEF_H */
