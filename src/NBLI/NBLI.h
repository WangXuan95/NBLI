#ifndef __NBLI_H__
#define __NBLI_H__

#include <cstdint>

// return : non-NULL : pointer to compressed data
//              NULL : failed
uint8_t *NBLIcompress (size_t &comp_size, uint8_t *p_img, bool is_rgb, uint32_t height, uint32_t width, bool use_golomb, bool use_avp, int16_t &near, uint32_t &crc32);

// return : non-NULL : pointer to pixel data
//              NULL : failed
uint8_t *NBLIdecompress (uint8_t *p_buf, bool &is_rgb, uint32_t &height, uint32_t &width, bool &use_golomb, bool &use_avp, int16_t &near, uint32_t &crc32);

#endif // __NBLI_H__

