#ifndef Vector3H
#define Vector3H
#include "Simul/Math/Align.h"
#include "Simul/Math/Export.h"
#include <iostream>

#ifdef _MSC_VER
	
	#pragma warning(push)
	#pragma warning(disable:4201)	// Disable the warning for nameless struct {float x,y,z,w;} in union
#endif

namespace simul
{
	namespace math
	{
		class Vector;
		class VirtualVector;
		class Matrix4x4;
		class Matrix;
		class Quaternion;


		/// A 3-element vector class
		SIMUL_MATH_EXPORT_CLASS Vector3
		{
		public:
			union
			{
				struct {float x,y,z,w;};
				float Values[4];
			};
			class BadSize{};
			class BadNumber{};
			Vector3();								///< Default constructor, values are set to zero.
			Vector3(const Vector3 &v);				///< Copy constructor.
			Vector3(float a,float b,float c);		///< Constructor, values are set to (a,b,c).
			Vector3(const float *x);				///< Constructor, values are set to (x[0],x[1],x[2]).
			
			operator const float*() const
			{
				return Values;
			}
			operator float*()
			{
				return Values;
			}
			void Define(float x,float y,float z);	///< Set the values to (x,y,z).
			void DefineValues(const float *x);			///< Set the values to (x[0],x[1],x[2]).
			void Zero();							///< Set all values to zero.

		/// Return true if any value is non-zero.
			bool NonZero() const;
		/// Return the address of the array of values. In most implementations, equal to the address of the Vector3 (the this pointer).
			float *FloatPointer(unsigned idx) const
			{
				return &(((float*)Values)[idx]);
			}
		/// Return the value at index index, which should be 0, 1 or 2.
			inline float &operator() (unsigned index)
			{
		#ifdef CHECK_MATRIX_BOUNDS
				if(index>=3)
					throw BadSize();
		#endif
				return Values[index];
			}
		/// Return the value at index index, which should be 0, 1 or 2.
			inline float operator() (unsigned index) const
			{                
		#ifdef CHECK_MATRIX_BOUNDS
				if(index>=3)
					throw BadSize();
		#endif
				return Values[index];
			}
			inline float &operator[] (unsigned index)
			{
		#ifdef CHECK_MATRIX_BOUNDS
				if(index>=3)
					throw BadSize();
		#endif
				return Values[index];
			}
			void operator=(const Vector3 &v);  
		/// Is this vector equal to v?
			bool operator==(const Vector3 &v)
			{
				return(Values[0]==v[0]&&
					Values[1]==v[1]&&
					Values[2]==v[2]);
			} 
			void operator-=(const Vector3 &v);		///< Subtract v from this.
			void operator+=(const Vector3 &v);		///< Add v to this.
			void operator-=(float f);				///< Subtract f from each element.
			void operator+=(float f);				///< Add f to each element.
			void operator*=(float f);				///< Multiply this vector by the scalar f.
			Vector3 operator-() const;				///< The negative of this vector.
			void operator/=(float f);				///< Divide this vector by the scalar f.
			void operator/=(const Vector3 &v);		///< Divide each component of the vector by the corresponding component of v.
			bool operator>(float R) const;			///< Is the magnitude of the vector greater than scalar R?
			float Magnitude() const;			//< Magnitude, or length of the vector.
			float SquareSize() const;			//< Square of the &Magnitude - slightly faster to calculate.

		//! Add the first three elements of Vector v to this - v should have 3 or more elements.
			void AddLarger(const Vector &v);
		//! Scale the vector so its magnitude is unity.
			bool Unit();
		//! Scale the vector so its magnitude is unity.
			bool Normalize();
			bool ContainsNAN() const;
			void CheckNAN() const;
		// Calculation operators
			friend Vector3 SIMUL_MATH_EXPORT_FN operator+(const Vector3 &v1,const Vector3 &v2);
			friend Vector3 SIMUL_MATH_EXPORT_FN operator-(const Vector3 &v1,const Vector3 &v2);
		#ifndef SIMD
			friend float SIMUL_MATH_EXPORT_FN operator*(const Vector3 &v1,const Vector3 &v2);
		#else
			friend SIMUL_MATH_EXPORT_FN __forceinline float operator*(const Vector3 &v1,const Vector3 &v2)
			{
				float temp;
				__asm
				{
					mov eax,v1
					movss xmm0,[eax]
					movss xmm1,[eax+4]
					movss xmm2,[eax+8]
					mov ecx,v2
					mulss xmm0,[ecx]
					mulss xmm1,[ecx+4]
					mulss xmm2,[ecx+8]
					addss xmm0,xmm1
					addss xmm0,xmm2
					movss temp,xmm0
				}
				return temp;
			}
		#endif
			friend float SIMUL_MATH_EXPORT_FN operator*(const Vector3 &v1,const Vector &v2);  
			friend Vector3 SIMUL_MATH_EXPORT_FN operator*(float d,const Vector3 &v2);
			friend Vector3 SIMUL_MATH_EXPORT_FN operator*(const Vector3 &v1,float d);
			friend Vector3 SIMUL_MATH_EXPORT_FN operator/(const Vector3 &v1,float d);
			friend Vector3 SIMUL_MATH_EXPORT_FN operator/(const Vector3 &v1,const Vector3 &v2);		///< Divide each component of v1 by the corresponding component of v2.
			friend Vector3 SIMUL_MATH_EXPORT_FN operator^(const Vector3 &v1,const Vector3 &v2);
			friend bool SIMUL_MATH_EXPORT_FN operator!=(Vector3 &v1,Vector3 &v2);
		/// Let R equal X1 plus X2.
			friend void SIMUL_MATH_EXPORT_FN Add(Vector3 &R,const VirtualVector &X1,const VirtualVector &X2);
		/// Let R equal X1 minus X2.
			friend void SIMUL_MATH_EXPORT_FN Subtract(Vector3 &R,const Vector3 &X1,const VirtualVector &X2);
			friend void SIMUL_MATH_EXPORT_FN Add(Vector3 &R,const Vector3 &X1,const Vector3 &X2);
		/// Let R equal X1 minus X2.
			friend void SIMUL_MATH_EXPORT_FN Subtract(Vector3 &R,const Vector3 &X1,const Vector3 &X2);
			friend void SIMUL_MATH_EXPORT_FN Reflect(Vector3 &R,const Vector3 &N,const Vector3 &D);
		/// Multiply each element of V1 by the corresponding element of V2, putting the result into ret.
			friend void SIMUL_MATH_EXPORT_FN MultiplyElements(Vector3 &ret,const Vector3 &V1,const Vector3 &V2);
			friend void SIMUL_MATH_EXPORT_FN SwapVector3(Vector3 &V1,Vector3 &V2);
			friend bool SIMUL_MATH_EXPORT_FN DistanceCheck(const Vector3 &X1,const Vector3 &X2,float R);
		/// Add f*V1 to V2.
			friend void SIMUL_MATH_EXPORT_FN AddFloatTimesVector(Vector3 &V2,float f,const Vector3 &V1);
			friend void SIMUL_MATH_EXPORT_FN AddFloatTimesVector(Vector3 &V3,const Vector3 &V2,const float f,const Vector3 &V1);
		/// Dot product of D with (X1-X2) - slightly faster than doing two dot products and subtracting.
			friend float SIMUL_MATH_EXPORT_FN DotDifference(const Vector3 &D,const Vector3 &X1,const Vector3 &X2);
		/// Vector cross product of v1 and v2.
			friend void SIMUL_MATH_EXPORT_FN CrossProduct(Vector3 &result,const Vector3 &v1,const Vector3 &v2);
			friend void SIMUL_MATH_EXPORT_FN CrossProduct(Vector3 &result,const VirtualVector &v1,const VirtualVector &v2);
			friend void SIMUL_MATH_EXPORT_FN AddCrossProduct(Vector3 &result,const Vector3 &v1,const Vector3 &v2);
			friend void SIMUL_MATH_EXPORT_FN AddDoubleCross(Vector3 &result,const Vector3 &v1,const Vector3 &v2);
			friend void SIMUL_MATH_EXPORT_FN Precross(Matrix4x4 &M,const Vector3 &v);
			friend void SIMUL_MATH_EXPORT_FN AddDoublePrecross(Matrix4x4 &M,float f,const Vector3 &v);
			friend void SIMUL_MATH_EXPORT_FN SubtractDoubleCross(Vector3 &result,const Vector3 &v1,const Vector3 &v2);                                    
			friend void SIMUL_MATH_EXPORT_FN SubtractDoublePrecross(Matrix4x4 &M,float f,const Vector3 &v);
			friend bool SIMUL_MATH_EXPORT_FN FiniteLineIntercept(Vector3 &i,const Vector3 &surf_x,const Vector3 &surf_n,const Vector3 &line_x,const Vector3 &line_d);
			friend bool SIMUL_MATH_EXPORT_FN FiniteLineInterceptInclusive(Vector3 &i,const Vector3 &surf_x,const Vector3 &surf_n,const Vector3 &line_x,const Vector3 &line_d,float Tolerance);
			friend bool SIMUL_MATH_EXPORT_FN FiniteLineIntercept(Vector3 &i,const Vector3 &surf_x,const Vector3 &surf_n,const Vector3 &line_x,const float l,const Vector3 &line_d) ;
			friend void SIMUL_MATH_EXPORT_FN Intercept(Vector3 &i,const Vector3 &surf_x,const Vector3 &surf_n,const Vector3 &line_x,const Vector3 &line_d);
			friend float SIMUL_MATH_EXPORT_FN LineInterceptPosition(Vector3 &i,const Vector3 &surf_x,const Vector3 &surf_n,const Vector3 &line_x,const Vector3 &line_d);
			friend void SIMUL_MATH_EXPORT_FN UnitRateVector(Vector3 &Rate,Vector3 &Sized,Vector3 &SizedRate);
			friend void SIMUL_MATH_EXPORT_FN InsertVectorAsRow(Matrix &M,const Vector3 &v,const unsigned row);
			friend void SIMUL_MATH_EXPORT_FN InsertVectorAsColumn(Matrix &M,const Vector3 &v,const unsigned row);
			friend void SIMUL_MATH_EXPORT_FN InsertNegativeVectorAsColumn(Matrix &M,const Vector3 &v,const unsigned row);
			friend void SIMUL_MATH_EXPORT_FN ColumnToVector(const Matrix4x4 &M,Vector3 &V,unsigned Row);
			friend void SIMUL_MATH_EXPORT_FN RowToVector(const Matrix4x4 &M,Vector3 &V,unsigned Row);
			friend void SIMUL_MATH_EXPORT_FN AddVectorToRow(Matrix &M,unsigned Row,Vector3 &V);
			friend float SIMUL_MATH_EXPORT_FN MatrixRowTimesVector(const Matrix &M,const unsigned Row,const Vector3 &V);
			friend void SIMUL_MATH_EXPORT_FN RowToVector(const Matrix &M,Vector3 &V,unsigned Row);
			friend void SIMUL_MATH_EXPORT_FN MultiplyAndSubtract(Vector3 &v2,const Matrix &m,const Vector3 &v1);
			friend void SIMUL_MATH_EXPORT_FN Multiply3(Vector3 &v2,const Matrix4x4 &M,const Vector3 &v1); 
			friend void SIMUL_MATH_EXPORT_FN Multiply4(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);   
			friend void SIMUL_MATH_EXPORT_FN TransposeMultiply4(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);
		/// Multiply Matrix M by Vector3 V1, put the result in Vector3 V2. The Matrix must be of height and width 3.
			friend void SIMUL_MATH_EXPORT_FN Multiply(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);
			friend void SIMUL_MATH_EXPORT_FN Multiply(Vector3 &V2,const Matrix &M,const Vector3 &V1);
		/// Multiply V1 by f, put the result in V2.
			friend void SIMUL_MATH_EXPORT_FN Multiply(Vector3 &V2,const float f,const Vector3 &V1);
		/// Multiply Quaternion q by v, put the result in ret.
			friend void SIMUL_MATH_EXPORT_FN Multiply(Vector3 &ret,const Quaternion &q,const Vector3 &v);
			friend void SIMUL_MATH_EXPORT_FN MultiplyAndAdd(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);
			friend void SIMUL_MATH_EXPORT_FN TransposeMultiply(Vector3 &v2,const Matrix &m,const Vector3 &v1);
			friend void SIMUL_MATH_EXPORT_FN MultiplyAndSubtract(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);

			friend void SIMUL_MATH_EXPORT_FN AddQuaternionTimesVector(Vector3 &ret,const Quaternion &q,const Vector3 &v);
			friend void SIMUL_MATH_EXPORT_FN Divide(Vector3 &ret,const Quaternion &q,const Vector3 &v);

		/// Project position vector X onto the surface defined by position S and normal N.
			friend void SIMUL_MATH_EXPORT_FN VectorOntoSurface(Vector3 &X,const Vector3 &S,const Vector3 &N);
		/// Resolve vector X onto the surface through the origin with normal N.
			friend void SIMUL_MATH_EXPORT_FN VectorOntoOriginPlane(Vector3 &X,const Vector3 &N);
		/// The distance from position X to the nearest point on the surface S,N.
			friend float SIMUL_MATH_EXPORT_FN HeightAboveSurface(Vector3 &X,const Vector3 &S,const Vector3 &N);
		/// Multipy the transpose of M by V1, putting the result in V2, but using only the first 3 rows and columns of M.
			friend void SIMUL_MATH_EXPORT_FN TransposeMultiply3x3(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);
		/// The largest x y and z of two vectors.
			friend Vector3 SIMUL_MATH_EXPORT_FN Max(const Vector3 &v1,const Vector3 &v2);
		/// The smallest x y and z of two vectors.
			friend Vector3 SIMUL_MATH_EXPORT_FN Min(const Vector3 &v1,const Vector3 &v2);
		/// Linearly interpolate between two vectors.
			friend Vector3 SIMUL_MATH_EXPORT_FN Lerp(float x,const Vector3 &v1,const Vector3 &v2);
		/// Catmull-Rom interpolate with parameter \a t along a spline formed by vectors \a x0 , \a x1 , \a x2 and \a x3 .
			friend Vector3 SIMUL_MATH_EXPORT_FN CatmullRom(float t,const Vector3 &x0,const Vector3 &x1,const Vector3 &x2,const Vector3 &x3);
			friend void SIMUL_MATH_EXPORT_FN ClosestApproach(float &alpha1,float &alpha2,const Vector3 &X1,const Vector3 &D1,const Vector3 &X2,const Vector3 &D2);
			friend SIMUL_MATH_EXPORT class Matrix;
			friend SIMUL_MATH_EXPORT class Quaternion;     
			friend SIMUL_MATH_EXPORT class Vector;
		#ifdef PLAYSTATION_2
		}  __attribute__((aligned (16)));  
		#else
	};
	#endif
	//------------------------------------------------------------------------------
	inline float DotProduct3(const Vector3 &v1,const Vector3 &v2)
	{
	#ifndef SIMD
	#ifndef PLAYSTATION2
		float total=0.0f;
		for(unsigned i=0;i<3;i++)
			total+=v1[i]*v2[i];
		return total;
	#else
		register float d;
		asm __volatile__("  
			.set noreorder
			lwc1		$f1, 0(%1)			// load v1[X]
			lwc1		$f2, 0(%2)			// load v2[X]
			lwc1		$f3, 4(%1)			// load v1[Y]
			lwc1		$f4, 4(%2)			// load v2[Y]
			lwc1		$f5, 8(%1)			// load v1[Z]
			lwc1		$f6, 8(%2)			// load v2[Z]
			mula.s	$f1, $f2				// ACC = v1[X] * v2[X]
			madda.s	$f3, $f4				// ACC += v1[Y] * v2[Y]
			madd.s	%0, $f5, $f6		// rv = ACC + v1[Z] * v2[Z]
		"					: "=f" (d)
							:"r" (v1.Values), "r" (v2.Values)
							: );
		return d;
	#endif
	#else
		/*float temp;
			__asm
			{
			// Load v1's float address into eax
				mov eax,v1//t1
				mov eax,[eax+4]
			// Load v2's float address into ebx
				mov ebx,v2//t2   
				mov ebx,[ebx+4]
				movss xmm0,[eax]
				movss xmm1,[eax+4]
				movss xmm2,[eax+8]
				mulss xmm0,[ebx]
				mulss xmm1,[ebx+4]
				mulss xmm2,[ebx+8]
			// Multiply with v2's data, and store the result in xmm0
				addss xmm0,xmm1
				addss xmm0,xmm2
			// Put the contents of xmm0[0] back into v1.
				movss temp,xmm0
			}
			return temp;*/
			float temp;
			__asm
			{
			// Load v1's float address into eax
				mov eax,v1
			// Load v2's float address into ebx
				mov ebx,v2 
			// Load v1's data into xmm0
				movaps xmm0,[eax]
			// Multiply with v2's data, and store the result in xmm0
				mulps xmm0,[ebx]
			// Now add the four sums together:
				movhlps xmm1,xmm0
				addps xmm0,xmm1         // = X+Z,Y+W,--,--
				movaps xmm1,xmm0
				shufps xmm1,xmm1,1      // Y+W,--,--,--
				addss xmm0,xmm1
			// Put the contents of xmm0[0] back into v1.
				movss temp,xmm0
			}
			return temp;
	#endif
	}

	extern SIMUL_MATH_EXPORT std::ostream &operator<<(std::ostream &, Vector3 const &);
	extern SIMUL_MATH_EXPORT std::istream &operator>>(std::istream &, Vector3 &);
}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif
