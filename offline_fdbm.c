//
// offline_fdbm.c - allowing a user to focus on a speaker in a certain direction
//
// Frequency Domain Binaural Model (FDBM):
// original Paper can be found here,
// https://www.jstage.jst.go.jp/article/ast/24/4/24_4_172/_pdf
//
// this algorithm is a bit different compared to the original one.
// some modifications have been made in several parts
//
// to compile:
//     make TARGET=offline_fdbm
//
// to run:
//     ./offline_fdbm
//
// written by:
//
//    IRWANSYAH - USAGAWA Laboratory - Kumamoto University
//    irwansyah@ieee.org
//
// last updated August 7, 2018.
//

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#define LEN(x) (sizeof(x)/sizeof(x[0]))

#define fs 16000  // sampling rate
#define ch 2      // number of channels
#define N 256     // buffersize
#define theta 0   // target direction
#define Fcut 1250 // cut-off frequency
#define beta 3    // >0 to enhance a sound, =0 no enhancement.

// IPD and ILD models, these are just approximation;
// better performance with real HRTF dataset
// http://sound.media.mit.edu/resources/KEMAR.html

// I guess - this model is good enough for low frequencies
inline double IPDm(double F, double Q) {
    double ipd;
    ipd = (0.0040260*F+0.2825852)*sin(Q*M_PI/180);
    return ipd;
}

// I guess - this model is good enough for high frequencies
inline double ILDm(double F, double Q) {
    double ild;
    ild  = (0.0032276*F+3.2096991);
    ild *= sin((-3.8220e-5*F+1.5477)*Q*M_PI/180);
    return ild;
}

inline float fast_atan(float x) {
    return 0.7853 * x - x * ((int)x - 1) * (0.2447 + 0.0663 * (int)x);
}

