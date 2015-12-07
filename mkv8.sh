#!/bin/sh
echo    "******************************"
echo    "*     Make AArch64 Uboot     *"
echo    "******************************"
make ARCHV=aarch64 --jobs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`

