#ifndef __MAPPER_H__
#define __MAPPER_H__

#include <cstdint>


template <int16_t MAXVAL>
class Mapper {
private:
    const static int16_t nBIT = ((MAXVAL+1+15) / 16) * 16;
    int16_t map          [MAXVAL+1];
    int16_t map_backward [MAXVAL+1];
    
    // encode bit-mask (map) to stream
    // code : 0bbbbbbbbbbbbbbb  | bbbbbbbbbbbbbbb is 15-bit data                                            |
    //        1xxxxxxxxxyyyyyy  | repeat prevb for xxxxxxxxx+16 times, and repeat ~prevb for yyyyyy+1 times |
    
    inline static void getPrevB (int16_t &prevb, uint16_t code) {  // get prevb by last code
        if (code & 0x8000) {
            if ((code & 0x3F) == 0x3F) prevb = !prevb;
        } else {
            prevb = code & 1;
        }
    }
    
    inline uint16_t *encodeMask (uint16_t *p_buf) {
        int16_t prevb = 0;
        for (int i=0; i<=MAXVAL; ) {
            uint16_t code = 0x8000;
            int j;
            for (j=i; j<=MAXVAL && map[j]==prevb; j++);
            if (0<=(j-i-16) && (j-i-16)<512) {
                code |= (j-i-16) << 6;
                for (i=j+1; i<=MAXVAL && (i-j)<64 && map[i]==(!prevb); i++);
                code |= (i-j-1);
            } else {
                for (j=i; (i-j)<15; i++) {
                    code <<= 1;
                    if (i <= MAXVAL)
                        code |= map[i] & 1;
                }
            }
            *(p_buf++) = code;
            getPrevB(prevb, code);
        }
        return p_buf;
    }
    
    inline uint16_t *decodeMask (uint16_t *p_buf) {
        int16_t prevb = 0;
        for (int i=0; i<=MAXVAL; ) {
            uint16_t code = *(p_buf++);
            if (code & 0x8000) {
                for (int j=(((code>>6)&0x1FF)+16); j>0; j--)
                    if (i <= MAXVAL)
                        map[i++] = prevb;
                for (int j=((code&0x3F)+1); j>0; j--)
                    if (i <= MAXVAL)
                        map[i++] = !prevb;
            } else {
                for (int j=15-1; j>=0; j--)
                    if (i <= MAXVAL)
                        map[i++] = (code>>j) & 1;
            }
            getPrevB(prevb, code);
        }
        return p_buf;
    }
    
public:
    inline Mapper () {
        for (int16_t i=0; i<=MAXVAL; i++) {
            map[i] = 0;
        }
    }
    
    inline void add (int16_t v) {
        map[v] = 1;
    }
    
    inline uint16_t *maskCodec (uint16_t *p_buf, bool IS_ENC) {
        if (IS_ENC) return encodeMask(p_buf);
        else        return decodeMask(p_buf);
    }
    
    inline int16_t prepareMap () {
        int16_t maxv=-1, w=0;
        for (int16_t v=0; v<=MAXVAL; v++) {
            if (map[v]) {
                map         [v] = w;
                map_backward[w] = v;
                w ++;
                maxv ++;
            }
        }
        return maxv;
    }
    
    inline void forward (int16_t &v) {
        v = map[v];
    }
    
    inline void backward (int16_t &v) {
        v = map_backward[v];
    }
};

#endif  // __MAPPER_H__

