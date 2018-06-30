#include "common.h"
#include "buffer_data.h"

double audio_bufferR[BUFFER_SIZE] = {0};
double audio_bufferL[BUFFER_SIZE] = {0};

double audio_re_bufferR[BUFFER_SIZE/2 + 1] = {0};
double audio_im_bufferR[BUFFER_SIZE/2 + 1] = {0};

double audio_re_bufferL[BUFFER_SIZE/2 + 1] = {0};
double audio_im_bufferL[BUFFER_SIZE/2 + 1] = {0};

double audio_power_bufferR[BUFFER_SIZE/2 + 1] = {0};
double audio_power_bufferL[BUFFER_SIZE/2 + 1] = {0};

double audio_angle_bufferR[BUFFER_SIZE/2 + 1] = {0};
double audio_angle_bufferL[BUFFER_SIZE/2 + 1] = {0};
