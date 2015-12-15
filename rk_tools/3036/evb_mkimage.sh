#!/bin/sh
PROGRAM=${0##*/}
LOCAL_PATH=${0%/*}

LOADER=${1:-RK3036_uboot.img}

rm ${LOADER}

echo Generating ${LOADER}...
evb_3036/tools/mkimage -T rksd -d evb_3036/spl/u-boot-spl.bin ${LOCAL_PATH}/out
cat ${LOCAL_PATH}/out evb_3036/u-boot-dtb.bin >> ${LOADER}
echo Done...
