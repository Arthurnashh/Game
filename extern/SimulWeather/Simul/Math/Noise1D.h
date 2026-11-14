#ifndef SIMUL_MATH_NOISE1D_H
#define SIMUL_MATH_NOISE1D_H
#include "Simul/Math/Export.h"
#include "Simul/Math/Noise3D.h"

namespace simul
{
	namespace math
	{
		//! A 3D Perlin noise class.
		SIMUL_MATH_EXPORT_CLASS Noise1D
		{
			unsigned frequency;
			float *noise_buffer;
			float value_at(unsigned i) const;
			float noise1(float p) const;
			int numOctaves;
			float persistence;
			NoiseFilter *filter;
			unsigned buffer_size;
			simul::graph::meta::MemoryInterface *memoryInterface;
			RandomNumberGenerator *noise_random;
		public:
			Noise1D(simul::graph::meta::MemoryInterface *mem=NULL);
			virtual ~Noise1D();
		//! Define the grid of pseudo-random numbers to be used in the PerlinNoise3D function. The parameter freq is the
		//! frequency, or grid-size.
			void Setup(unsigned freq,int RandomSeed,int octaves,float persistence=.5f);
			int GetNoiseFrequency() const;
			float PerlinNoise1D(float x) const;
			void SetFilter(NoiseFilter *nf);
		// MemoryUsageInterface
			virtual unsigned GetMemoryUsage() const;
		};
	}
}

#endif // NOISE_H
