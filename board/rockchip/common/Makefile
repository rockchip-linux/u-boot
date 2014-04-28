#********************************************************************************
#		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
#			--  ALL RIGHTS RESERVED  --
#File Name:	
#Author:         
#Created:        
#Modified:
#Revision:       1.00
#********************************************************************************

obj-y += crc.o
obj-y += rkimage.o
obj-y += idblock.o
obj-y += fastboot.o
obj-y	+= key.o
obj-$(CONFIG_ZYF) += enter_recovery.o
obj-$(CONFIG_ZYF) += parameter.o
obj-$(CONFIG_ZYF) += rsa.o
obj-$(CONFIG_ZYF) += sha.o
obj-$(CONFIG_ZYF) += rc4_enc.o
obj-$(CONFIG_ZYF) += boot.o
obj-$(CONFIG_ZYF) += std_commod.o
ifdef CONFIG_NAND
obj-y += storage_nand.o
else
obj-y += storage.o
endif

obj-$(CONFIG_ZYF) += chipDepend.o


