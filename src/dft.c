#include "common.h"
#include "dft.h"

fcomplex_t fft_R = {
    fft_re_R,
    fft_im_R,
};

fcomplex_t fft_L = {
    fft_re_L,
    fft_im_L,
};

// Twiddle factors ( roots of unity)
const float W[] = {
    1.00000, 0.99970, 0.99880, 0.99729, 0.99518, 0.99248, 0.98918, 0.98528,
    0.98079, 0.97570, 0.97003, 0.96378, 0.95694, 0.94953, 0.94154, 0.93299,
    0.92388, 0.91421, 0.90399, 0.89322, 0.88192, 0.87009, 0.85773, 0.84485,
    0.83147, 0.81758, 0.80321, 0.78835, 0.77301, 0.75721, 0.74095, 0.72425,
    0.70711, 0.68954, 0.67156, 0.65317, 0.63439, 0.61523, 0.59570, 0.57581,
    0.55557, 0.53500, 0.51410, 0.49290, 0.47139, 0.44961, 0.42755, 0.40524,
    0.38268, 0.35989, 0.33689, 0.31368, 0.29028, 0.26671, 0.24298, 0.21910,
    0.19509, 0.17096, 0.14673, 0.12241, 0.09801, 0.07356, 0.04906, 0.02454,
    -0.00000,-0.02454,-0.04907,-0.07357,-0.09802,-0.12241,-0.14673,-0.17097,
    -0.19509,-0.21911,-0.24298,-0.26672,-0.29029,-0.31369,-0.33689,-0.35990,
    -0.38269,-0.40525,-0.42756,-0.44962,-0.47140,-0.49290,-0.51411,-0.53500,
    -0.55557,-0.57581,-0.59570,-0.61524,-0.63440,-0.65318,-0.67156,-0.68954,
    -0.70711,-0.72425,-0.74095,-0.75721,-0.77301,-0.78835,-0.80321,-0.81759,
    -0.83147,-0.84486,-0.85773,-0.87009,-0.88192,-0.89323,-0.90399,-0.91421,
    -0.92388,-0.93300,-0.94155,-0.94953,-0.95694,-0.96378,-0.97003,-0.97570,
    -0.98079,-0.98528,-0.98918,-0.99248,-0.99519,-0.99729,-0.99880,-0.99970,
    -1.00000,-0.99970,-0.99880,-0.99729,-0.99518,-0.99248,-0.98918,-0.98528,
    -0.98078,-0.97570,-0.97003,-0.96377,-0.95694,-0.94953,-0.94154,-0.93299,
    -0.92388,-0.91421,-0.90399,-0.89322,-0.88192,-0.87008,-0.85772,-0.84485,
    -0.83146,-0.81758,-0.80320,-0.78834,-0.77300,-0.75720,-0.74095,-0.72424,
    -0.70710,-0.68953,-0.67155,-0.65317,-0.63439,-0.61522,-0.59569,-0.57580,
    -0.55556,-0.53499,-0.51409,-0.49289,-0.47139,-0.44960,-0.42755,-0.40523,
    -0.38267,-0.35989,-0.33688,-0.31367,-0.29027,-0.26670,-0.24297,-0.21909,
    -0.19508,-0.17095,-0.14672,-0.12240,-0.09801,-0.07355,-0.04906,-0.02453,
    0.00001, 0.02455, 0.04908, 0.07358, 0.09803, 0.12242, 0.14674, 0.17097,
    0.19510, 0.21911, 0.24299, 0.26672, 0.29030, 0.31369, 0.33690, 0.35991,
    0.38269, 0.40525, 0.42757, 0.44962, 0.47141, 0.49291, 0.51411, 0.53501,
    0.55558, 0.57582, 0.59571, 0.61524, 0.63440, 0.65318, 0.67157, 0.68955,
    0.70712, 0.72426, 0.74096, 0.75722, 0.77302, 0.78835, 0.80322, 0.81759,
    0.83148, 0.84486, 0.85774, 0.87009, 0.88193, 0.89323, 0.90400, 0.91422,
    0.92388, 0.93300, 0.94155, 0.94953, 0.95694, 0.96378, 0.97003, 0.97571,
    0.98079, 0.98528, 0.98918, 0.99248, 0.99519, 0.99729, 0.99880, 0.99970
};

