#include "common.h"
#include "fdbm.h"
#include "dft.h"
#include "ipdild_data.h"

// debug flags
#define FDBM        1   // OK
#define BUF_TO_LR   1   // OK
#define FFT_IFFT    1   // OK (Almost optimized)
#define IPD_ILD     0   // NOT OK (Still Testing)
#define APPLY_GAIN  0   // OK

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt
struct fdbm_context {
    // audio driver memory pointer
    int16_t* io_samples;
    // algorithm samples
    int16_t samples[SAMPLES_COUNT];
    size_t total_samples;
    size_t channel_samples;
    size_t ipdild_samples;
    size_t ipdild_icut;
    // signal data
    float L[CHANNEL_SAMPLES_COUNT];
    float R[CHANNEL_SAMPLES_COUNT];
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

INVISIBLE void get_buffer_LR(int16_t* buffer, size_t size, float* L, float* R) {
    for (register int i = 0; i < size/2; ++i) {
        L[i] = buffer[2u * i];
        R[i] = buffer[2u * i + 1u];
    }
}

INVISIBLE void set_buffer_LR(float* L, float* R, int16_t* buffer, size_t size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2u * i] = L[i];
        buffer[2u * i + 1u] = R[i];
    }
}

INVISIBLE fdbm_context_t prepare_context(const char* buffer) {
    fdbm_context_t ctx;
    ctx.total_samples = SAMPLES_COUNT;
    ctx.channel_samples = CHANNEL_SAMPLES_COUNT;
    ctx.ipdild_samples = IPDILD_LEN;
    ctx.ipdild_icut = expand(IPDILD_FCUT*CHANNEL_SAMPLES_COUNT/RATE);

    // memcpy
    ctx.io_samples = (int16_t*)buffer;
    for (size_t i = 0; i < ctx.total_samples; i++) {
        ctx.samples[i] = ctx.io_samples[i];
    }

    // split
    #if (BUF_TO_LR == 1)
    get_buffer_LR(ctx.samples, ctx.total_samples, ctx.L, ctx.R);
    #endif

    // initializing pointers
    ctx.data_IPD = ctx.data;
    ctx.data_ILD = ctx.data + ctx.ipdild_icut;

    // debug(" FDBM data @%X, data_IPD @%X, data_ILD @%X", ctx.data, ctx.data_IPD, ctx.data_ILD);
    // 2D fft
    #if (FFT_IFFT == 1)
    dft2_IPDILD(ctx.L, ctx.R, &ctx.fft_L, &ctx.fft_R, ctx.data, ctx.ipdild_icut, ctx.channel_samples);
    #endif
    // debug("FDBM data @%X, data_IPD @%X, data_ILD @%X", ctx.data, ctx.data_IPD, ctx.data_ILD);

    ctx.mu_IPD = ctx.mu;
    ctx.mu_ILD = ctx.mu + ctx.ipdild_icut;
    return ctx;
}

// loop enrolling is necessary... (neon?)
INVISIBLE void compare_ILDIPD(fdbm_context_t* ctx, int doa) {
    #if (IPD_ILD == 1)
    int i_theta = (doa + IPDILD_DEG_MAX)/IPDILD_DEG_STEP;
    float* local_IPDtarget = (float*)IPDtarget[i_theta];
    float* local_ILDtarget = (float*)ILDtarget[i_theta];
    // for (size_t i = 0; i < ctx->ipdild_samples; i++) {
    //     debug("IPDILDtarget[%d] %f ,%f", i, local_IPDtarget[i], local_ILDtarget[i]);
    // }
    for (register int i = 0; i < ctx->ipdild_icut; ++i) {
        // debug("IPD at %d: data %2.6f, target %2.6f, maxmin %2.6f", i, ctx->data_IPD[i], local_IPDtarget[i], IPDmaxmin[i]);
        ctx->mu_IPD[i] = abs(ctx->data_IPD[i]-local_IPDtarget[i]) / IPDmaxmin[i];
    }
    for (register int i = 0; i < ctx->ipdild_samples - ctx->ipdild_icut; ++i) {
        ctx->mu_ILD[i] = abs(ctx->data_ILD[i]-local_ILDtarget[i]) / ILDmaxmin[i];
    }
    // PLOT
    // plot("mu plot", ctx->mu, ctx->channel_samples);
    // for (size_t i = 0; i < ctx->ipdild_samples/4; i+=4) {
    //     debug("%f, %f, %f, %f", ctx->mu[4u * i], ctx->mu[4u * i + 1], ctx->mu[4u * i + 2], ctx->mu[4u * i + 3]);
    // }
    #endif
}

INVISIBLE void apply_Gain(fdbm_context_t* ctx) {
    #if (APPLY_GAIN == 1)
    for (register int i = 0; i < ctx->ipdild_samples; ++i) {
        // here the magic! (FFT = FFT * G)
        ctx->Gain[i] = pow16(1 - limit(0.0, ctx->mu[i], 1.0));
        // debug("mu[%d] %f, Gain[%d] %f", i, ctx->mu[i], i, ctx->Gain[i]);
        ctx->fft_L.re[i] *= ctx->Gain[i];
        ctx->fft_L.im[i] *= ctx->Gain[i];
        ctx->fft_R.re[i] *= ctx->Gain[i];
        ctx->fft_R.im[i] *= ctx->Gain[i];
        // mirror!
        ctx->fft_L.re[ctx->channel_samples-i] *= ctx->Gain[i];
        ctx->fft_L.im[ctx->channel_samples-i] *= ctx->Gain[i];
        ctx->fft_R.re[ctx->channel_samples-i] *= ctx->Gain[i];
        ctx->fft_R.im[ctx->channel_samples-i] *= ctx->Gain[i];
    }
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
        ctx->io_samples[i] = ctx->samples[i];
    }
}

void applyFDBM_simple1(char* buffer, size_t size, int doa) {
    #if(FDBM == 1)
    debug("Running FDBM... receiving %lu samples", size);
    if (doa == DOA_NOT_INITIALISED) {
        warning("NO CHANGES 'DOA_NOT_INITIALISED'!");
        return ;
    } else {
        // secure doa
        int local_doa = limit(DOA_LEFT, doa, DOA_CENTER);
        debug("Running FDBM... DOA = %d !", local_doa);
        // prepare context
        fdbm_context_t fdbm = prepare_context(buffer);
        debug("Running FDBM... comparing ILDIPD!");
        // compare with DataBase:
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