int main()
{
    // launching FFmpeg via a pipe to read and write wav files.
    // sudo apt-get install ffmpeg

    int16_t Sig[N][ch];
    char charbufin[321];
    char charbufout[321];
    char f_format[] = "s16le";               // sample format
    char filenamein[] = "input-sound.wav";   // mixed speech signal
    char filenameout[] = "output-sound.wav"; // enhanced speech signal
    sprintf(charbufin,"ffmpeg -hide_banner -loglevel quiet -i %s -f %s -ac %d -",filenamein,f_format,ch);
    sprintf(charbufout,"ffmpeg -hide_banner -loglevel quiet -y -f %s -ar %d -ac %d -i - %s",f_format,fs,ch,filenameout);

    FILE *pipein, *pipeout;
    pipein = popen(charbufin,"r");
    pipeout = popen(charbufout,"w");

    // define sine, cosine, half-cycle sine window, ...
    // IPD/ILD target and (gNorm) segregation coeff ...
    // to speed up calculation of STFT and ISTFT
    // for speech segregation purpose.

    int n, k;
    double win[N*2], cos_nk[N*2+1][N+1], sin_nk[N*2+1][N+1];
    double gNorm[N+1],F,IPDILDm[N+1];
    for (n=0;n<=N*2;n++)
    {
        for (k=0;k<=N;k++)
        {
            cos_nk[n][k] = cos(2*M_PI*k*n/LEN(win));
            sin_nk[n][k] = sin(2*M_PI*k*n/LEN(win));
            if (n==0){
                F = k*fs/(2*N);
                if (k==0){
                    gNorm[0] = 1.0;
                }
                else{
                    gNorm[k]   = (F<Fcut) ? 2*IPDm(F,90.0) : 2*ILDm(F,90.0);
                }
                IPDILDm[k] = (F<Fcut) ? IPDm(F,theta)  : ILDm(F,theta);
            }
        }
        if (n!=N*2){
            win[n] = sin(2*M_PI*0.5*n/LEN(win));
        }
    }

    int16_t xreadbuf[N*ch];
    double xi[N][ch];
    double x[N*2][ch];
    double xbuf[N*2][ch];

    // initialize the segregation filter as 0
    double G[N+1]; for (k=0;k<=N;k++) G[k]=1.0;

    double time_spent = 0.0;
    int N_iteration = 0;

    int count, i;
    while (count = fread(xreadbuf,2,N*ch,pipein))
    {
        clock_t begin = clock();

        if (count != N*ch) break;

        N_iteration++;

        double XRe[N+1][ch] ={0}; // real values of DFT
        double XIm[N+1][ch] ={0}; // imaginary values of DFT
        double xinv[N*2][ch]={0}; // enhanced signal from invers DFT

        double XisRe[N+1]={0}; // real values of interaural spectrogram
        double XisIm[N+1]={0}; // imaginary values of interaural spectrogram
        double mu[N+1]   ={0}; //  weight factor

        // apply discrete Fourier transform (DFT)
        for (n=0;n<N;n++)
        {
            for (i=0;i<ch;i++)
            {
                xi[n][i]  = xreadbuf[n*ch+i];
                x[N+n][i] = xi[n][i]*win[N+n];

                for (k=0;k<=N;k++){
                    XRe[k][i] += x[n][i]*cos_nk[n][k] + x[N+n][i]*cos_nk[N+n][k];
                    XIm[k][i] -= x[n][i]*sin_nk[n][k] + x[N+n][i]*sin_nk[N+n][k];
                }
            }
        }

        // estimate the segregation filter
        for (k=0;k<=N;k++)
        {
            // calculate the interaural spectrogram
            XisRe[k] += XRe[k][1]*XRe[k][0] + XIm[k][1]*XIm[k][0];
            XisIm[k] += XIm[k][1]*XRe[k][0] - XRe[k][1]*XIm[k][0];
            XisRe[k] /= XRe[k][0]*XRe[k][0] + XIm[k][0]*XIm[k][0];
            XisIm[k] /= XRe[k][0]*XRe[k][0] + XIm[k][0]*XIm[k][0];

            F = k*fs/N;
            if (F<Fcut){
                // compare with IPD target
                mu[k] =  fabs(fast_atan(XisIm[k]/XisRe[k])-IPDILDm[k]);
            }
            else {
                // compare with ILD target
                mu[k] = fabs(10*log10(XisRe[k]*XisRe[k]+XisIm[k]*XisIm[k])-IPDILDm[k]);
            }
            mu[k] /= gNorm[k];
            G[k]   = pow(10,-mu[k]*beta); // the segregation filter
        }

        // apply invers discrete Fourier transform (IDFT) to the signal
        // after involving the segregation filter G[k]
        for (n=0;n<N;n++)
        {
            for (i=0;i<ch;i++)
            {
                for (k=0;k<=N;k++)
                {
                    if ((k==0)||(k==N)){
                        xinv[n][i]   += G[k]*(XRe[k][i]*cos_nk[n][k]-XIm[k][i]*sin_nk[n][k]);
                        xinv[N+n][i] += G[k]*XRe[k][i]*cos_nk[N+n][k]-XIm[k][i]*sin_nk[N+n][k];
                    }
                    else{
                        xinv[n][i]   += G[k]*XRe[k][i]*(cos_nk[n][k]+cos_nk[N*2-n][k]);
                        xinv[n][i]   -= G[k]*XIm[k][i]*(sin_nk[n][k]-sin_nk[N*2-n][k]);
                        xinv[N+n][i] += G[k]*XRe[k][i]*(cos_nk[N+n][k]+cos_nk[N-n][k]);
                        xinv[N+n][i] -= G[k]*XIm[k][i]*(sin_nk[N+n][k]-sin_nk[N-n][k]);
                    }
                }
                xinv[n][i]   /= N*2;
                xinv[N+n][i] /= N*2;

                // perfect reconstruction condition
                // read this for details
                // http://eeweb.poly.edu/iselesni/EL713/STFT/stft_inverse.pdf
                xbuf[n][i]   = xbuf[N+n][i] + xinv[n][i]*win[n];
                xbuf[N+n][i] = xinv[N+n][i]*win[N+n];

                x[n][i] = xi[n][i]*win[n];
            }
        }

        clock_t end = clock();
        time_spent += (double)(end - begin) / CLOCKS_PER_SEC;

        // store the enhanced speech signal and write the signal to a wav file
        for (n=0;n<N;n++) {
            Sig[n][0] = (int16_t) xbuf[n][0];
            Sig[n][1] = (int16_t) xbuf[n][1];
        }
        fwrite(Sig,2,N*2,pipeout);
    }
    // close input and output pipes
    pclose(pipein);
    pclose(pipeout);

    // time taken to perform FDBM on a single frame.
    printf("\nElapsed time is %.5f milliseconds.\n\n",time_spent*1000/N_iteration);
}
