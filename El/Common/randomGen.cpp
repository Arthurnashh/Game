#include <El/elementpch.hpp>
#include "randomGen.hpp"
//#include "global.hpp"

#include "isaac.hpp"

/*
Functions in this source often appear during profiling
We therefore decide to optimize them for speed rather than size.
*/

#pragma optimize("t",on)

/*!
multiplier to convert hash values into <0..1> range
*/
const float RandInvDivider32767 = 1.0f/32767.0f;

float RandomGenerator::RandomValue( int seed ) const
{
  // fast hash function
	int val = (seed ^ 61) ^ (seed >> 16);
	val = val + (val << 3);
	val = val ^ (val >> 4);
	val = val * 0x27d4eb2d;
	val = val ^ (val >> 15);
	return (val&0x7fff)*RandInvDivider32767;
}

float RandomGenerator::RandomValue() const
{
  // using Linear congruential generator
  // same LCG parameters as used by Gcc
  // see http://en.wikipedia.org/wiki/Linear_congruential_generator
  // see also http://tracker.coreboot.org/trac/coreboot/browser/trunk/payloads/libpayload/libc/rand.c
  // use bits 0..30
  int mask = (1U<<31)-1;
  // note: some implementations seem to apply mask before assignment to seed, some after it
  // most likely it does not matter much? ??
  int next = (1103515245*_seed + 12345)&mask;
  _seed = next;
	return next*(1.0f/mask);
}

int RandomGenerator::GetSeed( int x, int z, int y ) const
{
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
  // other possible implementations:
  /*
  return x+z+y;
  */
  /*
	// make x, z out of order
	// bitwise interleave x, z, and y
	int xz=0;
	// only 12 bits is significant
	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;
	xz<<=1,xz|=y&1,y>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;
	xz<<=1,xz|=y&1,y>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;
	xz<<=1,xz|=y&1,y>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;
	xz<<=1,xz|=y&1,y>>=1;
	return xz;
	*/
}

int RandomGenerator::GetSeed( int x, int z ) const
{
	// make x, z out of order
	// bitwise interleave x and z
	int xz=0;
	// only 12 bits is significant
	// consider: use inline assembly (use rotation with carry for bit interleaving) 
	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;

	xz<<=1,xz|=x&1,x>>=1;
	xz<<=1,xz|=z&1,z>>=1;

	return xz;
}

RandomGenerator GRandGen INIT_PRIORITY_NORMAL;

extern DWORD GlobalTickCount();

/*!
\patch_internal 1.05 Date 7/17/2001 by Ondra.
- New: Random generator may be constructed with given seeds.
This was required to have reproducible generator for landscape bumps.
*/

RandomGenerator::RandomGenerator(int seed1, int seed2)
:_seed(seed2)
{
}

RandomGenerator::RandomGenerator()
:_seed(GlobalTickCount()+3256)
{
}

float RandomGenerator::Gauss(float min, float mid, float max) const
{
	float gauss =
	(
		RandomValue() + RandomValue() +
		RandomValue() + RandomValue()
	) * 0.25;
	float delay = 0;
	if (gauss < 0.5)
	{
		float coef = gauss * 2;
		delay = min + (mid - min) * coef;
	}
	else
	{
		float coef = gauss * 2 - 1;
		delay = mid + (max - mid) * coef;
	}
	return delay;
}

float RandomGenerator::PlusMinus(float a, float b) const
{
	return a - b + 2.0f * b * RandomValue();
}
