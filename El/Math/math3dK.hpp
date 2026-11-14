#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATH3DK_HPP
#define _MATH3DK_HPP

#include "mathDefs.hpp"

// homogenous vector and matrix arithmetics
// (C) 1997, SUMA
// KNI version

#if _DEBUG
  #define ASSERT_XMM(x) Assert( ((int)(x)&15)==0 )
  #define ASSERT_THIS_XMM() ASSERT_XMM(this)
#else
  #define ASSERT_XMM(x)
  #define ASSERT_THIS_XMM()
#endif

#if _M_IX86_FP>0 || _M_X64
// SSE implementation

# include <xmmintrin.h>

#if 1
# define Vector3KVal const Vector3K &
# define Vector3KPar const Vector3K &
# define FloatQuadPar const FloatQuad &
# define FloatQuadVal const FloatQuad &
# define PASS_IN_REGISTER
#else
// passing by value would be nice, however it seems compiler does not want to do it for SSE
# define Vector3KVal const Vector3K &
# define Vector3KPar Vector3K
# define FloatQuadPar const FloatQuad &
# define FloatQuadVal FloatQuad
# define PASS_IN_REGISTER
#endif

#elif _M_PPC

# include <ppcintrinsics.h>
# include <Es/Common/win.h>
# include <xboxmath.h>
  //@{ simulate SSE data types using PPC counterparts
  typedef __vector4 __m128;
  //@}
  
  //@{ simulate some common SSE intrinsics using direct PPC counterparts
  // TODO: use PPC intrinsic instead of XMVector functions
# define _mm_set_ps1(x) XMVectorReplicate(x)
# define _mm_set_ps(a,b,c,d) XMVectorSet(d,c,b,a)
# define _mm_add_ps(a,b) XMVectorAdd(a,b)
# define _mm_sub_ps(a,b) XMVectorSubtract(a,b)
# define _mm_mul_ps(a,b) XMVectorMultiply(a,b)
# define _mm_div_ps(a,b) XMVectorMultiply(a,XMVectorReciprocal(b))
# define _mm_min_ps(a,b) XMVectorMin(a,b)
# define _mm_max_ps(a,b) XMVectorMax(a,b)
# define _mm_setzero_ps() XMVectorZero()
  //@}
  
# if 0 // passing by value preferred, however it is causing ICE in 5632 XDK with debug configuration
#   define Vector3KVal const Vector3K &
#   define Vector3KPar const Vector3K &
#   define FloatQuadPar const FloatQuad &
#   define FloatQuadVal const FloatQuad &
#   define PASS_IN_REGISTER
# else
#   define Vector3KVal Vector3K
#   define Vector3KPar Vector3K
#   define FloatQuadPar FloatQuad
#   define FloatQuadVal FloatQuad
#   define PASS_IN_REGISTER __declspec(passinreg)
# endif

#endif

#include <math.h>
#include <float.h>
#include <Es/Common/fltopts.hpp>

#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/typeOpts.hpp>

#include "mathOpt.hpp"

// use parameter placeholder to explicitly disable initialization of elements

//#define vecAlign __declspec(align(16))
#define vecAlign

PASS_IN_REGISTER class Vector3K;
class Matrix3K;
class Matrix4K;

extern const Vector3K VZeroK;
extern const Vector3K VUpK;
extern const Vector3K VForwardK;
extern const Vector3K VAsideK;

extern const Matrix3K M3ZeroK;
extern const Matrix3K M3IdentityK;

extern const Matrix4K M4ZeroK;
extern const Matrix4K M4IdentityK;

/// one float passed in all elements of the vector
PASS_IN_REGISTER class FloatQuad
{
	__m128 _quad;
	public:
  explicit __forceinline FloatQuad( const __m128 &q ):_quad(q) {}
	__forceinline FloatQuad( float f ):_quad(_mm_set_ps1(f)) {}
	__forceinline operator const __m128 &() const {return _quad;}
	/// we want FloatQuad to be usable everywhere where float can be used
	/** often we want the conversion to float to be done as late as possible.
	This avoids situation where we would have to convert back to FloatQuad again, like in Normalize
	This is particularly helpful on X360 VMX, because vector operations have the same latency as scalat ones there
	 */
	__forceinline operator float () const
	{
	  // return the first element
	  return *(float *)&_quad;
	}
};

#ifdef _M_PPC
__forceinline FloatQuad CreateFloatQuadConst(float x)
{
  static const __vector4 xq={x,x,x,x};
  return FloatQuad(xq);
}
#define FloatQuadConst(x) CreateFloatQuadConst(x)
#else
#define FloatQuadConst(x) FloatQuad(x)
#endif

//@{ implement all operators used commonly on float for FloatQuad as well
/** note:
on PC using _mm_mul_ss would be faster on SSE, however it would lead to incorrect results
currently we are not using K arithmetics on the PC anyway
*/

__forceinline FloatQuad operator - (FloatQuadPar a)
{
  #ifdef _M_PPC
  return FloatQuad(XMVectorNegate(a));
  #else
  return FloatQuadConst(0)-a;
  #endif
}

