#include "precomp.h"

#define SIM_MATH
#include <string.h>
#include <stdio.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include "DebugNew.h"			// Enable Visual Studio's memory debugging
#endif
#include "Simul/Base/SimMemory.h"

#include <algorithm>
#include <assert.h>

#include "Matrix4x4.h"    
#include "Vector3.h"
#include "SimVector.h"


using namespace simul;
using namespace math;

Matrix4x4::Matrix4x4()
{
}     

Matrix4x4::Matrix4x4(const Matrix4x4 &M)
{
	operator=(M);
}

Matrix4x4::Matrix4x4(const float *x)
{
	for(unsigned i=0;i<4;i++)
	{
		for(unsigned j=0;j<4;j++)
			Values[i*4+j]=x[i*4+j];
	}
	for(unsigned j=0;j<4;j++)
		Values[3*4+j]=x[3*4+j];
	Values[3*4+3]=1.f;
}

Matrix4x4::Matrix4x4(	float x11,float x12,float x13,float x14,
					float x21,float x22,float x23,float x24,
					float x31,float x32,float x33,float x34,
					float x41,float x42,float x43,float x44)
{
	operator()(0,0)=x11;
	operator()(0,1)=x12;
	operator()(0,2)=x13;
	operator()(0,3)=x14;
	operator()(1,0)=x21;
	operator()(1,1)=x22;
	operator()(1,2)=x23;
	operator()(1,3)=x24;
	operator()(2,0)=x31;
	operator()(2,1)=x32;
	operator()(2,2)=x33;
	operator()(2,3)=x34;
	operator()(3,0)=x41;
	operator()(3,1)=x42;
	operator()(3,2)=x43;
	operator()(3,3)=x44;
}
Matrix4x4::~Matrix4x4()
{
//	Values=NULL;
#ifdef CANNOT_ALIGN
//	V=NULL;
#endif
}

float &Matrix4x4::operator()(unsigned row,unsigned col)
{
#ifdef CHECK_MATRIX_BOUNDS
	if(row>=4||col>=4||row<0||col<0)
		throw OutOfRange();
#endif           
	return Values[row*4+col];
}  

float Matrix4x4::operator()(unsigned row,unsigned col) const
{
#ifdef CHECK_MATRIX_BOUNDS
	if(row>=4||col>=4||row<0||col<0)
		throw OutOfRange();
#endif
	return Values[row*4+col];
}

void Matrix4x4::operator=(const Matrix4x4 &M)
{
#ifndef SIMD
	for(unsigned i=0;i<16;i++)
		Values[i]=M.Values[i];
#else
	__asm
	{
		mov edi,this
		mov esi,M
		mov ecx,16

		rep movsd
	}
#endif
}


Matrix4x4 &Matrix4x4::operator*=(float d)
{
	register unsigned i,j;
	for (i=0;i<4;i++)
	{
		for (j=0;j<4;j++)
		{
			Values[i*4+j]*=d;
		}
	}
	return *this;
}                       

Matrix4x4 &Matrix4x4::operator/=(float d)
{
	register unsigned i,j;
	for (i=0;i<4; i++)
	{
		for (j=0;j<4;j++)
		{
			Values[i*4+j]/=d;
		}
	}
	return *this;
}       
const Vector3 &Matrix4x4::GetRowVector(unsigned r) const
{
	return *(reinterpret_cast<const Vector3 *>(&Values[r*4]));
}
void Matrix4x4::Transpose()
{
	Matrix4x4 T;
	Transpose(T);
	operator=(T);
}
void Matrix4x4::Transpose(Matrix4x4 &T) const
{
#ifdef SIMD
	_asm
	{
		mov eax,this
		movaps xmm0,[eax]	
		movaps xmm1,[eax+16]
		movaps xmm2,[eax+32]
		movaps xmm3,[eax+48]
		movaps xmm4,xmm0
		movaps xmm5,xmm2
		movaps xmm6,xmm0
		movaps xmm7,xmm2
		shufps xmm4,xmm1,0x44
		shufps xmm5,xmm3,0x44
		shufps xmm6,xmm1,0xEE
		shufps xmm7,xmm3,0xEE
		movaps xmm0,xmm4
		movaps xmm1,xmm4
		movaps xmm2,xmm6
		movaps xmm3,xmm6
		shufps xmm0,xmm5,0x88
		shufps xmm1,xmm5,0xDD
		shufps xmm2,xmm7,0x88
		shufps xmm3,xmm7,0xDD
		mov ebx,T
		movaps [ebx]	,xmm0
		movaps [ebx+16]	,xmm1
		movaps [ebx+32]	,xmm2
		movaps [ebx+48]	,xmm3
	};
/*	_MM_TRANSPOSE4_PS(row0, row1, row2, row3)
            tmp0   = _mm_shuffle_ps((row0), (row1), 0x44);          \
            tmp2   = _mm_shuffle_ps((row0), (row1), 0xEE);          \
            tmp1   = _mm_shuffle_ps((row2), (row3), 0x44);          \
            tmp3   = _mm_shuffle_ps((row2), (row3), 0xEE);          \
                                                                    \
            (row0) = _mm_shuffle_ps(tmp0, tmp1, 0x88);              \
            (row1) = _mm_shuffle_ps(tmp0, tmp1, 0xDD);              \
            (row2) = _mm_shuffle_ps(tmp2, tmp3, 0x88);              \
            (row3) = _mm_shuffle_ps(tmp2, tmp3, 0xDD);              \*/
#else
	for(unsigned i=0;i<4;i++)
	{
		for(unsigned j=0;j<4;j++)
			T.Values[i*4+j]=Values[j*4+i];
	}
#endif
}

