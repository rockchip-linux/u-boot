/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	timer.h
Desc 	:	TIMER内部寄存器定义
Author 	:  	yangkai
Date 	:	2008-11-11
Notes 	:   

********************************************************************/
#ifdef DRIVERS_TIMER
#ifndef _TIMER_H
#define _TIMER_H
/********************************************************************
**                            宏定义                                *
********************************************************************/

/********************************************************************
**                          结构定义                                *
********************************************************************/
typedef volatile struct tagTIMER_REG
{
    uint32 Timer1LoadCount;     // Load Count Register
    uint32 Timer1CurrentValue;  // Current Value Register
    uint32 Timer1ControlReg;    // Control Register
    uint32 Timer1EOI;           // End-of-Interrupt Register
    uint32 Timer1IntStatus;     // Interrupt Status Register
    uint32 Timer2LoadCount;
    uint32 Timer2CurrentValue;
    uint32 Timer2ControlReg;
    uint32 Timer2EOI;
    uint32 Timer2IntStatus;
    uint32 Timer3LoadCount;
    uint32 Timer3CurrentValue;
    uint32 Timer3ControlReg;
    uint32 Timer3EOI;
    uint32 Timer3IntStatus;
    uint32 Reserved[(0xa0 - 0x3c)/4];
    uint32 TimersIntStatus;     // Interrupt Status Register
    uint32 TimersEOI;           // End-of-Interrupt Register
    uint32 TimersRawIntStatus;  // Raw Interrupt Status Register
}TIMER_REG, *pTIMER_REG;


/********************************************************************
**                          变量定义                                *
********************************************************************/
#undef EXT
#ifdef IN_TIMER
    #define EXT
#else    
    #define EXT extern
#endif    

EXT pTIMER_REG  g_timerReg;     //TIMER寄存器结构体指针
EXT pFunc       g_timerIRQ[3];  //保存TIMER中断回调函数
EXT uint32      g_timerTick[3]; //保存TIMER定时毫秒数

/********************************************************************
**                          结构定义                                *
********************************************************************/
typedef enum _TIMER_NUM
{
    TIMER1 = 0,
    TIMER2,
    TIMER3,
    TIMER_MAX
}eTIMER_NUM;

/*
typedef enum _TIMER_MODE
{
    TIMER_FREE_RUNNING = 0,
    TIMER_USER_DEFINED,
    TIMER_MODE_MAX
}eTIMER_MODE;
*/

/********************************************************************
**                          变量定义                                *
********************************************************************/

/********************************************************************
**                      对外函数接口声明                            *
********************************************************************/
extern uint32 TimerStart(eTIMER_NUM timerNum,              //启动TIMER
                            uint32 msTick, 
                            pFunc callBack);
extern uint32 TimerStop(eTIMER_NUM timerNum);              //停止TIMER
extern uint32 TimerGetCount(eTIMER_NUM timerNum);          //查询当前计数值
extern uint32 TimerAdjustCount(void);           //APB CLK改变时调整计数初值

extern uint32 TimerMask(eTIMER_NUM timerNum);              //MASK Timer中断
extern uint32 TimerUnmask(eTIMER_NUM timerNum);            //UNMASK Timer中断


/********************************************************************
**                          表格定义                                *
********************************************************************/
#endif
#endif
