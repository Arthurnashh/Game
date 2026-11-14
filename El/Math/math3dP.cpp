// homogenous vector and matrix arithmetics
// (C) 1997, SUMA
#include <El/elementpch.hpp>

#include "math3d.hpp"
#include "common/enf_mathlib.h"

#ifndef _T_MATH

#pragma warning(disable:4073)
#pragma init_seg(lib)

//#pragma optimize ("t",on)

#include "math3dP.hpp"
#include <Es/Framework/debugLog.hpp>

const Vector3P VZeroP(0,0,0) INIT_PRIORITY_HIGH;
const Vector3P VUpP(0,1,0) INIT_PRIORITY_HIGH;
const Vector3P VForwardP(0,0,1) INIT_PRIORITY_HIGH;
const Vector3P VAsideP(1,0,0) INIT_PRIORITY_HIGH;

const Matrix3P M3ZeroP
(
  0,0,0,
  0,0,0,
  0,0,0
) INIT_PRIORITY_HIGH;
const Matrix3P M3IdentityP
(
  1,0,0,
  0,1,0,
  0,0,1
) INIT_PRIORITY_HIGH;

const Matrix4P M4ZeroP
(
  0,0,0,
  0,0,0,
  0,0,0,
  0,0,0
) INIT_PRIORITY_HIGH;
const Matrix4P M4IdentityP
(
  1,0,0,0,
  0,1,0,0,
  0,0,1,0
) INIT_PRIORITY_HIGH;



//-----------------------------------------------------------------------
__forceinline float DEG2RAD(float deg) 
{ 
	return (deg * 3.14159265f) / 180.0f; 
}


//-----------------------------------------------------------------------
// ! ENF orientation, rebuild to DayZ

void SetRollPitchYawScale(Matrix3P& m, float roll, float pitch, float yaw, float scale)
{
	Assert(!BadFloat(roll));
	Assert(!BadFloat(pitch));
	Assert(!BadFloat(yaw));
	Assert(!BadFloat(scale));

	if(roll == 0.0f && pitch == 0.0f && yaw == 0.0f)
	{
		m._11 = scale;
		m._12 = 0.0f;
		m._13 = 0.0f;

		m._21 = 0.0f;
		m._22 = scale;
		m._23 = 0.0f;

		m._31 = 0.0f;
		m._32 = 0.0f;
		m._33 = scale;
	}
	else
	{
		float	angle;
		float sr, sp, sy, cr, cp, cy;

		angle = DEG2RAD(yaw);
		sy = sinf(angle);
		cy = cosf(angle);
		angle = DEG2RAD(pitch);
		sp = sinf(angle);
		cp = cosf(angle);
		angle = DEG2RAD(roll);
		sr = sinf(angle);
		cr = cosf(angle);

		m._11 = cp * cy * scale;
		m._12 = cp * sy * scale;
		m._13 = -sp * scale;

		m._21 = -(-1*sr*sp*cy+-1*cr*-sy) * scale;
		m._22 = -(-1*sr*sp*sy+-1*cr*cy) * scale;
		m._23 = sr*cp * scale;

		m._31 = (cr*sp*cy+-sr*-sy) * scale;
		m._32 = (cr*sp*sy+-sr*cy) * scale;
		m._33 = cr*cp * scale;
	}
}



bool Matrix4P::IsFinite() const
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
bool Matrix3P::IsFinite() const
{
  for( int i=0; i<3; i++ )
  {
    if( !_finite(Get(i,0)) ) return false;
    if( !_finite(Get(i,1)) ) return false;
    if( !_finite(Get(i,2)) ) return false;
  }
  return true;
}

float Matrix4P::Characteristic() const
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

void Matrix4P::SetIdentity()
{
  *this=M4IdentityP;
}

bool Matrix4P::IsIdentity() const
{
  return *this == M4IdentityP;
}

void Matrix4P::SetZero()
{
  *this=M4ZeroP;
}

void Matrix4P::SetTranslation( Vector3PPar offset )
{
  SetIdentity();
  SetPosition(offset);
}

