/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Heungjun Kim <riverful.kim@samsung.com>
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
 *
 */

#ifndef _RK29XX_CPU_H
#define _RK29XX_CPU_H

#include "rk29_memmap.h"
#ifndef __ASSEMBLY__
#include <asm/io.h>
/* CPU detection macros */

static inline void rk29_set_cpu_id(void)
{
	/*the rk29 cpu id in rk29_cpu_id*/
	return ;
}
#endif

#endif	/* _S5PC1XX_CPU_H */
