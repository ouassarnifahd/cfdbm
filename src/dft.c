#include "common.h"
#include "dft.h"

// Twiddle factors (N = 512 roots of unity)
// const float W[] = {
//     1.00000, 0.99992, 0.99970, 0.99932, 0.99880, 0.99812, 0.99729, 0.99631,
//     0.99518, 0.99391, 0.99248, 0.99090, 0.98918, 0.98730, 0.98528, 0.98311,
//     0.98079, 0.97832, 0.97570, 0.97294, 0.97003, 0.96698, 0.96378, 0.96043,
//     0.95694, 0.95331, 0.94953, 0.94561, 0.94154, 0.93734, 0.93299, 0.92851,
//     0.92388, 0.91911, 0.91421, 0.90917, 0.90399, 0.89867, 0.89322, 0.88764,
//     0.88192, 0.87607, 0.87009, 0.86397, 0.85773, 0.85135, 0.84485, 0.83822,
//     0.83147, 0.82459, 0.81758, 0.81046, 0.80321, 0.79584, 0.78835, 0.78074,
//     0.77301, 0.76517, 0.75721, 0.74914, 0.74095, 0.73265, 0.72425, 0.71573,
//     0.70711, 0.69837, 0.68954, 0.68060, 0.67156, 0.66241, 0.65317, 0.64383,
//     0.63439, 0.62486, 0.61523, 0.60551, 0.59570, 0.58580, 0.57581, 0.56573,
//     0.55557, 0.54532, 0.53500, 0.52459, 0.51410, 0.50354, 0.49290, 0.48218,
//     0.47139, 0.46054, 0.44961, 0.43861, 0.42755, 0.41643, 0.40524, 0.39399,
//     0.38268, 0.37131, 0.35989, 0.34842, 0.33689, 0.32531, 0.31368, 0.30200,
//     0.29028, 0.27852, 0.26671, 0.25486, 0.24298, 0.23106, 0.21910, 0.20711,
//     0.19509, 0.18304, 0.17096, 0.15885, 0.14673, 0.13458, 0.12241, 0.11022,
//     0.09801, 0.08579, 0.07356, 0.06132, 0.04906, 0.03680, 0.02454, 0.01227,
//    -0.00000,-0.01228,-0.02454,-0.03681,-0.04907,-0.06132,-0.07357,-0.08580,
//    -0.09802,-0.11023,-0.12241,-0.13458,-0.14673,-0.15886,-0.17097,-0.18304,
//    -0.19509,-0.20712,-0.21911,-0.23106,-0.24298,-0.25487,-0.26672,-0.27852,
//    -0.29029,-0.30201,-0.31369,-0.32531,-0.33689,-0.34842,-0.35990,-0.37132,
//    -0.38269,-0.39400,-0.40525,-0.41643,-0.42756,-0.43862,-0.44962,-0.46054,
//    -0.47140,-0.48219,-0.49290,-0.50354,-0.51411,-0.52459,-0.53500,-0.54533,
//    -0.55557,-0.56574,-0.57581,-0.58580,-0.59570,-0.60552,-0.61524,-0.62486,
//    -0.63440,-0.64384,-0.65318,-0.66242,-0.67156,-0.68060,-0.68954,-0.69838,
//    -0.70711,-0.71573,-0.72425,-0.73266,-0.74095,-0.74914,-0.75721,-0.76517,
//    -0.77301,-0.78074,-0.78835,-0.79584,-0.80321,-0.81046,-0.81759,-0.82459,
//    -0.83147,-0.83823,-0.84486,-0.85136,-0.85773,-0.86398,-0.87009,-0.87607,
//    -0.88192,-0.88764,-0.89323,-0.89868,-0.90399,-0.90917,-0.91421,-0.91912,
//    -0.92388,-0.92851,-0.93300,-0.93734,-0.94155,-0.94561,-0.94953,-0.95331,
//    -0.95694,-0.96043,-0.96378,-0.96698,-0.97003,-0.97294,-0.97570,-0.97832,
//    -0.98079,-0.98311,-0.98528,-0.98730,-0.98918,-0.99090,-0.99248,-0.99391,
//    -0.99519,-0.99631,-0.99729,-0.99812,-0.99880,-0.99932,-0.99970,-0.99992,
//    -1.00000,-0.99992,-0.99970,-0.99932,-0.99880,-0.99812,-0.99729,-0.99631,
//    -0.99518,-0.99391,-0.99248,-0.99090,-0.98918,-0.98730,-0.98528,-0.98310,
//    -0.98078,-0.97832,-0.97570,-0.97294,-0.97003,-0.96697,-0.96377,-0.96043,
//    -0.95694,-0.95330,-0.94953,-0.94560,-0.94154,-0.93734,-0.93299,-0.92850,
//    -0.92388,-0.91911,-0.91421,-0.90916,-0.90399,-0.89867,-0.89322,-0.88764,
//    -0.88192,-0.87607,-0.87008,-0.86397,-0.85772,-0.85135,-0.84485,-0.83822,
//    -0.83146,-0.82458,-0.81758,-0.81045,-0.80320,-0.79583,-0.78834,-0.78073,
//    -0.77300,-0.76516,-0.75720,-0.74913,-0.74095,-0.73265,-0.72424,-0.71572,
//    -0.70710,-0.69837,-0.68953,-0.68059,-0.67155,-0.66241,-0.65317,-0.64382,
//    -0.63439,-0.62485,-0.61522,-0.60550,-0.59569,-0.58579,-0.57580,-0.56572,
//    -0.55556,-0.54532,-0.53499,-0.52458,-0.51409,-0.50353,-0.49289,-0.48218,
//    -0.47139,-0.46053,-0.44960,-0.43861,-0.42755,-0.41642,-0.40523,-0.39398,
//    -0.38267,-0.37131,-0.35989,-0.34841,-0.33688,-0.32530,-0.31367,-0.30200,
//    -0.29027,-0.27851,-0.26670,-0.25486,-0.24297,-0.23105,-0.21909,-0.20710,
//    -0.19508,-0.18303,-0.17095,-0.15885,-0.14672,-0.13457,-0.12240,-0.11021,
//    -0.09801,-0.08579,-0.07355,-0.06131,-0.04906,-0.03680,-0.02453,-0.01226,
//     0.00001, 0.01228, 0.02455, 0.03682, 0.04908, 0.06133, 0.07358, 0.08581,
//     0.09803, 0.11023, 0.12242, 0.13459, 0.14674, 0.15887, 0.17097, 0.18305,
//     0.19510, 0.20712, 0.21911, 0.23107, 0.24299, 0.25488, 0.26672, 0.27853,
//     0.29030, 0.30202, 0.31369, 0.32532, 0.33690, 0.34843, 0.35991, 0.37133,
//     0.38269, 0.39400, 0.40525, 0.41644, 0.42757, 0.43863, 0.44962, 0.46055,
//     0.47141, 0.48219, 0.49291, 0.50355, 0.51411, 0.52460, 0.53501, 0.54534,
//     0.55558, 0.56574, 0.57582, 0.58581, 0.59571, 0.60552, 0.61524, 0.62487,
//     0.63440, 0.64384, 0.65318, 0.66243, 0.67157, 0.68061, 0.68955, 0.69839,
//     0.70712, 0.71574, 0.72426, 0.73266, 0.74096, 0.74915, 0.75722, 0.76518,
//     0.77302, 0.78075, 0.78835, 0.79584, 0.80322, 0.81046, 0.81759, 0.82460,
//     0.83148, 0.83823, 0.84486, 0.85136, 0.85774, 0.86398, 0.87009, 0.87608,
//     0.88193, 0.88765, 0.89323, 0.89868, 0.90400, 0.90917, 0.91422, 0.91912,
//     0.92388, 0.92851, 0.93300, 0.93734, 0.94155, 0.94561, 0.94953, 0.95331,
//     0.95694, 0.96043, 0.96378, 0.96698, 0.97003, 0.97294, 0.97571, 0.97832,
//     0.98079, 0.98311, 0.98528, 0.98730, 0.98918, 0.99090, 0.99248, 0.99391,
//     0.99519, 0.99631, 0.99729, 0.99812, 0.99880, 0.99932, 0.99970, 0.99992
// };

