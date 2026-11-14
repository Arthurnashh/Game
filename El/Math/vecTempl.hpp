#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VEC_TEMPL_HPP
#define _VEC_TEMPL_HPP

// template vector maths
// this code is adapted and exdentded from
// http://www.flipcode.com/tutorials/tut_fastmath.shtml
// by mailto:tarce@austin.rr.com

// there are also some other
// BLITZ: http:/????
// MET: http://met.sourceforge.net/
// reference list: http://oonumerics.org/oon/

#pragma warning(disable:4786)

///////////////////////////////////////////////////////////////////////////
// VECTOR
///////////////////////////////////////////////////////////////////////////
namespace vt
{ // using namespace: usefull to hide operator + from conflicting

// short name is used to avoid warning about long names as much as possible

///////////////////////////////////////////////////////////////////////////
// ARGUMENTS
///////////////////////////////////////////////////////////////////////////

// templates together with namespaces lead to very long symbols

template< class ta_a >
class Arg
{
    const ta_a& Argv;
public:
		enum {Dimension=ta_a::Dimension};
    __forceinline Arg( const ta_a& A ) : Argv( A ) {}
    __forceinline const float Evaluate( const int i ) const 
    { return Argv.Evaluate( i ); }
};

/*
// float to vector propagation - sometimes may be very usefull,
// but is also quite dangerous
template<> 
class Arg< const float >
{
    const ta_a& Argv;
public:
    __forceinline Arg( const ta_a& A ) : Argv( A ) {}
    __forceinline const float Evaluate( const int i ) const 
    { return Argv; }
};
*/

struct Mul;
struct MulFV;

// note: it should be possible to pass scalar type as template argument
template< int ta_dimension>
class EBase;

// vector by float
template< class ta_a, class ta_eval >
class Exp2vf;

template< class ta_a, class ta_eval >
class Exp2vf
{
    const Arg<ta_a>   Arg1;
    const float   Arg2;

public:
		enum {Dimension=ta_a::Dimension};
    __forceinline Exp2vf( const ta_a& A1, const float& A2 ) : Arg1( A1 ), Arg2( A2 ) {}
    __forceinline const float Evaluate ( const int i ) const
    { return ta_eval::Evaluate( i, Arg1, Arg2 ); }
};

// float by vector
template< class ta_b, class ta_eval >
class Exp2fv
{
    const float   Arg1;
    const Arg<ta_b>   Arg2;

public:
		enum {Dimension=ta_b::Dimension};
    __forceinline Exp2fv( const float& A1, const ta_b& A2 ) : Arg1( A1 ), Arg2( A2 ) {}
    __forceinline const float Evaluate ( const int i ) const
    { return ta_eval::Evaluate( i, Arg1, Arg2 ); }
};

template< class ta_a, class ta_eval >
class vecexp_1
{
    const Arg<ta_a>   Arg1;

public:
		enum {Dimension=ta_a::Dimension};
    __forceinline vecexp_1( const ta_a& A1 ) : Arg1( A1 ) {}

    __forceinline const float Evaluate( const int i ) const
    { return ta_eval::Evaluate( i, Arg1.Evaluate( i ) ); }
};


///////////////////////////////////////////////////////////////////////////
// EXPRESSIONS
///////////////////////////////////////////////////////////////////////////

// handling of general member function on any expression supporting "Evaluate"

template <int ta_dimension>
struct Func
{
	//enum {ta_dimension=VecType::Dimension};

  //////////////////////////////////////////////////////////////////
  // DOT PRODUCT
  //////////////////////////////////////////////////////////////////
  template< class ta, class tb >
  struct dot_prod
  {
      template< int I, class R, int Dummy=0 >
      struct recurse
      {
          enum { COUNTER = I+1 };

          static __forceinline float Eval( const ta& A, const tb& B ) 
          {
              return A.Evaluate( I ) * B.Evaluate( I ) +
                  recurse<COUNTER,int>::Eval( A, B );
          }
      };

      template<int Dummy> struct recurse<ta_dimension,int,Dummy>
      {
          static __forceinline float Eval( const ta& A, const tb& B )
          {
              return 0;
          }
      };

      static __forceinline float Eval( const ta& A, const tb& B )
      {
          return recurse<0,int>::Eval( A, B );
      }
  };

