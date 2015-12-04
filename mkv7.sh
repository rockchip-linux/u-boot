#!/bin/sh
echo    "******************************"
echo    "*  Make armv8 aarch32 Uboot  *"
echo    "******************************"
make --jobs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`

