/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include "config.h"

#include "common.h"
#include "command.h"
#include "image.h"
#include "byteorder.h"
#include "arm926ejs.h"
#include "setup.h"

#include "AFPTool.h"
#include "mk_various_file.h"
#include "bootloader.h"
#include "cramfs_fs.h"
#include "../common/crc/sha.h"

/***********************************************************************/
// config 
//#define CONFIG_CMDLINE_TAG
//#define CONFIG_SETUP_MEMORY_TAGS
//#define CONFIG_INITRD_TAG

extern BootInfo gBootInfo;
extern void   MMUDisable(void);
extern int find_mtd_part(cmdline_mtd_partition* this_mtd, const char* part_name);
extern uint32 CRC_32CheckBuffer( unsigned char * aData, unsigned long aSize );
extern int LoadOemImage(uint32 partIndex,uint32 Mode);

//#pragma arm section code = "LOADER2"

enum{
	RESTORE_KERNEL = 0,
	RESTORE_RECOVERY,
	RESTORE_BOOT
};

//#define MIN(x,y) ( (x)<(y)?(x):(y) )

#define BYTE2SEC(x) (((x)-1)/512 + 1)


#define TAG_KERNEL			0x4C4E524B
#define TAG_2908_KERNEL		0x38303932
#define TAG_2906_KERNEL		0x36303932


typedef struct tagKernelImg
{
	uint32	tag;
	uint32	size;
	char	image[1];
	uint32	crc;
}KernelImg, *PKernelImg;


#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

typedef struct tag_boot_img_hdr
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned int kernel_size;  /* size in bytes */
    unsigned int kernel_addr;  /* physical load addr */

    unsigned int ramdisk_size; /* size in bytes */
    unsigned int ramdisk_addr; /* physical load addr */

    unsigned int second_size;  /* size in bytes */
    unsigned int second_addr;  /* physical load addr */

    unsigned int tags_addr;    /* physical addr for kernel tags */
    unsigned int page_size;    /* flash page size we assume */
    unsigned int unused[2];    /* future expansion: should be 0 */

    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */
    
    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned int id[8]; /* timestamp / checksum / sha1 / etc */

    unsigned char reserved[0x400-0x260];
    unsigned long signTag; //0x4E474953
    unsigned long signlen; //128
    unsigned char rsaHash[128];
}boot_img_hdr;

#define MAX_SNAPSHOT_SEGMENT   16

typedef struct {
	unsigned int p_offset; // align 512 bytes
	unsigned int p_vaddr;
	unsigned int p_filesz;
	unsigned int p_memsz;
} TPIB32_Phdr;

typedef struct {
	unsigned char t_ident[8];
	unsigned long t_version;
	//unsigned short t_machine;
	//unsigned short t_type;
	unsigned int hash;
	unsigned int t_filesz;
	unsigned int t_entry;
	unsigned int t_phoff; 
	unsigned short t_phentsize; 
	unsigned short t_phnum;
	TPIB32_Phdr phdr[MAX_SNAPSHOT_SEGMENT];
} TPIB32_Thdr;

#define RK_ARMLINUX_MACHINETYPE		1616 // ROCK CHIP ,848 	// at91926e

static uint32 g_atag_addr = 0x60000800;
static int g_machine_type = RK_ARMLINUX_MACHINETYPE;

#define LOAD_DATA(load_addr, offset, size) (CopyFlash2Memory(load_addr, offset, BYTE2SEC(size)))

/*********************************************************************/
typedef struct sdramfile_tag
{
	char * 	filename ;
	uint32	addr ;
	uint32	size;
} sdram_file;
/*********************************************************************/
extern void DisableIRQ(void);


static struct tag *params;
static struct tag *params0;

// 为了尽可能保证 LOADER 不变，通过外部传递 CMDLINE来引导 
// 内核，cmdline可有别的文件传入.
// 前面流出一部分空间用于存放 LOADER 源代码或者信息.
// by HSL@RK,20090324,可以考虑 MACHINE TYPE 通过CMDLINE来传递，以保证通用.
static sdram_file arlix_file_info [] = 
{
	{"image" , SDRAM_BASE_ADDRESS+0x8000 , 0 },  //非压缩的内核Image文件.
	//{"ramdisk.gz" , SDRAM_BASE_ADDRESS+0x1000000 , 0 },  // 压缩的 根文件系统.
	{"cmdline" , SDRAM_BASE_ADDRESS+0x800 , 0 }, // 传给内核的命令行. ARM最长为 1024.
	{"kernel.img", SDRAM_BASE_ADDRESS+0x8000, 0}, // 经过封装后的Image
	{"parameter-ram", SDRAM_BASE_ADDRESS+0x200, 0}, // 参数文件中包含有cmdline
	{""/*"taglist" */, SDRAM_BASE_ADDRESS+0x1000 , 0 }, //传递给内核的TAGLIST. and the end.
	//	{"",0,0}	// the end.
} ; 

