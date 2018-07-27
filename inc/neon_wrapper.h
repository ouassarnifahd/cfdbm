#ifndef __HEADER_NEON_WRAPPER__
#define __HEADER_NEON_WRAPPER__

#include <arm_neon.h>

INVISIBLE void neon_f32q_add(const float* a, const float* b, float* c) {
    float32x4_t _a = vld1q_f32(a), _b = vld1q_f32(b);
    float32x4_t _c = vaddq_f32(_a, _b);
    vst1q_f32(c, _c);
}
#endif /* end of include guard: __HEADER_NEON_WRAPPER__ */
