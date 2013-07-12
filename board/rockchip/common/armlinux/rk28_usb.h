/*
 * Gadget Driver for rockchip usb
 *
 * Copyright (C) 2009 Rochchip, Inc.
 * Author: Hsl
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/freezer.h>
#include <linux/kthread.h>

#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/tty.h>
#include <linux/console.h>
#include <linux/tty_flip.h>  
#include <linux/mm.h>  
#include <linux/file.h>  
#include <linux/dma-mapping.h>

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include<asm/types.h>
#include<asm/ptrace.h>
#include<asm/irq_regs.h>

#include<asm/arch/gpio.h>
//#define RK28_PRINT
#include<asm/arch/rk28_debug.h>
#include<asm/arch/rk28_msc_ext.h>

#endif

#define RKUSB_BUFFER_SIZE               512
#define RW_BUFFER_NUM                     2

#define RKUSB_VID                       0x071b
#define RKUSB_PID                       0x322f

#define DMA_ADDR_INVALID	(~(dma_addr_t)0)

/*******************************************************************
¹Ì¼þÉý¼¶ÃüÁî¼¯
*******************************************************************/
#define   RKUSB_CMD_BASE                              0Xf0
#define 	K_FW_SDRAM_READ_10                      (RKUSB_CMD_BASE+0)    //XXX: read SDRAM at special stats such as panic.
#define 	K_FW_SDRAM_WRITE_10                     (RKUSB_CMD_BASE+1)
#define   K_FW_SESSION			      (RKUSB_CMD_BASE+2)// ADD BY HSL.
/* new command */
#define   K_FW_TRACE				 (RKUSB_CMD_BASE+3)  // for trace proc ,get all register for jtag.
#define   K_FW_GETLOG				 (RKUSB_CMD_BASE+4)  // get log info .
#define   K_FW_SETLOG				 (RKUSB_CMD_BASE+5)  // set log command.
#define   K_FW_XFERFILE				(RKUSB_CMD_BASE+6)  //XXX: read . write file .
#define   K_FW_FUNCALL				(RKUSB_CMD_BASE+7)  //XXX: call functions in irq .
#define   K_FW_GETSYMB				(RKUSB_CMD_BASE+8)  //XXX: get system symbol,for printk stack .
#define   K_FW_TASKFUN				(RKUSB_CMD_BASE+9)  //XXX: thing do at task .

#define   K_FW_GETRESULT				(RKUSB_CMD_BASE+0Xe)  /* for get last cmd result and status */
#define   K_FW_USBCMD				(RKUSB_CMD_BASE+0xf)  /* cmd for usb,fireware update */
#define   RKUSB_CMD_END                                                    K_FW_USBCMD


#define FW_TRACE				K_FW_TRACE
#define FW_GETLOG			K_FW_GETLOG
#define FW_SETLOG			K_FW_SETLOG
#define FW_XFERFILE			K_FW_XFERFILE
#define FW_FUNCALL			K_FW_FUNCALL
#define FW_GETSYMS			K_FW_GETSYMB
#define FW_USBCMD			K_FW_USBCMD

/*
 * micro foe lun , func , len , offset .
 */
#define DEV_LUN( dev )                          ( dev->cb.mylun )
#define DEV_CMD( dev )                          ( dev->cb.Code )
#define DEV_FUNC( dev )                        ( dev->cb.subCode )
#define DEV_OFFSET( dev )                     ( dev->cb.Lba )
#define DEV_LENGTH( dev )                     ( dev->cb.DataTransferLength )
#define DEV_PID( dev )                            (dev->cb.pid)

#define LUN_KERNEL                              0       
#define LUN_ANDROID_MAIN                1
#define LUN_SHELL                                 2
#define LUN_ANDROID_EVENT              3

/* CMD K_FW_GETLOG */
#define FUNC_LOG_GETLUNS                 1         /* get log num & log struct size */
#define FUNC_LOG_INIT                         2       /* init log , MUST CALL  */
#define FUNC_LOG_GETSTRUCT             3      /* get all log struct info */
#define FUNC_LOG_OFFLEN                    4      /* get log offset and len */
#define FUNC_LOG_UNINIT                     5       /* deinit log , MUST CALL for shell */



