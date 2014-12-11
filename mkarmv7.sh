#!/bin/sh
echo    "******************************"
echo    "*      Make armv7 Uboot      *"
echo    "******************************"
make --jobs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`

