/*
 * This file is derived from crc32.c from the zlib-1.1.3 distribution
 * by Jean-loup Gailly and Mark Adler.
 */

/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifdef USE_HOSTCC
#include <arpa/inet.h>
#else
#include <common.h>
#endif
#include <compiler.h>
#include <u-boot/crc.h>

#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
#include <watchdog.h>
#endif
#include "u-boot/zlib.h"

#define local static
#define ZEXPORT	/* empty */

#define tole(x) cpu_to_le32(x)

#ifdef DYNAMIC_CRC_TABLE

local int crc_table_empty = 1;
local uint32_t crc_table[256];
local void make_crc_table OF((void));

/*
  Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The table is simply the CRC of all possible eight bit values.  This is all
  the information needed to generate CRC's on data a byte at a time for all
  combinations of CRC register values and incoming bytes.
*/
local void make_crc_table()
{
  uint32_t c;
  int n, k;
  uLong poly;		/* polynomial exclusive-or pattern */
  /* terms of polynomial defining this crc (except x^32): */
  static const Byte p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  /* make exclusive-or pattern from polynomial (0xedb88320L) */
  poly = 0L;
  for (n = 0; n < sizeof(p)/sizeof(Byte); n++)
    poly |= 1L << (31 - p[n]);

  for (n = 0; n < 256; n++)
  {
    c = (uLong)n;
    for (k = 0; k < 8; k++)
      c = c & 1 ? poly ^ (c >> 1) : c >> 1;
    crc_table[n] = tole(c);
  }
  crc_table_empty = 0;
}
#else
/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by make_crc_table)
 */