/* CMD K_FW_XFERFILE */
#define FUNC_RFILE_INFO                     0   /* file path , file size */
#define FUNC_RFILE_DATA                     1   /* file data */
#define FUNC_WFILE_INFO                     2   /* not use,at file path */
#define FUNC_WFILE_DATA                    3
#define FUNC_XFER_FILE_PATH            4   /* file path/r/w */


/* CMD K_FW_USBCMD */
#define FUNC_UCMD_DEVREADY                0 /* device ready or not */
#define FUNC_UCMD_DISCONNECTMSC     1
#define FUNC_UCMD_SYSTEMINFO            2 /* get system info,string */
#define FUNC_UCMD_BUFSIZE                    3 /* get buf size(max transfer len one time ) */


#define FUNC_UCMD_RESTART                   0XE1 /* restart machine,to loader or normal,lun for type */
#define FUNC_UCMD_GETVERSION             0XE2 /* get loader version,system version ,linux version */
#define FUNC_UCMD_GETCHIPINFO           0XE3 /* chip info,size & string */
#define FUNC_UCMD_GETSN                       0XE4 /* get serial */

/* lun for FUNC_UCMD_GETVERSION 
 * format: int size , char* version info.
*/
#define LUN_USCM_VER_LOADER              0
#define LUN_USCM_VER_FIRMWARE         1
#define LUN_USCM_VER_KERNEL               2
#define LUN_USCM_VER_OPENCORE          3
#define LUN_USCM_VER_ANDROID            LUN_USCM_VER_FIRMWARE

/* for command FW_GETSYMS */
#define FUNC_GSYM_KERNEL                0 /*  get kernel symbols */
#define FUNC_GSYM_GETTASKS            1 /* get all task info(the field we need) */
#define FUNC_GSYM_GETTASKVM         2 /* get mm,vm of task by pid , 0 for current */

/* for cmd FW_TRACE */
#define FUNC_TRACE_BREAKSET          0  /* set break point */
#define FUNC_TRACE_BREAKCLEAR     1  /* clear break point */
#define FUNC_TRACE_GOON                  2 /* go on after break */
#define FUNC_TRACE_GETREG               3 /* get current regs & user regs */
//#define FUNC_TRACE_GETDATA             4 /* get data & code & stack */
#define FUNC_TRACE_INQUIRY             5 /* check weather run at breakpoint now */
#define FUNC_TRACE_CALLFUNC           6 /* call function at break task env */
//#define FUNC_TRACE_WRITEADDR        7/* write sdram by addr */
#define FUNC_TRACE_SETREG               8 /* at break point ,change reg r0-r15,cpsr,spsr */
#define FUNC_TRACE_ARMHALT            9 /* STOP current running */

/* func for cmd K_FW_TASKFUN */
#define FUNC_TASKF_PING                      0  /* task ping */
//#define FUNC_TASKF_CALLFUN               1  /* CALL FUNCTION AT TASK ENV,USE K_FW_FUNCALL. */
//#define FUNC_TASKF_GETDATA               2  /* use K_FW_SDRAM_READ_10+pid */
//#define FUNC_TASKF_SETDATA               3  /* use K_FW_SDRAM_WRITE_10+pid */
#define FUNC_TASKF_TASKCMD              4 /* get cmdline of task by pid , 0 for current */

struct log_buffer  {
        char            name[20];
        char           *va_start;
        int             total_len;
        int             property; /* bit0:readable , bit1:writable bit 31:ready to read , bit 30:ready to write */
        
        int(*getlog)(char** , int* , int* );
        int(*getlog_start_length)(int* , int*);
        int(*setlog)(struct log_buffer*, char* , int );
        
        int             len;
        int             offset;
        void *         private_data;
        
};


#ifdef __KERNEL__
/* Command Block Wrapper */
struct rkusb_cb_wrap {
	__le32	Signature;		/* Contains 'USBC' */
	u32	Tag;			/* Unique per command id */
	__le32	DataTransferLength;	/* Size of the data */
	u8	Flags;			/* Direction in bit 7 */
	u8	Lun;			/* LUN (normally 0) */
	u8	Length;			/* Of the CDB, <= MAX_COMMAND_SIZE */
	//u8	CDB[16];		/* Command Data Block */
	u8          Code;
             u8          subCode;
             u32        Lba;
             u32        pid;
             u8          mylun;
             u8          direct;
             u8          Reseved2[4];
} __attribute__((packed)) ;
#define DEV_DATA_IN( dev )              ( dev->cb.Flags &0x80 )

