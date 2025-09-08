#!/bin/bash
#
# Copyright (c) 2022 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#
set -e

FIT_DIR="fit"
IMG_UBOOT="uboot.img"
IMG_BOOT="boot.img"
IMG_RECOVERY="recovery.img"
ITB_UBOOT="${FIT_DIR}/uboot.itb"
ITB_BOOT="${FIT_DIR}/boot.itb"
ITB_RECOVERY="${FIT_DIR}/recovery.itb"
SIG_BIN="data2sign.bin"
SIG_UBOOT="${FIT_DIR}/uboot.data2sign"
SIG_BOOT="${FIT_DIR}/boot.data2sign"
SIG_RECOVERY="${FIT_DIR}/recovery.data2sign"
SIG_CFG_DIR="${FIT_DIR}/fit_signcfg"
SIG_CONFIG="${SIG_CFG_DIR}/sign.readonly_config"
MINIALL_INI="${SIG_CFG_DIR}/MINIALL.ini"
DTC_TOOL="${SIG_CFG_DIR}/dtc"
# offs
OFFS_DATA="0x1200"
# placeholder address
FDT_ADDR_PLACEHOLDER="0xffffff00"
KERNEL_ADDR_PLACEHOLDER="0xffffff01"
RAMDISK_ADDR_PLACEHOLDER="0xffffff02"
# tools
MKIMAGE="./tools/mkimage"
RK_SIGN_TOOL="../rkbin/tools/rk_sign_tool"
FIT_UNPACK="./scripts/fit-unpack.sh"
CHECK_SIGN="./tools/fit_check_sign"
# key
KEY_DIR="keys/"
RSA_PRI_KEY="keys/dev.key"
RSA_PUB_KEY="keys/dev.pubkey"
RSA_CRT_KEY="keys/dev.crt"
LEGACY_RSA_PRI_KEY="legacy_keys/dev.key"
LEGACY_RSA_PUB_KEY="legacy_keys/dev.pubkey"
LEGACY_RSA_CRT_KEY="legacy_keys/dev.crt"
SIGNATURE_KEY_NODE="/signature/key-dev"
SPL_DTB="spl/u-boot-spl.dtb"
UBOOT_DTB="u-boot.dtb"
# its
ITS_UBOOT="u-boot.its"
ITS_BOOT="boot.its"
ITS_RECOVERY="recovery.its"
ARG_VER_UBOOT="0"
ARG_VER_BOOT="0"
ARG_VER_RECOVERY="0"

function help()
{
	echo
	echo "usage:"
	echo "    $0 [args]"
	echo
	echo "args:"
	echo "    --rollback-index-recovery  <decimal integer>"
	echo "    --rollback-index-boot      <decimal integer>"
	echo "    --rollback-index-uboot     <decimal integer>"
	echo "    --version-recovery         <decimal integer>"
	echo "    --version-boot             <decimal integer>"
	echo "    --version-uboot            <decimal integer>"
	echo "    --boot_img                 <boot image>"
	echo "    --recovery_img             <recovery image>"
	echo "    --args                     <arg>"
	echo "    --ini-loader               <loader ini file>"
	echo "    --ini-trust                <trust ini file>"
	echo "    --no-check"
	echo "    --no-sign"
	echo "    --spl-new"
	echo
}

function arg_check_decimal()
{
	if [ -z $1 ]; then
		help
		exit 1
	fi

	decimal=`echo $1 |sed 's/[0-9]//g'`
	if [ ! -z ${decimal} ]; then
		echo "ERROR: $1 is not decimal integer"
		help
		exit 1
	fi
}

function check_its()
{
	cat $1 | while read line
	do
		file=`echo ${line} | sed -n "/incbin/p" | awk -F '"' '{ printf $2 }' | tr -d ' '`
		if [ ! -f ${file} ]; then
			echo "ERROR: No ${file}"
			exit 1
		fi
	done
}

