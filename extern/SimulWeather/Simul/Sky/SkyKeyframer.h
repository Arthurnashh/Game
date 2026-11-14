#ifndef SIMUL_ALTITUDEFADETABLE_H
#define SIMUL_ALTITUDEFADETABLE_H

#include "Simul/Sky/Export.h"
#include "Simul/Sky/Binding.h"
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Sky/Float4.h"
#include "Simul/Sky/SkyInterface.h"
#include "Simul/Sky/Texture1D4.h"
#include "Simul/Sky/Texture2D4.h"
#include "Simul/Sky/SkyTexturesCallback.h"
#include "Simul/Sky/FadeTableInterface.h"
#include "Simul/Sky/BaseKeyframer.h"
#include "Simul/Sky/Stars.h"

#include "Simul/Base/Referenced.h"
#include <vector>
#include <stack>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace sky
	{
		class SkyInterface;
		class OvercastCallback;
//! A class that maintains tables of sky colours and distance-fade values.

//! The SkyKeyframer is useful because it stores sky and distance-fade information that takes a while to calculate,
//! caches and interpolates this data. So advanced distance fades can be used in real time.
//! SkyKeyframer works by obtaining fade values from a SkyInterface, and storing three tables.
//! Any call to GetIsotropicColourLossFactor or GetIsotropicInscatterFactor returns a value
//! interpolated between the first two tables based on time.
//! Update() should be called once per frame, and this function partially fills the third table
//! based on the same interpolation. When the interpolation completes a step,
//! the tables are cycled.

//! Normally an instance of the SkyKeyframer is kept as a member of the sky renderer, and is created in
//! simul::sky::BaseSkyRenderer. Therefore any class derived from simul::sky::BaseSkyRenderer will have
//! a SkyKeyframer built-in and ready for use.

//! To use the SkyKeyframer to generate fade textures, a callback is employed. To create a new sky renderer, derive a class from
//! SkyTexturesCallback, implement its functions, and 
//! use SetCallback() to tell the SkyKeyframer where to call. Then when necessary the SkyKeyframer will call these functions
//! of the derived class. For example, the derived class can create and update three textures each for loss, inscatter, and sky.

//! The SkyKeyframer calculates the fade and sky for
//! several altitudes at once - the exact number is specified with SetNumAltitudes.
//! This permits interpolation - so instead of re-generating the stored sky and fades as altitude changes,
//! we simply change a single texture-coordinate, obtained from GetAltitudeTexCoord().
//! The usual usage would be to have the sky as a 2D texture - one axis for elevation, one for viewing altitude,
//! and have the fades as 3D textures - an axis for distance, one for elevation, one for altitude.
//! Try to minimize the number of altitudes required, in order to maximize performance - 4 or 8 should suffice  - and
//! use SetAltitudeRangeKm on initialization to the range you will use in-game.
//! This class implements BaseSkyInterface - this is useful because a SkyKeyframer provides an extra interface between
//! a sky implementation and anything that might use the BaseSkyInterface, making sure that things like ambient and
//! directional light match exactly with what's in the fade tables.
		SIMUL_SKY_EXPORT_CLASS SkyKeyframer : public simul::base::Referenced,public FadeTableInterface
			, public BaseKeyframer, public BaseSkyInterface
		{
		public:
			SkyKeyframer(SkyInterface *si,SkyTexturesCallback *c=NULL,int fade_height=128,int fade_width=64,float max_dist_km=300.f);
			virtual ~SkyKeyframer();
			virtual const SkyKeyframer &operator=(const SkyKeyframer& aft);
			META_BeginProperties
				META_ValueRangePropertyWithSetCall(float	,MaxDistanceKm,10.f,5000.f	,SetRecalculate	,"The maximum distance for the fade.")		
				META_ValueRangePropertyWithSetCall(float	,MaxAltitudeKm,1.f,100.f	,SetRecalculate	,"The maximum altitude that will be used for fade and sky colour calculations. Few aircraft can fly higher than 15km.")
				META_ValuePropertyWithSetCall(bool			,EarthTest					,SetRecalculate	,"Whether light should stop at the planet's surface - normally false.")
				META_ValueRangePropertyWithSetCall(unsigned	,TableHeight,2,2048			,SetRecalculate	,"The height of the table, i.e. the number of elevation steps.")
				META_ValueRangePropertyWithSetCall(unsigned	,TableWidth,2,512			,SetRecalculate	,"The width of the table, i.e. the number of distance steps.")
				META_ValueProperty(bool						,FillTexturesAsBlocks		,"If true, the callback object should implement FillFadeTextureBlocks instead of FillFadeTexturesSequentially. This will depend on the graphics API.")
				META_ValueProperty(bool						,CalculateAllAltitudes		,"If true, all altitudes are calculated for fades, otherwise only the current altitudes; set this true if fade textures cannot be partially updated in the graphics API while being used, e.g. DirectX 11.")
				META_ValueProperty(int						,StartDayNumber				,"Day Number from 12am, January 1st, 2000")
        META_ValueProperty(bool						,UseKeyframerDate				,"If sate from keyframes should be used")
				META_ValuePropertyWithSetCall(bool			,UseTbb						,ChangedTbb		,"Use Intel(R) Threading Building Blocks(TM) for automatic multi-threading.")
				META_ValuePropertyWithSetCall(bool			,LinkKeyframeTimeAndDaytime	,LinkChanged	,"Whether to make daytime the same as a keyframe's time value. Otherwise, daytime is an independent value, relative to the start date.")
				META_ValuePropertyWithSetCall(bool			,AutoBrightness				,SetRecalculate	,"Whether the brightness of each keyframe is calculated automatically based on the available, sun, moon, or starlight.")
				META_ValueRangePropertyWithSetCall(float	,AutoBrightnessStrength		,0.f,1.f,SetRecalculate	,"For AutoBrightness, how strongly the brightness is adjusted for darker skies.")
				META_ValueProperty(float,StarBrightness,"The brightness multiplier for all stars.")
				META_ValuePropertyWithSetCall(float,MaxStarMagnitude,StarsChanged,"The maximum magnitude for stars - larger values are dimmer, the human eye can see up to magnitude 6 or 7.")		
			META_EndProperties
			META_BeginStates
				META_ValueRangeStateWithSetCall(float		,AltitudeKM		,0.f,100.f	,AltitudeChanged,"Altitude in km of the viewer - set this each frame.")
			META_EndStates
			
			//! What date is referred to as time zero.
			void GetStartDate(int &y,int &m,int &d) const;
			//! Set the date for time=0.
			void SetStartDate(int y,int m,int d);

			void SetLatitude(float lat_radians);
			float GetLatitude() const;

			void SetSkyInterface(SkyInterface *s);
			SkyInterface *GetSkyInterface();
			
			virtual float GetDaytime() const;
			virtual void SetDaytime(float d);
			
			virtual void SortKeyframes();

			// Implementing BaseSkyInterface:
			virtual float GetTime() const;
			virtual void SetTime(float d);
			virtual float GetMieEccentricity() const;
			virtual const float4 &GetDirectionToLight() const;
			virtual const float4 &GetDirectionToSun() const;
			virtual const float4 &GetDirectionToMoon() const;
			virtual float4 GetLocalIrradiance(float altitude_km) const;
			virtual const float4 &GetAmbientLight(float altitude_km) const;
			virtual float4 GetMieRayleighRatio() const;
			virtual float4 GetIsotropicColourLossFactor	(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const;
			virtual float4 GetIsotropicInscatterFactor	(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const;
			virtual float4 GetInscatterAngularMultiplier(float cosine,float mie_factor,float alt_km=0) const;
			virtual float GetVisibilityDistance(float view_alt_km);
			virtual float GetHazeForVisibilityDistance(float view_alt_km,float horiz_dist_km);
			int GetAltitudeIndex() const;
			float GetAltitudeInterpolation() const;
			void SetCallback(SkyTexturesCallback *c);
			void SetAltitudeRangeKm(float min_km,float max_km);
			void TimeStep(float dt);
			virtual float GetDistanceToSpace(float Elevation,float h_km) const;
			virtual float GetDistanceToEarth(float Elevation,float h_km) const;
			//! Per-frame update call, this function updates the interpolation and the tables.
			virtual void Update();
			// from FadeTableInterface:
			float4 GetIsotropicColourLossFactor(float elevation,float min_dist_km,float max_dist_km) const;
			float4 GetIsotropicInscatterFactor(float elevation,float min_dist_km,float max_dist_km) const;
			// Saveable parameters:
			bool GetAutomaticSunPosition() const;
			bool GetAutomaticMoonPosition() const;
			void SetAutomaticSunPosition(bool val);
			void SetAutomaticMoonPosition(bool val);
			//
			virtual void Reset();
			//! How many altitudes to calculate - minimum 1, maximum 64.
			void SetNumAltitudes(unsigned a);
			unsigned GetNumAltitudes() const;
			void SetAltitude(unsigned i,float a);
			float GetAltitude(unsigned i) const;
			//! Returns the texture coordinate for altitude - constant per-frame.
			float GetAltitudeTexCoord() const;
			float GetInterpolation();
			SkyTexturesCallback *GetCallback();

			void SetOvercastCallback(simul::sky::OvercastCallback *c);
			void SetRecalculate();
			void KeyframesChanged(){SetRecalculate();}
			//! Create a number of evenly spaced keyframes.
			void SetUniformKeyframes(int Steps,float range=1.f);
			float GetHazeForVisibilityDistance(int keyframe_index,float alt_km,float dist_km);
			float GetVisibilityDistance(int keyframe_index,float alt_km);

			//! The keyframe structure for simul::sky::SkyKeyframer
			SIMUL_SKY_EXPORT_STRUCT SkyKeyframe : public BaseKeyframe
			{
				float daytime;					//!< The time this keyframe represents, in a floating-point number of days from the starting midnight of the sequence. If simul::sky::SkyKeyframer::SetLinkKeyframeTimeAndDaytime is true, this is the same as the keyframe's time value.
				float4 Rayleigh;				//< The Rayleigh scattering coefficients (x=red,y=green,z=blue).
				float4 Mie;						//< The Mie scattering coefficients (x=red,y=green,z=blue).
				float Haze;						//< The amount of haze, mist or fog.
				float HazeBaseKm;				//< The base altitude, above which haze starts to decrease in density.
				float HazeScaleKm;				//< The vertical scale over which haze reduces with altitude.
				float HazeEccentricity;			//< The anisotropy of Mie scattering.
				float SunAzimuth;				//< Horizontal angle to the sun. This will be overridden if sun position is automatic.
				float SunElevation;				//< Angle of the sun above the horizon. This will be overridden if sun position is automatic.
				float MoonElevation;			//< Angle of the moon above the horizon. This will be overridden if moon position is automatic.
				float MoonAzimuth;				//< Horizontal angle to the moon. This will be overridden if moon position is automatic.
				float BrightnessAdjustment;		//< A multiplier for brightness of the scene at this time.
				bool UseMoon;					//< Whether the moon, and not the sun, should be used for light direction.
				unsigned OvercastChecksum;
				SkyKeyframe():BaseKeyframe(),daytime(0.5f),Rayleigh(0.00749f,0.01388f,0.02878f,0),
					Mie(0.0046f,0.0046f,0.0046f,0),
					Haze(1.f),HazeBaseKm(1.f),HazeScaleKm(0.5f),HazeEccentricity(0.82f),
					SunAzimuth(0.f),SunElevation(0.f),
					MoonAzimuth(0.f),MoonElevation(0.f),UseMoon(false)
					,BrightnessAdjustment(1.f),OvercastChecksum(0)
				{
				}
				virtual ~SkyKeyframe(){};
				std::ostream &Save(std::ostream &os) const;
				std::istream &Load(std::istream &is);
				virtual bool HasValue(const char *name) const;
				virtual float GetValue(const char *name) const;
				virtual void SetValue(const char *name,float val);
				virtual int GetInt(const char *name) const;
				virtual void SetInt(const char *name,int val);
				virtual unsigned GetPropertiesChecksum() const;
			};
			BaseKeyframe *InsertKeyframe(float t);
			//! The next keyframe not partially built or being used. This keyframe's properties can be modified without any pause or rebuild.
			SkyKeyframe *GetNextModifiableKeyframe();

			std::ostream &Save(std::ostream &os) const;
			std::istream &Load(std::istream &is);
			// Clear the keyframe sequence:
			void New();
			//
			virtual float4 GetKeyframeHorizonColour(int k);
			virtual float4 GetKeyframeZenithColour(int k);
			virtual float4 GetKeyframeSunlightColour(int k);

			//! Convert a floating-point daytime into hour, minute, second and milliseconds.
			static void DaytimeToClockTime(float &t,int &d,int &h,int &m,int &s,int &ms);
			Stars stars;

      void CalcAutoBrightnessForKeyframe(SkyKeyframe* K);

      void SetSunOrMoon(bool use, float value);
		protected:
			void StarsChanged();
			void UpdateMaxDistanceTexture();
			float CalcMaxDistanceKm(float alt_km,float elev) const;
			virtual void LinkChanged();
			float BrightnessAdjustment;
			void DoInterpolation();
			float4 GetIsotropicColourLossFactor	(float alt_km,float elevation,float dist_km,bool earth_test=false,int steps=0) const;
			float4 GetIsotropicInscatterFactor	(float alt_km,float elevation,float dist_km,bool earth_test=false,int steps=0) const;
			void FillFadeTextures(int alt_index,int texture_index,int texel_index,int num_texels,const float *loss_float4_array,const float *inscatter_float4_array);
			void SetOvercastChecksum(SkyKeyframe *K);
			void ApplySkyKeyframe(SkyKeyframe *K);
			void ChangedTbb();
			int day;
			std::stack<int> old_doy;
			std::stack<float> old_dt;
			std::stack<float> old_SunElevation	;
			std::stack<float> old_SunAzimuth	;
			std::stack<float> old_MoonElevation	;
			std::stack<float> old_MoonAzimuth	;

			virtual void AltitudeChanged();
			mutable bool recalc;
			float interp;
			SkyInterface *skyInterface;
			SiderealSkyInterface *siderealSkyInterface;
			SkyTexturesCallback *callback;
			simul::sky::OvercastCallback *overcastCallback;
			SkyKeyframe interpolatedKeyframe;
			bool InterpolateKeyframe(SkyKeyframe *K);
			void CycleTexturesForward();
			void CycleTexturesBackward();
			float time;
			void RestartNextKeyframe();
			void InitFadeTexture(int i);
			virtual void StoreSky();
			virtual void ApplySkyKeyframe(int i);
			virtual void RestoreSky();
			virtual void ChangedSkyInterface();
			bool initialized;
			float GetAltInterp(float alt_km) const;
			int GetAltIndex(float alt_km) const;
			struct TexArray
			{
				Texture2D4 loss_lookup[3];
				Texture2D4 inscatter_lookup[3];
				Texture1D4 sky_lookup[3];
				float altitude;
				unsigned fadeTexIndex[3];
				unsigned skyTexIndex[3];
			};
			mutable std::vector<TexArray*> lookups;
			float alt_interp;
			void FillCallbackTexture(int i);
			void FillCallbackTextures();
			void InitFadeTextures();
			float4 next_direction_to_sun;
			float last_alt;
			float UseAltitudeKm;
			unsigned last_checksum;
			int which_texture;
			void CalcOnTheFly(TexArray *tex,int idx) const;

      // 1 = sun, 0 = moon
      float sunOrMoon;
      bool useSunOrMoon;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif