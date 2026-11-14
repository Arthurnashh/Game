#include "precomp.h"

#include <math.h>
#include <stdlib.h>
#include "./RandomNumberGenerator.h"
#ifdef _MSC_VER
	// C4512 - "assignment operator could not be generated". Boost classes cause this warning.
	// Also C4127 - "conditional expression is constant"
	#pragma warning(push)
	#pragma warning(disable:4512)
	#pragma warning(disable:4127)
#endif

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
typedef boost::minstd_rand base_generator_type;

#pragma optimize("",off)
using namespace simul::math;

simul::math::RandomNumberGenerator::RandomNumberGenerator()
{
	base_generator_type *generator=new base_generator_type;
	gnr=generator;
}

simul::math::RandomNumberGenerator::~RandomNumberGenerator()
{
	base_generator_type *generator=(base_generator_type*)gnr;
	delete generator;
}

float simul::math::RandomNumberGenerator::FRand(float minval,float maxval) const
{
	base_generator_type *generator=(base_generator_type*)gnr;
	boost::uniform_real<float> uni_dist(0,1);
	boost::variate_generator<base_generator_type&, boost::uniform_real<float> > uni(*generator, uni_dist);
	return minval+(maxval-minval)*uni();
}
 
void simul::math::RandomNumberGenerator::Seed(int RandomSeed)
{
	base_generator_type *generator=(base_generator_type*)gnr;
	generator->seed(RandomSeed);
}

void simul::math::RandomNumberGenerator::RandomDirection(float &x,float &y,float &z)
{
	float az=0,el=0;
	RandomAngle(az,el);
	x=sin(az)*cos(el);
	y=cos(az)*cos(el);
	z=sin(el);
}

void simul::math::RandomNumberGenerator::SphericalRandom(float &x,float &y,float &z)
{
	float r=1.f-pow(FRand(),4.f);
	float az=FRand()*2*3.1415926536f;
	float el=asin(FRand()*2.f-1.f);
	x=r*sin(az)*cos(el);
	y=r*cos(az)*cos(el);
	z=r*sin(el);
}

void simul::math::RandomNumberGenerator::RandomAngle(float &az,float &el)
{
	az=FRand()*2*3.1415926536f;
	el=asin(FRand()*2.f-1.f);
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
