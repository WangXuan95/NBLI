#ifndef __ADVANCED_PREDICTOR_H__
#define __ADVANCED_PREDICTOR_H__

#include <cstdint>


template <int32_t N>
class AdvancedPredictor {
private :
    const static int64_t ALPHA = 5;
    const static int64_t BETA  = 3;
    
    const static int64_t FB1 = 12;
    const static int64_t FB2 = 2;
    const static int64_t FB3 = (FB1 - FB2);
    
    const static int64_t BIAS      = (2    << FB2);
    const static int64_t BIAS_MAX  = (1024 << FB2);
    const static int64_t BIAS_COEF = 21;
    
    const static int32_t M = 1 + N + N*N;
    
    inline static int64_t CLIP(int64_t v, int64_t a, int64_t b)  { return (v<a) ? a : (v>b) ? b : v; }
    inline static int64_t ABS (int64_t a)                        { return (a<0) ? -a : a; }
    inline static void    SWAP(int64_t &a, int64_t &b)           { int64_t t=a; a=b; b=t; }
    
    inline static int64_t &G2D(int64_t *p, int32_t i, int32_t j) { return *(p + N*i + j); }
    
    
    // return:  false : failed
    //          true  : success
    inline static bool solveAxb (int64_t *p_mat_A, int64_t *p_vec_b) {
        int32_t i, j, k, kk;
        int64_t Akk, Aik, Akj;
        
        for (k=0; k<(N-1); k++) {
            // find main row number kk -------------------------------------
            kk = k;
            for (i=k+1; i<N; i++)
                if (ABS(G2D(p_mat_A, i, k)) > ABS(G2D(p_mat_A, kk, k)))
                    kk = i;
            
            // swap row kk and k -------------------------------------------
            if (kk != k) {
                SWAP(p_vec_b[k], p_vec_b[kk]);
                for (j=k; j<N; j++)
                    SWAP(G2D(p_mat_A, k, j), G2D(p_mat_A, kk, j));
            }
            
            // gaussian elimination ----------------------------------------
            Akk = G2D(p_mat_A, k, k);
            if (Akk == 0) return false;
            for (i=k+1; i<N; i++) {
                Aik = G2D(p_mat_A, i, k);
                G2D(p_mat_A, i, k) = 0;
                if (Aik != 0) {
                    for (j=k+1; j<N; j++) {
                        Akj = G2D(p_mat_A, k, j);
                        G2D(p_mat_A, i, j) -= Akj * Aik / Akk;
                    }
                    Akj = p_vec_b[k];
                    p_vec_b[i] -= Akj * Aik / Akk;
                }
            }
        }
        
        for (k=(N-1); k>0; k--) {
            Akk = G2D(p_mat_A, k, k);
            if (Akk == 0) return false;
            for (i=0; i<k; i++) {
                Aik = G2D(p_mat_A, i, k);
                G2D(p_mat_A, i, k) = 0;
                if (Aik != 0) {
                    Akj = p_vec_b[k];
                    p_vec_b[i] -= Akj * Aik / Akk;
                }
            }
        }
        
        return true;
    }
    
    
    int32_t width;
    int64_t maxval, fit_base, pxf;
    int64_t *p_B_row, *p_F_row, *p_B, *p_F, p_E[M], vec_n[N];
    
    
public :
    inline AdvancedPredictor (int32_t _width, int64_t _maxval) {
        width    = _width;
        maxval   = _maxval;
        fit_base = _maxval >> 1;
        
        p_B_row = new int64_t [width * M * 2];
        p_F_row = p_B_row + width*M;
        for (int32_t i=0; i<(width*M*2); i++) p_B_row[i] = 0;
    }
    
    
    inline ~AdvancedPredictor () {
        delete[] p_B_row;
    }
    
    
    inline void prepareAtStartOfLine () {
        for (int32_t i=0; i<M; i++) p_E[i] = 0;
        
        for (int32_t j=width-1; j>=0; j--) {
            int64_t *p_B  = p_B_row + (M * j);
            int64_t *p_F  = p_F_row + (M * j);
            int64_t *p_F2 = p_F_row + (M *(j+1));
            int64_t ab = BETA;
            for (int32_t k=0; k<M; k++) {
                if (j == width-1)
                    p_F[k] = 0;
                else
                    p_F[k] = (p_F2[k] * (ab-1) + ab/2) / ab;
                p_F[k] += p_B[k];
                ab = ALPHA;
            }
        }
    }
    
    
    inline void predict (int32_t j, int16_t &px, int16_t a, int16_t b, int16_t c, int16_t d, int16_t e, int16_t f, int16_t g, int16_t h, int16_t q) {
        if (N > 0) vec_n[0] = a - fit_base;
        if (N > 1) vec_n[1] = b - fit_base;
        if (N > 2) vec_n[2] = c - fit_base;
        if (N > 3) vec_n[3] = d - fit_base;
        if (N > 4) vec_n[4] = e - fit_base;
        if (N > 5) vec_n[5] = f - fit_base;
        if (N > 6) vec_n[6] = g - fit_base;
        if (N > 7) vec_n[7] = h - fit_base;
        if (N > 8) vec_n[8] = q - fit_base;
        
        p_B = p_B_row + (M * j);
        p_F = p_F_row + (M * j);
        
        
        int64_t  dataset [M];
        int64_t *vec_b = dataset + 1;
        int64_t *mat_A = dataset + 1 + N;
        
        for (int32_t k=1; k<M; k++)
            dataset[k] = p_E[k] + p_F[k];
        
        for (int32_t k=0; k<N; k++) {
            vec_b[k]         += BIAS << FB3;
            G2D(mat_A, k, k) += BIAS * N;
        }
        
        if (solveAxb(mat_A, vec_b)) {
            pxf = fit_base << FB1;
            
            for (int32_t k=0; k<N; k++) {
                int64_t Akk = G2D(mat_A, k, k);
                pxf += (((vec_b[k] * vec_n[k]) << FB2) + (Akk>>1)) / Akk;
            }
            
            pxf = CLIP(pxf, 0, (maxval<<FB1));
            
            px  = (int16_t)((pxf + (1<<FB1>>1)) >> FB1);
        } else {
            pxf = (int64_t)px << FB1;
        }
    }
    
    
    inline void update (int16_t x) {
        int64_t xx = x;
        int64_t s_curr = ABS(pxf - (xx<<FB1));
        int64_t s_sum  = (p_E[0] + p_F[0]) + (s_curr * BETA / (BETA-1));
        
        int64_t  dataset [M];
        int64_t *vec_b = dataset + 1;
        int64_t *mat_A = dataset + 1 + N;
        
        dataset[0] = s_curr;
        
        xx -= fit_base;
        
        s_sum = CLIP((s_sum+(1<<FB1)), (1<<FB1), (16<<FB1));
        int64_t s_sum2 = s_sum >> 1;
        
        for (int32_t k=0; k<N; k++)
            vec_b[k]             = (((      xx * vec_n[k]) << (4+FB1+FB1)) + s_sum2) / s_sum;   // b = x * n
        
        for (int32_t j=0; j<N; j++)
            for (int32_t k=0; k<N; k++)
                G2D(mat_A, j, k) = (((vec_n[j] * vec_n[k]) << (4+FB2+FB1)) + s_sum2) / s_sum;   // A = n * n.T
        
        int64_t ab = BETA;
        
        for (int32_t k=0; k<M; k++) {
            p_B[k] *= (ab-1);
            p_B[k] += (ab>>1);
            p_B[k] /=  ab;
            p_B[k] += dataset[k];
            p_E[k] *= (ab-1);
            p_E[k] += (ab>>1);
            p_E[k] /=  ab;
            p_E[k] += p_B[k];
            ab = ALPHA;
        }
    }
};


#endif // __ADVANCED_PREDICTOR_H__
