#include <linux/types.h>
#include <u-boot/crc.h>
#include <asm/arch/rk30_drivers.h>


uint32 CRC_32CheckBuffer( unsigned char * aData, unsigned long aSize )
{
    uint32 crc = 0;
    int i=0;
    if( aSize <= 4 )
    {
        return 0;
    }
    aSize -= 4;

    for(i=3; i>=0; i--)
        crc = (crc<<8)+(*(aData+aSize+i));

    if( crc32(0, aData, aSize) == crc )
        return crc;

    return 0;
}