function check_rsa_algo()
{
	if grep -q '^CONFIG_FIT_ENABLE_RSA4096_SUPPORT=y' .config ; then
		rsa_algo="rsa4096"
	else
		rsa_algo="rsa2048"
	fi
	if ! grep -qr ${rsa_algo} $1 ; then
		echo "ERROR: Wrong rsa_algo in its file. It should be ${rsa_algo}."
		exit 1
	fi
}

function check_rsa_keys()
{
	if [ ! -f ${RSA_PRI_KEY} ]; then
		echo "ERROR: No ${RSA_PRI_KEY} "
		exit 1
	elif [ ! -f ${RSA_PUB_KEY} ]; then
		echo "ERROR: No ${RSA_PUB_KEY} "
		exit 1
	elif [ ! -f ${RSA_CRT_KEY} ]; then
		echo "ERROR: No ${RSA_CRT_KEY} "
		exit 1
	fi
}

function validate_arg()
{
	case $1 in
		--no-check|--no-sign|--spl-new|--burn-key-hash)
			shift=1
			;;
		--ini-trust|--ini-loader|--rollback-index-boot|--rollback-index-recovery|--rollback-index-uboot|--boot_img|--recovery_img|--version-uboot|--version-boot|--version-recovery|--chip)
			shift=2
			;;
		*)
			shift=0
			;;
	esac
	echo ${shift}
}

function fit_process_args()
{
	if [ $# -eq 0 ]; then
		help
		exit 0
	fi

	if grep -q '^CONFIG_FIT_SIGNATURE=y' .config ; then
		ARG_SIGN="y"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--args)
				ARG_VALIDATE=$2
				shift 2
				;;
			--boot_img)     # boot.img
				ARG_BOOT_IMG=$2
				shift 2
				;;
			--chip)
				ARG_CHIP=$2
				shift 2
				;;
			--recovery_img) # recovery.img
				ARG_RECOVERY_IMG=$2
				shift 2
				;;
			--boot_img_dir) # boot.img components directory
				ARG_BOOT_IMG_DIR=$2
				shift 2
				;;
			--no-check)     # No hostcc fit signature check
				ARG_NO_CHECK="y"
				shift 1
				;;
			--no-sign)
				ARG_NO_SIGN="y"
				ARG_SIGN="n"
				shift 1
				;;
			--ini-trust)    # Assign trust ini file
				ARG_INI_TRUST=$2
				shift 2
				;;
			--ini-loader)   # Assign loader ini file
				ARG_INI_LOADER=$2
				shift 2
				;;
			--spl-new)      # Use current build u-boot-spl.bin to pack loader
				ARG_SPL_NEW="y"
				# Whether aarch32 or not, spl only support 64 bits version.
				if grep -q '^CONFIG_ARM64_BOOT_AARCH32=y' .config ; then
					echo "ERROR: SPL doesn't support 32-bit. Please build 64-bit defconfig and update u-boot-spl.bin to rkbin first."
					exit 1
				fi
				shift 1
				;;
			--rollback-index-boot)
				ARG_ROLLBACK_IDX_BOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--rollback-index-recovery)
				ARG_ROLLBACK_IDX_RECOVERY=$2
				arg_check_decimal $2
				shift 2
				;;
			--rollback-index-uboot)
				ARG_ROLLBACK_IDX_UBOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--version-uboot)
				ARG_VER_UBOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--version-boot)
				ARG_VER_BOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--version-recovery)
				ARG_VER_RECOVERY=$2
				arg_check_decimal $2
				shift 2
				;;
			--burn-key-hash)
				ARG_BURN_KEY_HASH="y"
				shift 1
				;;
			--spl-fwver)
				ARG_FIT_FWVER="${ARG_FIT_FWVER} --spl-fwver $2"
				shift 2
				;;
			--fwver)
				ARG_FIT_FWVER="${ARG_FIT_FWVER} --fwver $2"
				shift 2
				;;
			*)
				help
				exit 1
				;;
		esac
	done
}