#ifndef CONFIG_ROCKCHIP
local const uint32_t crc_table[256] = {
tole(0x00000000L), tole(0x77073096L), tole(0xee0e612cL), tole(0x990951baL),
tole(0x076dc419L), tole(0x706af48fL), tole(0xe963a535L), tole(0x9e6495a3L),
tole(0x0edb8832L), tole(0x79dcb8a4L), tole(0xe0d5e91eL), tole(0x97d2d988L),
tole(0x09b64c2bL), tole(0x7eb17cbdL), tole(0xe7b82d07L), tole(0x90bf1d91L),
tole(0x1db71064L), tole(0x6ab020f2L), tole(0xf3b97148L), tole(0x84be41deL),
tole(0x1adad47dL), tole(0x6ddde4ebL), tole(0xf4d4b551L), tole(0x83d385c7L),
tole(0x136c9856L), tole(0x646ba8c0L), tole(0xfd62f97aL), tole(0x8a65c9ecL),
tole(0x14015c4fL), tole(0x63066cd9L), tole(0xfa0f3d63L), tole(0x8d080df5L),
tole(0x3b6e20c8L), tole(0x4c69105eL), tole(0xd56041e4L), tole(0xa2677172L),
tole(0x3c03e4d1L), tole(0x4b04d447L), tole(0xd20d85fdL), tole(0xa50ab56bL),
tole(0x35b5a8faL), tole(0x42b2986cL), tole(0xdbbbc9d6L), tole(0xacbcf940L),
tole(0x32d86ce3L), tole(0x45df5c75L), tole(0xdcd60dcfL), tole(0xabd13d59L),
tole(0x26d930acL), tole(0x51de003aL), tole(0xc8d75180L), tole(0xbfd06116L),
tole(0x21b4f4b5L), tole(0x56b3c423L), tole(0xcfba9599L), tole(0xb8bda50fL),
tole(0x2802b89eL), tole(0x5f058808L), tole(0xc60cd9b2L), tole(0xb10be924L),
tole(0x2f6f7c87L), tole(0x58684c11L), tole(0xc1611dabL), tole(0xb6662d3dL),
tole(0x76dc4190L), tole(0x01db7106L), tole(0x98d220bcL), tole(0xefd5102aL),
tole(0x71b18589L), tole(0x06b6b51fL), tole(0x9fbfe4a5L), tole(0xe8b8d433L),
tole(0x7807c9a2L), tole(0x0f00f934L), tole(0x9609a88eL), tole(0xe10e9818L),
tole(0x7f6a0dbbL), tole(0x086d3d2dL), tole(0x91646c97L), tole(0xe6635c01L),
tole(0x6b6b51f4L), tole(0x1c6c6162L), tole(0x856530d8L), tole(0xf262004eL),
tole(0x6c0695edL), tole(0x1b01a57bL), tole(0x8208f4c1L), tole(0xf50fc457L),
tole(0x65b0d9c6L), tole(0x12b7e950L), tole(0x8bbeb8eaL), tole(0xfcb9887cL),
tole(0x62dd1ddfL), tole(0x15da2d49L), tole(0x8cd37cf3L), tole(0xfbd44c65L),
tole(0x4db26158L), tole(0x3ab551ceL), tole(0xa3bc0074L), tole(0xd4bb30e2L),
tole(0x4adfa541L), tole(0x3dd895d7L), tole(0xa4d1c46dL), tole(0xd3d6f4fbL),
tole(0x4369e96aL), tole(0x346ed9fcL), tole(0xad678846L), tole(0xda60b8d0L),
tole(0x44042d73L), tole(0x33031de5L), tole(0xaa0a4c5fL), tole(0xdd0d7cc9L),
tole(0x5005713cL), tole(0x270241aaL), tole(0xbe0b1010L), tole(0xc90c2086L),
tole(0x5768b525L), tole(0x206f85b3L), tole(0xb966d409L), tole(0xce61e49fL),
tole(0x5edef90eL), tole(0x29d9c998L), tole(0xb0d09822L), tole(0xc7d7a8b4L),
tole(0x59b33d17L), tole(0x2eb40d81L), tole(0xb7bd5c3bL), tole(0xc0ba6cadL),
tole(0xedb88320L), tole(0x9abfb3b6L), tole(0x03b6e20cL), tole(0x74b1d29aL),
tole(0xead54739L), tole(0x9dd277afL), tole(0x04db2615L), tole(0x73dc1683L),
tole(0xe3630b12L), tole(0x94643b84L), tole(0x0d6d6a3eL), tole(0x7a6a5aa8L),
tole(0xe40ecf0bL), tole(0x9309ff9dL), tole(0x0a00ae27L), tole(0x7d079eb1L),
tole(0xf00f9344L), tole(0x8708a3d2L), tole(0x1e01f268L), tole(0x6906c2feL),
tole(0xf762575dL), tole(0x806567cbL), tole(0x196c3671L), tole(0x6e6b06e7L),
tole(0xfed41b76L), tole(0x89d32be0L), tole(0x10da7a5aL), tole(0x67dd4accL),
tole(0xf9b9df6fL), tole(0x8ebeeff9L), tole(0x17b7be43L), tole(0x60b08ed5L),
tole(0xd6d6a3e8L), tole(0xa1d1937eL), tole(0x38d8c2c4L), tole(0x4fdff252L),
tole(0xd1bb67f1L), tole(0xa6bc5767L), tole(0x3fb506ddL), tole(0x48b2364bL),
tole(0xd80d2bdaL), tole(0xaf0a1b4cL), tole(0x36034af6L), tole(0x41047a60L),
tole(0xdf60efc3L), tole(0xa867df55L), tole(0x316e8eefL), tole(0x4669be79L),
tole(0xcb61b38cL), tole(0xbc66831aL), tole(0x256fd2a0L), tole(0x5268e236L),
tole(0xcc0c7795L), tole(0xbb0b4703L), tole(0x220216b9L), tole(0x5505262fL),
tole(0xc5ba3bbeL), tole(0xb2bd0b28L), tole(0x2bb45a92L), tole(0x5cb36a04L),
tole(0xc2d7ffa7L), tole(0xb5d0cf31L), tole(0x2cd99e8bL), tole(0x5bdeae1dL),
tole(0x9b64c2b0L), tole(0xec63f226L), tole(0x756aa39cL), tole(0x026d930aL),
tole(0x9c0906a9L), tole(0xeb0e363fL), tole(0x72076785L), tole(0x05005713L),
tole(0x95bf4a82L), tole(0xe2b87a14L), tole(0x7bb12baeL), tole(0x0cb61b38L),
tole(0x92d28e9bL), tole(0xe5d5be0dL), tole(0x7cdcefb7L), tole(0x0bdbdf21L),
tole(0x86d3d2d4L), tole(0xf1d4e242L), tole(0x68ddb3f8L), tole(0x1fda836eL),
tole(0x81be16cdL), tole(0xf6b9265bL), tole(0x6fb077e1L), tole(0x18b74777L),
tole(0x88085ae6L), tole(0xff0f6a70L), tole(0x66063bcaL), tole(0x11010b5cL),
tole(0x8f659effL), tole(0xf862ae69L), tole(0x616bffd3L), tole(0x166ccf45L),
tole(0xa00ae278L), tole(0xd70dd2eeL), tole(0x4e048354L), tole(0x3903b3c2L),
tole(0xa7672661L), tole(0xd06016f7L), tole(0x4969474dL), tole(0x3e6e77dbL),
tole(0xaed16a4aL), tole(0xd9d65adcL), tole(0x40df0b66L), tole(0x37d83bf0L),
tole(0xa9bcae53L), tole(0xdebb9ec5L), tole(0x47b2cf7fL), tole(0x30b5ffe9L),
tole(0xbdbdf21cL), tole(0xcabac28aL), tole(0x53b39330L), tole(0x24b4a3a6L),
tole(0xbad03605L), tole(0xcdd70693L), tole(0x54de5729L), tole(0x23d967bfL),
tole(0xb3667a2eL), tole(0xc4614ab8L), tole(0x5d681b02L), tole(0x2a6f2b94L),
tole(0xb40bbe37L), tole(0xc30c8ea1L), tole(0x5a05df1bL), tole(0x2d02ef8dL)
};
#else
local const uint32_t crc_table[256] = {
tole(0x00000000L), tole(0x04c10db7L), tole(0x09821b6eL), tole(0x0d4316d9L),
tole(0x130436dcL), tole(0x17c53b6bL), tole(0x1a862db2L), tole(0x1e472005L),
tole(0x26086db8L), tole(0x22c9600fL), tole(0x2f8a76d6L), tole(0x2b4b7b61L),
tole(0x350c5b64L), tole(0x31cd56d3L), tole(0x3c8e400aL), tole(0x384f4dbdL),
tole(0x4c10db70L), tole(0x48d1d6c7L), tole(0x4592c01eL), tole(0x4153cda9L),
tole(0x5f14edacL), tole(0x5bd5e01bL), tole(0x5696f6c2L), tole(0x5257fb75L),
tole(0x6a18b6c8L), tole(0x6ed9bb7fL), tole(0x639aada6L), tole(0x675ba011L),
tole(0x791c8014L), tole(0x7ddd8da3L), tole(0x709e9b7aL), tole(0x745f96cdL),
tole(0x9821b6e0L), tole(0x9ce0bb57L), tole(0x91a3ad8eL), tole(0x9562a039L),
tole(0x8b25803cL), tole(0x8fe48d8bL), tole(0x82a79b52L), tole(0x866696e5L),
tole(0xbe29db58L), tole(0xbae8d6efL), tole(0xb7abc036L), tole(0xb36acd81L),
tole(0xad2ded84L), tole(0xa9ece033L), tole(0xa4aff6eaL), tole(0xa06efb5dL),
tole(0xd4316d90L), tole(0xd0f06027L), tole(0xddb376feL), tole(0xd9727b49L),
tole(0xc7355b4cL), tole(0xc3f456fbL), tole(0xceb74022L), tole(0xca764d95L),
tole(0xf2390028L), tole(0xf6f80d9fL), tole(0xfbbb1b46L), tole(0xff7a16f1L),
tole(0xe13d36f4L), tole(0xe5fc3b43L), tole(0xe8bf2d9aL), tole(0xec7e202dL),
tole(0x34826077L), tole(0x30436dc0L), tole(0x3d007b19L), tole(0x39c176aeL),
tole(0x278656abL), tole(0x23475b1cL), tole(0x2e044dc5L), tole(0x2ac54072L),
tole(0x128a0dcfL), tole(0x164b0078L), tole(0x1b0816a1L), tole(0x1fc91b16L),
tole(0x018e3b13L), tole(0x054f36a4L), tole(0x080c207dL), tole(0x0ccd2dcaL),
tole(0x7892bb07L), tole(0x7c53b6b0L), tole(0x7110a069L), tole(0x75d1addeL),
tole(0x6b968ddbL), tole(0x6f57806cL), tole(0x621496b5L), tole(0x66d59b02L),
tole(0x5e9ad6bfL), tole(0x5a5bdb08L), tole(0x5718cdd1L), tole(0x53d9c066L),
tole(0x4d9ee063L), tole(0x495fedd4L), tole(0x441cfb0dL), tole(0x40ddf6baL),
tole(0xaca3d697L), tole(0xa862db20L), tole(0xa521cdf9L), tole(0xa1e0c04eL),
tole(0xbfa7e04bL), tole(0xbb66edfcL), tole(0xb625fb25L), tole(0xb2e4f692L),
tole(0x8aabbb2fL), tole(0x8e6ab698L), tole(0x8329a041L), tole(0x87e8adf6L),
tole(0x99af8df3L), tole(0x9d6e8044L), tole(0x902d969dL), tole(0x94ec9b2aL),
tole(0xe0b30de7L), tole(0xe4720050L), tole(0xe9311689L), tole(0xedf01b3eL),
tole(0xf3b73b3bL), tole(0xf776368cL), tole(0xfa352055L), tole(0xfef42de2L),
tole(0xc6bb605fL), tole(0xc27a6de8L), tole(0xcf397b31L), tole(0xcbf87686L),
tole(0xd5bf5683L), tole(0xd17e5b34L), tole(0xdc3d4dedL), tole(0xd8fc405aL),
tole(0x6904c0eeL), tole(0x6dc5cd59L), tole(0x6086db80L), tole(0x6447d637L),
tole(0x7a00f632L), tole(0x7ec1fb85L), tole(0x7382ed5cL), tole(0x7743e0ebL),
tole(0x4f0cad56L), tole(0x4bcda0e1L), tole(0x468eb638L), tole(0x424fbb8fL),
tole(0x5c089b8aL), tole(0x58c9963dL), tole(0x558a80e4L), tole(0x514b8d53L),
tole(0x25141b9eL), tole(0x21d51629L), tole(0x2c9600f0L), tole(0x28570d47L),
tole(0x36102d42L), tole(0x32d120f5L), tole(0x3f92362cL), tole(0x3b533b9bL),
tole(0x031c7626L), tole(0x07dd7b91L), tole(0x0a9e6d48L), tole(0x0e5f60ffL),
tole(0x101840faL), tole(0x14d94d4dL), tole(0x199a5b94L), tole(0x1d5b5623L),
tole(0xf125760eL), tole(0xf5e47bb9L), tole(0xf8a76d60L), tole(0xfc6660d7L),
tole(0xe22140d2L), tole(0xe6e04d65L), tole(0xeba35bbcL), tole(0xef62560bL),
tole(0xd72d1bb6L), tole(0xd3ec1601L), tole(0xdeaf00d8L), tole(0xda6e0d6fL),
tole(0xc4292d6aL), tole(0xc0e820ddL), tole(0xcdab3604L), tole(0xc96a3bb3L),
tole(0xbd35ad7eL), tole(0xb9f4a0c9L), tole(0xb4b7b610L), tole(0xb076bba7L),
tole(0xae319ba2L), tole(0xaaf09615L), tole(0xa7b380ccL), tole(0xa3728d7bL),
tole(0x9b3dc0c6L), tole(0x9ffccd71L), tole(0x92bfdba8L), tole(0x967ed61fL),
tole(0x8839f61aL), tole(0x8cf8fbadL), tole(0x81bbed74L), tole(0x857ae0c3L),
tole(0x5d86a099L), tole(0x5947ad2eL), tole(0x5404bbf7L), tole(0x50c5b640L),
tole(0x4e829645L), tole(0x4a439bf2L), tole(0x47008d2bL), tole(0x43c1809cL),
tole(0x7b8ecd21L), tole(0x7f4fc096L), tole(0x720cd64fL), tole(0x76cddbf8L),
tole(0x688afbfdL), tole(0x6c4bf64aL), tole(0x6108e093L), tole(0x65c9ed24L),
tole(0x11967be9L), tole(0x1557765eL), tole(0x18146087L), tole(0x1cd56d30L),
tole(0x02924d35L), tole(0x06534082L), tole(0x0b10565bL), tole(0x0fd15becL),
tole(0x379e1651L), tole(0x335f1be6L), tole(0x3e1c0d3fL), tole(0x3add0088L),
tole(0x249a208dL), tole(0x205b2d3aL), tole(0x2d183be3L), tole(0x29d93654L),
tole(0xc5a71679L), tole(0xc1661bceL), tole(0xcc250d17L), tole(0xc8e400a0L),
tole(0xd6a320a5L), tole(0xd2622d12L), tole(0xdf213bcbL), tole(0xdbe0367cL),
tole(0xe3af7bc1L), tole(0xe76e7676L), tole(0xea2d60afL), tole(0xeeec6d18L),
tole(0xf0ab4d1dL), tole(0xf46a40aaL), tole(0xf9295673L), tole(0xfde85bc4L),
tole(0x89b7cd09L), tole(0x8d76c0beL), tole(0x8035d667L), tole(0x84f4dbd0L),
tole(0x9ab3fbd5L), tole(0x9e72f662L), tole(0x9331e0bbL), tole(0x97f0ed0cL),
tole(0xafbfa0b1L), tole(0xab7ead06L), tole(0xa63dbbdfL), tole(0xa2fcb668L),
tole(0xbcbb966dL), tole(0xb87a9bdaL), tole(0xb5398d03L), tole(0xb1f880b4L)
};
#endif /* CONFIG_ROCKCHIP */
#endif

