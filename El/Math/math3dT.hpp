#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATH3DT_HPP
#define _MATH3DT_HPP

// homogenous vector and matrix arithmetics
// (C) 1997, SUMA

// template based Pentium version
#include "mathDefs.hpp"

#include <math.h>
#include <float.h>
#include <Es/Common/fltopts.hpp>

#include "mathOpt.hpp"

#include "vecTempl.hpp"

// use parameter placeholder to explicitly disable initialization of elements

#ifndef vecAlign
	// no alignment required in P implementation
	#define vecAlign
#endif

// define how should const references be passed
#define Vector3TPar const Vector3T &
#define Vector3TVal const Vector3T &

#define Matrix3TPar const Matrix3T &
#define Matrix3TVal const Matrix3T &

#define Matrix4TPar const Matrix4T &
#define Matrix4TVal const Matrix4T &

class Vector3T;
class Matrix3T;
class Matrix4T;

extern const Vector3T VZeroT;
extern const Vector3T VUpT;
extern const Vector3T VForwardT;
extern const Vector3T VAsideT;

extern const Matrix3T M3ZeroT;
extern const Matrix3T M3IdentityT;

extern const Matrix4T M4ZeroT;
extern const Matrix4T M4IdentityT;

#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/array.hpp>


class Vector3T : public vt::EBase<3>
{
	friend class Matrix4T;
	friend class Matrix3T;

  typedef vt::EBase<3> vbase;

	public:

  __forceinline  Vector3T( const float x, const float y, const float z )
  { Set(0) = x; Set(1) = y; Set(2) = z; }
  __forceinline  Vector3T(){}

	// this point performs expression evaluation 
  template< class ta_type > __forceinline
  Vector3T& operator = ( const ta_type& A )
  { vbase::operator = ( A ); return *this; }

	/**/
	// TODO: use function Copy for both operator = and copy constructor
  template< class ta_type > __forceinline
  Vector3T( const ta_type& A )
  { vbase::operator = ( A );}
	/**/

	__forceinline float X() const {return Evaluate(0);}
	__forceinline float Y() const {return Evaluate(1);}
	__forceinline float Z() const {return Evaluate(2);}

	// feature complete Vector3T
	// access to some template functions
	__forceinline Coord operator * ( Vector3TPar op ) const
	{
		return DotProduct(op);
	}
	__forceinline Vector3T operator * ( const Matrix3T &op ) const
	{
		return Vector3T(VMultiplyLeft,*this,op);
	}

	// template optimal features
	// set functions
	void SetMultiplyLeft( Vector3TPar v, const Matrix3T &a );

	void SetRotate( const Matrix4T &a, Vector3TPar v );

	void SetFastTransform( const Matrix4T &a, Vector3TPar v );

	__forceinline void SetMultiply( const Matrix3T &a, Vector3TPar v );
	__forceinline void SetMultiply( const Matrix4T &a, Vector3TPar v );

	float SetPerspectiveProject( const Matrix4T &a, Vector3TPar o );

	// constructors
	__forceinline void Init() {} // init 4th component

	__forceinline Vector3T( enum _noInit ){}

	__forceinline Vector3T( enum _vMultiply, const Matrix3T &a, Vector3TPar v ){SetMultiply(a,v);}
	__forceinline Vector3T( enum _vMultiply, const Matrix4T &a, Vector3TPar v ){SetMultiply(a,v);}
	__forceinline Vector3T( enum _vMultiplyLeft, Vector3TPar v, const Matrix3T &a ){SetMultiplyLeft(v,a);}

	__forceinline Vector3T( enum _vRotate, const Matrix4T &a, Vector3TPar v ){SetRotate(a,v);}
	__forceinline Vector3T( enum _vFastTransform, const Matrix4T &a, Vector3TPar v ){SetFastTransform(a,v);}
	__forceinline Vector3T( enum _vFastTransformA, const Matrix4T &a, Vector3TPar v ){SetFastTransform(a,v);}


	bool IsFinite() const;

	template <class ta>
	__forceinline Vector3T Modulate( const ta &op ) const
	{
		return Vector3T
		(
			Get(0)*op.Evaluate(0),
			Get(1)*op.Evaluate(1),
			Get(2)*op.Evaluate(2)
		);
	}

	// operators that are optimal even with non-template implementation
	Vector3T &operator *= ( Coord f )
	{
		_e[0]*=f;
		_e[1]*=f;
		_e[2]*=f;
		return *this;
	}
	Vector3T &operator /= ( Coord f )
	{
		float invF=1/f;
		_e[0]*=invF;
		_e[1]*=invF;
		_e[2]*=invF;
		return *this;
	}

	// features that are sub-optimal (non-templated)

	// features that really should be templated
	__forceinline Vector3T operator - () const
	{
		return Vector3T(-X(),-Y(),-Z());
	}

	Vector3T CrossProduct( Vector3TPar op ) const;
	Vector3T Normalized() const;
	void Normalize(); // no return to avoid using instead of Normalized

	float CosAngle( Vector3TPar op ) const;
	float Distance( Vector3TPar op ) const;

	__forceinline float Distance2( Vector3TPar op ) const
	{
		return (*this-op).SquareSize();
	}
	__forceinline float Distance2Inline( Vector3TPar op ) const
	{
		return (*this-op).SquareSizeInline();
	}

	float DistanceXZ( Vector3TPar op ) const;
	float DistanceXZ2( Vector3TPar op ) const;

	/*
	Coord Size() const
	{
		// optimization: |v|=|v|*|v|/|v|
		float size2=SquareSizeInline();
		if( size2==0 ) return 0;
		Coord invSize=InvSqrt(size2);
		return size2*invSize;
	}
	*/
	Coord InvSize() const {return InvSqrt(SquareSize());}
	Coord InvSquareSize() const {return Inv(SquareSize());}

	/*
	Coord SquareSizeXZ() const {return X()*X()+Z()*Z();}
	Coord SizeXZ() const
	{
		float size2=SquareSizeXZ();
		if( size2==0 ) return 0;
		Coord invSize=InvSqrt(size2);
		return size2*invSize;
	}
	*/
	Coord InvSizeXZ() const {return InvSqrt(SquareSizeXZ());}
	Coord InvSquareSizeXZ() const {return Inv(SquareSizeXZ());}

	// features that are not used often
	Vector3T Project( Vector3TPar op ) const;
	Matrix3T Tilda() const;

  /**
   * Solves homogen. equation system, returns reliability of the result
   *  (higher values are better) or -1 if not solvable.
   */
  Coord SetSolve2x2 ( Coord a, Coord b, Coord c, Coord d, Coord e, Coord f );

  /**
   * Finds eigenvector to the given eigenvalue ("a" to "f" are items
   *  of symmetric matrix to be solved). Returns reliability (higher is better)
   *  or -1 if not solvable.
   */
  Coord SetEigenVector ( Coord a, Coord b, Coord c, Coord d, Coord e, Coord f );

  /// Returns true for [1,1,1]
  bool AllOnes () const;

};

TypeIsSimple(Vector3T);

class Matrix3T
{
	friend class Vector3T;
	friend class Matrix4T;
	
	// homogeneous matrix - transformations
	private:

	// non-array implementation
	Vector3T _aside;
	Vector3T _up;
	Vector3T _dir;
	__forceinline Coord Get( int i, int j ) const {return (&_aside)[j][i];}
	__forceinline Coord &Set( int i, int j ) {return (&_aside)[j][i];}

	public:
	// functions that load matrix with data
	// used internally in constructors, but may be useful also to other purpose
	void SetIdentity();
	void SetZero();
	void SetRotationX( Coord angle );
	void SetRotationY( Coord angle );
	void SetRotationZ( Coord angle );

  //! Sets the rotation matrix according to an arbitrary axis
  void SetRotationAxis(Vector3TPar axis, Coord angle);
  
	void SetScale( Coord x, Coord y, Coord z );

	void SetScale( float scale );
  /// get average scale  (some vectors can be after transformation longer)
	float Scale2() const; 
  /// get maximal scale
  float MaxScale2() const;
  /// optimization: calculate scale and maxScale at once
  void ScaleAndMaxScale2(float& scale, float& maxScale) const;
 	
