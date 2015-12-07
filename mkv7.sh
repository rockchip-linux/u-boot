#!/bin/sh
echo    "******************************"
echo    "*     Make AArch32 Uboot     *"
echo    "******************************"
make --jobs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`