#if 0
/* =========================================================================
 * This function can be used by asm versions of crc32()
 */
const uint32_t * ZEXPORT get_crc_table()
{
#ifdef DYNAMIC_CRC_TABLE
  if (crc_table_empty) make_crc_table();
#endif
  return (const uint32_t *)crc_table;
}
#endif

/* ========================================================================= */
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define DO_CRC(x) crc = tab[(crc ^ (x)) & 255] ^ (crc >> 8)
# else
#  define DO_CRC(x) crc = tab[((crc >> 24) ^ (x)) & 255] ^ (crc << 8)
# endif

/* ========================================================================= */

/* No ones complement version. JFFS2 (and other things ?)
 * don't use ones compliment in their CRC calculations.
 */
uint32_t ZEXPORT crc32_no_comp(uint32_t crc, const Bytef *buf, uInt len)
{
    const uint32_t *tab = crc_table;
    const uint32_t *b =(const uint32_t *)buf;
    size_t rem_len;
#ifdef DYNAMIC_CRC_TABLE
    if (crc_table_empty)
      make_crc_table();
#endif
    crc = cpu_to_le32(crc);
    /* Align it */
    if (((long)b) & 3 && len) {
	 uint8_t *p = (uint8_t *)b;
	 do {
	      DO_CRC(*p++);
	 } while ((--len) && ((long)p)&3);
	 b = (uint32_t *)p;
    }

    rem_len = len & 3;
    len = len >> 2;
    for (--b; len; --len) {
	 /* load data 32 bits wide, xor data 32 bits wide. */
	 crc ^= *++b; /* use pre increment for speed */
	 DO_CRC(0);
	 DO_CRC(0);
	 DO_CRC(0);
	 DO_CRC(0);
    }
    len = rem_len;
    /* And the last few bytes */
    if (len) {
	 uint8_t *p = (uint8_t *)(b + 1) - 1;
	 do {
	      DO_CRC(*++p); /* use pre increment for speed */
	 } while (--len);
    }

    return le32_to_cpu(crc);
}
#undef DO_CRC

