#include "common.h"
#include "fdbm.h"
#include "dft.h"
#include "ipdild_data.h"
#include "buffer_data.h"

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt

#define limit(min, x, max) ((max<(x))? max : (((x)<min) ? min : (x)))

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

    long tsc1 = get_cyclecount();
    // split
    get_buffer_LR(samples, size, audio_bufferR, audio_bufferL);
    long tsc2 = get_cyclecount();
    warning("spliting cycle time %lu", get_cyclediff(tsc1, tsc2));

    // 2 fft
    dft_pow_ang(audio_bufferR, &audio_fft_bufferR, audio_power_bufferR, audio_angle_bufferR, size);
    dft_pow_ang(audio_bufferL, &audio_fft_bufferL, audio_power_bufferL, audio_angle_bufferL, size);
    tsc1 = get_cyclecount();
    warning("2 fft cycle time %lu", get_cyclediff(tsc2, tsc1));
    // compare with DataBase (dicotomie):
    // -90:90 --> -90:0 --> -45:0 --> -45:-25 --> -45:-35 --> -40:-35 --> -40

    // apply Gain

    // 2 ifft
    idft(&audio_fft_bufferR, audio_bufferR, size);
    idft(&audio_fft_bufferL, audio_bufferL, size);
    tsc2 = get_cyclecount();
    warning("2 ifft cycle time %lu", get_cyclediff(tsc1, tsc2));

    // reassemble
    set_buffer_LR(audio_bufferL, audio_bufferL, buffer, size);
    tsc1 = get_cyclecount();
    warning("reassemble cycle time %lu", get_cyclediff(tsc2, tsc1));
}

void applyFBDM_simple2(char* buffer, int size, int doa1, int doa2);

void applyFBDM(char* buffer, int size, const int const * doa, int sd);