function fit_raw_compile()
{
	# Verified-boot: should rebuild code but don't need to repack images.
	if [ "${ARG_SIGN}" == "y" ]; then
		./make.sh --raw-compile ${ARG_FIT_FWVER}
	fi
	rm ${FIT_DIR} -rf && mkdir -p ${FIT_DIR} && mkdir -p ${SIG_CFG_DIR}
}

function fit_gen_uboot_itb()
{
	# generate u-boot.its file
	./make.sh itb ${ARG_INI_TRUST}

	# check existance of file in its
	check_its ${ITS_UBOOT}

	if [ "${ARG_SIGN}" != "y" ]; then
		${MKIMAGE} -f ${ITS_UBOOT} -E -p ${OFFS_DATA} ${ITB_UBOOT} -v ${ARG_VER_UBOOT}
		if [ "${ARG_SPL_NEW}" == "y" ]; then
			./make.sh --spl ${ARG_INI_LOADER}
			echo "pack loader with new: spl/u-boot-spl.bin"
		else
			./make.sh loader ${ARG_INI_LOADER}
		fi
	else
		check_rsa_keys

		if ! grep -q '^CONFIG_SPL_FIT_SIGNATURE=y' .config ; then
			echo "ERROR: CONFIG_SPL_FIT_SIGNATURE is disabled"
			exit 1
		fi

		# rollback-index
		if grep -q '^CONFIG_SPL_FIT_ROLLBACK_PROTECT=y' .config ; then
			ARG_SPL_ROLLBACK_PROTECT="y"
			if [ -z ${ARG_ROLLBACK_IDX_UBOOT} ]; then
				echo "ERROR: No arg \"--rollback-index-uboot <n>\""
				exit 1
			fi
		fi

		if [ "${ARG_SPL_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`grep 'rollback-index' ${ITS_UBOOT} | awk -F '=' '{ printf $2 }' | tr -d ' '`
			sed -i "s/rollback-index = ${VERSION}/rollback-index = <${ARG_ROLLBACK_IDX_UBOOT}>;/g" ${ITS_UBOOT}
		fi

		# Generally, boot.img is signed before uboot.img, so the ras key can be found
		# in u-boot.dtb. If not found, let's insert rsa key anyway.
		if ! fdtget -l ${UBOOT_DTB} /signature >/dev/null 2>&1 ; then
			${MKIMAGE} -f ${ITS_UBOOT} -k ${KEY_DIR} -K ${UBOOT_DTB} -E -p ${OFFS_DATA} -r ${ITB_UBOOT} -v ${ARG_VER_UBOOT}
			echo "## Adding RSA public key into ${UBOOT_DTB}"
		fi

		# Pack
		${MKIMAGE} -f ${ITS_UBOOT} -k ${KEY_DIR} -K ${SPL_DTB} -E -p ${OFFS_DATA} -r ${ITB_UBOOT} -v ${ARG_VER_UBOOT}
		mv ${SIG_BIN} ${SIG_UBOOT}

		# burn-key-hash
		if [ "${ARG_BURN_KEY_HASH}" == "y" ]; then
			if grep -q '^CONFIG_SPL_FIT_HW_CRYPTO=y' .config ; then
				fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} burn-key-hash 0x1
			else
				echo "ERROR: --burn-key-hash requires CONFIG_SPL_FIT_HW_CRYPTO=y"
				exit 1
			fi
		fi

		# rollback-index read back check
		if [ "${ARG_SPL_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`fdtget -ti ${ITB_UBOOT} /configurations/conf rollback-index`
			if [ "${VERSION}" != "${ARG_ROLLBACK_IDX_UBOOT}" ]; then
				echo "ERROR: Failed to set rollback-index for ${ITB_UBOOT}";
				exit 1
			fi
		fi

		# burn-key-hash read back check
		if [ "${ARG_BURN_KEY_HASH}" == "y" ]; then
			if [ "`fdtget -ti ${SPL_DTB} ${SIGNATURE_KEY_NODE} burn-key-hash`" != "1" ]; then
				echo "ERROR: Failed to set burn-key-hash for ${SPL_DTB}";
				exit 1
			fi
		fi

		# host check signature
		if [ "${ARG_NO_CHECK}" != "y" ]; then
			if [ "${ARG_SPL_NEW}" == "y" ]; then
				 ${CHECK_SIGN} -f ${ITB_UBOOT} -k ${SPL_DTB} -s
			else
				spl_file="../rkbin/"`sed -n "/FlashBoot=/s/FlashBoot=//p" ${ARG_INI_LOADER}  |tr -d '\r'`
				offs=`fdtdump -s ${spl_file} | head -1 | awk -F ":" '{ print $2 }' | sed "s/ found fdt at offset //g" | tr -d " "`
				if [ -z ${offs}  ]; then
					echo "ERROR: invalid ${spl_file} , unable to find fdt blob"
				fi
				offs=`printf %d ${offs} ` # hex -> dec
				dd if=${spl_file} of=spl/u-boot-spl-old.dtb bs=${offs} skip=1 >/dev/null 2>&1
				${CHECK_SIGN} -f ${ITB_UBOOT} -k spl/u-boot-spl-old.dtb -s
			fi
		fi

		# minimize u-boot-spl.dtb: clear as 0 but not remove property.
		if grep -q '^CONFIG_SPL_FIT_HW_CRYPTO=y' .config ; then
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,r-squared 0x0
			if grep -q '^CONFIG_SPL_ROCKCHIP_CRYPTO_V1=y' .config ; then
				fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
				fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@np
			else
				fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
				fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@c
			fi
		else
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,exponent-BN 0x0
			fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@c
			fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@np
		fi

		# repack spl
		if [ "${ARG_SPL_NEW}" == "y" ]; then
			cat spl/u-boot-spl-nodtb.bin > spl/u-boot-spl.bin
			if ! grep -q '^CONFIG_SPL_SEPARATE_BSS=y' .config ; then
				cat spl/u-boot-spl-pad.bin >> spl/u-boot-spl.bin
			fi
			cat ${SPL_DTB} >> spl/u-boot-spl.bin

			./make.sh --spl ${ARG_INI_LOADER}
			echo "## pack loader with new: spl/u-boot-spl.bin"
		else
			./make.sh loader ${ARG_INI_LOADER}
		fi

		if [ "${ARG_BURN_KEY_HASH}" == "y" ]; then
			echo "## ${SPL_DTB}: burn-key-hash=1"
		fi
	fi

	rm -f u-boot.itb u-boot.img u-boot-dtb.img
	mv ${ITS_UBOOT} ${FIT_DIR}
}

function fit_gen_boot_itb()
{
	if [ ! -z ${ARG_BOOT_IMG} ]; then
		${FIT_UNPACK} -f ${ARG_BOOT_IMG} -o ${FIT_DIR}/unpack
		ITS_BOOT="${FIT_DIR}/unpack/image.its"
	else
		compression=`awk -F"," '/COMPRESSION=/  { printf $1 }' ${ARG_INI_TRUST} | tr -d ' ' | cut -c 13-`
		if [ -z "${compression}" ]; then
			compression="none"
		fi
		./arch/arm/mach-rockchip/make_fit_boot.sh -c ${compression} > ${ITS_BOOT}
		check_its ${ITS_BOOT}
	fi

	if [ "${ARG_SIGN}" != "y" ]; then
		${MKIMAGE} -f ${ITS_BOOT} -E -p ${OFFS_DATA} ${ITB_BOOT} -v ${ARG_VER_BOOT}
	else
		check_rsa_keys

		check_rsa_algo ${ITS_BOOT}

		if [ "${ARG_SIGN}" != "y" ]; then
			echo "ERROR: CONFIG_FIT_SIGNATURE is disabled"
			exit 1
		fi

		if grep -q '^CONFIG_FIT_ROLLBACK_PROTECT=y' .config ; then
			ARG_ROLLBACK_PROTECT="y"
			if [ -z ${ARG_ROLLBACK_IDX_BOOT} ]; then
				echo "ERROR: No arg \"--rollback-index-boot <n>\""
				exit 1
			fi
			if ! grep -q '^CONFIG_OPTEE_CLIENT=y' .config ; then
				echo "ERROR: Don't support \"--rollback-index-boot <n>\""
				exit 1
			fi
		fi

		# fixup
		FDT_ADDR_R=`strings env/built-in.o | grep 'fdt_addr_r=' | awk -F "=" '{ print $2 }'`
		KERNEL_ADDR_R=`strings env/built-in.o | grep 'kernel_addr_r=' | awk -F "=" '{ print $2 }'`
		RMADISK_ADDR_R=`strings env/built-in.o | grep 'ramdisk_addr_r=' | awk -F "=" '{ print $2 }'`
		sed -i "s/${FDT_ADDR_PLACEHOLDER}/${FDT_ADDR_R}/g"         ${ITS_BOOT}
		sed -i "s/${KERNEL_ADDR_PLACEHOLDER}/${KERNEL_ADDR_R}/g"   ${ITS_BOOT}
		sed -i "s/${RAMDISK_ADDR_PLACEHOLDER}/${RMADISK_ADDR_R}/g" ${ITS_BOOT}

		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`grep 'rollback-index' ${ITS_BOOT} | awk -F '=' '{ printf $2 }' | tr -d ' '`
			sed -i "s/rollback-index = ${VERSION}/rollback-index = <${ARG_ROLLBACK_IDX_BOOT}>;/g" ${ITS_BOOT}
		fi

		${MKIMAGE} -f ${ITS_BOOT} -k ${KEY_DIR} -K ${UBOOT_DTB} -E -p ${OFFS_DATA} -r ${ITB_BOOT} -v ${ARG_VER_BOOT}
		mv ${SIG_BIN} ${SIG_BOOT}

		# rollback-index read back check
		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`fdtget -ti ${ITB_BOOT} /configurations/conf rollback-index`
			if [ "${VERSION}" != "${ARG_ROLLBACK_IDX_BOOT}" ]; then
				echo "ERROR: Failed to set rollback-index for ${ITB_BOOT}";
				exit 1
			fi
		fi

		# host check signature
		if [ "${ARG_NO_CHECK}" != "y" ]; then
			 ${CHECK_SIGN} -f ${ITB_BOOT} -k ${UBOOT_DTB}
		fi

		# minimize u-boot.dtb: clearn as 0 but not remove property.
		if grep -q '^CONFIG_FIT_HW_CRYPTO=y' .config ; then
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,r-squared 0x0
			if grep -q '^CONFIG_ROCKCHIP_CRYPTO_V1=y' .config ; then
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			else
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fi
		else
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,exponent-BN 0x0
		fi
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@c
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@np
	fi

	mv ${ITS_BOOT} ${FIT_DIR}
}

function fit_gen_recovery_itb()
{
	if [ ! -z ${ARG_RECOVERY_IMG} ]; then
		${FIT_UNPACK} -f ${ARG_RECOVERY_IMG} -o ${FIT_DIR}/unpack
		ITS_RECOVERY="${FIT_DIR}/unpack/image.its"
	else
		echo "ERROR: No recovery.img"
		exit 1
	fi

	if [ "${ARG_SIGN}" != "y" ]; then
		${MKIMAGE} -f ${ITS_RECOVERY} -E -p ${OFFS_DATA} ${ITB_RECOVERY} -v ${ARG_VER_RECOVERY}
	else
		check_rsa_keys

		check_rsa_algo ${ITS_RECOVERY}

		if [ "${ARG_SIGN}" != "y" ]; then
			echo "ERROR: CONFIG_FIT_SIGNATURE is disabled"
			exit 1
		fi

		if grep -q '^CONFIG_FIT_ROLLBACK_PROTECT=y' .config ; then
			ARG_ROLLBACK_PROTECT="y"
			if [ -z ${ARG_ROLLBACK_IDX_RECOVERY} ]; then
				echo "ERROR: No arg \"--rollback-index-recovery <n>\""
				exit 1
			fi
			if ! grep -q '^CONFIG_OPTEE_CLIENT=y' .config ; then
				echo "ERROR: Don't support \"--rollback-index-recovery <n>\""
				exit 1
			fi
		fi

		# fixup
		FDT_ADDR_R=`strings env/built-in.o | grep 'fdt_addr_r=' | awk -F "=" '{ print $2 }'`
		KERNEL_ADDR_R=`strings env/built-in.o | grep 'kernel_addr_r=' | awk -F "=" '{ print $2 }'`
		RMADISK_ADDR_R=`strings env/built-in.o | grep 'ramdisk_addr_r=' | awk -F "=" '{ print $2 }'`
		sed -i "s/${FDT_ADDR_PLACEHOLDER}/${FDT_ADDR_R}/g"         ${ITS_RECOVERY}
		sed -i "s/${KERNEL_ADDR_PLACEHOLDER}/${KERNEL_ADDR_R}/g"   ${ITS_RECOVERY}
		sed -i "s/${RAMDISK_ADDR_PLACEHOLDER}/${RMADISK_ADDR_R}/g" ${ITS_RECOVERY}

		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`grep 'rollback-index' ${ITS_RECOVERY} | awk -F '=' '{ printf $2 }' | tr -d ' '`
			sed -i "s/rollback-index = ${VERSION}/rollback-index = <${ARG_ROLLBACK_IDX_RECOVERY}>;/g" ${ITS_RECOVERY}
		fi

		${MKIMAGE} -f ${ITS_RECOVERY} -k ${KEY_DIR} -K ${UBOOT_DTB} -E -p ${OFFS_DATA} -r ${ITB_RECOVERY} -v ${ARG_VER_RECOVERY}
		mv ${SIG_BIN} ${SIG_RECOVERY}

		# rollback-index read back check
		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`fdtget -ti ${ITB_RECOVERY} /configurations/conf rollback-index`
			if [ "${VERSION}" != "${ARG_ROLLBACK_IDX_RECOVERY}" ]; then
				echo "ERROR: Failed to set rollback-index for ${ITB_RECOVERY}";
				exit 1
			fi
		fi

		# host check signature
		if [ "${ARG_NO_CHECK}" != "y" ]; then
			 ${CHECK_SIGN} -f ${ITB_RECOVERY} -k ${UBOOT_DTB}
		fi

		# minimize u-boot.dtb: clearn as 0 but not remove property.
		if grep -q '^CONFIG_FIT_HW_CRYPTO=y' .config ; then
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,r-squared 0x0
			if grep -q '^CONFIG_ROCKCHIP_CRYPTO_V1=y' .config ; then
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			else
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fi
		else
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,exponent-BN 0x0
		fi
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@c
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@np
	fi

	mv ${ITS_RECOVERY} ${FIT_DIR}
}

function fit_gen_uboot_img()
{
	ITB=$1

	if [ -z ${ITB} ]; then
		ITB=${ITB_UBOOT}
	fi

	ITB_MAX_NUM=`sed -n "/SPL_FIT_IMAGE_MULTIPLE/p" .config | awk -F "=" '{ print $2 }'`
	ITB_MAX_KB=`sed  -n "/SPL_FIT_IMAGE_KB/p" .config | awk -F "=" '{ print $2 }'`
	ITB_MAX_BS=$((ITB_MAX_KB*1024))
	ITB_BS=`ls -l ${ITB} | awk '{ print $5 }'`

	if [ ${ITB_BS} -gt ${ITB_MAX_BS} ]; then
		echo "ERROR: pack ${IMG_UBOOT} failed! ${ITB} actual: ${ITB_BS} bytes, max limit: ${ITB_MAX_BS} bytes"
		exit 1
	fi

	rm -f ${IMG_UBOOT}
	for ((i = 0; i < ${ITB_MAX_NUM}; i++));
	do
		cat ${ITB} >> ${IMG_UBOOT}
		truncate -s %${ITB_MAX_KB}K ${IMG_UBOOT}
	done
}

function fit_gen_boot_img()
{
	ITB=$1

	if [ -z ${ITB} ]; then
		ITB=${ITB_BOOT}
	fi

	if [ "${ITB}" != "${IMG_BOOT}" ]; then
		cp ${ITB} ${IMG_BOOT} -f
	fi
}

function fit_gen_recovery_img()
{
	ITB=$1

	if [ -z ${ITB} ]; then
		ITB=${ITB_RECOVERY}
	fi

	if [ "${ITB}" != "${IMG_RECOVERY}" ]; then
		cp ${ITB} ${IMG_RECOVERY} -f
	fi
}

function fit_gen_loader()
{
	if [ "${ARG_SIGN}" == "y" ]; then
		${RK_SIGN_TOOL} cc --chip ${ARG_CHIP: 2: 6}
		if grep -q '^CONFIG_SPL_REVOKE_PUB_KEY=y' .config ; then
			${RK_SIGN_TOOL} lk --key ${LEGACY_RSA_PRI_KEY} --pubkey ${LEGACY_RSA_PUB_KEY}
			${RK_SIGN_TOOL} ss --flag=0x80
		else
			${RK_SIGN_TOOL} lk --key ${RSA_PRI_KEY} --pubkey ${RSA_PUB_KEY}
		fi
		if ls *loader*.bin >/dev/null 2>&1 ; then
			${RK_SIGN_TOOL} sl --loader *loader*.bin
		fi
		if ls *download*.bin >/dev/null 2>&1 ; then
			${RK_SIGN_TOOL} sl --loader *download*.bin
		fi
		if ls *idblock*.img >/dev/null 2>&1 ; then
			${RK_SIGN_TOOL} sb --idb *idblock*.img
		fi
	fi
}

function fit_msg_uboot()
{
	if [ "${ARG_SIGN}" != "y" ]; then
		MSG_SIGN="no-signed"
	else
		MSG_SIGN="signed"
	fi

	VERSION=`fdtget -ti ${ITB_UBOOT} / version`
	if [ "${VERSION}" != "" ]; then
		MSG_VER=", version=${VERSION}"
	fi

	if [ "${ARG_SPL_ROLLBACK_PROTECT}" == "y" ]; then
		echo "Image(${MSG_SIGN}${MSG_VER}, rollback-index=${ARG_ROLLBACK_IDX_UBOOT}): ${IMG_UBOOT} (with uboot, trust...) is ready"
	else
		echo "Image(${MSG_SIGN}${MSG_VER}): ${IMG_UBOOT} (FIT with uboot, trust...) is ready"
	fi
}

function fit_msg_boot()
{
	if [ -z "${ARG_BOOT_IMG}" ]; then
		return;
	fi

	if [ "${ARG_SIGN}" != "y" ]; then
		MSG_SIGN="no-signed"
	else
		MSG_SIGN="signed"
	fi

	VERSION=`fdtget -ti ${ITB_BOOT} / version`
	if [ "${VERSION}" != "" ]; then
		MSG_VER=", version=${VERSION}"
	fi

	if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
		echo "Image(${MSG_SIGN}${MSG_VER}, rollback-index=${ARG_ROLLBACK_IDX_BOOT}): ${IMG_BOOT} is ready"
	else
		echo "Image(${MSG_SIGN}${MSG_VER}): ${IMG_BOOT} (FIT with kernel, fdt, resource...) is ready"
	fi
}

function fit_msg_recovery()
{
	if [ -z "${ARG_RECOVERY_IMG}" ]; then
		return;
	fi

	if [ "${ARG_SIGN}" != "y" ]; then
		MSG_SIGN="no-signed"
	else
		MSG_SIGN="signed"
	fi

	VERSION=`fdtget -ti ${ITB_RECOVERY} / version`
	if [ "${VERSION}" != "" ]; then
		MSG_VER=", version=${VERSION}"
	fi

	if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
		echo "Image(${MSG_SIGN}${MSG_VER}, rollback-index=${ARG_ROLLBACK_IDX_RECOVERY}): ${IMG_RECOVERY} is ready"
	else
		echo "Image(${MSG_SIGN}${MSG_VER}): ${IMG_RECOVERY} (FIT with kernel, fdt, resource...) is ready"
	fi
}

function fit_msg_loader()
{
	if ls *loader*.bin >/dev/null 2>&1 ; then
		LOADER=`ls *loader*.bin`
	fi

	if ls *idblock*.img >/dev/null 2>&1 ; then
		LOADER=`ls *idblock*.img`
	fi

	if [ "${ARG_SIGN}" == "y" ]; then
		echo "Image(signed): ${LOADER} (with spl, ddr...) is ready"
	else
		echo "Image(no-signed): ${LOADER} (with spl, ddr...) is ready"
	fi
}

function fit_msg_u_boot_loader()
{
	if ls *loader*.bin >/dev/null 2>&1 ; then
		LOADER=`ls *loader*.bin`
	fi

	if ls *idblock*.img >/dev/null 2>&1 ; then
		LOADER=`ls *idblock*.img`
	fi

	if [ "${ARG_SIGN}" == "y" ]; then
		echo "Image(signed): ${LOADER} (with u-boot, ddr...) is ready"
	else
		echo "Image(no-signed): ${LOADER} (with u-boot, ddr...) is ready"
	fi
}

function fit_signcfg_export()
{
	if [ "${ARG_NO_SIGN}" == "y" ]; then
		if ls *loader*.bin >/dev/null 2>&1 ; then
			LOADER=`ls *loader*.bin`
		elif ls *download*.bin >/dev/null 2>&1 ; then
			LOADER=`ls *download*.bin`
		else
			echo "ERROR: No loader found"
			exit 1
		fi
		cp ${ARG_INI_LOADER} ${MINIALL_INI}
		cp .config ${SIG_CONFIG}
		cp ./scripts/dtc/dtc ${DTC_TOOL}

		mkdir -p ${SIG_CFG_DIR}/test_images/
		cp uboot.img ${SIG_CFG_DIR}/test_images/
		cp ${LOADER} ${SIG_CFG_DIR}/test_images/
		tar zcvf ${SIG_CFG_DIR}/test_images.tar.gz ${SIG_CFG_DIR}/test_images >/dev/null 2>&1
		rm -rf ${SIG_CFG_DIR}/test_images/

		FDT_ADDR_R=`strings env/built-in.o | grep 'fdt_addr_r=' | awk -F "=" '{ print $2 }'`
		KERNEL_ADDR_R=`strings env/built-in.o | grep 'kernel_addr_r=' | awk -F "=" '{ print $2 }'`
		RMADISK_ADDR_R=`strings env/built-in.o | grep 'ramdisk_addr_r=' | awk -F "=" '{ print $2 }'`
		echo "fdt_addr_r=${FDT_ADDR_R}" >> ${SIG_CONFIG}
		echo "kernel_addr_r=${KERNEL_ADDR_R}" >> ${SIG_CONFIG}
		echo "ramdisk_addr_r=${RMADISK_ADDR_R}" >> ${SIG_CONFIG}

		CSUM=`sha256sum u-boot-nodtb.bin  | awk '{ print $1 }'`
		echo "uboot_sha256sum=${CSUM}" >> ${SIG_CONFIG}
		CSUM=`sha256sum spl/u-boot-spl-nodtb.bin  | awk '{ print $1 }'`
		echo "spl_sha256sum=${CSUM}" >> ${SIG_CONFIG}
		SIZE=`ls -l  spl/u-boot-spl-nodtb.bin | awk '{ print $5 }'`
		echo "spl_size=${SIZE}" >> ${SIG_CONFIG}

		BUILD_MAIL=`git config --get user.email`
		BUILD_HOST=`hostname`
		BUILD_USER=${USER}
		BUILD_DATE=`date`
		echo "BUILD: ${BUILD_MAIL} # ${BUILD_USER}@${BUILD_HOST} # ${BUILD_DATE}" >> ${SIG_CONFIG}
	fi
}
