#ifndef __HEADER_PREPROCESSOR__
#define __HEADER_PREPROCESSOR__

// preprocessor functions hacks (next time maybe)
#define _return break
#define _def(code) do { code; } while(0)
#define func(code) _def(code)
#define __FUNC_HACKS__

// stringify... (not used!)
#define _str(x) #x
#define str(x) _str(x)

#define _cat(x,y) (x ## y)
#define cat(x,y) _cat(x,y)

// maths
// expand __(-\*)__
#define _expand(MACRO) (MACRO)
#define expand(x) _expand(_expand(x))
// Sorry for this CC hope you'll do it till the end...
#define pow2(x) (expand(x) * expand(x))
// pow2(x) = (x * x)
#define pow4(x) pow2(pow2(x))
// pow4(x) = pow2((x * x)) = ((x * x) * (x * x))
#define pow8(x) pow4(pow2(x))
// pow8(x) = pow4((x * x)) = (((x * x) * (x * x)) * ((x * x) * (x * x)))
#define pow16(x) pow4(pow4(x))
// pow16(x) = pow4(((x * x) * (x * x))) = ((((x * x) * (x * x)) * ((x * x) * (x * x))) * (((x * x) * (x * x)) * ((x * x) * (x * x))))

#define max(x,y) (((x)<(y))?(y):(x))
#define abs(x) max((-x), (x))

#define limit(min, x, max) (((max) > (x)) ? (((x) > (min)) ? (x) : (min) ) : (max) )

#endif /* end of include guard: __HEADER_PREPROCESSOR__ */
