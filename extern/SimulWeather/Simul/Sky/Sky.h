#ifndef SIMUL_SKY_H
#define SIMUL_SKY_H


#include "Simul/Sky/Export.h"
#include "Simul/Sky/Binding.h"
#include "Simul/Sky/BaseSky.h"
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Sky/Float4.h"
#include "Simul/Sky/SkyInterface.h"
#include "Simul/Sky/Texture1D4.h"
#include "Simul/Sky/Texture2D4.h"
#include "Simul/Math/RandomNumberGenerator.h"

namespace simul
{
	namespace sky
	{
//! A class that calculates the sky colour.

/*! Sky tracks the time of day, time of year, and viewing location in order to
calculate the relative position of the sun in the sky and hence the lighting.
If you set the year, it can even calculate the position of the moon.
TimeStep() advances the time of day, or you can use SetTime(), or SetHourOfTheDay().

There are three components to an atmosphere in a Simul sky. Most of the colour is determined by the
air density, which is a fixed function of altitude. The air scatters light with Rayleigh scattering, which has three
colour components. You can adjust this scattering using SetDefaultRayleigh(). This value is multiplied
by the density scale to get the final Rayleigh scattering coefficients ( use GetRayleigh() ).

There may also be haze, usually concentrated toward the ground. Haze scatters light differently to air,
it performs Mie scattering, which has a different angular dependence to Rayleigh scattering.

The sky may be overcast, in which case less light reaches the lower altitudes. To achieve this effect, use
SetOvercast(), SetOvercastBaseKm(), and SetOvercastRangeKm() with values obtained from the cloud node.

Finally, Ozone may be present at higher altitudes. Use SetOzoneLevel() to define the amount of ozone (default 0.001),
and SetOzoneLayerAltitude() to determine the altitude where it peaks, in km (default 22km).


\section permanent_atmospheric_properties Permanent Sky Properties
The simul::sky::Sky class stores internal lookup tables to speed calculation. By changing the LookupTableSize property using
simul::sky::Sky::SetLookupTableSize, it is possible to alter the accuracy.

The properties of PlanetRadius, AtmosphereThickness and LookupTableSize are generally set only once, or left at their defaults, as
any change will cause a recalculation of the lookup tables. The same goes for Density, which is the ratio of atmospheric density
at sea level to the standard Earth atmosphere.

Other permanent properties can be changed without recalculating the lookups, although it is best to do so infrequently. These include

The MieEccentricity is a value that affects how strongly Mie-scattered light through haze is dispersed. If eccentricity is high (close to 1),
the scattering is preferentially forward, leading to a glow around the sun, whereas if it is low, the haze is distributed more evenly.
	\image html "eccentricity_low.png"
	\image html "eccentricity_high.png"

The SunIrradiance value determines the colour and strength of sunlight, and is expressed in physical units. The apparent size of the sun is
set using SetSunRadiusArcMinutes.

Sky calculation is performed with ray-tracing, and the number of ray-trace steps is set using \link simul::sky::Sky::SetSteps SetSteps \endlink.
Typically 6 steps are enough, but more may be necessary for high-accuracy, especially when the sun is close to the horizon, such as in this case,
where the number of steps is too low, leading to an over-bright effect on the ground below the sun:
	\image html "sky_steps_too_low.png"

 \section transient_atmospheric_properties Transient Sky Properties

The main factor that changes over time in the sky is the Daytime property, between zero and one. This change can be handled by the
Sky Keyframer, which effectively performs the function of a keyframer for the sky. The Sky Keyframer will also adjust the sky's
Overcast property, a measure of how much the clouds block sunlight from reaching the lower atmosphere. The associated properties
 of OvercastBaseKm and OvercastRangeKm set a base altitude and range in kilometres, at the base altitude, the full overcast
 value is applied, while above the top of the overcast range, there is no loss of light.
 
 The haze value determines how much Mie-scattered haze (i.e. mist or fog) is present. Haze is considered to have a density that falls-off
 exponentially with altitude, so the HazeScaleHeightKm property determines the scaling height for this exponential. The
 HazeBaseHeightKm is also used, below this height, full haze is applied.

 Fog and mist are both effectively low-level clouds. Fog is defined as having visibility less than 1km, it is called mist when visibility
 is between 1 and 2 km.
 
 As time passes and the sun moves, you can use GetAzimuthToSunRadians and GetElevationToSunRadians to find its position.
 The functions GetDirectionToSun and GetDirectionToMoon can also be used.


*/
		SIMUL_SKY_EXPORT_CLASS Sky : public BaseSky, public SiderealSkyInterface, public OpticalLengthInterface
		{
		public:
			Sky();
			virtual ~Sky();
			unsigned GetAtmosphericPropertiesChecksum() const;
			unsigned GetAtmosphericStateChecksum() const;
			META_BeginProperties
				META_ValueRangePropertyWithSetCall(float,Density				,0.f	,10.f				,RemakeSkyLookupTable
																											,"Density at sea level, proportional to the default. It is not usually necessary to change this.")
				META_ReferencePropertyWithSetCall(float4,Rayleigh											,ChangedRayleighCoefficients,"Rayleigh scattering coefficients at standard sea level density, xyz = red, green, blue.")
				META_ReferencePropertyWithSetCall(float4,ColourWavelengthsNm								,ChangedRayleighCoefficients,"Wavelengths in nanometres, xyz = red, green, blue. These values are used to calculate the Mie coefficients is MieWavelengthExponent is non-zero, and the Rayleigh coefficients if AutoRayleighCoefficients is true.")
				
				META_PropertyCategory(HazeAndOvercastValues)
				META_ReferencePropertyWithSetCall(float4,Mie												,ChangedMieCoefficients,"Mie scattering coefficients at standard sea level density, xyz = red, green, blue.")
				
				META_ValueRangePropertyWithSetCall(float,MieWavelengthExponent	, -12.f	,0.f				,CalcMieCoefficients,"A measure of the colour-dependence of Mie scattering, -1.3=default, 0=uniform.")

				META_EndPropertyCategory(HazeAndOvercastValues)
				META_ValueRangePropertyWithSetCall(int	,Steps					,1		,50					,StepsChanged,"The number of integration steps taken to calculate results.")
				META_ValueRangePropertyWithSetCall(float,OzoneLevel				,0.f	,.05f				,OzoneChanged,"Amount of Ozone in the atmosphere.")
				META_ValueRangeProperty(float			,OzoneLayerAltitude		,22.f	,0		,120.f		,"Mean altitude in km of the Ozone layer.")
				META_ValueRangeProperty(float			,Timezone				,0		,-12.f	,11.5f		,"Timezone in hours, +/- relative to GMT.")
				META_ReferencePropertyWithSetCall(float4,Ozone												,OzoneChanged,"Ozone absorption coefficients")
				META_ValueRangePropertyWithSetCall(unsigned,LookupTableSize		,16,256,					RemakeSkyLookupTable
																											,"Size, from 16 to 256, of the table used to store densities and optical lengths.")
																											
				META_ValueProperty(bool,AutoUpdateTables	,"Whether to update the tables automatically when a value changes. Usually true in a game/simulation context.");											
				META_ValueProperty(bool,ThreadSafety		,"Whether to use thread-safety - use if multiple threads may call the same Sky.");
				META_ValueProperty(bool	,AutoIncrementDayOfTheYear	,"If enabled, DayOfTheYear is increased when a Daytime greater than or equal to 1 is set, and decreased if a negative Daytime is set.")
			META_EndProperties
			META_BeginStates 
				META_StateCategory(SunPosition)
					META_ValuePropertyWithSetCall(bool		,AutomaticSunPosition								,UpdateSunPosition,"If true, automatically calculate sun position based on time of day and location; if false, sun position is set manually using SetDirectionToSun().")
					META_ValueRangeStateWithSetCall(int		,DayOfTheYear	,0		,364						,UpdateSunPosition,"Day of the year from 0 to 364.")
					META_ValueRangeStateWithSetCall(float	,LatitudeRadians,-3.14f/2.f,3.14f/2.f				,UpdateSunPosition,"Latitude in radians.")
					META_ValueRangeStateWithSetCall(float	,LongitudeRadians,-3.14159f,3.14159f				,UpdateSunPosition,"Longitude in radians.")
					META_ValueStateWithSetCall(int			,Year												,UpdateSunPosition,"Year - does not affect Sun position, but affects Moon position.")
				META_EndStateCategory(SunPosition)
				META_ValuePropertyWithSetCall(bool			,AutomaticMoonPosition								,UpdateSunPosition,"If true, automatically calculate sun position based on time of day and location; if false, sun position is set manually using SetDirectionToSun().")

			META_EndStates

			//States with no internal variable:
			float GetHourOfTheDay() const;
			void SetHourOfTheDay(float h);

			void TimeStep(float dt);

			virtual const float4 &GetDirectionToLight() const;

			float4	GetSkyColour						(const float4 &vertexDir,const float4 &input,float altitude_km) const;
			float4	GetSkyColour						(const float4 &vertexDir,float altitude_km) const;
			float4	GetColourLossFactor					(const float4 &view_pos_km,const float4 &pos_km,bool earth_test=false,int steps=0) const;
			float4	GetIsotropicInscatterFactor			(const float4 &view_pos_km,const float4 &pos_km,bool earth_test=false,int steps=0) const;
			float4	GetInscatter						(const float4 &view_pos_km,const float4 &pos_km,bool earth_test=false,int steps=0) const;
			float	GetOzoneAtAltitude					(float alt_km) const;

			float4 GetIsotropicColourLossFactor			(float view_alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const;
			//float4 GetIsotropicColourLossCoefficients	(float view_alt_km,float elevation,float dist_km,bool earth_test=false,int steps=0) const;
			float4 GetIsotropicInscatterFactor			(float view_alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const;
			float4 GetIsotropicInscatter				(float view_alt_km,float elevation,float dist_km,bool earth_test=false,int steps=0) const;
			float4 GetInscatterAngularMultiplier		(float cos0,float mie_factor,float alt_km=0) const;

			virtual float GetVisibilityDistance(float view_alt_km);
			virtual float GetHazeForVisibilityDistance(float view_alt_km,float horiz_dist_km);

			float4 GetLocalIrradiance		(float altitude_km) const;

			//! Convert year, month and day into a Julian Day Number.
			static int GetDayNumber(int year,int month,int day);
			static void GetDateFromDayNumber(int d,int &year,int &month,int &day);

			const float *SiderealToEarthMatrix() const;
	//		float4 GetSunlightFactor	(float alt_km)			const;
			
			float GetTimeInSeconds() const;
			void ApplyFade(float4 &outpt,const float4 &pos_km,const float4 &input);
			float4 GetMieRayleighRatio() const;
			float4 GetMieCoefficients() const;
			float4 GetRayleighCoefficients() const;
			float4 GetZenithColour(float altitude_km) const;
			float4 GetHorizonColour(float altitude_km) const;

			float GetSunAzimuth() const;
			float GetSunElevation() const;
			float4 GetApparentDirectionToSun() const;
			void GetMoonPosition(float &az,float &el) const;

			float GetDensityFactorAtAltitude(float h_km) const;
			float GetDensityAtAltitude(float h_km) const; // Density in kg/m^3 at altitude \a h_km (kilometres).

			// OpticalLengthInterface
			float GetOpticalLength(float Elevation,float h_km,float distance_to_edge) const;
			//! Get the effective optical length (equivalent length at sea level density) of a ray
			//! cast from altitude \a h_km at angle \a Elevation above the horizon.
			float GetOpticalLength(float Elevation,float h_km) const;
			//! Get the effective optical length of haze of a ray cast from altitude \a
			//! h_km at angle \a Elevation above the horizon.
			float GetHazeFactorOpticalLength(float Elevation,float h_km) const;
			//! Get the effective optical length of ozone of a ray cast from altitude \a
			//! h_km at angle \a Elevation above the horizon.
			float GetOzoneOpticalLength(float Elevation,float h_km) const;

			static float HenyeyGreenstein(float g,float cos0);

			void InitLookupTables();
			static void CalcAutomaticRayleighCoefficients(float4 &r,const float4 &ColourWavelengthsNm);

			//! Get the month and day.
			void GetDate(int &M,int &D) const;		//< 1 = January, 12 = December
			void GetDate(int &M,int &D,int day_of_the_year) const;
			void SetDate(int M,int D);

			//! How many days in the specified year.
			int DaysInTheYear(int y) const;
		protected:
			virtual void AtmosphereThicknessChanged();
			virtual void PlanetRadiusChanged();
			virtual void SetMoonPosByDirection();
			virtual void StarlightChanged();
			mutable void* sky_mutex;
			mutable void* dens_mutex;
			float4 rayleigh;
			void UpdateSunPosition();
			void UpdateMoonPosition();
			void GetSolarTime(float *solar_hours,float standard_hours,float meridian,float longitude,int day_of_the_year);
			void GetSunPosition(float &azimuth,float &elevation,float declination,float hours);
			void GetSunPosition(float &azimuth,float &elevation,float time_of_day,float longitude,float time_zone);
			void CalcMieCoefficients();
			float GetSiderealTimeAtGreenwich() const;
			float GetLocalSiderealTime() const;
			void AscensionDeclinationToAzimuthElevation(double delta,double RA,float &az,float &el) const;
			float GetDayNumber() const;				// From midnight December 31st 1999 = 1st January 2000
			bool MakeSkyLookupTable() const;
			void RemakeSkyLookupTable() const;
			void MakeDensityLookupTable() const;

			float earth_test_scale;

			mutable Texture2D4 sky_lookup;
			mutable Texture1D4 density_lookup;
			mutable bool density_lookup_valid;
			mutable bool sky_lookup_valid;
			float GetHazeOpticalLength(float Elevation,float h_km) const;
			void Lookup(float Elevation,float h_km,float &opt_len,float &mean_alt,float &distance_to_edge) const;
			void LookupDensities(float h_km,float &dens,float &ozone,float &haze) const;
			simul::math::RandomNumberGenerator random;
			virtual void OzoneChanged();
			virtual void HazeChanged();
			void StepsChanged();
			float *step_centres;
			float *step_lengths;
			void CalcAutomaticRayleighCoefficients();
			void ChangedRayleighCoefficients();
			void ChangedMieCoefficients();
			//BaseSky
			void TimeChanged();
			void SetSunPosByDirection();
			void SetSunPosByAngle();
			void SetMoonPosByAngle();
			float4 GetSunlightFactor(const float4 &Pos,const float4 &DirectionToLight) const;
		};
	}
}
#endif
