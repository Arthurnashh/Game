#ifndef SIMUL_CLOUDS_BASECLOUDRENDERER_H
#define SIMUL_CLOUDS_BASECLOUDRENDERER_H
#pragma once
#include "Simul/Clouds/Export.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Clouds/CloudRenderCallback.h"
#include "Simul/Clouds/CloudGeometryHelper.h"
#include "Simul/Clouds/CloudKeyframer.h"
#include "Simul/Clouds/ThunderCloudNode.h"

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace sky
	{
		class BaseSkyInterface;
		class OvercastCallback;
	}
	namespace clouds
	{
		class CloudInterface;
		class CloudKeyframer;
		class LightningRenderInterface;
		class CloudGeometryHelper;
		class ThunderCloudNode;

		//! Cloud renderers implement this callback so they can provide shadow textures and other data to
		//! terrain renderers.
		class CloudShadowCallback
		{
		public:
			//! Get the list of three platform-specific textures used for cloud rendering. Cast these from void* to the
			//! platform's texture pointer (e.g. LPDIRECT3DTEXTURE9, GLuint, ID3D11Texture3D*, etc.
			virtual void **GetCloudTextures()=0;
			//! Do the clouds wrap horizontally?
			virtual bool GetWrap() const=0;
			//! Get the xyz scales of the clouds in metres.
			virtual const float *GetCloudScales() const=0;
			//! Get the xyz offset of the clouds (only xy are usually relevant).
			virtual const float *GetCloudOffset() const=0;
			//! Get the cloud texture interpolation value.
			virtual float GetInterpolation() const=0;
		};

		//! Base class for real-time volumetric cloud renderers. Source code is provided for this class
		//! Derive a new renderer from this base class, or use SimulCloudRenderer or SimulGLCloudRenderer.
		SIMUL_CLOUDS_EXPORT_CLASS BaseCloudRenderer
			: public CloudRenderCallback
			, public CloudShadowCallback
			, public simul::graph::meta::Group
		{
		public:
			BaseCloudRenderer(bool thundercloud=true);
			virtual ~BaseCloudRenderer();
			META_BeginProperties
				META_ValueRangePropertyWithSetCall(float,Detail,0.f,2.f,DetailChanged,"Cloud layer rendering detail between 0.f and 1.f")
			META_EndProperties
			Macro_EnableRecursiveStreaming(simul::graph::meta::Group)
			Macro_VirtualNode(BaseCloudRenderer)
			/*! \name Functions	*/	
			/*\*/
			//! Get the current wind speed, either set externally or interpolated from keyframes.
			float GetWindSpeed() const;
			//! Get the current wind heading in degrees clockwise from North, either set externally or interpolated from keyframes.
			float GetWindHeadingDegrees() const;
			//! Set the wind horizontal velocity components in metres per second.
			void SetWind(float speed,float heading_degrees);
			//! Get an interface to the Simul cloud object.
			simul::clouds::CloudInterface *GetCloudInterface();
			//! Set the sky interface.
			void SetSkyInterface(simul::sky::BaseSkyInterface *si);
			//! Get a pointer to the keyframer object which the BaseCloudRenderer owns.
			simul::clouds::CloudKeyframer *GetCloudKeyframer();
			//! Get the interface for rendering lightning.
			simul::clouds::LightningRenderInterface *GetLightningRenderInterface();
			//! Set the texture coordinate (constant per-frame) for altitude.
			void SetAltitudeTextureCoordinate(float f)
			{
				altitude_tex_coord=f;
			}
			//! Is the license key up to date?
			bool IsLicenseOK() const;
			//! Set the interpolation value of the distance fade.
			void SetFadeInterpolation(float f)
			{
				fade_interp=f;
			}
			//! Between zero and one: how overcast is the sky?
			simul::sky::OvercastCallback *GetOvercastCallback() const;
			//! How much, if any precipitation should be shown, between zero and one.
			float GetPrecipitationIntensity() const;
			//! How much, if any precipitation should be shown, between zero and one.
			float GetPrecipitationIntensity(const float pos[]) const;
			//! Set the precipitation strength.
			void SetPrecipitation(float p){precip_strength=p;}
			//! Get a value for how much the sun is blocked from the clouds.
			float GetSunOcclusion() const;
			//! Returns a number between 0 and 1 for the transition between rain and snow due to keyframe properties.
			float GetRainToSnow() const;
			//! A convenient interface to override the cloudbase height for all keyframes.
			void SetCloudBaseKm(float h_km);
			//! Save a cloud keyframe sequence, e.g. to a .seq file.
			std::ostream &Save(std::ostream &os) const;
			//! Load a cloud keyframe sequence, e.g. from a .seq file.
			std::istream &Load(std::istream &is) const;
			//! Clear the sequence()
			void New();
			//! A callback function that translates from daytime values to overcast settings. Used for
			// clouds to tell sky when it is overcast.
			simul::sky::OvercastCallback *GetOvercastCallback();
			//! Set the overall required cloudiness, or humidity.
			void SetCloudiness(float h);
			//! Where supported, enable lightning generation.
			void SetEnableStorms(bool s);

			//! Adjust the noise texture
			void SetNoiseTextureProperties(int size,int freq,int octaves,float persistence);
			//! Fade mode: 0=CPU, 1=Vertex, 2=Fragment
			enum FadeMode
			{
				CPU=0,FRAGMENT
			};
			//! Where supported, set the fade mode - CPU or FRAGMENT.
			virtual void SetFadeMode(FadeMode f);
			//! Lightning
			bool UpdateIlluminationTexture(const float *view_pos,float dt);
			//! Per-frame update
			virtual void Update(float dt);

			//! Get a float between zero and one which represents the interpolation between cloud keyframes.
			virtual bool GetWrap() const;
			//! Get the xyz scales of the clouds in metres.
			virtual const float *GetCloudScales() const;
			//! Get the xyz offset of the clouds (only xy are usually relevant).
			virtual const float *GetCloudOffset() const;
			//! Get the cloud texture interpolation value.
			virtual float GetInterpolation() const;

			// Implementing CloudRenderCallback
			virtual bool CanPerformGPULighting() const {return false;}
			virtual void SetGPULightingParameters(const float *,const unsigned *,const float *){}
			virtual void PerformFullGPURelight(int,float *,float *){}
	
			virtual void SetIlluminationGridSize(unsigned,unsigned,unsigned){}
			virtual void FillIlluminationSequentially(int,int,int,const unsigned char *){}
			virtual void FillIlluminationBlock(int,int,int,int,int,int,int,const unsigned char *){}
			/*\*/
			
			//! Causes all cloud textures to be relit - only needed for debugging.
			void ForceRelight();
		protected:
			// Data for GPU lighting:
			float LightToDensityTransform[16];
			unsigned light_gridsizes[3];
			float light_extinctions[4];

			//
			float sun_occlusion;
			unsigned illumination_texel_index[4];
			void DetailChanged();
			virtual bool CreateNoiseTexture(bool override_file=false)=0;
			float altitude_tex_coord;
			simul::clouds::CloudInterface *cloudInterface;
			simul::clouds::LightningRenderInterface* lightningRenderInterface;
			simul::base::SmartPtr<simul::clouds::CloudKeyframer> cloudKeyframer;
			simul::base::SmartPtr<simul::clouds::CloudGeometryHelper> helper;
			simul::sky::BaseSkyInterface *skyInterface;
			simul::base::SmartPtr<simul::clouds::FastCloudNode> cloudNode;
			unsigned cloud_tex_width_x;
			unsigned cloud_tex_length_y;
			unsigned cloud_tex_depth_z;
			float precip_strength;
			simul::sky::float4 cam_pos;
			float thunder_volume;
			float fade_interp;
			bool enable_lightning;

			float last_time;
			int noise_texture_size;
			int noise_texture_frequency;
			int texture_octaves;
			float texture_persistence;
			FadeMode fade_mode;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif