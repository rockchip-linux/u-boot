/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * Configuation settings for the rk3xxx chip platform.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;


static void iomem_read(void __iomem *addr, int len, int iosize)
{
	int i;

	while (len) {
		printf("0x%08x: ", addr);
		i = 0;
		while(i < 16 && len) {
			switch (iosize) {
				case 1:
					printf(" %02x", *(unsigned char *)addr);
					break;
				case 2:
					printf(" %04x", *(unsigned short *)addr);
					break;
				case 4:
					printf(" %08x", *(unsigned int *)addr);
					break;
			}
			i += iosize;
			addr += iosize;
			len -= iosize;
		}
		printf("\n");
	}
}


static void iomem_write(void __iomem *addr, int len, int iosize, unsigned int value)
{
	switch (iosize) {
		case 1:
			while (len) {
				*(unsigned char *)addr = value;
				len -= iosize;
				addr += iosize;
			}
			break;
		case 2:
			while (len) {
				*(unsigned short *)addr = value;
				len -= iosize;
				addr += iosize;
			}
			break;
		case 4:
			while (len) {
				*(unsigned int *)addr = value;
				len -= iosize;
				addr += iosize;
			}
			break;
	}
}


static void iomem_show_help(void)
{
	printf("Raw memory i/o utility - $Revision: 1.0$\n\n");
	printf("io -1|2|4 -r|w <addr> [-l <len>] [<value>]\n\n");
	printf("    -1|2|4     Sets memory access size in bytes (default byte)\n");
	printf("    -r|w       Read from or Write to memory (default read)\n");
	printf("    -l <len>   Length in bytes of area to access (defaults to\n");
	printf("               one access, or whole file length)\n");
	printf("    <addr>     The memory address to access\n");
	printf("    <val>      The value to write (implies -w)\n\n");
	printf("Examples:\n");
	printf("    io -2 -r 200 -l 100 	Reads 100 bytes from addr 200\n");
	printf("    io -1 -w 0x1000 0x12	Writes 0x12 to location 0x1000\n");
	printf("\n");
}



static int do_io_tool(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int addr = 0;
	int iosize = 0;
	int len = 0;
	unsigned int value = 0;
	int rw;
	int index = 0;

	if (argc <= 3) {
		iomem_show_help();
		return -1;
	}

	index = 1;
	if (!strcmp(argv[index], "-1")) {
		iosize = 1;
	} else if (!strcmp(argv[index], "-2")) {
		iosize = 2;
	} else if (!strcmp(argv[index], "-4")) {
		iosize = 4;
	} else {
		iomem_show_help();
		return -1;
	}
	index++;

	if (!strcmp(argv[index], "-r")) {
		rw = 0;
	} else if (!strcmp(argv[index], "-w")) {
		rw = 1;
	} else {
		iomem_show_help();
		return -1;
	}
	index++;

	addr = simple_strtoul(argv[index], NULL, 0);
	if (((iosize == 2) && (addr & 1)) || ((iosize == 4) && (addr & 3))) {
		printf("Badly aligned <addr> for access size\n");
		return -1;
	}
	index++;

	len = 0;
	if (!strcmp(argv[index], "-l")) {
		index++;
		len = simple_strtoul(argv[index], NULL, 0);
		index++;
		if ((iosize == 2 && (len & 1)) || (iosize == 4 && (len & 3))) {
			printf("Badly aligned <size> for access size\n");
			return -1;
		}
	} 
	if (!len) {
		len = iosize;
	}

	if (rw == 0) {
		iomem_read(addr, len, iosize);
	} else {
		value = simple_strtoul(argv[index], NULL, 0);
		iomem_write(addr, len, iosize, value);
	}

	return 0;
}

U_BOOT_CMD(io, CONFIG_SYS_MAXARGS, 1,	do_io_tool,
	"IO memory Read/Write Tool",
	""
);

