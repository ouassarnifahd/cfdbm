/* ----------------------------------------------------------------------

** Fast approximation to the log2() function.  It uses a two step

** process.  First, it decomposes the floating-point number into

** a fractional component F and an exponent E.  The fraction component

** is used in a polynomial approximation and then the exponent added

** to the result.  A 3rd order polynomial is used and the result

** when computing db20() is accurate to 7.984884e-003 dB.

** ------------------------------------------------------------------- */

float log2f_approx_coeff[4] = {1.23149591368684f, -4.11852516267426f, 6.02197014179219f, -3.13396450166353f};

float log2f_approx(float X)

{

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