__forceinline FloatQuad operator * (FloatQuadPar a, FloatQuadPar b) {return FloatQuad(_mm_mul_ps(a,b));}
__forceinline FloatQuad operator * (FloatQuadPar a, float b) {return float(a)*float(b);}
__forceinline FloatQuad operator * (float a, FloatQuadPar b) {return float(a)*float(b);}

__forceinline FloatQuad operator + (FloatQuadPar a, FloatQuadPar b) {return FloatQuad(_mm_add_ps(a,b));}
__forceinline float operator + (FloatQuadPar a, float b) {return float(a)+float(b);}
__forceinline float operator + (float a, FloatQuadPar b) {return float(a)+float(b);}

__forceinline FloatQuad operator - (FloatQuadPar a, FloatQuadPar b) {return FloatQuad(_mm_sub_ps(a,b));}
__forceinline FloatQuad operator - (FloatQuadPar a, float b) {return float(a)-float(b);}
__forceinline FloatQuad operator - (float a, FloatQuadPar b) {return float(a)-float(b);}

/** no vector division on VMX - we need to return a scalar */

__forceinline float operator / (FloatQuadPar a, FloatQuadPar b) {return float(a)/float(b);}
__forceinline float operator / (FloatQuadPar a, float b) {return float(a)/float(b);}
__forceinline float operator / (float a, FloatQuadPar b) {return float(a)/float(b);}

static __forceinline  FloatQuad Square( FloatQuadPar x ) {return x*x;}


#ifdef _M_PPC // X360 specific optimizations
__forceinline __m128 SelLE (FloatQuadPar s, FloatQuadPar b)
{
  //return _mm_cmple_ps(s,b);
  return XMVectorLessOrEqual(s,b);
}

/// optimized inverse square root
static inline FloatQuad InvSqrt( FloatQuadPar x )
{
  FloatQuad rs = FloatQuad(__vrsqrtefp(x));
  return (FloatQuadConst(3.0f) - rs * rs * x) * rs * FloatQuadConst(0.5f);
}
#endif


