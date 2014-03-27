#ifndef _MK_VARIOUS_FILE_H_
#define _MK_VARIOUS_FILE_H_

#define MAXBLINE				200
#define MAX_TAG_LINES			50

#define MAX_MACHINE_MODEL		64
#define MAX_MANUFACTURER		64

#define MAX_UINT			0xFFFFFFFF

#define PART_NAME				32
#define FILE_NAME				32
#define MAX_PARTS				20
#define MAX_MTDID				64

#define TAG_BEGIN				"BEGIN"
#define TAG_END					"END"

#define TAG_RAM					"RAM"
#define TAG_PARTITIONS			"PARTITIONS"
#define TAG_MACHINE_INFO		"MACHINE_INFO"

#define PARTNAME_PARAMETER		"parameter"
#define PARTNAME_MISC			"misc"
#define PARTNAME_KERNEL			"kernel"
#define PARTNAME_BOOT			"boot"
#define PARTNAME_RECOVERY		"recovery"
#define PARTNAME_SYSTEM			"system"
#define PARTNAME_BACKUP			"backup"
//#define PARTNAME_FW_BACKUP		"fwbackup"


#define ROOT_NFS(str, nfs) sprintf(str, "root=/dev/nfs nfsroot=%s:%s ip=%s:%s:%s:%s:rk28:eth0:off", \
								(nfs)->server_ip, (nfs)->nfs_dir, (nfs)->target_ip, (nfs)->server_ip, (nfs)->default_gw, (nfs)->netmask)
#define ROOT_FLASH				"root=/dev/mtdblock"
#define CMDLINE_FILE			"cmdline"

#define PARAMETER_FILE			"parameter"
#define PARAMETER_NFS_FILE		"parameter-nfs"
#define PARAMETER_RAM_FILE		"parameter-ram"
#define PARAMETER_SIMPLE_FILE	"parameter-simp"

#define RUN_FLASH				"run"
#define RUN_NFS					"run-nfs"
#define RUN_RAM					"run-ram"
#define RUN_SIMPLE				"run-simp"

#define KERNEL_IMG				"kernel.img"
#define MYROOTFS_IMG			"myrootfs.img"

#define FLG_PART_FILE			1<<0
#define FLG_PART_HIDE			1<<1

// 去除字符串x中最左边连续为c的字符
#define EATCHAR_L(x, c)	do{\
							for (; *(x) == (c); (x)++);\
						  }while(0)

// 去除字符串x中最右边连续为c的字符
#define EATCHAR_R(x, c)	do{ \
							char *_p_=x+strlen(x)-1; \
							for (; *(_p_) == (c); *(_p_)='\0',(_p_)--); \
						  }while(0)

typedef struct  
{
	char server_ip[16];
	char target_ip[16];
	char default_gw[16];
	char netmask[16];
	char nfs_dir[512];
}NFS_INFO;

typedef struct{
	int size;
	unsigned long base_addr;
	unsigned long atag_addr;
	unsigned long krnl_addr;
}RAMINFO;

typedef struct  
{
	unsigned long magic;
	unsigned long machine_id;
	char manufacturer[MAX_MANUFACTURER];
	char machine_model[MAX_MACHINE_MODEL];
}MACHINEINFO;

typedef struct  
{
	char name[PART_NAME];
	unsigned long offset;
	unsigned long size;
	int flag;
}PARTINFO;

typedef struct  
{
	char mtd_id[MAX_MTDID];
	int part_num;
	PARTINFO parts[MAX_PARTS];
}PARTSTABLE;

#endif
