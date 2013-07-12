/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    MAINLOOP.C 
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.57(cmy):
                    去除芯片识别、cramfs等相关代码
                    添加boot.img/recovery.img的检查及修复
                    添加bootloader的本地升级功能
********************************************************************************
********************************************************************************/

#define	    IN_MAIN
#include	"config.h"
#include    "mk_various_file.h"
#include    <stdarg.h>



extern uint32       	gLoaderHasGetIdblock;
extern void             start_linux(PBootInfo pboot_info);
extern uint32           part2_data[];
uint32* gLoaderTlb ;//= (uint32*)0x60040000;
extern uint32 IsMMUEnable;
extern void P_RC4(unsigned char * buf, unsigned short len);
extern void P_RC4_ext(unsigned char * buf, unsigned short len);
extern void RkldTimePowerOnInit( void );
extern void change_cmd_for_recovery(PBootInfo boot_info , char * rec_cmd );
extern int dispose_bootloader_cmd(struct bootloader_message *msg, mtd_partition *misc_part);


uint32 g_bootRecovery;
uint32 g_FwEndLba;
uint32 g_BootRockusb;

uint32 g_isRk2908Chip;


/* 内核加载地址
 * 从该地址开始的3M空间内，可做为临时空间存放数据
 */
uint32 krnl_load_addr = 0;

char bootloader_ver[24]="";
uint16 internal_boot_bloader_ver = 0;
uint16 update_boot_bloader_ver = 0;

/**************************************************************************
IRQ中断服务子程序
***************************************************************************/
void __user_initial_stackheap(void)
{
}

/* 读取IDBlock中的版本信息
 */
int get_bootloader_ver(char *boot_ver)
{
	int i=0;
	uint8 *buf = (uint8*)&gIdDataBuf[0];
    memset(bootloader_ver,0,24);
    
	if( *(uint32*)buf == 0xfcdc8c3b )
	{
		uint16 year, date;
       // GetIdblockDataNoRc4((uint8*)&gIdDataBuf[0],512);
        GetIdblockDataNoRc4((uint8*)&gIdDataBuf[128*2],512);
        GetIdblockDataNoRc4((uint8*)&gIdDataBuf[128*3],512);
		year = *(uint16*)((uint8*)buf+512+18);
		date = *(uint16*)((uint8*)buf+512+20);
		internal_boot_bloader_ver = *(uint16*)((uint8*)buf+512+22);
		//loader_tag_set_version( year<<16 |date , ver>>8 , ver&0xff );
		sprintf(bootloader_ver,"%04X-%02X-%02X#%X.%02X", 
				year,
				(uint8)((date>>8)&0x00FF), (uint8)(date&0x00FF),
				(uint8)((internal_boot_bloader_ver>>8)&0x00FF), (uint8)(internal_boot_bloader_ver&0x00FF));
		return 0;
	}
	return -1;
}





uint8* g_32secbuf;
uint8* g_cramfs_check_buf;

uint8* g_pIDBlock;
uint8* g_pLoader;
uint8* g_pReadBuf;
uint8* g_pFlashInfoData;

/*
    begin_addr: 指示从何处开始作为存放大块数据之用
                该变量由parameter文件中取得，目前为kernel代码加载位置
 */
void setup_space(uint32 begin_addr)
{
    uint32 next = 0;

/*
    提供一块32*(512+16)大小的空间，存放临时数据
 */
    g_32secbuf = (uint8*)begin_addr;
    next += 32*528;

/*
    TODO: 在此可分配一系列的空间，用于存放临时数据，如:
        g_16secbuf1 = (uint8*)next;
        next += 16*528;
        g_4secbuf2 = (uint8*)next;
        next += 4*528;
 */

/*****************************************************************/

/*
    g_cramfs_check_buf 所指空间为具体分区的大小
    只在校验cramfs文件系统时使用
    注: 目前recovery分区已占了2.4MB
 */
    g_cramfs_check_buf = (uint8*)begin_addr;

/*****************************************************************/

/*
    以下变量在升级Loader时用到，在升级loader时不应使用前面所列的变量
    
    g_pIDBlock: 2048*(512+16), 存放读取到的ID Block内容
    g_pLoader:  192*1024, 存放读取到的升级用的loader文件内容
    g_pReadBuf: 在升级时作读取数据的缓存，做校验用的
 */
    g_pIDBlock = (uint8*)begin_addr;
    next = begin_addr + 2048*528;
    g_pLoader = (uint8*)next;
    next += 192*1024;
    g_pReadBuf = (uint8*)next;
    next += MAX_WRITE_SECTOR*528;
    g_pFlashInfoData = (uint8*)next;
    next += 2048;

/*****************************************************************/

}

