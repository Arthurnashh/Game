// homogenous vector and matrix arithmetics
// (C) 1997, SUMA
#include <El/elementpch.hpp>

#ifdef _T_MATH

#pragma optimize ("t",on)

#include "math3dT.hpp"
//#include "vecTempl.hpp"


#include <Es/Framework/debugLog.hpp>

const Matrix3T M3ZeroT
(
	0,0,0,
	0,0,0,
	0,0,0
);
const Matrix3T M3IdentityT
(
	1,0,0,
	0,1,0,
	0,0,1
);

const Matrix4T M4ZeroT
(
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0
);
const Matrix4T M4IdentityT
(
	1,0,0,0,
	0,1,0,0,
	0,0,1,0
);

const Vector3T VZeroT(0,0,0);
const Vector3T VUpT(0,1,0);
const Vector3T VForwardT(0,0,1);
const Vector3T VAsideT(1,0,0);


#if _MSC_VER
bool Matrix4T::IsFinite() const
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
bool Matrix3T::IsFinite() const
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

float Matrix4T::Characteristic() const
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

void Matrix4T::SetIdentity()
{
	*this=M4IdentityT;
}

void Matrix4T::SetZero()
{
	*this=M4ZeroT;
}

void Matrix4T::SetTranslation( Vector3TPar offset )
{
	SetIdentity();
	SetPosition(offset);
}

void Matrix4T::SetRotationX( Coord angle )
{
	SetIdentity();
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	Set(1,1)=+c,Set(1,2)=-s;
	Set(2,1)=+s,Set(2,2)=+c;
}

