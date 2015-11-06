#
# (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
# Peter, Software Engineering, <superpeter.cai@gmail.com>.
#
# SPDX-License-Identifier:	GPL-2.0+
#
#
#
# Innovator has 1 bank of 256 MB SDRAM
# Physical Address:
# 6000'0000 to 7000'0000
#
#
# Linux-Kernel is expected to be at 6000'8000, entry 6000'8000
# (mem base + reserved)
#
#
# For use with external or internal boots.

ALL-y += RKLoader_uboot.bin

