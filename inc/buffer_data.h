#ifndef __HEADER_BUFFER_DATA__
#define __HEADER_BUFFER_DATA__

// time spent in capture = n / rate = 32 ms
#define RAW_BUFFER_SIZE (256)
#define CHANNEL_BUFFER_SIZE (RAW_BUFFER_SIZE/2)

// #define

#define SINT16_MAX (((1ull<<15)-1))

extern double audio_bufferR[RAW_BUFFER_SIZE];
extern double audio_bufferL[RAW_BUFFER_SIZE];

extern double audio_re_bufferR[CHANNEL_BUFFER_SIZE];
extern double audio_im_bufferR[CHANNEL_BUFFER_SIZE];

extern double audio_re_bufferL[CHANNEL_BUFFER_SIZE];
extern double audio_im_bufferL[CHANNEL_BUFFER_SIZE];

extern double audio_power_bufferR[CHANNEL_BUFFER_SIZE];
extern double audio_power_bufferL[CHANNEL_BUFFER_SIZE];

extern double audio_angle_bufferR[CHANNEL_BUFFER_SIZE];
extern double audio_angle_bufferL[CHANNEL_BUFFER_SIZE];

#endif /* end of include guard: __HEADER_BUFFER_DATA__ */
