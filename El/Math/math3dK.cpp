// homogenous vector and matrix arithmetics
// (C) 1997, SUMA
#include <El/elementpch.hpp>

#if _M_IX86_FP>0 || _M_X64 || defined _M_PPC

#include "math3dK.hpp"

#pragma warning(disable:4073)
#pragma init_seg(lib)

#include <Es/Framework/debugLog.hpp>

const Vector3K VZeroK(0,0,0) INIT_PRIORITY_HIGH;
const Vector3K VAsideK(1,0,0) INIT_PRIORITY_HIGH;
const Vector3K VUpK(0,1,0) INIT_PRIORITY_HIGH;
const Vector3K VForwardK(0,0,1) INIT_PRIORITY_HIGH;

const Matrix3K M3ZeroK
(
	0,0,0,
	0,0,0,
	0,0,0
) INIT_PRIORITY_HIGH;
const Matrix3K M3IdentityK
(
	1,0,0,
	0,1,0,
	0,0,1
) INIT_PRIORITY_HIGH;

const Matrix4K M4ZeroK
(
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0
) INIT_PRIORITY_HIGH;
const Matrix4K M4IdentityK
(
	1,0,0,0,
	0,1,0,0,
	0,0,1,0
) INIT_PRIORITY_HIGH;

#if _MSC_VER
bool Matrix4K::IsFinite() const
{
	for( int i=0; i<3; i++ )
	{
		if( !_finite(Get(i,0)) ) return false;
		if( !_finite(Get(i,1)) ) return false;
		if( !_finite(Get(i,2)) ) return false;
		if( !_finite(GetPos(i)) ) return false;
	}
	return true;
}
bool Matrix3K::IsFinite() const
{
	for( int i=0; i<3; i++ )
	{
		if( !_finite(Get(i,0)) ) return false;
		if( !_finite(Get(i,1)) ) return false;
		if( !_finite(Get(i,2)) ) return false;
	}
	return true;
}
#endif

float Matrix4K::Characteristic() const
{
	// used in fast comparison
	// sum of all data members
	float sum1=0; // lower dependecies
	float sum2=0;
	for( int i=0; i<3; i++ )
	{
		sum1+=Get(i,0);
		sum2+=Get(i,1);
		sum1+=Get(i,2);
		sum2+=GetPos(i);
	}
	return sum1+sum2;
}

void Matrix4K::SetTranslation( Vector3KPar offset )
{
	SetIdentity();
	SetPosition(offset);
}

void Matrix4K::SetRotationX( Coord angle )
{
	SetIdentity();
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	Set(1,1)=+c,Set(1,2)=-s;
	Set(2,1)=+s,Set(2,2)=+c;
}

