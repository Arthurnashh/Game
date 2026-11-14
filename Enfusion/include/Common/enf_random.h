#ifndef _ENF_RANDOM_H
#define _ENF_RANDOM_H


#include "enf_config.h"
#include "common/enf_mathlib.h"
#include "system/enf_timer.h"
#include <stdlib.h>


namespace enf
{
/** @addtogroup Common
@{*/

/*!
maximum value 
*/
const int ENF_MAX_INT_RAND	= 0x7fff;
const float INV_RAND_MAX	= (1.0f/((float) ENF_MAX_INT_RAND + 0.0001f));
const float INV_HASH_VALUE	= 1.0f/65536.0f;


//--------------------------------------------------------------------------
/*!
simple class for random numbers
*/
//--------------------------------------------------------------------------
class RandNumber
{
public:
	//-----------------------------------------------------------------------------
	/*!
	set random seed
	*/
	//-----------------------------------------------------------------------------
	static ENF_INLINE void Randomize()
	{
		enf_srand(enf_toint(Timer::GetTime()*1000.0f) );
	}


	//-----------------------------------------------------------------------------
	/*!
	set seed
	*/
	//-----------------------------------------------------------------------------
	static ENF_INLINE void SetSeed(uint seed)
	{
		enf_srand(seed);
	}


	//--------------------------------------------------------------------------
	/*!
	return random float number with the normal distribution from the range <0.0, 1.0>
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float RandFloat01 ()				
	{ 	
		return (float) (((float) enf_rand()*INV_RAND_MAX)); 
	}

	//--------------------------------------------------------------------------
	/*!
	return random float number with the normal distribution from the range <-1.0, 1.0>
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float RandFloat11 ()				
	{ 	
		return (float) 2.0f*(((float) enf_rand()*INV_RAND_MAX)-0.5f); 
	}

	//--------------------------------------------------------------------------
	/*!
	return random float number with the normal distribution from the range <0.0 to X>
	\param X  value
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float RandFloat0X (float X)		
	{ 	
		return (float) (((float) enf_rand()*INV_RAND_MAX))*X; 
	}

	//--------------------------------------------------------------------------
	/*!
	return random float number with the normal distribution from the range <-X to X>
	\param X  value
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float RandFloatXX (float X)
	{ 	
		return (float) 2.0f*X*(((float) enf_rand()*INV_RAND_MAX) - 0.5f); 
	}


	//--------------------------------------------------------------------------
	/*!
	return random integer number with the normal distribution from the range <0, X - 1>
	\param X  value
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE int RandInt0X (int X)				
	{	
		enf_assert(X >= 0 && X <= ENF_MAX_INT_RAND);
		return (enf_rand() * X) / (ENF_MAX_INT_RAND + 1);
	}


	//--------------------------------------------------------------------------
	/*!
	return random integer number with the uniform distribution from the range <0, X - 1> except the "last" number
	\param X		value
	\param last number not to be returned
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE int RandInt0XExcept (int X, int last)
	{	
		enf_assert(X > 1 && X <= ENF_MAX_INT_RAND);
		int ret = (enf_rand() * (X - 1)) / (ENF_MAX_INT_RAND + 1);
		if(ret == last)
		{
			ret = ret >= X - 1 ? ret - 1 : ret + 1;
			enf_assert(ret >= 0 && ret < X);
		}
		return ret;
	}


	//--------------------------------------------------------------------------
	/*!
	return random integer number with the normal distribution from the range <-X + 1,  X - 1>
	\param X  value
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE int RandIntXX (int X)				
	{	
		enf_assert(X >= 0 && X <= ENF_MAX_INT_RAND);
		return (enf::enf_rand() * X*2) / (ENF_MAX_INT_RAND + 1) - X;
	}

	
	//--------------------------------------------------------------------------
	/*!
	Returns random number with Gauss distribution (http://en.wikipedia.org/wiki/File:Normal_Distribution_PDF.svg)
	e.g. for sigma = 1 and mean = 0, the values will be distrubuted from -3.5 to 3.5 and the most values will
	be around 0.

	This computation is almost 4 times faster than previous one and more accurate.

	The algorithm uses the ratio of uniforms method of A.J. Kinderman
	and J.F. Monahan augmented with quadratic bounding curves.

	\param sigma	sigma (smerodatna odchylka, sirka gausse)
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float GaussFloat(float sigma)
	{	
		float q,u,v,x,y;

		//	Generate P = (u,v) uniform in rect. enclosing acceptance region 
		//	Make sure that any random numbers <= 0 are rejected, since
		//	gaussian() requires uniforms > 0, but RandomUniform() delivers >= 0.

		do 
		{
			u = RandNumber::RandFloat01() + 1e-8f;
			v = RandNumber::RandFloat01();
		
			v = 1.7156f * (v - 0.5f);

			// Evaluate the quadratic form
			x = u - 0.449871f;
			y = fabs(v) + 0.386595f;
			q = x * x + y * (0.19600f * y - 0.25472f * x);

			// Accept P if inside inner ellipse 
			if (q < 0.27597f)
				break;

			//  Reject P if outside outer ellipse, or outside acceptance region 
		} while ((q > 0.27846f) || (v * v > -4.0f * log(u) * u * u));

		//Return ratio of P's coordinates as the normal deviate 
		enf_assert(u > 0.0f);

		return sigma*v/u;
	}


	//--------------------------------------------------------------------------
	/*!
	returns random number with Gauss distribution
	\param mean		mean  (stredni hodnota = na ose X, kde je nejvetsi hustota pravdepodobnosti)
	\param sigma	sigma (smerodatna odchylka, sirka gausse)
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float GaussFloat(float sigma, float mean)
	{		
		return mean + GaussFloat(sigma);
	}


	//--------------------------------------------------------------------------
	/*!
	returns random number with Gauss normal distribution, ret is from <-1, 1>, but
	due to normal distribution, the values from <-1, -0.5> and <0.5, 1> are very rare
	*/
	//--------------------------------------------------------------------------
	static ENF_INLINE float GaussFloat()
	{		
		return GaussFloat(1.0f/6.0f);
	}


