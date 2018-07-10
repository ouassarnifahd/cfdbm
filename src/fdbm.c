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

// plot (gnuplot)
INVISIBLE void plot(const char* title, const float* data, size_t len) {
    #ifndef __arm__
    FILE *gnuplot = popen("gnuplot -p", "w");
    // here config
    fprintf(gnuplot, "set title '%s'\n", title);
    fprintf(gnuplot, "set grid\n");
    fprintf(gnuplot, "set xlabel 'Samples'\n");
    fprintf(gnuplot, "set ylabel 'Amplitude'\n");
    // here data...
    fprintf(gnuplot, "plot '-'\n");
    for (int i = 0; i < len; i++)
    fprintf(gnuplot, "%f\n", data[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
    #endif
}

// loop enrolling is necessary... (neon?)
INVISIBLE void get_buffer_LR(const int16_t* buffer, size_t size, float* L, float* R) {
    for (register int i = 0; i < size/2; ++i) {
        L[i] = (double)buffer[2u * i]/(double)1000.0;
        R[i] = (double)buffer[2u * i + 1u]/(double)1000.0;//SINT16_MAX;
        // log_printf("(L=%hi, R=%hi) -> (L=%f, R=%f)\n", buffer[2u * i], buffer[2u * i + 1u], L[i], R[i]);
    }
}
// loop enrolling is necessary... (neon?)
INVISIBLE void set_buffer_LR(const float* L, const float* R, int16_t* buffer, size_t size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2u * i] = limit(-SINT16_MAX, (int16_t)(L[i] * 1000.0), SINT16_MAX);
        buffer[2u * i + 1u] = limit(-SINT16_MAX, (int16_t)(R[i] * 1000.0), SINT16_MAX);
        // log_printf("(L=%f, R=%f) -> (L=%hi, R=%hi)\n", L[i], R[i], buffer[2u * i], buffer[2u * i + 1u]);
    }
}

INVISIBLE fdbm_context_t prepare_context(char* buffer) {
    fdbm_context_t ctx;
    ctx.raw_buffer = buffer;
    ctx.raw_size = RAW_FDBM_BUFFER_SIZE;
    ctx.total_samples = RAW_TO_SAMPLES(RAW_FDBM_BUFFER_SIZE);
    ctx.channel_samples = CHANNEL_SAMPLES_COUNT;
    ctx.ildipd_samples = ILDIPD_LEN;
    // split
    get_buffer_LR((const int16_t*)ctx.raw_buffer, ctx.total_samples, ctx.L, ctx.R);
    // plot("Left data before", ctx.L, ctx.channel_samples);
    // plot("Right data before", ctx.R, ctx.channel_samples);

    // 2D fft
    dft2_IPDILD(ctx.L, ctx.R, &ctx.fft_L, &ctx.fft_R, ctx.data_ILD, ctx.data_IPD, ctx.channel_samples);
    // PLOT
    // plot("IPD data", ctx.data_IPD, ctx.ildipd_samples);
    // plot("ILD data", ctx.data_ILD, ctx.ildipd_samples);

    ctx.mu_IPD = ctx.mu;
    ctx.mu_ILD = ctx.mu + ctx.ildipd_samples;
    return ctx;
}

// loop enrolling is necessary... (neon?)
INVISIBLE void compare_ILDIPD(fdbm_context_t* ctx, int doa) {
    int i_theta = (doa + ILDIPD_DEG_MAX)/ILDIPD_DEG_STEP;
    float* local_IPDtarget = (float*)IPDtarget[i_theta];
    float* local_ILDtarget = (float*)ILDtarget[i_theta];
    for (register int i = 0; i < ctx->ildipd_samples; ++i) {
        ctx->mu_IPD[i] = limit(0.0, abs(ctx->data_IPD[i]-local_IPDtarget[i]) / IPDmaxmin[i], 1.0);
        ctx->mu_ILD[i] = limit(0.0, abs(ctx->data_ILD[i]-local_ILDtarget[i]) / ILDmaxmin[i], 1.0);
    }
    // PLOT
    // plot("mu plot", ctx->mu, ctx->channel_samples);
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
    // plot("Left data after", ctx->L, ctx->channel_samples);
    // plot("Right data after", ctx->R, ctx->channel_samples);
}

void applyFDBM_simple1(char* buffer, size_t size, int doa) {
    debug("Running FDBM... receiving %lu samples", size);
    if (doa == DOA_NOT_INITIALISED) {
        warning("NO CHANGES 'DOA_NOT_INITIALISED'!");
        return ;
    } else {
        // secure doa
        int local_doa = limit(DOA_LEFT, doa, DOA_RIGHT);
        debug("Running FDBM... DOA = %d !", local_doa);
        // prepare context
        fdbm_context_t fdbm = prepare_context(buffer);
        debug("Running FDBM... comparing ILDIPD!");
        // compare with DataBase:
        compare_ILDIPD(&fdbm, local_doa);
        debug("Running FDBM... Applying gain!");
        // apply Gain
        apply_mu(&fdbm);
        // generate output
        debug("Running FDBM... Finishing!");
        prepare_signal(&fdbm);
    }
    // error("TEST FDBM: STOP!");
}

void applyFDBM_simple2(char* buffer, size_t size, int doa1, int doa2);

void applyFDBM(char* buffer, size_t size, const int const * doa, int sd);
