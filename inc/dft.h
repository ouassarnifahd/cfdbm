#ifndef __HEADER_DFT__
#define __HEADER_DFT__

#include "buffer_data.h"

typedef struct {
    float re;
    float im;
} fcomplex_t;

extern fcomplex_t audio_fft_bufferR[BUFFER_SIZE/2 + 1];
extern fcomplex_t audio_fft_bufferL[BUFFER_SIZE/2 + 1];

void dft_pow_ang(float* x, fcomplex_t* X, float* P, float* A, size_t len);

void idft(fcomplex_t* X, float* x, size_t len);

#endif /* end of include guard: __HEADER_DFT__ */