/* Command Status Wrapper */
struct rkusb_cs_wrap {
	__le32	Signature;		/* Should = 'USBS' */
	u32	Tag;			/* Same as original command */
	__le32	Residue;		/* Amount not transferred */
	u8	Status;			/* See below */
} __attribute__((packed)) ;

/* Command Status Wrapper */
struct rkusb_result {
	u32          Signature;		/* Should = 'LSHR' */
	u8	Code;			/* Same as original command */
	u8	subCode;			/* Same as original sub command */
	u8	Lun;			/* Same as original lun */
	u8	Status;			/* See below */
	u32	retvalue;		/* the value to be return */
} __attribute__((packed)) ;
#define RKUSB_RESULT_SIG		0x4C534852	/* Spells for usb cmd result */
#define RKUSB_RESULT_LEN                sizeof( struct rkusb_result )

extern char _text[], _etext[], __data_start[], _end[], 
        __bss_start[], _edata[]  , _stext[] , __start_rodata[];
extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const u8 kallsyms_names[] __attribute__((weak));

/* tell the compiler that the count isn't in the small data section if the arch
 * has one (eg: FRV)
 */
extern const unsigned long kallsyms_num_syms
__attribute__((weak, section(".rodata")));

extern const u8 kallsyms_token_table[] __attribute__((weak));
extern const u16 kallsyms_token_index[] __attribute__((weak));

extern const unsigned long kallsyms_markers[] __attribute__((weak));


#define RKUSB_BULK_CB_WRAP_LEN	31
#define RKUSB_BULK_CB_SIG		0x43425355	/* Spells out USBC */
#define RKUSB_BULK_IN_FLAG	0x80

#define RKUSB_BULK_CS_WRAP_LEN	13
#define RKUSB_BULK_CS_SIG		0x53425355	/* Spells out 'USBS' */
#define RKUSB_STATUS_PASS		0
#define RKUSB_STATUS_FAIL		1
#define RKUSB_STATUS_PHASE_ERROR	2

#define RKUSB_PHASE_CMD                         0
#define RKUSB_PHASE_DATAOUT                  1
#define RKUSB_PHASE_DATAIN                     2
#define RKUSB_PHASE_DATAOUT_CSW          3
#define RKUSB_PHASE_DATAIN_CSW             4

#define LOG_PROT_READ                   (1<<0)
#define LOG_PROT_WRITE                  (1<<1)

/* tmp value */
#define LOG_PROT_MAYWRITE                  (1<<16)

struct rkusb_dev;

#define RKUSB_CB_OK_NONE               0
#define RKUSB_CB_OK_CSW                 1
#define RKUSB_CB_FAILD                      -1
#define RKUSB_CB_FAILD_CSW           -2

/* 20100126,HSL@RK,usb adb instead,only support printk log here!*/
#define LOG_NUM                                 1 

typedef int (*data_cb )( struct rkusb_dev * );  /* return 0:OK ,CSW. 1:OK NONE,else : error */
struct rw_buffer;
typedef int (*buffer_cb )( struct rw_buffer* );

struct rkusb_dev {
             //struct usb_function function;
             //spinlock_t lock;
             int phase;
             int luns;          /* 0: kernel log ,read only,1:android log,read only,2:shell ,write only */
             struct log_buffer       log[LOG_NUM];
             struct rkusb_cb_wrap       cb;
             struct rkusb_cs_wrap       cs;
             struct rkusb_result            cr;
             
             struct usb_ep *ep_in;
             struct usb_request *req_in;
             
             struct usb_ep *ep_out;
             struct usb_request *req_out;

             /* 20100203,HSL@RK,when use 64K buffer,no need xfer_cb.one time transfer !*/
            //data_cb  xfer_cb;
            //unsigned int          xfer_max_size;        /* for out size buffer set */

            data_cb  final_cb;
            int         rkusb_enable;

            struct rkusb_dev_fsg*    fsg_dev;
            //struct task_struct	*thread_task;
            struct task_struct	*slave_task;
            void                *private_tmp;

            char                *in_req_buf;
            char                *buf;
            int                   buf_length;
            int                   buf_filled;
            int                   buf_nfill;
};


#define __rkusb_clear_cb( dev )  do{ dev->fsg_dev->usb_in = NULL;\
                                dev->fsg_dev->task_cb = NULL;}while( 0 )