void Matrix4T::SetRotationY( Coord angle )
{
	SetIdentity();
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	Set(0,0)=+c,Set(0,2)=-s;
	Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix4T::SetRotationZ( Coord angle )
{
	SetIdentity();
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	Set(0,0)=+c,Set(0,1)=-s;
	Set(1,0)=+s,Set(1,1)=+c;
}

void Matrix4T::SetScale( Coord x, Coord y, Coord z )
{
	SetZero();
	Set(0,0)=x;
	Set(1,1)=y;
	Set(2,2)=z;
	//Set(3,3)=1;
}

void Matrix4T::SetPerspective( Coord cLeft, Coord cTop )
{
	SetZero();
	// xg=x*near/right :: <-1,+1>
	Set(0,0)=1.0f/cLeft;
	// yg=y*near/top :: <-1,+1>
	Set(1,1)=1.0f/cTop;
	
	Set(2,2)=1.0; // DirectX has Q here, Q = zFar/(zFar-zNear)

	// zg=-w*1
	SetPos(2)=-1; // DirectX has -Q/zNear here
	
	// wg=z
	//Set(3,2)=1; 
	
	// this gives actual z result (suppose w=1) = zg/wg =
	// zRes(z)=-1/z;
}

void Matrix4T::Orthogonalize()
{
	Vector3TVal dir=Direction();
	Vector3TVal up=DirectionUp();
	SetDirectionAndUp(dir,up);
}
void Matrix3T::Orthogonalize()
{
	Vector3TVal dir=Direction();
	Vector3TVal up=DirectionUp();
	SetDirectionAndUp(dir,up);
}

void Matrix4T::SetOriented( Vector3TPar dir, Vector3TPar up )
{
	SetIdentity();
	SetDirectionAndUp(dir,up);
}

void Matrix4T::SetView( Vector3TPar point, Vector3TPar dir, Vector3TPar up )
{
	Matrix4T translate(MTranslation,-point);
	Matrix4T direction(MDirection,dir,up);
	SetMultiply(direction,translate);
}


void Matrix4T::SetAdd( const Matrix4T &a, const Matrix4T &b )
{
	_orientation._aside = a._orientation._aside+b._orientation._aside;
	_orientation._up = a._orientation._up+b._orientation._up;
	_orientation._dir = a._orientation._dir+b._orientation._dir;
	_position = a._position+b._position;
}

void Matrix4T::SetMultiply( const Matrix4T &a, const Matrix4T &b )
{
	// matrix multiplication
	int i,j;
	// b(3,0)=0, b(3,1)=0, b(3,2)=0, b(3,3)=1
	// a(3,0)=0, a(3,1)=0, a(3,2)=0, a(3,3)=1
	for( i=0; i<3; i++ ) for( j=0; j<3; j++ )
	{
		Set(i,j)=
		(
			a.Get(i,0)*b.Get(0,j)+
			a.Get(i,1)*b.Get(1,j)+
			a.Get(i,2)*b.Get(2,j)
		);
	}
	for( i=0; i<3; i++ )
	{
		SetPos(i)=
		(
			a.Get(i,0)*b.GetPos(0)+
			a.Get(i,1)*b.GetPos(1)+
			a.Get(i,2)*b.GetPos(2)+
			a.GetPos(i)
		);
	}
	//for( j=0; j<3; j++ ) Set(3,j)=0;
	//Set(3,3)=1;
}

void Matrix4T::SetMultiply( const Matrix4T &a, float b )
{
	_orientation._aside = a._orientation._aside*b;
	_orientation._up = a._orientation._up*b;
	_orientation._dir = a._orientation._dir*b;
	_position = a._position*b;
}

void Matrix4T::AddMultiply( const Matrix4T &a, float b )
{
	_orientation._aside += a._orientation._aside*b;
	_orientation._up += a._orientation._up*b;
	_orientation._dir += a._orientation._dir*b;
	_position += a._position*b;
}

void Matrix4T::SetMultiplyByPerspective( const Matrix4T &a, const Matrix4T &b )
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

inline float InvSquareSize( float x, float y, float z )
{
	return Inv(x*x+y*y+z*z);
}

Vector3T Vector3T::Normalized() const
{
	Coord size2=SquareSizeInline();
	if( size2==0 ) return *this;
	Coord invSize=InvSqrt(size2);
	return Vector3T(X()*invSize,Y()*invSize,Z()*invSize);
}

void Vector3T::Normalize() // no return to avoid using instead of Normalized
{
	Coord size2=SquareSizeInline();
	if( size2==0 ) return;
	Coord invSize=InvSqrt(size2);
	Set(0)*=invSize,Set(1)*=invSize,Set(2)*=invSize;
}


Vector3T Vector3T::CrossProduct( Vector3TPar op ) const
{
	//Vector3T ret;
	float x=Y()*op.Z()-Z()*op.Y();
	float y=Z()*op.X()-X()*op.Z();
	float z=X()*op.Y()-Y()*op.X();
	return Vector3T(x,y,z);
}

float Vector3T::CosAngle( Vector3TPar op ) const
{
	return DotProduct(op)*InvSqrt(op.SquareSizeInline()*SquareSizeInline());
}


float Vector3T::Distance( Vector3TPar op ) const
{
	return (*this-op).Size();
}

float Vector3T::DistanceXZ( Vector3TPar op ) const
{
	return (*this-op).SizeXZ();
}
float Vector3T::DistanceXZ2( Vector3TPar op ) const
{
	return (*this-op).SquareSizeXZ();
}

Vector3T Vector3T::Project( Vector3TPar op ) const
{
	return op*DotProduct(op)*op.InvSquareSize();
}

bool VerifyFloat( float x );

#if _MSC_VER
bool Vector3T::IsFinite() const
{
	if( !VerifyFloat(Get(0)) ) return false;
	if( !VerifyFloat(Get(1)) ) return false;
	if( !VerifyFloat(Get(2)) ) return false;
	//if( !_finite(Get(0)) ) return false;
	//if( !_finite(Get(1)) ) return false;
	//if( !_finite(Get(2)) ) return false;
	return true;
}
#endif

#define NO_ASM 0
#define ASM_VC5 1
#define ASM_ICL45 2

#if !_PIII // special optimization for PIII

// no handoptimized assembly - use C version
// VC++6 - 64 cycles per iteration

void Vector3T::SetFastTransform( const Matrix4T &a, Vector3TPar o )
{
	float r0=a.Get(0,0)*o[0]+a.Get(0,1)*o[1]+a.Get(0,2)*o[2]+a.GetPos(0);
	float r1=a.Get(1,0)*o[0]+a.Get(1,1)*o[1]+a.Get(1,2)*o[2]+a.GetPos(1);
	float r2=a.Get(2,0)*o[0]+a.Get(2,1)*o[1]+a.Get(2,2)*o[2]+a.GetPos(2);
	Set(0)=r0;
	Set(1)=r1;
	Set(2)=r2;
}

#endif

void Vector3T::SetMultiplyLeft( Vector3TPar o, const Matrix3T &a )
{ // vector rotation - only 3x3 matrix is used, translation is ignored
	// u=M*v
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

Coord Vector3T::SetSolve2x2( Coord a, Coord b, Coord c, Coord d, Coord e, Coord f )
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

bool Vector3T::AllOnes () const
{
  return( IsOne(Get(0)) &&
    IsOne(Get(1)) &&
    IsOne(Get(2)) );
}

Coord Vector3T::SetEigenVector ( Coord a, Coord b, Coord c, Coord d, Coord e, Coord f )
{
  // try rows 0 and 1:
  Coord reliability = SetSolve2x2(a,b,c,b,d,e);
  if ( reliability > 0.f ) return reliability;
  // try rows 1 and 2:
  return SetSolve2x2(b,d,e,c,e,f);
}

void Matrix4T::SetInvertRotation( const Matrix4T &op )
{
	// invert orientation
	_orientation.SetInvertRotation(op.Orientation());
	// invert translation
	SetPosition(Rotate(-op.Position()));
}

void Matrix4T::SetInvertScaled( const Matrix4T &op )
{
	// invert orientation
	_orientation.SetInvertScaled(op.Orientation());
	// invert translation
	Vector3T pos;
	pos.SetMultiply(Orientation(),-op.Position());
	SetPosition(pos);
}

void Matrix4T::SetInvertGeneral( const Matrix4T &op )
{
	// invert orientation
	_orientation.SetInvertGeneral(op.Orientation());
	// invert translation
	SetPosition(Rotate(-op.Position()));
}

void Matrix3T::SetNormalTransform( const Matrix3T &op )
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

void Matrix3T::SetIdentity()
{
	*this=M3IdentityT;
}

void Matrix3T::SetZero()
{
	*this=M3ZeroT;
}

void Matrix3T::SetRotationX( Coord angle )
{
	Coord s=sin(angle),c=cos(angle);
	SetIdentity();
	Set(1,1)=+c,Set(1,2)=-s;
	Set(2,1)=+s,Set(2,2)=+c;
}

void Matrix3T::SetRotationY( Coord angle )
{
	Coord s=sin(angle),c=cos(angle);
	SetIdentity();
	Set(0,0)=+c,Set(0,2)=-s;
	Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix3T::SetRotationZ( Coord angle )
{
	Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
	SetIdentity();
	Set(0,0)=+c,Set(0,1)=-s;
	Set(1,0)=+s,Set(1,1)=+c;
}

void Matrix3T::SetRotationAxis(Vector3TPar axis, Coord angle)
{
  // Normalize input vector
  Vector3T axisN = axis;
  axisN.Normalize();
  
  // Convert axis and angle into a quaternion (w, x, y, z)
  float halfAngle = angle * 0.5f;
  float w = cos(halfAngle);
  float sinHalfAngle = sin(halfAngle);
  float x = sinHalfAngle * axisN.X();
  float y = sinHalfAngle * axisN.Y();
  float z = sinHalfAngle * axisN.Z();

  // Convert the quaternion into the matrix
  float wx = w*x*2.0f;
  float wy = w*y*2.0f;
  float wz = w*z*2.0f;
  float xx = x*x*2.0f;
  float xy = x*y*2.0f;
  float xz = x*z*2.0f;
  float yy = y*y*2.0f;
  float yz = y*z*2.0f;
  float zz = z*z*2.0f;

  Set(0, 0) = 1.0f - yy - zz;
  Set(0, 1) = xy - wz;
  Set(0, 2) = xz + wy;
  Set(1, 0) = xy + wz;
  Set(1, 1) = 1.0f - xx - zz;
  Set(1, 2) = yz - wx;
  Set(2, 0) = xz - wy;
  Set(2, 1) = yz + wx;
  Set(2, 2) = 1.0f - xx - yy;
}

void Matrix3T::SetScale( Coord x, Coord y, Coord z )
{
	SetZero();
	Set(0,0)=x;
	Set(1,1)=y;
	Set(2,2)=z;
	//Set(3,3)=1;
}

void Matrix3T::SetScale( float scale )
{
	// note: old scale may be zero
	float invOldScale=InvScale();
	if( invOldScale>0 )
	{
		//Matrix3T rescale(MScale,scale*invOldScale);
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
float Matrix3T::MaxScale2() const
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
float Matrix3T::Scale2() const
{
	// Frame transformation is composed from
	// rotation*scale
	// current scale can be determined as
	// orient * transpose orient

	//Matrix3T invR=InverseRotation();
	//Matrix3T oTo=(*this)*invR;
	// note all but [i][i] should be zero
	//float sx2=oTo(0,0);
	//float sy2=oTo(1,1);
	//float sz2=oTo(2,2);

	// optimized matrix transposition + multiplication
	Vector3T sv;
	for( int i=0; i<3; i++ )
	{
		sv[i]=Square(Get(i,0))+Square(Get(i,1))+Square(Get(i,2));
	}

	// calculate average scale
	float scale2=(sv[0]+sv[1]+sv[2])*(1.0/3);
	return scale2;
}

void Matrix3T::ScaleAndMaxScale2(float& scale, float& maxScale) const
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


void Matrix3T::SetDirectionAndUp( Vector3TPar dir, Vector3TPar up )
{
	SetDirection(dir.Normalized());
	// Project into the plane
	Coord t=up*Direction();
	Vector3TVal norm = (up-Direction()*t);
	SetDirectionUp(norm.Normalized());
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3T::SetDirectionAndAside( Vector3TPar dir, Vector3TPar aside )
{
	SetDirection(dir.Normalized());
	// Project into the plane
	Coord t=aside*Direction();
	Vector3TVal norm = (aside-Direction()*t);
	SetDirectionAside(norm.Normalized());
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	SetDirectionUp(Direction().CrossProduct(DirectionAside()));
}

void Matrix3T::SetUpAndAside( Vector3TPar up, Vector3TPar aside )
{
	SetDirectionUp(up.Normalized());
	// Project into the plane
	Coord t=DirectionUp()*aside;
	Vector3TVal norm = (aside-DirectionUp()*t);
	SetDirectionAside(norm.Normalized());
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	SetDirection(DirectionAside().CrossProduct(DirectionUp()));
}

void Matrix3T::SetUpAndDirection( Vector3TPar up, Vector3TPar dir )
{
	SetDirectionUp(up.Normalized());
	// Project into the plane
	Coord t=DirectionUp()*dir;
	// Calculate the vector pointing along the x axis (i.e. aside)
	// sign is experimental - I never know what is right and what is left
	// no need to normalize - cross product of two perpendicular unit vectors is unit vector
	Vector3TVal norm = (dir-DirectionUp()*t);
	SetDirection(norm.Normalized());
	SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}


void Matrix3T::SetTilda( Vector3TPar a )
{
	SetZero();
	Set(0,1)=-a[2],Set(0,2)=+a[1];
	Set(1,0)=+a[2],Set(1,2)=-a[0];
	Set(2,0)=-a[1],Set(2,1)=+a[0];
}

void Matrix3T::operator *= ( float op )
{
	_aside*=op;
	_up*=op;
	_dir*=op;
}


void Matrix3T::SetAdd( const Matrix3T &a, const Matrix3T &b )
{
	_aside = a._aside+b._aside;
	_up = a._up+b._up;
	_dir = a._dir+b._dir;
}

void Matrix3T::SetMultiply( const Matrix3T &a, const Matrix3T &b )
{
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
}

void Matrix3T::SetMultiply( const Matrix3T &a, float op )
{
	// matrix multiplication
	_aside = a._aside*op;
	_up = a._up*op;
	_dir = a._dir*op;
}

void Matrix3T::AddMultiply( const Matrix3T &a, float op )
{
	// matrix multiplication
	_aside += a._aside*op;
	_up += a._up*op;
	_dir += a._dir*op;
}

Matrix3T Matrix3T::operator + ( const Matrix3T &a ) const
{
	Matrix3T res;
	res._aside=_aside+a._aside;
	res._up=_up+a._up;
	res._dir=_dir+a._dir;
	return res;
}

Matrix3T Matrix3T::operator - ( const Matrix3T &a ) const
{
	Matrix3T res;
	res._aside=_aside-a._aside;
	res._up=_up-a._up;
	res._dir=_dir-a._dir;
	return res;
}

Matrix3T &Matrix3T::operator -= ( const Matrix3T &a )
{
	_aside-=a._aside;
	_up-=a._up;
	_dir-=a._dir;
	return *this;
}

Matrix3T &Matrix3T::operator += ( const Matrix3T &a )
{
	_aside+=a._aside;
	_up+=a._up;
	_dir+=a._dir;
	return *this;
}


// math for PCA:

void Matrix3T::SetCovarianceCenter ( const AutoArray<Vector3T> &data, const Matrix4T *m )
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
    Vector3T v;
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

bool Matrix3T::SetEigenStandard ( Matrix3TPar cov, Coord *eig )
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
  Vector3T v[3];
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

float Matrix3T::Determinant() const
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


bool Matrix3T::IsOrthogonal ( Vector3T *sqrSize ) const
{
  if ( !AbsSqrZero(fabs( _aside * _up  ) / _aside.Size()) ||
    !AbsSqrZero(fabs( _aside * _dir ) / _aside.Size()) ||
    !AbsSqrZero(fabs( _up    * _dir ) / _up.Size()) )
    return false;
  if ( sqrSize )
  {
    (*sqrSize)[0] = _aside.SquareSize();
    (*sqrSize)[1] =    _up.SquareSize();
    (*sqrSize)[2] =   _dir.SquareSize();
  }
  return true;
}

Matrix4T Matrix4T::operator + ( const Matrix4T &a ) const
{
	Matrix4T res;
	res._orientation._aside=_orientation._aside+a._orientation._aside;
	res._orientation._up=_orientation._up+a._orientation._up;
	res._orientation._dir=_orientation._dir+a._orientation._dir;
	res._position=_position+a._position;
	return res;
}

Matrix4T Matrix4T::operator - ( const Matrix4T &a ) const
{
	Matrix4T res;
	res._orientation._aside=_orientation._aside-a._orientation._aside;
	res._orientation._up=_orientation._up-a._orientation._up;
	res._orientation._dir=_orientation._dir-a._orientation._dir;
	res._position=_position-a._position;
	return res;
}

void Matrix4T::operator += ( const Matrix4T &op )
{
	_orientation._aside+=op._orientation._aside;
	_orientation._up+=op._orientation._up;
	_orientation._dir+=op._orientation._dir;
	_position+=op._position;
}
void Matrix4T::operator -= ( const Matrix4T &op )
{
	_orientation._aside-=op._orientation._aside;
	_orientation._up-=op._orientation._up;
	_orientation._dir-=op._orientation._dir;
	_position-=op._position;
}


void Matrix3T::SetInvertRotation( const Matrix3T &op )
{
#if _DEBUG
  Vector3T sizes;
  Assert(op.IsOrthogonal(&sizes) && AbsSqrZero(fabs(sizes[0] - 1)) && AbsSqrZero(fabs(sizes[1] - 1)) && AbsSqrZero(fabs(sizes[2] - 1)));
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

//#define FailF(format,args) DebF("%s(%d) : Format",__FILE__,__LINE__,args)

void Matrix3T::SetInvertScaled( const Matrix3T &op )
{
	#if 0
		// TODO: correct bug
    {
		// verify matrix is S*R
		// scaled matrix has form S*R, where S is scale matrix and R is rotation
		// (S*R)*(RT*ST)=S*ST=S*S
		Matrix3T t;
		for( int i=0; i<3; i++ )
		{
			t(0,i)=op(i,0),t(1,i)=op(i,1),t(2,i)=op(i,2);
		}
		Matrix3TVal se=op*t;
		// check if se is diagonal
		const float max=1e-4;
		float maxNSR=0;
		saturateMax(maxNSR,fabs(se(0,1)));
		saturateMax(maxNSR,fabs(se(0,2)));
		saturateMax(maxNSR,fabs(se(1,0)));
		saturateMax(maxNSR,fabs(se(1,2)));
		saturateMax(maxNSR,fabs(se(2,0)));
		saturateMax(maxNSR,fabs(se(2,1)));
		if( maxNSR>max )
		{
			LogF("%s(%d) : %f",__FILE__,__LINE__,maxNSR);
		}
    }
	#endif
	// matrix inversion is calculated based on these prepositions:
	// matrix is S*R, where S is scale, R is rotation
	// inversion of such matrix is Inv(S)*Inv(R)
	// Inv(R) is Transpose(R), Inv(S) is C: C(i,i)=1/S(i,i)
	// sizes of row(i) are scale coeficients a,b,c
	// all member are set
#if _DEBUG
  //Assert(op != M3ZeroT);
#endif
	//SetIdentity();
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

void Matrix3T::SetInvertGeneral( const Matrix3T &op )
{
  //Assert(op != M3ZeroT);
	#if 1
		// TODO: detect general matrices as soon as possible
		// check if they are really necessary
		// check if matrix is really general
		// if not, we can use scaled version
		// scaled matrix has form S*R, where S is scale matrix and R is rotation
		// (S*R)*(RT*ST)=S*ST=S*S
		// 
		Matrix3T t;
		for( int i=0; i<3; i++ )
		{
			t(0,i)=op(i,0),t(1,i)=op(i,1),t(2,i)=op(i,2);
		}
		Matrix3TVal se=op*t;
		// check if se is diagonal
		#if 0
		static int diag=0;
		static int unit=0;
		static int total=0;
		if( total>1000 )
		{
			LogF("G Diagonal: %.1f %% matrices",diag*100.0/total);
			total=0;
			diag=0;
		}
		total++;
		#endif
		const float max=1e-6;
		if
		(
			fabs(se(0,1))<max && fabs(se(0,2))<max &&
			fabs(se(1,0))<max && fabs(se(1,2))<max &&
			fabs(se(2,0))<max && fabs(se(2,1))<max
		)
		{
			// matrix is diagonal - use special case inversion
			//diag++;
			SetInvertScaled(op);
			return ;
		}
	#endif


	// calculate inversion using Gauss-Jordan elimination
	Matrix3T a=op;
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


// general calculations

void SaturateMin( Vector3T &min, Vector3TPar val )
{
	saturateMin(min[0],val[0]);
	saturateMin(min[1],val[1]);
	saturateMin(min[2],val[2]);
}
void SaturateMax( Vector3T &max, Vector3TPar val )
{
	saturateMax(max[0],val[0]);
	saturateMax(max[1],val[1]);
	saturateMax(max[2],val[2]);
}

void CheckMinMax( Vector3T &min, Vector3T &max, Vector3TPar val )
{
	saturateMin(min[0],val[0]),saturateMax(max[0],val[0]);
	saturateMin(min[1],val[1]),saturateMax(max[1],val[1]);
	saturateMin(min[2],val[2]),saturateMax(max[2],val[2]);
}

Vector3T VectorMin( Vector3TPar a, Vector3TPar b )
{
	return Vector3T
	(
		floatMin(a[0],b[0]),floatMin(a[1],b[1]),floatMin(a[2],b[2])
	);
}

Vector3T VectorMax( Vector3TPar a, Vector3TPar b )
{
	return Vector3T
	(
		floatMax(a[0],b[0]),floatMax(a[1],b[1]),floatMax(a[2],b[2])
	);
}

#endif
