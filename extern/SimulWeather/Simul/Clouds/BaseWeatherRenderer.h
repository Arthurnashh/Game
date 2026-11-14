#ifndef SIMUL_CLOUDS_BASEWEATHERRENDERER_H
#define SIMUL_CLOUDS_BASEWEATHERRENDERER_H
#include "Simul/Clouds/Export.h"
#include "Simul/Base/PropertyMacros.h"
#include <iostream>
namespace simul
{
	namespace sky
	{
		class BaseSkyRenderer;
		struct float4;
		class SkyKeyframer;
	}

	namespace clouds
	{
		class CloudKeyframer;
		class BaseLightningRenderer;
		class BaseCloudRenderer;
		class BasePrecipitationRenderer;
		//! The main base class for all weather renderers, such as SimulGLWeatherRenderer, SimulWeatherRenderer (DX9) etc. 
		//! See also \ref WeatherRenderer for more details on this class and its uses.
		SIMUL_CLOUDS_EXPORT_CLASS BaseWeatherRenderer
		{
		public:
			BaseWeatherRenderer();
			virtual ~BaseWeatherRenderer();
		META_BeginProperties
			META_ValueProperty(bool	,Godrays		,"If true, draw godrays where light comes through gaps in the clouds - implementation-dependent.")
			META_ValueRangeProperty(int	,Downscale	,2,1,8,"How much to reduce resolution in the buffers.")
		META_EndProperties
			//! Save a sky sequence (usually as a .seq file; this is how Sky Sequencer saves sequences).
			std::ostream &Save(std::ostream &os) const;
			//! Load a sequence (usually as a .seq file from Sky Sequencer).
			std::istream &Load(std::istream &is);
			//! Clear the sky sequence()
			void New();
			//! Connect-up sky, clouds etc.
			virtual void ConnectInterfaces()=0;
			//! Set the strength of rain or snow, between zero and one.
			void SetPrecipitation(float strength);
			//! This is the main controlling interface to set the time of day for Simul Weather. It overrides the SetDaytime interface in
			//! the Sky and SkyKeyframer classes.
			void SetDaytime(float dt);
			//! Set the altitude of the cloudbase, in kilometres. This overrides all keyframes.
			void SetCloudbaseKm(float h_km);
			//! Get the direction \a to the light (sun or moon).
			const simul::sky::float4 &GetDirectionToLight();
			//! Get the representative HDR colour of the horizon, as seen from the specified altitude \param view_altitude_km .
			const simul::sky::float4 &GetHorizonColour(float view_altitude_km);
			//! Get the HDR light colour, as seen from the specified altitude \param view_altitude_km
			const simul::sky::float4 &GetLightColour(float view_altitude_km) const;
			//! Get the HDR ambient colour, as seen from the specified altitude \param view_altitude_km
			const simul::sky::float4 &GetAmbientLightColour(float view_altitude_km) const;
			//! Get a pointer to the sky renderer.
			simul::sky::BaseSkyRenderer *GetBaseSkyRenderer()
			{
				return baseSkyRenderer;
			}
			//! Get a pointer to the cloud renderer.
			simul::clouds::BaseCloudRenderer *GetBaseCloudRenderer()
			{
				return baseCloudRenderer;
			}
			//! Get a pointer to the 2D (high-altitude/Cirrus) cloud renderer, or NULL if none is present.
			simul::clouds::BaseCloudRenderer *GetBase2DCloudRenderer()
			{
				return base2DCloudRenderer;
			}
			//! Convenience function to get the next sky keyframe that can be modified without any recalculation.
			simul::sky::SkyKeyframer *GetSkyKeyframer();
			//! Convenience function to get the next cloud keyframe that can be modified without any recalculation.
			simul::clouds::CloudKeyframer *GetCloudKeyframer();
			//! Enable or disable the sky.
			void EnableSky(bool s);
			//! Enable or disable the 3d and 2d cloud layers.
			virtual void EnableLayers(bool clouds3d,bool clouds2d);
			bool IsCloudLayer1Visible() const
			{
				return layer1;
			}
			bool IsCloudLayer2Visible() const
			{
				return layer2;
			}
		protected:
			simul::clouds::BaseLightningRenderer *baseLightningRenderer;
			simul::sky::BaseSkyRenderer *baseSkyRenderer;
			simul::clouds::BaseCloudRenderer *baseCloudRenderer;
			simul::clouds::BaseCloudRenderer *base2DCloudRenderer;
			simul::clouds::BasePrecipitationRenderer *basePrecipitationRenderer;
			bool layer1,layer2;
			bool show_sky;
		};
	}
}
#endif