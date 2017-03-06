#!/bin/sh
CROSS_COMPILE=aarch64-linux-gnu- CFLAGS='-gdwarf-3' make evb-rk3399_defconfig all -j8
~/src/gerrit-rockchip/u-boot/tools/loaderimage --pack --uboot ./u-boot.bin rk3399_uboot.img
echo "IMG ready!"
