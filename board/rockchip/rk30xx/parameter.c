#include <asm/arch/rk30_drivers.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <environment.h>
#include <asm/processor.h>
#include <asm/io.h>
#include "getmtdpart.h"
#include "parameter.h"
#include "maptable.h"

uint32 gParamBuffer[MAX_LOADER_PARAM/4];	// 限制最大的Parameters的字节数为:32*512
BootInfo gBootInfo;
cmdline_mtd_partition *g_mymtd = &(gBootInfo.cmd_mtd);
uint32 parameter_lba;
extern uint32 g_reserveblock;
extern uint32 CRC_32CheckBuffer( unsigned char * aData, unsigned long aSize );

uint32 str2hex(char *str)
{
    int32 i=0;
    uint32 value = 0;

    if(*str == '0' && ( *(str+1) == 'x' || *(str+1) == 'X' ) )
        str += 2;
    if( *str == 'x' || *str == 'X' )
        str += 1;

    for(i=0; *str!='\0'; i++,++str) 
    { 
        if(*str>='0' && *str<='9') 
            value = value*16+*str-'0'; 
        else if(*str>='a' && *str<='f') 
            value = value*16+*str-'a'+10; 
        else if(*str>='A' && *str<='F') 
            value = value*16+*str-'A'+10;
        else// 其它字符
            break;
    }
    return value;
}

char * find_prefix(char * buf, const char * fmt)
{
    if(strstr(buf, fmt) == buf)
    {
        buf += strlen(fmt);
        EATCHAR(buf, ' ');
        return buf;
    }
    return NULL;
}

int find_mtd_part(cmdline_mtd_partition* this_mtd, const char* part_name)
{
    int i=0;
    for(i=0; i<this_mtd->num_parts; i++)
    {
        if( !strcmp(part_name, this_mtd->parts[i].name) )
        {
            //printf("%s:offset = [%x]; size = [%x]\n", this_mtd->parts[i].name, this_mtd->parts[i].offset, this_mtd->parts[i].size);
            return i;
        }
    }

    return -1;
}

int parse_cmdline(PBootInfo pboot_info)
{
    // cmy: 解析命令行，获取分区信息
    pboot_info->cmd_mtd.num_parts = 0;
    pboot_info->cmd_mtd.mtd_id[0]='\0';

    if( mtdpart_parse(pboot_info->cmd_line, &pboot_info->cmd_mtd) )
    {		
        pboot_info->index_misc = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_MISC);
        pboot_info->index_kernel = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_KERNEL);
        pboot_info->index_boot= find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_BOOT);
        pboot_info->index_recovery = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_RECOVERY);		
        pboot_info->index_system = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_SYSTEM);
        pboot_info->index_cache = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_CACHE);
        pboot_info->index_kpanic = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_KPANIC);
        pboot_info->index_user_data = find_mtd_part(&pboot_info->cmd_mtd, PARTNAME_USERDATA);

        if(pboot_info->index_misc < 0 ||
                pboot_info->index_kernel < 0 ||
                pboot_info->index_boot < 0 ||
                pboot_info->index_recovery < 0
                ||pboot_info->index_system < 0)
        {
            return -1;
        }
    }
    else
    {
        printf("ERROR: parse cmdline failed!");
        return -2;
    }

    return 0;
}

void ParseLine(PBootInfo pboot_info, char *line)
{
    char * text;
    if(text = find_prefix(line, "MAGIC:"))
    {
        pboot_info->magic_code = str2hex(text);
    }
    else if(text = find_prefix(line, "MACHINE:"))
    {
        pboot_info->machine_type = (uint16)str2hex(text);
    }
    else if(text = find_prefix(line, "UBOOT_MASK:"))
    {
        pboot_info->uboot_mask = str2hex(text);
    }
    else if(text = find_prefix(line, "KERNEL_IMG:"))
    {
        pboot_info->kernel_load_addr = str2hex(text);
    }
    else if(text = find_prefix(line, "UBOOTDELAY:"))
    {
        strcpy(pboot_info->uboot_autoboottime, text);
    }
    /*
       else if(text = find_prefix(line, "BOOT_IMG:"))
       {
       pboot_info->boot_offset = str2hex(text);
       }
       else if(text = find_prefix(line, "RECOVERY_IMG:"))
       {
       pboot_info->recovery_offset = str2hex(text);
       }
       else if(text = find_prefix(line, "MISC_IMG:"))
       {
       pboot_info->misc_offset = str2hex(text);
       }
       */
    else if(text = find_prefix(line, "UBOOTMEDIA:"))
    {
        strcpy(pboot_info->bootmedia, text);
    }
    else if(text = find_prefix(line, "ETHADDR:"))
    {
        strcpy(pboot_info->ethaddr, text);
    }
    else if(text = find_prefix(line, "IPADDR:"))
    {
        strcpy(pboot_info->ipaddr, text);
    }
    else if(text = find_prefix(line, "SERVERIP:"))
    {
        strcpy(pboot_info->serverip, text);
    }
    else if( text = find_prefix(line, "CMDLINE:"))
    {
        strcpy( pboot_info->cmd_line, text );
        parse_cmdline(pboot_info);
    }
    else if( text = find_prefix(line,"FLAG:"))
    { printf("FLAG: %s\n",text);}
    else if( text = find_prefix(line,"FW_VERSION:"))
    {printf("FW_VERSION: %s\n",text);}
    else
        printf("Unknow param!\n");
}