  /// get average scale  (some vectors can be after transformation longer)
  float Scale() const
  {
    // if scale is zero, the matrix is singular
    float scale2=Scale2();
    Assert(scale2>0);
    return sqrt(scale2);
  }
  /// get maximal scale
  float MaxScale() const
  {
    // if scale is zero, the matrix is singular
    float scale2=MaxScale2();
    Assert(scale2>0);
    return sqrt(scale2);
  }
  void ScaleAndMaxScale(float& scale, float& maxScale) const
  {
    ScaleAndMaxScale2(scale, maxScale);
    Assert(scale>0 && maxScale>0);
    scale = sqrt(scale);
    maxScale = sqrt(maxScale);
  }
  
  /// get inverse scale
  float InvScale() const
  {
    float scale2=Scale2();
    if( scale2<=0 ) return 1; // singular matrix
    return InvSqrt(scale2);
  }
  
	void SetDirectionAndUp( Vector3TPar dir, Vector3TPar up ); // sets only 3x3 submatrix
	void SetUpAndAside( Vector3TPar up, Vector3TPar aside );
	void SetUpAndDirection( Vector3TPar up, Vector3TPar dir );
	void SetDirectionAndAside( Vector3TPar dir, Vector3TPar aside );

	__forceinline void InlineSetMultiply( const Matrix3T &a, float op )
	{
		SetMultiply(a,op);
	}
	__forceinline void InlineAddMultiply( const Matrix3T &a, float op )
	{
		AddMultiply(a,op);
	}
	__forceinline void InlineSetAdd( const Matrix3T &a, const Matrix3T &b )
	{
		SetAdd(a,b);
	}

	void AddMultiply( const Matrix3T &a, float op );
	void SetMultiply( const Matrix3T &a, const Matrix3T &b );
	void SetAdd( const Matrix3T &a, const Matrix3T &b );
	void SetMultiply( const Matrix3T &a, float op );
	void SetInvertRotation( const Matrix3T &op );
	void SetInvertScaled( const Matrix3T &op );
	void SetInvertGeneral( const Matrix3T &op );
	void SetNormalTransform( const Matrix3T &op );
	void SetTilda( Vector3TPar a );
	
	// placeholder parameter describes constructor type
	Matrix3T
	(
		float m00, float m01, float m02,
		float m10, float m11, float m12,
		float m20, float m21, float m22
	)
	{
		Set(0,0)=m00,Set(0,1)=m01,Set(0,2)=m02;
		Set(1,0)=m10,Set(1,1)=m11,Set(1,2)=m12;
		Set(2,0)=m20,Set(2,1)=m21,Set(2,2)=m22;
	}
	__forceinline Matrix3T(){}
	__forceinline Matrix3T( enum _noInit ){}
	//__forceinline Matrix3T( enum _mIdentity ){SetIdentity();}
	//__forceinline Matrix3T( enum _mZero ){SetZero();}
	__forceinline Matrix3T( enum _mRotationX, Coord angle ){SetRotationX(angle);}
	__forceinline Matrix3T( enum _mRotationY, Coord angle ){SetRotationY(angle);}
	__forceinline Matrix3T( enum _mRotationZ, Coord angle ){SetRotationZ(angle);}
	__forceinline Matrix3T( enum _mScale, Coord x, Coord y, Coord z ){SetScale(x,y,z);}
	__forceinline Matrix3T( enum _mScale, Coord x ){SetScale(x,x,x);}
	__forceinline Matrix3T( enum _mDirection, Vector3TPar dir, Vector3TPar up )
	{
		SetDirectionAndUp(dir,up);
	}
	__forceinline Matrix3T( enum _mUpAndDirection, Vector3TPar dir, Vector3TPar up )
	{
		SetUpAndDirection(dir,up);
	}
	__forceinline Matrix3T( enum _mMultiply, const Matrix3T &a, const Matrix3T &b ){SetMultiply(a,b);}
	__forceinline Matrix3T( enum _mMultiply, const Matrix3T &a, float op ){SetMultiply(a,op);}
	__forceinline Matrix3T( enum _mInverseRotation, const Matrix3T &a ){SetInvertRotation(a);}
	__forceinline Matrix3T( enum _mInverseGeneral, const Matrix3T &a ){SetInvertGeneral(a);}
	__forceinline Matrix3T( enum _mInverseScaled, const Matrix3T &a ){SetInvertScaled(a);}
	__forceinline Matrix3T( enum _mNormalTransform, const Matrix3T &a ){SetNormalTransform(a);}
	__forceinline Matrix3T( enum _mTilda, Vector3TPar a ){SetTilda(a);}
	
