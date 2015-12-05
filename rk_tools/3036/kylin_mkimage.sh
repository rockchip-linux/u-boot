#!/bin/sh
PROGRAM=${0##*/}
LOCAL_PATH=${0%/*}

LOADER=${1:-RK3036_uboot.img}

rm ${LOADER}

echo Generating ${LOADER}...
kylin/tools/mkimage -T rksd -d kylin/spl/u-boot-spl.bin ${LOCAL_PATH}/out
dd if=/dev/zero of=${LOADER} count=64
cat ${LOCAL_PATH}/out kylin/u-boot-dtb.bin >> ${LOADER}
echo Done...
