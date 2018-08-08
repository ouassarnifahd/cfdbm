#include "common.h"
#include "fdbm.h"
#include "dft.h"
#include "ipdild_data.h"

// debug flags
#define FDBM        1   // OK
#define BUF_TO_LR   1   // OK
#define FFT_IFFT    1   // OK (Almost optimized)
#define IPD_ILD     1   // NOT OK (Still Testing)
#define APPLY_GAIN  1   // NOT OK

// algorithm coherance?
#define SAVE_STATS  1

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt
struct fdbm_context {
    // audio driver memory pointer
    int16_t* io_samples;
    // algorithm samples
    int16_t samples[FDBM_SAMPLES_COUNT];
    size_t total_samples;
    size_t channel_samples;
    size_t ipdild_samples;
    size_t ipd_samples;
    size_t ild_samples;
    // signal data
    float L[FDBM_CHANNEL_SAMPLES_COUNT];
    float R[FDBM_CHANNEL_SAMPLES_COUNT];
    // fft data
    fcomplex_t fft_L;
    fcomplex_t fft_R;
    // IPDILD data
    float  data[IPDILD_LEN];
    float* data_IPD;
    float* data_ILD;
    // mu data
    float  mu[IPDILD_LEN];
    float* mu_IPD;
    float* mu_ILD;
    // Gain
    float Gain[IPDILD_LEN];
};
typedef struct fdbm_context fdbm_context_t;

// plot (gnuplot)
INVISIBLE void plot(const char* title, const float* data, size_t len) {
    #ifndef __arm__
    FILE *gnuplot = popen("gnuplot -p", "w");
    // here config
    fprintf(gnuplot, "with lines \n");
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

#if (SAVE_STATS == 1)
  int saved_chunks_count = 0;
  m_init(mutex_stats);
  #define FILE_init(name) func(FILE* F = fopen(name, "w"); fclose(F))
  #define FILE_just_append(name, MSG, ...) func(FILE* F = fopen(name, "a"); fprintf(F, MSG, ##__VA_ARGS__); fclose(F))
  #define FILE_secured_append(name, MSG, ...) secured_stuff(mutex_stats, FILE* F = fopen(name, "a"); fprintf(F, MSG, ##__VA_ARGS__); fclose(F))
#endif

INVISIBLE void get_buffer_LR(int16_t* buffer, size_t size, float* L, float* R) {
    for (register int i = 0; i < size/2; ++i) {
        L[i]  = buffer[2u * i];
        // L[i] /= SINT16_MAX;
        R[i]  = buffer[2u * i + 1u];
        // R[i] /= SINT16_MAX;
        #if (SAVE_STATS == 1)
          FILE_secured_append("LR_data.txt", "%f,%f\n", L[i], R[i]);
        #endif
    }
}

INVISIBLE void set_buffer_LR(float* L, float* R, int16_t* buffer, size_t size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2u * i] = L[i];
        // buffer[2u * i] =  L[i] * SINT16_MAX;
        buffer[2u * i + 1u] = R[i];
        // buffer[2u * i + 1u] = R[i] * SINT16_MAX;
    }
}

INVISIBLE fdbm_context_t prepare_context(const char* buffer) {
    fdbm_context_t ctx;
    ctx.total_samples = FDBM_SAMPLES_COUNT;
    ctx.channel_samples = FDBM_CHANNEL_SAMPLES_COUNT;
    ctx.ipdild_samples = IPDILD_LEN;
    ctx.ipd_samples = IPD_LEN;
    ctx.ild_samples = ILD_LEN;

    // memcpy
    ctx.io_samples = (int16_t*)buffer;
    debug("in buffer @%X", ctx.io_samples);
    for (size_t i = 0; i < ctx.total_samples; i++) {
        ctx.samples[i] = ctx.io_samples[i];
    }

    // split
    #if (BUF_TO_LR == 1)
    get_buffer_LR(ctx.samples, ctx.total_samples, ctx.L, ctx.R);
    #endif

    // initializing pointers
    ctx.data_IPD = ctx.data;
    ctx.data_ILD = ctx.data + ctx.ipd_samples;

    // 2D fft
    #if (FFT_IFFT == 1)
    dft2_IPDILD(ctx.L, ctx.R, &ctx.fft_L, &ctx.fft_R, ctx.data, ctx.ipd_samples, ctx.channel_samples);
    // memset(ctx.L, 0, sizeof(float)*FDBM_CHANNEL_SAMPLES_COUNT);
    // memset(ctx.R, 0, sizeof(float)*FDBM_CHANNEL_SAMPLES_COUNT);
    #endif

    #if (SAVE_STATS == 1)
      secured_stuff(mutex_stats,
      for (size_t i = 0; i < FDBM_CHANNEL_SAMPLES_COUNT; i++)
          FILE_just_append("FFT_data.txt", "%f,%f,%f,%f\n", ctx.fft_L.re[i], ctx.fft_L.im[i], ctx.fft_R.re[i], ctx.fft_R.im[i]));
    #endif

    // initializing pointers
    ctx.mu_IPD = ctx.mu;
    ctx.mu_ILD = ctx.mu + ctx.ipd_samples;
    return ctx;
}

