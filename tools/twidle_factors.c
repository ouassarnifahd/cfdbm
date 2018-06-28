//
// twiddle.c - Print array of twiddle factors
// Written by Ted Burke
// Last updated 10-12-2013
//
// To compile:
//    gcc twiddle.c -o twiddle.exe
//
// To run:
//    twiddle.exe
//

#include <stdio.h>
#include <math.h>

#define N 256

int main()
{
    int n;

    for (n=0 ; n<N ; ++n)
    {
        printf("%8.5lf", cos(n*6.2832/N));
        if (n<N-1) printf(",");
        if ((n+1)%8==0) printf("\n");
    }
    return 0;
}