#define ARMLIX_SF_CMDLINE()			(&arlix_file_info[1])
#define ARMLIX_SF_KERNEL()			(&arlix_file_info[2])
#define ARMLIX_SF_PARAMETER()		(&arlix_file_info[3])
#define ARMLIX_SF_TAGLIST()			(&arlix_file_info[4])
///////////////////////////////////////////////////////////////////////////////////
static sdram_file* boot_check_filename( char * filename )
{
	sdram_file 	*sf = &arlix_file_info[0];
	char 	lowfilename[256] ;
	char		* p ;
	char		*pd = lowfilename;

	// 去除路径，只取文件名.
	p = strrchr( filename , '\\' );
	if( !p )
	{// CMY: 在Linux中的路径名用'/'分隔
		p = strrchr( filename, '/' );
		if( !p )
			p = filename;
		else
			p++;
	}
	else
		{
		p++;
		}
	
	// to low case .
	do 
		{
		if( *p <= 'Z' && *p >= 'A' )
			{
			*pd = *p - 'A' + 'a' ;
			}
		else
			{
			*pd = *p;
			}
		pd ++;
		} while( *p++ );

	// check if exit!
	while( *(sf->filename) )
		{
// 原来Kernel Image只能取名为image，CMD_LINE只能取名为cmdline
// 现在Kernel Image可取名为imageXX..XX，CMD_LINE可取名为cmdlineXX..XX
#if 0
		if( !strcmp( sf->filename , lowfilename ) )
#else
		if( !strncmp( sf->filename , lowfilename, strlen(sf->filename) ) )
#endif
			return sf;
		sf++;
		}
	
	//sprintf(lowfilename,"init=%d" , 0x34567);
	return NULL;
}

#ifdef LINUX_BOOT
void boot_setfile_info(  char* filename , uint32 addr , uint32 size )
{
	sdram_file 	*sf = boot_check_filename( filename );
	if( sf  )
	{
		RkPrintf("Get file:%s\n",sf->filename );

		if ( sf == ARMLIX_SF_KERNEL() )
		{
			sf->addr = addr;
			ftl_memcpy((void*)(sf->addr), (void*)(sf->addr+8), size-12);
			sf->size = size-12;
		}
		else if( sf == ARMLIX_SF_PARAMETER() )
		{
			ParseParam( &gBootInfo, (char*)addr, size );
			strcpy( (char*)addr, gBootInfo.cmd_line );
			sf->addr = addr;
			sf->size = strlen((char*)sf->addr);
			if( sf->size >= 1024 )
			{
				sf->size = 1023;
				((char*)sf->addr)[1023] = 0;
			}
			g_atag_addr= gBootInfo.atag_addr;
			g_machine_type = gBootInfo.machine_type;
		}
		else if( sf == ARMLIX_SF_CMDLINE() ) // COMMAND LINE
			{
			//对于PC上输入的文本，需要进行判断，目前只支持
			// utf8 或者ASICC。不支持 UNICODE。
			//还有考虑没有结束符的问题.
			
			char 	*p = (char*)addr;
			sf->size = 0;
			
			//去掉前面的标志字符.
			if( p[0] == 0xef && p[1] == 0xbb && p[2]==0xbf )
				{
				p += 3;
				size -= 3;
				}
			while ( size > 0 )
				{
				if ( *p == ' ' )	//去掉前面部分空格.
					{
					p++;
					size--;
					}
				
				else if( p[0] == 0x0d && p[1] == 0x0a  )	//去掉回车换行.
					{
					p += 2;
					size -= 2;
					}
				else
					{
					break;
					}
				}
			
			if( *p > 'z' || *p < 'a' ) //第一个字符必须是字母.小写
				{
				return ;
				}

			//去掉结尾部分的回车.
			while(size>2   )	
				{
				if(p[size-2] == 0x0d && p[size-1] == 0x0a )
					size -= 2;
				else if( p[size-1] ==' ' )
					size --;
				else
					break;
				}

			p[size++] = 0;	//结尾增加结束符 0.
			
			// 两个地址不能重叠.
			sf->size = size ;
			
			if( (uint32)p > sf->addr )	//may copy , not crash data.
				{
				strcpy((char*)sf->addr , p);
				}
			else
				{
				sf->addr = (uint32)p ;
				}
			
			if( sf->size >= 1024 )
				{
				sf->size = 1024;
				((char*)sf->addr)[1023] = 0;
				}
			
			}
		else
			{
			sf->addr = addr ; 
			sf->size = size;
			}
		}
}
#endif
static void setup_start_tag ( void* );

static void setup_commandline_tag ( const char *cmdline );

#ifdef CONFIG_SERIAL_TAG
static void setup_serial_tag( void );
#endif

static void setup_end_tag ( void );


//#define SHOW_BOOT_PROGRESS(arg)

// NO HEAD SUPPURT CURRENT.
//extern image_header_t header;	/* from cmd_bootm.c */
int _cleanup_before_linux (void)
{
    MMUDeinit();
	return (0);
}