void Matrix4x4::Zero()
{
#ifndef SIMD
	for(unsigned i=0;i<16;i++)
			Values[i]=0;
#else
	unsigned Stride=4*4;
	_asm
	{
		mov edi,this
		mov ecx,16
		xor eax,eax
		//mov edi,[ebx]
		rep stos eax
	}
#endif
}

void Matrix4x4::Unit()
{
	Zero();
	for(unsigned i=0;i<4;i++)
		Values[i*5]=1.f;
}
void Matrix4x4::Identity()
{
	Unit();
}

void Matrix4x4::Set(const float x[])
{
	for(unsigned i=0;i<16;i++)
		Values[i]=x[i];
}

void Matrix4x4::AddScalarTimesMatrix(float val,const Matrix4x4 &M2)
{
	register unsigned i,j;
	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			Values[i*4+j]+=val*M2.Values[i*4+j];
		}
	}
}

namespace simul
{
	namespace math
	{
	void AddFloatTimesMatrix4x4(Matrix4x4 &result,const float f,const Matrix4x4 &M)
	{
		register unsigned i,j;
		for(i=0;i<4;i++)
		{
			for(j=0;j<4;j++)
			{
				result.Values[i*4+j]+=f*M.Values[i*4+j];
			}
		}
	}

void Multiply4x4(Matrix4x4 &M,const Matrix4x4 &A,const Matrix4x4 &B)
{
#ifdef SIMD
	float *Result	=(float *)(M.Values);
	float *APtr		=(float *)(A.Values);
	float *BPtr		=(float *)(B.Values);
	_asm
	{
		mov ebx,dword ptr[BPtr]
		mov ecx,dword ptr[Result]
		// Load thr rows from B into xmm2,3, 4 and 5:
		movaps xmm2,[ebx]  
		movaps xmm3,[ebx+16]  
		movaps xmm4,[ebx+32]  
		movaps xmm5,[ebx+48]  
		mov ebx,4
		mov eax,dword ptr[APtr]			// Load four Values from A into xmm1
		Row_Loop:
			movaps xmm1,[eax]				// put the row into xmm1.
			movaps xmm6,[eax]		// put the in xmm6
			movaps xmm7,[eax]		// put them in xmm7
			movaps xmm0,[eax]		// put them in xmm7
			shufps xmm1,xmm1,0xFF		// xmm1 to contain the 4th element of the row
			shufps xmm6,xmm6,0			// xmm6 to contain the first element of the row
			shufps xmm7,xmm7,0x55		// xmm6 to contain the 2nd element of the row
			shufps xmm0,xmm0,0xAA		// xmm0 to contain the 3rd element of the row
			mulps xmm6,xmm2			// 1st row to be added
			mulps xmm7,xmm3			// 2st row to be added
			mulps xmm0,xmm4			// 3st row to be added
			mulps xmm1,xmm5			// 4th row to be added
			addps xmm0,xmm7
			addps xmm1,xmm6
			addps xmm1,xmm0			// total

			add ecx,16
			add eax,16

			dec ebx
			movaps [ecx-16],xmm1		// store to result row
			cmp ebx,0
		jne Row_Loop
	}                
#else
#ifdef PLAYSTATION2
	unsigned BStride=4*4;			
	unsigned MStride=4*4;
	unsigned AStride=4*4;
	unsigned HW=3;
	//unsigned BWidth=(B.Width+3)&(~0x3);
	asm __volatile__
	(
	"   
	.set noreorder
		xor t5,t5			// j=0
		blez %8,iEscape
		iLoop:
			blez %7,jEscape
			xor t8,t8
			add t4,zero,%7	// j=BWidth
			jLoop:
				lw t0,0(%0)	// M0
				lw t1,0(%1)	// A0
				lw t2,0(%2)	// B0
				mul t3,t5,%3
				add t0,t3
				add t0,t8
				mul t3,t5,%4
				add t1,t3
				add t2,t8
				blez %6,kEscape  
				add t9,zero,%6
				vsub vf10,vf00,vf00
				ble t9,3,kTail
				kLoop:
					lqc2 vf01,0(t1)			// APos
					lqc2 vf02,0(t2)			// BPos
					add t2,%5
					lqc2 vf03,0(t2)			// BPos
					add t2,%5
					lqc2 vf04,0(t2)			// BPos
					add t2,%5
					lqc2 vf05,0(t2)			// BPos
					add t2,%5
					vmulax.xyzw		ACC, vf02, vf01
					vmadday.xyzw	ACC, vf03, vf01	
					vmaddaz.xyzw	ACC, vf04, vf01	
					vmaddw.xyzw		vf01,vf05, vf01	
					vadd vf10,vf10,vf01
					addi t9,-4
				bgt t9,3,kLoop
					addi t1,16
				blez t9,kFinish		// If Width1 or height of 2 is a multiple of 4.
				kTail:
				beq t9,2,Tail2
				beq t9,3,Tail3
				nop
				Tail1:
				lqc2 vf01,0(t1)			
				lqc2 vf02,0(t2)			
				vmulx vf1,vf2,vf1x
				vadd vf10,vf10,vf1
				sqc2 vf10,0(t0)
				b kFinish
	
				Tail2:
				lqc2 vf01,0(t1)			
				lqc2 vf02,0(t2)			
				add t2,%5
				lqc2 vf03,0(t2)			
				vmulax ACC,vf2,vf1x
				vmaddy vf1,vf3,vf1y
				vadd vf10,vf10,vf1
				b kFinish
	
				Tail3:
				lqc2 vf01,0(t1)			// APos
				lqc2 vf02,0(t2)			// BPos	
				add t2,%5
				lqc2 vf03,0(t2)			
				add t2,%5
				lqc2 vf04,0(t2)		
				add t2,%5
				vmulax ACC,vf2,vf1
				vmadday	ACC,vf3,vf1	
				vmaddz vf1,vf4,vf1
				vadd vf10,vf10,vf1

				kFinish:
				sqc2 vf10,0(t0)
				kEscape:
				addi t4,-4				// j+=4
			bgtz t4,jLoop				// >0
				addi t8,16
			jEscape:
			addi t5,1				// i+=1
			sub t3,%8,t5			// AHeight-i
		bgtz t3,iLoop				// >0
		nop
		iEscape:
			.set reorder
			"	:  
				: "g"(&M),"g"(&A),"g"(&B), "g"(MStride),"g"(AStride),
					"g"(BStride),"g"(HW),"g"(HW),"g"(HW)
				:  "t1","t2","t3","t4","t5","t6","t7","t8","t9",
					"$vf1", "$vf2", "$vf3", "$vf4", "$vf5","$vf10"
			);
//    M.CheckEdges();
#else
	register unsigned i,j,k;
	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			const float *m1row=&A.Values[i*4+0];
			float t=0.f;
			k=0;
			t+=m1row[k]*B(k,j);	++k;
			t+=m1row[k]*B(k,j);	++k;
			t+=m1row[k]*B(k,j);	++k;
			t+=m1row[k]*B(k,j);
			M(i,j)=t;
		}
	}
//    M.CheckEdges();
#endif
#endif     
}

