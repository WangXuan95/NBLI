#ifndef __PLANE_MODEL_H__
#define __PLANE_MODEL_H__

#include <cstdint>
//#include "AutoReorder.h"
#include "AdvancedPredictor.h"

#define  N_QD   12


template <bool USE_AVP, bool QUANT_TYPE, int N_EXTRA_CTX_BITS>
class PlaneModel {
public:
    int16_t a, d, r;
    int16_t x, w, qd, err2;
    
private :
    int16_t maxval, near;
    bool    SOL;
    int16_t errSOL, bSOL, dSOL, xSOL;
    int16_t b, c, e, f, g, h, q, s;
    int16_t px, adr, sign, pxc;
    int32_t ctx;
    
    AdvancedPredictor<6> *avp;
    
    //AutoReorder<24> *reorder;
    
    int32_t array_ctx [(N_QD*256)<<N_EXTRA_CTX_BITS];
    
    static const int CTX_SCALE = 11;
    static const int CTX_COEF  = 6;
    
    inline static int16_t ABS (int16_t a)                       { return (a<0) ? -a : a; }
    inline static int16_t MIN (int16_t a, int16_t b)            { return (a<b) ? a : b; }
    inline static int16_t CLIP(int16_t v, int16_t a, int16_t b) { return (v<a) ? a : (v>b) ? b : v; }
    
    inline void predict () {
        static const uint8_t lut_pt [] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7};
        
        int16_t px_ang, cost, csum, cmin, wt;
        
        cmin = csum = 2 * (ABS(a-e) + ABS(c-q) + ABS(b-c) + ABS(d-b));
        px_ang = 2 * a;
        
        cost = 2 * (ABS(a-c) + ABS(c-h) + ABS(b-f) + ABS(d-g));
        csum += cost;
        if (cmin > cost) {
            cmin = cost;
            px_ang = 2 * b;
        }
        
        cost = 2 * (ABS(a-q) + ABS(c-s) + ABS(b-h) + ABS(d-f));
        csum += cost;
        if (cmin > cost) {
            cmin = cost;
            px_ang = 2 * c;
        }
        
        cost = 2 * (ABS(a-b) + ABS(c-f) + ABS(b-g) + ABS(d-r));
        csum += cost;
        if (cmin > cost) {
            cmin = cost;
            px_ang = 2 * d;
        }
        
        cost = ABS(2*a-e-q) + ABS(2*c-q-s) + ABS(2*b-c-h) + ABS(2*d-b-f);
        csum += cost;
        if (cmin > cost) {
            cmin = cost;
            px_ang = a + c;
        }
        
        cost = ABS(2*a-q-c) + ABS(2*c-s-h) + ABS(2*b-h-f) + ABS(2*d-f-g);
        csum += cost;
        if (cmin > cost) {
            cmin = cost;
            px_ang = c + b;
        }
        
        cost = ABS(2*a-c-b) + ABS(2*c-h-f) + ABS(2*b-f-g) + ABS(2*d-g-r);
        csum += cost;
        if (cmin > cost) {
            cmin = cost;
            px_ang = b + d;
        }
        
        csum -= (7 * cmin);
        csum >>= 3;
        csum = MIN(csum, (sizeof(lut_pt)/sizeof(*lut_pt)-1));
        wt = lut_pt[csum];
        
        px = CLIP((9*(a+b)+((d-c)<<1)-e-f+4)>>3, 0, 2*maxval);
        
        px = ((8-wt)*px + wt*px_ang + 8) >> 4;
    }
    
    
    inline void getQD () {
        //static const uint8_t lut1_qd [] = {0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15};
        
        // look up this array to convert delta to quantize-delta (QD)
        static const uint8_t lut1_qd [] = {0, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11};
        static const uint8_t lut2_qd [] = {0, 1, 2   , 3   , 4, 4   , 5, 5, 5, 5      , 6, 6, 6, 6, 6, 6            , 7, 7, 7, 7, 7, 7, 7, 7, 7               , 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8                  , 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9                  , 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10                            , 11};
        
        qd = ABS(a-e) + ABS(b-c) + ABS(b-d) + ABS(a-c) + ABS(b-f) + ABS(d-g) + ABS(err2);
        
        if (QUANT_TYPE) {
            qd = MIN(qd, (sizeof(lut2_qd)/sizeof(*lut2_qd)-1));
            qd = lut2_qd[qd];
        } else {
            qd = MIN(qd, (sizeof(lut1_qd)/sizeof(*lut1_qd)-1));
            qd = lut1_qd[qd];
        }
    }
    
    inline void getContextAddr () {
        adr = qd;
        adr <<= 1;       adr |= (px > a);
        adr <<= 1;       adr |= (px > b);
        adr <<= 1;       adr |= (px > c);
        adr <<= 1;       adr |= (px > d);
        adr <<= 1;       adr |= (px > e);
        adr <<= 1;       adr |= (px > f);
        adr <<= 1;       adr |= (px > (2*a-e));
        adr <<= 1;       adr |= (px > (2*b-f));
    }
    
    inline void correctByContext () {
        sign = (ctx >> (CTX_SCALE-1)) & 1;
        pxc  = px + sign + (ctx >> CTX_SCALE);
        pxc  = CLIP(pxc, 0, maxval);
    }
    
    inline void getError () {
        err2 = CLIP((x-px), -(maxval/2), (maxval/2)) << 1;
    }
    
    inline void updateContext () {
        ctx  = ((ctx<<CTX_COEF) - ctx);
        ctx += ((int32_t)err2 << (CTX_SCALE-1));
        ctx += ((1 << (CTX_COEF-1)) - 1);
        ctx>>= CTX_COEF;
    }
    