/*
	image_addr : 固件的地址.
	initrd_addr : initrd 的地址
	initrd_size:  initrd 的大小(bytes)
	cmdline: 命令行的地址.
*/
void bootm_linux (uint32 image_addr, int machine_type, uint32 atag_addr, char* cmdline )
{

	void (*theKernel)(int zero, int arch, uint32 params);
	//FlashSaveDataToSram();
	theKernel = (void (*)(int, int, uint32))image_addr;

    if(SecureBootCheckOK == 0)
    {
        SecureBootDisable();
        if(g_BootRockusb) //进固件升级
        {
            RkPrintf ("Switch2Rocusb!\n");
            return;
        }
    }
#ifdef SECURE_BOOT_TEST
    SetSysData2Kernel(1);
#else
    SetSysData2Kernel(SecureBootCheckOK);
#endif    
    RkPrintf (" %d Starting kernel...@0x%x\n\n", RkldTimerGetTick(),image_addr);
                
	setup_start_tag ((void*)atag_addr);
	
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag ();
#endif

	setup_commandline_tag ( cmdline );

	setup_end_tag ();
    //loader_tag_set_timer( RkldTimerGetCount(0) , RkldTimerGetCount(2) );
	/* we assume that the kernel is in place */

	_cleanup_before_linux ();
	//进入 SVC模式.
#ifndef __GNUC__
    __asm { MSR	CPSR_c,#0xd3 };	//SVC , NO IRQ, FIQ.
#endif
	/* we assume that the kernel is in place */

	theKernel (0,  machine_type , (uint32)params0 );
}

static void setup_start_tag (void *addr)
{
	params0 = params = (struct tag *)(addr );
	
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}



static void setup_commandline_tag ( const char *cmdline )
{
	char *p;

	if ( !cmdline || !cmdline[0] )
		return;

	/* eat leading white space */
	for (p = (char*)cmdline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}



#ifdef CONFIG_SERIAL_TAG
void setup_serial_tag ( void )
{
	struct tag_serialnr serialnr;
	void get_board_serial(struct tag_serialnr *serialnr);

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
}
#endif

static void setup_end_tag ( void )
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}


void boot_run( void )
{
	
	if( arlix_file_info[0].size > 0 )	//下载了 LINUX IMAGE.
	{
		if( ARMLIX_SF_CMDLINE()->size > 0 )
		{
			bootm_linux(arlix_file_info[0].addr, g_machine_type, g_atag_addr, (char*)ARMLIX_SF_CMDLINE()->addr);
		}
		else
		{
			bootm_linux(arlix_file_info[0].addr, g_machine_type, g_atag_addr, (char*)NULL);
		}
	}
	else if( ARMLIX_SF_KERNEL()->size > 0 )
	{
		if( ARMLIX_SF_PARAMETER()->size > 0 )
		{
			bootm_linux(ARMLIX_SF_KERNEL()->addr, g_machine_type, g_atag_addr, gBootInfo.cmd_line);
		}
		else
		{
			bootm_linux(ARMLIX_SF_KERNEL()->addr, g_machine_type, g_atag_addr, (char*)NULL);
		}
	}
}

#if 0
static void setup_tags(uint32 atag_addr, const char* cmdline)
{
	PRINT_D("Enter\n");
//	PRINT_D("atag_addr=0x%08X, cmdline=%s\n", atag_addr, cmdline);
	setup_start_tag (( void*)atag_addr );
	
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag ();
#endif

	setup_commandline_tag ( cmdline );

	setup_end_tag ();
	PRINT_D("Leave\n");
}
#endif

#define MaxFlashReadSize  16384  //8MB
int32 CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec)
{
	uint8 * pSdram = (uint8*)dest_addr;
	uint16 sec = 0;
	uint32 LBA = src_addr;
	uint32 remain_sec = total_sec;

//	RkPrintf("Enter >> src_addr=0x%08X, dest_addr=0x%08X, total_sec=%d\n", src_addr, dest_addr, total_sec);

//	RkPrintf("(0x%X->0x%X)  size: %d\n", src_addr, dest_addr, total_sec);
	
	while(remain_sec > 0)
	{
		sec = (remain_sec > MaxFlashReadSize) ? MaxFlashReadSize : remain_sec;
		if(StorageReadLba(LBA,(uint8*)pSdram, sec) != 0)
		{
			return -1;
		}
		remain_sec -= sec;
		LBA += sec;
		pSdram += sec*512;
	}

//	RkPrintf("Leave\n");
	return 0;
}

int data_restore(uint32 src_offset, uint32 dest_offset, int sectors)
{
	uint16 sec = 0;
	uint32 remain_sec = sectors;
    uint16 mode = (src_offset > dest_offset)? 0 : 1; // 恢复的地址大于backup区地址，使用lba方式，否则使用img方式

	while(remain_sec > 0)
	{
		sec = (remain_sec>32)?32:remain_sec;

		memset((void*)g_32secbuf, 0, 32*512);
		
		if(StorageReadLba(src_offset, g_32secbuf, sec) != 0)
		{
			return -1;
		}

        if(StorageWriteLba(dest_offset, g_32secbuf, sec, mode) != 0)
		{
			return -2;
		}

		remain_sec -= sec;
		src_offset += sec;
		dest_offset += sec;
	}

	return 0;
}

void ReSizeRamdisk(PBootInfo pboot_info,uint32 ImageSize)
{
    char* sSize = NULL;
    int len = 0;
    char* s = NULL;
    char szFind[20]="";
     //判断ImageSize 合法
    ImageSize = (ImageSize + 0x3FFFF)&0xFFFF0000;//64KB 对齐
    //修改ramdisk大小
    sprintf(szFind, "initrd=0x");
    sSize = strstr(pboot_info->cmd_line, szFind);
    if( sSize != NULL )
    {
        sSize+=18;
        s = strstr(sSize, " ");
        len = s - sSize;
        if(sSize[0]=='0' && sSize[1]=='x' && len <= 10 && len >= 8)
        {
            //sprintf(szFind, "0x000000");
            //replace_fore_string(sSize,8, szFind);
            sprintf(szFind, "%08X",ImageSize);
            replace_fore_string(sSize+2,6+(len-8), szFind+(10-len));
        }                
    }
}