//void simul::math::Multiply4x4ByTranspose(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2)
void Multiply4x4ByTranspose(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2)
{
	unsigned i,j;
	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			float t=0.f;
			unsigned k=0;
			t+=M1.Values[i*4+k]*M2(j,k);	++k;
			t+=M1.Values[i*4+k]*M2(j,k);	++k;
			t+=M1.Values[i*4+k]*M2(j,k);
			M.Values[i*4+j]=t;
		}
	}
}
void Multiply3x3(Matrix4x4 &M,const Matrix4x4 &A,const Matrix4x4 &B)
{
#ifdef SIMD
/*	unsigned BStride		=4*4;
	unsigned MStride		=4*4;
	unsigned AStride		=4*4;
	unsigned W			=3;
	unsigned H			=3;
	unsigned Remainder	=3;
	float *Result	=(float *)(M.Values);
	float *APtr		=(float *)(A.Values);
	float *B0		=(float *)(B.Values);
	float *BPtr;*/
//	M.CheckEdges();
	_asm
	{
		mov ebx,B
		mov ecx,M
		// Load thr rows from B into xmm2,3, 4 and 5:
		movaps xmm2,[ebx]  
		movaps xmm3,[ebx+16]  
		movaps xmm4,[ebx+32]  
		movaps xmm5,[ebx+48]  
		mov ebx,3
		mov eax,A		// Load four Values from A into xmm1
		Row_Loop:
			xorps xmm1,xmm1		// put the row into xmm1.
			shufps xmm1,xmm1,0xFF	// xmm1 to contain the 4th element of the row
			movaps xmm6,[eax]		// put the row in xmm6
			shufps xmm6,xmm6,0		// xmm6 to contain the first element of the row
			movaps xmm7,[eax]		// put the row  in xmm7
			shufps xmm7,xmm7,0x55	// xmm6 to contain the 2nd element of the row
			movaps xmm0,[eax]		// put the row  in xmm7
			shufps xmm0,xmm0,0xAA	// xmm0 to contain the 3rd element of the row
			mulps xmm6,xmm2			// 1st row to be added
			mulps xmm7,xmm3			// 2st row to be added
			mulps xmm0,xmm4			// 3st row to be added
			mulps xmm1,xmm5			// 4th row to be added
			addps xmm0,xmm7
			addps xmm1,xmm6
			addps xmm1,xmm0			// total

			add ecx,16
			add eax,16

			dec ebx
			movaps [ecx-16],xmm1		// store to result row
			cmp ebx,0
		jne Row_Loop
		/*
		mov edx,H
		Row_Loop:
			mov edi,dword ptr[B0]   
			mov dword ptr[BPtr],edi
			mov edi,dword ptr[Result]
			mov esi,Remainder      
			mov ebx,dword ptr[BPtr]
			Column_Loop:
				mov eax,dword ptr[APtr]
				xorps xmm6,xmm6
				mov ecx,W
				Dot_Product_Loop:
				// Load four Values from A into xmm1
					movaps xmm1,[eax]
				// Load one value from B into xmm2
					movss xmm2,[ebx]  
				// look down one line
					add ebx,BStride
				// Load the 2nd value from B into xmm3
					movss xmm3,[ebx]  
				// look down to the 3rd line
					add ebx,BStride
				// Load the 3rd value from B into xmm4
					movss xmm4,[ebx] 
				// look down to the 4th line
					add ebx,BStride
				// Load the 4th value from B into xmm5
					movss xmm5,[ebx]    
				// now shuffle these last three Values into xmm2:
					shufps xmm2,xmm3,0x44
					shufps xmm2,xmm2,0x88
					shufps xmm4,xmm5,0x44
					shufps xmm4,xmm4,0x88
					shufps xmm2,xmm4,0x44         
   //	movups [edx],xmm2
			// Now xmm2 contains the four Values from B.
			// Multiply the four Values from A and B:
					mulps xmm1,xmm2
	//movups [edx],xmm1
			// Now add the four multiples to the four totals in xmm6:
					addps xmm6,xmm1
	//movups [edx],xmm6
					add eax,16
					add ebx,BStride
					sub ecx,4
					cmp ecx,3
				jg Dot_Product_Loop
				cmp ecx,0
				jle NoTail

				Tail_Loop:
					movss xmm1,[eax]
					movss xmm2,[ebx]
					mulss xmm1,xmm2   
					addss xmm6,xmm1
					add eax,4
					add ebx,BStride
					sub ecx,1
				jg Tail_Loop
				
				NoTail:
	//movups [edx],xmm1
	//movups [edx],xmm6
			// Now add the four sums together:
				shufps xmm1,xmm6,0xEE
	//movups [edx],xmm1
				shufps xmm1,xmm1,0xEE
	//movups [edx],xmm1
				addps xmm6,xmm1
	//movups [edx],xmm6
				shufps xmm1,xmm6,0x11
	//movups [edx],xmm1
				shufps xmm1,xmm1,0xEE
	//movups [edx],xmm1
				addss xmm6,xmm1
	//movups [edx],xmm6
	//pop edx
				movss [edi],xmm6
			// Add up the next M value
				add edi,4
			// Look at the next column of B:
				mov ebx,dword ptr[BPtr]
				add ebx,4       
				mov dword ptr[BPtr],ebx
				dec esi
				cmp esi,0
			jne Column_Loop
			mov edi,dword ptr[Result]
			mov eax,MStride
			add edi,eax
			mov dword ptr[Result],edi   
			mov edi,dword ptr[APtr]
			mov eax,AStride
			add edi,eax
			mov dword ptr[APtr],edi
			//pop edx
			dec edx
			cmp edx,0
		jne Row_Loop*/
	}                
#else
#ifdef PLAYSTATION2
	unsigned BStride=4*4;			
	unsigned MStride=4*4;
	unsigned AStride=4*4;
	unsigned HW=3;
	//unsigned BWidth=(B.Width+3)&(~0x3);
	asm __volatile__
	(
	"   
	.set noreorder
		xor t5,t5			// j=0
		blez %8,iEscape
		iLoop:
			blez %7,jEscape
			xor t8,t8
			add t4,zero,%7	// j=BWidth
			jLoop:
				lw t0,0(%0)	// M0
				lw t1,0(%1)	// A0
				lw t2,0(%2)	// B0
				mul t3,t5,%3
				add t0,t3
				add t0,t8
				mul t3,t5,%4
				add t1,t3
				add t2,t8
				blez %6,kEscape  
				add t9,zero,%6
				vsub vf10,vf00,vf00
				ble t9,3,kTail
				kLoop:
					lqc2 vf01,0(t1)			// APos
					lqc2 vf02,0(t2)			// BPos
					add t2,%5
					lqc2 vf03,0(t2)			// BPos
					add t2,%5
					lqc2 vf04,0(t2)			// BPos
					add t2,%5
					lqc2 vf05,0(t2)			// BPos
					add t2,%5
					vmulax.xyzw		ACC, vf02, vf01
					vmadday.xyzw	ACC, vf03, vf01	
					vmaddaz.xyzw	ACC, vf04, vf01	
					vmaddw.xyzw		vf01,vf05, vf01	
					vadd vf10,vf10,vf01
					addi t9,-4
				bgt t9,3,kLoop
					addi t1,16
				blez t9,kFinish		// If Width1 or height of 2 is a multiple of 4.
				kTail:
				beq t9,2,Tail2
				beq t9,3,Tail3
				nop
				Tail1:
				lqc2 vf01,0(t1)			
				lqc2 vf02,0(t2)			
				vmulx vf1,vf2,vf1x
				vadd vf10,vf10,vf1
				sqc2 vf10,0(t0)
				b kFinish
	
				Tail2:
				lqc2 vf01,0(t1)			
				lqc2 vf02,0(t2)			
				add t2,%5
				lqc2 vf03,0(t2)			
				vmulax ACC,vf2,vf1x
				vmaddy vf1,vf3,vf1y
				vadd vf10,vf10,vf1
				b kFinish
	
				Tail3:
				lqc2 vf01,0(t1)			// APos
				lqc2 vf02,0(t2)			// BPos	
				add t2,%5
				lqc2 vf03,0(t2)			
				add t2,%5
				lqc2 vf04,0(t2)		
				add t2,%5
				vmulax ACC,vf2,vf1
				vmadday	ACC,vf3,vf1	
				vmaddz vf1,vf4,vf1
				vadd vf10,vf10,vf1

				kFinish:
				sqc2 vf10,0(t0)
				kEscape:
				addi t4,-4				// j+=4
			bgtz t4,jLoop				// >0
				addi t8,16
			jEscape:
			addi t5,1				// i+=1
			sub t3,%8,t5			// AHeight-i
		bgtz t3,iLoop				// >0
		nop
		iEscape:
			.set reorder
			"	:  
				: "g"(&M),"g"(&A),"g"(&B), "g"(MStride),"g"(AStride),
					"g"(BStride),"g"(HW),"g"(HW),"g"(HW)
				:  "t1","t2","t3","t4","t5","t6","t7","t8","t9",
					"$vf1", "$vf2", "$vf3", "$vf4", "$vf5","$vf10"
			);
//    M.CheckEdges();
#else
	register unsigned i,j,k;
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			const float *m1row=&A.Values[i*4+0];
			float t=0.f;
			k=0;
			t+=m1row[k]*B(k,j);	++k;
			t+=m1row[k]*B(k,j);	++k;
			t+=m1row[k]*B(k,j);
			M(i,j)=t;
		}
	}
