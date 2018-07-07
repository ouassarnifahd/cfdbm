#include "common.h"
#include "fdbm.h"
#include "dft.h"
#include "ipdild_data.h"

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt

#define limit(min, x, max) ((max<(x))? max : (((x)<min) ? min : (x)))

static void get_buffer_LR(const int16_t* buffer, int size, float* L, float* R) {
    for (register int i = 0; i < size/2; ++i) {
        L[i] = buffer[2u * i]/(float)SINT16_MAX;
        R[i] = buffer[2u * i + 1u]/(float)SINT16_MAX;
        // log_printf("(L=%hi, R=%hi) -> (L=%f, R=%f)\n", buffer[2u * i], buffer[2u * i + 1u], L[i], R[i]);
    }
}

static void set_buffer_LR(const float* L, const float* R, int16_t* buffer, int size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2u * i] = limit(-SINT16_MAX, (int16_t)L[i] * SINT16_MAX, SINT16_MAX);
        buffer[2u * i + 1u] = limit(-SINT16_MAX, (int16_t)R[i] * SINT16_MAX, SINT16_MAX);
        // log_printf("(L=%f, R=%f) -> (L=%hi, R=%hi)\n", L[i], R[i], buffer[2u * i], buffer[2u * i + 1u]);
    }
}

void applyFDBM_simple1(char* buffer, int size, int doa) {
    debug("Entering FDBM...");
    int16_t* samples = (int16_t*) buffer;

    float audio_R[SAMPLES_COUNT] = {0};
    float audio_L[SAMPLES_COUNT] = {0};

    // split
    get_buffer_LR(samples, size, audio_L, audio_R);
    warning("Buffer Splited!");

    // TODO? Function context stack expantion!!
    float fft_re_R[CHANNEL_SAMPLES_COUNT] = {0};
    float fft_im_R[CHANNEL_SAMPLES_COUNT] = {0};

    float fft_re_L[CHANNEL_SAMPLES_COUNT] = {0};
    float fft_im_L[CHANNEL_SAMPLES_COUNT] = {0};

    fcomplex_t fft_R = {
        fft_re_R,
        fft_im_R,
    };

    fcomplex_t fft_L = {
        fft_re_L,
        fft_im_L,
    };

    float data_ILD[CHANNEL_SAMPLES_COUNT] = {0};
    float data_IPD[CHANNEL_SAMPLES_COUNT] = {0};

    // 2D fft
    dft2_IPD_ILD(audio_L, audio_R, &fft_L, &fft_R, data_ILD, data_IPD, size);
    warning("2D fft done!");

    // compare with DataBase (dicotomie):
    // -90:90 --> -90:0 --> -45:0 --> -45:-25 --> -45:-35 --> -40:-35 --> -40

    // apply Gain

    // 2D ifft
    idft2(&fft_L, &fft_R, audio_L, audio_R, size);
    warning("2D ifft done!");

    // reassemble
    set_buffer_LR(audio_L, audio_R, samples, size);
    warning("Buffer Reassembled!");
}

void applyFDBM_simple2(char* buffer, int size, int doa1, int doa2);

void applyFDBM(char* buffer, int size, const int const * doa, int sd);
