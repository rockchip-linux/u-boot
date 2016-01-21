#!/bin/sh
PROGRAM=${0##*/}
LOCAL_PATH=${0%/*}

LOADER=${1:-RK3036_uboot.img}

rm ${LOADER}

echo Generating ${LOADER}...
tools/mkimage -n rk3036 -T rksd -d spl/u-boot-spl.bin ${LOCAL_PATH}/out
cat ${LOCAL_PATH}/out u-boot-dtb.bin >> ${LOADER}
truncate -s "%4000K" ${LOADER}
echo Done...
