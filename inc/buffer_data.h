#ifndef __HEADER_BUFFER_DATA__
#define __HEADER_BUFFER_DATA__

#include "alsa_wrapper.h"

// time spent in capture = n / rate = 32 ms
#define SAMPLES_COUNT (512)
#define CHANNEL_SAMPLES_COUNT (SAMPLES_COUNT/2)

#define RAW_BUFFER_SIZE (SAMPLES_COUNT * get_frame_bytes())

#define SINT16_MAX (((1ull<<15)-1))

// extern float audio_R[SAMPLES_COUNT];
// extern float audio_L[SAMPLES_COUNT];
//
// extern float fft_re_R[CHANNEL_SAMPLES_COUNT];
// extern float fft_im_R[CHANNEL_SAMPLES_COUNT];
//
// extern float fft_re_L[CHANNEL_SAMPLES_COUNT];
// extern float fft_im_L[CHANNEL_SAMPLES_COUNT];
//
// extern float data_ILD[CHANNEL_SAMPLES_COUNT];
// extern float data_IPD[CHANNEL_SAMPLES_COUNT];

#endif /* end of include guard: __HEADER_BUFFER_DATA__ */
