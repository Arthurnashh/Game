#ifndef IterationH
#define IterationH

#include <vector>
#include "Simul/Math/Matrix.h"
#include "Simul/Math/SimVector.h"
#include "Simul/Math/Export.h"

#ifdef _MSC_VER
	
#ifdef WIN32
	#define SIMD
#endif
	#define ALIGN32  __declspec(align(32))
#else
	#define ALIGN32
#endif

namespace simul
{
	namespace math
	{
extern void SIMUL_MATH_EXPORT_FN SSORP(Matrix &A,Vector &X,Vector &B,unsigned Num,unsigned Lim,Vector &InverseDiagonals);       
extern void SIMUL_MATH_EXPORT_FN SSORP2(Matrix &A,Vector &X,Vector &B,unsigned Num,unsigned Lim,Vector &InverseDiagonals,void* Nv);

#ifdef SIMD
inline void AddDotProduct8(float &f,float *V1,float *V2)
{
	float temp=f;
	_asm 
	{
		mov eax,V1
		prefetcht1 [eax+32]
		movaps xmm1,[eax]
		mov ebx,V2
		mulps xmm1,[ebx]
		xorps xmm2,xmm2
		movss xmm2,temp;
		addps xmm2,xmm1
		movaps xmm1,[eax+16]
		mulps xmm1,[ebx+16]
		addps xmm2,xmm1
		movhlps xmm1,xmm2
		addps xmm2,xmm1
		pshufd xmm3,xmm2,01010101b
		addss xmm2,xmm3
		movss temp,xmm2
	}
	f=temp;
}
#else
extern void AddDotProduct8(float &f,float *V1,float *V2);
#endif
// = X+Z,Y+W,--,--	
// Y+W,--,--,--
           
#ifdef SIMD
inline void AddFloatTimesVector8(float *V2,const float f,float *V1)
{
	__asm
	{
		movss xmm1,f
		mov eax,V1
		prefetcht1 [eax+32]
		movaps xmm0,[eax]
		shufps xmm1,xmm1,0
		movaps xmm2,[eax+16]
		mov ecx,V2
		mulps xmm0,xmm1
		mulps xmm2,xmm1
		addps xmm0,[ecx]
		addps xmm2,[ecx+16]
		movaps [ecx],xmm0
		movaps [ecx+16],xmm2
	}
}
#else
extern void AddFloatTimesVector8(float *V2,const float f,float *V1);
#endif

inline bool s(float *x,float *NormalX,float *NormalY,float *NormalZ,float *NormalD,unsigned Num,float t)
{
#ifdef SIMD
	_asm
	{
		mov ebx,Num
		mov ecx,x
		movss xmm0,t
		shufps xmm0,xmm0,0
		movss xmm4,[ecx]
		shufps xmm4,xmm4,0
		movss xmm5,[ecx+4]
		shufps xmm5,xmm5,0
		movss xmm6,[ecx+8]
		shufps xmm6,xmm6,0
		mov ecx,NormalX
		mov esi,NormalY
		mov edi,NormalZ
		mov edx,NormalD
		iLoop:
			movaps xmm1,[ecx]
			movaps xmm2,[esi]
			movaps xmm3,[edi]
			movaps xmm7,[edx]
			addps xmm7,xmm0
			mulps xmm1,xmm4
			mulps xmm2,xmm5
			mulps xmm3,xmm6
			addps xmm1,xmm2
			addps xmm1,xmm3
			add ecx,16
			add edi,16
			add edx,16
			cmpltps xmm7,xmm1
			add ebx,-4
			add esi,16
			movmskps eax,xmm7
			cmp eax,0
			jne retfalse
			cmp ebx,0
		jg iLoop
		mov al,1
		jmp rettrue
		retfalse:
		mov al,0
		rettrue:
	};
#else
	t;
	for(unsigned i=0;i<Num;i+=4)
	{        
		ALIGN32 float d1[4];
		ALIGN32 float d2[4];
		ALIGN32 float d3[4];
		for(unsigned j=0;j<4;j++)
		{
			d1[j]=NormalX[i+j]*x[0];
			d2[j]=NormalY[i+j]*x[1];
			d3[j]=NormalZ[i+j]*x[2];
		}
		for(unsigned j=0;j<4;j++)
			d1[j]+=d2[j]+d3[j];
		for(unsigned j=0;j<4;j++)
			if(d1[j]>NormalD[i+j])
				return false;
	}
	return true;
#endif      
}
extern void SIMUL_MATH_EXPORT_FN YNMultiply(float *y,Matrix &BetaY,unsigned &yn,std::vector<unsigned> *YN1,std::vector<unsigned> *YN2,float *ntemp,Vector &nr);
extern void SIMUL_MATH_EXPORT_FN YNMultiply6(float *y,Matrix &BetaY,unsigned &yn,std::vector<unsigned> *YN1,std::vector<unsigned> *YN2,float *ntemp,Vector &nr);

extern void SIMUL_MATH_EXPORT_FN YNMultiply6a(float *y1,float *y2,Matrix &BetaY,unsigned &yn,unsigned Num,Vector &n_result,unsigned &N);
extern void SIMUL_MATH_EXPORT_FN YNMultiply6b(float *y1,float *y2,Matrix &BetaY,unsigned &yn,Vector &n_result,unsigned &N);


extern void SIMUL_MATH_EXPORT_FN NYMultiply6a(float *y1,float *y2,float *&YVec,unsigned VCN,Vector &n,unsigned &N);
extern void SIMUL_MATH_EXPORT_FN NYMultiply6b(float *y1,float *y2,float *&YVec,Vector &n,unsigned &N);
extern void SIMUL_MATH_EXPORT_FN NYMultiply6(float *y,float *YVec,std::vector<unsigned> *YN,Vector &n);
extern void SIMUL_MATH_EXPORT_FN ReplaceColumns3To5WithCrossProduct(Matrix &Beta);
extern void SIMUL_MATH_EXPORT_FN ApplyForceLimits(Vector &NOutput,Vector &NNormals,
	Vector &ForceMin,Vector &ForceMax,unsigned StartNormalN,unsigned StartFrictionN,
	std::vector<unsigned>&TransverseNSize,std::vector<unsigned>&VCN);

//#undef SIMD
extern void SIMUL_MATH_EXPORT_FN CrossProductRow3To5(float *&MatrixPtr,unsigned rowsize,unsigned num);
extern void SIMUL_MATH_EXPORT_FN ResponseAndB(float *B,float *M1,float *r,float *by,unsigned b_width,unsigned num);
#undef SIMD
}
}
#endif
