#include "common.h"
#include "buffer_data.h"

float audio_bufferR[BUFFER_SIZE] = {0};
float audio_bufferL[BUFFER_SIZE] = {0};

float audio_power_bufferR[BUFFER_SIZE/2 + 1] = {0};
float audio_power_bufferL[BUFFER_SIZE/2 + 1] = {0};

float audio_angle_bufferR[BUFFER_SIZE/2 + 1] = {0};
float audio_angle_bufferL[BUFFER_SIZE/2 + 1] = {0};