  //////////////////////////////////////////////////////////////////
  // EQUAL PRODUCT
  //////////////////////////////////////////////////////////////////
  template< class ta, class tb >
  struct equal
  {
      template< int I, class R, int Dummy=0 >
      struct recurse
      {
          enum { COUNTER = I+1 };

          static __forceinline bool Eval( const ta& A, const tb& B ) 
          {
              return A.Evaluate( I )==B.Evaluate( I ) && 
                  recurse<COUNTER,int>::Eval( A, B );
          }
      };

      template<int Dummy> struct recurse<ta_dimension,int,Dummy>
      {
          static __forceinline bool Eval( const ta& A, const tb& B )
          {
              return true;
          }
      };

      static __forceinline bool Eval( const ta& A, const tb& B )
      {
          return recurse<0,int>::Eval( A, B );
      }
  };

  //////////////////////////////////////////////////////////////////
  // MODULUS (SQUARE SIZE)
  //////////////////////////////////////////////////////////////////
  template< class ta>
  struct modulus
  {
      template< int I, class R, int Dummy=0 >
      struct recurse
      {
          enum { COUNTER = I+1 };

          static __forceinline float Eval( const ta& A) 
          {
						float t = A.Evaluate( I ); 
              return t*t + recurse<COUNTER,int>::Eval( A);
          }
      };

      template<int Dummy> struct recurse<ta_dimension,int,Dummy>
      {
          static __forceinline float Eval( const ta& A)
          {
              return 0;
          }
      };

      static __forceinline float Eval( const ta& A)
      {
          return recurse<0,int>::Eval(A);
      }
  };

  //////////////////////////////////////////////////////////////////
  // MODULUS (SQUARE SIZE)
  //////////////////////////////////////////////////////////////////
  template< class ta>
  struct xzmodulus
  {
		// modulus of members 0,2,4,...
      template< int I, class R, int Dummy=0 >
      struct recurse
      {
          enum { COUNTER = I+2 };

          static __forceinline float Eval( const ta& A) 
          {
						float t = A.Evaluate( I ); 
              return t*t + recurse<COUNTER,int>::Eval( A);
          }
      };

      template<int Dummy> struct recurse<ta_dimension,int,Dummy>
      {
          static __forceinline float Eval( const ta& A)
          {
              return 0;
          }
      };
			enum {ta_dimension1=ta_dimension+1};
      template<int Dummy> struct recurse<ta_dimension1,int,Dummy>
      {
          static __forceinline float Eval( const ta& A)
          {
              return 0;
          }
      };

      static __forceinline float Eval( const ta& A)
      {
          return recurse<0,int>::Eval(A);
      }
  };

  template< class ta>
  struct normalized
  {
    static Exp2vf<const EBase<3>,MulFV> Eval( const ta& A);
  };
};



template< class ta_a, class ta_eval >
class Exp1: Func<ta_a::Dimension>
{
  const Arg<ta_a> Arg1;

public:
	//note: we assume ta_a is same dimension as ta_b dimension
	enum {Dimension=ta_a::Dimension};
  __forceinline Exp1( const ta_a& A1) : Arg1( A1 ) {}
  __forceinline const float Evaluate ( const int i ) const
  { return ta_eval::Evaluate( i, Arg1 ); }
};

template< class ta_a, class ta_b, class ta_eval >
class Exp2: Func<ta_a::Dimension>
{
  const Arg<ta_a> Arg1;
  const Arg<ta_b> Arg2;

public:
  typedef Func<ta_a::Dimension> base;

	//note: we assume ta_a is same dimension as ta_b dimension
	enum {Dimension=ta_a::Dimension};
  __forceinline Exp2( const ta_a& A1, const ta_b& A2 ) : Arg1( A1 ), Arg2( A2 ) {}
  __forceinline const float Evaluate ( const int i ) const
  { return ta_eval::Evaluate( i, Arg1, Arg2 ); }


	// access to recursive functions
  float SquareSize() const {return base:: template modulus<Exp2>::Eval(*this);}
  __forceinline float SquareSizeInline() const {return base::template modulus<Exp2>::Eval(*this);}

  float SquareSizeXZ() const {return xzmodulus<Exp2>::Eval(*this);}

