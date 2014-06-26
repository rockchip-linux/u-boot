#
# (C) Copyright 2008 - 2014 Rockchip Electronics
# Peter, Software Engineering, <superpeter.cai@gmail.com>.
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

obj-y	+= cpu.o
obj-y	+= timer.o
obj-y	+= clock.o
obj-y	+= lowlevel_init.o
obj-y	+= irqs.o
obj-y	+= ddr.o
obj-y   += iomux.o
obj-y   += reset.o
obj-y	+= pm.o

obj-$(CONFIG_RK_PL330)	+= pl330.o
obj-$(CONFIG_RK_DMAC)	+= dma.o

obj-$(CONFIG_RK_PWM)   += pwm.o
