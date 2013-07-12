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

LIB	= $(obj)lib$(SOC).o

#COBJS	+= cpu_info.o
COBJS	+= timer.o
COBJS	+= clock.o
#COBJS	+= reset.o
#COBJS	+= interrupts.o
#COBJS	+= intc.o

SOBJS	+= cache.o
SOBJS	+= lowlevel_init.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS) $(SOBJS))

all:	 $(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
