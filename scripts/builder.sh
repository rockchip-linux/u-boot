#!/bin/bash
#
# Copyright (c) 2025 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#

set -e

platform_list=(
	rk3538
	rk3572
	rk3576
	rk3588
)

for platform in "${platform_list[@]}"; do
	# All xxx_defconfig
	for defconfig in configs/${platform}*_defconfig; do
		if [ -f "$defconfig" ]; then
			config_name=$(basename "$defconfig" _defconfig)
			echo
			echo "##################### Building: ${config_name} #####################"
			make distclean
			./make.sh "${config_name}"
		fi
	done

	# All xxx.config
	for configfile in configs/${platform}*.config; do
		if [ -f "$configfile" ]; then
			base_defconfig=$(grep -E '^CONFIG_BASE_DEFCONFIG=' "$configfile" | cut -d'=' -f2- | tr -d '"')
			if [ -n "$base_defconfig" ]; then
				config_name=$(basename "$configfile" .config)
				echo
				echo "##################### Building: ${config_name} (BASE=${base_defconfig}) #####################"
				make distclean
				./make.sh "${config_name}"
			fi
		fi
	done
done
