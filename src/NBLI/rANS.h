#ifndef __R_ANS_H__
#define __R_ANS_H__

#include <cstdint>


#define    NORM_BIT    14
#define    NORM_SUM    (1 << NORM_BIT)
#define    NORM_MASK   (NORM_SUM - 1)
#define    CODE_BIT    16
#define    CODE_LOW    (1 << CODE_BIT)
#define    CODE_MASK   (CODE_LOW - 1)
#define    CODE_HIGH   ((1 << (2*CODE_BIT-NORM_BIT)) - 1)



template <int VAL_BIT>
class Histogram {
private :
    union HistItem_t {
        struct {
            uint16_t hnrm;   // normalized histogram value
            uint16_t hsum;   // prefix-sum of histogram
        };
        uint32_t count;      // raw histogram (to count the input), need to be normalized later
    };                       // note: {count} share space with {hsum,value}
    
    HistItem_t hist [1<<VAL_BIT];
    
public :
    inline Histogram () {
        static_assert(2<=VAL_BIT && VAL_BIT<=10);
        static_assert(9<=NORM_BIT && NORM_BIT<=14);
        for (int i=0; i<(1<<VAL_BIT); i++)
            hist[i].count = 0;
    }
    
    inline void add (int i) {
        hist[i].count ++;
    }
    
    inline void get (int i, uint16_t &_hnrm, uint16_t &_hsum) {
        HistItem_t item = hist[i];
        _hnrm = item.hnrm;
        _hsum = item.hsum;
    }
    
    // normalize histogram, let its sum be NORM_SUM
    inline void normalize () {
        int nz_count = 0, nz_index = 0;
        uint32_t sum = 0;
        
        for (int i=0; i<(1<<VAL_BIT); i++) {
            if (hist[i].count > 0) {
                sum += hist[i].count;
                nz_count ++;     // how many non-zero values ?
                nz_index = i;    // a non-zero value
            }
        }
        
        if (nz_count <= 1) {
            hist[ nz_index].count = NORM_SUM - 1;
            hist[!nz_index].count = 1;
        
        } else {
            double scale = (double)NORM_SUM / sum;  // this func is only used in encoder, so using floating-point types here will not result in data being unable to be decoded across platforms
            
            sum = 0;
            
            for (int i=0; i<(1<<VAL_BIT); i++) {
                if (hist[i].count > 0) {
                    hist[i].count = (uint32_t)(0.49 + scale * hist[i].count);
                    if (hist[i].count == 0) hist[i].count = 1;
                    sum += hist[i].count;
                }
            }
            
            for (int i=0; sum>NORM_SUM; i=(i+1)&((1<<VAL_BIT)-1)) {
                if (hist[i].count > 1) {
                    hist[i].count --;
                    sum --;
                }
            }
            
            for (int i=0; sum<NORM_SUM; i=(i+1)&((1<<VAL_BIT)-1)) {
                if (hist[i].count > 0) {
                    hist[i].count ++;
                    sum ++;
                }
            }
        }
        
        for (int i=0; i<(1<<VAL_BIT); i++) {
            hist[i].hnrm = hist[i].count;
            hist[i].hsum = 0;
        }
    }
    
    inline void calculateAccumlate () {
        hist[0].hsum = 0;
        for (int i=1; i<(1<<VAL_BIT); i++) {
            hist[i].hsum = hist[i-1].hsum + hist[i-1].hnrm;
        }
    }
    
    inline void calculateDecodeLookupTable (uint16_t dlut[NORM_SUM]) {
        for (int i=0; i<(1<<VAL_BIT)-1; i++) {
            for (int j=hist[i].hsum; j<hist[i+1].hsum; j++) {
                dlut[j] = i;
            }
        }
        for (int j=hist[(1<<VAL_BIT)-1].hsum; j<NORM_SUM; j++) {
            dlut[j] = (1<<VAL_BIT)-1;
        }
    }
    
    // encode a normalized histogram to a uint16_t stream
    // use a simple compression code:
    //         |  a 16-bit code   | explain                                                               |
    //   code1 | 00AAAAAAAAAAAAAA | where AAAAAAAAAAAAAAA is a 14-bit histogram value                     |
    //   code2 | 01BBBBBBBCCCCCCC | where BBBBBBB and CCCCCCC are two 7-bit histogram deltas              |
    //   code3 | 10DDDDDDDEEEEEEE | where DDDDDDD and EEEEEEE are two 7-bit histogram values              |
    //   code4 | 110FFFFFGGGGHHHH | where FFFFF, GGGG, and HHHH are three 5-bit or 4-bit histogram values |
    //   code5 | 1110IIIJJJKKKLLL | where III, JJJ, KKK, and LLL are four 3-bit histogram values          |
    //   code6 | 1111MMRRRRRRRRRR | repeat MM for RRRRRRRRRR+1 times                                      |
    // note: only need to decode hist[1~(1<<VAL_BIT)-1]. The decoder calculates hist[0] = NORM_SUM - sum(hist[1~(1<<VAL_BIT)-1])
    inline uint16_t* encode (uint16_t *p_buf) {
        int16_t hprev = 0;
        for (int i=1; i<(1<<VAL_BIT); ) {
            int16_t h0 = hist[i].hnrm;
            int len;
            for (len=1; i+len<(1<<VAL_BIT) && h0==hist[i+len].hnrm; len++);
            if (len > 3 && h0 < 4) {
                *(p_buf++) = (h0<<10) | (len-1) | 0xF000;
                i += len;
            } else {
                int16_t h1 = (i < (1<<VAL_BIT)-1) ? hist[i+1].hnrm : INT16_MAX;
                int16_t h2 = (i < (1<<VAL_BIT)-2) ? hist[i+2].hnrm : INT16_MAX;
                int16_t h3 = (i < (1<<VAL_BIT)-3) ? hist[i+3].hnrm : INT16_MAX;
                int16_t d0 =                    (hprev-h0) & NORM_MASK;
                int16_t d1 = (i < (1<<VAL_BIT)-1) ? ((h0 - h1) & NORM_MASK) : INT16_MAX;
                
                if               (h0<8   && h1<8   && h2<8   && h3<8) {
                    *(p_buf++) = (h0<<9) | (h1<<6) | (h2<<3) |  h3 | 0xE000;
                    i += 4;
                } else if        (h0<32  && h1<16  && h2<16) {
                    *(p_buf++) = (h0<<8) | (h1<<4) |  h2           | 0xC000;
                    i += 3;
                } else if        (h0<128 && h1<128) {
                    *(p_buf++) = (h0<<7) |  h1                     | 0x8000;
                    i += 2;
                } else if        (d0<128&& d1<128) {
                    *(p_buf++) = (d0<<7) |  d1                     | 0x4000;
                    i += 2;
                } else {
                    *(p_buf++) =  h0;
                    i += 1;
                }
            }
            hprev = hist[i-1].hnrm;
        }
        return p_buf;
    }
    
    // decode a normalized histogram from a uint16_t stream
    inline uint16_t* decode (uint16_t *p_buf) {
        for (uint16_t i=0; i<(1<<VAL_BIT); i++) hist[i].hnrm = 0;
        for (uint16_t i=1; i<(1<<VAL_BIT); ) {
            uint16_t code = *(p_buf++);
            switch (code >> 12) {
                case 0:  case 1:  case 2:  case 3:              // code1
                    hist[i++].hnrm = code;
                    break;
                case 4:  case 5:  case 6:  case 7:              // code2
                    hist[i].hnrm = (hist[i-1].hnrm-((code>>7)&0x7F)) & NORM_MASK;
                    i++;
                    hist[i].hnrm = (hist[i-1].hnrm-( code    &0x7F)) & NORM_MASK;
                    i++;
                    break;
                case 8:  case 9:  case 10:  case 11:            // code3
                    hist[i++].hnrm = (code>>7) & 0x7F;
                    hist[i++].hnrm = (code   ) & 0x7F;
                    break;
                case 12: case 13:                               // code4
                    hist[i++].hnrm = (code>>8) & 0x1F;
                    hist[i++].hnrm = (code>>4) & 0x0F;
                    hist[i++].hnrm = (code   ) & 0x0F;
                    break;
                case 14:                                        // code5
                    hist[i++].hnrm = (code>>9) & 0x07;
                    hist[i++].hnrm = (code>>6) & 0x07;
                    hist[i++].hnrm = (code>>3) & 0x07;
                    hist[i++].hnrm = (code   ) & 0x07;
                    break;
                default:                                        // code6
                    uint16_t hrep = (code>>10) & 0x003;
                    int      len  = (code    ) & ((1<<VAL_BIT)-1);
                    for (len++; len>0; len--) hist[i++].hnrm = hrep;
                    break;
            }
        }
        hist[0].hnrm = NORM_SUM;
        for (int i=1; i<(1<<VAL_BIT); i++) hist[0].hnrm -= hist[i].hnrm;
        return p_buf;
    }
};



