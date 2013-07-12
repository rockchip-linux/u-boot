/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    fat.C
Author:		    XUESHAN LIN
Created:		1st JUL 2007
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#define	    IN_DRIVE
#include    "../../armlinux/config.h"
#include    "fatInclude.h"		//FAT头文件

/*********************************************************************************************************
函数描述:挂载驱动器
入口参数:盘符
出口参数:驱动器信息
调用函数:
说    明:系统盘为DISK_SYS, FLASH盘为DISK_FLASH, SD卡盘为DISK_SD
***************************************************************************/
pDRIVE_INFO Mount(uint8 Drive)
{
    pDRIVE_INFO Rt;
    Rt = NULL;
    switch (Drive)
    {
        case DISK_SYS:
            Rt = DriveInfo+Drive;
            if (Rt->Valid == 0)
            {
                DriveInfo[DISK_SYS].Valid=0;
                //DriveInfo[DISK_FLASH].Valid=0;
            }
            Rt = DriveInfo+Drive;
            if (Rt->Valid == 0)
            {
                Rt->Valid=1;
                Rt->DirDeep=0;      //初始目录深度为0即根目录
                Rt->FreeClus=-1;    //在每次挂载后必须重新计算剩余容量
                Rt->FdtRef.DirClus=-1;
                if (OK != CheckFileSystem(Drive))
                {
                    //StorageInit();//zyf 测试
                    //if (OK != CheckFileSystem(Drive))
                    {
                        Rt = NULL;
                    }
                }
            }
            break;
        default:
            break;
    }
    return ((pDRIVE_INFO)Rt);
}


/*********************************************************************************************************
函数描述:卸载驱动器
入口参数:盘符
出口参数:无
调用函数:
说    明:系统盘为DISK_SYS, FLASH盘为DISK_FLASH, SD卡盘为DISK_SD
***************************************************************************/
void Demount(uint8 Drive)
{
    if (Drive < MAX_DRIVE)
        DriveInfo[Drive].Valid=0;
}


/*********************************************************************************************************
函数描述:获取驱动器信息
入口参数:盘符
出口参数:驱动器信息
调用函数:
说    明:系统盘FLASH为'C'
***************************************************************************/
pDRIVE_INFO GetDriveInfo(uint8 Drive)
{
    pDRIVE_INFO Rt;
    
    Rt = NULL;
    if (Drive < MAX_DRIVE)
    {
        if (DriveInfo[Drive].Valid != 0x00) //未使用的驱动器
        {
            Rt = DriveInfo+Drive;
        }
    }
    return (Rt);
}


/*********************************************************************************************************
函数描述:获取驱动器
入口参数:Path
出口参数:盘符
调用函数:
说    明:第一个盘符为'C'
***************************************************************************/
uint8 GetDrive(char *Path)
{
    uint8 Drive;

	Drive=CurDrive;
	if (Path != NULL)
	{
    	if (Path[1] == ':')
    	{
            if (Path[0]>='c' && Path[0]<='z')
                Drive=Path[0]-'c';
            else if (Path[0]>='C' && Path[0]<='Z')
                Drive=Path[0]-'C';
            CurDrive = Drive;
    	}
	}
	return (Drive);
}


/*********************************************************************************************************
函数描述:获取驱动器容量
入口参数:Drive
出口参数:盘符
调用函数:
说    明:
***************************************************************************/
uint32 GetCapacity(uint8 Drive)
{
    return (256*2048);
    //return (FtlGetCapacity(Drive));
}


