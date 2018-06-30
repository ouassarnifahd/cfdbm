#include "common.h"
#include "buffer_data.h"

double audio_R[RAW_BUFFER_SIZE] = {0};
double audio_L[RAW_BUFFER_SIZE] = {0};

double fft_re_R[CHANNEL_BUFFER_SIZE] = {0};
double fft_im_R[CHANNEL_BUFFER_SIZE] = {0};

double fft_re_L[CHANNEL_BUFFER_SIZE] = {0};
double fft_im_L[CHANNEL_BUFFER_SIZE] = {0};

double data_ILD[CHANNEL_BUFFER_SIZE] = {0};
double data_IPD[CHANNEL_BUFFER_SIZE] = {0};
