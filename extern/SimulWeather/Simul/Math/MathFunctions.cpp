#include "precomp.h"

#define SIM_MATH
#include "MathFunctions.h"
#include <math.h>     
#include <algorithm>
//------------------------------------------------------------------------------
#ifdef WIN32
	//#define SIMD
#endif
#ifdef PLAYSTATION_2
	#pragma align(16)
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

namespace simul
{
	namespace math
	{
#if defined(WIN32) && !defined(__GNUC__)
//------------------------------------------------------------------------------
float Sine(float x)
{
	float Result;
	float *xx=&x;
	_asm
	{
		mov eax,xx
		fld [eax]
		fsin
		fstp Result
	}
	return Result;
}
#else
float Sine(float x)
{
	return sinf(x);
}
#endif
          
float Cosine(float x)
{
#ifdef SIMD
	x;
	float Result;
	_asm
	{
		fld x
		fcos
		fstp Result
	}
	return Result;
#else
#ifdef PLAYSTATION2
	return cosf(x);
#else  
	return cos(x);
#endif
#endif
}

#ifdef SIMD
/*float Fabs(float x)
{       
	float Result;
	x;
	_asm
	{
		fld x
		fabs
		fstp Result
	}
	return Result;
}     */        
#else  
#endif    

//------------------------------------------------------------------------------

float InverseTangent(float y,float x)
{
#ifdef SIMD
	x,y;
	float Result;
	_asm
	{
		fld y
		fld x
		fpatan
		fstp Result
	}
	return Result;
#else
#ifdef PLAYSTATION2
	return atan2f(y,x);
#else
	return (float)atan2((float)y,(float)x);
#endif  
#endif
}
//------------------------------------------------------------------------------
#ifdef SIMD   
float InverseCosine(float x)
{               
	return acos(x);
}
#else
#ifdef PLAYSTATION2
float InverseCosine(float x)
{
	return acosf(x);
}
#else   
float InverseCosine(float x)
{
	return acos(x);
}
#endif
#endif
class BadNumber{};
float Sqrt(float x)
{
#ifdef _DEBUG
#ifdef SIMD
	if(_isnan(x))
		throw BadNumber();
#endif
#ifdef SIMD
	if(x<0)
		throw BadNumber();
#endif
#endif
#ifdef SIMD
	float Result;
	_asm
	{
		fld x
		fsqrt
		fstp Result
	}        
	return Result;
#else
	return sqrt(x);
#endif
}
//------------------------------------------------------------------------------
void IntegerPowerOfTen(float num,float &man,int &Exp)
{
	if(fabs(num)<1e-8f)
		num=1;
	float expf=log(num);
	expf/=log(10.f);
	if(expf>=0)
		Exp=(int)(expf+0.5f);
	else
		Exp=(int)(expf-0.5f);
	float powr=pow(10.f,(float)Exp);
	man=num/powr;

}    
//------------------------------------------------------------------------------
void PowerOfTen(float num,float &Exp)
{
	if(fabs(num)<1e-8f)
		num=1.f;
	Exp=log(num);
	Exp/=log(10.f);
}

}
}