uint32_t ZEXPORT crc32 (uint32_t crc, const Bytef *p, uInt len)
{
#ifdef CONFIG_ROCKCHIP
#define DO_CRC(x) crc = tab[((crc >> 24) ^ (x)) & 255] ^ (crc << 8)
	const uint32_t *tab;
	tab	= crc_table;
	crc = cpu_to_le32(crc);
	do {
		DO_CRC(*p++);
	} while (--len);
	return le32_to_cpu(crc);
#undef DO_CRC
#else
     return crc32_no_comp(crc ^ 0xffffffffL, p, len) ^ 0xffffffffL;
#endif /* CONFIG_ROCKCHIP */
}

/*
 * Calculate the crc32 checksum triggering the watchdog every 'chunk_sz' bytes
 * of input.
 */
uint32_t ZEXPORT crc32_wd (uint32_t crc,
			   const unsigned char *buf,
			   uInt len, uInt chunk_sz)
{
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
	const unsigned char *end, *curr;
	int chunk;

	curr = buf;
	end = buf + len;
	while (curr < end) {
		chunk = end - curr;
		if (chunk > chunk_sz)
			chunk = chunk_sz;
		crc = crc32 (crc, curr, chunk);
		curr += chunk;
		WATCHDOG_RESET ();
	}
#else
	crc = crc32 (crc, buf, len);
#endif

	return crc;
}

void crc32_wd_buf(const unsigned char *input, unsigned int ilen,
		unsigned char *output, unsigned int chunk_sz)
{
	uint32_t crc;

	crc = crc32_wd(0, input, ilen, chunk_sz);
	crc = htonl(crc);
	memcpy(output, &crc, sizeof(crc));
}
