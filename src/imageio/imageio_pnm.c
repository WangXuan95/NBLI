#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


// return:   0 : success    1 : failed
// support:
//    - raw PGM (start with 'P5')
//    - raw PPM (start with 'P6')
int writePNMImageFile (const char *p_filename, const uint8_t *p_buf, int is_rgb, uint32_t height, uint32_t width) {
    size_t len;
    int failed;
    FILE *fp;
    
    if (width < 1 || height < 1)
        return 1;
    
    if ((fp = fopen(p_filename, "wb")) == NULL)
        return 1;
    
    fprintf(fp, "P%c\n%d %d\n255\n", (is_rgb?'6':'5'), width, height);
    
    len = (size_t)(is_rgb?3:1) * width * height;
    
    failed = (len != fwrite(p_buf, sizeof(uint8_t), len, fp));
    
    fclose(fp);
    return failed;
}



// get next number (regard # as comment)
static int fget_next_number (FILE *fp, int *p_num) {
    *p_num = -1;
    for (;;) {
        int ch = fgetc(fp);
        if (ch == EOF) {
            return ch;
        } else if (ch == '#') {
            for (;;) {
                ch = fgetc(fp);
                if (ch == EOF) {
                    return ch;
                }
                if (ch == '\r' || ch == '\n') {
                    break;
                }
            }
        } else if (ch >= '0' && ch <= '9') {
            *p_num = 0;
            while (ch >= '0' && ch <= '9') {
                (*p_num) *= 10;
                (*p_num) += (ch - '0');
                ch = fgetc(fp);
            }
            return ch;
        }
    }
}



// return:  NULL     : failed
//          non-NULL : pointer to image pixels, allocated by malloc(), need to be free() later
// support:
//    - plain PBM (start with 'P1')
//    - plain PGM (start with 'P2')
//    - plain PPM (start with 'P3')
//    - raw   PBM (start with 'P4')
//    - raw   PGM (start with 'P5')
//    - raw   PPM (start with 'P6')
uint8_t* loadPNMImageFile (const char *p_filename, int *p_is_rgb, uint32_t *p_height, uint32_t *p_width) {
    int      ch, P, T, W, H, maxval=1;
    size_t   i, j, len;
    uint8_t *p_buf;
    FILE *fp;
    
    if ((fp = fopen(p_filename, "rb")) == NULL)
        return NULL;
    
    P  = fgetc(fp);
    T  = fgetc(fp) - (int)'0';
    ch = fget_next_number(fp, &W);
    ch = fget_next_number(fp, &H);
    
    if (T==2 || T==3 || T==5 || T==6) { // PGM or PPM
        ch = fget_next_number(fp, &maxval);
    }
    
    if (P!='P' || T<1 || T>6 || W<1 || H<1 || maxval<1 || maxval>255) {
        fclose(fp);
        return NULL;
    }
    
    while (ch!='\n' && ch!=EOF) {
        ch = fgetc(fp);
    }
    
    *p_width  = W;
    *p_height = H;
    *p_is_rgb = (T==3 || T==6);         // PPM
    
    len = (size_t)((*p_is_rgb) ? 3 : 1) * W * H;
    
    p_buf = (uint8_t*)malloc(len + 8);
    
    if (p_buf) {
        int failed = 0;
        
        if (T==5 || T==6) {             // raw PGM or PPM
            failed = (len != fread(p_buf, sizeof(uint8_t), len, fp));
            
        } else if (T == 4) {            // raw PBM
            uint8_t *p = p_buf;
            for     (i=0; i<(size_t)H; i++) {
                for (j=0; j<(size_t)W; j+=8) {
                    ch = fgetc(fp);
                    failed = failed || (ch == EOF);
                    p[j  ] = ((ch>>7) & 1) ? 0 : 255;
                    p[j+1] = ((ch>>6) & 1) ? 0 : 255;
                    p[j+2] = ((ch>>5) & 1) ? 0 : 255;
                    p[j+3] = ((ch>>4) & 1) ? 0 : 255;
                    p[j+4] = ((ch>>3) & 1) ? 0 : 255;
                    p[j+5] = ((ch>>2) & 1) ? 0 : 255;
                    p[j+6] = ((ch>>1) & 1) ? 0 : 255;
                    p[j+7] = ( ch     & 1) ? 0 : 255;
                }
                p += W;
            }
            
        } else {                        // plain PBM, PGM or PPM
            for (i=0; i<len; i++) {
                fget_next_number(fp, &ch);
                failed = failed || (ch < 0);
                p_buf[i] = (T!=1) ? ch : (ch ? 0 : 255);
            }
        }
        
        if (failed) {
            free(p_buf);
            p_buf = NULL;
        }
    }
    
    fclose(fp);
    return p_buf;
}