	//-----------------------------------------------------------------------------
	/*!
	return hash value for some number, the number is from <0..65535>
	*/
	//-----------------------------------------------------------------------------
	static uint HashValue(uint val) 
	{
		val = ~val + (val << 15); // val = (val << 15) - val - 1;
		val = val ^ (val >> 12); /// >>>
		val = val + (val << 2);
		val = val ^ (val >> 4); /// >>>
		val = val * 2057; // val = (val + (val << 3)) + (val << 11);
		val = val ^ (val >> 16); // >>>

/*		val = (val+0x7ed55d16) + (val<<12);
		val = (val^0xc761c23c) ^ (val>>19);
		val = (val+0x165667b1) + (val<<5);
		val = (val+0xd3a2646c) ^ (val<<9);
		val = (val+0xfd7046c5) + (val<<3);
		val = (val^0xb55a4f09) ^ (val>>16);
*/

/*		val = (val ^ 61) ^ (val >> 16);
		val = val + (val << 3);
		val = val ^ (val >> 4);
		val = val * 0x27d4eb2d;
		val = val ^ (val >> 15);
*/
		return val & 0xFFFF;
	}

	//-----------------------------------------------------------------------------
	/*!
	return hash value for some number, the number is from <0..32767>
	*/
	//-----------------------------------------------------------------------------
	static float HashValue01(uint val) 
	{
		return HashValue(val)*INV_HASH_VALUE;
	}


