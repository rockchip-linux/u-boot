#!/bin/bash
PROGRAM=${0##*/}
LOCAL_PATH=${0%/*}

LOADER=${1:-RK3036_uboot.img}

function usage()
{
	echo "Script to flash rk3036 loader image."
	echo "usage:"
	echo -e "\t${0} <loader image path>"
	echo -e "\tdefault is ${LOADER}"
}

if [ ! -f ${LOADER} ];then
	echo "${LOADER} not exist!"
	usage
	exit -1
fi

echo "Waiting for rockchip device..."

while ! sudo lsusb -d 2207:301a ; do sleep .5; done

echo "Flashing ${LOADER}..."

sudo ${LOCAL_PATH}/upgrade_tool db ${LOCAL_PATH}/rk3036_boot.bin
sudo ${LOCAL_PATH}/upgrade_tool wl 64 ${LOADER}
sudo ${LOCAL_PATH}/upgrade_tool rd
