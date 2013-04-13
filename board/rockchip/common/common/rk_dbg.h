#ifndef _RK_DBG_H_
#define _RK_DBG_H_

enum{
	PRINT_NO=0,	// don't print
	PRINT_ERR,	// only print error info
	PRINT_WARN,	// only print warning&error info
	PRINT_INFO,	// print normal info
	PRINT_DBG,	// print dbg info
	PRINT_LDBG	// print max info
};

extern int g_dbgLevel;
extern int write_print_buffer( const char *fmt, ... );

/*
#ifdef DEBUG
	#define RkPrintf(...) \
	do \
	{ \
	    write_print_buffer(__VA_ARGS__); \
	    printf("[" __func__ "]: " __VA_ARGS__); \
	}while(0)
#else
	#define RkPrintf(...)
#endif
*/

#define PRINT(level,format,...) \
do {\
	if(level <= g_dbgLevel){ \
	    write_print_buffer("[" __func__ "]: " ,__VA_ARGS__); \
	    printf("[" __func__ "]: " ,__VA_ARGS__); \
	} \
}while(0)

#define PRINT_E(...) printf(__VA_ARGS__)//PRINT(PRINT_ERR,format, __VA_ARGS__)
#define PRINT_W(...) //printf( __VA_ARGS__)//PRINT(PRINT_WARN,format, __VA_ARGS__)
#define PRINT_I(...) //printf(__VA_ARGS__)//PRINT(PRINT_INFO,format, __VA_ARGS__)
#define PRINT_D(...) //printf(__VA_ARGS__)//PRINT(PRINT_DBG,format,  __VA_ARGS__)
#define PRINT_A(...) //printf(__VA_ARGS__)//PRINT(PRINT_LDBG,format, __VA_ARGS__)
#define RkPrintf(...)  printf(__VA_ARGS__)

#ifdef DEBUG
    extern void dbg_dump(unsigned char* data, unsigned long len);
    
	#define DUMPDATA(data, len) \
	do {\
		if(g_dbgLevel >= PRINT_LDBG) dbg_dump(data, len); \
	}while(0)
#else
	#define DUMPDATA(data, len)
#endif

#endif