/// vector - "Katmai" implementation
/**
Katmai was renamed to SSE and other implementations (X360 PowerPC VMX) are now defined here as well.
The name however stays.
*/
PASS_IN_REGISTER class Vector3K
{
	friend class Matrix4K;
	friend class Matrix3K;
	
	// 3D type - used for rendering, screen clipping ...
	private:
	__m128 _k;
	#define _k_e ( (float *)&_k )
	// array elements access
	//__forceinline const float *E() const {return (float *)&e;}
	//__forceinline float *E() {return (float *)&_k;}
	
	protected:
	__forceinline Coord Get( int i ) const {return _k_e[i];}
	__forceinline Coord &Set( int i ) {return _k_e[i];}
	
	public:
	__forceinline void Init() {ASSERT_THIS_XMM();_k_e[3]=0;} // init 4th component

	// data initializers
	void SetMultiply( const Matrix3K &a, Vector3KPar v );
	void SetMultiplyLeft( Vector3KPar v, const Matrix3K &a );

	void SetRotate( const Matrix4K &a, Vector3KPar v );
	void SetFastTransform( const Matrix4K &a, Vector3KPar v );
	void SetMultiply( const Matrix4K &a, Vector3KPar v );

	float SetPerspectiveProject( const Matrix4K &a, Vector3KPar o );
	

	// interoperability between __m128 and Vector3K types
	__forceinline Vector3K( const __m128 &src )
	{
		ASSERT_THIS_XMM();
		_k = src;
	}
  /*
	__forceinline Vector3K( const Vector3K &src )
	{
		ASSERT_THIS_XMM();
		_k = src._k;
	}

	__forceinline void operator =( const __m128 &src )
	{
		ASSERT_THIS_XMM();
		_k = src;
	}
	__forceinline void operator =( const Vector3K &src )
	{
		ASSERT_THIS_XMM();
		_k = src._k;
	}
	*/

	__forceinline const __m128 &GetM128() const {return _k;}

	// avoid denormals
	//#define INIT() _k=_mm_setzero_ps()
	#if _DEBUG
	#define INIT() _k=_mm_set_ps1(FLT_SNAN)
	#else
	#define INIT()
	#endif

	//#define INIT() 
	// constructors
	__forceinline Vector3K() {ASSERT_THIS_XMM();INIT();} // default no init
	Vector3K( Coord x, Coord y, Coord z ) {ASSERT_THIS_XMM();_k=_mm_set_ps(0,z,y,x);}
  Vector3K( FloatQuadPar x, FloatQuadPar y, FloatQuadPar z ) {ASSERT_THIS_XMM();_k=_mm_set_ps(0,z,y,x);}
	__forceinline Vector3K( enum _noInit ) {ASSERT_THIS_XMM();INIT();}
  __forceinline Vector3K( enum _guardInit ) {ASSERT_THIS_XMM();_k=_mm_set_ps1(FLT_SNAN);}
	__forceinline Vector3K( enum _vMultiply, const Matrix3K &a, Vector3KPar v )
	{ASSERT_THIS_XMM();INIT();SetMultiply(a,v);}
	__forceinline Vector3K( enum _vMultiplyLeft, Vector3KPar v, const Matrix3K &a )
	{ASSERT_THIS_XMM();INIT();SetMultiplyLeft(v,a);}
	__forceinline Vector3K( enum _vRotate, const Matrix4K &a, Vector3KPar v )
	{ASSERT_THIS_XMM();INIT();SetRotate(a,v);}
	__forceinline Vector3K( enum _vFastTransform, const Matrix4K &a, Vector3KPar v )
	{ASSERT_THIS_XMM();INIT();SetFastTransform(a,v);}
	__forceinline Vector3K( enum _vMultiply, const Matrix4K &a, Vector3KPar v )
	{ASSERT_THIS_XMM();INIT();SetFastTransform(a,v);}
	
	// properties
	__forceinline Coord operator [] ( int i ) const {return _k_e[i];}
	__forceinline Coord &operator [] ( int i ) {return _k_e[i];}
	
	__forceinline Coord X() const {return _k_e[0];}
	__forceinline Coord Y() const {return _k_e[1];}
	__forceinline Coord Z() const {return _k_e[2];}
	
	private:
	#ifndef _M_PPC
	friend __forceinline __m128 HorizontalSum4( __m128 a )
	{ 
		return _mm_add_ss(a,_mm_add_ss(_mm_shuffle_ps(a, a, 1),_mm_add_ss(_mm_shuffle_ps(a, a, 2),_mm_shuffle_ps(a, a, 3))));
	}
	friend __forceinline __m128 HorizontalSum3( __m128 a )
	{ 
		return _mm_add_ss(a,_mm_add_ss(_mm_shuffle_ps(a, a, 1),_mm_shuffle_ps(a, a, 2)));
	}
	#endif

	public:

	float SquareSize() const {return ((*this)*(*this));}
  __forceinline float SquareSizeInline() const {return ((*this)*(*this));}
  
	float Size() const
	{
#ifdef _M_PPC
    return XMVector3Length(_k).x;
#else
    float size2 = SquareSize();
    if( size2<=FLT_MIN ) return 0;
    Coord invSize=InvSqrt(size2);
    return size2*invSize;
#endif
	}
	float InvSize() const {return InvSqrt(SquareSize());}
	float InvSquareSize() const {return Inv(SquareSize());}

  /*
  using vector ops for 2-component dot-product has little sense.
  Such 2D operations are not much used in performance critical code anyway
  */
	float SquareSizeXZ() const {return X()*X()+Z()*Z();}
	float SizeXZ() const {return sqrt(SquareSizeXZ());}
	float InvSizeXZ() const {return InvSqrt(SquareSizeXZ());}
	float InvSquareSizeXZ() const {return Inv(SquareSizeXZ());}
	
	// vector arithmetics
	__forceinline Vector3K operator - () const
	{
		return _mm_sub_ps(_mm_setzero_ps(),_k);
	}
	__forceinline Vector3K operator + ( Vector3KPar op ) const
	{
		return _mm_add_ps(_k,op._k);
	}
	__forceinline Vector3K operator - ( Vector3KPar op ) const
	{
		return _mm_sub_ps(_k,op._k);
	}
	__forceinline Vector3K operator * ( FloatQuadPar op ) const
	{
		return _mm_mul_ps(op,_k);
	}
  __forceinline friend Vector3K operator * ( Coord f, Vector3KPar op );
  __forceinline friend Vector3K operator * ( FloatQuadPar f, Vector3KPar op );
  
	__forceinline Vector3K operator / ( Coord f ) const
	{
		return _mm_div_ps(_k,_mm_set_ps1(f));
	}
	__forceinline void operator += ( Vector3KPar op )	{_k=_mm_add_ps(_k,op._k);}
	__forceinline void operator -= ( Vector3KPar op ) {_k=_mm_sub_ps(_k,op._k);}
	__forceinline void operator *= ( FloatQuadPar f )	{_k = _mm_mul_ps(_k,f);}
	__forceinline void operator /= ( Coord f ) {_k = _mm_div_ps(_k,_mm_set_ps1(f));}

	__forceinline Vector3K Modulate( Vector3KPar op ) const {return _mm_mul_ps(_k,op._k);	}
	
	inline Vector3K NormalizedFast() const;
	Vector3K Normalized() const;
	void Normalize(); // no return to avoid using instead of Normalized
	
	/// Dot Product
	__forceinline float DotProduct( Vector3KPar op ) const
	{
#ifdef _M_PPC
    return XMVector3Dot(_k,op._k).x;
#else
    __m128 res=HorizontalSum3(_mm_mul_ps(_k,op._k));
    float ret;
    _mm_store_ss(&ret,res);
    return ret;
#endif
	}

  // same as DotProduct
	__forceinline float operator *( Vector3KPar op ) const
	{
#ifdef _M_PPC
    return XMVector3Dot(_k,op._k).x;
#else
    __m128 res=HorizontalSum3(_mm_mul_ps(_k,op._k));
    float ret;
    _mm_store_ss(&ret,res);
    return ret;
#endif
	}

	Vector3K operator * ( const Matrix3K &op ) const	{return Vector3K(VMultiplyLeft,*this,op);}
	float Distance( Vector3KPar op ) const;
	float Distance2( Vector3KPar op ) const;
	__forceinline float Distance2Inline( Vector3KPar op ) const	{return (op-*this).SquareSizeInline();}
	float DistanceXZ( Vector3KPar op ) const;
	float DistanceXZ2( Vector3KPar op ) const;
	float CosAngle( Vector3KPar op ) const;
	Vector3K CrossProduct( Vector3KPar op ) const
	{
    #ifdef _M_PPC
      return XMVector3Cross(_k,op._k);
    #else
	    float x=Y()*op.Z()-Z()*op.Y();
	    float y=Z()*op.X()-X()*op.Z();
	    float z=X()*op.Y()-Y()*op.X();
	    return Vector3K(x,y,z);
    #endif
	}
	Vector3K Project( Vector3KPar op ) const;
	Matrix3K Tilda() const;
	bool IsFinite() const;

  // TODO: comparison using SSE/VMX - combination of _mm_cmpneq_ps and dot-product could work
	bool operator == ( Vector3KPar cmp ) const {return cmp.X()==X() && cmp.Y()==Y() && cmp.Z()==Z();}
	bool operator != ( Vector3KPar cmp ) const {return cmp.X()!=X() || cmp.Y()!=Y() || cmp.Z()!=Z();}

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

TypeIsSimpleZeroed(Vector3K);

class Matrix3K
{
	friend class Vector3K;
	friend class Matrix4K;
	
	// homogeneous matrix - transformations
	private:
	vecAlign Vector3K _aside;
	vecAlign Vector3K _up;
	vecAlign Vector3K _dir;

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
  void SetRotationAxis(Vector3KPar axis, Coord angle);
	void SetScale( Coord x, Coord y, Coord z );

  void SetScale( float scale );
  /// get squared average scale (some vectors can be after transformation longer)
  float Scale2() const;
  /// get squared maximal scale
  float MaxScale2() const;
  // optimization calculate scale and maxscale at once
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

	void SetDirectionAndUp( Vector3KPar dir, Vector3KPar up ); // sets only 3x3 submatrix
	void SetUpAndAside( Vector3KPar up, Vector3KPar aside );
	void SetUpAndDirection( Vector3KPar up, Vector3KPar dir );
	void SetDirectionAndAside( Vector3KPar dir, Vector3KPar aside );

	void SetMultiply( const Matrix3K &a, const Matrix3K &b );
	void SetMultiply( const Matrix3K &a, FloatQuadPar f );

	__forceinline void InlineSetMultiply( const Matrix3K &src, FloatQuadPar f )
	{
		_aside = src._aside * f;
		_up = src._up * f;
		_dir = src._dir * f;
	}
	__forceinline void InlineAddMultiply( const Matrix3K &src, FloatQuadPar f )
	{
		_aside += src._aside * f;
		_up += src._up * f;
		_dir += src._dir * f;
	}
	__forceinline void InlineSetAdd( const Matrix3K &a, const Matrix3K &b )
	{
		_aside = a._aside + b._aside;
		_up = a._up + b._up;
		_dir = a._dir + b._dir;
	}

	void SetInvertRotation( const Matrix3K &op );
	void SetInvertScaled( const Matrix3K &op );
	void SetInvertGeneral( const Matrix3K &op );
	void SetNormalTransform( const Matrix3K &op );
	void SetTilda( Vector3KPar a );
	
	// placeholder parameter describes constructor type
	__forceinline Matrix3K(){} // default no init
	Matrix3K( Vector3KPar aside, Vector3KPar up, Vector3KPar dir )
	:_aside(aside),_up(up),_dir(dir){}
	
	Matrix3K
	(
		float m00, float m01, float m02,
		float m10, float m11, float m12,
		float m20, float m21, float m22
	)
	{
		SetDirectionAside(Vector3K(m00,m10,m20));
		SetDirectionUp(Vector3K(m01,m11,m21));
		SetDirection(Vector3K(m02,m12,m22));
		//Set(0,0)=m00,Set(0,1)=m01,Set(0,2)=m02;
		//Set(1,0)=m10,Set(1,1)=m11,Set(1,2)=m12;
		//Set(2,0)=m20,Set(2,1)=m21,Set(2,2)=m22;
	}
	__forceinline Matrix3K( enum _noInit ){}
  __forceinline Matrix3K( enum _guardInit ):_aside(GuardInit),_up(GuardInit),_dir(GuardInit){}
	__forceinline Matrix3K( enum _mRotationX, Coord angle ){SetRotationX(angle);}
	__forceinline Matrix3K( enum _mRotationY, Coord angle ){SetRotationY(angle);}
	__forceinline Matrix3K( enum _mRotationZ, Coord angle ){SetRotationZ(angle);}
  __forceinline Matrix3K( enum _mRotationAxis, Vector3KPar axis, Coord angle ) {SetRotationAxis(axis,angle);}
	__forceinline Matrix3K( enum _mScale, Coord x, Coord y, Coord z ){SetScale(x,y,z);}
	__forceinline Matrix3K( enum _mScale, Coord x ){SetScale(x,x,x);}
	__forceinline Matrix3K( enum _mDirection, Vector3KPar dir, Vector3KPar up )
	{
		SetDirectionAndUp(dir,up);
	}
	__forceinline Matrix3K( enum _mUpAndDirection, Vector3KPar dir, Vector3KPar up )
	{
		SetUpAndDirection(dir,up);
	}
	__forceinline Matrix3K( enum _mMultiply, const Matrix3K &a, const Matrix3K &b ){SetMultiply(a,b);}
	__forceinline Matrix3K( enum _mMultiply, const Matrix3K &a, float op ){SetMultiply(a,op);}
	__forceinline Matrix3K( enum _mInverseRotation, const Matrix3K &a ){SetInvertRotation(a);}
	__forceinline Matrix3K( enum _mInverseGeneral, const Matrix3K &a ){SetInvertGeneral(a);}
	__forceinline Matrix3K( enum _mInverseScaled, const Matrix3K &a ){SetInvertScaled(a);}
	__forceinline Matrix3K( enum _mNormalTransform, const Matrix3K &a ){SetNormalTransform(a);}
	__forceinline Matrix3K( enum _mTilda, Vector3KPar a ){SetTilda(a);}
	
	// following operators are defined so that no copy constuctor is used
	// if they are expanded inline, copy is not needed
	__forceinline Matrix3K operator * ( const Matrix3K &op ) const
	{
		// matrix multiplication
		return Matrix3K(MMultiply,*this,op);
	}
	__forceinline Vector3K operator * ( Vector3KPar op ) const
	{
		// vector transformation
		return Vector3K(VMultiply,*this,op);
	}
	__forceinline Matrix3K operator * ( float op ) const
	{
		return Matrix3K(MMultiply,*this,op);
	}
	__forceinline void InlineMultiply( Matrix3K &dst, float op ) const
	{
		dst._aside = _aside*op;
		dst._up = _up*op;
		dst._dir = _dir*op;
	}
	void operator *= ( float op );
	Matrix3K operator + ( const Matrix3K &a ) const;
	Matrix3K operator - ( const Matrix3K &a ) const;
	Matrix3K &operator += ( const Matrix3K &a );
	Matrix3K &operator -= ( const Matrix3K &a );
  bool operator ==(const Matrix3K &op) const {return _aside==op._aside && _up==op._up && _dir==op._dir;}
  bool operator !=(const Matrix3K &op) const {return _aside!=op._aside || _up!=op._up || _dir!=op._dir;}
	
	Matrix3K InverseRotation() const {return Matrix3K(MInverseRotation,*this);}
	Matrix3K InverseGeneral() const {return Matrix3K(MInverseGeneral,*this);}
	Matrix3K InverseScaled() const {return Matrix3K(MInverseScaled,*this);}
	// member access operators
	__forceinline Coord operator () ( int i, int j ) const {return Get(i,j);}
	__forceinline Coord &operator () ( int i, int j ) {return Set(i,j);}

	bool IsFinite() const;

  /**/
	// simple access to generic transformation matrix
  __forceinline Vector3KVal Direction() const {return _dir;}
	__forceinline Vector3KVal DirectionUp() const {return _up;}
	__forceinline Vector3KVal DirectionAside() const {return _aside;}
	__forceinline void SetDirection( Vector3KPar v ) {_dir=v;}
	__forceinline void SetDirectionUp( Vector3KPar v ) {_up=v;}
	__forceinline void SetDirectionAside( Vector3KPar v ) {_aside=v;}
	/**/

	void Orthogonalize();
  bool IsOrthogonal ( Vector3K *sqrSize ) const;

  /// Transformation matrix is non-mandatory, if NULL, no transform is performed
  void SetCovarianceCenter ( const AutoArray<Vector3K> &data, const Matrix4K *m =NULL );

  bool SetEigenStandard ( const Matrix3K &cov, Coord *eig =NULL );

  __forceinline float Distance2(const Matrix3K &mat) const
  {
    return (
      _dir.Distance2(mat._dir)+
      _up.Distance2(mat._up)+
      _aside.Distance2(mat._aside)
    );
  }
  /// calculate matrix determinant
  float Determinant() const;
  /// calculate determinant of upper-left 2x2 submatrix
  float Determinant2x2() const
  {
    return Get(0,0)*Get(1,1)-Get(1,0)*Get(0,1);
  }
};

TypeIsSimpleZeroed(Matrix3K);

class Matrix4K
{
	friend class Vector4;
	friend class Vector3K;
	
	// homogenous matrix - transformations
	private:
	vecAlign Matrix3K _orientation;
	vecAlign Vector3K _position;

	__forceinline Coord Get( int i, int j ) const {return _orientation.Get(i,j);}
	__forceinline Coord &Set( int i, int j ) {return _orientation.Set(i,j);}

	__forceinline Coord GetPos( int i ) const	{return _position.Get(i);}
	__forceinline Coord &SetPos( int i ) {return _position.Set(i);}
	
	public:
	// functions that load matrix with data
	// used internaly in constuctors, but may be useful also to other purpose
	void SetIdentity()
	{
		_orientation.SetIdentity();
		_position=VZeroK;
	}
	void SetZero()
	{
		_orientation.SetZero();
		_position=VZeroK;
	}
	void SetTranslation( Vector3KPar offset );
	void SetRotationX( Coord angle );
	void SetRotationY( Coord angle );
	void SetRotationZ( Coord angle );
  void SetRotationAxis(Vector3KPar axis, Coord angle) {_orientation.SetRotationAxis(axis,angle);_position=VZeroK;}
	void SetScale( Coord x, Coord y, Coord z );
	//void SetPerspective( Coord cNear, Coord cFar, Coord cLeft, Coord cTop );
	void SetPerspective( Coord cLeft, Coord cTop );

	// sets only 3x3 submatrix
	__forceinline void SetDirectionAndUp( Vector3KPar dir, Vector3KPar up )
	{
		_orientation.SetDirectionAndUp(dir,up);
	}
	__forceinline void SetUpAndAside( Vector3KPar up, Vector3KPar aside )
	{
		_orientation.SetUpAndAside(up,aside);
	}
	__forceinline void SetUpAndDirection( Vector3KPar up, Vector3KPar dir )
	{
		_orientation.SetUpAndDirection(up,dir);
	}
	__forceinline void SetDirectionAndAside( Vector3KPar dir, Vector3KPar aside )
	{
		_orientation.SetDirectionAndAside(dir,aside);
	}

	__forceinline void SetScale( float scale ){_orientation.SetScale(scale);}
  /// get average scale  (some vectors can be after transformation longer)
	__forceinline float Scale() const {return _orientation.Scale();}
	__forceinline float InvScale() const {return _orientation.InvScale();}
  /// get maximal scale
  __forceinline float MaxScale() const {return _orientation.MaxScale();}
  /// optimalization calculate scale and maxScale at once
  __forceinline void ScaleAndMaxScale(float& scale, float& maxScale) const {return _orientation.ScaleAndMaxScale(scale, maxScale);}


	void SetOriented( Vector3KPar dir, Vector3KPar up ); // sets whole 4x4 matrix
	void SetView( Vector3KPar point, Vector3KPar dir, Vector3KPar up );
	void SetMultiply( const Matrix4K &a, const Matrix4K &b );
	void SetMultiply( const Matrix4K &a, FloatQuadPar f );
	void AddMultiply( const Matrix4K &a, float b );
	void SetMultiplyByPerspective( const Matrix4K &A, const Matrix4K &B );
	void SetInvertRotation( const Matrix4K &op );
	void SetInvertScaled( const Matrix4K &op );
	void SetInvertGeneral( const Matrix4K &op );
	//void SetNormalTransform( const Matrix4K &op ); // in fact result is Matrix3K
	
	// placeholder parameter describes constructor type
	__forceinline Matrix4K(){} // default no init
	__forceinline Matrix4K
	(
		Vector3KPar aside, Vector3KPar up, Vector3KPar dir, Vector3KPar pos
	)
	:_orientation(aside,up,dir),_position(pos){}
	Matrix4K
	(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23
	)
	{
		//Set(0,0)=m00,Set(0,1)=m01,Set(0,2)=m02;
		//Set(1,0)=m10,Set(1,1)=m11,Set(1,2)=m12;
		//Set(2,0)=m20,Set(2,1)=m21,Set(2,2)=m22;
		//SetPos(0)=m03,SetPos(1)=m13,SetPos(2)=m23;
		SetDirectionAside(Vector3K(m00,m10,m20));
		SetDirectionUp(Vector3K(m01,m11,m21));
		SetDirection(Vector3K(m02,m12,m22));
		SetPosition(Vector3K(m03,m13,m23));
	}
	__forceinline Matrix4K( enum _mIdentity ){SetIdentity();}
	__forceinline Matrix4K( enum _noInit ){}
  __forceinline Matrix4K( enum _guardInit ):_orientation(GuardInit),_position(GuardInit){}
	__forceinline Matrix4K( enum _mZero ){SetZero();}
	__forceinline Matrix4K( enum _mTranslation, Vector3KPar offset ){SetTranslation(offset);}
	__forceinline Matrix4K( enum _mRotationX, Coord angle ){SetRotationX(angle);}
	__forceinline Matrix4K( enum _mRotationY, Coord angle ){SetRotationY(angle);}
	__forceinline Matrix4K( enum _mRotationZ, Coord angle ){SetRotationZ(angle);}
  __forceinline Matrix4K( enum _mRotationAxis, Vector3KPar axis, Coord angle ) {SetRotationAxis(axis,angle);}
	__forceinline Matrix4K( enum _mScale, Coord x, Coord y, Coord z ){SetScale(x,y,z);}
	__forceinline Matrix4K( enum _mScale, Coord x ){SetScale(x,x,x);}
  // Scale space according to center
  __forceinline Matrix4K( enum _mScale, Coord x, Vector3KPar center ) 
  {
    SetScale(x,x,x);
    SetPosition(center - Rotate(center));
  }
	__forceinline Matrix4K( enum _mPerspective, Coord cLeft, Coord cTop )
	{
		SetPerspective(cLeft,cTop);
	}
	__forceinline Matrix4K( enum _mDirection, Vector3KPar dir, Vector3KPar up )
	{
		SetOriented(dir,up);
	}
  __forceinline Matrix4K( enum _mUpAndDirection, Vector3KPar dir, Vector3KPar up )
  {
    SetUpAndDirection(dir,up);
    SetPosition(VZeroK);
  }
	__forceinline Matrix4K( enum _mView, Vector3KPar point, Vector3KPar dir, Vector3KPar up )
	{
		SetView(point,dir,up);
	}
	__forceinline Matrix4K( enum _mMultiply, const Matrix4K &a, const Matrix4K &b ){SetMultiply(a,b);}
	__forceinline Matrix4K( enum _mMultiply, const Matrix4K &a, FloatQuadPar f ){SetMultiply(a,f);}
	__forceinline Matrix4K( enum _mInverseRotation, const Matrix4K &a ){SetInvertRotation(a);}
	__forceinline Matrix4K( enum _mInverseScaled, const Matrix4K &a ){SetInvertScaled(a);}
	__forceinline Matrix4K( enum _mInverseGeneral, const Matrix4K &a ){SetInvertGeneral(a);}
	//Matrix4K( enum _mNormalTransform, const Matrix4K &a ){SetNormalTransform(a);}

	// following operators are defined so that no copy constructor is used
	// if they are expanded inline, copy is not needed
	__forceinline Matrix4K operator * ( const Matrix4K &op ) const
	{
		// matrix multiplication
		return Matrix4K(MMultiply,*this,op);
	}
	__forceinline Matrix4K operator * ( FloatQuadPar f ) const
	{
		// matrix multiplication
		return Matrix4K(MMultiply,*this,f);
	}
	__forceinline void InlineSetMultiply( const Matrix4K &src, FloatQuadPar f )
	{
		_orientation.InlineSetMultiply(src._orientation,f);
		_position = src._position*f;
	}
	__forceinline void InlineSetAdd( const Matrix4K &a, const Matrix4K &b )
	{
		_orientation.InlineSetAdd(a._orientation,b._orientation);
		_position = a._position+b._position;
	}
	__forceinline void InlineAddMultiply( const Matrix4K &src, FloatQuadPar f )
	{
		_orientation.InlineAddMultiply(src._orientation,f);
		_position += src._position*f;
	}

	Matrix4K operator + ( const Matrix4K &op ) const;
	Matrix4K operator - ( const Matrix4K &op ) const;
	void operator -= ( const Matrix4K &op );
	void operator += ( const Matrix4K &op );
  bool operator == (const Matrix4K &op) const {return _orientation==op._orientation && _position==op._position;}
  bool operator != (const Matrix4K &op) const {return _orientation!=op._orientation || _position!=op._position;}

	Vector3K Rotate( Vector3KPar op ) const;
	Vector3K FastTransform( Vector3KPar op ) const;
	__forceinline Vector3K operator *( Vector3KPar op ) const {return Vector3K(VMultiply,*this,op);}
	__forceinline Matrix4K InverseRotation() const {return Matrix4K(MInverseRotation,*this);}
	__forceinline Matrix4K InverseScaled() const {return Matrix4K(MInverseScaled,*this);}
	__forceinline Matrix4K InverseGeneral() const {return Matrix4K(MInverseGeneral,*this);}
	//Matrix4K NormalTransform() const {return Matrix4K(MNormalTransform,*this);}
	// member access operators
	//const Coord *operator [] ( int i ) const {return _k_e[i];}
	//Coord *operator [] ( int i ) {return _k_e[i];}

	__forceinline Coord operator () ( int i, int j ) const {return Get(i,j);}
	__forceinline Coord &operator () ( int i, int j ) {return Set(i,j);}

	float Characteristic() const; // used in fast comparison
	bool IsFinite() const;

	// simple access to generic transformation matrix
	__forceinline const Matrix3K &Orientation() const {return _orientation;}
	void SetOrientation( const Matrix3K &m ) {_orientation=m;}
  void SetOrientationScaleOnly(float scale) {_orientation.SetScale(scale,scale,scale);}

	__forceinline Vector3KVal Position() const {return _position;}
	__forceinline Vector3KVal Direction() const {return _orientation._dir;}
	__forceinline Vector3KVal DirectionUp() const {return _orientation._up;}
	__forceinline Vector3KVal DirectionAside() const {return _orientation._aside;}
  __forceinline void SetPosition( Vector3KPar v ) {_position=v;}
	__forceinline void SetDirection( Vector3KPar v ) {_orientation._dir=v;}
	__forceinline void SetDirectionUp( Vector3KPar v ) {_orientation._up=v;}
	__forceinline void SetDirectionAside( Vector3KPar v ) {_orientation._aside=v;}

	void Orthogonalize();

  __forceinline float Distance2(const Matrix4K &mat) const
  {
    return _orientation.Distance2(mat._orientation)+_position.Distance2(mat._position);
  }
};

TypeIsSimpleZeroed(Matrix4K);

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

inline Vector3K sign( Vector3KPar v )
{
	return Vector3K(fSign(v[0]),fSign(v[1]),fSign(v[2]));
}

inline Vector3K VectorMin( Vector3KPar a, Vector3KPar b ){return _mm_min_ps(a.GetM128(),b.GetM128());}

inline Vector3K VectorMax( Vector3KPar a, Vector3KPar b ){return _mm_max_ps(a.GetM128(),b.GetM128());}

#define Limit(speed,min,max) saturateFast(speed,min,max)

float Interpolativ( float control, float cMin, float cMax, float vMin, float vMax );
float AngleDifference( float a, float b );

inline void CheckMinMax( Vector3K &minV, Vector3K &maxV, Vector3KPar val )
{
  minV = _mm_min_ps(minV.GetM128(),val.GetM128());
  maxV = _mm_max_ps(maxV.GetM128(),val.GetM128());
}

inline void SaturateMin( Vector3K &minV, Vector3KPar val )
{
  minV = _mm_min_ps(minV.GetM128(),val.GetM128());
}
inline void SaturateMax( Vector3K &maxV, Vector3KPar val )
{
  maxV = _mm_max_ps(maxV.GetM128(),val.GetM128());
}

__forceinline void CheckMinMaxInline( Vector3K &minV, Vector3K &maxV, Vector3KPar val )
{
	minV = _mm_min_ps(minV.GetM128(),val.GetM128());
	maxV = _mm_max_ps(maxV.GetM128(),val.GetM128());
}

__forceinline Matrix3K Vector3K::Tilda() const {return Matrix3K(MTilda,*this);}

__forceinline void Vector3K::SetRotate( const Matrix4K &a, Vector3KPar v )
{
	SetMultiply(a.Orientation(),v);
}

inline float Vector3K::SetPerspectiveProject( const Matrix4K &a, Vector3KPar o )
{
	// optimize: suppose that matrix is scaled and shifted perspective projection, i.e.
	// member [3,2] is 1.0
	// zero members:
	// [0,1], [0,3],
	// [1,0], [1,3], [2,0], [2,1], [2,2]
	// [3,0], [3,1], [3,3]

	//note: [0,0] and [1,1] need not be 1,
	// TODO: optimize for KNI
	Coord oow=coord(1)/o.Get(2);
	Set(0) = a.Get(0,2)+a.Get(0,0)*o.Get(0)*oow;
	Set(1) = a.Get(1,2)+a.Get(1,1)*o.Get(1)*oow;
	Set(2) = a.Get(2,2)+a.GetPos(2)*oow;
	return oow;
}

#ifdef _M_PPC

inline void Vector3K::SetMultiply( const Matrix4K &a, Vector3KPar o )
{
  // matrix stored in major-column format
  // Power PC VMX optimization
  // similar to XMVector3Transform(o,a);
  XMVECTOR ox = __vspltw(o._k,0);
  XMVECTOR oy = __vspltw(o._k,1);
  XMVECTOR oz = __vspltw(o._k,2);
  /* note adding position last is numerically a little bit more stable (for large positions), however a little bit slower */
  XMVECTOR r = XMVectorMultiplyAdd(ox,a._orientation._aside._k,a._position._k);
  r = XMVectorMultiplyAdd(oy,a._orientation._up._k,r);
  _k = XMVectorMultiplyAdd(oz,a._orientation._dir._k,r);
}


#else

inline void Vector3K::SetMultiply( const Matrix4K &a, Vector3KPar o )
{
	// same as FastTransform, but inlined
	// matrix stored in major-column format
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
#endif

inline void Vector3K::SetMultiply( const Matrix3K &a, Vector3KPar o )
{ // vector rotation - only 3x3 matrix is used, translation is ignored
	// u=M*v
	// matrix stored in major-column format
	__m128 t1,t2,t3;

	t1 = _mm_set_ps1(o.X());
	t2 = _mm_set_ps1(o.Y());

	t1 = _mm_mul_ps( t1, a._aside._k );
	t2 = _mm_mul_ps( t2, a._up._k );

	t3 = _mm_set_ps1(o.Z());

	t1 = _mm_add_ps( t1, t2 );
	t3 = _mm_mul_ps( t3, a._dir._k );

	// sum a...
	_k = _mm_add_ps( t1, t3 );

}

__forceinline Vector3K operator * ( FloatQuadPar f, Vector3KPar op )
{
  return _mm_mul_ps(f,op._k);
}
__forceinline Vector3K operator * ( Coord f, Vector3KPar op )
{
  return _mm_mul_ps(FloatQuad(f),op._k);
}

#include "math3Dp.hpp"

Matrix4K ConvertPToK( const Matrix4P &m );
Matrix4P ConvertKToP( const Matrix4K &m );

Matrix3K ConvertPToK( const Matrix3P &m );
Matrix3P ConvertKToP( const Matrix3K &m );

inline Vector3P ConvertKToP( const Vector3K &v ) {return Vector3P(v.X(),v.Y(),v.Z());}
inline Vector3K ConvertPToK( const Vector3P &v ) {return Vector3K(v.X(),v.Y(),v.Z());}

inline Vector3K Vector3K::NormalizedFast() const
{
	#ifdef _M_PPC
		__vector4 sum	 = __vmsum3fp(_k, _k);
		__vector4 isqrt = __vrsqrtefp(sum);
		return __vmulfp(_k, isqrt);
	#else
		Coord invSize=InvSqrtFast(SquareSizeInline());
		return *this * invSize;
	#endif
}


#endif


