#include "common.h"
#include "fdbm.h"
#include "dft.h"
#include "ipdild_data.h"

// demo flags
#define DEMO_5SEC   0

// debug flags
#define FDBM        1   // OK
#define BUF_TO_LR   1   // OK
#define FFT_IFFT    1
#define ILD_IPD     0
#define APPLY_MU    0

// Features: swp half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpv4 idiva idivt
struct fdbm_context {
    // raw data
    int16_t* io_samples;
    int16_t samples[SAMPLES_COUNT];
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

// loop enrolling is necessary... (neon?)
INVISIBLE void get_buffer_LR(int16_t* buffer, size_t size, float* L, float* R) {
    for (register int i = 0; i < size/2; ++i) {
        L[i] = buffer[2u * i];
        R[i] = buffer[2u * i + 1u];
    }
}
// loop enrolling is necessary... (neon?)
INVISIBLE void set_buffer_LR(float* L, float* R, int16_t* buffer, size_t size) {
    for (register int i = 0; i < size/2; ++i) {
        buffer[2u * i] = L[i]; //limit(-SINT16_MAX, (L[i]), SINT16_MAX);
        buffer[2u * i + 1u] = R[i]; //limit(-SINT16_MAX, (R[i]), SINT16_MAX);
    }
}

INVISIBLE fdbm_context_t prepare_context(const char* buffer) {
    fdbm_context_t ctx;
    ctx.total_samples = SAMPLES_COUNT;
    ctx.channel_samples = CHANNEL_SAMPLES_COUNT;
    ctx.ildipd_samples = ILDIPD_LEN;

    // memcpy
    ctx.io_samples = buffer;
    for (size_t i = 0; i < ctx.total_samples; i++) {
        ctx.samples[i] = ctx.io_samples[i];
    }

    // split
    #if (BUF_TO_LR == 1)
    get_buffer_LR(ctx.samples, ctx.total_samples, ctx.L, ctx.R);
    #endif
    // plot("Left data before", ctx.L, ctx.channel_samples);
    // plot("Right data before", ctx.R, ctx.channel_samples);

    // 2D fft
    #if (FFT_IFFT == 1)
    dft2_IPDILD(ctx.L, ctx.R, &ctx.fft_L, &ctx.fft_R, ctx.data_ILD, ctx.data_IPD, ctx.channel_samples);
    #endif
    // plot("IPD data", ctx.data_IPD, ctx.ildipd_samples);
    // plot("ILD data", ctx.data_ILD, ctx.ildipd_samples);

    ctx.mu_IPD = ctx.mu;
    ctx.mu_ILD = ctx.mu + ctx.ildipd_samples;
    return ctx;
}

// loop enrolling is necessary... (neon?)
INVISIBLE void compare_ILDIPD(fdbm_context_t* ctx, int doa) {
    #if (ILD_IPD == 1)
    int i_theta = (doa + ILDIPD_DEG_MAX)/ILDIPD_DEG_STEP;
    float* local_IPDtarget = (float*)IPDtarget[i_theta];
    float* local_ILDtarget = (float*)ILDtarget[i_theta];
    for (register int i = 0; i < ctx->ildipd_samples; ++i) {
        ctx->mu_IPD[i] = limit(0.0, abs(ctx->data_IPD[i]-local_IPDtarget[i]) / IPDmaxmin[i], 1.0);
        ctx->mu_ILD[i] = limit(0.0, abs(ctx->data_ILD[i]-local_ILDtarget[i]) / ILDmaxmin[i], 1.0);
    }
    // PLOT
    // plot("mu plot", ctx->mu, ctx->channel_samples);
    #endif
}

INVISIBLE void apply_mu(fdbm_context_t* ctx) {
    #if (APPLY_MU == 1)
    for (register int i = 0; i < ctx->channel_samples; ++i) {
        // here the magic!
        ctx->Gain[i] = pow16(1 - ctx->mu[i]);
        ctx->fft_L.re[i] *= ctx->Gain[i];
        ctx->fft_L.im[i] *= ctx->Gain[i];
        ctx->fft_R.re[i] *= ctx->Gain[i];
        ctx->fft_R.im[i] *= ctx->Gain[i];
    }
    #endif
}

INVISIBLE void prepare_signal(fdbm_context_t* ctx, char* buffer) {
    // 2D ifft
    #if (FFT_IFFT == 1)
    idft2_SINE_WIN(&ctx->fft_L, &ctx->fft_R, ctx->L, ctx->R, ctx->channel_samples);
    #endif

    // reassemble
    #if (BUF_TO_LR == 1)
    set_buffer_LR(ctx->L, ctx->R, ctx->samples, ctx->total_samples);
    #endif
    // plot("Left data after", ctx->L, ctx->channel_samples);
    // plot("Right data after", ctx->R, ctx->channel_samples);

    // memcpy
    for (size_t i = 0; i < ctx->total_samples; i++) {
        ctx->io_samples[i] = ctx->samples[i];
    }
}

// plot_sync
int global_counterMemcpy = 0;
int global_counterTitle = 0;
#define PLOT_CHUNKS 60
m_init(mutex_plot);

void applyFDBM_simple1(char* buffer, size_t size, int doa) {
    #if(FDBM == 1)
    debug("Running FDBM... receiving %lu samples", size);

    int local_counterMemcpy;
    int local_counterTitle;

    secured_stuff(mutex_plot, local_counterMemcpy =
        (global_counterMemcpy < PLOT_CHUNKS) ? global_counterMemcpy + 1: 0);

    // static float fft_data_1s[CHANNEL_SAMPLES_COUNT * PLOT_CHUNKS];
    static float input_data[CHANNEL_SAMPLES_COUNT * PLOT_CHUNKS];
    static float output_data[CHANNEL_SAMPLES_COUNT * PLOT_CHUNKS];

    if (doa == DOA_NOT_INITIALISED) {
        warning("NO CHANGES 'DOA_NOT_INITIALISED'!");
        return ;
    } else {
        // secure doa
        int local_doa = limit(DOA_LEFT, doa, DOA_RIGHT);
        debug("Running FDBM... DOA = %d !", local_doa);
        // prepare context
        fdbm_context_t fdbm = prepare_context(buffer);

        if (local_counterMemcpy < PLOT_CHUNKS) {
            memcpy(input_data + local_counterMemcpy * CHANNEL_SAMPLES_COUNT, fdbm.L, CHANNEL_SAMPLES_COUNT * sizeof(float));
        }

        debug("Running FDBM... comparing ILDIPD!");
        // compare with DataBase:
        compare_ILDIPD(&fdbm, local_doa);
        debug("Running FDBM... Applying gain!");
        // apply Gain
        apply_mu(&fdbm);
        // generate output
        debug("Running FDBM... Finishing!");
        prepare_signal(&fdbm, buffer);

        // debug 1s plot
        memcpy(output_data + local_counterMemcpy * CHANNEL_SAMPLES_COUNT, fdbm.L, CHANNEL_SAMPLES_COUNT * sizeof(float));

        if (local_counterMemcpy > PLOT_CHUNKS) {
            secured_stuff(mutex_plot, local_counterTitle = global_counterTitle++);
            char inputTitle[20], outputTitle[20];
            sprintf(inputTitle, "INPUT #%d", local_counterTitle);
            sprintf(outputTitle, "OUTPUT #%d", local_counterTitle);
            plot(inputTitle, input_data, CHANNEL_SAMPLES_COUNT * PLOT_CHUNKS);
            plot(outputTitle, output_data, CHANNEL_SAMPLES_COUNT * PLOT_CHUNKS);
        }
        if(local_counterTitle > 2 ) error("TEST FDBM: STOP!");
    }
    #endif
}

void applyFDBM_simple2(char* buffer, size_t size, int doa1, int doa2) {

}

void applyFDBM(char* buffer, size_t size, const int const * doa, int sd);
