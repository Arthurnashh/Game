#ifndef SIMUL_CLOUDS_RAYTRACE_H
#define SIMUL_CLOUDS_RAYTRACE_H
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Clouds/Tex3d.h"
#include "Simul/Clouds/Texture3d4.h"
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/Resource.h"
#include "Simul/Graph/StandardNodes/ShowProgressInterface.h"
#include "Simul/Clouds/CloudGeometryHelper.h"
#include "Simul/Clouds/Export.h"
#ifdef _MSC_VER
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace sky
	{
		class SkyInterface;
		class SkyKeyframer;
		struct float4;
	}
	namespace clouds
	{
		class CloudInterface;
		SIMUL_CLOUDS_EXPORT_CLASS RayTrace : public simul::graph::meta::Node
			,public simul::graph::meta::ResourceUser<simul::graph::standardnodes::ShowProgressInterface>
		{
		public:
			RayTrace();
			virtual ~RayTrace();
			Texture3D4 noise_tex;
			void Trace(	const simul::sky::float4& p0,const simul::sky::float4& p1,
						simul::sky::float4& final, simul::sky::float4& trans);
			void MakeVolumetricNoiseTexture(unsigned texSize);
			void SetSky(simul::sky::SkyInterface *si);
			void ClearClouds();
			void AddCloud(simul::clouds::CloudInterface *ci);
			void RemoveUnusedClouds();
			void RenderBegin(const simul::sky::float4& view_pos_km,const simul::sky::float4& wind_offset,
						   const simul::sky::float4& view_dir,const simul::sky::float4& up_dir);
			void RenderEnd();
			META_BeginProperties
				META_ValueRangeProperty(float	,Exposure			,0.f	,-8.f,8.f	,"Brightness multiplier for the sky.")
				META_ValueRangePropertyWithSetCall(unsigned,Detail	,12		,100		,ChangedDetail,"Level of detail.")
				META_ValueRangeProperty(float	,NoisePersistence	,0.65f	,0.f,1.f	,"Effective roughness of cloud edges.")
				META_ValueRangeProperty(unsigned,NoiseOctaves		,6		,1	,12		,"Fractal detail steps for cloud edges.")
				META_ValueRangePropertyWithSetCall(float,GroundHeightKm,-5.f,0.f		,ChangedDetail,"Height of the ground - below this, the atmosphere is not illuminated.")
			META_EndProperties

			float GetBrightness() const {return pow(2.f,Exposure);}
			int GetLayers() const {return Layers;}
			float GetMaxDistance() const;
			bool IsCancelled() const {return cancel;}
		private:
			void ChangedDetail();
			float exposure;
			float direct_light_mult;
			int noiseTextureSize;
			int Layers;
			float ground_fade_thickness;
			float base_height;
			bool UnlitUnderground;
			bool rendering;
			bool cancel;
			int intermediate_steps;
			float sec_beta;
			sky::SkyKeyframer *skyKeyframer;
			simul::sky::float4 cam_pos;
			simul::sky::float4 cam_pos_km;
			sky::SkyInterface *skyInterface;
			CloudGeometryHelper helper;
			float sun_cosine;
			struct CloudAuxiliary
			{
				CloudAuxiliary();
				Texture3D4 cloud_tex;
				int GenerationNumber;
				int RelightNumber;
				float primary_proportion;
				float secondary_proportion;
				float ambient_proportion;
				float MaxTexCoordZ;
				float light_mult;
				simul::sky::float4 ambient;
				simul::math::Vector3 X1;
				simul::sky::float4 inverseDimensions;
				CloudInterface *cloudInterface;
				void Setup(CloudInterface *ci);
			};
			std::vector<CloudAuxiliary*> cloudAuxiliaries;
			simul::sky::float4 GetNoise(const simul::sky::float4& pos,CloudInterface *cloudInterface);
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif
