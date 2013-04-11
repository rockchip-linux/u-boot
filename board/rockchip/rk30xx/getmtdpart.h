#ifndef _GETMTDPART_H_
#define _GETMTDPART_H_

#define PART_NAME	32
#define MAX_PARTS	20
#define MAX_MTDID	64

#define MTD_CMD_WRITEABLE				0x400	/* Device is writeable */
#define MTD_POWERUP_LOCK			0x2000	/* Always locked after reset */

#define TOLOWER(x) ((x) | 0x20)

#define SIZE_REMAINING		0xFFFFFFFF

#define PARTNAME_MISC			"misc"
#define PARTNAME_KERNEL			"kernel"
#define PARTNAME_BOOT			"boot"
#define PARTNAME_RECOVERY		"recovery"
#define PARTNAME_CACHE			"cache"
#define PARTNAME_SYSTEM			"system"
#define PARTNAME_KPANIC         "kpanic"
#define PARTNAME_USERDATA       "userdata"

typedef struct tag_parameter_mtd_partition {
    char name[PART_NAME];			/* identifier string */
    unsigned int size;			/* partition size */
    unsigned int offset;		/* offset within the master MTD space */
    unsigned int mask_flags;		/* master MTD flags to mask out for this partition */
}parameter_mtd_partition;

typedef struct tag_cmdline_mtd_partition {
    char mtd_id[MAX_MTDID];
    int num_parts;
    parameter_mtd_partition parts[MAX_PARTS];
}cmdline_mtd_partition;

int mtdpart_parse(const char* string, cmdline_mtd_partition* this_mtd);

#endif
