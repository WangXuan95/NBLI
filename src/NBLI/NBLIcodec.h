#ifndef __NBLI_CODEC_H__
#define __NBLI_CODEC_H__

#include <algorithm>
#include "PlaneModel.h"
#include "Mapper.h"


#define  MAX_Y   255
#define  MAX_UV  (MAX_Y*2)

inline static void to_YUV (uint8_t *p_RGB, int16_t &Y, int16_t &U, int16_t &V) {
    U = p_RGB[0];  //R
    Y = p_RGB[1];  //G
    V = p_RGB[2];  //B
    U -= Y;
    V -= Y;
    Y += (U + V + 2) >> 2;
    V -= U >> 2;
    //U -= V >> 4;
    V += MAX_Y;
    U += MAX_Y;
}

inline static void to_RGB (uint8_t *p_RGB, int16_t Y, int16_t U, int16_t V) {
    U -= MAX_Y;
    V -= MAX_Y;
    //U += V >> 4;
    V += U >> 2;
    Y -= (U + V + 2) >> 2;
    V += Y;
    U += Y;
    if (Y < 0) {Y = 0;}    if (Y > MAX_Y) {Y = MAX_Y;}
    if (U < 0) {U = 0;}    if (U > MAX_Y) {U = MAX_Y;}
    if (V < 0) {V = 0;}    if (V > MAX_Y) {V = MAX_Y;}
    p_RGB[0] = U;  //R
    p_RGB[1] = Y;  //G
    p_RGB[2] = V;  //B
}


inline static void load_YUV_with_map (uint8_t *p_RGB, int16_t &Y, int16_t &U, int16_t &V, Mapper<MAX_UV> &mapU, Mapper<MAX_UV> &mapV, bool use_map) {
    to_YUV(p_RGB, Y, U, V);
    if (use_map) {
        mapU.forward(U);
        mapV.forward(V);
    }
}


inline static void store_YUV_with_map (uint8_t *p_RGB, int16_t Y, int16_t U, int16_t V, Mapper<MAX_UV> &mapU, Mapper<MAX_UV> &mapV, bool use_map) {
    if (use_map) {
        mapU.backward(U);
        mapV.backward(V);
    }
    to_RGB(p_RGB, Y, U, V);
}


template<bool IS_RGB, bool IS_ENC, bool USE_AVP, typename CODEC_T>
inline static uint16_t *NBLIcodec (uint16_t *p_buf, uint16_t *p_buf_end, uint8_t *p_img, uint32_t height, uint32_t width, int16_t near) {
    
    int16_t mU=MAX_UV, mV=MAX_UV;
    
    Mapper<MAX_UV> mapU, mapV;
    
    bool use_map = IS_RGB && near==0;
    
    if (use_map) {
        if (IS_ENC) {
            uint8_t *p_img_start = p_img;
            for (uint32_t i=0; i<height*width; i++) {
                int16_t Y, U, V;
                to_YUV(p_img, Y, U, V);
                mapU.add(U);
                mapV.add(V);
                p_img += 3;
            }
            p_img = p_img_start;
        }
        
        p_buf = mapU.maskCodec(p_buf, IS_ENC);
        p_buf = mapV.maskCodec(p_buf, IS_ENC);
        
        mU = mapU.prepareMap();
        mV = mapV.prepareMap();
    }
    
    CODEC_T codec(p_buf, p_buf_end);
    
    PlaneModel<USE_AVP, true , 0> modelU (width, mU    , near);
    PlaneModel<USE_AVP, true , 1> modelV (width, mV    , near);
    PlaneModel<USE_AVP, false, 2> modelY (width, MAX_Y , near);
    
    for (uint32_t i=0; i<height; i++) {
        if (IS_RGB) {
            modelU.prepareAtStartOfLine();
            modelV.prepareAtStartOfLine();
        }
        modelY.prepareAtStartOfLine();
        
        for (uint32_t j=0; j<width; j++) {
            if (i>0 && j+1<width) {
                if (IS_RGB) {
                    load_YUV_with_map((p_img-width*3+3), modelY.d, modelU.d, modelV.d, mapU, mapV, use_map);
                } else {
                    modelY.d = *(p_img-width  +1);
                }
            } else {
                modelU.d = modelU.a;
                modelV.d = modelV.a;
                modelY.d = modelY.a;
            }
            
            if (i>1 && j+2<width) {
                if (IS_RGB) {
                    load_YUV_with_map((p_img-width*6+6), modelY.r, modelU.r, modelV.r, mapU, mapV, use_map);
                } else {
                    modelY.r = *(p_img-width*2+2);
                }
            } else {
                modelU.r = modelU.d;
                modelV.r = modelV.d;
                modelY.r = modelY.d;
            }
            
            if (IS_ENC) {
                if (IS_RGB) {
                    load_YUV_with_map(p_img, modelY.x, modelU.x, modelV.x, mapU, mapV, use_map);
                } else {
                    modelY.x = *p_img;
                }
            }
            
            if (IS_RGB) {
                modelU.prepare(j);
                
                if (IS_ENC) {
                    modelU.mapXtoW();
                    codec.codec(N_QD+modelU.qd, modelU.w);
                } else {
                    modelU.w = codec.codec(N_QD+modelU.qd);
                }
                modelU.mapWtoX();
                
                modelU.postpare();
                
                modelV.err2 = (std::abs(modelV.err2) + std::abs(modelU.err2) + 1) >> 1;
                
                modelV.prepare(j, modelU.err2>0);
                
                if (IS_ENC) {
                    modelV.mapXtoW();
                    codec.codec(2*N_QD+modelV.qd, modelV.w);
                } else {
                    modelV.w = codec.codec(2*N_QD+modelV.qd);
                }
                modelV.mapWtoX();
                
                modelV.postpare();
                
                modelY.err2 = (std::abs(modelY.err2) + std::abs(modelU.err2) + std::abs(modelV.err2)) >> 1;
            }
            
            {
                if (IS_RGB)
                    modelY.prepare(j, ((modelU.err2>0)+((modelV.err2>0)<<1)));
                else
                    modelY.prepare(j);
                
                if (IS_ENC) {
                    modelY.mapXtoW();
                    codec.codec(modelY.qd, modelY.w);
                } else {
                    modelY.w = codec.codec(modelY.qd);
                }
                modelY.mapWtoX();
                
                modelY.postpare();
            }
            
            if (IS_RGB) {
                store_YUV_with_map(p_img, modelY.x, modelU.x, modelV.x, mapU, mapV, use_map);
            } else {
                *p_img = modelY.x;
            }
            
            if (IS_RGB) {
                modelU.moveTemplate();
                modelV.moveTemplate();
            }
            modelY.moveTemplate();
            
            p_img += IS_RGB ? 3 : 1;
        }
    }
    
    return codec.flush();
}


#endif // __NBLI_CODEC_H__
