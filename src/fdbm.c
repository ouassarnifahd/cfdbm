#include "common.h"
#include "fdbm.h"
#include "ipdild_data.h"
#include "buffer_data.h"

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt

#define limit(min, x, max) ((max<(x))? max : (((x)<min) ? min : (x)))

// fftw_plan fftw_plan_dft_r2c_1d(int n, double *in, fftw_complex *out, unsigned flags);
// fftw_plan fftw_plan_dft_c2r_1d(int n, fftw_complex *in, double *out, unsigned flags);

// static void mult_signal_by_sine_windows(short* signal, int size) {
//
// }

static void get_buffer_LR(const short const * buffer, int size, float* R, float* L) {
    for (register int i = 0; i < size/2; ++i) {
        R[i] = buffer[2 * i]/(float)SINT16_MAX;
        L[i] = buffer[2 * i + 1]/(float)SINT16_MAX;
    }
}

static void set_buffer_LR(const float const * R, const float const * L, short* buffer, int size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2 * i] = limit(-SINT16_MAX, (short)R[i] * SINT16_MAX, SINT16_MAX);
        buffer[2 * i + 1] = limit(-SINT16_MAX, (short)L[i] * SINT16_MAX, SINT16_MAX);
    }
}

void applyFBDM_simple1(char* buffer, int size, int doa) {
    short* samples = (short*) buffer;

    // split
    // get_buffer_LR(samples, size, audio_float_bufferR, audio_float_bufferL);

    // 2 fft
    // fftwf_plan_dft_r2c_1d(size, audio_float_bufferR, audio_fft_bufferR, FFTW_PRESERVE_INPUT);
    // fftwf_plan_dft_r2c_1d(size, audio_float_bufferL, audio_fft_bufferL, FFTW_PRESERVE_INPUT);

    // compare with DataBase (dicotomie):
    // -90:90 --> -90:0 --> -45:0 --> -45:-25 --> -45:-35 --> -40:-35 --> -40

    // apply Gain

    // 2 ifft
    // fftwf_plan_dft_c2r_1d(size, audio_fft_bufferR, audio_float_bufferR, NULL);
    // fftwf_plan_dft_c2r_1d(size, audio_fft_bufferL, audio_float_bufferL, NULL);

    // reassemble
    // set_buffer_LR(audio_float_bufferR, audio_float_bufferL, buffer, size);
}

void applyFBDM_simple2(char* buffer, int size, int doa1, int doa2);

void applyFBDM(char* buffer, int size, const int const * doa, int sd);