//    M.CheckEdges();
#endif
#endif     
}                

void Multiply3x3ByTranspose(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2)
{
	unsigned i,j;
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			float t=0.f;
			unsigned k=0;
			t+=M1.Values[i+k*4]*M2(j,k);	++k;
			t+=M1.Values[i+k*4]*M2(j,k);	++k;
			t+=M1.Values[i+k*4]*M2(j,k);
			M.Values[i*4+j]=t;
		}
	}
}

void MultiplyTransposeByMatrix3x3(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2)
{
	unsigned i,j;
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			float t=0.f;
			unsigned k=0;
			t+=M1.Values[k*4+i]*M2(k,j);	++k;
			t+=M1.Values[k*4+i]*M2(k,j);	++k;
			t+=M1.Values[k*4+i]*M2(k,j);
			M.Values[i*4+j]=t;
		}
	}
}
	}
}

void TransposeMultiply3x3(Vector3 &V2,const Matrix4x4 &M,const Vector3 &V1)
{
#ifdef SIMD
	// The Stride is the number of BYTES between successive rows:        
	unsigned MStride=4*4;
	unsigned W=(3+3)>>2;
	const unsigned H=3;
	//const float *MPtr=M.Values;
	//float *v1=(float *)(V1.Values);
	_asm
	{
		mov edx,H    
		mov edi,V2
		mov esi,M
		Row_Loop:  
			mov ebx,V1
			mov eax,esi
			xorps xmm6,xmm6
			mov ecx,W
			Dot_Product_Loop:
			// Load one value from M into xmm1
				movss xmm1,[eax]
				add eax,MStride
				movss xmm3,[eax]
				add eax,MStride
				movss xmm4,[eax]
				add eax,MStride
				movss xmm5,[eax]
			// now shuffle these last three Values into xmm1:
				shufps xmm1,xmm3,0x44
				shufps xmm1,xmm1,0x88
				shufps xmm4,xmm5,0x44
				shufps xmm4,xmm4,0x88
				shufps xmm1,xmm4,0x44
			// Load four values from V1 into xmm2
				movaps xmm2,[ebx]
			// Now xmm2 contains the four values from V1.
			// Multiply the four values from M and V1:
				mulps xmm1,xmm2
			// Now add the four multiples to the four totals in xmm6:
				addps xmm6,xmm1
			// Move eax to look at the next four values from this column of M:
				//add eax,16
				add eax,MStride
			// Move ebx to look at the next four values from V1:
				add ebx,16
				dec ecx
				cmp ecx,0
			jne Dot_Product_Loop
			// Now add the four sums together:
			shufps xmm1,xmm6,0xEE
			shufps xmm1,xmm1,0xEE
			addps xmm6,xmm1
			shufps xmm1,xmm6,0x11
			shufps xmm1,xmm1,0xEE
			addss xmm6,xmm1   
			movss [edi],xmm6
			Add edi,4
			add esi,4
			dec edx
			cmp edx,0
		jne Row_Loop
	}
#else              
	unsigned i,j;
	for(i=0;i<3;i++)
	{
		V2.Values[i]=0.f;
		for(j=0;j<3;j++)
			V2.Values[i]+=M(j,i)*V1.Values[j];
	}
#endif
}   