// loop enrolling is necessary... (neon?)
INVISIBLE void compare_ILDIPD(fdbm_context_t* ctx, int doa) {
    #if (IPD_ILD == 1)
    int i_theta = (doa + IPDILD_DEG_MAX)/IPDILD_DEG_STEP;
    // debug("i_theta = %d", i_theta);
    float* local_IPDtarget = (float*)IPDtarget[i_theta];
    float* local_ILDtarget = (float*)ILDtarget[i_theta];

    for (register int i = 0; i < ctx->ipd_samples; ++i) {
        ctx->mu_IPD[i] = fabs(ctx->data_IPD[i]-local_IPDtarget[i]) / IPDmaxmin[i];
        // debug("IPD at %d: abs(data %2.6f - target %2.6f)/maxmin %2.6f = mu %2.6f", i, ctx->data_IPD[i], local_IPDtarget[i], IPDmaxmin[i], ctx->mu_IPD[i]);
    }
    for (register int i = 0; i < ctx->ild_samples; ++i) {
        // debug("ILD at %d: abs(data %2.6f - target %2.6f)/maxmin %2.6f = mu %2.6f", i, ctx->data_ILD[i], local_ILDtarget[i], ILDmaxmin[i], ctx->mu_ILD[i]);
        ctx->mu_ILD[i] = fabs(ctx->data_ILD[i]-local_ILDtarget[i]) / ILDmaxmin[i];
    }
    #endif
}

INVISIBLE void apply_Gain(fdbm_context_t* ctx) {
    #if (APPLY_GAIN == 1)
    // debug("Entering IPDILD (fft_L.im @%X)", ctx->fft_L.im); // why????

    for (register int i = 0; i < ctx->ipdild_samples; ++i) {
        // here the magic! (FFT = FFT * G)
        ctx->Gain[i] = pow16(1 - limit(0.0, ctx->mu[i], 1.0));
        #if(SAVE_STATS == 1)
        // FILE_secured_append("IDPILD_data.txt", "%f\n", ctx->data[i]);
        // FILE_secured_append("MU_data.txt", "%f\n", ctx->mu[i]);
        // FILE_secured_append("GAIN_data.txt", "%f\n", ctx->Gain[i]);
        #endif

        // debug("mu[%d&%d] = %f, Gain[%d&%d] = %f", i, ctx->channel_samples-i-1, ctx->mu[i],
        //                                         i, ctx->channel_samples-i-1, ctx->Gain[i]);

        // first half
        // float res = ctx->Gain[i] * ctx->fft_L.re[i];
        // float diff = res - ctx->fft_L.re[i];
        // debug("in = %f, g = %f, res = %f, diff = %f", ctx->fft_L.re[i], ctx->Gain[i], res, diff);
        // debug("(fft_L_re = %f * Gain = %f) = %f", ctx->fft_L.re, ctx->Gain[i], ctx->Gain[i] * ctx->fft_L.re[i]);

        ctx->fft_L.re[i] *= ctx->Gain[i];
        // debug("out = %f", ctx->fft_L.re[i]);
        ctx->fft_L.im[i] *= ctx->Gain[i];
        ctx->fft_R.re[i] *= ctx->Gain[i];
        ctx->fft_R.im[i] *= ctx->Gain[i];
        // second half mirrored!
        ctx->fft_L.re[ctx->channel_samples-i] *= ctx->Gain[i];
        ctx->fft_L.im[ctx->channel_samples-i] *= ctx->Gain[i];
        ctx->fft_R.re[ctx->channel_samples-i] *= ctx->Gain[i];
        ctx->fft_R.im[ctx->channel_samples-i] *= ctx->Gain[i];
    }
    // debug("fft_L_re = %f * Gain = %f = %f", fft_L_re, ctx->Gain[i], ctx->fft_L.re[i]);
    // debug("leaving  IPDILD (fft_L.im @%X)", ctx->fft_L.im); // why????
    #endif
}

INVISIBLE void prepare_signal(fdbm_context_t* ctx) {
    // 2D ifft
    #if (FFT_IFFT == 1)
    idft2_SINE_WIN(&ctx->fft_L, &ctx->fft_R, ctx->L, ctx->R, ctx->channel_samples);
    #endif

    // reassemble
    #if (BUF_TO_LR == 1)
    set_buffer_LR(ctx->L, ctx->R, ctx->samples, ctx->total_samples);
    #endif

    // memcpy
    for (size_t i = 0; i < ctx->total_samples; i++) {
        // debug("buffer diff at index %d = %d", i, (int)(ctx->io_samples[i] - ctx->samples[i]));
        ctx->io_samples[i] = ctx->samples[i];
    }
    debug("out buffer @%X", ctx->io_samples);
}

void applyFDBM_simple1(char* buffer, size_t size, int doa) {
    #if(FDBM == 1)
    #if(SAVE_STATS == 1)
    if (saved_chunks_count == 0) {
        saved_chunks_count = 1;
        FILE_init("LR_data.txt");
        FILE_init("FFT_data.txt");
        // FILE_init("IDPILD_data.txt");
        // FILE_init("MU_data.txt");
        // FILE_init("GAIN_data.txt");
    }
    #endif
    debug("Running FDBM... receiving %lu samples", size);
    if (doa == DOA_NOT_INITIALISED) {
        warning("NO CHANGES 'DOA_NOT_INITIALISED'!");
        return ;
    } else {
        // secure doa
        int local_doa = limit(IPDILD_DEG_MIN, doa, IPDILD_DEG_MAX);
        // prepare context
        debug("Running FDBM... DOA = %d !", local_doa);
        fdbm_context_t fdbm = prepare_context(buffer);
        // compare with DataBase:
        debug("Running FDBM... comparing ILDIPD!");
        compare_ILDIPD(&fdbm, local_doa);
        // apply Gain
        debug("Running FDBM... Applying gain!");
        apply_Gain(&fdbm);
        // generate output
        debug("Running FDBM... Finishing!");
        prepare_signal(&fdbm);
    }
    #endif
}

void applyFDBM_simple2(char* buffer, size_t size, int doa1, int doa2) {

}

void applyFDBM(char* buffer, size_t size, const int const * doa, int sd);
