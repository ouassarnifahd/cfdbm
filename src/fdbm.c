#include "common.h"
#include "fdbm.h"
#include "dft.h"
#include "ipdild_data.h"

// #define FDBM_STEREO_OUTPUT

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt
struct fdbm_context {
    // raw data
    char *raw_buffer;
    size_t raw_size;
    size_t total_samples;
    size_t channel_samples;
    size_t ildipd_samples;
    // signal data
    float L[CHANNEL_SAMPLES_COUNT];
    float R[CHANNEL_SAMPLES_COUNT];
    // fft data
    fcomplex_t fft_L;
    fcomplex_t fft_R;
    // IPDILD data
    float data_IPD[ILDIPD_LEN];
    float data_ILD[ILDIPD_LEN];
    // mu data
    float mu[CHANNEL_SAMPLES_COUNT];
    float* mu_IPD;
    float* mu_ILD;
    // Gain
    float Gain[CHANNEL_SAMPLES_COUNT];
};
typedef struct fdbm_context fdbm_context_t;

// expand __(-\*)__
#define pow2(x) (x * x)
// pow2(x) = (x * x)
#define pow4(x) pow2(pow2(x))
// pow4(x) = pow2((x * x)) = ((x * x) * (x * x))
#define pow8(x) pow4(pow2(x))
// pow8(x) = pow4((x * x)) = (((x * x) * (x * x)) * ((x * x) * (x * x)))
#define pow16(x) pow4(pow4(x))
// pow16(x) = pow4(((x * x) * (x * x))) = ((((x * x) * (x * x)) * ((x * x) * (x * x))) * (((x * x) * (x * x)) * ((x * x) * (x * x))))

#define max(x,y) ((x<y)?y:x)
#define abs(x) max(-x, x)

#define limit(min, x, max) ((max<(x))? max : (((x)<min) ? min : (x)))

// loop enrolling is necessary... (neon?)
INVISIBLE void get_buffer_LR(const int16_t* buffer, size_t size, float* L, float* R) {
    for (register int i = 0; i < size/2; ++i) {
        L[i] = (float)buffer[2u * i]/(float)SINT16_MAX;
        R[i] = (float)buffer[2u * i + 1u]/(float)SINT16_MAX;
        // log_printf("(L=%hi, R=%hi) -> (L=%f, R=%f)\n", buffer[2u * i], buffer[2u * i + 1u], L[i], R[i]);
    }
}
// loop enrolling is necessary... (neon?)
INVISIBLE void set_buffer_LR(const float* L, const float* R, int16_t* buffer, size_t size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2u * i] = limit(-SINT16_MAX, (int16_t)(L[i] * SINT16_MAX), SINT16_MAX);
        buffer[2u * i + 1u] = limit(-SINT16_MAX, (int16_t)(R[i] * SINT16_MAX), SINT16_MAX);
        // log_printf("(L=%f, R=%f) -> (L=%hi, R=%hi)\n", L[i], R[i], buffer[2u * i], buffer[2u * i + 1u]);
    }
}

INVISIBLE fdbm_context_t prepare_context(char* buffer) {
    fdbm_context_t ctx;
    ctx.raw_buffer = buffer;
    ctx.raw_size = RAW_BUFFER_SIZE;
    ctx.total_samples = SAMPLES_COUNT;
    ctx.channel_samples = CHANNEL_SAMPLES_COUNT;
    ctx.ildipd_samples = ILDIPD_LEN;
    // split
    get_buffer_LR((const int16_t*)ctx.raw_buffer, ctx.total_samples, ctx.L, ctx.R);
    // 2D fft
    dft2_IPDILD(ctx.L, ctx.R, &ctx.fft_L, &ctx.fft_R,
                 ctx.data_ILD, ctx.data_IPD, ctx.channel_samples);

    ctx.mu_IPD = ctx.mu;
    ctx.mu_ILD = ctx.mu + ctx.ildipd_samples;
    return ctx;
}

// loop enrolling is necessary... (neon?)
INVISIBLE void compare_ILDIPD(fdbm_context_t* ctx, int doa) {
    int i_theta = (doa + ILDIPD_DEG_MAX)/ILDIPD_DEG_STEP;
    float* local_IPDtarget = IPDtarget[i_theta];
    float* local_ILDtarget = ILDtarget[i_theta];
    for (register int i = 0; i < ctx->ildipd_samples; ++i) {
        ctx->mu_IPD[i] = limit(-1.0, abs(ctx->data_IPD[i] - local_IPDtarget[i]) / IPDmaxmin[i], 1.0);
        ctx->mu_ILD[i] = limit(-1.0, abs(ctx->data_ILD[i] - local_ILDtarget[i]) / ILDmaxmin[i], 1.0);
    }
}

INVISIBLE void apply_mu(fdbm_context_t* ctx) {
    for (register int i = 0; i < ctx->channel_samples; ++i) {
        // here the magic!
        ctx->Gain[i] = pow16(1 - ctx->mu[i]);
        ctx->fft_L.re[i] *= ctx->Gain[i];
        ctx->fft_L.im[i] *= ctx->Gain[i];
        ctx->fft_R.re[i] *= ctx->Gain[i];
        ctx->fft_R.im[i] *= ctx->Gain[i];
    }
}

INVISIBLE void prepare_signal(fdbm_context_t* ctx) {
    // 2D ifft
    idft2_SINE_WIN(&ctx->fft_L, &ctx->fft_R, ctx->L, ctx->R, ctx->channel_samples);
    // reassemble
    set_buffer_LR(ctx->L, ctx->R, (int16_t*)ctx->raw_buffer, ctx->total_samples);
}

void applyFDBM_simple1(char* buffer, size_t size, int doa) {
    debug("Entering FDBM... %lu samples", size);
    if (doa == DOA_NOT_INITIALISED) {
        warning("NO CHANGES 'DOA_NOT_INITIALISED'!");
        return ;
    } else {
        // secure doa
        int local_doa = limit(DOA_LEFT, doa, DOA_RIGHT);
        // prepare context
        fdbm_context_t fdbm = prepare_context(buffer);
        // compare with DataBase:
        compare_ILDIPD(&fdbm, local_doa);
        // apply Gain
        apply_mu(&fdbm);
        // generate output
        prepare_signal(&fdbm);
    }
}

void applyFDBM_simple2(char* buffer, size_t size, int doa1, int doa2);

void applyFDBM(char* buffer, size_t size, const int const * doa, int sd);
