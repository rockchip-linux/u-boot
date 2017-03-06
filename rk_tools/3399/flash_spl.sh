#!/bin/sh

~/src/upgrade_tool/upgrade_tool DB /media/kever/data2/src/linux-sdk/rkbin/rk33/rk3399_loader_v1.08.106.bin
~/src/upgrade_tool/upgrade_tool WL 64 idbspl.img
~/src/upgrade_tool/upgrade_tool WL 512 bl3.itb
#~/src/upgrade_tool/upgrade_tool WL 512 u-boot.bin
~/src/upgrade_tool/upgrade_tool RD
