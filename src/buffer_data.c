#include "common.h"
#include "buffer_data.h"

double audio_bufferR[RAW_BUFFER_SIZE] = {0};
double audio_bufferL[RAW_BUFFER_SIZE] = {0};

double audio_re_bufferR[CHANNEL_BUFFER_SIZE] = {0};
double audio_im_bufferR[CHANNEL_BUFFER_SIZE] = {0};

double audio_re_bufferL[CHANNEL_BUFFER_SIZE] = {0};
double audio_im_bufferL[CHANNEL_BUFFER_SIZE] = {0};

double audio_power_bufferR[CHANNEL_BUFFER_SIZE] = {0};
double audio_power_bufferL[CHANNEL_BUFFER_SIZE] = {0};

double audio_angle_bufferR[CHANNEL_BUFFER_SIZE] = {0};
double audio_angle_bufferL[CHANNEL_BUFFER_SIZE] = {0};
