#ifndef __HEADER_BUFFER_DATA__
#define __HEADER_BUFFER_DATA__

// time spent in capture = n / rate = 32 ms
#define RAW_BUFFER_SIZE (256)
#define CHANNEL_BUFFER_SIZE (RAW_BUFFER_SIZE/2)

// #define

#define SINT16_MAX (((1ull<<15)-1))

extern double audio_R[RAW_BUFFER_SIZE];
extern double audio_L[RAW_BUFFER_SIZE];

extern double fft_re_R[CHANNEL_BUFFER_SIZE];
extern double fft_im_R[CHANNEL_BUFFER_SIZE];

extern double fft_re_L[CHANNEL_BUFFER_SIZE];
extern double fft_im_L[CHANNEL_BUFFER_SIZE];

extern double data_ILD[CHANNEL_BUFFER_SIZE];
extern double data_IPD[CHANNEL_BUFFER_SIZE];

#endif /* end of include guard: __HEADER_BUFFER_DATA__ */
