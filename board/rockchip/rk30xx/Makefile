#********************************************************************************
#		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
#			--  ALL RIGHTS RESERVED  --
#File Name:	
#Author:         
#Created:        
#Modified:
#Revision:       1.00
#********************************************************************************

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).o

COBJS-y := $(BOARD).o
COBJS-y += crc.o
COBJS-y += rkimage.o
COBJS-y += rkloader.o
COBJS-y += fastboot.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS-y))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