	// following operators are defined so that no copy constuctor is used
	// if they are expanded inline, copy is not needed
	__forceinline Matrix3T operator * ( const Matrix3T &op ) const
	{
		// matrix multiplication
		return Matrix3T(MMultiply,*this,op);
	}
	__forceinline Vector3T operator * ( Vector3TPar op ) const
	{
		// vector transformation
		return Vector3T(VMultiply,*this,op);
	}
	__forceinline Matrix3T operator * ( float op ) const
	{
		return Matrix3T(MMultiply,*this,op);
	}
	void operator *= ( float op );
	Matrix3T operator + ( const Matrix3T &a ) const;
	Matrix3T operator - ( const Matrix3T &a ) const;
	Matrix3T &operator += ( const Matrix3T &a );
	Matrix3T &operator -= ( const Matrix3T &a );
	
  bool operator ==(const Matrix3T &op) const {return _aside==op._aside&& _up==op._up && _dir==op._dir;}
  bool operator !=(const Matrix3T &op) const {return _aside!=op._aside || _up!=op._up || _dir!=op._dir;}
  
	__forceinline Matrix3T InverseRotation() const {return Matrix3T(MInverseRotation,*this);}
	__forceinline Matrix3T InverseGeneral() const {return Matrix3T(MInverseGeneral,*this);}
	__forceinline Matrix3T InverseScaled() const {return Matrix3T(MInverseScaled,*this);}
	__forceinline Matrix3T NormalTransform() const {return Matrix3T(MNormalTransform,*this);}
	// member access operators
	__forceinline Coord operator () ( int i, int j ) const {return Get(i,j);}
	__forceinline Coord &operator () ( int i, int j ) {return Set(i,j);}

	bool IsFinite() const;

	// simple access to generic transformation matrix
	__forceinline const Vector3T &Direction() const {return _dir;}
	__forceinline const Vector3T &DirectionUp() const {return _up;}
	__forceinline const Vector3T &DirectionAside() const {return _aside;}
	void SetDirection( const Vector3T &v ) {_dir=v;}
	void SetDirectionUp( const Vector3T &v ) {_up=v;}
	void SetDirectionAside( const Vector3T &v ) {_aside=v;}

  __forceinline float Distance2(const Matrix3T &mat) const
  {
    return _dir.Distance2(mat._dir)+_up.Distance2(mat._up)+_aside.Distance2(mat._aside);
  }
	void Orthogonalize();

  // math for PCA:

  /// Transformation matrix is non-mandatory, if NULL, no transform is performed
  void SetCovarianceCenter ( const AutoArray<Vector3T> &data, const Matrix4T *m =NULL );

  bool SetEigenStandard ( Matrix3TPar cov, Coord *eig =NULL );

  bool IsOrthogonal ( Vector3T *sqrSize ) const;
  /// calculate matrix determinant
  float Determinant() const;
  /// calculate determinant of upper-left 2x2 submatrix
  float Determinant2x2() const
  {
    return Get(0,0)*Get(1,1)-Get(1,0)*Get(0,1);
  }
};

// define EMPTY constructors
TypeIsSimple(Matrix3T);

class Matrix4T
{
	friend class Vector3T;
	
	// homogenous matrix - transformations
	private:
	Matrix3T _orientation;
	Vector3T _position;

	__forceinline Coord Get( int i, int j ) const {return _orientation.Get(i,j);}
	__forceinline Coord &Set( int i, int j ) {return _orientation.Set(i,j);}

	__forceinline Coord GetPos( int i ) const	{return _position.Get(i);}
	__forceinline Coord &SetPos( int i ) {return _position.Set(i);}
	
	public:
	// functions that load matrix with data
	// used internaly in constuctors, but may be useful also to other purpose
	void SetIdentity();
	void SetZero();
	void SetTranslation( Vector3TPar offset );
	void SetRotationX( Coord angle );
	void SetRotationY( Coord angle );
	void SetRotationZ( Coord angle );
	void SetScale( Coord x, Coord y, Coord z );
	//void SetPerspective( Coord cNear, Coord cFar, Coord cLeft, Coord cTop );
	void SetPerspective( Coord cLeft, Coord cTop );

