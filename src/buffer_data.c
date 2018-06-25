#include "common.h"
#include "buffer_data.h"

short raw_audio_capture_buffer[2 * BUFFER_SIZE];

float audio_float_bufferR[BUFFER_SIZE];
float audio_float_bufferL[BUFFER_SIZE];

// fftw_complex audio_fft_bufferR[BUFFER_SIZE];
// fftw_complex audio_fft_bufferL[BUFFER_SIZE];
