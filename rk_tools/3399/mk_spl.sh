#!/bin/sh
CROSS_COMPILE=aarch64-linux-gnu- CFLAGS='-gdwarf-3' make evb-rk3399_defconfig all -j8
tools/mkimage -n rk3399 -T rksd -d spl/u-boot-spl.bin idbspl.img
#truncate --size %512 u-boot.dtb
#truncate --size %512 u-boot-nodtb.bin
cp rk_tools/3399/bl31* .
cp board/rockchip/evb_rk3399/fit_spl_atf.its fit_spl_atf.its
tools/mkimage -f fit_spl_atf.its -E bl3.itb
rm bl31*
echo "IMG ready!"
echo "Write idbspl.img to 0x40"
echo "Write bl3.itb to 0x200"
