#!/bin/bash
#
# Copyright (C) 2021 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier:     GPL-2.0+
#

# Process args and auto set variables
source ./${srctree}/arch/arm/mach-rockchip/fit_args.sh
rm -f ${srctree}/*.digest ${srctree}/*.bin.gz

# Periph register base
if grep -q '^CONFIG_ROCKCHIP_RK3576=y' .config ; then
MAX_ADDR_VAL=$((0x10000000))
elif grep -q '^CONFIG_ROCKCHIP_RV1126B=y' .config ; then
MAX_ADDR_VAL=$((0x20000000))
elif grep -q '^CONFIG_ROCKCHIP_RV1103B=y' .config ; then
MAX_ADDR_VAL=$((0x20000000))
else
MAX_ADDR_VAL=$((0xf0000000))
fi

# dram base
DRAM_BASE_VAL=`sed -n "s/^CONFIG_DYNAMIC_SDRAM_BASE_DEFAULT=//p" ${srctree}/include/config/auto.conf`
if [ -z "${DRAM_BASE_VAL}" ]; then
	DRAM_BASE_VAL=`sed -n "/define CFG_SYS_SDRAM_BASE /p" ${srctree}/u-boot.cfg | awk '{ print $3 }'`
fi

# compression
if [ "${COMPRESSION}" == "gzip" ]; then
	SUFFIX=".gz"
	COMPRESS_CMD="gzip -kf9"
elif [ "${COMPRESSION}" == "lzma" ]; then
	SUFFIX=".lzma"
	COMPRESS_CMD="${srctree}/scripts/compress.sh lzma"
else
	COMPRESSION="none"
	SUFFIX=
fi

# nodes
function append_list()
{
	if [ -z "$1" ]; then
		echo "$2"
	elif [ -z "$2" ]; then
		echo "$1"
	else
		echo "$1, $2"
	fi
}

function gen_uboot_node()
{
	if [ -z ${UBOOT_LOAD_ADDR} ]; then
		return
	fi

	UBOOT="u-boot-nodtb.bin"
	echo "		uboot {
			description = \"U-Boot\";
			data = /incbin/(\"${UBOOT}${SUFFIX}\");
			type = \"standalone\";
			arch = \"${U_ARCH}\";
			os = \"U-Boot\";
			compression = \"${COMPRESSION}\";
			load = ${FIT_ADDR_PREFIX}<"${UBOOT_LOAD_ADDR}">;"
	if [ "${COMPRESSION}" != "none" ]; then
		openssl dgst -sha256 -binary -out ${UBOOT}.digest ${UBOOT}
		UBOOT_SZ=`ls -l ${UBOOT} | awk '{ print $5 }'`
		if [ ${UBOOT_SZ} -gt 0 ]; then
			${COMPRESS_CMD} ${srctree}/${UBOOT}
		else
			touch ${srctree}/${UBOOT}${SUFFIX}
		fi
		echo "			digest {
				value = /incbin/(\"./${UBOOT}.digest\");
				algo = \"sha256\";
			};"
	fi
	echo "			hash {
				algo = \"sha256\";
			};
		};"

	LOADABLE_UBOOT="\"uboot\""
}

function gen_fdt_node()
{
	if [ -z ${UBOOT_LOAD_ADDR} ]; then
		return
	fi

	echo "		fdt {
			description = \"U-Boot dtb\";
			data = /incbin/(\"./u-boot.dtb\");
			type = \"flat_dt\";
			arch = \"${U_ARCH}\";
			compression = \"none\";
			hash {
				algo = \"sha256\";
			};
		};"

	FDT_SIGN=", \"fdt\""
	FDT="fdt = \"fdt\"${PROP_KERN_DTB};"
};

function gen_kfdt_node()
{
	if [ -z ${UBOOT_LOAD_ADDR} ]; then
		return
	fi

	KERN_DTB=`sed -n "/CONFIG_EMBED_KERNEL_DTB_PATH=/s/CONFIG_EMBED_KERNEL_DTB_PATH=//p" .config | tr -d '"'`
	if [ -z ${KERN_DTB} ]; then
		return;
	fi

	if [ -f ${srctree}/${KERN_DTB} ]; then
	PROP_KERN_DTB=', "kern-fdt"';
	echo "		kern-fdt {
			description = \"${KERN_DTB}\";
			data = /incbin/(\"${KERN_DTB}\");
			type = \"flat_dt\";
			arch = \"${U_ARCH}\";
			compression = \"none\";
			hash {
				algo = \"sha256\";
			};
		};"
	fi
}

function gen_elf_nodes()
{
	local TYPE="$1"
	local NODE_PREFIX
	local DESCRIPTION
	local OS
	local LOADABLE_VAR
	local FILES
	local LOADABLES
	local FIRMWARE
	local FIRMWARE_LOAD_ADDR
	local NUM=1

	case "${TYPE}" in
	bl31)
		DESCRIPTION="ARM Trusted Firmware"
		NODE_PREFIX="atf"
		OS="arm-trusted-firmware"
		LOADABLE_VAR="LOADABLE_ATF"
		;;
	opensbi)
		DESCRIPTION="OpenSBI"
		NODE_PREFIX="opensbi"
		OS="opensbi"
		LOADABLE_VAR="LOADABLE_OPENSBI"
		;;
	*)
		echo "ERROR: Unknown firmware type '${TYPE}'" >&2
		return 1
		;;
	esac

	rm -f ${srctree}/${TYPE}_0x*.bin
	${srctree}/arch/arm/mach-rockchip/decode_elf.py ${TYPE} || return 1
	FILES=`ls -1 -S ${TYPE}_0x*.bin` || return 1

	for FIRMWARE in ${FILES}
	do
		FIRMWARE_LOAD_ADDR=`echo ${FIRMWARE} | awk -F "_" '{ printf $2 }' | awk -F "." '{ printf $1 }'`
		if [ "${COMPRESSION}" != "none" -a ${NUM} -eq 1 ]; then
			openssl dgst -sha256 -binary -out ${FIRMWARE}.digest ${FIRMWARE}
			${COMPRESS_CMD} ${FIRMWARE}

			echo "		${NODE_PREFIX}-${NUM} {
			description = \"${DESCRIPTION}\";
			data = /incbin/(\"./${FIRMWARE}${SUFFIX}\");
			type = \"firmware\";
			arch = \"${ARCH}\";
			os = \"${OS}\";
			compression = \"${COMPRESSION}\";
			load = ${FIT_ADDR_PREFIX}<"${FIRMWARE_LOAD_ADDR}">;"
			echo "			hash {
				algo = \"sha256\";
			};
			digest {
				value = /incbin/(\"./${FIRMWARE}.digest\");
				algo = \"sha256\";
			};
		};"
		else
			echo "		${NODE_PREFIX}-${NUM} {
			description = \"${DESCRIPTION}\";
			data = /incbin/(\"./${FIRMWARE}\");
			type = \"firmware\";
			arch = \"${ARCH}\";
			os = \"${OS}\";
			compression = \"none\";
			load = ${FIT_ADDR_PREFIX}<"${FIRMWARE_LOAD_ADDR}">;"
			echo "			hash {
				algo = \"sha256\";
			};
		};"
		fi

		if [ ${NUM} -gt 1 ]; then
			LOADABLES=`append_list "${LOADABLES}" "\"${NODE_PREFIX}-${NUM}\""`
		fi
		NUM=`expr ${NUM} + 1`
	done
	printf -v "${LOADABLE_VAR}" '%s' "${LOADABLES}"
}

function gen_bl31_node()
{
	gen_elf_nodes "bl31"
}

function gen_bl32_node()
{
	if [ -z ${TEE_LOAD_ADDR} ]; then
		return
	fi

	if [ "${ARCH}" == "arm" ]; then
		# If not AArch32 mode
		if ! grep  -q '^CONFIG_ARM64_BOOT_AARCH32=y' .config ; then
			ENTRY="entry = <"${TEE_LOAD_ADDR}">;"

			# if disable packing tee.bin
			if ! grep -q '^CONFIG_SPL_OPTEE_IMAGE=y' .config ; then
				return
			fi

		fi
	fi

	TEE="tee.bin"
	echo "		optee {
			description = \"TEE\";
			data = /incbin/(\"${TEE}${SUFFIX}\");
			type = \"firmware\";
			arch = \"${ARCH}\";
			os = \"op-tee\";
			compression = \"${COMPRESSION}\";
			${ENTRY}
			load = ${FIT_ADDR_PREFIX}<"${TEE_LOAD_ADDR}">;"
	if [ "${COMPRESSION}" != "none" ]; then
		openssl dgst -sha256 -binary -out ${TEE}.digest ${TEE}
		${COMPRESS_CMD} ${TEE}
		echo "			digest {
				value = /incbin/(\"./${TEE}.digest\");
				algo = \"sha256\";
			};"
	fi
	echo "			hash {
				algo = \"sha256\";
			};
		};"
	LOADABLE_TEE="\"optee\""
	FIRMWARE_TEE="firmware = \"optee\";"
	FIRMWARE_SIGN="\"firmware\""
}

function gen_mcu_node()
{
	for ((i=0, n=0; i<5; i++))
	do
		if [ ${i} -eq 0 ]; then
			MCU_ADDR=${MCU0_LOAD_ADDR}
		elif [ ${i} -eq 1 ]; then
			MCU_ADDR=${MCU1_LOAD_ADDR}
		elif [ ${i} -eq 2 ]; then
			MCU_ADDR=${MCU2_LOAD_ADDR}
		elif [ ${i} -eq 3 ]; then
			MCU_ADDR=${MCU3_LOAD_ADDR}
		elif [ ${i} -eq 4 ]; then
			MCU_ADDR=${MCU4_LOAD_ADDR}
		fi

		if [ -z ${MCU_ADDR} ]; then
			continue
		fi

		MCU_ADDR_VAL=$((MCU_ADDR))
		MCU="mcu${i}"
		echo "		${MCU} {
			description = \"${MCU}\";
			type = \"standalone\";
			arch = \"riscv\";
			load = ${FIT_ADDR_PREFIX}<"${MCU_ADDR}">;"

		# When allow to be compressed?
		# DRAM base < load addr < Periph register base
		# Periph register base < DRAM base < load addr
		if [ "${COMPRESSION}" != "none" -a "$((MCU_ADDR_VAL))" -gt "$((DRAM_BASE_VAL))" ] &&
		   [ "$((DRAM_BASE_VAL))" -gt "$((MAX_ADDR_VAL))" -o "$((MCU_ADDR_VAL))" -lt "$((MAX_ADDR_VAL))" ]; then
				openssl dgst -sha256 -binary -out ${MCU}.bin.digest ${MCU}.bin
				${COMPRESS_CMD} ${MCU}.bin
				echo "			data = /incbin/(\"./${MCU}.bin${SUFFIX}\");
				compression = \"${COMPRESSION}\";
				digest {
					value = /incbin/(\"./${MCU}.bin.digest\");
					algo = \"sha256\";
				};"
		else
			echo "			data = /incbin/(\"./${MCU}.bin\");
			compression = \"none\";"
		fi

		echo "			hash {
				algo = \"sha256\";
			};
		};"

		if [ ${n} -eq 0 ]; then
			STANDALONE_LIST=${STANDALONE_LIST}"\"${MCU}\""
		else
			STANDALONE_LIST=${STANDALONE_LIST}", \"${MCU}\""
		fi
		n=`expr ${n} + 1`

		STANDALONE_SIGN=", \"standalone\""
		STANDALONE_MCU="standalone = ${STANDALONE_LIST};"
	done

	if [ -z ${INIT0_LOAD_ADDR} ]; then
		return
	fi

	INIT="init0"
	echo "		${INIT} {
			description = \"${INIT}\";
			type = \"standalone\";
			arch = \"${ARCH}\";
			load = ${FIT_ADDR_PREFIX}<"${INIT0_LOAD_ADDR}">;
			data = /incbin/(\"./${INIT}.bin\");
			compression = \"none\";
			hash {
				algo = \"sha256\";
			};
		};"
	STANDALONE_MCU="standalone = \"init0\"${STANDALONE_LIST};"
}

function gen_loadable_node()
{
	for ((i=0; i<5; i++))
	do
		if [ ${i} -eq 0 ]; then
			LOAD_ADDR=${LOAD0_LOAD_ADDR}
		elif [ ${i} -eq 1 ]; then
			LOAD_ADDR=${LOAD1_LOAD_ADDR}
		elif [ ${i} -eq 2 ]; then
			LOAD_ADDR=${LOAD2_LOAD_ADDR}
		elif [ ${i} -eq 3 ]; then
			LOAD_ADDR=${LOAD3_LOAD_ADDR}
		elif [ ${i} -eq 4 ]; then
			LOAD_ADDR=${LOAD4_LOAD_ADDR}
		fi

		if [ -z ${LOAD_ADDR} ]; then
			continue
		fi

		LOAD_ADDR_VAL=$((LOAD_ADDR))
		LOAD="load${i}"
		echo "		${LOAD} {
			description = \"${LOAD}\";
			type = \"standalone\";
			arch = \"${ARCH}\";
			load = ${FIT_ADDR_PREFIX}<"${LOAD_ADDR}">;"

		# When allow to be compressed?
		# DRAM base < load addr < Periph register base
		# Periph register base < DRAM base < load addr
		if [ "${COMPRESSION}" != "none" -a "$((MCU_ADDR_VAL))" -gt "$((DRAM_BASE_VAL))" ] &&
		   [ "$((DRAM_BASE_VAL))" -gt "$((MAX_ADDR_VAL))" -o "$((MCU_ADDR_VAL))" -lt "$((MAX_ADDR_VAL))" ]; then
				openssl dgst -sha256 -binary -out ${LOAD}.bin.digest ${LOAD}.bin
				${COMPRESS_CMD} ${LOAD}.bin
				echo "			data = /incbin/(\"./${LOAD}.bin${SUFFIX}\");
				compression = \"${COMPRESSION}\";
				digest {
					value = /incbin/(\"./${LOAD}.bin.digest\");
					algo = \"sha256\";
				};"
		else
			echo "			data = /incbin/(\"./${LOAD}.bin\");
			compression = \"none\";"
		fi

		echo "			hash {
				algo = \"sha256\";
			};
		};"

		LOADABLE_OTHER=`append_list "${LOADABLE_OTHER}" "\"${LOAD}\""`
	done
}

function gen_opensbi_node()
{
	gen_elf_nodes "opensbi"
}

function gen_header()
{
	local TYPE="${1:-atf}"
	local FIRMWARE

	case "${TYPE}" in
	atf)
		FIRMWARE="ATF"
		;;
	opensbi)
		FIRMWARE="OpenSBI"
		;;
	*)
		echo "ERROR: Unknown firmware type '${TYPE}'" >&2
		return 1
		;;
	esac

echo "
/*
 * Copyright (C) 2020 Rockchip Electronic Co.,Ltd
 *
 * Simple U-boot fit source file containing ${FIRMWARE}/OP-TEE/U-Boot/dtb/MCU
 */