	//-----------------------------------------------------------------------------
	/*!
	return hash value for x, y, z number, the number is from <0..32767>
	*/
	//-----------------------------------------------------------------------------
	static uint HashValue(uint x, uint z, uint y)
	{
		//combine values
		x -= y; x -= z; x ^= (z>>13); 
		y -= z; y -= x; y ^= (x<<8); 
		z -= x; z -= y; z ^= (y>>13); 
		x -= y; x -= z; x ^= (z>>12);  
		y -= z; y -= x; y ^= (x<<16); 
		z -= x; z -= y; z ^= (y>>5); 
		x -= y; x -= z; x ^= (z>>3);
		y -= z; y -= x; y ^= (x<<10);
		z -= x; z -= y; z ^= (y>>15);
	
		return z;
	}

	//-----------------------------------------------------------------------------
	/*!
	return hash value for x, y, z number, the number is from <0..32767>
	*/
	//-----------------------------------------------------------------------------
	static float HashValue01(uint x, uint z, uint y)
	{
		return HashValue(x, y, z)*INV_HASH_VALUE;
	}


	//-----------------------------------------------------------------------------
	/*!
	return hash value for x, y number, the number is from <0..32767>
	*/
	//-----------------------------------------------------------------------------
	static uint HashValue(uint x, uint y)
	{
		//combine values
		uint xy = 0;

		xy<<=1,xy|=x&1,x>>=1;
		xy<<=1,xy|=y&1,y>>=1;

		xy<<=1,xy|=x&1,x>>=1;
		xy<<=1,xy|=y&1,y>>=1;

		xy<<=1,xy|=x&1,x>>=1;
		xy<<=1,xy|=y&1,y>>=1;

		xy<<=1,xy|=x&1,x>>=1;
		xy<<=1,xy|=y&1,y>>=1;

		xy<<=1,xy|=x&1,x>>=1;
		xy<<=1,xy|=y&1,y>>=1;

		xy<<=1,xy|=x&1,x>>=1;
		xy<<=1,xy|=y&1,y>>=1;

		return xy;
	}


	//-----------------------------------------------------------------------------
	/*!
	return hash value for x, y number, the number is from <0..32767>
	*/
	//-----------------------------------------------------------------------------
	static uint HashValue01(uint x, uint y)
	{
		return (uint)(HashValue(x, y)*INV_HASH_VALUE);
	}
};


//-----------------------------------------------------------------------------
/*!
instance of random class to be able
*/
//-----------------------------------------------------------------------------
class RandNumberInst
{
protected:
	mutable uint m_iSeed;

public:
	RandNumberInst()
	{
		m_iSeed = RandNumber::RandInt0X(ENF_MAX_INT_RAND);
	}

	RandNumberInst(uint seed)
	{
		m_iSeed = seed;
	}

	ENF_INLINE void SetSeed(uint seed)
	{
		m_iSeed = seed;
	}

	ENF_INLINE uint GetSeed() const 
	{
		return m_iSeed;
	}

	ENF_INLINE void Randomize()
	{
		m_iSeed = enf_toint(Timer::GetTime()*1000.0f) ;
	}

	/*!
	return random float number with the normal distribution from the range <0.0, 1.0>
	*/
	ENF_INLINE float RandFloat01 ()				
	{ 	
		return (float) (((float) enf_randInst(m_iSeed)*INV_RAND_MAX)); 
	}

	/*!
	return random float number with the normal distribution from the range <-1.0, 1.0>
	*/
	ENF_INLINE float RandFloat11 ()				
	{ 	
		return (float) 2.0f*(((float) enf_randInst(m_iSeed)*INV_RAND_MAX)-0.5f); 
	}

	/*!
	return random float number with the normal distribution from the range <0.0 to X>
	\param X  value
	*/
	ENF_INLINE float RandFloat0X (float X)		
	{ 	
		return (float) (((float) enf_randInst(m_iSeed)*INV_RAND_MAX))*X; 
	}