bool Matrix4x4::SymmetricInverse3x3(Matrix4x4 &Result)
{
	Matrix4x4 Fac;
	if(!CholeskyFactorization(Fac))
		return false;
	InverseFromCholeskyFactors(Result,Fac);
	return true;
}

bool Matrix4x4::CholeskyFactorization(Matrix4x4 &Fac)
{
	register unsigned i,j,k;
	float sum;
	for(i=0;i<3;i++)
	{
		for(j=i;j<3;j++)
		{
			sum=operator()(j,i);
			for(k=0;k<i;k++)
				sum-=Fac(i,k)*Fac(j,k);
			if(i==j)
            {
				if(sum<0)
                {
                    return false;
                }
		   		sum=Sqrt(sum);
			}
			else
			   	sum/=Fac(i,i);
			Fac(j,i)=sum;
		}
	}
	return true;
}
void Matrix4x4::Inverse(Matrix4x4 &Inv) const
{
	const Vector3 *XX=reinterpret_cast<const Vector3*>(RowPointer(0));
	const Vector3 *YY=reinterpret_cast<const Vector3*>(RowPointer(1));
	const Vector3 *ZZ=reinterpret_cast<const Vector3*>(RowPointer(2));
	Transpose(Inv);
	const Vector3 &xe=*(reinterpret_cast<const Vector3*>(RowPointer(3)));

	Inv(0,3)=0;
	Inv(1,3)=0;
	Inv(2,3)=0;
	Inv(3,0)=-((xe)*(*XX));
	Inv(3,1)=-((xe)*(*YY));
	Inv(3,2)=-((xe)*(*ZZ));
	Inv(3,3)=1.f;

}

void Matrix4x4::InverseFromCholeskyFactors(Matrix4x4 &Result,Matrix4x4 &Fac)
{
#ifndef SIMD
#ifndef PLAYSTATION2
	register unsigned i,j,k;
// Cholesky:
	float sum;
//Invert:
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			sum=0;
			if(i==j)
				sum=1.f;
			for(k=0;k<j;k++)
				sum-=Fac(j,k)*Result(i,k);
            sum/=Fac(j,j);
			Result(i,j)=sum;
        }
		for(j=3-1;j>=i;j--)
		{
			sum=Result(i,j);
			for(k=j+1;k<3;k++)
				sum-=Fac(k,j)*Result(i,k);
			sum/=Fac(j,j);
			Result(i,j)=sum;
		}
	}
	for(i=0;i<3;i++)
	{
		for(j=i+1;j<3;j++)
		{
			Result(j,i)=Result(i,j);
		}
	}
#else
	//unsigned Stride=4*4;
	unsigned TempStride=4*4;
	unsigned RStride=Result.4*4;
