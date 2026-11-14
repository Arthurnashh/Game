#ifndef SIMUL_SKY_BASESKY_H
#define SIMUL_SKY_BASESKY_H

#include "Simul/Sky/Export.h"
#include "Simul/Sky/Binding.h"
#include "Simul/Sky/BaseSky.h"
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Sky/Float4.h"
#include "Simul/Sky/SkyInterface.h"

namespace simul
{
	namespace sky
	{
		SIMUL_SKY_EXPORT_CLASS BaseSky : public SkyInterface
		{
		public:
			BaseSky();
			virtual ~BaseSky();
			META_BeginProperties
				META_ValueRangePropertyWithSetCall(float,AtmosphereThickness	,0.f	,100.f				,AtmosphereThicknessChanged	,"Maximum height in Km.")
				META_ValueRangePropertyWithSetCall(float,PlanetRadius			,100.f	,200000.f			,PlanetRadiusChanged		,"Radius in km of the planet, default=6378km (Earth).")
				META_ReferenceProperty(float4			,SunIrradiance																	,"Irradiance, watts/square metre of the sun in space, x=red, y=green, z=blue, w unused.")
				META_ReferencePropertyWithSetCall(float4,Starlight											,StarlightChanged			,"Irradiance due to stars - should be very small compared to the sun.")
			META_EndProperties
			META_BeginStates 
				META_StateCategory(SunPosition)
					META_ValueStateWithSetCall(float		,Time							,TimeChanged								,"Time value controlling light properties. This will be overriden by simul::clouds::BaseWeatherRenderer::SetTime, so use that function to control time-of-day if possible.")
					META_ValueRangeStateWithSetCall(float	,SunRadiusArcMinutes			,0.f,160.f				,UpdateSunPosition	,"Angular size of the sun.")
					META_ReferenceStateWithSetCall(float4	,DirectionToSun					,SetSunPosByDirection						,"Direction vector towards the sun.")
					META_ValueRangeStateWithSetCall(float	,AzimuthToSunRadians,-3.1415926536f*2.f,3.1415926536f*2.f,SetSunPosByAngle	,"Azimuthal angle towards the sun, in radians.")
					META_ValueRangeStateWithSetCall(float	,ElevationToSunRadians,-3.1415926536f/2.f,3.1415926536f/2.f,SetSunPosByAngle,"Elevation angle towards the sun, in radians.")
				META_EndStateCategory(SunPosition)	
				
				META_StateCategory(MoonPosition)
					META_ValueRangeStateWithSetCall(float	,MoonRadiusArcMinutes			,0.f,3600.f				,UpdateMoonPosition		,"Angular size of the Moon.")
					META_ReferenceStateWithSetCall(float4	,DirectionToMoon				,SetMoonPosByDirection							,"Direction vector towards the Moon.")
					META_ValueRangeStateWithSetCall(float	,AzimuthToMoonRadians,-3.1415926536f*2.f,3.1415926536f*2.f,SetMoonPosByAngle	,"Azimuthal angle towards the Moon, in radians.")
					META_ValueRangeStateWithSetCall(float	,ElevationToMoonRadians,-3.1415926536f/2.f,3.1415926536f/2.f,SetMoonPosByAngle	,"Elevation angle towards the Moon, in radians.")
				META_EndStateCategory(MoonPosition)	

				META_StateCategory(HazeAndOvercastValues)																										
					META_ValueRangeStateWithSetCall(float	,Haze				,0		,10000	,HazeChanged	,"Haze, the level of non-gaseous particles in the atmosphere. This can be changed in real time.")
					META_ValueRangeState(float				,MieEccentricity	,0.82f	,-.99f	,0.99f			,"Eccentricity of Mie (haze) scattering.")
					META_ValueRangeStateWithSetCall(float,HazeBaseHeightKm		,-25.f	,25.f				,HazeBaseHeightChanged,
																											"Base height of the haze curve. Changing this causes a rebuild of the internal tables, so use sparingly.")
					META_ValueRangeStateWithSetCall(float,HazeScaleHeightKm		,0.001f	,25.f				,HazeScaleHeightChanged,
																											"Characteristic height at which haze or fog tails off. Changing this causes a rebuild of the internal tables, so use sparingly.")

					META_ValueRangeState(float				,Overcast			,0.f,0.f,1.f					,"The overcast effect strength.")		
					META_ValueRangeState(float				,OvercastBaseKm		,0.f,-100.f,100.f				,"The base altitude for the overcast effect.")
					META_ValueRangeState(float				,OvercastRangeKm	,1.f,0.001f,100.f				,"The altitude range for the overcast effect, e.g. thickness of the cloud layer.")
				META_EndStateCategory(HazeAndOvercastValues)
			META_EndStates
			static float4 AzimuthElevationToDirection(float az,float el);
			virtual const float4 &GetDirectionToLight() const;
			float	GetHazeAtAltitude					(float alt_km) const;
			float	GetHazeFactorAtAltitude		(float alt_km) const;
			float	GetOvercastAtAltitude				(float alt_km) const;
			const float4 &GetAmbientLight				(float altitude_km) const;
			float4 GetSunlightFactor					(float) const;
			float4 GetSunlightColour					(float) const;
			float4 GetSunlightColour					(const float4 &) const;
			float4 GetSunlightFactor	(const float4 &pos_km)	const;
			float4 GetMoonIrradiance					(float altitude_km) const;
			virtual float GetVisibilityDistance(float view_alt_km);
			virtual float GetHazeForVisibilityDistance(float view_alt_km,float horiz_dist_km);
			float GetDistanceToSpace(float Elevation,float h_km) const;
			float GetDistanceToEarth(float Elevation,float h_km) const;
		protected:
			mutable float use_haze_base_km;
			mutable float use_haze_scale_km;
			float GetOvercastAtAltitudeRange(float alt_km,float prev_alt_km) const;
			virtual float4 GetSunlightFactor(const float4 &Pos,const float4 &DirectionToLight) const=0;
			float4 GetSunlightFactor(float alt_km,const float4 &DirectionToLight) const;
			//BaseSky
			virtual void HazeChanged(){}
			virtual void HazeBaseHeightChanged();
			virtual void HazeScaleHeightChanged();
			virtual void AtmosphereThicknessChanged(){}
			virtual void PlanetRadiusChanged(){}
			virtual void TimeChanged();
			virtual void SetSunPosByDirection();
			virtual void SetSunPosByAngle();
			virtual void SetMoonPosByAngle();
			virtual void UpdateSunPosition(){}
			virtual void UpdateMoonPosition(){}
			virtual void SetMoonPosByDirection();
			virtual void StarlightChanged(){}
		};
	}
}

#endif