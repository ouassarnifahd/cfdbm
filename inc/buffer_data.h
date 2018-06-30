#ifndef __HEADER_BUFFER_DATA__
#define __HEADER_BUFFER_DATA__

// time spent in capture = n / rate = 32 ms
#define RAW_BUFFER_SIZE (512)
#define DFT_BUFFER_SIZE (RAW_BUFFER_SIZE/2)

#define

#define SINT16_MAX (((1ull<<15)-1))

extern double audio_bufferR[BUFFER_SIZE];
extern double audio_bufferL[BUFFER_SIZE];

extern double audio_re_bufferR[BUFFER_SIZE/2 + 1];
extern double audio_im_bufferR[BUFFER_SIZE/2 + 1];

extern double audio_re_bufferL[BUFFER_SIZE/2 + 1];
extern double audio_im_bufferL[BUFFER_SIZE/2 + 1];

extern double audio_power_bufferR[BUFFER_SIZE/2 + 1];
extern double audio_power_bufferL[BUFFER_SIZE/2 + 1];

extern double audio_angle_bufferR[BUFFER_SIZE/2 + 1];
extern double audio_angle_bufferL[BUFFER_SIZE/2 + 1];

#endif /* end of include guard: __HEADER_BUFFER_DATA__ */
