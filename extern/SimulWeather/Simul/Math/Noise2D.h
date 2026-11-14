#ifndef SIMUL_MATH_NOISE2D_H
#define SIMUL_MATH_NOISE2D_H
#include "Simul/Math/Export.h"
#include "Simul/Math/Noise3D.h"

namespace simul
{
	namespace math
	{
		//! A 3D Perlin noise class.
		SIMUL_MATH_EXPORT_CLASS Noise2D : public NoiseInterface
		{
			unsigned frequency;
			float *noise_buffer;
			float value_at(unsigned i,unsigned j) const;
			float noise3(int p[2]) const;
			float noise3(float p[2]) const;
			int numOctaves;
			float persistence;
			NoiseFilter *filter;
			unsigned buffer_size;
			simul::graph::meta::MemoryInterface *memoryInterface;
			RandomNumberGenerator *noise_random;
		public:
			Noise2D(simul::graph::meta::MemoryInterface *mem=NULL);
			virtual ~Noise2D();
		//! Define the grid of pseudo-random numbers to be used in the PerlinNoise2D function. The parameter freq is the
		//! frequency, or grid-size.
			void Setup(unsigned freq,int RandomSeed,int octaves,float persistence=.5f);
			int GetNoiseFrequency() const;
			/*
			   In what follows "alpha" is the weight when the sum is formed.
			   Typically it is 2, As this approaches 1 the function is noisier.
			   "beta" is the harmonic scaling/spacing, typically 2.
			*/
			float PerlinNoise3D(float ,float ,float ) const	{return 0.f;}
			float PerlinNoise3D(int ,int ,int ) const		{return 0.f;}
			float PerlinNoise2D(float x,float y) const;
			float PerlinNoise2D(int x,int y) const;
			virtual void SetCacheGrid(int ,int ,int ){}
			virtual void SetCaching(bool){}
			void SetFilter(NoiseFilter *nf);
			void Mix(float interp,const Noise2D &N);
		// MemoryUsageInterface
			virtual unsigned GetMemoryUsage() const;
		};
	}
}

#endif // NOISE_H
