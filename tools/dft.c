// https://batchloaf.wordpress.com/2013/12/07/simple-dft-in-c/
// dft2.c - Basic, but slightly more efficient DFT
// Written by Ted Burke
// Last updated 10-12-2013
//
// To compile:
//    gcc dft2.c -o dft2.exe
//
// To run:
//    dft2.exe
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// N is assumed to be greater than 4 and a power of 2
#define N 64
#define PI2 6.2832

// Twiddle factors (64th roots of unity)
const float W[] = {
     1.00000, 0.99518, 0.98079, 0.95694, 0.92388, 0.88192, 0.83147, 0.77301,
     0.70711, 0.63439, 0.55557, 0.47139, 0.38268, 0.29028, 0.19509, 0.09801,
    -0.00000,-0.09802,-0.19509,-0.29029,-0.38269,-0.47140,-0.55557,-0.63440,
    -0.70711,-0.77301,-0.83147,-0.88192,-0.92388,-0.95694,-0.98079,-0.99519,
    -1.00000,-0.99518,-0.98078,-0.95694,-0.92388,-0.88192,-0.83146,-0.77300,
    -0.70710,-0.63439,-0.55556,-0.47139,-0.38267,-0.29027,-0.19508,-0.09801,
     0.00001, 0.09803, 0.19510, 0.29030, 0.38269, 0.47141, 0.55558, 0.63440,
     0.70712, 0.77302, 0.83148, 0.88193, 0.92388, 0.95694, 0.98079, 0.99519
};

int main()
{
    // time and frequency domain data arrays
    int n, k;                     // time and frequency domain indices
    float x[N];                   // discrete-time signal, x
    float Xre[N/2+1], Xim[N/2+1]; // DFT of x (real and imaginary parts)
    float P[N/2+1];               // power spectrum of x

    // Generate random discrete-time signal x in range (-1,+1)
    srand(time(0));
    for (n=0 ; n<N ; ++n) x[n] = ((2.0 * rand()) / RAND_MAX) - 1.0 + sin(PI2 * n * 5.7 / N);

    // Calculate DFT and power spectrum up to Nyquist frequency
    int to_sin = 3*N/4; // index offset for sin
    int a, b;
    for (k=0 ; k<=N/2 ; ++k)
    {
        Xre[k] = 0; Xim[k] = 0;
        a = 0; b = to_sin;
        for (n=0 ; n<N ; ++n)
        {
            Xre[k] += x[n] * W[a%N];
            Xim[k] -= x[n] * W[b%N];
            a += k; b += k;
        }
        P[k] = Xre[k]*Xre[k] + Xim[k]*Xim[k];
    }

    // Output results to MATLAB / Octave M-file for plotting
    FILE *f = fopen("dftplots.m", "w");
    fprintf(f, "n = [0:%d];\n", N-1);
    fprintf(f, "x = [ ");
    for (n=0 ; n<N ; ++n) fprintf(f, "%f ", x[n]);
    fprintf(f, "];\n");
    fprintf(f, "Xre = [ ");
    for (k=0 ; k<=N/2 ; ++k) fprintf(f, "%f ", Xre[k]);
    fprintf(f, "];\n");
    fprintf(f, "Xim = [ ");
    for (k=0 ; k<=N/2 ; ++k) fprintf(f, "%f ", Xim[k]);
    fprintf(f, "];\n");
    fprintf(f, "P = [ ");
    for (k=0 ; k<=N/2 ; ++k) fprintf(f, "%f ", P[k]);
    fprintf(f, "];\n");
    fprintf(f, "subplot(3,1,1)\nplot(n,x)\n");
    fprintf(f, "xlim([0 %d])\n", N-1);
    fprintf(f, "subplot(3,1,2)\nplot([0:%d],Xre,[0:%d],Xim)\n", N/2, N/2);
    fprintf(f, "xlim([0 %d])\n", N/2);
    fprintf(f, "subplot(3,1,3)\nstem([0:%d],P)\n", N/2);
    fprintf(f, "xlim([0 %d])\n", N/2);
    fclose(f);

    // exit normally
    return 0;
}
