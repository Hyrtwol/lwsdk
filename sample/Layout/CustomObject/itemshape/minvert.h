// Matrix Inversion from VMath.qq
#ifndef __MINVERT_H_INCLUDED
#define __MINVERT_H_INCLUDED

#include <math.h>
#include <lwtypes.h>
#include <lwmath.h>

#define ND		 3

typedef double		 Vector[ND];
typedef float		FVector[ND];
typedef short		SVector[ND];
typedef double		 Matrix[ND][ND];

void MatrixInit( Matrix m );
int MatrixInvert ( Matrix mInv, const Matrix m );
void MatrixApply( LWDVector v, Matrix mat, LWDVector src );

#endif