	// sets only 3x3 submatrix
	__forceinline void SetDirectionAndUp( Vector3TPar dir, Vector3TPar up )
	{
		_orientation.SetDirectionAndUp(dir,up);
	}
	__forceinline void SetUpAndAside( Vector3TPar up, Vector3TPar aside )
	{
		_orientation.SetUpAndAside(up,aside);
	}
	__forceinline void SetUpAndDirection( Vector3TPar up, Vector3TPar dir )
	{
		_orientation.SetUpAndDirection(up,dir);
	}
	__forceinline void SetDirectionAndAside( Vector3TPar dir, Vector3TPar aside )
	{
		_orientation.SetDirectionAndAside(dir,aside);
	}

  /// get average scale  (some vectors can be after transformation longer)
	__forceinline void SetScale( float scale ){_orientation.SetScale(scale);}
	__forceinline float Scale() const {return _orientation.Scale();}
  /// get maximal scale
  __forceinline float MaxScale() const {return _orientation.MaxScale();}
	//__forceinline float InvScale() const {return _orientation.InvScale();}
  /// optimalization calculate scale and maxScale at once
   __forceinline void ScaleAndMaxScale(float& scale, float& maxScale) const {return _orientation.ScaleAndMaxScale(scale, maxScale);}

	void SetOriented( Vector3TPar dir, Vector3TPar up ); // sets whole 4x4 matrix
	void SetView( Vector3TPar point, Vector3TPar dir, Vector3TPar up );
	void SetMultiply( const Matrix4T &a, const Matrix4T &b );
	void SetAdd( const Matrix4T &a, const Matrix4T &b );
	void SetMultiply( const Matrix4T &a, float b );
	void AddMultiply( const Matrix4T &a, float b );
	__forceinline void InlineSetMultiply( const Matrix4T &a, float op )
	{
		SetMultiply(a,op);
	}
	__forceinline void InlineAddMultiply( const Matrix4T &a, float op )
	{
		AddMultiply(a,op);
	}
	__forceinline void InlineSetAdd( const Matrix4T &a, const Matrix4T &b )
	{
		SetAdd(a,b);
	}

	void SetMultiplyByPerspective( const Matrix4T &A, const Matrix4T &B );
	void SetInvertRotation( const Matrix4T &op );
	void SetInvertScaled( const Matrix4T &op );
	void SetInvertGeneral( const Matrix4T &op );
	//void SetNormalTransform( const Matrix4T &op ); // in fact result is Matrix3T
	
	// placeholder parameter describes constructor type
	Matrix4T
	(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23
	)
	{
		Set(0,0)=m00,Set(0,1)=m01,Set(0,2)=m02;
		Set(1,0)=m10,Set(1,1)=m11,Set(1,2)=m12;
		Set(2,0)=m20,Set(2,1)=m21,Set(2,2)=m22;
		SetPos(0)=m03,SetPos(1)=m13,SetPos(2)=m23;
	}
	__forceinline Matrix4T(){}
	//__forceinline Matrix4T( enum _mIdentity ){SetIdentity();}
	__forceinline Matrix4T( enum _noInit ){}
	//__forceinline Matrix4T( enum _mZero ){SetZero();}
	__forceinline Matrix4T( enum _mTranslation, Vector3TPar offset ){SetTranslation(offset);}
	__forceinline Matrix4T( enum _mRotationX, Coord angle ){SetRotationX(angle);}
	__forceinline Matrix4T( enum _mRotationY, Coord angle ){SetRotationY(angle);}
	__forceinline Matrix4T( enum _mRotationZ, Coord angle ){SetRotationZ(angle);}
	__forceinline Matrix4T( enum _mScale, Coord x, Coord y, Coord z ){SetScale(x,y,z);}
	__forceinline Matrix4T( enum _mScale, Coord x ){SetScale(x,x,x);}
  // Scale space according to center
  __forceinline Matrix4T( enum _mScale, Coord x, Vector3TPar center ) 
  {
    SetScale(x,x,x);
    SetPosition(center - Rotate(center));
  }
	__forceinline Matrix4T( enum _mPerspective, Coord cLeft, Coord cTop )
	{
		SetPerspective(cLeft,cTop);
	}
	__forceinline Matrix4T( enum _mDirection, Vector3TPar dir, Vector3TPar up )
	{
		SetOriented(dir,up);
	}
  __forceinline Matrix4T( enum _mUpAndDirection, Vector3TPar up, Vector3TPar dir )
  {
    SetUpAndDirection(up,dir);
    SetPosition(VZeroT);
  }
	__forceinline Matrix4T( enum _mView, Vector3TPar point, Vector3TPar dir, Vector3TPar up )
	{
		SetView(point,dir,up);
	}
	__forceinline Matrix4T( enum _mMultiply, const Matrix4T &a, const Matrix4T &b ){SetMultiply(a,b);}
	__forceinline Matrix4T( enum _mMultiply, const Matrix4T &a, float b ){SetMultiply(a,b);}
	__forceinline Matrix4T( enum _mInverseRotation, const Matrix4T &a ){SetInvertRotation(a);}
	__forceinline Matrix4T( enum _mInverseScaled, const Matrix4T &a ){SetInvertScaled(a);}
	__forceinline Matrix4T( enum _mInverseGeneral, const Matrix4T &a ){SetInvertGeneral(a);}
	//Matrix4T( enum _mNormalTransform, const Matrix4T &a ){SetNormalTransform(a);}