void Matrix4K::SetRotationY( Coord angle )
{
	SetIdentity();
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	Set(0,0)=+c,Set(0,2)=-s;
	Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix4K::SetRotationZ( Coord angle )
{
	SetIdentity();
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	Set(0,0)=+c,Set(0,1)=-s;
	Set(1,0)=+s,Set(1,1)=+c;
}

void Matrix4K::SetScale( Coord x, Coord y, Coord z )
{
	SetZero();
	Set(0,0)=x;
	Set(1,1)=y;
	Set(2,2)=z;
	//Set(3,3)=1;
}

void Matrix4K::SetPerspective( Coord cLeft, Coord cTop )
{
	SetZero();
	// xg=x*near/right :: <-1,+1>
	Set(0,0)=1.0f/cLeft;
	// yg=y*near/top :: <-1,+1>
	Set(1,1)=1.0f/cTop;
	
	// zg=-w*1
	SetPos(2)=-1;
	
	// wg=z
	//Set(3,2)=1; 
	
	// this gives actual z result (suppose w=1) = zg/wg =
	// zRes(z)=-1/z;
}

void Matrix4K::Orthogonalize()
{
	Vector3KVal dir=Direction();
	Vector3KVal up=DirectionUp();
	SetDirectionAndUp(dir,up);
}
void Matrix3K::Orthogonalize()
{
	Vector3KVal dir=Direction();
	Vector3KVal up=DirectionUp();
	SetDirectionAndUp(dir,up);
}

void Matrix4K::SetOriented( Vector3KPar dir, Vector3KPar up )
{
	SetIdentity();
	SetDirectionAndUp(dir,up);
}

void Matrix4K::SetView( Vector3KPar point, Vector3KPar dir, Vector3KPar up )
{
	Matrix4K translate(MTranslation,-point);
	Matrix4K direction(MDirection,dir,up);
	SetMultiply(direction,translate);
}


#ifndef _M_PPC
void Matrix4K::SetMultiply( const Matrix4K &a, const Matrix4K &b )
{
	// SSE matrix multiplication
	_orientation._aside =
	(
		a._orientation._aside * b._orientation._aside.X() +
		a._orientation._up    * b._orientation._aside.Y() +
		a._orientation._dir   * b._orientation._aside.Z()
	);
	_orientation._up =
	(
		a._orientation._aside * b._orientation._up.X() +
		a._orientation._up    * b._orientation._up.Y() +
		a._orientation._dir   * b._orientation._up.Z()
	);
	_orientation._dir =
	(
		a._orientation._aside * b._orientation._dir.X() +
		a._orientation._up    * b._orientation._dir.Y() +
		a._orientation._dir   * b._orientation._dir.Z()
	);
	_position =
	(
		a._orientation._aside * b._position.X() + 
		a._orientation._up * b._position.Y() + 
		a._orientation._dir * b._position.Z() + 
		a._position
	);
}
#else

void Matrix4K::SetMultiply( const Matrix4K &a, const Matrix4K &b )
{
  const XMMATRIX &M2 = (const XMMATRIX &)a;
  const XMMATRIX &M1 = (const XMMATRIX &)b;
  XMMATRIX &Result = *(XMMATRIX *)this;
  // M2.r[i].w contains random data instead of 0 0 0 1
  // M1.r[i].w contains random data instead of 0 0 0 1
  // based on XMMatrixMultiply, version _VMX128_INTRINSICS_
  XMVECTOR M00, M01, M02, M03,
  M10, M11, M12, M13,
  M20, M21, M22, M23,
  M30, M31, M32, M33;
  XMVECTOR M00M02, M01M03, M10M12, M11M13,
    M20M22, M21M23, M30M32, M31M33;
  XMMATRIX M2T, P;

  P.r[0] = __vmrghw(M2.r[0], M2.r[2]);
  P.r[1] = __vmrghw(M2.r[1], M2.r[3]);
  P.r[2] = __vmrglw(M2.r[0], M2.r[2]);
  P.r[3] = __vmrglw(M2.r[1], M2.r[3]);

  M2T.r[0] = __vmrghw(P.r[0], P.r[1]);
  M2T.r[2] = __vmrghw(P.r[2], P.r[3]);
  M2T.r[1] = __vmrglw(P.r[0], P.r[1]);
  M2T.r[3] = __vmrglw(P.r[2], P.r[3]);

  // M1.r[i].w contains random data instead of 0 0 0 1
  // M2.r[i].w contains random data instead of 0 0 0 1
  // M2T.r[3] contains random data instead of 0 0 0 1

  M00 = __vmsum3fp(M1.r[0], M2T.r[0]);
  M02 = __vmsum3fp(M1.r[0], M2T.r[2]);
  M01 = __vmsum3fp(M1.r[0], M2T.r[1]);
  M03 = __vspltisw(0); //  __vm __vmsum4fp(M1.r[0], M2T.r[3]); // 0

  M10 = __vmsum3fp(M1.r[1], M2T.r[0]);
  M12 = __vmsum3fp(M1.r[1], M2T.r[2]);
  M11 = __vmsum3fp(M1.r[1], M2T.r[1]);
  M13 = __vspltisw(0); //__vmsum4fp(M1.r[1], M2T.r[3]); // 0

  M20 = __vmsum3fp(M1.r[2], M2T.r[0]);
  M22 = __vmsum3fp(M1.r[2], M2T.r[2]);
  M21 = __vmsum3fp(M1.r[2], M2T.r[1]);
  M23 = __vspltisw(0); //__vmsum4fp(M1.r[2], M2T.r[3]); // 0

  M30 = __vmsum3fp(M1.r[3], M2T.r[0]);
  M32 = __vmsum3fp(M1.r[3], M2T.r[2]);
  M31 = __vmsum3fp(M1.r[3], M2T.r[1]);
  M33 = XMVectorSplatOne(); // __vmsum4fp(M1.r[3], M2T.r[3]); // 1

  M00M02 = __vmrghw(M00, M02);
  M01M03 = __vmrghw(M01, M03);
  M10M12 = __vmrghw(M10, M12);
  M11M13 = __vmrghw(M11, M13);
  M20M22 = __vmrghw(M20, M22);
  M21M23 = __vmrghw(M21, M23);
  M30M32 = __vmrghw(M30, M32);
  M31M33 = __vmrghw(M31, M33);

  Result.r[0] = __vmrghw(M00M02, M01M03);
  Result.r[1] = __vmrghw(M10M12, M11M13);
  Result.r[2] = __vmrghw(M20M22, M21M23);
  Result.r[3] = __vaddfp(__vmrghw(M30M32, M31M33),M2.r[3]);
}

#endif

void Matrix4K::SetMultiply( const Matrix4K &a, FloatQuadPar f )
{
	_orientation.SetMultiply(a._orientation,f);
	_position=a._position*f;
}

void Matrix4K::AddMultiply( const Matrix4K &a, float b )
{
	_orientation._aside += a._orientation._aside*b;
	_orientation._up += a._orientation._up*b;
	_orientation._dir += a._orientation._dir*b;
	_position += a._position*b;
}

void Matrix4K::SetMultiplyByPerspective( const Matrix4K &a, const Matrix4K &b )
{
	// matrix multiplication
	int i,j;
	// b is perspective projection
	// b(3,0)=0, b(3,1)=0, b(3,2)=1, b(3,3)=0
	// a(3,0)=0, a(3,1)=0, a(3,2)=0, a(3,3)=1
	for( i=0; i<3; i++ ) for( j=0; j<3; j++ )
	{
		Set(i,j)=
		(
			a.Get(i,0)*b.Get(0,j)+
			a.Get(i,1)*b.Get(1,j)+
			a.Get(i,2)*b.Get(2,j)+
			//a.Get(i,3)*b.Get(3,j)
			a.GetPos(i)*(j==2)
		);
	}
	for( i=0; i<3; i++ )
	{
		SetPos(i)=
		(
			a.Get(i,0)*b.GetPos(0)+
			a.Get(i,1)*b.GetPos(1)+
			a.Get(i,2)*b.GetPos(2)
		);
	}
	//for( j=0; j<3; j++ ) Set(3,j)=b.Get(3,j);
	//Set(3,3)=0;
}

/*
Coord Vector3K::operator * ( Vector3KPar op ) const
{
	__m128 res=HorizontalSum3(_mm_mul_ps(_k,op._k));
	float ret;
	_mm_store_ss(&ret,res);
	return ret;
}
*/

inline float InvSquareSize( float x, float y, float z )
{
	return Inv(x*x+y*y+z*z);
}

Vector3K Vector3K::Normalized() const
{
  #ifdef _M_PPC
  return Vector3K(XMVector3Normalize(_k));
  #else
  Coord size2=SquareSizeInline();
  if( size2<=FLT_MIN ) return *this;
  Coord invSize=InvSqrt(size2);
  return *this * invSize;
  #endif
}

void Vector3K::Normalize() // no return to avoid using instead of Normalized
{ 
  #ifdef _M_PPC
  // XMVector3Normalize handles even singularity (zero) well
  _k = XMVector3Normalize(_k);
  #else
  Coord size2=SquareSizeInline();
  if( size2<=FLT_MIN ) return;
  Coord invSize=InvSqrt(size2);
  *this *= invSize;
  #endif
}


float Vector3K::CosAngle( Vector3KPar op ) const
{
	return DotProduct(op)*InvSqrt(op.SquareSize()*SquareSize());
}

float Vector3K::Distance( Vector3KPar op ) const
{
	return (*this-op).Size();
}

float Vector3K::Distance2( Vector3KPar op ) const
{
	return (op-*this).SquareSize();
}

float Vector3K::DistanceXZ( Vector3KPar op ) const
{
	return (op-*this).SizeXZ();
}

float Vector3K::DistanceXZ2( Vector3KPar op ) const
{
	Vector3K temp = op-*this;
	return temp.SquareSizeXZ();
}

Vector3K Vector3K::Project( Vector3KPar op ) const
{
	return op*DotProduct(op)*op.InvSquareSize();
}

#if _MSC_VER
bool Vector3K::IsFinite() const
{
	if( !_finite(Get(0)) ) return false;
	if( !_finite(Get(1)) ) return false;
	if( !_finite(Get(2)) ) return false;
	return true;
}
#endif

#if !_M_PPC
void Vector3K::SetFastTransform( const Matrix4K &a, Vector3KPar o )
{
	__m128 t1,t2,t3;

	t1 = _mm_set_ps1(o.X());
	t2 = _mm_set_ps1(o.Y());

	t1 = _mm_mul_ps( t1, a._orientation._aside._k );
	t2 = _mm_mul_ps( t2, a._orientation._up._k );

	t3 = _mm_set_ps1(o.Z());

	t1 = _mm_add_ps( t1, t2 );
	t3 = _mm_mul_ps( t3, a._orientation._dir._k );

	// sum a...
	t3 = _mm_add_ps( t3, a._position._k );
	_k = _mm_add_ps( t1, t3 );
}

Vector3K Matrix4K::FastTransform( Vector3KPar o ) const
{
	// matrix stored in major-column format
	__m128 t1,t2,t3;

	t1 = _mm_set_ps1(o.X());
	t2 = _mm_set_ps1(o.Y());

	t1 = _mm_mul_ps( t1, _orientation._aside._k );
	t2 = _mm_mul_ps( t2, _orientation._up._k );

	t3 = _mm_set_ps1(o.Z());

	t1 = _mm_add_ps( t1, t2 );
	t3 = _mm_mul_ps( t3, _orientation._dir._k );

	// sum a...
	t3 = _mm_add_ps( t3, _position._k );
	return _mm_add_ps( t1, t3 );
}

Vector3K Matrix4K::Rotate( Vector3KPar o ) const
{
	// matrix stored in major-column format
	__m128 t1,t2,t3;

	t1 = _mm_set_ps1(o.X());
	t2 = _mm_set_ps1(o.Y());

	t1 = _mm_mul_ps( t1, _orientation._aside._k );
	t2 = _mm_mul_ps( t2, _orientation._up._k );

	t3 = _mm_set_ps1(o.Z());

	t1 = _mm_add_ps( t1, t2 );
	t3 = _mm_mul_ps( t3, _orientation._dir._k );

	// sum a...
	return _mm_add_ps( t1, t3 );
}
#else
void Vector3K::SetFastTransform( const Matrix4K &a, Vector3KPar o )
{
  SetMultiply(a,o);
}

Vector3K Matrix4K::FastTransform( Vector3KPar o ) const
{
  ASSERT_XMM(this);
  ASSERT_XMM(&o);
  // TODOX360: inline or optimize this function
  return Vector3K(VFastTransform,*this,o);
}

Vector3K Matrix4K::Rotate( Vector3KPar o ) const
{
  ASSERT_XMM(this);
  ASSERT_XMM(&o);

  // TODOX360: 3x dot-product might be faster
  __m128 oX = _mm_set_ps1(o.X());
  __m128 oY = _mm_set_ps1(o.Y());
  __m128 oZ = _mm_set_ps1(o.Z());

  __m128 ret = __vmulfp( oX, _orientation._aside._k );
  ret = __vmaddfp( oY, _orientation._up._k,ret );
  return __vmaddfp( oZ, _orientation._dir._k,ret );
}
#endif

void Vector3K::SetMultiplyLeft( Vector3KPar o, const Matrix3K &a )
{ // vector rotation - only 3x3 matrix is used, translation is ignored
	// u=M*v
	Init();
	float o0=o[0],o1=o[1],o2=o[2];
	for( int i=0; i<3; i++ )
	{
		Set(i)=a.Get(0,i)*o0+a.Get(1,i)*o1+a.Get(2,i)*o2;
	}
}


#define MySqrEps      1.e-2f //#define MySqrEps      1.e-5f // TODOX360: verify where comes lowered precision from
#define MyEps         1.e-2f
#define AbsZero(x)    ((x) < MyEps)
#define AbsSqrZero(x) ((x) < MySqrEps)

Coord Vector3K::SetSolve2x2( Coord a, Coord b, Coord c, Coord d, Coord e, Coord f )
{
  Coord det01 = a * e - b * d;
  Coord det02 = a * f - c * d;
  Coord det12 = b * f - c * e;
  Coord ad01 = fabs(det01);
  Coord ad02 = fabs(det02);
  Coord ad12 = fabs(det12);
  Coord inv;
  if ( ad01 >= ad02 && ad01 >= ad12 )
  {
    if ( AbsZero(ad01) ) return -1.f;
    // result[2] is free variable
    inv = 1.0f / det01;
    Set(0) = -det12 * inv;
    Set(1) =  det02 * inv;
    Set(2) = -1.0f;
    return ad01;
  }
  if ( ad02 >= ad01 && ad02 >= ad12 )
  {
    if ( AbsZero(ad02) ) return -1.f;
    // result[1] is free variable
    inv = 1.0f / det02;
    Set(0) =  det12 * inv;
    Set(1) = -1.0f;
    Set(2) =  det01 * inv;
    return ad02;
  }
  if ( AbsZero(ad12) ) return -1.f;
  // result[0] is free variable
  inv = 1.0f / det12;
  Set(0) = -1.0f;
  Set(1) =  det02 * inv;
  Set(2) = -det01 * inv;
  return ad12;
}

#define IsOne(x)  ( (x) > 1.f-MySqrEps && (x) < 1.f+MySqrEps )

bool Vector3K::AllOnes () const
{
  return( IsOne(Get(0)) &&
    IsOne(Get(1)) &&
    IsOne(Get(2)) );
}

Coord Vector3K::SetEigenVector ( Coord a, Coord b, Coord c, Coord d, Coord e, Coord f )
{
  // try rows 0 and 1:
  Coord reliability = SetSolve2x2(a,b,c,b,d,e);
  if ( reliability > 0.f ) return reliability;
  // try rows 1 and 2:
  return SetSolve2x2(b,d,e,c,e,f);
}

void Matrix4K::SetInvertRotation( const Matrix4K &op )
{
	// matrix inversion is calculated based on these prepositions:
	// matrix is B*A, where A is orthogonal rotation, B is translation
	// inversion of such matrix is Inv(A)*Inv(B)
	// Inv(B) is Transpose(B), Inv(A) is -A
	// if row 3 of op must be (0,0,0,1),
	// size of row(i), i=0...2 must be 1.0
	SetIdentity();
	for( int i=0; i<3; i++ ) //for( int j=0; j<3; j++ )
	{
		Set(i,0)=op.Get(0,i);
		Set(i,1)=op.Get(1,i);
		Set(i,2)=op.Get(2,i);
	}
	Vector3K invTranslate(VRotate,*this,-op.Position());
	SetPosition(invTranslate);
}

void Matrix4K::SetInvertScaled( const Matrix4K &op )
{
	// matrix inversion is calculated based on these prepositions:
	// matrix is S*B*A, where S is scale, A is rotation, B is translation
	// inversion of such matrix is Inv(S)*Inv(A)*Inv(B)
	// Inv(B) is Transpose(B), Inv(A) is -A, Inv(S) is C: C(i,i)=1/S(i,i)
	// row 3 of op must be (0,0,0,1),
	// sizes of row(i) are scale coeficients a,b,c
	SetIdentity();
	// calculate scale
	float invScale0=InvSquareSize(op.Get(0,0),op.Get(0,1),op.Get(0,2));
	float invScale1=InvSquareSize(op.Get(1,0),op.Get(1,1),op.Get(1,2));
	float invScale2=InvSquareSize(op.Get(2,0),op.Get(2,1),op.Get(2,2));
	// invert rotation and scale
	for( int i=0; i<3; i++ )
	{
		Set(i,0)=op.Get(0,i)*invScale0;
		Set(i,1)=op.Get(1,i)*invScale1;
		Set(i,2)=op.Get(2,i)*invScale2;
	}
	
	// invert translation
	Vector3K invTranslate(VRotate,*this,-op.Position());
	SetPosition(invTranslate);
}

void Matrix4K::SetInvertGeneral( const Matrix4K &op )
{
  // invert orientation
  _orientation.SetInvertGeneral(op.Orientation());
  // invert translation
  SetPosition(Rotate(-op.Position()));
}


void Matrix3K::SetNormalTransform( const Matrix3K &op )
{
	// normal transformation for scale matrix (a,b,c) is (1/a,1/b,1/c)
	// all matrices we use are rotation combined with scale
	SetIdentity();
	int j;
	float invRow0size2=InvSquareSize(op.Get(0,0),op.Get(0,1),op.Get(0,2));
	float invRow1size2=InvSquareSize(op.Get(1,0),op.Get(1,1),op.Get(1,2));
	float invRow2size2=InvSquareSize(op.Get(2,0),op.Get(2,1),op.Get(2,2));
	for( j=0; j<3; j++ )
	{
		Set(0,j)=op.Get(0,j)*invRow0size2;
		Set(1,j)=op.Get(1,j)*invRow1size2;
		Set(2,j)=op.Get(2,j)*invRow2size2;
	}
}


// implementation of 3x3 matrices

void Matrix3K::SetIdentity()
{
	*this = M3IdentityK;
}

void Matrix3K::SetZero()
{
	*this = M3ZeroK;
}

void Matrix3K::SetRotationX( Coord angle )
{
	Coord s=sin(angle),c=cos(angle);
	SetIdentity();
	Set(1,1)=+c,Set(1,2)=-s;
	Set(2,1)=+s,Set(2,2)=+c;
}

void Matrix3K::SetRotationY( Coord angle )
{
	Coord s=sin(angle),c=cos(angle);
	SetIdentity();
	Set(0,0)=+c,Set(0,2)=-s;
	Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix3K::SetRotationZ( Coord angle )
{
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	SetIdentity();
	Set(0,0)=+c,Set(0,1)=-s;
	Set(1,0)=+s,Set(1,1)=+c;
}

void Matrix3K::SetRotationAxis(Vector3KPar axis, Coord angle)
{
  // Normalize input vector
  Vector3K axisN = axis;
  axisN.Normalize();

  // Convert axis and angle into a quaternion (w, x, y, z)
  float halfAngle = angle * 0.5f;
  float w = cos(halfAngle);
  float sinHalfAngle = sin(halfAngle);
  float x = sinHalfAngle * axisN.X();
  float y = sinHalfAngle * axisN.Y();
  float z = sinHalfAngle * axisN.Z();

  // Convert the quaternion into the matrix
  float wx = w*x*2;
  float wy = w*y*2;
  float wz = w*z*2;
  float xx = x*x*2;
  float xy = x*y*2;
  float xz = x*z*2;
  float yy = y*y*2;
  float yz = y*z*2;
  float zz = z*z*2;

  Set(0, 0) = 1 - yy - zz;
  Set(0, 1) = xy - wz;
  Set(0, 2) = xz + wy;
  Set(1, 0) = xy + wz;
  Set(1, 1) = 1 - xx - zz;
  Set(1, 2) = yz - wx;
  Set(2, 0) = xz - wy;
  Set(2, 1) = yz + wx;
  Set(2, 2) = 1 - xx - yy;
}

void Matrix3K::SetScale( Coord x, Coord y, Coord z )
{
	SetZero();
	Set(0,0)=x;
	Set(1,1)=y;
	Set(2,2)=z;
	//Set(3,3)=1;
}

void Matrix3K::SetScale( float scale )
{
	// note: old scale may be zero
	float invOldScale=InvScale();
	if( invOldScale>0 )
	{
		//Matrix3K rescale(MScale,scale*invOldScale);
		//(*this)=(*this)*rescale;
		(*this)*=scale*invOldScale;
	}
	else
	{
		SetScale(scale,scale,scale);
	}
	// note: any SetDirection will remove scale
}
/** Calculates estimation of maximal possible scale. 
* The correct  maximal possible scale is equal or lower than this estimation. The values are equal for orthogonal 
* matrices. 
*/
float Matrix3K::MaxScale2() const
{
  /// estimation sqrt(max_{i = 0..2} (|S_i|^2) + 2 * max_{i,j = 0..2, i != j} abs(S_i * S_j)) where S_i is i-th column
  float maxS = _aside.SquareSize(); // maximal column size
  saturateMax(maxS, _dir.SquareSize());
  saturateMax(maxS, _up.SquareSize());

  float maxSS = fabs(_aside * _dir); //maximal in-between column product
  saturateMax(maxSS, fabs(_aside * _up));
  saturateMax(maxSS, fabs(_dir * _up));

  maxS += 2 * maxSS;

  return maxS;
}
/** Calculates average scale. There can exist vectors with bigger scale.*/
float Matrix3K::Scale2() const
{
	// Frame transformation is composed from
	// translation*rotation*scale
	// current scale can be determined as
	// orient * transpose orient
	Matrix3K oTo=(*this)*InverseRotation();
	// note all but [i][i] should be zero
	float sx2=oTo(0,0);
	float sy2=oTo(1,1);
	float sz2=oTo(2,2);
	// calculate average scale
	return (sx2+sy2+sz2)*(1.0/3);
}

void Matrix3K::ScaleAndMaxScale2(float& scale, float& maxScale) const
{
  /// estimation max scale sqrt(max_{i = 0..2} (|S_i|^2) + 2 * max_{i,j = 0..2, i != j} abs(S_i * S_j)) where S_i is i-th column   
  scale = maxScale = _aside.SquareSize(); // maximal column size   
  float temp = _dir.SquareSize();
  saturateMax(maxScale, temp);
  scale += temp;
  temp = _up.SquareSize();
  saturateMax(maxScale, temp);
  scale += temp;
  scale *= 1.0f/3;

  float maxSS = fabs(_aside * _dir); //maximal in-between column product
  saturateMax(maxSS, fabs(_aside * _up));
  saturateMax(maxSS, fabs(_dir * _up));
  maxScale += 2 * maxSS;
}


bool Matrix3K::IsOrthogonal ( Vector3K *sqrSize ) const
{
  float sqAside = _aside.SquareSize();
  float sqUp = _up.SquareSize();
  if (sqAside==0 || sqUp==0) return false;
  if ( sqrSize )
  {
    (*sqrSize)[0] = sqAside;
    (*sqrSize)[1] = sqUp;
    (*sqrSize)[2] = _dir.SquareSize();
  }
  if ( !AbsZero(fabs( _aside * _up  ) * InvSqrt(sqAside)) ||
    !AbsZero(fabs( _aside * _dir ) * InvSqrt(sqAside)) ||
    !AbsZero(fabs( _up    * _dir ) * InvSqrt(sqUp)) )
    return false;
  return true;
}

void Matrix3K::SetDirectionAndUp( Vector3KPar dir, Vector3KPar up )
{
	SetDirection(dir.Normalized());
	// Project into the plane
	Coord t=up*Direction();
	Vector3K temp = up-Direction()*t;
	SetDirectionUp(temp.Normalized());
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3K::SetDirectionAndAside( Vector3KPar dir, Vector3KPar aside )
{
	SetDirection(dir.Normalized());
	// Project into the plane
	Coord t=aside*Direction();
	Vector3K temp = aside-Direction()*t;
	SetDirectionAside(temp.Normalized());
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	SetDirectionUp(Direction().CrossProduct(DirectionAside()));
}

void Matrix3K::SetUpAndAside( Vector3KPar up, Vector3KPar aside )
{
	SetDirectionUp(up.Normalized());
	// Project into the plane
	Coord t=DirectionUp()*aside;
	Vector3K temp = aside-DirectionUp()*t;
	SetDirectionAside(temp.Normalized());
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	SetDirection(DirectionAside().CrossProduct(DirectionUp()));
}

void Matrix3K::SetUpAndDirection( Vector3KPar up, Vector3KPar dir )
{
	SetDirectionUp(up.Normalized());
	// Project into the plane
	Coord t=DirectionUp()*dir;
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	Vector3K temp = dir-DirectionUp()*t;
	SetDirection(temp.Normalized());
	SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}


void Matrix3K::SetTilda( Vector3KPar a )
{
	SetZero();
	Set(0,1)=-a[2],Set(0,2)=+a[1];
	Set(1,0)=+a[2],Set(1,2)=-a[0];
	Set(2,0)=-a[1],Set(2,1)=+a[0];
	//Assert( ((*this)*a).SquareSize()<1e-10 );
}


void Matrix3K::operator *= ( float op )
{
	_aside*=op;
	_up*=op;
	_dir*=op;
}


void Matrix3K::SetMultiply( const Matrix3K &a, const Matrix3K &b )
{
	_aside =
	(
		a._aside * b._aside.X() +
		a._up    * b._aside.Y() +
		a._dir   * b._aside.Z()
	);
	_up =
	(
		a._aside * b._up.X() +
		a._up    * b._up.Y() +
		a._dir   * b._up.Z()
	);
	_dir =
	(
		a._aside * b._dir.X() +
		a._up    * b._dir.Y() +
		a._dir   * b._dir.Z()
	);
	/*
	// matrix multiplication


	for( int i=0; i<3; i++ ) for( int j=0; j<3; j++ )
	{
		Set(i,j)=
		(
			a.Get(i,0)*b.Get(0,j)+
			a.Get(i,1)*b.Get(1,j)+
			a.Get(i,2)*b.Get(2,j)
		);
	}
	*/
}

void Matrix3K::SetMultiply( const Matrix3K &a, FloatQuadPar f )
{
	// matrix multiplication
	_aside=a._aside*f;
	_up=a._up*f;
	_dir=a._dir*f;
}

Matrix3K &Matrix3K::operator += ( const Matrix3K &a )
{
	_aside += a._aside;
	_up += a._up;
	_dir += a._dir;
	return *this;
}

void Matrix4K::operator += ( const Matrix4K &a )
{
	_orientation._aside += a._orientation._aside;
	_orientation._up += a._orientation._up;
	_orientation._dir += a._orientation._dir;
	_position += a._position;
}

void Matrix4K::operator -= ( const Matrix4K &a )
{
	_orientation._aside -= a._orientation._aside;
	_orientation._up -= a._orientation._up;
	_orientation._dir -= a._orientation._dir;
	_position -= a._position;
}

Matrix3K Matrix3K::operator + ( const Matrix3K &a ) const
{
	Matrix3K res;
	res._aside=_aside+a._aside;
	res._up=_up+a._up;
	res._dir=_dir+a._dir;
	return res;
}

Matrix4K Matrix4K::operator + ( const Matrix4K &a ) const
{
	Matrix4K res;
	res._orientation._aside=_orientation._aside+a._orientation._aside;
	res._orientation._up=_orientation._up+a._orientation._up;
	res._orientation._dir=_orientation._dir+a._orientation._dir;
	res._position=_position+a._position;
	return res;
}

Matrix4K Matrix4K::operator - ( const Matrix4K &a ) const
{
	Matrix4K res;
	res._orientation._aside=_orientation._aside-a._orientation._aside;
	res._orientation._up=_orientation._up-a._orientation._up;
	res._orientation._dir=_orientation._dir-a._orientation._dir;
	res._position=_position-a._position;
	return res;
}

Matrix3K Matrix3K::operator - ( const Matrix3K &a ) const
{
	Matrix3K res;
	res._aside=_aside-a._aside;
	res._up=_up-a._up;
	res._dir=_dir-a._dir;
	return res;
}

Matrix3K &Matrix3K::operator -= ( const Matrix3K &a )
{
	_aside-=a._aside;
	_up-=a._up;
	_dir-=a._dir;
	return *this;
}


void Matrix3K::SetInvertRotation( const Matrix3K &op )
{
#if 0 // _DEBUG // TODOX360: check what hapenned to animations precision
  Vector3K sizes;
  Assert(op.IsOrthogonal(&sizes) && AbsSqrZero(fabs(sizes[0] - 1)) && AbsSqrZero(fabs(sizes[1] - 1)) && AbsSqrZero(fabs(sizes[2] - 1)) );
#endif
	// matrix inversion is calculated based on these prepositions:
	SetIdentity();
	for( int i=0; i<3; i++ ) //for( int j=0; j<3; j++ )
	{
		Set(i,0)=op.Get(0,i);
		Set(i,1)=op.Get(1,i);
		Set(i,2)=op.Get(2,i);
	}
}

void Matrix3K::SetInvertScaled( const Matrix3K &op )
{
	// matrix inversion is calculated based on these prepositions:
	// matrix is S*A, where S is scale, A is rotation
	// inversion of such matrix is Inv(S)*Inv(A)
	// Inv(A) is Transpose(A), Inv(S) is C: C(i,i)=1/S(i,i)
	// sizes of row(i) are scale coefficients a,b,c
  Assert(op != M3ZeroK);
	SetIdentity();
	// calculate scale
	float invScale0=InvSquareSize(op.Get(0,0),op.Get(0,1),op.Get(0,2));
	float invScale1=InvSquareSize(op.Get(1,0),op.Get(1,1),op.Get(1,2));
	float invScale2=InvSquareSize(op.Get(2,0),op.Get(2,1),op.Get(2,2));
	// invert rotation and scale
	for( int i=0; i<3; i++ ) //for( int j=0; j<3; j++ )
	{
		Set(i,0)=op.Get(0,i)*invScale0;
		Set(i,1)=op.Get(1,i)*invScale1;
		Set(i,2)=op.Get(2,i)*invScale2;
	}
}

#define Swap(a,b) {float p;p=a;a=b;b=p;}

void Matrix3K::SetInvertGeneral( const Matrix3K &op )
{
  Assert(op != M3ZeroK);
	// calculate inversion using Gauss-Jordan elimination
	Matrix3K a=op;
	// load result with identity
	SetIdentity();
	int row,col;
	// construct result by pivoting
	// pivot column
	for( col=0; col<3; col++ )
	{
		// use maximal number as pivot
		float max=0;
		int maxRow=col;
		for( row=col; row<3; row++ )
		{
			float mag=fabs(a.Get(row,col));
			if( max<mag ) max=mag,maxRow=row;
		}
		if( max<=0.0 ) continue; // no pivot exists
		// swap lines col and maxRow
		Swap(a.Set(col,0),a.Set(maxRow,0));
		Swap(a.Set(col,1),a.Set(maxRow,1));
		Swap(a.Set(col,2),a.Set(maxRow,2));
		Swap(Set(col,0),Set(maxRow,0));
		Swap(Set(col,1),Set(maxRow,1));
		Swap(Set(col,2),Set(maxRow,2));
		// use a(col,col) as pivot
		float quotient=1/a.Get(col,col);
		// make pivot 1
		a.Set(col,0)*=quotient,a.Set(col,1)*=quotient,a.Set(col,2)*=quotient;
		Set(col,0)*=quotient,Set(col,1)*=quotient,Set(col,2)*=quotient;
		// use pivot line to zero all other lines
		for( row=0; row<3; row++ ) if( row!=col )
		{
			float factor=a.Get(row,col);
			a.Set(row,0)-=a.Get(col,0)*factor;
			a.Set(row,1)-=a.Get(col,1)*factor;
			a.Set(row,2)-=a.Get(col,2)*factor;
			Set(row,0)-=Get(col,0)*factor;
			Set(row,1)-=Get(col,1)*factor;
			Set(row,2)-=Get(col,2)*factor;
		}
	}
	// result constructed
}

// math for PCA:

void Matrix3K::SetCovarianceCenter ( const AutoArray<Vector3K> &data, const Matrix4K *m )
{
  Coord sx  = 0.0f;
  Coord sy  = 0.0f;
  Coord sz  = 0.0f;
  Coord sxx = 0.0f;
  Coord sxy = 0.0f;
  Coord sxz = 0.0f;
  Coord syy = 0.0f;
  Coord syz = 0.0f;
  Coord szz = 0.0f;
  int i;
  int len = data.Size();
  if ( len < 3 )
  {
    SetIdentity();
    return;
  }
  for ( i = 0; i < len; i++ )
  {
    Vector3K v;
    if ( m )
      v.SetMultiply(*m,data[i]);
    else
      v = data[i];
    sx  += v[0];
    sy  += v[1];
    sz  += v[2];
    sxx += v[0] * v[0];
    sxy += v[0] * v[1];
    sxz += v[0] * v[2];
    syy += v[1] * v[1];
    syz += v[1] * v[2];
    szz += v[2] * v[2];
  }
  Coord inv = 1.0f / len;
  Set(0,0) =            (sxx - inv * sx * sx) * inv;
  Set(0,1) = Set(1,0) = (sxy - inv * sx * sy) * inv;
  Set(0,2) = Set(2,0) = (sxz - inv * sx * sz) * inv;
  Set(1,1) =            (syy - inv * sy * sy) * inv;
  Set(1,2) = Set(2,1) = (syz - inv * sy * sz) * inv;
  Set(2,2) =            (szz - inv * sz * sz) * inv;
}

#define Pi3   (H_PI/3.0)
#define Delta coord(2.0*Pi3)
#define Inv3  (1.0f/3.0f)
#define Inv6  (1.0f/6.0f)
#define Inv27 (1.0f/27.0f)

bool Matrix3K::SetEigenStandard ( const Matrix3K &cov, Coord *eig )
{
  Coord eigS[3];
  if ( !eig ) eig = eigS;
  // assuming "cov" to be symmetric matrix:
  Coord a = cov.Get(0,0);
  Coord b = cov.Get(0,1);
  Coord c = cov.Get(0,2);
  Coord d = cov.Get(1,1);
  Coord e = cov.Get(1,2);
  Coord f = cov.Get(2,2);
  Coord bb = b * b;
  Coord cc = c * c;
  Coord ee = e * e;
  // characteristic polynomial: L^3 + a1 * L^2 + a2 * L + a3
  Coord a1 = - a - d - f;
  Coord a2 = d*f + a*f + a*d - bb - cc - ee;
  Coord a3 = a * ee + f * bb + d * cc - a * d * f - 2.0f * b * c * e;
  Coord a12 = a1 * a1;
  Coord q = (a12 * Inv3 - a2) * Inv3;
  Coord r = a1 * (a2 * Inv6 - a12 * Inv27) - 0.5f * a3;
  Coord sqrtq = coord(sqrt(q));
  Coord t;
  if ( q * q * q < r * r )
  {
    t = (r > 0.0f) ? 0.0f : coord(Pi3);
  }
  else
  {
    Coord cost = r / (q * sqrtq);
    saturate(cost, -1.0f, 1.0f); // cost might be of value -1.0000001
    t = coord(acos(cost)) * Inv3;
  }
  Coord a13 = - a1 * Inv3;
  sqrtq += sqrtq;
  eig[0] = sqrtq * coord(cos(t)) + a13;
  // classics:
  //eig[1] = sqrtq * coord(cos(t += Delta)) + a13;
  //eig[2] = sqrtq * coord(cos(t + Delta)) + a13;
  // Vieto's formulae:
  eig[1] = sqrtq * coord(cos(t + Delta)) + a13;
  eig[2] = - a1 - eig[0] - eig[1];

  // eigenvectors:
  Vector3K v[3];
  Coord    rel[3];
  int i;
  for ( i = 0; i < 3; i++ )
    rel[i] = v[i].SetEigenVector(a-eig[i],b,c,d-eig[i],e,f-eig[i]);
  int p0, p1;                               // p0-th will be the most reliable vector
  if ( rel[0] > rel[1] )                    // p1-th the second most reliable one..
  {
    p0 = 0;
    p1 = 1;
  }
  else
  {
    p0 = 1;
    p1 = 0;
  }
  if ( rel[p1] < rel[2] )
    if ( rel[p0] < rel[2] )
    {
      p1 = p0;
      p0 = 2;
    }
    else
      p1 = 2;

  // the most reliable eigenvector:
  if ( rel[p0] < 0.f )
  {
    _aside[0] = 1.f;
    _aside[1] = _aside[2] = 0.f;
  }
  else
  {
    _aside = v[p0];
    _aside.Normalize();
  }

  // the second one:
  if ( rel[p1] < 0.f )
  {
    _up[0] = _aside[1];
    _up[1] = _aside[2];
    _up[2] = _aside[0];
  }
  else
    _up = v[p1];
  _up = _aside.CrossProduct(_up);
  _up.Normalize();

  // the third one:
  _dir = _aside.CrossProduct(_up);          // implicit unit length

  return true;
}

float Matrix3K::Determinant() const
{
  return (
    +Get(0,0)*Get(1,1)*Get(2,2)
    +Get(1,0)*Get(2,1)*Get(0,2)
    +Get(0,1)*Get(2,0)*Get(1,2)
    -Get(0,0)*Get(2,1)*Get(1,2)    
    -Get(1,1)*Get(2,0)*Get(0,2)
    -Get(2,2)*Get(0,1)*Get(1,0)
  );
}

Matrix4P ConvertKToP( const Matrix4K &m )
{
	Matrix4P ret;
	Vector3P pos(m.Position()[0],m.Position()[1],m.Position()[2]);
	Vector3P aside(m.DirectionAside()[0],m.DirectionAside()[1],m.DirectionAside()[2]);
	Vector3P up(m.DirectionUp()[0],m.DirectionUp()[1],m.DirectionUp()[2]);
	Vector3P dir(m.Direction()[0],m.Direction()[1],m.Direction()[2]);
	ret.SetPosition(pos);
	ret.SetDirectionAside(aside);
	ret.SetDirectionUp(up);
	ret.SetDirection(dir);
	return ret;
}
Matrix4K ConvertPToK( const Matrix4P &m )
{
	Matrix4K ret;
	Vector3K pos(m.Position()[0],m.Position()[1],m.Position()[2]);
	Vector3K aside(m.DirectionAside()[0],m.DirectionAside()[1],m.DirectionAside()[2]);
	Vector3K up(m.DirectionUp()[0],m.DirectionUp()[1],m.DirectionUp()[2]);
	Vector3K dir(m.Direction()[0],m.Direction()[1],m.Direction()[2]);
	ret.SetPosition(pos);
	ret.SetDirectionAside(aside);
	ret.SetDirectionUp(up);
	ret.SetDirection(dir);
	return ret;
}

Matrix3P ConvertKToP( const Matrix3K &m )
{
	Matrix3P ret;
	Vector3P aside(m.DirectionAside()[0],m.DirectionAside()[1],m.DirectionAside()[2]);
	Vector3P up(m.DirectionUp()[0],m.DirectionUp()[1],m.DirectionUp()[2]);
	Vector3P dir(m.Direction()[0],m.Direction()[1],m.Direction()[2]);
	ret.SetDirectionAside(aside);
	ret.SetDirectionUp(up);
	ret.SetDirection(dir);
	return ret;
}
Matrix3K ConvertPToK( const Matrix3P &m )
{
	Matrix3K ret;
	Vector3K aside(m.DirectionAside()[0],m.DirectionAside()[1],m.DirectionAside()[2]);
	Vector3K up(m.DirectionUp()[0],m.DirectionUp()[1],m.DirectionUp()[2]);
	Vector3K dir(m.Direction()[0],m.Direction()[1],m.Direction()[2]);
	ret.SetDirectionAside(aside);
	ret.SetDirectionUp(up);
	ret.SetDirection(dir);
	return ret;
}

#endif