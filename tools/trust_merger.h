/*
 * Rockchip trust image generator
 *
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef TRUST_MERGER_H
#define TRUST_MERGER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>


#define VERSION             "v1.0 (2015-06-15)"


/* config file */
#define SEC_VERSION         "[VERSION]"
#define SEC_BL30            "[BL30_OPTION]"
#define SEC_BL31            "[BL31_OPTION]"
#define SEC_BL32            "[BL32_OPTION]"
#define SEC_BL33            "[BL33_OPTION]"
#define SEC_OUT             "[OUTPUT]"

#define OPT_MAJOR           "MAJOR"
#define OPT_MINOR           "MINOR"
#define OPT_SEC             "SEC"
#define OPT_PATH            "PATH"
#define OPT_ADDR            "ADDR"
#define OPT_OUT_PATH        "PATH"

/* options */
#define OPT_VERBOSE         "--verbose"
#define OPT_HELP            "--help"
#define OPT_VERSION         "--version"
#define OPT_MERGE           "--pack"
#define OPT_UNPACK          "--unpack"
#define OPT_SUBFIX          "--subfix"


#define DEF_MAJOR           0
#define DEF_MINOR           0
#define DEF_BL30_PATH       "bl30.bin"
#define DEF_BL31_PATH       "bl31.bin"
#define DEF_BL32_PATH       "bl32.bin"
#define DEF_BL33_PATH       "bl33.bin"

#define DEF_OUT_PATH        "trust.img"

#define DEF_CONFIG_FILE     "RKTRUST.ini"


#define MAX_LINE_LEN        256
#define SCANF_EAT(in)       fscanf(in, "%*[ \r\n\t/]")

#define ENTRY_ALIGN         (2048)

enum {
	BL30_SEC = 0,
	BL31_SEC,
	BL32_SEC,
	BL33_SEC,
	BL_MAX_SEC
};

typedef char line_t[MAX_LINE_LEN];

typedef struct {
	bool		sec;
	uint32_t	id;
	char		path[MAX_LINE_LEN];
	uint32_t	addr;
} bl_entry_t;

typedef struct {
	uint16_t	major;
	uint16_t	minor;
	bl_entry_t	bl3x[BL_MAX_SEC];
	line_t        	outPath;
} OPT_T;


#define TRUST_HEAD_TAG			"BL3X"
#define SIGNATURE_SIZE			256	//signature size, unit is byte
#define TRUST_HEADER_SIZE		2048

typedef struct tagTRUST_HEADER {
	uint32_t tag;
	uint32_t version;
	uint32_t flags;
	uint32_t size;
	uint32_t reserved[4];
	uint32_t RSA_N[64];
	uint32_t RSA_E[64];
	uint32_t RSA_C[64];
} TRUST_HEADER, *PTRUST_HEADER;


typedef struct tagCOMPONENT_DATA {
	uint32_t HashData[8];
	uint32_t LoadAddr;
	uint32_t reserved[3];
} COMPONENT_DATA, *PCOMPONENT_DATA;


typedef struct tagTRUST_COMPONENT {
	uint32_t ComponentID;
	uint32_t StorageAddr;
	uint32_t ImageSize;
	uint32_t reserved;
} TRUST_COMPONENT, *PTRUST_COMPONENT;


#endif /* TRUST_MERGER_H */