//Invert:
/*float sum;
    for(unsigned i=0;i<Height;i++)
	{
		for(unsigned j=0;j<Height;j++)
		{
			sum=0;
			if(i==j)
				sum=1;
			for(unsigned k=0;k<j;k++)
				sum-=Fac(j,k)*Result(i,k);
			sum/=Fac(j,j);
			Result(i,j)=sum;
		}
		for(unsigned j=Height-1;j>=i;j--)
		{
			sum=Result(i,j);
			for(unsigned k=j+1;k<Height;k++)
				sum-=Fac(k,j)*Result(i,k);
			sum/=Fac(j,j);
			Result(i,j)=sum;
		}
	}  */   
	if(Height==1)
	{
		Result.Values[0]=1.f/Fac.Values[0];
		return;
	}
	if(Height==2)
	{
		/*float sum;
		Result(0,0)=1.f/Fac(0,0);
		
		sum=-Fac(1,0)*Result(0,0);
		sum/=Fac(1,1);
		//Result(0,1)=sum;
		
		//sum=Result(0,1);
		sum/=Fac(1,1);
		Result(0,1)=sum;

		sum=Result(0,0);
		sum-=Fac(1,0)*Result(0,1);
		sum/=Fac(0,0);
		Result(0,0)=sum;

		//Result(1,0)=0.f;
		
		sum=1.f;
		sum/=Fac(1,1);
		//Result(1,1)=sum;
			
		//sum=Result(1,1);
		sum/=Fac(1,1);
		Result(1,1)=sum;

		Result(1,0)=Result(0,1);*/
		asm __volatile__
		(
		"
			.set noreorder
// All the following nonsense just to put 1.0 into fpu register f1:
			addi t1,zero,1
			mtc1 t1,$f3
			cvt.s.w $f1,$f3
			
			lwc1 f0,0(%0)
			add t0,%2,%0
			lwc1 f10,0(t0)
			lwc1 f11,4(t0)
			
			div.s f2,f1,f0
			
			sub.s f4,f2,f2
			mul.s f3,f2,f10
			div.s f3,f3,f11
			div.s f3,f3,f11
			sub.s f4,f4,f3
			
			swc1 f4,4(%1)
			
			mul.s f5,f10,f4
			sub.s f5,f2,f5
			div.s f5,f5,f0
			swc1 f5,0(%1)
			
			add t1,%1,%3
			
			div.s f6,f1,f11
			div.s f6,f6,f11
			swc1 f6,4(t1)
			
			swc1 f4,0(t1)
			
			.set reorder
		"	:
			: "g"(Fac.Values),"g"(Result.Values),"g"(TempStride),"g"(RStride)
			:
		);
		return;
	}
	asm __volatile__
	(
	"
		.set noreorder
// All the following nonsense just to put 1.0 into fpu register f1:
		addi t1,zero,1
		mtc1 t1,$f3
		cvt.s.w $f1,$f3
// All the following nonsense just to put 0.0 into fpu register f0:
		xor t0,t0
		mtc1 t0,f0
		//cvt.s.w f0,f3
//-----------------------------------------------------------------
		lw t0,0(%0)					// Fac(i,0)
		lw t1,0(%1)					// Result(i,0)
		lw t9,12(%1)				// Height
		xor s0,s0
		iLoop1:
			lw t2,0(%0)				// Fac(j,0)
			lw t3,0(%0)				// Fac(j,j)
			add t7,zero,t1			// Result(i,j)
			xor s1,s1
			jLoop1:
				lwc1 f2,0(t3)		// Fac(j,j) 	
				div.s f2,f1,f2		// f2=1/f2
				mfc1 t8,f2			// t6=f2
				ctc2 t8,$vi21		// put 1/Fac(j,j) in the immediate register I

				vsub vf1,vf0,vf0
				
				add t4,zero,t2		// Fac(j,k)
				add t5,zero,t1		// Result(i,k)
				beqz s1,NoTail
				xor s2,s2			// k=0
				ble s1,3,kEscape1
				and s4,s1,~0x3		// 4-align s1 into s4
				vsuba ACC,vf0,vf0	// ACC=0
				kLoop1:
					lqc2 vf2,0(t4)
					addi t4,16
					lqc2 vf3,0(t5)
					vmsuba ACC,vf2,vf3
					addi s2,4
				blt s2,s4,kLoop1
					addi t5,16
// Now vf1 contains 4 sums to be added together.
				vmaddx vf1,vf0,vf0x	//ACC+0,0,0,0
				vaddx vf5,vf1,vf1x
				vaddy vf5,vf5,vf1y
				vaddz vf1,vf5,vf1z
			kEscape1:
				sub s2,s1,s2
				beqz s2,NoTail
				lqc2 vf2,0(t4)
				lqc2 vf3,0(t5)		// All of next 4 values
				vmul vf2,vf2,vf3
				beq s2,1,NoTail
				vsubx vf1,vf1,vf2x	// w of vf1+ x of vf2
				beq s2,2,NoTail
				vsuby vf1,vf1,vf2y	// w of vf1+ y of vf2
				vsubz vf1,vf1,vf2z	// w of vf1+ z of vf2
			NoTail:
				
				bne s0,s1,offdiaga
				nop
				
				vadd vf1,vf1,vf0
				
				offdiaga:
				vmul vf1,vf1,vf0	// sum in vf1.w
				vmuli vf1,vf1,I		// *1/Fac(j,j)
				
				and t4,t7,~0xF		// 4-align the Result(i,j) pointer
				and t5,s1,3
				beq t5,3,esccase1a	// W
				nop
				beq t5,2,esccase1a	// Z
					vmr32 vf1,vf1
				beq t5,1,esccase1a	// Y
					vmr32 vf1,vf1
				b emptycase1
					vmr32 vf1,vf1	// X
				esccase1a:
				lqc2 vf2,0(t4)		// load Result(i,j) aligned on 16 byte boundary
				vadd vf1,vf1,vf2
				emptycase1:
				sqc2 vf1,0(t4)
				
				add t2,%2
				add t3,%2			// Fac(j+1,j)
				addi t3,4			// Fac(j+1,j+1)
				addi s1,1
			blt s1,t9,jLoop1
				addi t7,4			// Result(i,j+1)
			lw t2,0(%0)				// Fac(0,j)
			lw t3,0(%0)				// Fac(j,j)
        	add s1,zero,t9			// j=Height
        	addi s1,-1
			add t7,zero,s1			// t7=Height-1
			muli t7,t7,4			// t7=(Height-1)*4
			add t7,t7,t1			// Result(i,j), j=Height-1
			add t5,%2,4				// t7=(TempStride+4)
			mul t5,t5,s1			// (Height-1)*(TempStride+4)
			add t3,t3,t5			// Fac(j,j)
			jLoop2:
				lwc1 f2,0(t7)		// sum=Result(i,j)
				suba.s f2,f0		// ACC=f2
				add s2,s1,1			// k=j+1
				
				add t4,%2,t3		// Fac(k,j)
				add t5,t7,4			// Result(i,k)
				
				bge s2,t9,kEscape2	// k<Height
				nop
				kLoop2:
					lwc1 f3,0(t4)	// Fac(k,j)
					lwc1 f4,0(t5)	// Result(i,k)
					msuba.s f3,f4
					//mul.s f5,f3,f4	// Fac(k,j)*Result(i,k)
					//sub.s f2,f2,f5	// sum-=Fac(k,j)*Result(i,k);
					add t4,%2
					addi s2,1		// k++
				blt s2,t9,kLoop2	// k<Height
					addi t5,4
				kEscape2:
				madd.s f2,f0,f0
				lwc1 f6,0(t3)		// Fac(j,j)
				div.s f2,f2,f6		// sum/=Fac(j,j);
				swc1 f2,0(t7)		// Result(i,j)=sum;
				sub t3,%2			// Fac(j-1,j)
				addi t3,-4			// Fac(j-1,j-1)
				addi t2,-4			// Fac(0,j-1)
				addi s1,-1			//j--
			bge s1,s0,jLoop2		//j>=i
				addi t7,-4			// Result(i,j-1)
			add t0,t0,%2			// Fac(i+1,0)
			addi s0,1
		blt s0,t9,iLoop1
			add t1,t1,%3			// Result(i+1,0)
		xor s0,s0
		lw t0,0(%1)					// Result(i,i)
		iLoopSymmetry:
			add s1,zero,s0
			addi s1,1
			add t1,zero,t0			// Result(i,j)
			addi t1,4				// Result(i,j)
			add t2,%3,t0			// Result(j,i)
			jLoopSymmetry:
				lwc1 f1,0(t1)
				swc1 f1,0(t2)
				addi t1,4			// Result(i,j)
				addi s1,1
			blt s1,t9,jLoopSymmetry
				add t2,%3			// Result(j,i)
			add t0,%3
			addi t0,4
			addi s0,1
		blt s0,t9,iLoopSymmetry
		nop
	"	:
		: "g"(&Fac),"g"(&Result),"g"(TempStride),"g"(RStride)
		: "s0","s1","s2","s3","s4","s5","s6","s7"
	);