void Matrix4P::SetRotationX( Coord angle )
{
  SetIdentity();
  Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
  Set(1,1)=+c,Set(1,2)=-s;
  Set(2,1)=+s,Set(2,2)=+c;
}

void Matrix4P::SetRotationY( Coord angle )
{
  SetIdentity();
  Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
  Set(0,0)=+c,Set(0,2)=-s;
  Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix4P::SetRotationZ( Coord angle )
{
  SetIdentity();
  Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
  Set(0,0)=+c,Set(0,1)=-s;
  Set(1,0)=+s,Set(1,1)=+c;
}

void Matrix4P::SetRotationY(Coord s, Coord c)
{
  SetIdentity();
  Set(0,0)=+c,Set(0,2)=-s;
  Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix4P::SetScale( Coord x, Coord y, Coord z )
{
  SetZero();
  Set(0,0)=x;
  Set(1,1)=y;
  Set(2,2)=z;
  //Set(3,3)=1;
}

void Matrix4P::SetPerspective( Coord cLeft, Coord cTop )
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

void Matrix4P::Orthogonalize()
{
  Vector3PVal dir=Direction();
  Vector3PVal up=DirectionUp();
  SetDirectionAndUp(dir,up);
}
void Matrix3P::Orthogonalize()
{
  Vector3PVal dir=Direction();
  Vector3PVal up=DirectionUp();
  SetDirectionAndUp(dir,up);
}

void Matrix4P::SetOriented( Vector3PPar dir, Vector3PPar up )
{
  SetDirectionAndUp(dir,up);
  SetPosition(VZeroP);
}

void Matrix4P::SetView( Vector3PPar point, Vector3PPar dir, Vector3PPar up )
{
  Matrix4P translate(MTranslation,-point);
  Matrix4P direction(MDirection,dir,up);
  SetMultiply(direction,translate);
}


void Matrix4P::SetAdd( const Matrix4P &a, const Matrix4P &b )
{
  _orientation._aside = a._orientation._aside+b._orientation._aside;
  _orientation._up = a._orientation._up+b._orientation._up;
  _orientation._dir = a._orientation._dir+b._orientation._dir;
  _position = a._position+b._position;
}

void Matrix4P::SetMultiply( const Matrix4P &a, float b )
{
  _orientation._aside = a._orientation._aside*b;
  _orientation._up = a._orientation._up*b;
  _orientation._dir = a._orientation._dir*b;
  _position = a._position*b;
}

void Matrix4P::AddMultiply( const Matrix4P &a, float b )
{
  _orientation._aside += a._orientation._aside*b;
  _orientation._up += a._orientation._up*b;
  _orientation._dir += a._orientation._dir*b;
  _position += a._position*b;
}

void Matrix4P::SetMultiplyByPerspective( const Matrix4P &a, const Matrix4P &b )
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

Vector3P Vector3P::Normalized() const
{
  Coord size2=SquareSizeInline();
  if( size2<=FLT_MIN ) return *this;
  Coord invSize=InvSqrt(size2);
  return Vector3P(X()*invSize,Y()*invSize,Z()*invSize);
}

float Vector3P::Normalize() // no return to avoid using instead of Normalized
{  
  Coord size2=SquareSizeInline();
  if( size2<=FLT_MIN ) 
    return 0.f;
  Coord invSize=InvSqrt(size2);
  Set(0)*=invSize,Set(1)*=invSize,Set(2)*=invSize;
  return 1/invSize;
}

Coord Vector3P::NormalizeSize ()
{
  Coord size2=SquareSizeInline();
  if( size2<=FLT_MIN ) return 0.0f;
  Coord size = coord(sqrt(size2));
  Coord invSize = 1.0f / size;
  Set(0)*=invSize,Set(1)*=invSize,Set(2)*=invSize;
  return size;
}

float Vector3P::CosAngle( Vector3PPar op ) const
{
  float denom2 = op.SquareSizeInline()*SquareSizeInline();
  if (denom2<=0) return 0;
  return DotProduct(op)*InvSqrt(denom2);
}

float Vector3P::SinAngle( Vector3PPar op, Vector3PPar worldsTop ) const
{
  float denom2 = op.SquareSizeInline()*SquareSizeInline();
  if (denom2<=0) return 0;
  Vector3P xprod = CrossProduct(op);
  float sz = xprod.Size();
  if (xprod.DotProduct(worldsTop)<0) sz = -sz;
  return sz*InvSqrt(denom2);
}


float Vector3P::Distance( Vector3PPar op ) const
{
  return (*this-op).Size();
}

Vector3P Vector3P::Project( Vector3PPar op ) const
{
  return op*DotProduct(op)*op.InvSquareSize();
}

bool VerifyFloat( float x );


bool Vector3P::IsFinite() const
{
  if( !VerifyFloat(Get(0)) ) return false;
  if( !VerifyFloat(Get(1)) ) return false;
  if( !VerifyFloat(Get(2)) ) return false;
  //if( !_finite(Get(0)) ) return false;
  //if( !_finite(Get(1)) ) return false;
  //if( !_finite(Get(2)) ) return false;
  return true;
}

#define NO_ASM 0
#define ASM_VC5 1
#define ASM_ICL45 2

void Vector3P::SetMultiplyLeft( Vector3PPar o, const Matrix3P &a )
{ // vector rotation - only 3x3 matrix is used, translation is ignored
  // u=M*v
  float o0=o[0],o1=o[1],o2=o[2];
  Set(0)=a.Get(0,0)*o0+a.Get(1,0)*o1+a.Get(2,0)*o2;
  Set(1)=a.Get(0,1)*o0+a.Get(1,1)*o1+a.Get(2,1)*o2;
  Set(2)=a.Get(0,2)*o0+a.Get(1,2)*o1+a.Get(2,2)*o2;
}

#define MySqrEps      1.e-5f
#define MyEps         1.e-3f
//#define AbsZero(x)    (x == 0)
#define AbsZero(x)    (x < MyEps)
#define AbsSqrZero(x) (x < MySqrEps)

Coord Vector3P::SetSolve2x2 ( Coord a, Coord b, Coord c,
                              Coord d, Coord e, Coord f )
{
  Coord det01 = a * e - b * d;
  Coord det02 = a * f - c * d;
  Coord det12 = b * f - c * e;
  Coord ad01 = fabs(det01);
  Coord ad02 = fabs(det02);
  Coord ad12 = fabs(det12);
  Coord inv;
  if ( ad01 >= ad02 &&
       ad01 >= ad12 )
  {
    if ( AbsZero(ad01) ) return -1.f;
      // result[2] is free variable
    inv = 1.0f / det01;
    Set(0) = -det12 * inv;
    Set(1) =  det02 * inv;
    Set(2) = -1.0f;
    return ad01;
  }
  if ( ad02 >= ad01 &&
       ad02 >= ad12 )
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

bool Vector3P::AllOnes () const
{
  return( IsOne(Get(0)) &&
          IsOne(Get(1)) &&
          IsOne(Get(2)) );
}

Coord Vector3P::SetEigenVector ( Coord a, Coord b, Coord c,
                                 Coord d, Coord e, Coord f )
{
    // try rows 0 and 1:
  Coord reliability = SetSolve2x2(a,b,c,b,d,e);
  if ( reliability > 0.f ) return reliability;
    // try rows 1 and 2:
  return SetSolve2x2(b,d,e,c,e,f);
}

Matrix4P& Matrix4P::Reflect(const enf::MPlane& plane)
{
	Assert(false);

/*
	_11 = 2.0f * plane.x * -plane.x + 1.0f;
	_12 = 2.0f * plane.y * -plane.x;
	_13 = 2.0f * plane.z * -plane.x;

	_21 = 2.0f * plane.x * -plane.y;
	_22 = 2.0f * plane.y * -plane.y + 1.0f;
	_23 = 2.0f * plane.z * -plane.y;

	_31 = 2.0f * plane.x * -plane.z;
	_32 = 2.0f * plane.y * -plane.z;
	_33 = 2.0f * plane.z * -plane.z + 1.0f;

	_41 = 2.0f * plane.x * plane.w;
	_42 = 2.0f * plane.y * plane.w;
	_43 = 2.0f * plane.z * plane.w;
*/

	//ENFORCE, should it be transposed?
	_orientation._11 = 2.0f * plane.x * -plane.x + 1.0f;
	_orientation._12 = 2.0f * plane.y * -plane.x;
	_orientation._13 = 2.0f * plane.z * -plane.x;

	_orientation._21 = 2.0f * plane.x * -plane.y;
	_orientation._22 = 2.0f * plane.y * -plane.y + 1.0f;
	_orientation._23 = 2.0f * plane.z * -plane.y;

	_orientation._31 = 2.0f * plane.x * -plane.z;
	_orientation._32 = 2.0f * plane.y * -plane.z;
	_orientation._33 = 2.0f * plane.z * -plane.z + 1.0f;

	_position.x = 2.0f * plane.x * plane.w;
	_position.y = 2.0f * plane.y * plane.w;
	_position.z = 2.0f * plane.z * plane.w;

	return *this;
}

void Matrix4P::SetInvertRotation( const Matrix4P &op )
{
  // invert orientation
  _orientation.SetInvertRotation(op.Orientation());
  // invert translation
  SetPosition(Rotate(-op.Position()));
}

void Matrix4P::SetInvertScaled( const Matrix4P &op )
{
  // invert orientation
  _orientation.SetInvertScaled(op.Orientation());
  // invert translation
  Vector3P pos;
  pos.SetMultiply(Orientation(),-op.Position());
  SetPosition(pos);
}

void Matrix4P::SetInvertGeneral( const Matrix4P &op )
{
  // invert orientation
  _orientation.SetInvertGeneral(op.Orientation());
  // invert translation
  SetPosition(Rotate(-op.Position()));
}

void Matrix3P::SetNormalTransform( const Matrix3P &op )
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

void Matrix3P::SetIdentity()
{
  *this=M3IdentityP;
}

void Matrix3P::SetZero()
{
  *this=M3ZeroP;
}

void Matrix3P::SetRotationX( Coord angle )
{
  Coord s=sin(angle),c=cos(angle);
  SetIdentity();
  Set(1,1)=+c,Set(1,2)=-s;
  Set(2,1)=+s,Set(2,2)=+c;
}

void Matrix3P::SetRotationY( Coord angle )
{
  Coord s=sin(angle),c=cos(angle);
  SetIdentity();
  Set(0,0)=+c,Set(0,2)=-s;
  Set(2,0)=+s,Set(2,2)=+c;
}

void Matrix3P::SetRotationZ( Coord angle )
{
  Coord s=(Coord)sin(angle),c=(Coord)cos(angle);
  SetIdentity();
  Set(0,0)=+c,Set(0,1)=-s;
  Set(1,0)=+s,Set(1,1)=+c;
}

void Matrix3P::SetRotationAxis(Vector3PPar axis, Coord angle)
{
  // Normalize input vector
  Vector3P axisN = axis;
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

void Matrix3P::SetScale( Coord x, Coord y, Coord z )
{
  SetZero();
  Set(0,0)=x;
  Set(1,1)=y;
  Set(2,2)=z;
  //Set(3,3)=1;
}

void Matrix3P::SetScale( float scale )
{
  Assert(_finite(scale));
  // note: old scale may be zero
  float invOldScale=InvScale();
  if( invOldScale>0 )
  {
    //Matrix3P rescale(MScale,scale*invOldScale);
    //(*this)=(*this)*rescale;
    (*this)*=scale*invOldScale;
  }
  else
  {
    SetScale(scale,scale,scale);
  }
  // note: any SetDirection will remove scale
}
/** Calculates average scale. There can exist vectors with bigger scale.*/
float Matrix3P::Scale2() const
{
  // Frame transformation is composed from
  // rotation*scale
  // current scale can be determined as
  // orient * transpose orient

  //Matrix3P invR=InverseRotation();
  //Matrix3P oTo=(*this)*invR;
  // note all but [i][i] should be zero
  //float sx2=oTo(0,0);
  //float sy2=oTo(1,1);
  //float sz2=oTo(2,2);

  // optimized matrix transposition + multiplication
  Vector3P sv;
  for( int i=0; i<3; i++ )
  {
    sv[i]=Square(Get(i,0))+Square(Get(i,1))+Square(Get(i,2));
  }

  // calculate average scale
  return (sv[0]+sv[1]+sv[2])*(1.0/3);
}

/** Calculates estimation of squared maximal possible scale. 
  * The correct squared maximal possible scale is equal or lower than this estimation. The values are equal for orthogonal 
  * matrices. 
  */
float Matrix3P::MaxScale2() const
{
  /// estimation sqrt(max_{i = 0..2} (|S_i|^2) + 2 * max_{i,j = 0..2, i != j} abs(S_i * S_j)) where S_i is i-th column
  float maxS = _aside.SquareSize(); // maximal column size
  saturateMax(maxS, _dir.SquareSize());
  saturateMax(maxS, _up.SquareSize());

  float maxSS = fabs(_aside * _dir); //maximal in-between column product
  saturateMax(maxSS, fabs(_aside * _up));
  saturateMax(maxSS, fabs(_dir * _up));

  return maxS + 2 * maxSS;
}

void Matrix3P::ScaleAndMaxScale2(float& scale, float& maxScale) const
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

void Matrix3P::SetDirectionAndUp( Vector3PPar dir, Vector3PPar up )
{
  SetDirection(dir.Normalized());
  // Project into the plane
  Coord t=up*Direction();
  SetDirectionUp((up-Direction()*t).Normalized());
  // Calculate the vector pointing along the x axis (i.e. aside)
  // sign is experimental - I never know what is right and what is left
  // no need to normalize - cross product of two perpendicular unit vectors is unit vector
  SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}

void Matrix3P::SetDirectionAndAside( Vector3PPar dir, Vector3PPar aside )
{
  SetDirection(dir.Normalized());
  // Project into the plane
  Coord t=aside*Direction();
  SetDirectionAside((aside-Direction()*t).Normalized());
  // Calculate the vector pointing along the x axis (i.e. aside)
  // sign is experimental - I never know what is right and what is left
  // no need to normalize - cross product of two perpendicular unit vectors is unit vector
  SetDirectionUp(Direction().CrossProduct(DirectionAside()));
}

void Matrix3P::SetUpAndAside( Vector3PPar up, Vector3PPar aside )
{
  SetDirectionUp(up.Normalized());
  // Project into the plane
  Coord t=DirectionUp()*aside;
  SetDirectionAside((aside-DirectionUp()*t).Normalized());
  // Calculate the vector pointing along the x axis (i.e. aside)
  // sign is experimental - I never know what is right and what is left
  // no need to normalize - cross product of two perpendicular unit vectors is unit vector
  SetDirection(DirectionAside().CrossProduct(DirectionUp()));
}

void Matrix3P::SetUpAndDirection( Vector3PPar up, Vector3PPar dir )
{
  SetDirectionUp(up.Normalized());
  // Project into the plane
  Coord t=DirectionUp()*dir;
  // Calculate the vector pointing along the x axis (i.e. aside)
  // sign is experimental - I never know what is right and what is left
  // no need to normalize - cross product of two perpendicular unit vectors is unit vector
  SetDirection((dir-DirectionUp()*t).Normalized());
  SetDirectionAside(DirectionUp().CrossProduct(Direction()));
}


void Matrix3P::SetTilda( Vector3PPar a )
{
  SetZero();
  Set(0,1)=-a[2],Set(0,2)=+a[1];
  Set(1,0)=+a[2],Set(1,2)=-a[0];
  Set(2,0)=-a[1],Set(2,1)=+a[0];
}

void Matrix3P::operator *= ( float op )
{
  _aside*=op;
  _up*=op;
  _dir*=op;
}


void Matrix3P::SetAdd( const Matrix3P &a, const Matrix3P &b )
{
  _aside = a._aside+b._aside;
  _up = a._up+b._up;
  _dir = a._dir+b._dir;
}


void Matrix3P::SetMultiply( const Matrix3P &a, float op )
{
  // matrix multiplication
  _aside = a._aside*op;
  _up = a._up*op;
  _dir = a._dir*op;
}

void Matrix3P::AddMultiply( const Matrix3P &a, float op )
{
  // matrix multiplication
  _aside += a._aside*op;
  _up += a._up*op;
  _dir += a._dir*op;
}

Matrix3P Matrix3P::operator + ( const Matrix3P &a ) const
{
  Matrix3P res;
  res._aside=_aside+a._aside;
  res._up=_up+a._up;
  res._dir=_dir+a._dir;
  return res;
}

Matrix3P Matrix3P::operator - ( const Matrix3P &a ) const
{
  Matrix3P res;
  res._aside=_aside-a._aside;
  res._up=_up-a._up;
  res._dir=_dir-a._dir;
  return res;
}

Matrix3P &Matrix3P::operator -= ( const Matrix3P &a )
{
  _aside-=a._aside;
  _up-=a._up;
  _dir-=a._dir;
  return *this;
}

Matrix3P &Matrix3P::operator += ( const Matrix3P &a )
{
  _aside+=a._aside;
  _up+=a._up;
  _dir+=a._dir;
  return *this;
}


Matrix4P Matrix4P::operator + ( const Matrix4P &a ) const
{
  Matrix4P res;
  res._orientation._aside=_orientation._aside+a._orientation._aside;
  res._orientation._up=_orientation._up+a._orientation._up;
  res._orientation._dir=_orientation._dir+a._orientation._dir;
  res._position=_position+a._position;
  return res;
}

Matrix4P Matrix4P::operator - ( const Matrix4P &a ) const
{
  Matrix4P res;
  res._orientation._aside=_orientation._aside-a._orientation._aside;
  res._orientation._up=_orientation._up-a._orientation._up;
  res._orientation._dir=_orientation._dir-a._orientation._dir;
  res._position=_position-a._position;
  return res;
}

void Matrix4P::operator += ( const Matrix4P &op )
{
  _orientation._aside+=op._orientation._aside;
  _orientation._up+=op._orientation._up;
  _orientation._dir+=op._orientation._dir;
  _position+=op._position;
}
void Matrix4P::operator -= ( const Matrix4P &op )
{
  _orientation._aside-=op._orientation._aside;
  _orientation._up-=op._orientation._up;
  _orientation._dir-=op._orientation._dir;
  _position-=op._position;
}


void Matrix3P::SetInvertRotation( const Matrix3P &op )
{
#if _DEBUG
  Vector3P sizes;
  Assert(op.IsOrthogonal(&sizes) && AbsSqrZero(fabs(sizes[0] - 1)) && AbsSqrZero(fabs(sizes[1] - 1)) && AbsSqrZero(fabs(sizes[2] - 1)));
#endif

  for( int i=0; i<3; i++ ) //for( int j=0; j<3; j++ )
  {
    Set(i,0)=op.Get(0,i);
    Set(i,1)=op.Get(1,i);
    Set(i,2)=op.Get(2,i);
  }
}

//#define FailF(format,args) DebF("%s(%d) : Format",__FILE__,__LINE__,args)

void Matrix3P::SetInvertScaled( const Matrix3P &op )
{
  // matrix inversion is calculated based on these prepositions:
  // matrix is S*R, where S is scale, R is rotation
  // inversion of such matrix is Inv(R)*Inv(S)
  // Inv(R) is Transpose(R), Inv(S) is C: C(i,i)=1/S(i,i)
  // sizes of row(i) are scale coeficients a,b,c
  // all member are set
  //SetIdentity();
#if _DEBUG
  Assert(op != M3ZeroP);
#endif

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

  #if _DEBUG
    // verify the result is matrix inverse
    {
      // verify matrix is S*R
      // scaled matrix has form S*R, where S is scale matrix and R is rotation
      // (S*R)*(RT*ST)=S*ST=S*S
      // transpose
      Matrix3P se = *this*op;
      // check if the result is identity
      const float max=3e-6f;
      if(
        fabs(se(0,0)-1)<max && fabs(se(1,1)-1)<max && fabs(se(2,2)-1)<max &&
        fabs(se(0,1))<max && fabs(se(0,2))<max &&
        fabs(se(1,0))<max && fabs(se(1,2))<max &&
        fabs(se(2,0))<max && fabs(se(2,1))<max
      )
      {}
      else
      {
        LogF("%s(%d) : Inverse failed - not scale*rotation?",__FILE__,__LINE__);
      }
    }
  #endif

}

#define Swap(a,b) {float p;p=a;a=b;b=p;}

void Matrix3P::SetInvertGeneral( const Matrix3P &op )
{
  Assert(op != M3ZeroP);
  #if 1
    // TODO: detect general matrices as soon as possible
    // check if they are really necessary
    // check if matrix is really general
    // if not, we can use scaled version
    // scaled matrix has form S*R, where S is scale matrix and R is rotation
    // (S*R)*(RT*ST)=S*ST=S*S
    // 
    Matrix3P t;
    for( int i=0; i<3; i++ )
    {
      t(0,i)=op(i,0),t(1,i)=op(i,1),t(2,i)=op(i,2);
    }
    Matrix3PVal se=op*t;
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
      fabs(se(0,1))<max*fabs(se(0,0)) && fabs(se(0,2))<max*fabs(se(0,0)) &&
      fabs(se(1,0))<max*fabs(se(1,1)) && fabs(se(1,2))<max*fabs(se(1,1)) &&
      fabs(se(2,0))<max*fabs(se(2,2)) && fabs(se(2,1))<max*fabs(se(2,2))
    )
    {
      // matrix is diagonal - use special case inversion
      //diag++;
      SetInvertScaled(op);
      return ;
    }
  #endif


  // calculate inversion using Gauss-Jordan elimination
  Matrix3P a=op;
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

void Matrix3P::SetCovarianceCenter ( const AutoArray<Vector3P> &data, const Matrix4P *m )
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
    Vector3P v;
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

bool Matrix3P::SetEigenStandard ( Matrix3PPar cov, Coord *eig )
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
  Vector3P v[3];
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

float Matrix3P::Determinant() const
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

bool Matrix3P::IsOrthogonal ( Vector3P *sqrSize ) const
{
  float sqAside = _aside.SquareSize();
  float sqUp = _up.SquareSize();
  if (sqAside==0 || sqUp==0) return false;
  if ( !AbsZero(fabs( _aside * _up  ) * InvSqrt(sqAside)) ||
       !AbsZero(fabs( _aside * _dir ) * InvSqrt(sqAside)) ||
       !AbsZero(fabs( _up    * _dir ) * InvSqrt(sqUp)) )
    return false;
  if ( sqrSize )
  {
    (*sqrSize)[0] = sqAside;
    (*sqrSize)[1] = sqUp;
    (*sqrSize)[2] = _dir.SquareSize();
  }
  return true;
}

// general calculations

void SaturateMin( Vector3P &min, Vector3PPar val )
{
  saturateMin(min[0],val[0]);
  saturateMin(min[1],val[1]);
  saturateMin(min[2],val[2]);
}
void SaturateMax( Vector3P &max, Vector3PPar val )
{
  saturateMax(max[0],val[0]);
  saturateMax(max[1],val[1]);
  saturateMax(max[2],val[2]);
}

void CheckMinMax( Vector3P &min, Vector3P &max, Vector3PPar val )
{
  saturateMin(min[0],val[0]),saturateMax(max[0],val[0]);
  saturateMin(min[1],val[1]),saturateMax(max[1],val[1]);
  saturateMin(min[2],val[2]),saturateMax(max[2],val[2]);
}

Vector3P VectorMin( Vector3PPar a, Vector3PPar b )
{
  return Vector3P
  (
    floatMin(a[0],b[0]),floatMin(a[1],b[1]),floatMin(a[2],b[2])
  );
}

Vector3P VectorMax( Vector3PPar a, Vector3PPar b )
{
  return Vector3P
  (
    floatMax(a[0],b[0]),floatMax(a[1],b[1]),floatMax(a[2],b[2])
  );
}


#endif
