#ifndef __HEADER_BUFFER_DATA__
#define __HEADER_BUFFER_DATA__

#define BUFFER_SIZE 256
#define SINT16_MAX (((1ull<<16)-1)/2)

extern short raw_audio_capture_buffer[2 * BUFFER_SIZE];

extern float audio_float_bufferR[BUFFER_SIZE];
extern float audio_float_bufferL[BUFFER_SIZE];

// extern fftw_complex audio_fft_bufferR[BUFFER_SIZE];
// extern fftw_complex audio_fft_bufferL[BUFFER_SIZE];

#endif /* end of include guard: __HEADER_BUFFER_DATA__ */