	// following operators are defined so that no copy constuctor is used
	// if they are expanded inline, copy is not needed
	__forceinline Matrix4T operator * ( const Matrix4T &op ) const
	{
		// matrix multiplication
		return Matrix4T(MMultiply,*this,op);
	}
	__forceinline Matrix4T operator * ( float op ) const
	{
		return Matrix4T(MMultiply,*this,op);
	}

	Matrix4T operator + ( const Matrix4T &op ) const;
	Matrix4T operator - ( const Matrix4T &op ) const;
	void operator += ( const Matrix4T &op );
	void operator -= ( const Matrix4T &op );

  bool operator == (const Matrix4T &op) const {return _orientation==op._orientation && _position==op._position;}
  bool operator != (const Matrix4T &op) const {return _orientation!=op._orientation || _position!=op._position;}
  
	__forceinline Vector3T Rotate( Vector3TPar op ) const {return Vector3T(VRotate,*this,op);}
	__forceinline Vector3T FastTransform( Vector3TPar op ) const {return Vector3T(VFastTransform,*this,op);}
	__forceinline Vector3T FastTransformA( Vector3TPar op ) const {return Vector3T(VFastTransformA,*this,op);}
	__forceinline Vector3T operator *( Vector3TPar op ) const {return Vector3T(VMultiply,*this,op);}
	__forceinline Matrix4T InverseRotation() const {return Matrix4T(MInverseRotation,*this);}
	__forceinline Matrix4T InverseScaled() const {return Matrix4T(MInverseScaled,*this);}
	__forceinline Matrix4T InverseGeneral() const {return Matrix4T(MInverseGeneral,*this);}
	//Matrix4T NormalTransform() const {return Matrix4T(MNormalTransform,*this);}
	// member access operators
	//const Coord *operator [] ( int i ) const {return _e[i];}
	//Coord *operator [] ( int i ) {return _e[i];}

	__forceinline Coord operator () ( int i, int j ) const {return Get(i,j);}
	__forceinline Coord &operator () ( int i, int j ) {return Set(i,j);}

	float Characteristic() const; // used in fast comparison
	bool IsFinite() const;

	// simple access to generic transformation matrix
	__forceinline const Matrix3T &Orientation() const {return _orientation;}
	void SetOrientation( const Matrix3T &m ) {_orientation=m;}
  void SetOrientationScaleOnly(float scale) {_orientation.SetScale(scale,scale,scale);}

	__forceinline const Vector3T &Position() const {return _position;}
	__forceinline const Vector3T &Direction() const {return _orientation._dir;}
	__forceinline const Vector3T &DirectionUp() const {return _orientation._up;}
	__forceinline const Vector3T &DirectionAside() const {return _orientation._aside;}
	void SetPosition( const Vector3T &v ) {_position=v;}
	void SetDirection( const Vector3T &v ) {_orientation._dir=v;}
	void SetDirectionUp( const Vector3T &v ) {_orientation._up=v;}
	void SetDirectionAside( const Vector3T &v ) {_orientation._aside=v;}

