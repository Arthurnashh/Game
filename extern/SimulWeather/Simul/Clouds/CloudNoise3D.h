#ifndef SIMUL_MATH_CLOUDNOISE3D_H
#define SIMUL_MATH_CLOUDNOISE3D_H
#include "Simul/Math/Noise3D.h"
namespace simul
{
	namespace clouds
	{
		class CloudNoise3D : public simul::math::Noise3D
		{
			float z_max;
			float time;
			float period;
			float cnoise3(float p[3]) const;
			float cvalue_at(int i,int j,int k) const;
			mutable int kmax;
			mutable float initial_grid_height;
		public:
			CloudNoise3D(simul::graph::meta::MemoryInterface *mem=0);
			virtual ~CloudNoise3D();
			void SetInitialGridHeight(float h);
			float PerlinNoise3D(float x,float y,float z) const;
			void SetTime(float t);
			void SetPeriod(float p);
		};
	}
}

#endif