#define __rkusb_set_epin_cb( dev , cb )  do{ dev->fsg_dev->usb_in = cb; }while(0)
#define __rkusb_set_task_cb( dev , cb )  do{ dev->fsg_dev->task_cb = cb; }while(0)

#define rkusb_wakeup_thread( dev )              do{dev->fsg_dev->wake(dev->fsg_dev->fsg);\
                                                                        __rkusb_set_task_cb(dev,rkusb_do_thread_cmd);}while(0)
                                                                        
#define rkusb_sleep_thread( dev )                  dev->fsg_dev->sleep(dev->fsg_dev->fsg)
#define rkusb_handed_epout( dev )                 (dev->fsg_dev->usb_out)

#define KERNEL_STACK_SIZE		(THREAD_SIZE) 
#define HANDLE          int
#else
typedef unsigned __int64	u64;
typedef int		pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#define offset_of(TYPE, MEMBER) ((unsigned int) &((TYPE *)0)->MEMBER)

struct pt_regs {
	long uregs[18];
};

#define ARM_cpsr	                16
#define ARM_pc		15
#define ARM_lr		14
#define ARM_sp		13
#define ARM_ip		12
#define ARM_fp		11
#define ARM_r10		10
#define ARM_r9		9
#define ARM_r8		8
#define ARM_r7		7
#define ARM_r6		6
#define ARM_r5		5
#define ARM_r4		4
#define ARM_r3		3
#define ARM_r2		2
#define ARM_r1		1
#define ARM_r0		0
#define ARM_ORIG_r0	17

#define KERNEL_STACK_SIZE		(8192) 
#define TYPE_ARGV(type,n)                          (argc>=((n)+1)?(type)argv[n]:(type)NULL)
#define ARGV_ISTRING( n )                          (argv[n]>0x10000)
#define ISTRING( value )                                (value>0x10000 && value < 0x10000000)

#endif          /* define __KERNEL__  */

#define STACK_LENGTH( sp )              (KERNEL_STACK_SIZE-(sp&(KERNEL_STACK_SIZE-1)))
#define KSYM_NAME_LEN           128
#define KSYM_SYMBOL_LEN	  128
#define TASK_CMD_LEN              1024

typedef struct __kernel_symbol{
	int			size;	
	char		*_stext;
	char		*_text;
	char		*_etext;
	char		*_data;
	char		*_edata;
	char		*__bss_start;
	char		*_end;

	int		total_syms_size;
	unsigned char	*kallsyms_start;	/* for kernel & pc */
	unsigned long 	*_kallsyms_addresses;
	unsigned long 	_kallsyms_num_syms;
	unsigned char	*_kallsyms_names;
	unsigned long 	*_kallsyms_markers;
	unsigned char	*_kallsyms_token_table;
	unsigned short          *_kallsyms_token_index;
} KERNEL_SYMBOL;

#ifndef NR_OPEN_DEFAULT
#define NR_OPEN_DEFAULT		32
#define TASK_COMM_LEN		16
#endif

typedef struct __task_info{
        int     struct_size;
        volatile long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
        void *stack;
        int prio, static_prio, normal_prio;
        void *sched_class;
        u64			sum_exec_runtime; /* ns-->to us */
        void *mm, *active_mm;
        pid_t pid;
        unsigned int rt_priority;
        unsigned long utime, stime;
        char comm[TASK_COMM_LEN];
        void * fd_array[NR_OPEN_DEFAULT]; /* for open files */
        //char    cmd[TASK_CMD_LEN];

        pid_t   tgid;
        u64     start_time; 
        uid_t uid,euid,suid,fsuid;
        gid_t gid,egid,sgid,fsgid;
} TASK_INFO;
#define TASK_INFO_SIZE                  offset_of(TASK_INFO,cmd);

struct __task_all {
        int     size;
        int     ti_size;
        int     task_total_num;
        int     task_num;
        struct __task_info       *start_ts;
        u64	now;                            /* for cacle task run time */
} ;
typedef struct __task_all  ALL_TASK;

typedef struct __task_vam_info{
        unsigned long start;		
        unsigned long end;
        unsigned long flags;
        void               *file;
        char               mape_filename[48];
} TASK_VMA_INFO ;

/* vma flags */
#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE		0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008
#define VM_GROWSDOWN	0x00000100	/* general info on the segment */


