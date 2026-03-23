#!/bin/bash
#
# Copyright (c) 2020 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#

srctree=$PWD

function help()
{
	echo
	echo "Description:"
	echo "    Process args for all rockchip fit generator script, and providing variables for it's caller"
	echo
	echo "Usage:"
	echo "    $0 [args]"
	echo
	echo "[args]:"
	echo "--------------------------------------------------------------------------------------------"
	echo "    arg                 type       output variable       description"
	echo "--------------------------------------------------------------------------------------------"
	echo "    -c [comp]     ==>   <string>   COMPRESSION           set compression: \"none\", \"gzip\""
	echo "    -m0 [offset]  ==>   <hex>      MCU0_LOAD_ADDR        set mcu0.bin load address"
	echo "    -m1 [offset]  ==>   <hex>      MCU1_LOAD_ADDR        set mcu1.bin load address"
	echo "    -m2 [offset]  ==>   <hex>      MCU2_LOAD_ADDR        set mcu2.bin load address"
	echo "    -m3 [offset]  ==>   <hex>      MCU3_LOAD_ADDR        set mcu3.bin load address"
	echo "    -m4 [offset]  ==>   <hex>      MCU4_LOAD_ADDR        set mcu4.bin load address"
	echo "    -l0 [offset]  ==>   <hex>      LOAD0_LOAD_ADDR       set load0.bin load address"
	echo "    -l1 [offset]  ==>   <hex>      LOAD1_LOAD_ADDR       set load1.bin load address"
	echo "    -l2 [offset]  ==>   <hex>      LOAD2_LOAD_ADDR       set load2.bin load address"
	echo "    -l3 [offset]  ==>   <hex>      LOAD3_LOAD_ADDR       set load3.bin load address"
	echo "    -l4 [offset]  ==>   <hex>      LOAD4_LOAD_ADDR       set load4.bin load address"
	echo "    -t [offset]   ==>   <hex>      TEE_LOAD_ADDR         set tee.bin load address"
	echo "    (none)        ==>   <hex>      UBOOT_LOAD_ADDR       set U-Boot load address"
	echo "    (none)        ==>   <string>   ARCH                  set arch: \"arm\", \"arm64\""
	echo
}

DRAM_BASE=`sed -n "/CONFIG_SYS_SDRAM_BASE=/s/CONFIG_SYS_SDRAM_BASE=//p" ${srctree}/include/autoconf.mk|tr -d '\r'`

if [ $# -eq 1 ]; then
	# default
	TEE_OFFSET=0x08400000
	TEE_LOAD_ADDR="0x"$(echo "obase=16;$((DRAM_BASE+TEE_OFFSET))"|bc)
else
	# args
	while [ $# -gt 0 ]; do
		case $1 in
			--help|-help|help|--h|-h)
				help
				exit
				;;
			-c)
				COMPRESSION=$2
				shift 2
				;;
			-i0)
				INIT0_LOAD_ADDR=$2
				shift 2
				;;
			-m0)
				MCU0_LOAD_ADDR=$2
				shift 2
				;;
			-m1)
				MCU1_LOAD_ADDR=$2
				shift 2
				;;
			-m2)
				MCU2_LOAD_ADDR=$2
				shift 2
				;;
			-m3)
				MCU3_LOAD_ADDR=$2
				shift 2
				;;
			-m4)
				MCU4_LOAD_ADDR=$2
				shift 2
				;;
			-l0)
				LOAD0_LOAD_ADDR=$2
				shift 2
				;;
			-l1)
				LOAD1_LOAD_ADDR=$2
				shift 2
				;;
			-l2)
				LOAD2_LOAD_ADDR=$2
				shift 2
				;;
			-l3)
				LOAD3_LOAD_ADDR=$2
				shift 2
				;;
			-l4)
				LOAD4_LOAD_ADDR=$2
				shift 2
				;;
			-t)
				TEE_LOAD_ADDR=$2

				if ! grep -q '^CONFIG_MOS_SUPPORT=y' .config ; then
					# Compatible leagcy: Offset
					if ((TEE_LOAD_ADDR < DRAM_BASE));  then
						TEE_LOAD_ADDR="0x"$(echo "obase=16;$((DRAM_BASE+$2))"|bc)
					fi
				fi
				shift 2
				;;
			*)
				echo "Invalid arg: $1"
				help
				exit 1
				;;
		esac
	done
fi

if ! grep -q '^CONFIG_FIT_OMIT_UBOOT=y' .config ; then
	UBOOT_LOAD_ADDR=`sed -n "/CONFIG_SYS_TEXT_BASE=/s/CONFIG_SYS_TEXT_BASE=//p" ${srctree}/include/autoconf.mk|tr -d '\r'`
fi

# ARCH
U_ARCH="arm"
if grep -q '^CONFIG_ARM64=y' .config ; then
	ARCH="arm64"
	U_ARCH="arm64"
elif grep -q '^CONFIG_ARM64_BOOT_AARCH32=y' .config ; then
	ARCH="arm64"
else
	ARCH="arm"
fi