// N = 10
const float W[] = {
    1.00000, 0.86603, 0.50000,-0.00000,-0.50000,-0.86603,-1.00000,-0.86603, -0.50000, 0.00000, 0.50000, 0.86603
};

// Sine Windows (N = 512)
const float SINE[] = {
    0.00000, 0.00615, 0.01230, 0.01844, 0.02459, 0.03073, 0.03688, 0.04302,
    0.04916, 0.05530, 0.06144, 0.06758, 0.07371, 0.07984, 0.08596, 0.09209,
    0.09821, 0.10432, 0.11044, 0.11655, 0.12265, 0.12875, 0.13484, 0.14093,
    0.14702, 0.15309, 0.15917, 0.16523, 0.17129, 0.17735, 0.18339, 0.18943,
    0.19547, 0.20149, 0.20751, 0.21352, 0.21952, 0.22552, 0.23150, 0.23748,
    0.24345, 0.24940, 0.25535, 0.26129, 0.26722, 0.27314, 0.27905, 0.28495,
    0.29084, 0.29671, 0.30258, 0.30843, 0.31427, 0.32011, 0.32592, 0.33173,
    0.33752, 0.34330, 0.34907, 0.35483, 0.36057, 0.36629, 0.37201, 0.37771,
    0.38339, 0.38906, 0.39472, 0.40036, 0.40599, 0.41160, 0.41719, 0.42277,
    0.42834, 0.43388, 0.43941, 0.44493, 0.45043, 0.45591, 0.46137, 0.46682,
    0.47224, 0.47765, 0.48305, 0.48842, 0.49378, 0.49911, 0.50443, 0.50973,
    0.51501, 0.52027, 0.52551, 0.53073, 0.53593, 0.54111, 0.54627, 0.55141,
    0.55653, 0.56163, 0.56670, 0.57176, 0.57679, 0.58180, 0.58679, 0.59176,
    0.59670, 0.60162, 0.60652, 0.61140, 0.61625, 0.62108, 0.62589, 0.63067,
    0.63543, 0.64017, 0.64488, 0.64956, 0.65423, 0.65886, 0.66348, 0.66806,
    0.67263, 0.67716, 0.68167, 0.68616, 0.69062, 0.69505, 0.69946, 0.70384,
    0.70819, 0.71252, 0.71682, 0.72109, 0.72534, 0.72956, 0.73375, 0.73791,
    0.74205, 0.74615, 0.75023, 0.75428, 0.75831, 0.76230, 0.76626, 0.77020,
    0.77411, 0.77798, 0.78183, 0.78565, 0.78944, 0.79320, 0.79693, 0.80062,
    0.80429, 0.80793, 0.81154, 0.81512, 0.81866, 0.82218, 0.82566, 0.82911,
    0.83254, 0.83593, 0.83928, 0.84261, 0.84591, 0.84917, 0.85240, 0.85560,
    0.85876, 0.86190, 0.86500, 0.86807, 0.87110, 0.87411, 0.87708, 0.88001,
    0.88292, 0.88579, 0.88862, 0.89142, 0.89419, 0.89693, 0.89963, 0.90230,
    0.90493, 0.90753, 0.91010, 0.91263, 0.91512, 0.91758, 0.92001, 0.92240,
    0.92476, 0.92708, 0.92937, 0.93162, 0.93384, 0.93602, 0.93816, 0.94028,
    0.94235, 0.94439, 0.94639, 0.94836, 0.95029, 0.95219, 0.95405, 0.95587,
    0.95766, 0.95941, 0.96113, 0.96281, 0.96445, 0.96606, 0.96763, 0.96916,
    0.97066, 0.97212, 0.97354, 0.97493, 0.97628, 0.97759, 0.97887, 0.98010,
    0.98131, 0.98247, 0.98360, 0.98469, 0.98574, 0.98676, 0.98774, 0.98868,
    0.98958, 0.99045, 0.99128, 0.99207, 0.99282, 0.99354, 0.99422, 0.99486,
    0.99546, 0.99603, 0.99656, 0.99705, 0.99750, 0.99792, 0.99829, 0.99863,
    0.99894, 0.99920, 0.99943, 0.99962, 0.99977, 0.99988, 0.99996, 1.00000,
    1.00000, 0.99996, 0.99988, 0.99977, 0.99962, 0.99943, 0.99920, 0.99894,
    0.99863, 0.99829, 0.99792, 0.99750, 0.99705, 0.99656, 0.99603, 0.99546,
    0.99486, 0.99422, 0.99354, 0.99282, 0.99207, 0.99128, 0.99045, 0.98958,
    0.98868, 0.98774, 0.98676, 0.98574, 0.98469, 0.98360, 0.98247, 0.98131,
    0.98010, 0.97887, 0.97759, 0.97628, 0.97493, 0.97354, 0.97212, 0.97066,
    0.96916, 0.96763, 0.96606, 0.96445, 0.96281, 0.96113, 0.95941, 0.95766,
    0.95587, 0.95405, 0.95219, 0.95029, 0.94836, 0.94639, 0.94439, 0.94235,
    0.94028, 0.93816, 0.93602, 0.93384, 0.93162, 0.92937, 0.92708, 0.92476,
    0.92240, 0.92001, 0.91758, 0.91512, 0.91263, 0.91010, 0.90753, 0.90493,
    0.90230, 0.89963, 0.89693, 0.89419, 0.89142, 0.88862, 0.88579, 0.88292,
    0.88001, 0.87708, 0.87411, 0.87110, 0.86807, 0.86500, 0.86190, 0.85876,
    0.85560, 0.85240, 0.84917, 0.84591, 0.84261, 0.83928, 0.83593, 0.83254,
    0.82911, 0.82566, 0.82218, 0.81866, 0.81512, 0.81154, 0.80793, 0.80429,
    0.80062, 0.79693, 0.79320, 0.78944, 0.78565, 0.78183, 0.77798, 0.77411,
    0.77020, 0.76626, 0.76230, 0.75831, 0.75428, 0.75023, 0.74615, 0.74205,
    0.73791, 0.73375, 0.72956, 0.72534, 0.72109, 0.71682, 0.71252, 0.70819,
    0.70384, 0.69946, 0.69505, 0.69062, 0.68616, 0.68167, 0.67716, 0.67263,
    0.66806, 0.66348, 0.65886, 0.65423, 0.64956, 0.64488, 0.64017, 0.63543,
    0.63067, 0.62589, 0.62108, 0.61625, 0.61140, 0.60652, 0.60162, 0.59670,
    0.59176, 0.58679, 0.58180, 0.57679, 0.57176, 0.56670, 0.56163, 0.55653,
    0.55141, 0.54627, 0.54111, 0.53593, 0.53073, 0.52551, 0.52027, 0.51501,
    0.50973, 0.50443, 0.49911, 0.49378, 0.48842, 0.48305, 0.47765, 0.47224,
    0.46682, 0.46137, 0.45591, 0.45043, 0.44493, 0.43941, 0.43388, 0.42834,
    0.42277, 0.41719, 0.41160, 0.40599, 0.40036, 0.39472, 0.38906, 0.38339,
    0.37771, 0.37201, 0.36629, 0.36057, 0.35483, 0.34907, 0.34330, 0.33752,
    0.33173, 0.32592, 0.32011, 0.31427, 0.30843, 0.30258, 0.29671, 0.29084,
    0.28495, 0.27905, 0.27314, 0.26722, 0.26129, 0.25535, 0.24940, 0.24345,
    0.23748, 0.23150, 0.22552, 0.21952, 0.21352, 0.20751, 0.20149, 0.19547,
    0.18943, 0.18339, 0.17735, 0.17129, 0.16523, 0.15917, 0.15309, 0.14702,
    0.14093, 0.13484, 0.12875, 0.12265, 0.11655, 0.11044, 0.10432, 0.09821,
    0.09209, 0.08596, 0.07984, 0.07371, 0.06758, 0.06144, 0.05530, 0.04916,
    0.04302, 0.03688, 0.03073, 0.02459, 0.01844, 0.01230, 0.00615, 0.00000
};

