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
 *
 * moved from board/rockchip/rk30sdk/nandmaptable.h
 */

#ifndef _MAPTABLE_H_
#define _MAPTABLE_H_

struct maptable{
    uint8_t  pattern[4];
    unsigned int  pba2lbamaptable[0x10000];	
};

int initmaptable(void);
uint32_t  getLBA(uint32_t pba);
uint32_t  getPBA(uint32_t lba);

#endif /*_MAPTABLE_H_*/
