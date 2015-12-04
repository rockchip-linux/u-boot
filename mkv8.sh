#!/bin/sh
echo    "******************************"
echo    "*  Make armv8 aarch64 Uboot  *"
echo    "******************************"
make ARCHV=aarch64 --jobs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`

