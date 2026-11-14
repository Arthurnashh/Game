#ifndef SIMUL_GRAPH_CLOUDS_INTERPOLATEDNOISE3D_H
#define SIMUL_GRAPH_CLOUDS_INTERPOLATEDNOISE3D_H
#include "Simul/Clouds/CloudNoise3D.h"
#include "Simul/Math/Noise3D.h"
#include "Simul/Graph/Meta/MemoryInterface.h"
namespace simul
{
	namespace clouds
	{
		inline float nlerp(float l,float h,float f)
		{
			return l+((h-l)*f);
		}
		class InterpolatedNoise3D : public simul::math::NoiseInterface
		{
			CloudNoise3D *noise1;
			CloudNoise3D *noise2;
			CloudNoise3D *next_noise;
			float interp;
			bool caching;
			unsigned grid_x;
			unsigned grid_y;
			unsigned grid_z;
			unsigned grid_max;
			simul::graph::meta::MemoryInterface *memoryInterface;
		public:
			InterpolatedNoise3D(simul::graph::meta::MemoryInterface *mi=NULL):
				noise1(NULL),
				noise2(NULL),
				next_noise(NULL),
				memoryInterface(mi)
			{
				interp=0.f;
				caching=false;
				if(!noise1)
				{
					if(mi)
						noise1=new(mi) CloudNoise3D(mi);
					else
						noise1=new CloudNoise3D();
				}
				if(!noise2)
				{
					if(mi)
						noise2=new(mi) CloudNoise3D(mi);
					else
						noise2=new CloudNoise3D();
				}
				if(!next_noise)
				{
					if(mi)
						next_noise=new(mi) CloudNoise3D(mi);
					else
						next_noise=new CloudNoise3D();
				}
			}
			void SetInterp(float i)
			{
				interp=i;
			}
			void SetMembers(CloudNoise3D *n1,CloudNoise3D *n2,CloudNoise3D *next)
			{
				noise1=n1;
				noise2=n2;
				next_noise=next;
			}
			virtual ~InterpolatedNoise3D()
			{
				if(memoryInterface)
				{
					operator delete(noise1,memoryInterface);
					operator delete(noise2,memoryInterface);
					operator delete(next_noise,memoryInterface);
				}
				else
				{
					delete noise1;
					delete noise2;
					delete next_noise;
				}
			}
			virtual void BeginPartialBuild(unsigned freq,int RandomSeed,int octaves,float persistence=.5f)
			{
				std::swap(noise1,noise2);
				std::swap(next_noise,noise2);
			}
			virtual void Setup(unsigned NoiseResolution,int RandomSeed,int NoiseOctaves,float NoisePersistence)
			{
				noise1->Setup(NoiseResolution,RandomSeed,NoiseOctaves,NoisePersistence);
				noise2->Setup(NoiseResolution,RandomSeed+1,NoiseOctaves,NoisePersistence);
				next_noise->Setup(NoiseResolution,RandomSeed+2,NoiseOctaves,NoisePersistence);
			}
			virtual void SetCacheGrid(int x,int y,int z)
			{
				grid_x=x;
				grid_y=y;
				grid_z=z;
				grid_max=std::max(std::max(grid_x,grid_y),grid_z);
				noise1->SetCacheGrid(x,y,z);
				noise2->SetCacheGrid(x,y,z);
				next_noise->SetCacheGrid(x,y,z);
			}
			virtual float PerlinNoise3D(float x,float y,float z) const
			{
				return nlerp(noise1->PerlinNoise3D(x,y,z),noise2->PerlinNoise3D(x,y,z),interp);
			}
			virtual float PerlinNoise3D(int x,int y,int z) const
			{
				//if(caching)
					return nlerp(noise1->Noise3D::PerlinNoise3D(x,y,z),noise2->Noise3D::PerlinNoise3D(x,y,z),interp);
			/*	else
				{
					float fx=(float)x/(float)grid_max;
					float fy=(float)y/(float)grid_max;
					float fz=(float)z/(float)grid_max;
					float n1=noise1->PerlinNoise3D(fx,fy,fz);
					float n2=noise2->PerlinNoise3D(fx,fy,fz);
					float n=nlerp(n1,n2,interp);
					return n;
				}*/
			}
			virtual float PerlinNoise2D(float x,float y) const
			{
				return nlerp(noise1->PerlinNoise2D(x,y),noise2->PerlinNoise2D(x,y),interp);
			}
			virtual float PerlinNoise2D(int x,int y) const
			{
				return nlerp(noise1->PerlinNoise2D(x,y),noise2->PerlinNoise2D(x,y),interp);
			}
			virtual void SetFilter(simul::math::NoiseFilter *nf)
			{
				noise1->SetFilter(nf);
				noise2->SetFilter(nf);
				next_noise->SetFilter(nf);
			}
			virtual unsigned GetMemoryUsage() const
			{
				// This owns the noise pointers, so the memory usage is :
				unsigned memory=sizeof(this);
				if(noise1)
					memory+=noise1->GetMemoryUsage();
				if(noise2)
					memory+=noise2->GetMemoryUsage();
				if(next_noise)
					memory+=next_noise->GetMemoryUsage();
				return memory;
			}
			void SetInitialGridHeight(float h)
			{
				noise1->SetInitialGridHeight(h);
				noise2->SetInitialGridHeight(h);
				next_noise->SetInitialGridHeight(h);
			}

			void UpdatePartial(float NoiseInterp)
			{
				SetInterp(NoiseInterp);
				if(NoiseInterp>=0.5f)
				{
				}
			}
		};
	}
};
#endif