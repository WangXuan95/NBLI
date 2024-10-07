#ifndef __AUTO_REORDER_H__
#define __AUTO_REORDER_H__

#include <cstdint>


template <int16_t N>
class AutoReorder {
private:
    uint32_t hist    [N];
    int16_t fore_map [N];
    int16_t back_map [N];
    
public:
    inline AutoReorder () {
        for (int16_t i=0; i<N; i++) {
            hist[i] = (N - 1 - i) * 2;
            fore_map[i] = back_map[i] = i;
        }
    }
    
    inline void forward (int16_t &x) {
        x = (0<=x && x<N) ? fore_map[x] : x;
    }
    
    inline void backward (int16_t &y) {
        y = (0<=y && y<N) ? back_map[y] : y;
    }
    
    inline void add (int16_t x) {
        if (0<=x && x<N) {
            int16_t y = fore_map[x];
            hist[y] ++;
            if (y > 0) {
                int16_t  y2 = y - 1;
                int16_t  x2 = back_map[y2];
                uint32_t h  = hist[y];
                uint32_t h2 = hist[y2];
                if (h2 < h) {
                    hist[y ] = h2;
                    hist[y2] = h;
                    back_map[y ] = x2;
                    back_map[y2] = x;
                    fore_map[x ] = y2;
                    fore_map[x2] = y;
                }
            }
        }
    }
};

#endif // __AUTO_REORDER_H__
