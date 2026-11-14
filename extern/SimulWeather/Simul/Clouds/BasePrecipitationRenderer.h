#ifndef SIMUL_CLOUDS_BASEPrecipitationRENDERER_H
#define SIMUL_CLOUDS_BASEPRECIPITATIONRENDERER_H
#include "Simul/Clouds/Export.h"
#include "Simul/Graph/Meta/Group.h"

namespace simul
{
	namespace sky
	{
		struct float4;
	}
	namespace clouds
	{
		//! The main base class for all Precipitation renderers, such as SimulGLPrecipitationRenderer, SimulPrecipitationRenderer (DX9) etc.
		SIMUL_CLOUDS_EXPORT_CLASS BasePrecipitationRenderer
			:public simul::graph::meta::Group
		{
		public:
			BasePrecipitationRenderer();
			virtual ~BasePrecipitationRenderer();
			META_BeginProperties
				META_ValueRangeProperty(float,RainSpeedMin,1.6f,0.f,100.f,"Speed of light rain.")
				META_ValueRangeProperty(float,RainSpeedMax,10.f,0.f,100.f,"Speed of heavy rain.")
				META_ValueRangeProperty(float,WindEffect,0.1f,0.f,1.f,"Strength of the effect of wind on precipitation direction.")
				META_ValueRangeProperty(float,WindEffectForSnow,0.1f,0.f,1.f,"WindEffect, for snow defaults.")
				META_ValueRangeProperty(float,WindEffectForRain,0.1f,0.f,1.f,"WindEffect, for rain defaults.")
				META_ValueRangePropertyWithSetCall(float,Aspect,0.01f,1000.f,TextureRepeatChanged,"How much precipitation texture is stretched vertically.")
				META_ValuePropertyWithSetCall(int		,TextureRepeat		,TextureRepeatChanged,"How many times the texture is repeated around 360 degrees.")
				META_ValuePropertyWithSetCall(int		,RainTextureRepeat	,TextureRepeatChanged,"How many times the rain texture is repeated around 360 degrees.")
				META_ValueRangeProperty(float,Waver,0.f,0.f,1.f,"How much precipitation layers waver as they fall.")
			META_EndProperties
			Macro_EnableRecursiveStreaming(simul::graph::meta::Group)
			Macro_Node(BasePrecipitationRenderer)
			//! Call this once per frame to update the clouds.
			virtual void Update(float dt);
			//! Set the strength of the rain or precipitation effect.
			void SetIntensity(float i)
			{
				rain_intensity=i;
			}
			void SetWind(float speed,float heading_degrees)
			{
				wind_heading=heading_degrees*3.14159f/180.f;
				wind_speed=speed;
			}
			void ApplyDefaultRainSettings();
			void ApplyDefaultSnowSettings();
			//! Set the colour of light, e.g. sunlight, that illuminates the precipitation.
			void SetLightColour(const float c[4]);
			//! Save data, e.g. to a .seq file.
			std::ostream &Save(std::ostream &os) const;
			//! Load data e.g. from a .seq file.
			std::istream &Load(std::istream &is);
			//! Clear the data()
			void New();

			float DefaultRainSpeedMin;
			float DefaultRainSpeedMax;
			float DefaultSnowSpeedMin;
			float DefaultSnowSpeedMax;
		protected:
			virtual void TextureRepeatChanged();
			float radius,height,offs,rain_intensity,wind_heading,wind_speed;
			float light_colour[4];
			float rain_speed;
		};
	}
}
#endif