/********************************************************************************
*********************************************************************************
                        COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
                                --  ALL RIGHTS RESERVED  --

File Name:      session.C
Author:         HSL
Created:        2009.03.19
Modified:
Revision:       1.00
********************************************************************************
********************************************************************************/
#include    "config.h"
#include 	"session.h"

#ifdef LINUX_BOOT
static session_head 	curren_sh;	//当前 SESSION 信息
static uint32 			xfer_len;	// 已经传输的数据长度.
static uint32			real_sdram_addr;
static uint32			real_sdram_addr_bak;

__align(8)
static file_info			current_file_info;

/********************************************************************/
extern void boot_setfile_info(  char* filename , uint32 addr , uint32 size );
extern uint32 CacheFlushDRegion(uint32 adr, uint32 size);

/********************************************************************/
void boot_session_sendstop(void)
{

	uint8 *tmp=BulkInBuf;
	session_stop ss;
	ftl_memcpy(tmp, (uint8*)&gCSW, sizeof(gCSW));
	ss.status = 0;	// stop 
	ss.function = curren_sh.function ; //Swap16( curren_sh.function);
	ss.Code = curren_sh.Code;
	ss.error = 0;	// 0: OK , else : error

	ftl_memcpy(tmp+sizeof(gCSW), (uint8*)&ss, sizeof(session_stop) );
	WriteBulkEndpoint( sizeof(gCSW)+sizeof(session_stop)  , tmp);
	FWCmdPhase=K_CommandPhase;
	FW_WR_Mode = FW_WR_MODE_LBA;

	// 文件传输完成，进行注册.
	if( curren_sh.function == SESSION_FUN_TRANSFILE )
		{
		boot_setfile_info( current_file_info.fname , real_sdram_addr_bak , current_file_info.fsize );
		current_file_info.fname[0] = 0;	//clear for next file 
		}
	else if( curren_sh.function == SESSION_FUN_TRANSPARAM)	//由于DMA 传输最少需要 4BYTE对其
		{
		((char*)real_sdram_addr_bak)[ curren_sh.transLen ] = 0;
		}

}

// 处理 SESSION 操作中DO 中断.
void boot_session_dataout( uint16 len )
{
//	RkPrintf("len:0x%x,trans:0x%x,total:0x%x\n", len , xfer_len ,curren_sh.transLen);
	
	CacheFlushDRegion((uint32)real_sdram_addr,(uint32)len );
     	//ftl_memcpy((void*)real_sdram_addr,(uint8*)DataBuf, len );
	xfer_len += len ;
	real_sdram_addr += len;
	
	if( xfer_len >= curren_sh.transLen )
		{
	//	RkPrintf("send stop,%x,%x" , xfer_len ,curren_sh.transLen);
		//发送 session stop pack.
		CSWHandler(CSW_GOOD ,0);
		///ReadBulkEndpoint( len , DataBuf); //for next command.
        ReadBulkEndpoint(31, (uint8*)&gCBW);
		/* set the 0 for string file '\0' */
		*((char*)real_sdram_addr) = 0;
		boot_session_sendstop();
		}
	else
		{
		//for next pack .
		//FW_DataLenCnt = 0;
		    ReadBulkEndpoint( len , (void*)real_sdram_addr);
		}

}

void boot_session_datain( void )
{
}

void boot_session_handle( void )
{
	//curren_sh =*( (session_head*)&gCBW.Code);
	ftl_memcpy( (char*)&curren_sh , (char*)DataBuf+STRUCT_OFFSET(CBW,Code) , sizeof(session_head) );
	
	RkPrintf("code:%x,func=%d,translen=%x\n", curren_sh.Code ,curren_sh.function , curren_sh.transLen );
	xfer_len = 0;
	
	if( curren_sh.status == 0 )
		{
		goto Error;
		return ;
		}
	
	switch ( curren_sh.function )
		{
		case SESSION_FUN_TRANSFILE:	// second .
			{
			session_transfile *sf = (session_transfile*)(&curren_sh.content[0]);
			if( curren_sh.transLen != current_file_info.fsize || current_file_info.fname[0] == 0 )	//必须和第一个匹配.
				goto Error;

			if( sf->sdram_lba < SDRAM_BASE_ADDRESS )	//上限无法判断.
				goto Error;
			
			// 考虑到使用方便，地址直接使用物理地址,而不是 LBA地址.
			real_sdram_addr_bak = real_sdram_addr = sf->sdram_lba ; //(sf->sdram_lba<<9)+SDRAM_BASE_ADDRESS;
			//real_sdram_addr_bak = real_sdram_addr = (sf->sdram_lba<<9)+SDRAM_BASE_ADDRESS;
			
			}
			break;
		case SESSION_FUN_TRANSPARAM:	//文件信息,first .
			{
			if( curren_sh.transLen >= sizeof(current_file_info) )
				goto Error;
			
			real_sdram_addr_bak = real_sdram_addr = (uint32)&current_file_info;

			// 增加文件名结束符 0.--在传输结束后增加.
			//((char*)real_sdram_addr)[ curren_sh.transLen ] = 0;
			//memset( (char*)real_sdram_addr , 0x00 , sizeof(current_file_info) );
			}
			break;
		default:
			goto Error;
		}
	
	if(  curren_sh.transLen > 0 )
	{
		FW_WR_Mode = FW_WR_MODE_SESSION;
		FW_DataLenCnt = 0;
		FWCmdPhase = K_OutDataPhase;
		ReadBulkEndpoint(curren_sh.transLen, (void*)real_sdram_addr);
	}
	else
	{
		CSWHandler(CSW_GOOD ,0);
		boot_session_sendstop();
	}
	return ;
Error :	
	CSWHandler(CSW_FAIL,0);
	SendCSW();
}
#endif
