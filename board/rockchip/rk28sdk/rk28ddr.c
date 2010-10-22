/*
 * (C) Copyright 2009 rockchip
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/rk28_drivers.h>
#include "rk28ddr.h"


int getddrzise(void)
{
       unsigned int capability = 0; 
	capability = (0x1 << ((0x2 >> ((pDDR_Reg->CTRL_REG_08 >> 16) & 0x1))  //bus width
                                + (13 - (pDDR_Reg->CTRL_REG_21 & 0x7))  //col
                                + (2 + ((pDDR_Reg->CTRL_REG_05 >> 8) & 0x1))  //bank
                                + (15 - ((pDDR_Reg->CTRL_REG_19 >> 24) & 0x7))));  //row
        if(((pDDR_Reg->CTRL_REG_17 >> 24) & 0x3) == 0x3)
        {

                capability += (0x1 << ((0x2 >> ((pDDR_Reg->CTRL_REG_08 >> 16) & 0x1))  //bus width
                                + (13 - ((pDDR_Reg->CTRL_REG_21 >> 8) & 0x7))  //col
                                + (2 + ((pDDR_Reg->CTRL_REG_05 >> 16) & 0x1))  //bank
                                + (15 - (pDDR_Reg->CTRL_REG_20 & 0x7))));  //row
        }
        return (capability);
}

