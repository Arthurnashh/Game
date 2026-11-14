#ifndef MATRIX4X4H
#define MATRIX4X4H
#include "Simul/Math/Export.h"
#include "Simul/Math/Matrix.h"   
#include "Simul/Math/Align.h" 
#include <assert.h>

#ifdef WIN32
	#pragma pack(push,16)
#endif
namespace simul
{
	namespace math
	{
		class Vector;
		class Quaternion;

		/// A 4 by 4 Matrix class.

		/// A fixed-size 4 by 4 matrix class.
		/// It is used mainly for transformations - see SimulOrientation.
		SIMUL_MATH_EXPORT_CLASS Matrix4x4
		{
		protected:
			float Values[16];
		public:
			Matrix4x4();
			Matrix4x4(const Matrix4x4 &M);
			Matrix4x4(const float *x);
			Matrix4x4(	float x11,float x12,float x13,float x14,
						float x21,float x22,float x23,float x24,
						float x31,float x32,float x33,float x34,
						float x41,float x42,float x43,float x44);
			~Matrix4x4();
			class OutOfRange{};
			bool operator==(const Matrix4x4 &M)
			{
				for(unsigned i=0;i<16;i++)
				{
					if(Values[i]!=M.Values[i])
						return false;
				}
				return true;
			}
			bool operator!=(const Matrix4x4 &M)
			{
				return !(operator==(M));
			}
			operator const float*() const
			{
				return Values;
			}
			void operator+=(const Matrix4x4 &M);
			void operator-=(const Matrix4x4 &M);
			float &operator()(unsigned row,unsigned col);
			float &operator[](int pos)
			{
				assert(pos>=0&&pos<16);
				return Values[pos];
			}
			float operator[](int pos) const
			{
				assert(pos>=0&&pos<16);
				return Values[pos];
			}
			float &operator[](unsigned pos)
			{
				assert(pos>=0&&pos<16);
				return Values[pos];
			}
			float operator[](unsigned pos) const
			{
				assert(pos>=0&&pos<16);
				return Values[pos];
			}
			float operator()(unsigned row,unsigned col) const;
			const Vector3 &GetRowVector(unsigned r) const;
			void operator=(const Matrix4x4 &M);
			Matrix4x4& operator*=(float d);  
			Matrix4x4& operator/=(float d);
			void Transpose(Matrix4x4 &T) const;
			void Transpose();
			float *RowPointer(unsigned row)
			{
				assert(row>=0&&row<4);
				return &(Values[row*4]);
			}
			const float *RowPointer(unsigned row) const
			{
				assert(row>=0&&row<4);
				return &(Values[row*4]);
			}
			float *GetValues(){return Values;}
			const float *GetValues() const{return Values;}
			void Zero();				///< Set all values to zero.
			void Unit();				///< Make a unit matrix.
			void Identity();			///< Make a unit matrix.
			void Set(const float []);	///< Set the 16 values.
			bool SymmetricInverse3x3(Matrix4x4 &Result);
			bool CholeskyFactorization(Matrix4x4 &Fac);
			void InverseFromCholeskyFactors(Matrix4x4 &Result,Matrix4x4 &Fac);
			void AddScalarTimesMatrix(float val,const Matrix4x4 &m2);
			void InsertColumn(unsigned col,const Vector3 &v);
			void InsertRow(unsigned col,const Vector3 &v);
			void Inverse(Matrix4x4 &Inv) const;

			friend void SIMUL_MATH_EXPORT_FN Multiply4x4(Matrix4x4 &M,const Matrix4x4 &A,const Matrix4x4 &B);
			friend void SIMUL_MATH_EXPORT_FN Multiply3x3(Matrix4x4 &M,const Matrix4x4 &A,const Matrix4x4 &B);
			friend void SIMUL_MATH_EXPORT_FN Multiply4x4ByTranspose(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2);   
			friend void SIMUL_MATH_EXPORT_FN Multiply3x3ByTranspose(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2);   
			friend void SIMUL_MATH_EXPORT_FN MultiplyTransposeByMatrix3x3(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2);
			friend void SIMUL_MATH_EXPORT_FN QuaternionToMatrix(Matrix4x4 &m,const Quaternion &q); 
			friend void SIMUL_MATH_EXPORT_FN MatrixToQuaternion(Quaternion &q,const Matrix4x4 &m); 
			friend void SIMUL_MATH_EXPORT_FN ColumnToVector(const Matrix4x4 &M,Vector3 &V,unsigned Row);
			friend void SIMUL_MATH_EXPORT_FN RowToVector(const Matrix4x4 &M,Vector3 &V,unsigned Row);
			friend void SIMUL_MATH_EXPORT_FN TransposeMultiply4(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);  
			friend void SIMUL_MATH_EXPORT_FN Multiply3(Vector3 &v2,const Matrix4x4 &m,const Vector3 &v1);    
			friend void SIMUL_MATH_EXPORT_FN FPUQuaternionToMatrix(Matrix4x4 &m,const Quaternion &q);
			friend void SIMUL_MATH_EXPORT_FN AddDoublePrecross(Matrix4x4 &M,float f,const Vector3 &v);
			friend void SIMUL_MATH_EXPORT_FN SubtractDoublePrecross(Matrix4x4 &M,float f,const Vector3 &v);
			friend void SIMUL_MATH_EXPORT_FN Matrix::InsertMatrix(const Matrix4x4 &M,unsigned row,unsigned col);
			friend void SIMUL_MATH_EXPORT_FN Multiply(Vector3 &v2,const Matrix4x4 &m,const Vector3 &v1);
			friend void SIMUL_MATH_EXPORT_FN MultiplyAndAdd(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);
			friend void SIMUL_MATH_EXPORT_FN MultiplyAndSubtract(Matrix4x4 &m,const Matrix4x4 &m1,const Matrix4x4 &m2);
			friend void SIMUL_MATH_EXPORT_FN MultiplyAndSubtract(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1);
			friend void SIMUL_MATH_EXPORT_FN MultiplyByScalar(Matrix4x4 &result,const float f,const Matrix4x4 &m);
			friend void SIMUL_MATH_EXPORT_FN AddFloatTimesMatrix4x4(Matrix4x4 &result,const float f,const Matrix4x4 &m);
			friend void SIMUL_MATH_EXPORT_FN Precross(Matrix4x4 &M,const Vector3 &v);
		};
		extern void SIMUL_MATH_EXPORT_FN Multiply4x4(Matrix4x4 &M,const Matrix4x4 &A,const Matrix4x4 &B);
	}
}

#ifdef WIN32
#pragma pack(pop)
#endif

#endif