class CtxValPairStack {
private:
    uint16_t *p_top, *p_btm;
    
public:
    inline void operator= (uint16_t *_p_buf) {
        p_top = p_btm = _p_buf;
    }
    
    inline bool not_empty () {
        return p_top != p_btm;
    }
    
    inline void push (uint16_t ctx, uint16_t val) {    // 7-bit ctx, 9-bit val
        p_top --;
        p_top[0] = (ctx<<9) | val;
    }
    
    inline void pop (uint16_t &ctx, uint16_t &val) {
        ctx = p_top[0] >> 9;
        val = p_top[0] & 0x1FF;
        p_top ++;
    }
};



template <bool IS_ENC, int N_CTX, int VAL_BIT>
class rANSwithGlobalHistogram {
private :
    CtxValPairStack stack;
    Histogram<VAL_BIT> *hists;
    uint16_t (*tab_dlut) [NORM_SUM];
    uint16_t       *p_buf;
    uint32_t        ans;
    
    inline static void reverseStream (uint16_t *p_start, uint16_t *p_final) {
        p_final --;
        while (p_start < p_final) {
            uint16_t tmp = *p_start;
            *p_start = *p_final;
            *p_final = tmp;
            p_start ++;
            p_final --;
        }
    }
    
    inline void loadANS () {
        ans <<= CODE_BIT;
        ans  |= *(p_buf++);
    }
    
    inline void writeANS () {
        *(p_buf++) = ans;
        ans >>= CODE_BIT;
    }
    
    // add a data for encoder
    inline void add (uint16_t ctx, uint16_t val) {
        hists[ctx].add(val);
        stack.push(ctx, val);
    }
    
    inline void encode (uint16_t ctx, uint16_t val) {
        uint16_t hnrm, hsum;
        hists[ctx].get(val, hnrm, hsum);
        uint32_t nan = ans / hnrm;
        if (nan > CODE_HIGH) {
            writeANS();
            nan = ans / hnrm;
        }
        ans %= hnrm;
        ans += (nan << NORM_BIT);
        ans += hsum;
    }
    
    inline uint16_t decode (uint16_t ctx) {
        uint16_t hnrm, hsum;
        uint16_t lb  = ans & NORM_MASK;
        uint16_t val = tab_dlut[ctx][lb];
        hists[ctx].get(val, hnrm, hsum);
        ans>>= NORM_BIT;
        ans *= hnrm;
        ans += lb;
        ans -= hsum;
        if (ans < CODE_LOW)
            loadANS();
        return val;
    }
    
public:
    inline uint16_t codec (uint16_t ctx, uint16_t val=0) {
        if (IS_ENC)
            add(ctx, val);
        else
            val = decode(ctx);
        return val;
    }
    
    inline rANSwithGlobalHistogram (uint16_t *_p_buf, uint16_t *_p_buf_end_for_encoder=NULL) {   // construct
        static_assert(1<=N_CTX && N_CTX<=64);
        
        p_buf = _p_buf;
        
        ans = CODE_LOW;
        
        hists = new Histogram<VAL_BIT> [N_CTX];
        
        if (IS_ENC) {
            stack = _p_buf_end_for_encoder;
            
        } else {
            tab_dlut = (uint16_t (*) [NORM_SUM]) new uint16_t[N_CTX * NORM_SUM];
            
            for (uint16_t ctx=0; ctx<N_CTX; ctx++) {
                p_buf = hists[ctx].decode(p_buf);
                hists[ctx].calculateAccumlate();
                hists[ctx].calculateDecodeLookupTable(tab_dlut[ctx]);
            }
            
            loadANS();
            loadANS();
        }
    }
    
    inline ~rANSwithGlobalHistogram () {
        delete[] hists;
        if (!IS_ENC)
            delete[] tab_dlut;
    }
    
    // encode all data, call it when all data have been added
    inline uint16_t *flush () {
        if (IS_ENC) {
            for (uint16_t ctx=0; ctx<N_CTX; ctx++) {
                hists[ctx].normalize();
                hists[ctx].calculateAccumlate();
                p_buf = hists[ctx].encode(p_buf);
            }
            
            uint16_t *p_buf_ans_start = p_buf;
            
            while (stack.not_empty()) {
                uint16_t  ctx, val;
                stack.pop(ctx, val);
                encode   (ctx, val);
            }
            
            writeANS();
            writeANS();
            
            reverseStream(p_buf_ans_start, p_buf);
        }
        
        return p_buf;
    }
};


#endif // __R_ANS_H__
