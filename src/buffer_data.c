#include "common.h"
#include "buffer_data.h"

typedef struct {float re, im;} fcomplex_t;

float audio_bufferR[BUFFER_SIZE] = {0};
float audio_bufferL[BUFFER_SIZE] = {0};

fcomplex_t audio_fft_bufferR[BUFFER_SIZE/2 + 1] = {0};
fcomplex_t audio_fft_bufferL[BUFFER_SIZE/2 + 1] = {0};

float audio_power_bufferR[BUFFER_SIZE/2 + 1] = {0};
float audio_power_bufferL[BUFFER_SIZE/2 + 1] = {0};

float audio_angle_bufferR[BUFFER_SIZE/2 + 1] = {0};
float audio_angle_bufferL[BUFFER_SIZE/2 + 1] = {0};
