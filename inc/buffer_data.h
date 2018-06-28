#ifndef __HEADER_BUFFER_DATA__
#define __HEADER_BUFFER_DATA__

#define BUFFER_SIZE 512
#define SINT16_MAX (((1ull<<16)-1)/2)

extern float audio_bufferR[BUFFER_SIZE];
extern float audio_bufferL[BUFFER_SIZE];

extern float audio_re_bufferR[BUFFER_SIZE/2 + 1];
extern float audio_im_bufferR[BUFFER_SIZE/2 + 1];

extern float audio_re_bufferL[BUFFER_SIZE/2 + 1];
extern float audio_im_bufferL[BUFFER_SIZE/2 + 1];

extern float audio_power_bufferR[BUFFER_SIZE/2 + 1];
extern float audio_power_bufferL[BUFFER_SIZE/2 + 1];

extern float audio_angle_bufferR[BUFFER_SIZE/2 + 1];
extern float audio_angle_bufferL[BUFFER_SIZE/2 + 1];

#endif /* end of include guard: __HEADER_BUFFER_DATA__ */
