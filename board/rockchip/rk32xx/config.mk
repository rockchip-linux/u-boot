#********************************************************************************
#		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
#			--  ALL RIGHTS RESERVED  --
#File Name:	
#Author:         
#Created:        
#Modified:
#Revision:       1.00
#********************************************************************************
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

ALL-y += $(obj)RKLoader_uboot.bin

