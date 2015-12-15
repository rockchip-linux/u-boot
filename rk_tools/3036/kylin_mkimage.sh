#!/bin/sh
PROGRAM=${0##*/}
LOCAL_PATH=${0%/*}

LOADER=${1:-RK3036_uboot.img}

rm ${LOADER}

echo Generating ${LOADER}...
kylin/tools/mkimage -T rksd -d kylin/spl/u-boot-spl.bin ${LOCAL_PATH}/out
cat ${LOCAL_PATH}/out kylin/u-boot-dtb.bin >> ${LOADER}
truncate -s "%4000K" ${LOADER}
echo Done...
