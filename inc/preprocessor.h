#ifndef __HEADER_PREPROCESSOR__
#define __HEADER_PREPROCESSOR__

// preprocessor functions hacks (next time maybe)
#define _return break
#define _def(code) do { code; } while(0)
#define def(code) _def(code)
#define __FUNC_HACKS__

// stringify... (not used!)
#define _str(x) #x
#define str(x) _str(x)

#define _cat(x,y) (x ## y)
#define cat(x,y) _cat(x,y)

#endif /* end of include guard: __HEADER_PREPROCESSOR__ */