int rk29_check_bootimg_sha (const boot_img_hdr *hdr)
{
	SHA_CTX ctx;
	uint8_t* sha;
	unsigned  sha_id[8]={0,};
	int i;
	uint32_t second_data=0x0;		
 	
    SHA_init(&ctx);
    SHA_update(&ctx, (void *)hdr->kernel_addr, hdr->kernel_size);
    SHA_update(&ctx, &hdr->kernel_size, sizeof(hdr->kernel_size));
    SHA_update(&ctx, (void *)hdr->ramdisk_addr, hdr->ramdisk_size);
    SHA_update(&ctx, &hdr->ramdisk_size, sizeof(hdr->ramdisk_size)); 
    SHA_update(&ctx, (void *)second_data, hdr->second_size);
    SHA_update(&ctx, &hdr->second_size, sizeof(hdr->second_size)); 
	SHA_update(&ctx, &hdr->tags_addr, sizeof(hdr->tags_addr)); 
	SHA_update(&ctx, &hdr->page_size, sizeof(hdr->page_size)); 
	SHA_update(&ctx, &hdr->unused, sizeof(hdr->unused)); 
	SHA_update(&ctx, &hdr->name, sizeof(hdr->name));
	SHA_update(&ctx, &hdr->cmdline, sizeof(hdr->cmdline)); 
	sha = SHA_final(&ctx);
	ftl_memcpy(sha_id, sha,SHA_DIGEST_SIZE > sizeof(hdr->id) ? sizeof(hdr->id) : SHA_DIGEST_SIZE); 

	for(i=0;i<8;i++)
	{
		if (hdr->id[i] !=sha_id[i])
			return 0;
	}

	return 1;
}

int32 checkboothdr(boot_img_hdr * boothdr)
{
    if((boothdr->kernel_addr > 0x60E00000 && boothdr->kernel_addr < 0x61000000) ||(boothdr->ramdisk_addr > 0x60E00000 && boothdr->ramdisk_addr < 0x61000000) 
    || boothdr->kernel_addr > 0xA0000000 || boothdr->ramdisk_addr > 0xA0000000
    || boothdr->kernel_addr < 0x60000000 || boothdr->ramdisk_addr < 0x60000000)
    {
        PRINT_E("kernel=%x,ramdisk=%x\n", boothdr->kernel_addr , boothdr->ramdisk_addr);
        return -1;
    }
    return 0;
}



#define FLASH_PAGE_SIZE 32 //16KB
int32 bootKernelInBootImg(PBootInfo pboot_info,boot_img_hdr * boothdr,uint32 offset)
{
    int kernelSize =  ((boothdr->kernel_size + FLASH_PAGE_SIZE*512 -1) / (FLASH_PAGE_SIZE*512))*(FLASH_PAGE_SIZE);
    int ramdiskSize = ((boothdr->ramdisk_size + FLASH_PAGE_SIZE*512 -1) / (FLASH_PAGE_SIZE*512))*(FLASH_PAGE_SIZE);
    
    //PRINT_E("offset = %x\n", offset);
    //PRINT_E(" boothdr->kernel_addr = %x,boothdr->ramdisk_addr = %x\n", boothdr->kernel_addr , boothdr->ramdisk_addr);
    //PRINT_E(" boothdr->kernel_size = %x,boothdr->ramdisk_size = %x\n", boothdr->kernel_size , boothdr->ramdisk_size);
    //PRINT_E(" kernelSize = %x,ramdiskSize = %x\n", kernelSize , ramdiskSize);
    if(checkboothdr(boothdr))
    {
        return -2;
    }

    if( LOAD_DATA(boothdr->kernel_addr, offset+FLASH_PAGE_SIZE, kernelSize*512) != 0 )
        return -3;

    if( LOAD_DATA(boothdr->ramdisk_addr, offset+FLASH_PAGE_SIZE+kernelSize, ramdiskSize*512) != 0 )
        return -3;

    ReSizeRamdisk(pboot_info,ramdiskSize*512);

    //sha校验
    if(rk29_check_bootimg_sha(boothdr) == 0)
    {
       PRINT_E("SHA ERROR!\n");
       return -4;
    }
    
	//机器已经lock，固件就要签名
    if(SecureBootLock_backup && SecureBootCheckOK == 0)
    {
        PRINT_E("FW unsigned!\n");
        return -5;
    }
    
    //PRINT_E("CMDLINE: %s\n", pboot_info->cmd_line);
    powerOn();
    bootm_linux(boothdr->kernel_addr,
                pboot_info->machine_type,
                pboot_info->atag_addr,
                pboot_info->cmd_line);
    return -5;
}