void SysLowFormatCheck(void)
{
    if(FWLowFormatEn)
    {
        RkPrintf("FTLLowFormat,tick=%d\n" , RkldTimerGetTick());
        FW_SorageLowFormat();
        FWLowFormatEn = 0;
    }
}

int loader_tag_checkboot( void ) 
{
    uint32 loader_flag = IReadLoaderFlag();
    int boot = BOOT_NORMAL;
    if((loader_flag&0xFFFFFF00) == SYS_LOADER_REBOOT_FLAG)
    {
        boot = loader_flag&0xFF;
    }
    ISetLoaderFlag(SYS_KERNRL_REBOOT_FLAG|boot);
    return boot;
}

void Switch2MSC(void)
{
#ifdef DRIVERS_USB_APP
    uint8 *paramBuffer = (uint8*)DataBuf;
    if( 0 == GetParam(parameter_lba, (void*)paramBuffer) )
    {
        int UserPartIndex;
        memset(&gBootInfo, 0, sizeof(gBootInfo));
        ParseParam( &gBootInfo, ((PLoaderParam)paramBuffer)->parameter, ((PLoaderParam)paramBuffer)->length );
        UserPartIndex = find_mtd_part(&gBootInfo.cmd_mtd, "user");
        if(UserPartIndex > 0)
        {
            extern uint32 UserPartOffset;
            UserPartOffset = gBootInfo.cmd_mtd.parts[UserPartIndex].offset;
ReConnectUsbBoot:            
            RkPrintf("UserPartOffset = 0x%x\n",UserPartOffset);
            FW_ReIntForUpdate();
            MscInit();
        }
    }
    else
    {
        //RkPrintf("no  parameter\n");
        UserPartOffset = 0;
        goto ReConnectUsbBoot;
    }
#endif    
}
/***************************************************************************
循环主程序
二级Loader:SDRAM 以识别和初始化，主频100M
***************************************************************************/
int Main(void)
{
	int     bootNorecovery = 0;
	int     secureBootDisable = 0;
	int     boot = 0;
	char    recv_cmd[48]={0};
	uint8   *paramBuffer = (uint8*)DataBuf;

    g_bootRecovery = FALSE;
	g_BootRockusb = 0;
    FWLowFormatEn = 0;
    FWSetResetFlag = 0;
    g_FwEndLba = 0;
#if(PALTFORM==RK30XX)
	ChipTypeCheck();
#endif
    RkldTimePowerOnInit();
#ifdef DEBUG
// 串口初始化，从此之后可打印信息
	serial_init();   
	RkPrintf("BUILD=====%d\n", RkldTimerGetTick());
#endif

    SetARMPLL(640);
    InterruptInit();
    RockusbKeyInit(&key_rockusb);
	//RkPrintf("PMU_PWRDN_CON = %x\n",read_XDATA32(PMU_BASE_ADDR+0x8));
	if(SYS_LOADER_ERR_FLAG == IReadLoaderFlag() )
	{
		RkPrintf("Loader flag is ture\n");
		g_BootRockusb = 2;
		
		ISetLoaderFlag(BOOT_LOADER);
	}
	gLoaderTlb = (uint32*)0x60040000;//256KB
	MMUCreateTlb(gLoaderTlb);
#ifdef L2CACHE_ENABLE  
	L2CacheLineConfig(L2CACHE_WAY_SIZE,L2CACHE_WAY_NUM);
#endif	
	MMUInit((uint32)gLoaderTlb);  //开启MMU,CHCHE
#ifdef USE_RECOVER_IMG
// 根据TAG来判断这次所需做的事情
    //recv_cmd[0]=0;
    switch( loader_tag_checkboot() )
    {
	case BOOT_LOADER:
	    RkPrintf("Set g_BootRockusb=TRUE\n");
        g_BootRockusb = 2;
        //while(g_FwEndLba == 0);
        break;
	case BOOT_RECOVER:
    	RkPrintf("Set g_bootRecovery=TRUE\n");
        g_bootRecovery = 1;
        recv_cmd[0]=0;
        break;
    case BOOT_WIPEDATA:
        g_bootRecovery = 1;
        strcpy(recv_cmd," recovery_cmd=--wipe_data");       /* need first space!!*/       
        break;
    case BOOT_WIPEALL:
        g_bootRecovery = 1;
        strcpy(recv_cmd," recovery_cmd=--wipe_all");
        break;           
	case BOOT_NORECOVER:
	    RkPrintf("Set bootNorecovery=TRUE\n");
        bootNorecovery = 1;
        break;
    case BOOT_SECUREBOOT_DISABLE:
        secureBootDisable = 1;
        break;
	default:
        break;
	}
#endif
	if( StorageInit() == 0)
		RkPrintf("OK! %d\n", RkldTimerGetTick());
	else
		RkPrintf("Fail!\n");
#ifdef RK_FLASH_BOOT_EN 
    FlashTimingCfg(GetAHBCLK());
#endif    
    SecureBootCheck();
// 获取当前loader的版本号
	get_bootloader_ver(NULL);
	RkPrintf("Boot ver: %s\n", bootloader_ver);
    //rsa_test();

	parameter_lba = 0;

// 如果是要进升级模式，则不需要处理parameter
//	if(g_BootRockusb==FALSE)
	{
        // 获取参数
        uint8 *paramBuffer = (uint8*)DataBuf;
   	    /*if( GetPortState(&key_rockusb)) //检查默认recover按键
	    {
            //RkPrintf("default RECOVERY key is pressed\n","");
            g_BootRockusb = 1;
	    }*/
        //RkPrintf("get parameter\n");
        KeyCombinationNum = 0;
        memset(&key_combination[0], 0, sizeof(key_config)*MAX_COMBINATION_KEY);
        memset(&gBootInfo, 0, sizeof(gBootInfo));
		if( 0 == GetParam(parameter_lba, (void*)paramBuffer) )
		{
			BackupParam((void*)paramBuffer);
ReParseParam:
			//RkPrintf("analyze parameter\n");
			// 解析参数
			ParseParam( &gBootInfo, ((PLoaderParam)paramBuffer)->parameter, ((PLoaderParam)paramBuffer)->length );
			// 在cmdline末尾添加loader版本信息(从IDBlock获取)
			strcat(gBootInfo.cmd_line, " bootver=");
			strcat(gBootInfo.cmd_line, bootloader_ver);
			
			// 在cmdline末尾添加固件版本信息(从parameter获取)
			strcat(gBootInfo.cmd_line, " firmware_ver=");
			strcat(gBootInfo.cmd_line, gBootInfo.fw_version);
			
			// 根据内核地址分配大数据的缓存位置
			setup_space(gBootInfo.kernel_load_addr);

            if( !g_bootRecovery ) {
                // 根据按键确认是跑rockusb还是进入recovery系统，或者进入android
                int recovery_key = checkKey(&g_BootRockusb, &g_bootRecovery);
                if(recovery_key)
                {
                    char temp_str[30];
                    sprintf(temp_str," recovery_key=%d" , recovery_key);
                    strcat(gBootInfo.cmd_line, temp_str);
                }                
                if(g_bootRecovery)
                    g_BootRockusb = 0;
        	}
		}
		else// 没有参数，则直接出rockusb
		{
			if(0 == GetBackupParam((void*)paramBuffer))
			{
                RkPrintf("read backup Parameter\n");
                goto ReParseParam;
			}
		    RkPrintf("!!!No parameter\n");
			g_BootRockusb = 2;
			SecureBootEn = FALSE;
		}
	}
	
#ifdef INSTANT_BOOT_EN
    if(g_BootRockusb == 0)
    {
        uint32 ret = BootSnapshot(&gBootInfo);
        RkPrintf("BootSnapshot ret = %x\n",ret);
    }
#endif

#ifdef SECURE_BOOT_TEST
	if(g_BootRockusb == 0 )//|| (SecureBootEn && g_BootRockusb != 2))
#else
    if(g_BootRockusb == 0 )//|| (SecureBootEn && g_BootRockusb != 2))
#endif
	{
        mtd_partition *misc_part = NULL;
        uint8 misc_buf[2048] = {0};
        //SoftReset(); //自动重启测试
#ifdef USE_RECOVER
        PRINT_I("get command in misc\n");
		// MISC分区中存放着系统(或者工具)传给loader的命令
		misc_part = gBootInfo.cmd_mtd.parts+gBootInfo.index_misc;
		if(StorageReadLba( misc_part->offset+32, (void*)misc_buf, 4)==0 )
		{
			struct bootloader_message *msg = (struct bootloader_message*)misc_buf;
			if(strlen(msg->command)>0)
			{
			    PRINT_I("execute command\n");
			    // 处理传进来的命令
				if( dispose_bootloader_cmd(msg, misc_part) )
				    goto BOOT_RKUSB;// 执行命令失败
			}
			else
			    PRINT_I("no command\n");
		}

		// 强制不进入recovery系统
        if( bootNorecovery )
            g_bootRecovery = 0;
            
		if(g_bootRecovery)// 要从recovery启动，需修改cmdline
		{
            PRINT_I("change cmdline for recovery\n" );
			change_cmd_for_recovery(&gBootInfo , recv_cmd);
		}
#endif
        //loader_tag_set_bootflag(g_bootRecovery,0,0);
        
        if(g_bootRecovery)
            PRINT_I("Boot into recovery system\n" );
        else
            PRINT_I("Normal boot\n" );
        //DRVDelayMs(4000);
        // 启动Linux，执行成功则不返回。
        RkPrintf("start_linux=====%d\n", RkldTimerGetTick());
		start_linux(&gBootInfo);
	}

BOOT_RKUSB:
// 进入USB 升级模式
	RkPrintf("UsbBoot,%d\n" , RkldTimerGetTick());
    FW_SDRAM_ExcuteAddr = 0;
//    g_BootRockusb = 0;
ReConnectUsbBoot:
    UsbBoot();
	RkPrintf("UsbHook,%d\n" , RkldTimerGetTick());
    UsbHook();
    //Switch2MSC(); //debug
    // loader_tag_set_bootflag(0x2 , 0 , 0);
	powerOff();
    while(1)
    {
    }
}


