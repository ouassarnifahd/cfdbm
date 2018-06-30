#include "common.h"
#include "buffer_data.h"

float audio_R[RAW_BUFFER_SIZE] = {0};
float audio_L[RAW_BUFFER_SIZE] = {0};

float fft_re_R[CHANNEL_BUFFER_SIZE] = {0};
float fft_im_R[CHANNEL_BUFFER_SIZE] = {0};

float fft_re_L[CHANNEL_BUFFER_SIZE] = {0};
float fft_im_L[CHANNEL_BUFFER_SIZE] = {0};

float data_ILD[CHANNEL_BUFFER_SIZE] = {0};
float data_IPD[CHANNEL_BUFFER_SIZE] = {0};
