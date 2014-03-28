/***
 * from external/genext2fs/genext2fs.c
 */

// block size

#define BLOCKSIZE         1024

// magic number for ext

#define EXT2_MAGIC_NUMBER  0xEF53
#define EXT3_MAGIC_NUMBER  0xF30A


// on-disk structures
// this trick makes me declare things only once
// (once for the structures, once for the endianness swap)

#define superblock_decl \
    udecl32(s_inodes_count)        /* Count of inodes in the filesystem */ \
    udecl32(s_blocks_count)        /* Count of blocks in the filesystem */ \
    udecl32(s_r_blocks_count)      /* Count of the number of reserved blocks */ \
    udecl32(s_free_blocks_count)   /* Count of the number of free blocks */ \
    udecl32(s_free_inodes_count)   /* Count of the number of free inodes */ \
    udecl32(s_first_data_block)    /* The first block which contains data */ \
    udecl32(s_log_block_size)      /* Indicator of the block size */ \
    decl32(s_log_frag_size)        /* Indicator of the size of the fragments */ \
    udecl32(s_blocks_per_group)    /* Count of the number of blocks in each block group */ \
    udecl32(s_frags_per_group)     /* Count of the number of fragments in each block group */ \
    udecl32(s_inodes_per_group)    /* Count of the number of inodes in each block group */ \
    udecl32(s_mtime)               /* The time that the filesystem was last mounted */ \
    udecl32(s_wtime)               /* The time that the filesystem was last written to */ \
    udecl16(s_mnt_count)           /* The number of times the file system has been mounted */ \
    decl16(s_max_mnt_count)        /* The number of times the file system can be mounted */ \
    udecl16(s_magic)               /* Magic number indicating ex2fs */ \
    udecl16(s_state)               /* Flags indicating the current state of the filesystem */ \
    udecl16(s_errors)              /* Flags indicating the procedures for error reporting */ \
    udecl16(s_minor_rev_level)     /* The minor revision level of the filesystem */ \
    udecl32(s_lastcheck)           /* The time that the filesystem was last checked */ \
    udecl32(s_checkinterval)       /* The maximum time permissable between checks */ \
    udecl32(s_creator_os)          /* Indicator of which OS created the filesystem */ \
    udecl32(s_rev_level)           /* The revision level of the filesystem */ \
    udecl16(s_def_resuid)          /* The default uid for reserved blocks */ \
    udecl16(s_def_resgid)          /* The default gid for reserved blocks */

#define groupdescriptor_decl \
    udecl32(bg_block_bitmap)       /* Block number of the block bitmap */ \
    udecl32(bg_inode_bitmap)       /* Block number of the inode bitmap */ \
    udecl32(bg_inode_table)        /* Block number of the inode table */ \
    udecl16(bg_free_blocks_count)  /* Free blocks in the group */ \
    udecl16(bg_free_inodes_count)  /* Free inodes in the group */ \
    decl16(bg_used_dirs_count)    /* Number of directories in the group */ \
    udecl16(bg_pad)

#define inode_decl \
    udecl16(i_mode)                /* Entry type and file mode */ \
    udecl16(i_uid)                 /* User id */ \
    udecl32(i_size)                /* File/dir size in bytes */ \
    udecl32(i_atime)               /* Last access time */ \
    udecl32(i_ctime)               /* Creation time */ \
    udecl32(i_mtime)               /* Last modification time */ \
    udecl32(i_dtime)               /* Deletion time */ \
    udecl16(i_gid)                 /* Group id */ \
    udecl16(i_links_count)         /* Number of (hard) links to this inode */ \
    udecl32(i_blocks)              /* Number of blocks used (1 block = 512 bytes) */ \
    udecl32(i_flags)               /* ??? */ \
    udecl32(i_reserved1) \
    utdecl32(i_block,15)           /* Blocks table */ \
    udecl32(i_version)             /* ??? */ \
    udecl32(i_file_acl)            /* File access control list */ \
    udecl32(i_dir_acl)             /* Directory access control list */ \
    udecl32(i_faddr)               /* Fragment address */ \
    udecl8(i_frag)                 /* Fragments count*/ \
    udecl8(i_fsize)                /* Fragment size */ \
    udecl16(i_pad1)

#define directory_decl \
    udecl32(d_inode)               /* Inode entry */ \
    udecl16(d_rec_len)             /* Total size on record */ \
    udecl16(d_name_len)            /* Size of entry name */

#define decl8(x) int8 x;
#define udecl8(x) uint8 x;
#define decl16(x) int16 x;
#define udecl16(x) uint16 x;
#define decl32(x) int32 x;
#define udecl32(x) uint32 x;
#define utdecl32(x,n) uint32 x[n];

typedef struct
{
    superblock_decl
    uint32 s_reserved[235];       // Reserved
} superblock;

typedef uint8 block[BLOCKSIZE];

/* Filesystem structure that support groups */
#if BLOCKSIZE == 1024
typedef struct
{
    block zero;            // The famous block 0
    superblock sb;         // The superblock
//    groupdescriptor gd[0]; // The group descriptors
} filesystem;
#else
#error UNHANDLED BLOCKSIZE
#endif

// now the endianness swap

#undef decl8
#undef udecl8
#undef decl16
#undef udecl16
#undef decl32
#undef udecl32
#undef utdecl32

#define decl8(x)
#define udecl8(x)
#define decl16(x) this->x = swab16(this->x);
#define udecl16(x) this->x = swab16(this->x);
#define decl32(x) this->x = swab32(this->x);
#define udecl32(x) this->x = swab32(this->x);
#define utdecl32(x,n) { int i; for(i=0; i<n; i++) this->x[i] = swab32(this->x[i]); }