uint32 getSysProtAddr(void)
{
    uint32 sysProtAddr = 0;
    if(gBootInfo.index_backup)
    {
        sysProtAddr = gBootInfo.cmd_mtd.parts[gBootInfo.index_backup].offset + gBootInfo.cmd_mtd.parts[gBootInfo.index_backup].size;
    }
    return sysProtAddr;
}
#if 0
#define TAG_KERNEL			0x4C4E524B
typedef struct tagKernelImg
{
	uint32	tag;
	uint32	size;
	char	image[1];
	uint32	crc;
}KernelImg, *PKernelImg;

void UsbBootLinux(uint32 KernelAddr,uint32 Parameter)
{
    KernelImg * pKernrlImg = (KernelImg * )KernelAddr;
    if(Parameter == 0)
    {
        void (*pfun)(void);
        pfun = (void (*)(void))KernelAddr;
        cleanup_before_linux ();
        //进入 SVC模式.
        //__asm { MSR CPSR_c,#0xd3 }; //SVC , NO IRQ, FIQ.
        /* we assume that the kernel is in place */
        pfun();
    }
    memset(&gBootInfo, 0, sizeof(gBootInfo));
    RkPrintf("analyze parameter\n");
    // 解析参数
    ParseParam( &gBootInfo, (char *)Parameter, 512);

    // 在cmdline末尾添加loader版本信息(从IDBlock获取)
    strcat(gBootInfo.cmd_line, " bootver=");
    strcat(gBootInfo.cmd_line, bootloader_ver);

    // 在cmdline末尾添加固件版本信息(从parameter获取)
    strcat(gBootInfo.cmd_line, " firmware_ver=");
    strcat(gBootInfo.cmd_line, gBootInfo.fw_version);

    // 根据内核地址分配大数据的缓存位置
    //setup_space(gBootInfo.kernel_load_addr);
    gBootInfo.kernel_load_addr = KernelAddr;
    if(pKernrlImg->tag == TAG_KERNEL)
    {
        ftl_memcpy((uint8*)gBootInfo.kernel_load_addr,(uint8*)(gBootInfo.kernel_load_addr+8),pKernrlImg->size);        
    }

	RkPrintf("END ===== %d\n", RkldTimerGetTick());

	PRINT_I("CMDLINE: %s\n", gBootInfo.cmd_line);
#if 0    
    bootm_linux(gBootInfo.kernel_load_addr,
				gBootInfo.machine_type,
				gBootInfo.atag_addr,
				gBootInfo.cmd_line);
#else    
    //ftl_memcpy((char *)0x60000000,gBootInfo.cmd_line,1024);
	bootm_linux(gBootInfo.kernel_load_addr,
				gBootInfo.machine_type,
				gBootInfo.atag_addr,
				gBootInfo.cmd_line);
#endif    
}
#endif
