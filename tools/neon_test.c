/*
arm-linux-gnueabi-gcc-4.6 -O2 -march=armv7-a -mtune=cortex-a9 -ftree-vectorize -mhard-float -mfloat-abi=softfp -mfpu=neon -ffast-math -mvectorize-with-neon-quad -S neon_test.c
*/
void NeonTest(short int * __restrict a, short int * __restrict b, short int * __restrict z)
{
  int i;
  for (i = 0; i < 200; i++) {
    z[i] = a[i] * b[i];
  }
}
