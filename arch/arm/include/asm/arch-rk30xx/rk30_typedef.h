/*
 * (C) Copyright 2013
 * peter, Software Engineering, <superpeter.cai@gmail.com>.
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
#ifndef     _DRIVER_TYPEDEF_H
#define     _DRIVER_TYPEDEF_H

typedef     unsigned char           uint8;
typedef     signed char             int8;
typedef     unsigned short int      uint16;
typedef     signed short int        int16;
typedef     unsigned int            uint32;
typedef     signed int              int32;
typedef     unsigned long long      uint64;
typedef     signed long long        int64;

#ifndef		TRUE
#  define	TRUE    1
#endif

#ifndef		FALSE
#  define	FALSE   0
#endif

#ifndef		NULL
#  define	NULL	0
#endif

#ifndef		OK
#  define	OK	0
#endif

#ifndef		ERROR
#  define	ERROR	!0
#endif

#endif /* _DRIVER_TYPEDEF_H */