  __forceinline float Distance2(const Matrix4T &mat) const
  {
    return
    (
      _orientation.Distance2(mat._orientation)+
      _position.Distance2(mat._position)
    );
  }
	void Orthogonalize();

};

TypeIsSimple(Matrix4T);

// in fact vector is not the same as point
// some operation meaningfull on point have no mean on vector and vice versa
// unmeaningfull are:

// Point+Point
// Point DotProduct Point
// Point CrossProduct Point
// Translate vector
// and maybe some more

// it would be too complicated to build all arithmetics twice so for now
// we suppose the types are equivalent

inline Vector3T sign( Vector3TPar v )
{
	return Vector3T(fSign(v[0]),fSign(v[1]),fSign(v[2]));
}

Vector3T VectorMin( Vector3TPar a, Vector3TPar b );
Vector3T VectorMax( Vector3TPar a, Vector3TPar b );

#define Limit(speed,min,max ) saturateFast(speed,min,max)

float Interpolativ( float control, float cMin, float cMax, float vMin, float vMax );
float AngleDifference( float a, float b );

void CheckMinMax( Vector3T &min, Vector3T &max, Vector3TPar val );

__forceinline void CheckMinMaxInline( Vector3T &min, Vector3T &max, Vector3TPar val )
{
	saturateMin(min[0],val[0]),saturateMax(max[0],val[0]);
	saturateMin(min[1],val[1]),saturateMax(max[1],val[1]);
	saturateMin(min[2],val[2]),saturateMax(max[2],val[2]);
}

void SaturateMin( Vector3T &min, Vector3TPar val );
void SaturateMax( Vector3T &min, Vector3TPar val );

// it can improve semantics if we strictly distinguish these two types in source

__forceinline Matrix3T Vector3T::Tilda() const {return Matrix3T(MTilda,*this);}

__forceinline void Vector3T::SetRotate( const Matrix4T &a, Vector3TPar v )
{
	SetMultiply(a.Orientation(),v);
}

inline float Vector3T::SetPerspectiveProject( const Matrix4T &a, Vector3TPar o )
{
	// optimize: suppose that matrix is scaled and shifted perspective projection, i.e.
	// member [3,2] is 1.0
	// zero members:
	// [0,1], [0,3],
	// [1,0], [1,3], [2,0], [2,1], [2,2]
	// [3,0], [3,1], [3,3]

	//note: [0,0] and [1,1] need not be 1,
	Coord oow=coord(1)/o.Get(2);
	Set(0) = a.Get(0,2)+a.Get(0,0)*o.Get(0)*oow;
	Set(1) = a.Get(1,2)+a.Get(1,1)*o.Get(1)*oow;
	Set(2) = a.Get(2,2)+a.GetPos(2)*oow;
	return oow;
}

__forceinline void Vector3T::SetMultiply( const Matrix4T &a, Vector3TPar o )
{
	// same as SetFastTransform, but inlined
	float r0=a.Get(0,0)*o[0]+a.Get(0,1)*o[1]+a.Get(0,2)*o[2]+a.GetPos(0);
	float r1=a.Get(1,0)*o[0]+a.Get(1,1)*o[1]+a.Get(1,2)*o[2]+a.GetPos(1);
	float r2=a.Get(2,0)*o[0]+a.Get(2,1)*o[1]+a.Get(2,2)*o[2]+a.GetPos(2);
	Set(0)=r0;
	Set(1)=r1;
	Set(2)=r2;
}

__forceinline void Vector3T::SetMultiply( const Matrix3T &a, Vector3TPar o )
{ // vector rotation - only 3x3 matrix is used, translation is ignored
	// u=M*v
	float o0=o[0],o1=o[1],o2=o[2];
	Set(0)=a.Get(0,0)*o0+a.Get(0,1)*o1+a.Get(0,2)*o2;
	Set(1)=a.Get(1,0)*o0+a.Get(1,1)*o1+a.Get(1,2)*o2;
	Set(2)=a.Get(2,0)*o0+a.Get(2,1)*o1+a.Get(2,2)*o2;
}

class Matrix4P;
class Matrix3P;

Matrix4T ConvertPToT( const Matrix4P &m );
Matrix4P ConvertTToP( const Matrix4T &m );

Matrix3T ConvertPToT( const Matrix3P &m );
Matrix3P ConvertTToP( const Matrix3T &m );


#endif


