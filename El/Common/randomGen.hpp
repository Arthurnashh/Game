#ifndef _RANDOMGEN_HPP
#define _RANDOMGEN_HPP

#include "Es/Common/fltopts.hpp"

class RandomGenerator
{
	mutable int _seed;
	
	public:
	RandomGenerator(int seed1, int seed2);
	RandomGenerator();
	/// random value from a sequence
	float RandomValue() const;
	float RandomValue11() const
	{
		return RandomValue()*2.0f - 1.0f;
	}
	
	/** 
	@param beg minimal value (inclusive)
	@param end maximal value (exclusive)
	*/
	int RandomInt(int beg, int end) const
	{
	  int rand = toIntFloor(RandomValue()*(end-beg))+beg;
	  saturate(rand,beg,end-1);
	  return rand;
	  
	}

	///return random boolean value
	bool RandomBool() const
	{
		return (RandomValue()<0.5f);
	}

	/// set sequence start
	void SetSeed( int seed ) {_seed=seed;}
	
	/// random value based on explicit seed
	float RandomValue(int seed) const;
	/// random value based on explicit 2D seed
	float RandomValue(int x, int z) const {return RandomValue(GetSeed(x,z));}
	/// random value based on explicit 3D seed
	float RandomValue(int x, int z, int y) const {return RandomValue(GetSeed(x,z,y));}
	
	float Gauss(float min, float mid, float max) const;
	float PlusMinus(float a, float b) const;

 	int GetSeed(int x, int z) const;
 	int GetSeed(int x, int z, int y) const;
};

extern RandomGenerator GRandGen;

#endif
