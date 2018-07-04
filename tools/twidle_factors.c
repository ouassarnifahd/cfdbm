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

int main(int argc, char const *argv[])
{

    if (argc != 2) {
        printf("usage %s <number>\n", argv[0]);
    }

    int N = atoi(argv[1]);

    for (int n=0 ; n<N ; ++n)
    {
        printf("%8.5lf", cos(n*6.2832/N));
        if (n<N-1) printf(",");
        if ((n+1)%8==0) printf("\n");
    }
    printf("\n");
    return 0;
}
