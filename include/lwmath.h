#ifndef	LWMATH_H
#define	LWMATH_H

#ifndef PI
    #define PI     (3.141592653589793238462643383279502884197169399375105820974944592307816)
#endif
#ifndef TWOPI
    #define TWOPI  (2.0 * PI)
#endif
#ifndef HALFPI
    #define HALFPI (0.5 * PI)
#endif

#define VSET(a,x)       ((a)[0]=(x), (a)[1]=(x), (a)[2]=(x))
#define VCLR(a)         VSET(a,0.0)
#define VCPY(a,b)       ((a)[0] =(b)[0], (a)[1] =(b)[1], (a)[2] =(b)[2])
#define VSCL(a,x)       ((a)[0]*= (x),   (a)[1]*= (x),   (a)[2]*= (x))
#define VADD(a,b)       ((a)[0]+=(b)[0], (a)[1]+=(b)[1], (a)[2]+=(b)[2])
#define VSUB(a,b)       ((a)[0]-=(b)[0], (a)[1]-=(b)[1], (a)[2]-=(b)[2])
#define VADDS(a,b,x)    ((a)[0]+=(b)[0]*(x), (a)[1]+=(b)[1]*(x), (a)[2]+=(b)[2]*(x))
#define VSCL3(r,a,x)    ((r)[0]=(a)[0]*(x),    (r)[1]=(a)[1]*(x),    (r)[2]=(a)[2]*(x))
#define VADD3(r,a,b)    ((r)[0]=(a)[0]+(b)[0], (r)[1]=(a)[1]+(b)[1], (r)[2]=(a)[2]+(b)[2])
#define VSUB3(r,a,b)    ((r)[0]=(a)[0]-(b)[0], (r)[1]=(a)[1]-(b)[1], (r)[2]=(a)[2]-(b)[2])
#define VADDS3(r,a,b,x) ((r)[0]=(a)[0]+(b)[0]*(x), (r)[1]=(a)[1]+(b)[1]*(x), (r)[2]=(a)[2]+(b)[2]*(x))
#define VDOT(a,b)       ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2])

#ifndef VLEN
    #define VLEN(a)       sqrt(VDOT(a,a))
#endif

#define VCROSS(r,a,b)   ((r)[0] = (a)[1]*(b)[2] - (a)[2]*(b)[1],\
			 (r)[1] = (a)[2]*(b)[0] - (a)[0]*(b)[2],\
			 (r)[2] = (a)[0]*(b)[1] - (a)[1]*(b)[0])
#define VMUL3(r,a,b) ((r)[0]=(a)[0]*(b)[0], (r)[1]=(a)[1]*(b)[1], (r)[2]=(a)[2]*(b)[2])

#ifndef	ABS
    #define ABS(a) ((a < 0) ? (-(a)) : (a))
#endif

#ifndef	MAX
    #define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef	MIN
    #define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
    #define CLAMP(a,b,c) (((a) < (b)) ? (b) : (((a) > (c)) ? (c) : (a)))
#endif

#define SWAP(a,b)	{ a^=b; b^=a; a^=b; }

#define RADIANS(deg)		((deg)*0.017453292519943295769236907684886)
#define DEGREES(rad)		((rad)*57.2957795130823208767981548141052)

#endif