uint32 LoadTempBuf[512];
int32 kernel_load_check(PBootInfo pboot_info, uint32 load_addr, uint32 offset , int index, uint32 * pImageSize)
{
	uint32 size = 0;
	uint32 crc32 = 0;
	KernelImg *kImage = (KernelImg*)LoadTempBuf;//tmpBuf;
    boot_img_hdr *boothdr = (boot_img_hdr *)LoadTempBuf;
	int seg = 0;
	int image_size = 0;
	int is_kernel_in_boot = 0;
	
// 先读取4个Sector，获取到kernel的头部信息
	if(StorageReadLba(offset, kImage, 4) != 0)
	{
		return -1;
	}

	if(kImage->tag != TAG_KERNEL)
	{
        if(memcmp(boothdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0)
        {
            if(SecureBootEn && boothdr->signTag == 0x4E474953)
            {
                if(SecureBootSignCheck(boothdr->rsaHash,boothdr->id,boothdr->signlen) ==  FTL_OK)
                {
                    SecureBootCheckOK = 1;
                }
            }
            
            #ifdef LOAD_OEM_DATA
            if(pboot_info->index_kernel) //load oem data
            {
                LoadOemImage(pboot_info->index_kernel,SecureBootCheckOK?0:1); //secure boot时不执行第三方代码
            }
            #endif
            
            #if(PALTFORM==RK29XX)
            //if((g_Rk29xxChip ==RK2908_CHIP_TAG || g_Rk29xxChip ==RK2906_CHIP_TAG))
            {
            #if 1
                int kernelSize =  ((boothdr->kernel_size + FLASH_PAGE_SIZE*512 -1) / (FLASH_PAGE_SIZE*512))*(FLASH_PAGE_SIZE);
                int ramdiskSize = ((boothdr->ramdisk_size + FLASH_PAGE_SIZE*512 -1) / (FLASH_PAGE_SIZE*512))*(FLASH_PAGE_SIZE);
                int ramdisk_addr = boothdr->ramdisk_addr;
                int ramdisk_offset = offset+FLASH_PAGE_SIZE+kernelSize;
                if(checkboothdr(boothdr))
                {
                    return -5;
                }
                
                load_addr = pboot_info->kernel_load_addr;//boothdr->kernel_addr;
                pboot_info->is_kernel_in_boot = 1;
                is_kernel_in_boot = 1;
                
                if(StorageReadLba(offset+FLASH_PAGE_SIZE,kImage,4) != 0)
                {
                    return -1;
                }
                
                if(kImage->tag == TAG_2908_KERNEL || kImage->tag == TAG_2906_KERNEL )
                { 
                    PRINT_I("Rk290x Kernel format!\n");
                    if( LOAD_DATA(ramdisk_addr, ramdisk_offset, ramdiskSize*512) != 0 )
                        return -3;
                    
                    ReSizeRamdisk(pboot_info,ramdiskSize*512); 
                }
                else 
                {
                    //if((index == pboot_info->index_recovery)||(g_Rk29xxChip ==RK2918_CHIP_TAG) || (g_Rk29xxChip ==RK2906_6_CHIP_TAG))//old kernel tag
                    //{
                        if(StorageReadLba(offset, kImage, 4) != 0)
                        {
                            return -1;
                        }
                        return bootKernelInBootImg(pboot_info,boothdr,offset);
                    //}
					//PRINT_E("Error: chip is 290x,FW for 2918!\n");
                    //return -5;
                }
                offset+=FLASH_PAGE_SIZE;
            #else
                PRINT_E("Error: chip is 290x,FW for 2918!\n");
                return -5;
            #endif
            }
            #else
            {
                pboot_info->is_kernel_in_boot = 1;
                return bootKernelInBootImg(pboot_info,boothdr,offset);
            }
            #endif
        }

        #if(PALTFORM==RK29XX)
        if(kImage->tag == TAG_2908_KERNEL || kImage->tag == TAG_2906_KERNEL )
        {
            ;
        }
		else
        #endif
		{
            PRINT_E("E:Invaid tag(0x%08X)!\n", kImage->tag);
            return -5;
		}
	}
    else if(SecureBootLock_backup)
    {
        return -8;
    }

	image_size = kImage->size;
    *pImageSize = image_size;
    
	seg = (4*512-8);
	ftl_memcpy((void*)load_addr, kImage->image, seg);

// 接着加载后继的Image数据 也要读取文件末尾的crc校验值，以便进行校验
	size = image_size-seg+4 + 512; // 2908 多读512

	if( LOAD_DATA(load_addr+seg, offset+4, size) != 0 )
		return -3;
#ifndef RK_LOADER_FOR_FT	
// 进行CRC32校验
    //uint32 startTime = RkldTimerGetTick();
    //uint32 endTime;
	crc32 = CRC_32CheckBuffer((uint8*)load_addr, image_size+4);
	if(!crc32)
	{
		PRINT_E("E:CRC failed!\n");
 		return -4;
	}
	//endTime = RkldTimerGetTick();
	//PRINT_I("Check time... %d \n",endTime - startTime);
#if(PALTFORM==RK29XX)
	if(0)//(kImage->tag == TAG_2908_KERNEL && g_Rk29xxChip ==RK2908_CHIP_TAG)||(kImage->tag == TAG_2906_KERNEL && g_Rk29xxChip ==RK2906_CHIP_TAG))//2908 check
    {// 2906 和2908 不做加密
        unsigned long ranseek;

        crc32 = CRC_32CheckBuffer((uint8*)load_addr+image_size+4, 512);
        if(!crc32)
        {
            PRINT_E("E: 290x CRC failed!\n");
            return -4;
        }
        ranseek = *(unsigned long*)(load_addr+image_size+4);

        #if 0
        PRINT_E("Error: image_size = %x!\n",image_size);
        PRINT_E("Error: ranseek = %x!\n",*(unsigned long*)(load_addr+image_size+4));
        PRINT_E("Error: image_size = %x!\n",*(unsigned long*)(load_addr+image_size+8));
        PRINT_E("Error: CRC = %x!\n",*(unsigned long*)(load_addr+image_size+12));
        PRINT_E("Error: image_size = %x!\n",ranseek^(*(unsigned long*)(load_addr+image_size+8)));
        PRINT_E("Error: CRC = %x!\n", ranseek^(*(unsigned long*)(load_addr+image_size+12)));
        #endif
        if(image_size != (ranseek^(*(unsigned long*)(load_addr+image_size+8))))
        {
            PRINT_E("Error: 290x image_size check failed!\n");
            return -5;
        }
        
        if((*(unsigned long*)(load_addr+image_size)) != (ranseek^(*(unsigned long*)(load_addr+image_size+12))))
        {
            PRINT_E("Error: 290x kernel CRC check failed!\n");
            return -5;
        }
    }	

    if(is_kernel_in_boot)
    {
         powerOn();
         bootm_linux(load_addr,
                     pboot_info->machine_type,
                     pboot_info->atag_addr,
                     pboot_info->cmd_line);
    }
#endif
#endif
	return 0;
}

#ifdef USE_RECOVER
/*
 * 由于在分区修复后，继续加载该分区时会进行数据校验
 * 所以这边在还原数据时，不需要check写入的数据
 */
int32 part_recover(PBootInfo pboot_info, int part_index)
{
	int i=0;
	uint8 buf[2048]={0};
	RKIMAGE_HDR *hdr = (RKIMAGE_HDR*)buf;//g_4secbuf;//raw_hdr;
	mtd_partition *backup_part = NULL;
	mtd_partition *restore_part = pboot_info->cmd_mtd.parts+part_index;

	if(pboot_info->index_backup < 0)
	{
		PRINT_E("W:No backup part!\n");
		return -1;
	}

    /*if(pboot_info->index_fwbackup >= 0 )
    {
        backup_part = pboot_info->cmd_mtd.parts+pboot_info->index_fwbackup;
        StorageReadLba(backup_part->offset, hdr, 4);
        if(hdr->tag != RKIMAGE_TAG)
        {
            backup_part = pboot_info->cmd_mtd.parts+pboot_info->index_backup;
        }
    }
    else
    {*/
        backup_part = pboot_info->cmd_mtd.parts+pboot_info->index_backup;
    //}
	
	if( StorageReadLba(backup_part->offset, hdr, 4) != 0)
	{
		return -2;
	}

	if(hdr->tag != RKIMAGE_TAG)
	{
	    PRINT_E("E:Invaid tag(0x%08X) in backup!\n", hdr->tag);
		return -3;
	}

	for(i=0; i<hdr->item_count; i++)
	{
		if( !strcmp(hdr->item[i].name, restore_part->name) )
		{
			uint32 offset = backup_part->offset+BYTE2SEC(hdr->item[i].offset);
			
			if( data_restore(offset, restore_part->offset, MIN(restore_part->size, BYTE2SEC(hdr->item[i].usespace))) )
			{
				PRINT_E("Recover failed\n");
				return -5;
			}
			else
			{
				PRINT_E("Recover success\n");
				return 0;
			}
			break;
		}
	}

	//PRINT_E("Can't find partitions in backup: %s\n", restore_part->name);
	return -6;
}
#endif

/*
    return:
        0 - OK
        other - failed!
 */
int fs_check_sb(int img_offset, int* fssize)
{
    struct cramfs_super * super = (struct cramfs_super *)g_32secbuf;
    unsigned char plain[8];

	if(StorageReadLba(img_offset, (uint8*)super, 4) != 0)
	{
		return -1;
	}
//	dbg_dump((unsigned char*)super, 128);

	/* superblock tests */
	if (super->magic != CRAMFS_MAGIC)
	{
		super = (struct cramfs_super*)((uint8*)super+PAD_SIZE);
		if(super->magic != CRAMFS_MAGIC)
		{
			//PRINT_E("superblock magic not found\n");
			return -2;
		}
	}

	if (super->size < PAGE_CACHE_SIZE) {
		//PRINT_E("superblock size (%d) too small\n", super->size);
		return -4;
	}
	
    if(fssize) *fssize = super->size;
    
    return 0;
}

int cramfs_load_check(uint32 img_offset)
{
#if 0
    int fssize = 0;
    return(fs_check_sb(img_offset, &fssize));
#else
// 先读取4个Sector，获取到cramfs的头部信息
	uint32 crc32 = 0;
	int iResult = 0;
    uint8* buff = g_cramfs_check_buf;
    int fssize = 0;
	/* superblock tests */
	iResult = fs_check_sb(img_offset, &fssize);
    if(iResult)
        return iResult;
        
    if(fssize > 0x600000)//分区大于6MB时不检查
        return 0;
        
// super->size的值等于cramfs文件的大小,文件末尾是附加的crc校验值
	if( LOAD_DATA((uint32)buff, img_offset, fssize+4) != 0 )
		return -2;

// 进行CRC32校验
	crc32 = CRC_32CheckBuffer((uint8*)buff, fssize+4);
	if(!crc32)
	{
		//PRINT_E("Error: CRC check failed!\n");
		return -3;
	}

//	PRINT_I("Check OK!\n");

	return 0;
	#endif
}

int32 load_image(PBootInfo pboot_info, int index, uint32 load_addr, int recovered)
{
	int iResult = 0;
	uint32 ImageSize;
	mtd_partition *part = pboot_info->cmd_mtd.parts+index;

	PRINT_I("Loading %s ...\n", part->name);
	SecureBootCheckOK = 0;
    if(load_addr != 0)
    {
        iResult = kernel_load_check(pboot_info, load_addr, part->offset , index,  &ImageSize);
    }
    else
    {
        iResult = cramfs_load_check(part->offset);// load_addr为0时，使用cramfs
    }
	
	if( iResult == 0 )
	{
		PRINT_I("Load ok!\n");
        if((load_addr!=0) && (index == pboot_info->index_recovery || index ==pboot_info->index_boot))
        {
            ReSizeRamdisk(pboot_info,ImageSize);
         } 
		return 0;
	}
#ifdef USE_RECOVER	
	else
	{// 加载失败，进行恢复
		PRINT_E("Load failed!\n");
        /*if(pboot_info->is_kernel_in_boot && index ==pboot_info->index_boot)
        {
            return -1; //如果boot出错，不从backup里面恢复
        }*/
		
// 只有当存在backup分区，且为CRC校验失败时，才进行恢复
		if( !recovered && pboot_info->index_backup>=0 && (iResult!=-5) )// && (iResult==-2 || iResult==-4) )
		{
			PRINT_E("Begin recover...\n");
			recovered = 1;
            FW_ReIntForUpdate();
			if( part_recover(pboot_info, index) )
			{
				return -1;
			}
			else
			{
				return load_image(pboot_info, index, load_addr, 1);
			}
		}
	}
#endif
	return -2;
}

void start_linux(PBootInfo pboot_info)
{
    pboot_info->is_kernel_in_boot = 0;
    
#ifndef RK_LOADER_FOR_FT
	if( strstr(pboot_info->cmd_line, "root=/dev/nfs")==NULL )
	{// 不是从NFS启动时需要检测boot/recovery分区
	    #ifdef USE_RECOVER_IMG
		if(g_bootRecovery)
		{
			char* s = NULL;
		    char szFind[128]="";
            if(pboot_info->ramdisk_load_addr == 0)//使用cramfs
            {
                s = strstr(pboot_info->cmd_line, "root=");
                if(s != NULL)
                {
                    char newstr[16] = "\0";
                    char* p = strchr(s, ' ');
                    int len = (p?(p-s):strlen(s));
                    
                    sprintf(newstr, "root=/dev/mtdblock%d", pboot_info->index_recovery);
                    replace_fore_string(s, len, newstr);
                }
            }
            if(load_image(pboot_info, pboot_info->index_recovery, pboot_info->ramdisk_load_addr, 0))
                return;
		}
		else
		#endif
		{
            if(load_image(pboot_info, pboot_info->index_boot, pboot_info->ramdisk_load_addr, 0))
            {
                //if(pboot_info->is_kernel_in_boot)//如果kernel在boot里面，那么boot出错时从recovery引导
                {
                    char    recv_cmd[2];
                    recv_cmd[0]=0;
                    g_bootRecovery = TRUE;
                    change_cmd_for_recovery(&gBootInfo , recv_cmd); 
                    //loader_tag_set_bootflag(g_bootRecovery,0,0); 
                    if(load_image(pboot_info, pboot_info->index_recovery, pboot_info->ramdisk_load_addr, 0))
                        return;
                }
                //else
                {
                //    return;
                }
            }
		}
	}
#endif
	if( load_image(pboot_info, pboot_info->index_kernel, pboot_info->kernel_load_addr, 0) )
		return;
    
	//RkPrintf("END ===== %d\n", RkldTimerGetTick());
	//PRINT_E("CMDLINE: %s\n", pboot_info->cmd_line);
    powerOn();
    FlashDeInit();
#ifdef RK_LOADER_FOR_FT
    pboot_info->cmd_line[0] = 0; //FT 测试loader，不能用在android上
#endif
	bootm_linux(pboot_info->kernel_load_addr,
				pboot_info->machine_type,
				pboot_info->atag_addr,
				pboot_info->cmd_line);
}

extern uint32 JSHashBase(uint8 * buf,uint32 len,uint32 hash);
int LoadOemImage(uint32 partIndex,uint32 Mode)
{
	int i;
	int err;
	TPIB32_Thdr *thdr = (TPIB32_Thdr*)&usbXferBuf[0]; 
	uint8 * pbuf = (uint8*)&usbXferBuf[0]; 
	TPIB32_Phdr *phdr;
    uint32 BootSnapshotBufLen;
    uint32 BootSnapshotBufFileOffset;
    uint32 BootSnapshotBufOffset;
    void (*theKernel)(void);
    mtd_partition *part = gBootInfo.cmd_mtd.parts+partIndex;
    uint32 partSize;
    uint32 hashCheck = 0;
    uint32 hashData = 0x47C6A7E6;
    
    partSize = (part->size) << 9;
    if(StorageReadLba(part->offset, &usbXferBuf[0], 1) != 0)
    {
        return -1;
    }
    
#ifdef INSTANT_BOOT_EN
    if(Mode==0)
    {
        if (memcmp(thdr->t_ident, "TPIB20", 6))
        {
            return -2;
        }
        powerOn();   //电源先锁住
    }
    else
#endif    
    {
        if (memcmp(thdr->t_ident, "RKOEM10", 7))
        {
            return -2;
        }
        hashCheck = thdr->hash;
    }

    if(StorageReadLba(part->offset + 1, &usbXferBuf[128], 4095) != 0)  //读2MB的数据
    {
        return -3;
    }
    
    BootSnapshotBufFileOffset = 0;
    BootSnapshotBufLen = 4096*512;
    BootSnapshotBufOffset = 0;
        
    for (i = 0; i < thdr->t_phnum; i++) 
    {
        uint32 p_vaddr;
        phdr = &thdr->phdr[i];
        if (phdr->p_memsz == 0)
            continue;
                
        p_vaddr = phdr->p_vaddr;
        if(p_vaddr >=0xC0000000)
            p_vaddr -= 0x60000000;

        if(p_vaddr < 0x60400000 || phdr->p_filesz >= partSize) //参数异常
            return -5;
                
        if(phdr->p_filesz + (phdr->p_offset - BootSnapshotBufFileOffset) > BootSnapshotBufLen)
        {
            uint32 copyLen =  BootSnapshotBufLen - (phdr->p_offset - BootSnapshotBufFileOffset);
            ftl_memcpy((void*)p_vaddr, pbuf + BootSnapshotBufOffset + (phdr->p_offset - BootSnapshotBufFileOffset),copyLen);
            if(phdr->p_filesz - copyLen > 512)
            {
                uint32 ReadLen = (phdr->p_filesz - copyLen)>>9;
                if(StorageReadLba(part->offset + ((BootSnapshotBufFileOffset + BootSnapshotBufLen)>>9), (void*)(p_vaddr + copyLen), ReadLen) != 0)
                {
                    return -4;
                }
                copyLen += (ReadLen<<9);
                BootSnapshotBufFileOffset += (ReadLen<<9);
            }
            
            if(StorageReadLba(part->offset + ((BootSnapshotBufFileOffset + BootSnapshotBufLen)>>9), &usbXferBuf[1024*256], 2048) != 0)  //读1MB的数据
            {
                return -4;
            }

            BootSnapshotBufFileOffset += BootSnapshotBufLen;
            BootSnapshotBufLen = 2048*512;
            BootSnapshotBufOffset = 1024*1024;
            if(phdr->p_filesz - copyLen)
            {
                ftl_memcpy((void*)(p_vaddr + copyLen), pbuf + BootSnapshotBufOffset, phdr->p_filesz - copyLen);
            }
        }
        else
        {
            ftl_memcpy((void*)p_vaddr, pbuf + BootSnapshotBufOffset + (phdr->p_offset - BootSnapshotBufFileOffset), phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz) 
        {
           ftl_memset((void*)(p_vaddr + phdr->p_filesz), 0,phdr->p_memsz - phdr->p_filesz);
        }
        if(hashCheck)
        {
            hashData = JSHashBase((void*)p_vaddr,phdr->p_filesz,hashData);
        }
    }
#ifdef INSTANT_BOOT_EN
    if(Mode==0 && thdr->t_entry)
    {
        theKernel = (void (*)(void))thdr->t_entry;
        if((uint32)theKernel >= 0xC0000000)
           (uint32)theKernel -= 0x60000000;
        RkPrintf("BootSnapshot addr 0x%x ...\n",(uint32)theKernel);
        _cleanup_before_linux ();
        //进入 SVC模式.
#ifndef __GNUC__
        __asm { MSR CPSR_c,#0xd3 }; //SVC , NO IRQ, FIQ.
#endif
        /* we assume that the kernel is in place */
        theKernel();
    }else
#endif
    if(Mode==1 && thdr->t_entry)
    {
        theKernel = (void (*)(void))thdr->t_entry;
        if(hashCheck)
        {
            if(hashCheck != hashData)
               return -6;
        }
        theKernel();
    }
    return 0;
}

#ifdef INSTANT_BOOT_EN
void rknand_print_hex_data(uint8 *s,uint8 * buf,uint32 len)
{
    uint32 i,j,count;
    RkPrintf("%s",s);
    for(i=0;i<len;i+=4)
    {
       RkPrintf("%x %x %x %x",buf[i],buf[i+1],buf[i+2],buf[i+3]);
    } 
}

int BootSnapshot(PBootInfo pboot_info)
{
	if(pboot_info->index_snapshot)
    {
        FW_ReIntForUpdate();
        return(LoadOemImage(pboot_info->index_snapshot,0));
    }
}
#endif

#ifdef USE_RECOVER	
// 以'\n'分隔的多个命令
int execute_cmd(PBootInfo pboot_info, char* cmdlist, bool* reboot)
{
	char* cmd = cmdlist;

	*reboot = FALSE;
	while(*cmdlist)
	{
		if(*cmdlist=='\n') *cmdlist='\0';
		++cmdlist;
	}
	
	while(*cmd)
	{
		PRINT_I("bootloader cmd: %s\n", cmd);
		if( !strcmp(cmd, "recover-recovery") )// 修复 recovery 分区
		{
			if( part_recover(pboot_info, pboot_info->index_recovery) )
				return -1;
		}
		else if( !strcmp(cmd, "recover-boot") )// 修复 boot 分区
		{
			if( part_recover(pboot_info, pboot_info->index_boot) )
				return -1;
		}
		else if( !strcmp(cmd, "recover-kernel") )// 修复 kernel 分区
		{
			if( part_recover(pboot_info, pboot_info->index_kernel) )
				return -1;
		}
		else if( !strcmp(cmd, "recover-system") )// 修复 system 分区
		{
			int index = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_SYSTEM);
			if(index < 0)
				PRINT_E("Can't find part(%s)\n", PARTNAME_SYSTEM);
			else if( part_recover(pboot_info, index) )//index >= 0 
				return -1;
		}
		else if( !strcmp(cmd, "update-bootloader") )// 升级 bootloader
		{
		    PRINT_I("--- update bootloader ---\n");
			if( update_loader()==0 )
			{// cmy: 升级完成后重启
			    *reboot = TRUE;
            }
			else
			{// cmy: 升级失败
				return -1;
		    }
		}
		else
			PRINT_I("Unsupport cmd: %s\n", cmd);
			
		cmd += strlen(cmd)+1;
	}

	return 0;
}
#endif

//#pragma arm section code
