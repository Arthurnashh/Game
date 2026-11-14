#ifndef SIMUL_MATH_NOISE3D_H
#define SIMUL_MATH_NOISE3D_H
#include "Simul/Math/Export.h"
#include "Simul/Graph/StandardNodes/MemoryUsageInterface.h"

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			class MemoryInterface;
		}
	}
	namespace math
	{
		class RandomNumberGenerator;
		class NoiseFilter
		{
		public:
			virtual float Filter(float) const = 0;
		};
		SIMUL_MATH_EXPORT_CLASS NoiseInterface
			:public simul::graph::standardnodes::MemoryUsageInterface
		{
		public:
			virtual ~NoiseInterface(){}
			virtual void Setup(unsigned freq,int RandomSeed,int octaves,float persistence=0.5f)=0;
			virtual float PerlinNoise3D(float x,float y,float z) const=0;
			virtual float PerlinNoise3D(int x,int y,int z) const=0;
			virtual float PerlinNoise2D(float x,float y) const=0;
			virtual void SetCacheGrid(int x,int y,int z)=0;
			virtual void SetFilter(NoiseFilter *nf) =0;
		};
		//! A 3D Perlin noise class.
		SIMUL_MATH_EXPORT_CLASS Noise3D : public NoiseInterface
		{
		public:
			Noise3D(simul::graph::meta::MemoryInterface *mem=0);
			virtual ~Noise3D();
		//! Define the grid of pseudo-random numbers to be used in the PerlinNoise3D function. The parameter freq is the
		//! frequency, or grid-size.
		//! /p RandomSeed is a reproducible seed for the pseudo-random number generation.
		//! /p octaves is the number of octaves of noise super-imposed.
		//! \p persistence is the relative scaling of subsequent octaves - higher persistence leads to a more noisy function.
			void Setup(unsigned freq,int RandomSeed,int octaves,float persistence=.5f);
			unsigned GetNoiseFrequency() const;
			float PerlinNoise3D(float x,float y,float z) const;
			float PerlinNoise3D(int x,int y,int z) const;
			float PerlinNoise2D(float ,float ) const	{return 0.f;}
			float PerlinNoise2D(int ,int ) const		{return 0.f;}
			virtual void SetCacheGrid(int x,int y,int z);
			void SetFilter(NoiseFilter *nf);
			virtual unsigned GetMemoryUsage() const;
		protected:
			unsigned frequency;
			float *noise_buffer;
			float value_at(unsigned i,unsigned j,unsigned k) const;
			float noise3(int p[3]) const;
			float noise3(float p[3]) const;
			int numOctaves;
			float persistence;
			NoiseFilter *filter;
			unsigned buffer_size;
			simul::graph::meta::MemoryInterface *memoryInterface;
			RandomNumberGenerator *noise_random;
			float sum;
			unsigned grid_x,grid_y,grid_z;
			unsigned grid_max;
		};
	}
}

#endif // NOISE_H
