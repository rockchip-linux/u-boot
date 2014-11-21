/*
 * SHA1 routine optimized to do word accesses rather than byte accesses,
 * and to avoid unnecessary copies into the context array.
 *
 * This was initially based on the Mozilla SHA1 implementation, although
 * none of the original Mozilla code remains.
 */

#ifndef _BLOCK_SHA_H_
#define _BLOCK_SHA_H_

//#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned long long size;
	unsigned int H[5];
	unsigned int W[16];
} SHA_CTX;

void SHA_init(SHA_CTX *ctx);
void SHA_update(SHA_CTX *ctx, const void* data, unsigned int len);
uint8_t* SHA_final(SHA_CTX *ctx);

/* Convenience method. Returns digest parameter value. */
const uint8_t* SHA(const void *data, int len, uint8_t *digest);

#define SHA_DIGEST_SIZE 20

#ifdef __cplusplus
}
#endif

#endif
