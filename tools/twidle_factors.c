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
#include <stdlib.h>
#include <math.h>

int main(int argc, char const *argv[])
{

    if (argc != 3) {
        printf("usage %s -[s|t] <number>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[2]);

    for (int n=0 ; n<N ; ++n)
    {
        if (argv[1][1] == 't') {
            // twiddle
            printf("%8.5lf", cos(n*6.28318531/N));
        } else if (argv[1][1] == 's') {
            // sine
            printf("%8.5lf", sin(n*3.14159265/(N-1)));
        } else {
            printf("usage %s -[s|t] <number>\n", argv[0]);
            break;
        }
        if (n<N-1) printf(",");
        if ((n+1)%8==0) printf("\n");
    }
    printf("\n");
    return 0;
}
