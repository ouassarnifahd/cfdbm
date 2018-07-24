#ifndef __HEADER_DFT__
#define __HEADER_DFT__

#include "alsa_wrapper.h"

#define FFT_NYQUIST_FREQ_BUFFER (CHANNEL_SAMPLES_COUNT/2)

typedef struct {
    float re[CHANNEL_SAMPLES_COUNT];
    float im[CHANNEL_SAMPLES_COUNT];
} fcomplex_t;

void dft_pow_ang(float* x, fcomplex_t* X, float* P, float* A, size_t len);

void dft2_IPDILD(float* xl, float* xr, fcomplex_t* Xl, fcomplex_t* Xr,
                  float* ILD, float* IPD, size_t len);

void idft(fcomplex_t* X, float* x, size_t len);

void idft2_SINE_WIN(fcomplex_t* Xl, fcomplex_t* Xr, float* xl, float* xr, size_t len);

#endif /* end of include guard: __HEADER_DFT__ */