#endif
#else
	unsigned i;
	unsigned H1=3;
	unsigned TempStride=4*4;
	unsigned RStride=4*4;
	Matrix4x4 *DT=&Fac;
	Matrix4x4 *R=&Result;
	float one[]={1,0,0,0};
	float *one1=one;
	float *Resulti0=NULL;
	float *DetTempjj=NULL;
	Result.Zero();
	_asm
	{
		mov eax,one1;
		movups xmm5,[eax]
		mov eax,0
		mov esi,R
		iLoop:
			mov i,eax
			mov edx,DT
			mov Resulti0,esi			// Result(i,0)
			mov ebx,DT					// Result(i,0)
			mov DetTempjj,ebx			// Result(i,0)
        	push esi					// Result(i,j)
			mov ebx,0
			jLoop1:
				mov edi,edx				// Fac(j,k)
				push edx   
//mov edx,tt
				cmp ebx,i
				jne Off_Diagonal
				movaps xmm1,xmm5
				jmp On_Diagonal
			Off_Diagonal:
				xorps xmm1,xmm1
			On_Diagonal:
//movups [edx],xmm1
				mov eax,Resulti0
				mov ecx,0
				cmp ecx,ebx
				jge kEscape1
//movups [edx],xmm1
				kLoop1:
					movaps xmm2,[edi]   // Fac(j,k)
//movups [edx],xmm2
					mulps xmm2,[eax]    // Result(i,k)
//movups [edx],xmm2
					subps xmm1,xmm2
//movups [edx],xmm1
					add edi,16			// Fac(j,k+4)
					add eax,16			// Result(i,k+4)
                	add ecx,4
					cmp ecx,ebx
				jl kLoop1
				kEscape1:
			// Now add the four sums together:
				movhlps xmm2,xmm1
				addps xmm1,xmm2         // = X+Z,Y+W,--,--
				movaps xmm2,xmm1
				shufps xmm2,xmm2,1      // Y+W,--,--,--
				addss xmm1,xmm2
				mov edi,DetTempjj
				movss xmm2,[edi]
				divss xmm1,xmm2
				movss [esi],xmm1
				add edi,TempStride
				add edi,4    
				add esi,4
				mov DetTempjj,edi
				pop edx
				add edx,TempStride		// Fac(j+1,0)
				add ebx,1				// j++
				cmp ebx,H1				// j<Height
			jl jLoop1
                      
			pop esi
			push esi
			mov edx,DT// Fac(0,0)
			mov eax,H1
			dec eax						// =j
			imul eax,4
			mov edx,Resulti0			// Result(i,0)  
			add edx,eax					// Result(i,j)
	
			mov ebx,DetTempjj			// Fac(j,j)
			sub ebx,TempStride
			add ebx,-4
			mov DetTempjj,ebx			// Fac(j,j)

			mov ebx,H1
			dec ebx						// ebx,Height-1
                      
			mov edi,ebx					// =j
			imul edi,4
			add esi,edi                 // Result(i,j)
			jLoop2:
				mov eax,ebx				// =j
				add eax,1				// =j+1
				mul TempStride		// (j+1)*TempStride
				mov edi,DT// Fac(0,0)
				add edi,eax             // Fac(k,0)   
				mov eax,ebx				// =j
				imul eax,4				// j*4  
				add edi,eax             // Fac(k,j)
			//mov edx,tt
				mov eax,ebx             // =j
				imul eax,4              // (j+1)*4
				add eax,Resulti0        // Result(i,j)
				movss xmm1,[eax]
			//movss sum,xmm1
				add eax,4               // Result(i,k)
				mov ecx,ebx
				inc ecx
				cmp ecx,H1
				jge kEscape2
				kLoop2:
					movss xmm2,[edi]	// Fac(k,j)
				//movss sum,xmm2
					mulss xmm2,[eax]    // Result(i,k)
				//movss sum,xmm2
					subss xmm1,xmm2
				//movss sum,xmm1
					add edi,TempStride
					add eax,4
					add ecx,1
					cmp ecx,H1
                jl kLoop2
				kEscape2:
				mov edi,DetTempjj
				movss xmm2,[edi]
		   //	movss sum,xmm2
				divss xmm1,xmm2
			//movss sum,xmm1
				movss [esi],xmm1
				sub edi,TempStride
				add edi,-4
				mov DetTempjj,edi
				add esi,-4				// Result(i,j-1)
				//add edx,-4				// Fac(0,j-1)

				add ebx,-1				// j--

				cmp ebx,i				// j>=i
			jge jLoop2

			pop esi
			add esi,RStride				// Result(i,0)
			mov eax,i
            add eax,1
			cmp eax,H1
		jl iLoop

		mov esi,R
		mov eax,4
		dec eax
		jle SymmEscape
		mov edi,esi					// Result(i+1,i)
		iSymmLoop:
			mov edx,edi   
			add edx,4
			mov ecx,edi   
			add ecx,RStride
			mov ebx,eax
			jSymmLoop:
				mov esi,[edx]
				mov [ecx],esi
				add ecx,RStride
				add edx,4
				dec ebx
			jg jSymmLoop
			add edi,RStride
			add edi,4
			dec eax
		jg iSymmLoop
		SymmEscape:
	}
