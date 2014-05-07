#ifndef _CHIP_DEPEND_H
#define _CHIP_DEPEND_H

//定义Loader启动异常类型
#define SYS_LOADER_ERR_FLAG      0X1888AAFF 

extern void FW_NandDeInit(void);

extern void ISetLoaderFlag(uint32 flag);
extern uint32 IReadLoaderFlag(void);

extern void rkplat_uart2UsbEn(uint32 en);

#endif