//
// param  字符串
// line   获取到的一行数据存放在该变量中
// 返回值 偏移的位置
//
char* getline(char* param, int32 len, char *line)
{
    int i=0;
    //char *string=param;

    for(i=0; i<len; i++)
    {
        if(param[i]=='\n' || param[i]=='\0')
            break;

        if(param[i] != '\r')
            *(line++) = param[i];
    }

    *line='\0';

    return param+i+1;
}

int CheckParam(PLoaderParam pParam)
{
    uint32 crc = 0;
    //	PRINT_D("Enter\n");

    if( pParam->tag != 0xFFFFFFFF && pParam->tag != 0 )
    {
        if(pParam->tag != PARM_TAG)
        {
            printf("Warning: Parameter's tag not match(0x%08X)!\n", pParam->tag);
            return -2;
        }

        if( pParam->length > (MAX_LOADER_PARAM-12) )
        {
            printf("ERROR: Invalid parameter length(%d)!\n", pParam->length);
            return -3;
        }

        crc = CRC_32CheckBuffer(pParam->parameter, pParam->length+4);
        if(!crc)
        {
            printf("ERROR: CRC check failed!\n");
            return -4;
        }
    }
    else
    {
        printf("Warning: No parameter or Parameter's tag not match!\n");
        return -1;
    }

    return 0;
}

int32 GetParam(uint32 *buf)
{
    /*
    PLoaderParam param = (PLoaderParam)buf;
    int iResult = 0;
    int i=0;
    int iRet = 0;
    struct mtd_info *mtd = &nand_info[0];
    struct nand_chip *chip= nand_info[0].priv;
    uint32 LBAAddr,PBAAddr;
    int read_len = MAX_LOADER_PARAM;

    LBAAddr = g_reserveblock*mtd->erasesize + 0;//从原先的烧写机制上看parameter数据必定写在lba = 0的地址

    PBAAddr = getPBA(LBAAddr);

    // 功能未验证
    for(i=0; i<PARAMETER_NUM; i++)
    {
        printf("TRY[%d]\n", i);
        if(0 == nand_read_skip_bad(mtd, PBAAddr, &read_len,(u_char *)buf))
        {
            iResult = CheckParam(param);
            if(iResult >= 0)
            {
                return 0;
            }
            else
            {
                printf("Invalid parameter file\n");
                iRet = -1;
            }
        }
        else
        {
            printf("nand_read_skip_bad error !!\n");
            iRet = -2;
        }

        printf("Parameter read failed\n");
    }

    //	PRINT_D("Leave with 0\n");
    return iRet;
    */
    //TODO:read from emmc
    return 0;
}

extern unsigned long memparse (char *ptr, char **retptr);

