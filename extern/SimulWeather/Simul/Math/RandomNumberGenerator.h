#ifndef SIMULMATH_RANDOMNUMBERGENERATOR_H
#define SIMULMATH_RANDOMNUMBERGENERATOR_H
#include "Simul/Math/Export.h"
namespace simul
{
	namespace math
	{
		SIMUL_MATH_EXPORT_CLASS RandomNumberGenerator
		{
			void *gnr;
		public:
			RandomNumberGenerator();
			virtual ~RandomNumberGenerator();
			void Seed(int RandomSeed);
			float FRand(float minval=0.f,float maxval=1.f) const;
			void RandomDirection(float &x,float &y,float &z);
			void SphericalRandom(float &x,float &y,float &z);
			void RandomAngle(float &az,float &el);
		};
	}
}

#endif // NOISE_H
