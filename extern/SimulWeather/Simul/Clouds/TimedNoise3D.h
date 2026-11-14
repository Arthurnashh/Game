#ifndef SIMUL_MATH_TimedNoise3D_H
#define SIMUL_MATH_TimedNoise3D_H
#include "Simul/Math/Noise3D.h"
#include "Simul/Clouds/Export.h"
namespace simul
{
	namespace clouds
	{
		SIMUL_CLOUDS_EXPORT_CLASS TimedNoise3D : public simul::math::Noise3D
		{
			float z_max;
			float time;
			float cnoise3(float p[3],float angle,int kmax) const;
			float cnoise2(float p[2],float angle,int freq) const;
			float cvalue_2d_at(int i,int j,float angle,int freq) const;
			float cvalue_at(int i,int j,int k,float angle,int kmax) const;
			float initial_grid_height;
		public:
			TimedNoise3D(simul::graph::meta::MemoryInterface *mem=0);
			virtual ~TimedNoise3D();
			void SetInitialGridHeight(float h);
			float PerlinNoise3D(float x,float y,float z) const;
			void SetTime(float t);
		};
	}
}

#endif

