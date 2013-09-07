#ifndef BMP_IMAGE_H
#define BMP_IMAGE_H

#ifndef _LINUX_TYPES_H
#include <stdint.h>
#endif

#ifndef RK_BLK_SIZE
#define RK_BLK_SIZE     512
#endif

#define BMP_IMAGE_TAG   0x12345678

typedef struct {
    uint32_t tag;
} bmp_image_header_t;                                                                                                                  

typedef struct {
    uint32_t offset;
    uint8_t  size;
    uint8_t  level;
    uint8_t  loaded;
} bmp_image_t;

#endif //BMP_IMAGE_H