/*	for(i=0;i<Height;i++)
	{
		for(j=i+1;j<Height;j++)
		{
			Result(j,i)=Result(i,j);
		}
	}*/
#endif
}            
Matrix4x4 Temp3x3;

void Matrix4x4::operator+=(const Matrix4x4 &M)
{
#ifndef SIMD
register unsigned i,j;
	for (i=0;i<4; i++)
	{
		for (j=0;j<4;j++)
		{
			Values[i*4+j]+=M.Values[i*4+j];
		}
	}
#else
	unsigned MStride=4*4;
	unsigned Stride=4*4;
	_asm
	{
		mov ecx,this
		mov ebx,M
		mov esi,this
		mov edi,M
		mov eax,4
		Row_Loop:      
			mov ebx,4
			push esi
			push edi
			Col_Loop:
				movaps xmm2,[edi]  
				movaps xmm1,[esi]
				addps xmm1,xmm2
				movaps [esi],xmm1
				add edi,16
				add esi,16
				sub ebx,4   
				cmp ebx,0
			jg Col_Loop
			pop edi    
			pop esi
			add edi,MStride
			add esi,Stride
			dec eax
			cmp eax,0
		jg Row_Loop
	}
#endif
}

void Matrix4x4::operator-=(const Matrix4x4 &M)
{
#ifndef SIMD
	register unsigned i,j;
	for(i=0;i<4;i++)
	{
		for (j=0;j<4;j++)
		{
			Values[i*4+j]=Values[i*4+j]-M.Values[i*4+j];
		}
	}
#else
	unsigned MStride=4*4;
	unsigned Stride=4*4;
	float *val=Values;
	const float *Mval=M.Values;
	if(!Mval)
		return;
	unsigned H1=4;
	unsigned W1=4;
	_asm
	{                     
		mov esi,this
		mov edi,M
		mov eax,H1
		Row_Loop:
			mov ebx,W1
			push esi
			push edi
			Col_Loop:
				movaps xmm1,[edi]  
				movaps xmm2,[esi]
				subps xmm2,xmm1
				movaps [esi],xmm2
				add edi,16
				add esi,16
				sub ebx,4   
				cmp ebx,0
			jg Col_Loop
			pop edi    
			pop esi
			add edi,MStride
			add esi,Stride
			dec eax   
			cmp eax,0
		jg Row_Loop
	}
#endif
}
void Matrix4x4::InsertColumn(unsigned col,const Vector3 &v)
{
	assert(col>=0&&col<4);
	Values[col  ]=v.x;
	Values[col+4]=v.y;
	Values[col+8]=v.z;
}
void Matrix4x4::InsertRow(unsigned r,const Vector3 &v)
{
	assert(r>=0&&r<4);
	Values[r*4+0]=v.x;
	Values[r*4+1]=v.y;
	Values[r*4+2]=v.z;
}
void simul::math::MultiplyAndSubtract(Matrix4x4 &M,const Matrix4x4 &M1,const Matrix4x4 &M2)
{
	register unsigned i,j,k;
	for(i=0;i<4;++i)
	{
		for(j=0;j<4;j++)
		{
			for(k=0;k<4;k++)
			{
				M.Values[i*4+j]-=M1(i,k)*M2(k,j);
			}
		}
	}
}
void simul::math::MultiplyByScalar(Matrix4x4 &result,const float f,const Matrix4x4 &M)
{
	register unsigned i,j;
	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			result.Values[i*4+j]=f*M.Values[i*4+j];
		}
	}
}
