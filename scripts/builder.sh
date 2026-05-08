#!/bin/bash
#
# Copyright (c) 2025 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#

set -e

dedupe_ignore_keys=(
	CONFIG_TRUST_INI
	CONFIG_LOADER_INI
)

platform_list=(
	rk3538
	rk3572
	rk3576
	rk3588
)

config_group_key()
{
	local configfile=$1
	local ignore_expr

	ignore_expr=$(printf '%s|' "${dedupe_ignore_keys[@]}")
	ignore_expr=${ignore_expr%|}

	grep -v -E "^(${ignore_expr})=" "${configfile}" | sed '/^$/d' | sort | sha1sum | awk '{print $1}'
}

for platform in "${platform_list[@]}"; do
	declare -A built_group=()
	declare -A built_group_repr=()

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
				group_key=$(config_group_key "$configfile")

				if [ -n "${built_group[$group_key]}" ]; then
					echo
					echo "##################### Skipping: ${config_name} (same build as ${built_group_repr[$group_key]}) #####################"
					continue
				fi

				built_group[$group_key]=1
				built_group_repr[$group_key]=${config_name}
				echo
				echo "##################### Building: ${config_name} (BASE=${base_defconfig}) #####################"
				make distclean
				./make.sh "${config_name}"
			fi
		fi
	done
done