	float Size() const
	{
		float size2=SquareSizeXZ();
		if( size2==0 ) return 0;
		float invSize=InvSqrt(size2);
		return size2*invSize;
	}
	float SizeXZ() const
	{
		float size2=SquareSizeInline();
		if( size2==0 ) return 0;
		float invSize=InvSqrt(size2);
		return size2*invSize;
	}
	float InvSize() const
	{
		float size2=SquareSizeXZ();
		if( size2==0 ) return 0;
		return InvSqrt(size2);
	}

	// normalization
	// note: some functions need to be implemented without recursion
	// it might introduce some problems

  template< class ta_type >
  __forceinline float DotProduct( const ta_type& A ) const
  {
    return dot_prod<Exp2,ta_type>::Eval( *this, A );
  }

  template< class ta_type >
	float CosAngle( const ta_type &op ) const
	{
	  // it might be more efficient to evaluate both sides first, and compute dot + modulus later
	  // implementation may evaluate both expressions twice
		Assert(ta_a::Dimension==3);
		Assert(ta_type::Dimension==3);
		return DotProduct(op)*InvSqrt(op.SquareSizeInline()*SquareSizeInline());
	}

  template< class ta_type >
	EBase<3> CrossProduct( const ta_type &op ) const
	{
	  //Vector3T ret;
		Assert(ta_a::Dimension==3);
		Assert(ta_type::Dimension==3);
		float thisX = Evaluate(0);
		float thisY = Evaluate(1);
		float thisZ = Evaluate(2);
		float opX = op.Evaluate(0);
		float opY = op.Evaluate(1);
		float opZ = op.Evaluate(2);
	  EBase<3> ret;
	  ret.Set(0) = thisY*opZ-thisZ*thisY;
	  ret.Set(1) = thisZ*opX-thisX*thisZ;
	  ret.Set(2) = thisX*opY-thisY*thisX;
	  return ret;
	}
	
  __forceinline Exp2vf<const EBase<3>,MulFV> Normalized() const;
  
  

  template< class ta_c2 >
  __forceinline const Exp2< const Exp2, const ta_c2, Mul > Modulate( const ta_c2& Pb ) const
  {
    return Exp2< const Exp2, const ta_c2, Mul >( *this, Pb );
  }

};


template <int ta_dimension>
template <class ta>
__forceinline Exp2vf<const EBase<3>,MulFV> Func<ta_dimension>::normalized<ta>::Eval( const ta& A)
{
  // first convert to 3 floats, to avoid recomputing multiple times
  // TODO: handle any dimension
  EBase<3> op;
  op.Set(0) = A.Evaluate(0);
  op.Set(1) = A.Evaluate(1);
  op.Set(2) = A.Evaluate(2);
  
  Coord size2=op.SquareSizeInline();
  Coord invSize=size2<=FLT_MIN ? 1.0f : InvSqrt(size2);
  return op * invSize;
}  


///////////////////////////////////////////////////////////////////////////
// BASE CLASS
///////////////////////////////////////////////////////////////////////////

// note: it should be possible to pass scalar type as template argument
template<int ta_dimension>
class EBase: public Func<ta_dimension>
{
	protected:
	float _e[ta_dimension];

	public:

		enum {Dimension=ta_dimension};

    __forceinline const float &Get(const int i ) const {return _e[i];}
    __forceinline float&Set(const int i ) {return _e[i];} 
    __forceinline float&  operator[]( const int i ) {return _e[i];} 
    __forceinline const float &operator[] ( const int i ) const {return _e[i];} 
    __forceinline const float Evaluate( const int i ) const {return _e[i];}

    //////////////////////////////////////////////////////////////////
    // ASSIGMENT
    //////////////////////////////////////////////////////////////////
    template<class ta>
    struct Assignment
    {
        //template< int I, class R>
        template< int I, class R >
        struct recurse
        {
            enum { COUNTER = I+1 };

            static __forceinline void Assign
						(
							EBase<ta_dimension>& V, const ta& A,
							void Operator(float &a, const float &b)
						) 
            {
                //V[I] = A.Evaluate( I );
                Operator(V[I],A.Evaluate( I ));
                recurse<COUNTER,int>::Assign( V, A, Operator);
            }
        };

        template<> struct recurse<ta_dimension,int>
        {
            static __forceinline void Assign
						(
							EBase<ta_dimension>& V, const ta& A, 
							void Operator(float &a, const float &b)
						)
						{}
        };

