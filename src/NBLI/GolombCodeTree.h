#ifndef __GOLOMB_CODE_TREE_H__
#define __GOLOMB_CODE_TREE_H__

#include <cstdint>


template <uint32_t PBIT>
class ProbabilityModel {
private:
    uint16_t pr;
    
public:
    inline ProbabilityModel () {
        pr = (1<<PBIT)>>1;
    }
    
    inline double getf() const {      // only for calculating entropy
        return (double)pr / (1<<PBIT);
    }
    
    inline operator uint16_t() const {
        return pr;
    }
    
    inline void update (bool bin) {
        if (bin) pr += ((1<<PBIT)-pr) >> (PBIT-7);
        else     pr -=            pr  >> (PBIT-7);
    }
};



/*template <uint32_t PBIT, uint32_t ORDER>
class OrderedProbabilityModel {
private:
    ProbabilityModel<PBIT> pc [1<<ORDER];
    uint32_t b;
    
public:
    inline OrderedProbabilityModel () {
        b = 0;
    }
    
    inline double getf() const {      // only for calculating entropy
        return pc[b].getf();
    }
    
    inline operator uint16_t() const {
        return pc[b];
    }
    
    inline void update (bool bin) {
        pc[b].update(bin);
        b <<= 1;
        b  |= bin;
        b  &= (1<<ORDER)-1;
    }
};*/



template <uint32_t PBIT, bool IS_ENC>
class AriCodec {
private:
    uint32_t  sft, low, rng;
    uint16_t  *p_buf;
    
    inline void normalize () {
        if (IS_ENC)
            *(p_buf++) = (low >> 16);
        else
            sft = (sft<<16) | *(p_buf++);
        low <<= 16;
        rng <<= 16;
        rng  |= 0xFFFF;
    }

public:
    inline void init (uint16_t *_p_buf) {
        p_buf = _p_buf;
        sft = 0;
        if (!IS_ENC) {
            normalize();
            normalize();
        }
        low = 0;
        rng = 0xFFFFFF;
    }
    
    inline bool codec (uint16_t prob, bool bin) {
        uint32_t mid = (uint64_t)prob * rng >> PBIT;
        
        if (!IS_ENC)
            bin = (sft <= low+mid);
        
        if (bin) {
            rng = mid;
        } else {
            mid ++;
            rng -= mid;
            low += mid;
        }
        
        if (rng < 0xFF) {
            if ((low&0xFFFF) + rng > 0xFFFF)
                rng = 0xFFFF - (0xFFFF & low);
            normalize();
        }
        
        return bin;
    }
    
    inline uint16_t *flush() {
        if (IS_ENC) {
            normalize();
            normalize();
        }
        return p_buf;
    }
};



template <bool IS_ENC, int N_CH, int N_CTX>
class GolombCodeTree {
private :
    AriCodec         <14, IS_ENC> ac;
    ProbabilityModel <14> prob_array [N_CH][N_CTX+1][256];   //OrderedProbabilityModel<14, 1> 
    
    int k_step;
    
public :
    inline GolombCodeTree (uint16_t *_p_buf, uint16_t *_p_buf_end=NULL, int _k_step=3) {
        k_step = _k_step;
        ac.init(_p_buf);
    }
    
    // when N_CTX=12, k_step=3: qd = 0 1 2 3 4 5 6 7 8 9 10 11 12
    //                          k = 0 0 0 1 1 1 2 2 2 3  3  3  3   KM=3
    
    inline uint16_t codec (uint16_t ctx, uint16_t val=0) {
        uint16_t qd = ctx % N_CTX;
        uint16_t ch = ctx / N_CTX;
        
        const int KM = (N_CTX-1) / k_step;   // k_max
        int i=0, k;
        bool bin;
        
        for (;;) {
            k = qd / k_step;
            if (k > KM) k = KM;
            
            if (IS_ENC)
                bin = (((qd>=N_CTX)*256+i) >> KM) < (val >> k);
            
            bin = ac.codec((uint16_t)prob_array[ch][qd][i], bin);
            prob_array[ch][qd][i].update(bin);
            
            if (!bin)
                break;
            
            i += (1 << KM);
            if (i >= 256) {
                qd = (k + 1) * k_step;
                i >>= 1;
                if (qd >= N_CTX) {
                    qd = N_CTX;
                    i = 0;
                }
            }
        }
        
        if (!IS_ENC)
            val = (((qd>=N_CTX)*256+i) >> KM) << k;
        
        for (i++, k--; k>=0; k--) {
            if (IS_ENC)
                bin = (val >> k) & 1;
            
            bin = ac.codec((uint16_t)prob_array[ch][qd][i], bin);
            prob_array[ch][qd][i].update(bin);
            
            if (!IS_ENC)
                val += bin ? (1<<k) : 0;
            
            i += bin ? (1<<k) : 1;
        }
        
        return val;
    }
    
    inline uint16_t *flush() {
        return ac.flush();
    }
};


#endif // __GOLOMB_CODE_TREE_H__