typedef  struct __task_vam{
        int                  size;
        unsigned long start_code, end_code, start_data, end_data;
        unsigned long start_brk, brk, start_stack;
        unsigned long arg_start, arg_end, env_start, env_end;
        int                  flag;      /* bit0:1 mm exit */
        int                  pid;
        int             vi_size;
        int             vi_tolnum;      /* total vma num */
        int             vi_xfer;           /* transfer vma num */
        TASK_VMA_INFO *vi;       
} TASK_VAM;

typedef struct __vi_map {
        HANDLE          handle;
        char                *map_buf;
        unsigned int     file_size;
        void                 *elf_handle;
        TASK_VMA_INFO *tvi;
} VI_MAP;


typedef struct __vm_map {
        VI_MAP                  *vmp;
        int                           map_num;
        TASK_VAM              *tvm;
} VM_MAP;

#define MAX_BREAKS                      3

typedef struct __break_regs {
	int		size;
	struct pt_regs  	*user_regs;     /* system call */
	struct pt_regs  	*irq_regs;       /* kernel state at irq */
	struct pt_regs  	*expt_regs;     /* user except reg */
	struct pt_regs	*kernel_regs;  /* break point kernel reg */
	int                             pid;                   /* break or panic task pid */
	int                      	 catch_type; /* may change every time 1:fault,0:break,2:other */
	unsigned long            copy_kernel_stack; /* for bake of irq stack */
} BREAK_REGS;

/* for application , kernel use rk28_dump_task function 
*  as struct cpu_context_save.
*/
typedef struct __cpu_context_save {
	unsigned long	r4;
	unsigned long	r5;
	unsigned long	r6;
	unsigned long	r7;
	unsigned long	r8;
	unsigned long	r9;
	unsigned long	sl;
	unsigned long	fp;
	unsigned long	sp;
	unsigned long	pc;    
} CPU_CONTEXT_SAVE;
#define CCS_OFFSET_AT_THREADINFO          28

typedef struct __set_regs {
	unsigned long	reg_bit_mask;  /* bit 0 for r0,...*/
	unsigned long            reg_value[18];   /* value of reg , length not fix */
} SET_REGS;

#define BKP_FLAG_STOP_WAIT                       (1<<1) /* WAIT OR WHILE */
#define BKP_FLAG_CHECK                                 (1<<2)  /* have check */
#define BKP_CHECK_MASK                               0XFFFF0000
#define BKP_FLAG_CMP_EQ                             (1<<16) 
#define BKP_FLAG_CMP_GREAT                      (1<<17) 
#define BKP_FLAG_CMP_LESS                       (1<<18) 
#define BKP_FLAG_CMP_STRING                     (1<<19)         /* string or int */
#define BKP_FLAG_CMP_STRINCLUDE           (1<<20)       /* strstr */ 
#define BKP_FLAG_ADDRESS                          (1<<21)       /* src is the addr,load it */ 

#define BKP_FLAG_SOURE_REG( flag )          ((flag>>24)&0xf) /* reg index r0-r10 */
#define BKP_FLAG_SOURE_USER( flag )        (((flag>>24)&0xf) ==0xf) /* reg index r0-r10 */
#define BKP_CMP_MAX_REG                          10 /* max useful reg=r10  */
#define BKP_FLAG_SOURE_PID( flag )        (((flag>>24)&0xf) ==12) /* reg index r0-r10 */
#define BKP_FLAG_SOURE_SIZE( flag )       (((flag>>28)&0x3) ) /* 0:4BYTE,1:2,2:1,3:reserve */
struct __bk_check_condition {
        unsigned long flags;
        unsigned long addr;
        union{
        long    int_value;
        char    *string;
        } u;
};

/* for file xfer */
#define MAX_PATH_LEN             256
#define FILE_XFER_MAGIC        0x48534c00
typedef struct __file_info{
        int                     size;
        int                    rw;   /* magic:0x48534c. 0:read , 1:write . */
        unsigned long   file_size;
        int                    mod;     /* file mode */
        char    file_path[MAX_PATH_LEN];
        void    *file;
        void    *private_data;
        int      error;         /* 0: OK*/
} FILE_INFO;

#ifndef HOST_READ
#define HOST_READ               1
#define HOST_WRITE              0
#define DEV_ROCKADB           2
#endif 