float log2f_approx_coeff[4] = {1.23149591368684f, -4.11852516267426f, 6.02197014179219f, -3.13396450166353f};

static inline __attribute__((always_inline)) float FastArcTan(float x) {
    return 0.7853 * x - x * ((int)x - 1) * (0.2447 + 0.0663 * (int)x);
}

static inline __attribute__((always_inline)) float log2f_approx(float X) {
    float *C = &log2f_approx_coeff[0];
    float Y;
    float F;
    int E;

    // This is the approximation to log2()
    F = frexpf(fabsf(X), &E);

    //  Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
    Y = *C++;
    Y *= F;
    Y += (*C++);
    Y *= F;
    Y += (*C++);
    Y *= F;
    Y += (*C++);
    Y += E;
    return(Y);
}

#define log10f_fast(x)  (log2f_approx(x)*0.3010299956639812f)

void dft_pow_ang(float* x, fcomplex_t* X, float* P, float* A, size_t len) {

    // time and frequency domain data arrays
    int n, k;      // time and frequency domain indices

    // Calculate DFT and power spectrum up to Nyquist frequency
    int to_sin = 3 * len / 4; // index offset for sin
    int a, b;
    for (k = 0; k <= len/2; ++k) {
        X->re[k] = 0; X->im[k] = 0;
        a = 0; b = to_sin;
        for (n = 0; n < len; ++n) {
            X->re[k] += x[n] * W[a % len];
            X->im[k] -= x[n] * W[b % len];
            a += k; b += k;
        }
        P[k] = X->re[k] * X->re[k] + X->im[k] * X->im[k];
        A[k] = FastArcTan(X->im[k] / X->re[k]);
    }
}

void dft2_IPD_ILD(float* xl, float* xr, fcomplex_t* Xl, fcomplex_t* Xr, float* ILD, float* IPD, size_t len) {

    // time and frequency domain data arrays
    int n, k;      // time and frequency domain indices
    float Xr_l_re, Xr_l_im, P;

    // Calculate DFT and power spectrum up to Nyquist frequency
    int to_sin = 3 * len / 4; // index offset for sin
    int a, b;
    for (k = 0; k <= len/2; ++k) {
        Xl->re[k] = 0; Xl->im[k] = 0;
        Xr->re[k] = 0; Xr->im[k] = 0;
        a = 0; b = to_sin;
        for (n = 0; n < len; ++n) {
            Xl->re[k] += xl[n] * W[a % len];
            Xl->im[k] -= xl[n] * W[b % len];
            Xr->re[k] += xr[n] * W[a % len];
            Xr->im[k] -= xr[n] * W[b % len];
            a += k; b += k;
        }

        P = Xl->re[k] * Xl->re[k] + Xl->im[k] * Xl->im[k];
        Xr_l_re  = Xr->re[k] * Xl->re[k];
        Xr_l_im  = Xr->re[k] * Xl->im[k];
        Xr_l_re -= Xr->im[k] * Xl->im[k];
        Xr_l_im += Xr->im[k] * Xl->re[k];
        Xr_l_re /= P;
        Xr_l_im /= P;
        IPD[k] = FastArcTan(Xr_l_im / Xr_l_re);
        ILD[k] = 20 * log10f_fast(Xr_l_re * Xr_l_re + Xr_l_im * Xr_l_im);
    }
}

void idft(fcomplex_t* X, float* x, size_t len) {

    // time and frequency domain data arrays
    int n, k;      // time and frequency domain indices

    // Calculate IDFT
    int to_sin = 3 * len / 4; // index offset for sin
    int a, b;
    for (n = 0; n < len; ++n) {
        x[n] = 0;
        a = 0; b = to_sin;
        for (k = 0; k < len; ++k) {
            if (k <= len/2) {
                x[n] += X->re[k] * W[a % len];
                x[n] += X->im[k] * W[b % len];
            } else {
                x[n] += X->re[k-len/2] * W[a % len];
                x[n] += X->im[k-len/2] * W[b % len];
            }
            a += n; b += n;
        }
    }

}

