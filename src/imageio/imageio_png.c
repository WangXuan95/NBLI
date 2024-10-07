#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>



static void write_png_chunk (char *p_name, uint8_t *p_data, uint32_t len, FILE *fp) {
    const static uint32_t crc_table[] = {0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
    uint32_t i, crc=0xFFFFFFFF;
    fputc(((len>>24) & 0xFF), fp);
    fputc(((len>>16) & 0xFF), fp);
    fputc(((len>> 8) & 0xFF), fp);
    fputc(((len    ) & 0xFF), fp);
    fwrite(p_name, sizeof(uint8_t),   4, fp);
    if (len > 0)
        fwrite(p_data, sizeof(uint8_t), len, fp);
    for (i=0; i<4; i++) {
        crc ^= p_name[i];
        crc = (crc >> 4) ^ crc_table[crc & 15];
        crc = (crc >> 4) ^ crc_table[crc & 15];
    }
    for (i=0; i<len; i++) {
        crc ^= p_data[i];
        crc = (crc >> 4) ^ crc_table[crc & 15];
        crc = (crc >> 4) ^ crc_table[crc & 15];
    }
    crc = ~crc;
    fputc(((crc>>24) & 0xFF), fp);
    fputc(((crc>>16) & 0xFF), fp);
    fputc(((crc>> 8) & 0xFF), fp);
    fputc(((crc    ) & 0xFF), fp);
}



// return:   0 : success    1 : failed
int writePNGImageFile (const char *p_filename, const uint8_t *p_buf, int is_rgb, uint32_t height, uint32_t width) {
    size_t   w = (is_rgb?3:1)*width + 1;
    uint32_t adler_a=1, adler_b=0;
    size_t   i;
    uint8_t *p_dst, *p, *p_last_blk;
    FILE    *fp;
    
    if (width < 1 || height < 1)
        return 1;
    
    p_dst = p = p_last_blk = (uint8_t*)malloc((w+6)*height + 65536);
    if (p_dst == NULL)
        return 1;
    
    if ((fp = fopen(p_filename, "wb")) == NULL) {
        free(p_dst);
        return 1;
    }
    
    fwrite("\x89PNG\r\n\32\n", sizeof(char), 8, fp);    // 8-bit PNG magic
    
    sprintf((char*)p_dst, "%c%c%c%c%c%c%c%c\x08%c%c%c%c", 
        (uint8_t)( width>>24), (uint8_t)( width>>16), (uint8_t)( width>>8), (uint8_t)( width),
        (uint8_t)(height>>24), (uint8_t)(height>>16), (uint8_t)(height>>8), (uint8_t)(height),
        (is_rgb ? 2 : 0),
        0, 0, 0
    );
    write_png_chunk((char*)"IHDR", p_dst, 13, fp);
    
    *p++ = 0x78;
    *p++ = 0x01;
    for (i=0; i<(w*height); i++) {
        if (i%0xFFFF == 0) {
            *p++ = 0;         // deflate block start (5bytes)
            *p++ = 0xFF;
            *p++ = 0xFF;
            *p++ = 0x00;
            *p++ = 0x00;
            p_last_blk = p;
        }
        if (i%w == 0) {
            *p = 0;           // filter at each start of line
        } else {
            *p = *(p_buf++);  // pixel byte
        }
        adler_a = (adler_a + *p)      % 65521;
        adler_b = (adler_b + adler_a) % 65521;
        p ++;
    }
    adler_a |= (adler_b << 16);
    adler_b  = (p - p_last_blk);        // length of the last deflate block
    p_last_blk[-5] = 1;
    p_last_blk[-4] = (  adler_b    ) & 0xFF;
    p_last_blk[-3] = (  adler_b >>8) & 0xFF;
    p_last_blk[-2] = ((~adler_b)   ) & 0xFF;
    p_last_blk[-1] = ((~adler_b)>>8) & 0xFF;
    *p++ = (adler_a>>24) & 0xFF;
    *p++ = (adler_a>>16) & 0xFF;
    *p++ = (adler_a>> 8) & 0xFF;
    *p++ = (adler_a    ) & 0xFF;
    write_png_chunk((char*)"IDAT", p_dst, (size_t)(p-p_dst), fp);
    
    write_png_chunk((char*)"IEND", p_dst, 0, fp);
    
    free(p_dst);
    fclose(fp);
    return 0;
}



#include "uPNG/uPNG.h"


// return:  NULL     : failed
//          non-NULL : pointer to image pixels, allocated by malloc(), need to be free() later
uint8_t* loadPNGImageFile (const char *p_filename, int *p_is_rgb, uint32_t *p_height, uint32_t *p_width) {
    upng_t     *p_upng;
    upng_error  err;
    upng_format png_format;
    static const char *upng_format_names[] = {
        (const char*)"BADFORMAT",
        (const char*)"RGB8",
        (const char*)"RGB16",
        (const char*)"RGBA8",
        (const char*)"RGBA16",
        (const char*)"LUMA1",
        (const char*)"LUMA2",
        (const char*)"LUMA4",
        (const char*)"LUMA8",
        (const char*)"LUMA_ALPHA1",
        (const char*)"LUMA_ALPHA2",
        (const char*)"LUMA_ALPHA4",
        (const char*)"LUMA_ALPHA8"
    };
    size_t img_size;
    uint8_t *p_dst_base, *p_dst;
    const uint8_t *p_src;
    
    p_upng = upng_new_from_file(p_filename);
    
    if (p_upng == NULL)
        return NULL;
    
    err = upng_decode(p_upng);
    
    if (err != UPNG_EOK) {
        if (err==UPNG_EUNSUPPORTED || err==UPNG_EUNINTERLACED || err==UPNG_EUNFORMAT)
            printf("   ***ERROR: this PNG format is not-yet supported, error code = %d\n", err);
        upng_free(p_upng);
        return NULL;
    }
    
    png_format = upng_get_format(p_upng);
    
    if (png_format != UPNG_RGBA8 && png_format != UPNG_RGB8 && png_format != UPNG_LUMINANCE8) {
        printf("   ***ERROR: only support LUMA8, RGB8, and RGBA8. But this PNG is %s\n", upng_format_names[png_format]);
        upng_free(p_upng);
        return NULL;
    }
    
    *p_is_rgb = (png_format != UPNG_LUMINANCE8);
    *p_height = upng_get_height(p_upng);
    *p_width  = upng_get_width(p_upng);
    
    img_size = (size_t)((*p_is_rgb)?3:1) * (*p_height) * (*p_width);
    
    p_dst_base = p_dst = (uint8_t*)malloc(img_size);
    
    if (p_dst_base) {
        size_t i;
        p_src = upng_get_buffer(p_upng);
        if (png_format == UPNG_RGBA8) {
            printf("   *warning: disard alpha channel of this PNG\n");
            for (i=(size_t)(*p_height)*(*p_width); i>0; i--) {
                p_dst[0] = p_src[0];
                p_dst[1] = p_src[1];
                p_dst[2] = p_src[2];
                p_dst += 3;
                p_src += 4;
            }
        } else {
            for (i=img_size; i>0; i--) {
                *(p_dst++) = *(p_src++);
            }
        }
    }
    
    upng_free(p_upng);
    
    return p_dst_base;
}
