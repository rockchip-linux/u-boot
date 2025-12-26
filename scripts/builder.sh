#!/bin/bash
#
# Copyright (c) 2025 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#

set -e

platform_list=(
	rk3538
	rk3576
	rk3588
)

for platform in "${platform_list[@]}"; do
	echo
	echo "##################### Building: ${platform} #####################"
	make distclean
	./make.sh "${platform}"
done
