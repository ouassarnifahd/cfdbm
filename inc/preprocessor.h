#ifndef __HEADER_PREPROCESSOR__
#define __HEADER_PREPROCESSOR__

// stringify...
#define _str(x) #x
#define str(x) _str(x)

#define _cat(x,y) (x ## y)
#define cat(x,y) _cat(x,y)

#endif /* end of include guard: __HEADER_PREPROCESSOR__ */
