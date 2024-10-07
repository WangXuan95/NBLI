#include <cstddef>
#include "rANS.h"
#include "GolombCodeTree.h"
#include "NBLIcodec.h"
#include "Header.h"
#include "CRC32.h"


inline static bool NBLIsizeInvalid (uint32_t height, uint32_t width) {
    return !((0<height) && (height<=32000) && (0<width) && (width<=32000));
}


// return : non-NULL : pointer to compressed data
//              NULL : failed
uint8_t *NBLIcompress (size_t &comp_size, uint8_t *p_img, bool is_rgb, uint32_t height, uint32_t width, bool use_golomb, bool use_avp, int16_t &near, uint32_t &crc32) {
    if (NBLIsizeInvalid(height, width))
        return NULL;
    
    size_t img_size = (size_t)(is_rgb?3:1) * height * width;
    size_t buf_size = img_size + 1048576;
    
    uint16_t *p_buf_base = new uint16_t [buf_size];
    uint16_t *p_buf      = p_buf_base;
    uint16_t *p_buf_end  = p_buf_base + buf_size;
    
    p_buf = writeHeader(p_buf, height, width, is_rgb, use_golomb, use_avp, near, 0);
    
    if (is_rgb) {
        if (use_golomb) {
            if (use_avp) {
                p_buf = NBLIcodec<true, true, true,            GolombCodeTree<true, 3, N_QD>> (p_buf, p_buf_end, p_img, height, width, near);
            } else {
                p_buf = NBLIcodec<true, true,false,            GolombCodeTree<true, 3, N_QD>> (p_buf, p_buf_end, p_img, height, width, near);
            }
        } else {
            if (use_avp) {
                p_buf = NBLIcodec<true, true, true, rANSwithGlobalHistogram<true, 3*N_QD, 9>> (p_buf, p_buf_end, p_img, height, width, near);
            } else {
                p_buf = NBLIcodec<true, true,false, rANSwithGlobalHistogram<true, 3*N_QD, 9>> (p_buf, p_buf_end, p_img, height, width, near);
            }
        }
    } else {
        if (use_golomb) {
            if (use_avp) {
                p_buf = NBLIcodec<false,true, true,            GolombCodeTree<true, 1, N_QD>> (p_buf, p_buf_end, p_img, height, width, near);
            } else {
                p_buf = NBLIcodec<false,true,false,            GolombCodeTree<true, 1, N_QD>> (p_buf, p_buf_end, p_img, height, width, near);
            }
        } else {
            if (use_avp) {
                p_buf = NBLIcodec<false,true, true,   rANSwithGlobalHistogram<true, N_QD, 8>> (p_buf, p_buf_end, p_img, height, width, near);
            } else {
                p_buf = NBLIcodec<false,true,false,   rANSwithGlobalHistogram<true, N_QD, 8>> (p_buf, p_buf_end, p_img, height, width, near);
            }
        }
    }
    
    if (crc32) {
        crc32 = calculateCRC32(p_img, img_size);
        writeHeader(p_buf_base, height, width, is_rgb, use_golomb, use_avp, near, crc32);
    }
    
    comp_size = 2 * (p_buf - p_buf_base);
    
    return (uint8_t*)p_buf_base;
}



// return : non-NULL : pointer to pixel data
//              NULL : failed
uint8_t *NBLIdecompress (uint8_t *p_buf, bool &is_rgb, uint32_t &height, uint32_t &width, bool &use_golomb, bool &use_avp, int16_t &near, uint32_t &crc32) {
    if (1 & (size_t)p_buf)    // buffer address must align to 2
        return NULL;
    
    uint16_t *p_u16 = (uint16_t*)p_buf;
    
    p_u16 = loadHeader(p_u16, height, width, is_rgb, use_golomb, use_avp, near, crc32);
    
    if (p_u16==NULL || NBLIsizeInvalid(height, width))
        return NULL;
    
    size_t img_size = (size_t)(is_rgb?3:1) * height * width;
    uint8_t  *p_img = new uint8_t [img_size];
    
    if (is_rgb) {
        if (use_golomb) {
            if (use_avp) {
                NBLIcodec<true, false, true,            GolombCodeTree<false, 3, N_QD>> (p_u16, NULL, p_img, height, width, near);
            } else {
                NBLIcodec<true, false,false,            GolombCodeTree<false, 3, N_QD>> (p_u16, NULL, p_img, height, width, near);
            }
        } else {
            if (use_avp) {
                NBLIcodec<true, false, true, rANSwithGlobalHistogram<false, 3*N_QD, 9>> (p_u16, NULL, p_img, height, width, near);
            } else {
                NBLIcodec<true, false,false, rANSwithGlobalHistogram<false, 3*N_QD, 9>> (p_u16, NULL, p_img, height, width, near);
            }
        }
    } else {
        if (use_golomb) {
            if (use_avp) {
                NBLIcodec<false,false, true,            GolombCodeTree<false, 1, N_QD>> (p_u16, NULL, p_img, height, width, near);
            } else {
                NBLIcodec<false,false,false,            GolombCodeTree<false, 1, N_QD>> (p_u16, NULL, p_img, height, width, near);
            }
        } else {
            if (use_avp) {
                NBLIcodec<false,false, true,   rANSwithGlobalHistogram<false, N_QD, 8>> (p_u16, NULL, p_img, height, width, near);
            } else {
                NBLIcodec<false,false,false,   rANSwithGlobalHistogram<false, N_QD, 8>> (p_u16, NULL, p_img, height, width, near);
            }
        }
    }
    
    if (crc32) {
        if (crc32 != calculateCRC32(p_img, img_size)){
            delete[] p_img;
            return NULL;
        }
    }
    
    return p_img;
}
