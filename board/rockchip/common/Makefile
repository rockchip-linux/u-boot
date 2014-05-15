#
# (C) Copyright 2008 - 2014 Rockchip Electronics
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

obj-y += crc.o
obj-y += rsa.o
obj-y += sha.o
obj-y += rc4_enc.o
obj-y += rkimage.o
obj-y += idblock.o
obj-y += fastboot.o
obj-y += recovery.o
obj-y += parameter.o
obj-y += boot.o
obj-y += ftl_std.o
obj-y += key.o
obj-y += chipDepend.o

ifdef CONFIG_NAND
obj-y += storage_nand.o
else
obj-y += storage.o
endif