        static __forceinline void Assign
				(
					EBase<ta_dimension>& V, const ta& A,
					void Operator(float &a, const float &b)
				)
        {
            recurse<0,int>::Assign( V, A, Operator );
        }
    };

		// different kinds of assignement

		// operator =
		__forceinline static void AssignFloat(float &a, const float &b) {a=b;}

    template< class ta_type > __forceinline
    const EBase<ta_dimension>& operator = ( const ta_type& A )
    {
        Assignment<ta_type>::Assign( *this, A, AssignFloat);
        return *this;
    }

		// operator +=
		__forceinline static void AssignAddFloat(float &a, const float &b) {a+=b;}

    template< class ta_type > __forceinline
    const EBase<ta_dimension>& operator += ( const ta_type& A )
    {
        Assignment<ta_type>::Assign( *this, A, AssignAddFloat);
        return *this;
    }

		// operator -=
		__forceinline static void AssignSubFloat(float &a, const float &b) {a-=b;}

    template< class ta_type > __forceinline
    const EBase<ta_dimension>& operator -= ( const ta_type& A )
    {
        Assignment<ta_type>::Assign( *this, A, AssignSubFloat);
        return *this;
    }

		// operator *=
		/*
		__forceinline static void AssignMulFloat(float &a, const float &b) {a*=b;}

    const EBase<ta_dimension>& operator *= ( const float &A )
    {
        Assignment<EBase<ta_dimension> >::Assign( *this, A, AssignMulFloat);
        return *this;
    }
		*/

//////////////////////////////////////////////////////////
// move to include

	// access to recursive functions
  __forceinline float SquareSize() const {return modulus<EBase>::Eval(*this);}
  __forceinline float SquareSizeInline() const {return modulus<EBase>::Eval(*this);}

  __forceinline float SquareSizeXZ() const {return xzmodulus<EBase>::Eval(*this);}


  template< class ta_type > __forceinline
  bool operator !=( const ta_type& A ) const
  {
      return !equal<EBase,ta_type>::Eval( *this, A );
  }

  template< class ta_type > __forceinline
  bool operator ==( const ta_type& A ) const
  {
      return equal<EBase,ta_type>::Eval( *this, A );
  }


	float Size() const
	{
		float size2=SquareSizeInline();
		if( size2==0 ) return 0;
		float invSize=InvSqrt(size2);
		return size2*invSize;
	}
	float SizeXZ() const
	{
		float size2=SquareSizeXZ();
		if( size2==0 ) return 0;
		float invSize=InvSqrt(size2);
		return size2*invSize;
	}
	float InvSize() const
	{
		float size2=SquareSizeInline();
		if( size2==0 ) return 0;
		return InvSqrt(size2);
	}

	// normalization
	// note: some functions need to be implemented without recursion
	// it might introduce some problems

  template< class ta_type > __forceinline
  float DotProduct( const ta_type& A ) const
  {
      return dot_prod<EBase,ta_type>::Eval( *this, A );
  }

  template< class ta_type >
	float CosAngle( const ta_type &op ) const
	{
		Assert(ta_dimension==3);
		return DotProduct(op)*InvSqrt(op.SquareSizeInline()*SquareSizeInline());
	}


  __forceinline Exp2vf<const EBase<3>,MulFV> Normalized() const;

};

template< class ta_a, class ta_b, class ta_eval >
__forceinline Exp2vf<const EBase<3>,MulFV> Exp2<ta_a,ta_b,ta_eval>::Normalized() const
{
  return normalized<Exp2>::Eval(*this);
}

template<int ta_dimension>
__forceinline Exp2vf<const EBase<3>,MulFV> EBase<ta_dimension>::Normalized() const
{
  return normalized<EBase>::Eval(*this);
}

/*
template< int ta_dimension>
class EEBase: public EFunc<EBase<ta_dimension>,ta_dimension>
{
};
*/

// base extended with operators

///////////////////////////////////////////////////////////////////////////
// ADD
///////////////////////////////////////////////////////////////////////////

struct Add
{
    template< class ta_a, class ta_b > __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const ta_b& B )
    { return A.Evaluate(i) + B.Evaluate(i); }
};