float log2f_approx_coeff[4] = {1.23149591368684f, -4.11852516267426f, 6.02197014179219f, -3.13396450166353f};

// plot (gnuplot)
INVISIBLE void plot(const char* title, const float* data, size_t len) {
    #ifndef __arm__
    FILE *gnuplot = popen("gnuplot -p", "w");
    // here config
    fprintf(gnuplot, "set title '%s';\n", title);
    fprintf(gnuplot, "set grid;\n");
    fprintf(gnuplot, "set xlabel 'Samples';\n");
    fprintf(gnuplot, "set ylabel 'Amplitude';\n");
    // here data...
    fprintf(gnuplot, "plot '-'\n");
    for (int i = 0; i < len; i++)
    fprintf(gnuplot, "%f\n", data[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
    #endif
}

INVISIBLE void fft_plot(const float* data, size_t len) {
    #ifndef __arm__
    FILE *gnuplot = popen("gnuplot -p", "w");
    // here config
    fprintf(gnuplot, "set title 'FFT';\n");
    fprintf(gnuplot, "set grid;\n");
    fprintf(gnuplot, "set xlabel 'FREQ';\n");
    fprintf(gnuplot, "set ylabel 'dB';\n");
    // here data...
    fprintf(gnuplot, "plot '-'\n");
    for (int i = 0; i < len; i++)
    fprintf(gnuplot, "%f\n", data[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
    #endif
}

INVISIBLE float FastArcTan(float x) {
    return 0.7853 * x - x * ((int)x - 1) * (0.2447 + 0.0663 * (int)x);
}

INVISIBLE float log2f_approx(float X) {
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

#define log10f_fast(x) (log2f_approx(x)*0.3010299956639812f)

// TODO debug and check results

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
            // debug("k = %d, n = %d, x = %f, re = %f, im = %f; (cos = %f, sin = %f)",
            //     k, n, x[n], X->re[k], X->im[k], W[a % len], W[b % len]);
        }
        X->re[len-k] =  X->re[k];
        X->im[len-k] = -X->im[k];

        P[k] = sqrt(X->re[k] * X->re[k] + X->im[k] * X->im[k]);
        A[k] = FastArcTan(X->im[k] / X->re[k]);
    }
}

// TODO optimise
void dft2_IPDILD(float* xl, float* xr, fcomplex_t* Xl, fcomplex_t* Xr, float* ILD, float* IPD, size_t len) {

    // time and frequency domain data arrays
    register int n, k;      // time and frequency domain indexes
    float Xr_l_re, Xr_l_im, P[CHANNEL_SAMPLES_COUNT], A[CHANNEL_SAMPLES_COUNT];

    // Calculate DFT and power spectrum up to Nyquist frequency
    int to_sin = 3 * len / 4; // index offset for sin
    int a, b, len_2 = len >> 1;
    for (k = 0; k <= len_2; ++k) {
        Xl->re[k] = 0; Xl->im[k] = 0;
        Xr->re[k] = 0; Xr->im[k] = 0;
        a = 0; b = to_sin;
        for (n = 0; n < len; ++n) {
            Xl->re[k] += xl[n] * W[a % len];
            Xl->im[k] -= xl[n] * W[b % len];
            Xr->re[k] += xr[n] * W[a % len];
            Xr->im[k] -= xr[n] * W[b % len];
            a += k; b = (b + k) % len;
        }
        Xl->re[len-k] =  Xl->re[k];
        Xl->im[len-k] = -Xl->im[k];
        Xr->re[len-k] =  Xr->re[k];
        Xr->im[len-k] = -Xr->im[k];

        // Calculate abs and angle
        P[k]  = Xl->re[k] * Xl->re[k];
        P[k] += Xl->im[k] * Xl->im[k];
        P[k]  = sqrt(P[k]);

        // FFT_PLOT
        // P[k]  = 20 * log10f_fast(P[k]);
        //
        // A[k]  = Xr->re[k] * Xr->re[k];
        // A[k] += Xr->im[k] * Xr->im[k];
        // A[k]  = sqrt(A[k]);
        // A[k]  = 20 * log10f_fast(A[k]);

        Xr_l_re  = Xr->re[k] * Xl->re[k];
        Xr_l_im  = Xr->re[k] * Xl->im[k];
        Xr_l_re -= Xr->im[k] * Xl->im[k];
        Xr_l_im += Xr->im[k] * Xl->re[k];
        Xr_l_re /= P[k];
        Xr_l_im /= P[k];
        IPD[k] = FastArcTan(Xr_l_im / Xr_l_re);
        ILD[k] = 20 * log10f_fast(Xr_l_re * Xr_l_re + Xr_l_im * Xr_l_im);
    }
    // plot fft_L
    // fft_plot(P, CHANNEL_SAMPLES_COUNT/2);
    // plot("FFT Right", A, CHANNEL_SAMPLES_COUNT/2);
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
            // if (k < len/2) {
                x[n] += X->re[k] * W[a % len];
                x[n] -= X->im[k] * W[b % len];
            // } else {
                // x[n] += X->re[len-k] * W[a % len];
                // x[n] -= X->im[len-k] * W[b % len];
            // }
            a += n; b += n;
        }
        x[n] /= len;
    }

}

