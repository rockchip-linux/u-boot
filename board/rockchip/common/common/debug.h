/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	debug.h
Desc 	:	定义驱动中共用代码
Author 	:  	yangkai
Date 	:	2008-11-12
Notes 	:   

********************************************************************/
#ifndef _DEBUG_H
#define _DEBUG_H


//#define DEBUG_DRIVER//异常检查开关
//#define DEBUG_MSG   //消息输出开关

#ifdef DEBUG_DRIVER

#define Assert(cond,msg,num)                \
    do{                                     \
        if (!(cond))                        \
        {                                   \
            printf("%-45s %12d  %s  %d\n", msg, num, __FILE__,__LINE__);\
            abort();                        \
        }                                   \
    }while(0);

#else

#define Assert(cond,msg,num)

#endif


#ifdef  DEBUG_MSG

#define PRINTF(...)     \
    do\
    {\
        printf(__VA_ARGS__);\
        printf("\n");\
    }while(0);
    
//    printf("  %s:%d\n",__FILE__,__LINE__);

#else

#define PRINTF(...)

#endif

#endif