/dts-v1/;

/ {
	description = \"FIT Image with ${FIRMWARE}/OP-TEE/U-Boot/MCU\";
	#address-cells = <1>;

	images {
"
}

function gen_riscv_header()
{
	gen_header "opensbi"
}

function gen_firmware_configurations()
{
	local TYPE="$1"
	local PLATFORM
	local FIRMWARE
	local LOADABLE_FIRMWARE
	local ALGO_PADDING
	local ALGO_NAME
	local LOADABLES

	PLATFORM=`sed -n "/CONFIG_DEFAULT_DEVICE_TREE/p" .config | awk -F "=" '{ print $2 }' | tr -d '"'`
	PLATFORM=${PLATFORM##*/}
	case "${TYPE}" in
	arm64)
		FIRMWARE="atf-1"
		LOADABLE_FIRMWARE="${LOADABLE_ATF}"
		;;
	riscv)
		FIRMWARE="opensbi-1"
		LOADABLE_FIRMWARE="${LOADABLE_OPENSBI}"
		;;
	*)
		echo "ERROR: Unknown configuration type '${TYPE}'" >&2
		return 1
		;;
	esac
	if [ -z "${LOADABLE_FIRMWARE}" ]; then
		LOADABLE_UBOOT="\"uboot\""
	fi

	if grep -q '^CONFIG_FIT_ENABLE_RSASSA_PSS_SUPPORT=y' .config ; then
		ALGO_PADDING="				padding = \"pss\";"
	fi
	if grep -q '^CONFIG_FIT_ENABLE_RSA4096_SUPPORT=y' .config ; then
		ALGO_NAME="				algo = \"sha256,rsa4096\";"
	else
		ALGO_NAME="				algo = \"sha256,rsa2048\";"
	fi
	LOADABLES=`append_list "${LOADABLE_UBOOT}" "${LOADABLE_FIRMWARE}"`
	LOADABLES=`append_list "${LOADABLES}" "${LOADABLE_TEE}"`
	LOADABLES=`append_list "${LOADABLES}" "${LOADABLE_OTHER}"`

