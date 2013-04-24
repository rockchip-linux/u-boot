/*
 * (C) Copyright 2013
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <linux/linkage.h>

#include <linux/linkage.h>

	.align 5
ENTRY(lowlevel_init)

ENDPROC(lowlevel_init)


ENTRY(ResetCpu)
	LDR	R2, =0x20020004
	LDR	R1, =0x00000003
 	STR	R1, [R2,#0]
 	LDR     R1, =0xeca8
	LDR     R2, =0x20000104
	LDR     R3, =0x10000000
 	STR	R3, [R0,#0]  ;// remap
 	STR	R1, [R2,#0]
ResetCpu_loop:
     	B  	ResetCpu_loop
ENDPROC(ResetCpu)