int initrd_parse(const char* string, uint32_t* pInitstart, uint32_t* pInitlen)
{
    char *token = NULL;
    char *s=NULL;
    char *p = NULL,*q=NULL;
    char cmdline[MAX_LINE_CHAR];
    char initrd_start[16],initrd_len[16];


    strcpy(cmdline, string);

    token = strtok(cmdline, " ");
    while(token != NULL)
    {
        if( !strncmp(token, "initrd=", strlen("initrd=")) )
        {
            s = token + strlen("initrd=");
            break;

        }
        token = strtok(NULL, " ");
    }

    if(s != NULL)
    {
        if (!(p = strchr(s, ',')))
            return 0;

        strncpy(initrd_start, s, p-s);
#if 0
        if(!(q=strchr(p,' ')))
            return 0;
#endif

        q=strchr(p,' ');

        strncpy(initrd_len, ++p, q-p);	

    }

    //    printf("initrd_start  = [%s], initrd_len=[%s],p=[%s],q=[%s] \n",initrd_start,initrd_len,p,q);


    *pInitstart = memparse(initrd_start,&initrd_start);

    *pInitlen = memparse(initrd_len,&initrd_len);
    //printf("pInitstart = 0x%x, pInitlen=0x%x \n",*pInitstart ,*pInitlen);

    return 1;

}
void ParseParam(PBootInfo pboot_info, char *param, uint32 len)
{
    // 一行最多1024Bytes
    char *prev_param=NULL;
    char line[MAX_LINE_CHAR] = "\0";
    int32 remain_len = (int32)len;

    //	RkPrintf("Enter\n");
    //	RkPrintf("--------------- PARAM ---------------\n");
    while(remain_len>0)
    {
        // 获取一行数据(不含回车换行符，且左边不含空格)
        prev_param = param;
        param = getline(param, remain_len, line);
        remain_len -= (param-prev_param);

        //		RkPrintf("%s\n", line);
        // 去除空行及注释行
        if( line[0] != 0 && line[0] != '#' )
        {// 该行不是空行，并且不是注释行
            ParseLine(pboot_info, line);
        }
    }
    //	RkPrintf("--------------------------------------\n");
    //	RkPrintf("Leave\n");
}

int32 rk30_bootParameter(void)
{

    uint8 *	buf1 = (uint8 *)&gParamBuffer[0];
    char  * buf2 = (uint8 *)&gParamBuffer[MAX_LOADER_PARAM/8];
    uint32   flashAddr, initrd_start, initrd_len;
    uint32  lls = 0;

    /*
    struct mtd_info *mtd = &nand_info[0];
    struct nand_chip *chip= nand_info[0].priv;


    if(gBootInfo.uboot_mask&0x1)//工具之前是使用512为单位
    {
        lls = 9;
    }

    if( initrd_parse(&(gBootInfo.cmd_line),&initrd_start,&initrd_len))
    {

        if(initrd_start>0x60000000)
        {
            flashAddr =  (gBootInfo.cmd_mtd.parts[gBootInfo.index_boot].offset<<lls) ;
            printf("rk30_getParameter:flashAddr=0x%x\n",flashAddr);
            chip->select_chip(mtd, 0);
            memset((u_char *)initrd_start,0xFF,initrd_len);
            nand_read_skip_bad(mtd,flashAddr,&initrd_len,(u_char *)initrd_start);
        }
    }


    flashAddr =  gBootInfo.cmd_mtd.parts[gBootInfo.index_kernel].offset<<lls; 

    if(strcmp(gBootInfo.bootmedia,"nand")!=0)
    {
        setenv("ethaddr", gBootInfo.ethaddr);
        printf("ethaddr = [%s]\n", gBootInfo.ethaddr);

        setenv("ipaddr", gBootInfo.ipaddr);
        printf("ipaddr = [%s]\n", gBootInfo.ipaddr);

        setenv("serverip", gBootInfo.serverip);  
        printf("serverip = [%s]\n", gBootInfo.serverip);

    }*/
    //TODO:use emmc

    sprintf(buf1, "%s", gBootInfo.bootmedia);
    sprintf(buf2, " read 0x%X 0x%X 0x%X; bootm 0x%X", gBootInfo.kernel_load_addr, flashAddr, (gBootInfo.cmd_mtd.parts[gBootInfo.index_kernel].size<<lls), gBootInfo.kernel_load_addr);
    strcat(buf1, buf2);
    setenv("bootcmd", buf1);
    printf("bootcmd = [%s]\n", buf1);

    setenv("bootargs", gBootInfo.cmd_line);
    printf("bootargs = [%s]\n", gBootInfo.cmd_line);

    setenv("bootdelay", gBootInfo.uboot_autoboottime);

    return 0;
}

int rk30_get_system_parameter(int32 *flashAddr,int32 *flashSize)
{
    uint32  lls = 0;

    if(gBootInfo.uboot_mask&0x1)//工具之前是使用512为单位
    {
        lls = 9;
    }

    *flashAddr =  gBootInfo.cmd_mtd.parts[gBootInfo.index_system].offset<<lls; 
    *flashSize = gBootInfo.cmd_mtd.parts[gBootInfo.index_system].size<<lls; 


    return 0;

}

