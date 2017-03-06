#!/bin/sh

# You can use DB, WL to replace these command set
# Flash U-Boot to offset 0x4000, atf to offset 0x6000
~/src/upgrade_tool/upgrade_tool UL ~/src/upgrade_tool/rk3399miniloader_V040601_emmc100M_DDR200.bin

~/src/upgrade_tool/upgrade_tool DI uboot rk3399_uboot.img ~/src/upgrade_tool/parameter.txt
~/src/upgrade_tool/upgrade_tool RD
