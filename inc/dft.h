#ifndef __HEADER_DFT__
#define __HEADER_DFT__

#include "buffer_data.h"

typedef struct {
    float *re;
    float *im;
} fcomplex_t;

extern fcomplex_t fft_R;
extern fcomplex_t fft_L;

void dft_pow_ang(float* x, fcomplex_t* X, float* P, float* A, size_t len);

void dft2_IPD_ILD(float* xl, float* xr, fcomplex_t* Xl, fcomplex_t* Xr,
                  float* ILD, float* IPD, size_t len);

void idft(fcomplex_t* X, float* x, size_t len);

void idft2(fcomplex_t* Xl, fcomplex_t* Xr, float* xl, float* xr, size_t len);

#endif /* end of include guard: __HEADER_DFT__ */