echo "	};

	configurations {
		default = \"conf\";
		conf {
			description = \"${PLATFORM}\";
			rollback-index = <0x0>;
			firmware = \"${FIRMWARE}\";
			loadables = ${LOADABLES};
			${STANDALONE_MCU}
			${FDT}
			signature {
				${ALGO_NAME}
				${ALGO_PADDING}
				key-name-hint = \"dev\";
				sign-images = \"firmware\", \"loadables\"${FDT_SIGN}${STANDALONE_SIGN};
			};
		};
	};
};
"
}

function gen_arm64_configurations()
{
	gen_firmware_configurations "arm64"
}

function gen_arm_configurations()
{
PLATFORM=`sed -n "/CONFIG_DEFAULT_DEVICE_TREE/p" .config | awk -F "=" '{ print $2 }' | tr -d '"'`
if grep -q '^CONFIG_FIT_ENABLE_RSASSA_PSS_SUPPORT=y' .config ; then
        ALGO_PADDING="                          padding = \"pss\";"
fi
if grep -q '^CONFIG_FIT_ENABLE_RSA4096_SUPPORT=y' .config ; then
	ALGO_NAME="				algo = \"sha256,rsa4096\";"
else
	ALGO_NAME="				algo = \"sha256,rsa2048\";"
fi
if [ ! -z "${LOADABLE_UBOOT}" ] || [ ! -z "${LOADABLE_OTHER}" ]; then
	LOADABLE_UBOOT="\"uboot\""
	LOADABLE_LIST=`append_list "${LOADABLE_UBOOT}" "${LOADABLE_OTHER}"`
	LOADABLES="loadables = ${LOADABLE_LIST};"
	if [ -z ${FIRMWARE_SIGN} ]; then
		LOADABLES_SIGN="\"loadables\""
	else
		LOADABLES_SIGN=", \"loadables\""
	fi
fi

echo "	};

	configurations {
		default = \"conf\";
		conf {
			description = \"${PLATFORM}\";
			rollback-index = <0x0>;
			${FIRMWARE_TEE}
			${LOADABLES}
			${STANDALONE_MCU}
			${FDT}
			signature {
				${ALGO_NAME}
				${ALGO_PADDING}
				key-name-hint = \"dev\";
				sign-images = ${FIRMWARE_SIGN}${LOADABLES_SIGN}${FDT_SIGN}${STANDALONE_SIGN};
			};
		};
	};
};
"
}

function gen_riscv_configurations()
{
	gen_firmware_configurations "riscv"
}