void idft2_SINE_WIN(fcomplex_t* Xl, fcomplex_t* Xr, float* xl, float* xr, size_t len) {
    // time and frequency domain data arrays
    register int n, k;      // time and frequency domain indexes

    // Calculate IDFT
    int to_sin = 3 * len / 4; // index offset for sin
    int a, b, len_2 = len >> 1;
    for (n = 0; n < len; ++n) {
        xl[n] = 0; xr[n] = 0;
        a = 0; b = to_sin;
        for (k = 0; k < len; ++k) {
            // if (k < len_2) {
                xl[n] += Xl->re[k] * W[a % len];
                xl[n] -= Xl->im[k] * W[b % len];
                xr[n] += Xr->re[k] * W[a % len];
                xr[n] -= Xr->im[k] * W[b % len];
            // } else {
            //     xl[n] += Xl->re[len-k] * W[a % len];
            //     xl[n] += Xl->im[len-k] * W[b % len];
            //     xr[n] += Xr->re[len-k] * W[a % len];
            //     xr[n] += Xr->im[len-k] * W[b % len];
            // }
            a += n; b += n;
        }
        xl[n] /= len;
        xr[n] /= len;
        // xl[n] *= SINE[n];
        // xr[n] *= SINE[n];
    }
}

// int main(int argc, char const *argv[]) {
//     init_log();
//     float x[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
//     float s[12];
//     float P[12], A[12];
//     fcomplex_t X;
//     dft_pow_ang(x, &X, P, A, 12);
//     for (size_t i = 0; i < 12; i++) {
//         printf("%f %fi, ", X.re[i], X.im[i]);
//     }
//     printf("\n");
//     idft(&X, s, 12);
//     for (size_t i = 0; i < 12; i++) {
//         printf("%f, ", s[i]);
//     }
//     printf("\n");
//     return 0;
// }