public :
    inline void mapXtoW_lossless () {
        int16_t tw = MIN(pxc, (maxval-pxc));
        int16_t sw = (x >= pxc);
        w = ABS(x - pxc);
        if (w <= 0) {
            w = 0;
        } else if (w <= tw) {
            w = 2*w - (sw^sign);
        } else {
            w += tw;
        }
    }
    
    inline void mapWtoX_lossless () {
        int16_t tw = MIN(pxc, (maxval-pxc));
        x = pxc;
        if        (w > 2*tw) {
            x += ((pxc < ((maxval+1)/2)) ? (w-tw) : (tw-w));
        } else if (w > 0) {
            tw = (w + 1) >> 1;
            x += (((w&1) ^ sign) ? tw : -tw);
        }
    }
    
    inline void mapXtoW () {
        int16_t tw = (MIN(pxc, (maxval-pxc)) + near) / (2*near + 1);
        int16_t sw = (x >= pxc);
        w = ABS(x - pxc);
        w = (w + near) / (2*near + 1);
        if (w <= 0) {
            w = 0;
        } else if (w <= tw) {
            w = 2*w - (sw^sign);
        } else {
            w += tw;
        }
        //reorder[pxc].forward(w);
    }
    
    inline void mapWtoX () {
        //reorder[pxc].backward(w);
        //reorder[pxc].add(w);
        int16_t tw = (MIN(pxc, (maxval-pxc)) + near) / (2*near + 1);
        int16_t sw;
        if (w <= 0) {
            x  = 0;
            sw = 0;
        } else if (w <= 2*tw) {
            x  = (w + 1) / 2;
            sw = (w & 1) ^ sign;
        } else {
            x  = w - tw;
            sw = (pxc < ((maxval+1)/2)) ? 1 : 0;
        }
        x *= (2*near + 1);
        x = pxc + (sw ? x : -x);
        x = CLIP(x, 0, maxval);
    }
    
    inline void prepare (uint32_t j, int16_t extra_ctx_bits=0) {
        predict();
        if (USE_AVP)
            avp->predict(j, px, a, b, c, d, e, f, g, h, q);
        getQD();
        getContextAddr();
        if (N_EXTRA_CTX_BITS > 0) {
            adr <<= N_EXTRA_CTX_BITS;
            adr |= extra_ctx_bits & ((1<<N_EXTRA_CTX_BITS)-1);
        }
        ctx = array_ctx[adr];
        correctByContext();
    }
    
    inline void postpare () {
        getError();
        updateContext();
        array_ctx[adr] = ctx;
        if (USE_AVP)
            avp->update(x);
    }
    
    inline void prepareAtStartOfLine () {
        if (USE_AVP)
            avp->prepareAtStartOfLine();
        SOL = true;
        err2 = errSOL;
        h = s = f = bSOL;  g = dSOL;
        q = c = b = xSOL;
        e = a = b;
    }
    
    inline void moveTemplate () {
        if (SOL) {
            SOL = false;
            errSOL = err2;
            bSOL = b;
            dSOL = d;
            xSOL = x;
        }
        s = h;   h = f;   f = g;   g = r;
        q = c;   c = b;   b = d;
        e = a;   a = x;
    }
    
    inline PlaneModel (uint32_t _width, int16_t _maxval, int16_t _near) {
        maxval = _maxval;
        near = _near;
        
        bSOL = dSOL = xSOL = a = b = c = d = e = f = g = h = q = r = s = ((maxval+1)/2);
        errSOL = err2 = 0;
        
        for (int index=0; index<(int)(sizeof(array_ctx)/sizeof(array_ctx[0])); index++)
            array_ctx[index] = 0;
        
        //reorder = new AutoReorder<24> [maxval+1];
        
        if (USE_AVP)
            avp = new AdvancedPredictor<6>(_width, _maxval);
    }
    
    inline ~PlaneModel () {
        //delete[] reorder;
        if (USE_AVP)
            delete avp;
    }
};


#endif // __PLANE_MODEL_H__