	/*!
	return random float number with the normal distribution from the range <-X to X>
	\param X  value
	*/
	ENF_INLINE float RandFloatXX (float X)
	{ 	
		return (float) 2.0f*X*(((float) enf_randInst(m_iSeed)*INV_RAND_MAX) - 0.5f); 
	}

	/*!
	return random integer number with the normal distribution from the range <0, X - 1>
	\param X  value
	*/
	ENF_INLINE int RandInt0X (int X)				
	{	
		enf_assert(X >= 0 && X <= ENF_MAX_INT_RAND);
		return (enf_randInst(m_iSeed) * X) / (ENF_MAX_INT_RAND + 1);
	}

	/*!
	return random integer number with the uniform distribution from the range <0, X - 1> except the "last" number
	\param X		value
	\param last number not to be returned
	*/
	ENF_INLINE int RandInt0XExcept (int X, int last)
	{	
		enf_assert(X >= 1 && X <= ENF_MAX_INT_RAND);
		int ret = (enf_randInst(m_iSeed) * (X - 1)) / (ENF_MAX_INT_RAND + 1);
		if(ret == last)
		{
			ret = ret >= X - 1 ? ret - 1 : ret + 1;
			enf_assert(ret >= 0 && ret < X);
		}
		return ret;
	}

	/*!
	return random integer number with the normal distribution from the range <-X + 1,  X - 1>
	\param X  value
	*/
	ENF_INLINE int RandIntXX (int X)				
	{	
		enf_assert(X >= 0 && X <= ENF_MAX_INT_RAND);
		return (enf_randInst(m_iSeed) * X*2) / (ENF_MAX_INT_RAND + 1) - X;
	}


	/*!
	Returns random number with Gauss distribution (http://en.wikipedia.org/wiki/File:Normal_Distribution_PDF.svg)
	e.g. for sigma = 1 and mean = 0, the values will be distrubuted from -3.5 to 3.5 and the most values will
	be around 0.

	This computation is almost 4 times faster than previous one and more accurate.

	The algorithm uses the ratio of uniforms method of A.J. Kinderman
	and J.F. Monahan augmented with quadratic bounding curves.

	\param sigma	sigma (smerodatna odchylka, sirka gausse)
	*/
	ENF_INLINE float GaussFloat(float sigma)
	{	
		float q,u,v,x,y;

		//	Generate P = (u,v) uniform in rect. enclosing acceptance region 
		//	Make sure that any random numbers <= 0 are rejected, since
		//	gaussian() requires uniforms > 0, but RandomUniform() delivers >= 0.

		do 
		{
			u = RandFloat01() + 1e-8f;
			v = RandFloat01();
		
			v = 1.7156f * (v - 0.5f);

			// Evaluate the quadratic form
			x = u - 0.449871f;
			y = fabs(v) + 0.386595f;
			q = x * x + y * (0.19600f * y - 0.25472f * x);

			// Accept P if inside inner ellipse 
			if (q < 0.27597f)
				break;

			//  Reject P if outside outer ellipse, or outside acceptance region 
		} while ((q > 0.27846f) || (v * v > -4.0f * log(u) * u * u));

		//Return ratio of P's coordinates as the normal deviate 
		enf_assert(u > 0.0f);

		return sigma*v/u;
	}

	/*!
	returns random number with Gauss distribution
	\param mean		mean  (stredni hodnota = na ose X, kde je nejvetsi hustota pravdepodobnosti)
	\param sigma	sigma (smerodatna odchylka, sirka gausse)
	*/
	ENF_INLINE float GaussFloat(float sigma, float mean)
	{		
		return mean + GaussFloat(sigma);
	}

	/*!
	returns random number with Gauss normal distribution, ret is from <-1, 1>, but
	due to normal distribution, the values from <-1, -0.5> and <0.5, 1> are very rare
	*/
	ENF_INLINE float GaussFloat()
	{		
		return GaussFloat(1.0f/6.0f);
	}
};


//@}
}
#endif