void idft2(fcomplex_t* Xl, fcomplex_t* Xr, float* xl, float* xr, size_t len) {
    // time and frequency domain data arrays
    int n, k;      // time and frequency domain indices

    // Calculate IDFT
    int to_sin = 3 * len / 4; // index offset for sin
    int a, b;
    for (n = 0; n < len; ++n) {
        xl[n] = 0; xr[n] = 0;
        a = 0; b = to_sin;
        for (k = 0; k < len; ++k) {
            if (k <= len/2) {
                xl[n] += Xl->re[k] * W[a % len];
                xl[n] += Xl->im[k] * W[b % len];
                xr[n] += Xr->re[k] * W[a % len];
                xr[n] += Xr->im[k] * W[b % len];
            } else {
                xl[n] += Xl->re[k-len/2] * W[a % len];
                xl[n] += Xl->im[k-len/2] * W[b % len];
                xr[n] += Xr->re[k-len/2] * W[a % len];
                xr[n] += Xr->im[k-len/2] * W[b % len];
            }
            a += n; b += n;
        }
    }
}

// int main()
// {
//     // time and frequency domain data arrays
//     int n, k;                     // time and frequency domain indices
//     float x[N];                   // discrete-time signal, x
//     float Xre[N/2+1], Xim[N/2+1]; // DFT of x (real and imaginary parts)
//     float P[N/2+1];               // power spectrum of x
//
//     // Generate random discrete-time signal x in range (-1,+1)
//     srand(time(0));
//     for (n=0 ; n<N ; ++n) x[n] = ((2.0 * rand()) / RAND_MAX) - 1.0 + sin(PI2 * n * 5.7 / N);
//
//     // Calculate DFT and power spectrum up to Nyquist frequency
//     int to_sin = 3*N/4; // index offset for sin
//     int a, b;
//     for (k=0 ; k<=N/2 ; ++k)
//     {
//         Xre[k] = 0; Xim[k] = 0;
//         a = 0; b = to_sin;
//         for (n=0 ; n<N ; ++n)
//         {
//             Xre[k] += x[n] * W[a%N];
//             Xim[k] -= x[n] * W[b%N];
//             a += k; b += k;
//         }
//         P[k] = Xre[k]*Xre[k] + Xim[k]*Xim[k];
//     }
//
//     // Output results to MATLAB / Octave M-file for plotting
//     FILE *f = fopen("dftplots.m", "w");
//     fprintf(f, "n = [0:%d];\n", N-1);
//     fprintf(f, "x = [ ");
//     for (n=0 ; n<N ; ++n) fprintf(f, "%f ", x[n]);
//     fprintf(f, "];\n");
//     fprintf(f, "Xre = [ ");
//     for (k=0 ; k<=N/2 ; ++k) fprintf(f, "%f ", Xre[k]);
//     fprintf(f, "];\n");
//     fprintf(f, "Xim = [ ");
//     for (k=0 ; k<=N/2 ; ++k) fprintf(f, "%f ", Xim[k]);
//     fprintf(f, "];\n");
//     fprintf(f, "P = [ ");
//     for (k=0 ; k<=N/2 ; ++k) fprintf(f, "%f ", P[k]);
//     fprintf(f, "];\n");
//     fprintf(f, "subplot(3,1,1)\nplot(n,x)\n");
//     fprintf(f, "xlim([0 %d])\n", N-1);
//     fprintf(f, "subplot(3,1,2)\nplot([0:%d],Xre,[0:%d],Xim)\n", N/2, N/2);
//     fprintf(f, "xlim([0 %d])\n", N/2);
//     fprintf(f, "subplot(3,1,3)\nstem([0:%d],P)\n", N/2);
//     fprintf(f, "xlim([0 %d])\n", N/2);
//     fclose(f);
//
//     // exit normally
//     return 0;
// }