template< class ta_c1, class ta_c2 > __forceinline 
const Exp2< const ta_c1, const ta_c2, Add > 
operator + ( const ta_c1& Pa, const ta_c2& Pb )
{
    return Exp2< const ta_c1, const ta_c2, Add >( Pa, Pb );
}

///////////////////////////////////////////////////////////////////////////
// VMUL
///////////////////////////////////////////////////////////////////////////

struct Mul
{
    template< class ta_a, class ta_b > __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const ta_b& B )
    { return A.Evaluate(i) * B.Evaluate(i); }
};

///////////////////////////////////////////////////////////////////////////
// Neg
///////////////////////////////////////////////////////////////////////////

struct Neg
{
    template< class ta_a> __forceinline static 
    const float Evaluate( const int i, const ta_a& A)
    { return -A.Evaluate(i); }
};

template< class ta_c1 > __forceinline 
const Exp1< const ta_c1, Neg > 
operator - ( const ta_c1& Pa)
{
    return Exp1< const ta_c1, Neg >( Pa );
}

///////////////////////////////////////////////////////////////////////////
// SUB
///////////////////////////////////////////////////////////////////////////

struct Sub
{
    template< class ta_a, class ta_b > __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const ta_b& B )
    { return A.Evaluate(i) - B.Evaluate(i); }
};

template< class ta_c1, class ta_c2 > __forceinline 
const Exp2< const ta_c1, const ta_c2, Sub > 
operator - ( const ta_c1& Pa, const ta_c2& Pb )
{
    return Exp2< const ta_c1, const ta_c2, Sub >( Pa, Pb );
}

///////////////////////////////////////////////////////////////////////////
// MUL
///////////////////////////////////////////////////////////////////////////

struct MulFV
{
    template< class ta_a> __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const float &B )
    { return A.Evaluate(i) * B; }
};

template< class ta_c1> __forceinline 
const Exp2vf< const ta_c1, MulFV > 
operator * ( const ta_c1& Pa, const float &Pb )
{
    return Exp2vf< const ta_c1, MulFV >( Pa, Pb );
}

struct MulVF
{
    template< class ta_b> __forceinline static 
    const float Evaluate( const int i, const float& A, const ta_b &B )
    { return A * B.Evaluate(i); }
};

template< class ta_c1> __forceinline 
const Exp2fv< const ta_c1, MulVF > 
operator * ( const float& Pa, const ta_c1 &Pb )
{
    return Exp2fv< const ta_c1, MulVF >( Pa, Pb );
}

/*
struct mul
{
    template< class ta_a, class ta_b > __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const ta_b& B )
    { return A.Evaluate(i) * B.Evaluate(i); }
};

template< class ta_c1, class ta_c2 > __forceinline 
const Exp2< const ta_c1, const ta_c2, mul > 
operator * ( const ta_c1& Pa, const ta_c2& Pb )
{
    return Exp2< const ta_c1, const ta_c2, mul >( Pa, Pb );
}
*/

///////////////////////////////////////////////////////////////////////////
// DIV
///////////////////////////////////////////////////////////////////////////

struct DivFV
{
    template< class ta_a> __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const float &B )
    { return A.Evaluate(i) / B; }
};

template< class ta_c1> __forceinline 
const Exp2vf< const ta_c1, DivFV > 
operator / ( const ta_c1& Pa, const float &Pb )
{
    return Exp2vf< const ta_c1, DivFV >( Pa, Pb );
}

/*
struct DivVF
{
    template< class ta_b> __forceinline static 
    const float Evaluate( const int i, const float& A, const ta_b &B )
    { return A / B.Evaluate(i); }
};

template< class ta_c1> __forceinline 
const Exp2fv< const ta_c1, DivVF > 
operator / ( const float& Pa, const ta_c1 &Pb )
{
    return Exp2fv< const ta_c1, DivVF >( Pa, Pb );
}
*/

/*
struct div
{
    template< class ta_a, class ta_b > __forceinline static 
    const float Evaluate( const int i, const ta_a& A, const ta_b& B )
    { return A.Evaluate(i) * B.Evaluate(i); }
};

template< class ta_c1, class ta_c2 > __forceinline 
const Exp2< const ta_c1, const ta_c2, div > 
operator * ( const ta_c1& Pa, const ta_c2& Pb )
{
    return Exp2< const ta_c1, const ta_c2, div >( Pa, Pb );
}
*/

}; // end of namespace

#endif