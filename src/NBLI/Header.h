#ifndef __HEADER_H__
#define __HEADER_H__

#include <cstdint>

#define  HEADER1       ('n' | ((int)'b'<<8))    // "nb"
#define  HEADER2_gray  ('i' | ((int)'G'<<8))    // "iG"
#define  HEADER2_RGB   ('i' | ((int)'C'<<8))    // "iC"

inline static uint16_t *writeHeader (uint16_t *p_buf, uint32_t height, uint32_t width, bool is_rgb, bool use_golomb, bool use_avp, int16_t &near, uint32_t crc32) {
    *(p_buf++) = HEADER1;
    *(p_buf++) = is_rgb ? HEADER2_RGB : HEADER2_gray;
    *(p_buf++) = (uint16_t) height;
    *(p_buf++) = (uint16_t) width;
    *(p_buf++) = (uint16_t)(crc32>>16);
    *(p_buf++) = (uint16_t) crc32;
    near &= 7;
    *(p_buf++) = (uint16_t)( near | (1&0)<<3 | (1&0)<<4 | (1&0)<<5 | (1&use_avp)<<6 | (1&use_golomb)<<7 );
    return p_buf;
}

inline static uint16_t *loadHeader (uint16_t *p_buf, uint32_t &height, uint32_t &width, bool &is_rgb, bool &use_golomb, bool &use_avp, int16_t &near, uint32_t &crc32) {
    height = width = 0;
    if (*(p_buf++) != HEADER1)
        return NULL;                     // failed
    uint16_t header2 = *(p_buf++);
    if        (header2 == HEADER2_gray) {
        is_rgb = false;
    } else if (header2 == HEADER2_RGB) {
        is_rgb = true;
    } else {
        return NULL;                     // failed
    }
    height = *(p_buf++);
    width  = *(p_buf++);
    crc32  = *(p_buf++);
    crc32<<= 16;
    crc32 |= *(p_buf++);
    uint16_t flags = *(p_buf++);
    near = flags & 7;
    //bit2 = 1 & (flags>>2);
    //bit3 = 1 & (flags>>3);
    //bit4 = 1 & (flags>>4);
    //bit5 = 1 & (flags>>5);
    use_avp = 1 & (flags>>6);
    use_golomb = 1 & (flags>>7);
    return p_buf;
}

#endif // __HEADER_H__
