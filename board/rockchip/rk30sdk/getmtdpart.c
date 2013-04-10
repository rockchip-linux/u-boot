#include <common.h>
#include <asm/arch/rk30_drivers.h>
#include "getmtdpart.h"
#include "parameter.h"
#include "maptable.h"

#if 0
int isxdigit(int c)
{
    if( (c >= 'A' && c <= 'Z')
            || (c>='a' && c<='z')
            || (c>='0' && c<='9'))
        return 1;

    return 0;
}

int isdigit(int c)
{
    if(c>='0' && c<='9')
        return 1;

    return 0;
}

unsigned long simple_strtoull(const char *cp,char **endp,unsigned int base)
{
    unsigned long result = 0,value;

    if (!base) {
        base = 10;
        if (*cp == '0') {
            base = 8;
            cp++;
            if ((TOLOWER(*cp) == 'x') && isxdigit(cp[1])) {
                cp++;
                base = 16;
            }
        }
    } else if (base == 16) {
        if (cp[0] == '0' && TOLOWER(cp[1]) == 'x')
            cp += 2;
    }
    while (isxdigit(*cp)
            && (value = isdigit(*cp) ? *cp-'0' : TOLOWER(*cp)-'a'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}
#endif
unsigned long memparse (char *ptr, char **retptr)
{
    unsigned long ret = simple_strtoull (ptr, retptr, 0);

    switch (**retptr) {
        case 'G':
        case 'g':
            ret <<= 10;
        case 'M':
        case 'm':
            ret <<= 10;
        case 'K':
        case 'k':
            ret <<= 10;
            (*retptr)++;
        default:
            break;
    }
    return ret;
}

int get_part(char* parts, parameter_mtd_partition* this_part, int* part_index)
{
    char delim;
    unsigned int mask_flags;
    unsigned int size = 0;
    unsigned int offset = 0;
    unsigned int ipbaoffset,ipbasize,ipbasizeend;
    char name[PART_NAME]="\0";
    char spbaoffset[10]="\0";
    char spbasize[10]="\0";
    char *tmpoffsetparts=NULL, *tmpsizeparts=NULL;

    if (*parts == '-')
    {	/* assign all remaining space to this partition */
        size = SIZE_REMAINING;
        parts++;
    }
    else
    {
        tmpsizeparts = parts;
        size = memparse(parts, &parts);
    }

    /* check for offset */
    if (*parts == '@')
    {
        parts++;
        tmpoffsetparts = parts;
        offset = memparse(parts, &parts);
    }

    ipbaoffset =getPBA(offset);
    sprintf(spbaoffset, "0x%08x", ipbaoffset);
    if(tmpoffsetparts!=NULL)
        memcpy(tmpoffsetparts,spbaoffset,parts-tmpoffsetparts);

    if(size== SIZE_REMAINING)
    {
        ipbasize = size; 
    }
    else   // bad blocks more than range system mtdpart partition size, the size must content bad blocks,need to regulate 
    {
        ipbasizeend=getPBA(ipbaoffset+size);
        ipbasize = ipbasizeend - ipbaoffset;
        sprintf(spbasize, "0x%08x", ipbasize);
        if(tmpsizeparts!=NULL)
            memcpy(tmpsizeparts,spbasize,(tmpoffsetparts-1)-tmpsizeparts);
    }

    mask_flags = 0; /* this is going to be a regular partition */
    delim = 0;

    /* now look for name */
    if (*parts == '(')
    {
        delim = ')';
    }

    if (delim)
    {
        char *p;

        if ((p = strchr(parts+1, delim)) == 0)
        {
            //			printk(KERN_ERR ERRP "no closing %c found in partition name\n", delim);
            return 0;
        }
        strncpy(name, parts+1, p-(parts+1));
        parts = p + 1;
    }
    else
    { /* Partition_000 */
        sprintf(name, "Partition_%03d", *part_index);
    }

    /* test for options */
    if (strncmp(parts, "ro", 2) == 0)
    {
        mask_flags |= MTD_CMD_WRITEABLE;
        parts += 2;
    }

    /* if lk is found do NOT unlock the MTD partition*/
    if (strncmp(parts, "lk", 2) == 0)
    {
        mask_flags |= MTD_POWERUP_LOCK;
        parts += 2;
    }

    this_part->size = ipbasize;
    this_part->offset = ipbaoffset;
    this_part->mask_flags = mask_flags;
    sprintf( this_part->name, "%s", name);

    if( (++(*part_index) < MAX_PARTS) && (*parts == ',') )
    {
        get_part(++parts, this_part+1, part_index);
    }


    return 1;
}

int mtdpart_parse(const char* cmdline, cmdline_mtd_partition* this_mtd)
{
    char *s=NULL;
    char *p = NULL;

    if(!(s = strstr(cmdline, "mtdparts=")))
    {
        printf("no mtdparts= found at cmdline:%s\n" , cmdline );
        return -1;
    };
    if(!(p = strchr(s, ':')))
    {
        printf("no mtd id name found at cmdline:%s\n" , cmdline );
        return -1;
    };


    strncpy(this_mtd->mtd_id, s, p-s);
    s = p+1;
    get_part(s, this_mtd->parts+this_mtd->num_parts, &(this_mtd->num_parts));


    return 1;
}

