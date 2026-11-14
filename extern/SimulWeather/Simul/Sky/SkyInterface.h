#ifndef SIMUL_SKY_INTERFACE_H
#define SIMUL_SKY_INTERFACE_H

#include "Simul/Sky/Export.h"
#include "Simul/Base/PropertyMacros.h"

namespace simul
{
	namespace sky
	{
		SIMUL_SKY_EXPORT_STRUCT float4;
		//! An abstract interface class for a sky that is calculated using sidereal data.
		SIMUL_SKY_EXPORT_CLASS SiderealSkyInterface
		{
		public:
			VIRTUAL_ValueGetAndSet(float,HourOfTheDay				,"Time of day in floating-point hours.")
			VIRTUAL_ValueGetAndSet(float,Density					,"Ratio of density at sea-level to earth standard sea-level density")
			VIRTUAL_ReferenceGetAndSet(float4,Rayleigh				,"Rayleigh scattering at sea-level.")
			VIRTUAL_ReferenceGetAndSet(float4,Mie					,"Mie scattering.")
			VIRTUAL_ValueGetAndSet(float,OzoneLevel					,"Amount of Ozone in the atmosphere.")
			VIRTUAL_ValueGetAndSet(float,OzoneLayerAltitude			,"Altitude in km of the Ozone layer.")
			VIRTUAL_ValueGetAndSet(float,Timezone					,"Timezone in hours +/- relative to GMT")
			VIRTUAL_ReferenceGetAndSet(float4,Ozone					,"Ozone absorption coefficients")
			VIRTUAL_ValueGetAndSet(bool,AutomaticSunPosition		,"If true, automatically calculate sun position based on time of day, date and location; if false, sun position is set manually using SetDirectionToSun().")
			VIRTUAL_ValueGetAndSet(bool,AutomaticMoonPosition		,"If true, automatically calculate moon position based on time of day, date, and location; if false, moon position is set manually using SetDirectionToMoon().")
			VIRTUAL_ValueGetAndSet(float,LatitudeRadians			,"Latitude in radians.")
			VIRTUAL_ValueGetAndSet(float,LongitudeRadians			,"Longitude in radians.")
			VIRTUAL_ValueGetAndSet(int,DayOfTheYear					,"Day, from zero to 364 or 365 (in a leap year).")
			VIRTUAL_ValueGetAndSet(int,Year							,"Year, AD/CE.")
			VIRTUAL_ReferenceGetAndSet(float4,ColourWavelengthsNm	,"Wavelengths in nanometres, xyz = red, green, blue. These values are used to calculate the Mie coefficients is MieWavelengthExponent is non-zero, and the Rayleigh coefficients if CalcAutomaticRayleighCoefficients is called.")
			//! Calculate Rayleigh coefficients for air using colour wavelengths ColourWavelengthsNm.
			virtual void CalcAutomaticRayleighCoefficients()=0;
			//! Get the month and day: 1 = January, 12 = December; day is between 1 and 31.
			virtual void GetDate(int &M,int &D) const=0;
			virtual void GetDate(int &M,int &D,int day_of_the_year) const=0;
			virtual void SetDate(int M,int D)=0;
			virtual int DaysInTheYear(int y) const=0;
			//! Get a 4x4 transformation matrix to transform sidereal coordinates into azimuth and elevation.
			virtual const float *SiderealToEarthMatrix() const=0;

		};
		//! An abstract interface class for skies that can calculate optical lengths.
		SIMUL_SKY_EXPORT_CLASS OpticalLengthInterface
		{
		public:
			// OpticalLengthInterface
			virtual float GetOpticalLength(float Elevation,float h_km,float distance_to_edge) const=0;
			//! Get the effective optical length (equivalent length at sea level density) of a ray
			//! cast from altitude \a h_km at angle \a Elevation above the horizon.
			virtual float GetOpticalLength(float Elevation,float h_km) const=0;
			//! Get the effective optical length of haze of a ray cast from altitude \a
			//! h_km at angle \a Elevation above the horizon.
			virtual float GetHazeFactorOpticalLength(float Elevation,float h_km) const=0;
			//! Get the effective optical length of ozone of a ray cast from altitude \a
			//! h_km at angle \a Elevation above the horizon.
			virtual float GetOzoneOpticalLength(float Elevation,float h_km) const=0;
		};

		//! An abstract interface class for skies that can calculate optical lengths.
		SIMUL_SKY_EXPORT_CLASS AtmosphereDensityInterface
		{
		public:
			virtual float GetDensityFactorAtAltitude(float h_km)		const=0;		//!< Return the density at the stated altitude as a proportion of the value at sea level.
			virtual float GetDensityAtAltitude	(float h_km)			const=0;		//!< Return the density at the stated altitude.
		};
			
		//! An abstract interface class for skies.
		SIMUL_SKY_EXPORT_CLASS BaseSkyInterface
		{
			VIRTUAL_ValueGetAndSet(float,Time	,"Time value, usually as a daytime from 0 to 1. This will be overriden by simul::clouds::BaseWeatherRenderer::SetTime, so use that function to control time-of-day if possible.")
			VIRTUAL_ValueGet(float,MieEccentricity									,"Eccentricity of Mie (haze) scattering in the sky.")
			//! Get the direction to the current strongest light source - could be the sun or the moon.
			virtual const float4 &GetDirectionToLight() const=0;
			//! Get the direction to the sun.
			virtual const float4 &GetDirectionToSun() const=0;
			//! Get the direction to the moon.
			virtual const float4 &GetDirectionToMoon() const=0;
			//! Get the sunlight that reaches the altitude \a altitude_km.
			virtual float4 GetLocalIrradiance(float altitude_km) const=0;
			//! Get the ambient light irradiance at the specified altitude.
			virtual const float4 &GetAmbientLight(float altitude_km) const=0;
			//! Get the ratio of Mie to Rayleigh scattering at unit air and haze density. A useful helper function as some shaders use this ratio.
			virtual float4 GetMieRayleighRatio	() const=0;
			//! Get the loss factor of an object at position \a pos_km seen from \a view_pos_km.

			//! When an object has initial colour \a c, multiply each member of \a c by the corresponding member of
			//! the loss factor to get the colour as seen by the viewer.
			virtual float4 GetIsotropicColourLossFactor	(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const=0;
			//! Get the inscatter factor in the xyz - the 3-element factor which is multiplied by the 3-element factor Beta to get the inscattered colour.
			//! In the w element is the haze factor.
			virtual float4 GetIsotropicInscatterFactor	(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const=0;
			//! Get the multiplier for an inscatter factor that gives the total inscatter, where \a cosine is the cosine between
			//! the angle to the sun and the ray direction, \a mie_factor is the proportional haze factor (stored int the w element of the inscatter factor)
			//! and \a alt_km is the viewing altitude in km.
			virtual float4 GetInscatterAngularMultiplier(float cosine,float mie_factor,float alt_km=0) const=0;
			//! From the altitude specified, the distance in km to 10% visibility (mainly a factor of fog/haze).
			virtual float GetVisibilityDistance(float view_alt_km)=0;
			//! Get the haze that will produce the required visibility distance in the horizontal direction.
			virtual float GetHazeForVisibilityDistance(float view_alt_km,float horiz_dist_km)=0;
			//! Considering the atmosphere as a shell of thickness GetAtmosphereThickness(), from altitude \a h_km,
			//! this function returns how far a ray cast at angle \a Elevation above the horizon would travel
			//! to reach the outer radius of the shell.
			virtual float GetDistanceToSpace(float Elevation,float h_km) const=0;
			//! From the specified altitude h_km, and in the direction of Elevation, how far is it in Km to the surface?
			virtual float GetDistanceToEarth(float Elevation,float h_km) const=0;
		};
		//! An abstract interface class for skies.
		SIMUL_SKY_EXPORT_CLASS SkyInterface : public BaseSkyInterface
		{
		public:
			VIRTUAL_ValueGetAndSet(float,PlanetRadius								,"Radius of the planet, default 6378 km (Earth)")
			VIRTUAL_ValueGetAndSet(float,AtmosphereThickness						,"Maximum height in Km, effectively where the atmosphere stops.")
			VIRTUAL_ReferenceGetAndSet(float4,SunIrradiance							,"RGB irradiance of the sun in space, in watts per square metre.")
			VIRTUAL_ReferenceGetAndSet(float4,Starlight								,"RGB irradiance of the stars in watts per square metre.")

			VIRTUAL_ValueGetAndSet(float,Haze										,"Haze, the level of non-gaseous particles in the atmosphere.")
			VIRTUAL_ValueGetAndSet(float,HazeScaleHeightKm							,"Haze scale height, representing the drop-off from ground level.")
			VIRTUAL_ValueGetAndSet(float,HazeBaseHeightKm							,"Haze base height, the height above ground level where haze starts to fall off.")
			VIRTUAL_ValueSet(float		,MieEccentricity							,"Eccentricity of Mie (haze) scattering in the sky.")
			VIRTUAL_ValueGetAndSet(float,Overcast									,"The overcast effect strength.")		
			VIRTUAL_ValueGetAndSet(float,OvercastBaseKm								,"The base altitude for the overcast effect.")
			VIRTUAL_ValueGetAndSet(float,OvercastRangeKm							,"The altitude range for the overcast effect, e.g. thickness of the cloud layer.")
			
			VIRTUAL_ValueGetAndSet(float,SunRadiusArcMinutes						,"Angular size of the sun.")
			VIRTUAL_ValueGetAndSet(float,MoonRadiusArcMinutes						,"Angular size of the moon.")
			VIRTUAL_ValueGetAndSet(float,AzimuthToSunRadians						,"Azimuthal angle towards the sun, in radians.")
			VIRTUAL_ValueGetAndSet(float,ElevationToSunRadians						,"Elevation angle towards the sun, in radians.")
			VIRTUAL_ValueGetAndSet(float,AzimuthToMoonRadians						,"Azimuthal angle towards the moon, in radians.")
			VIRTUAL_ValueGetAndSet(float,ElevationToMoonRadians						,"Elevation angle towards the moon, in radians.")
			VIRTUAL_ReferenceGetAndSet(float4,DirectionToSun						,"Direction towards the sun. Use SetDirectionToSun to override the automatic sun direction that is calculated due to time and location.")
			VIRTUAL_ReferenceGetAndSet(float4,DirectionToMoon						,"Direction towards the moon. Use SetDirectionToMoon to override the automatic direction that is calculated due to time and location.")


		/*! \name General	*/
		/*\*/
			virtual unsigned GetAtmosphericPropertiesChecksum() const=0;
			virtual unsigned GetAtmosphericStateChecksum() const=0;

			//! Get the sky colour in direction \a dir where the viewer as at height \a altitude_km. 
			virtual float4 GetSkyColour			(const float4 &dir,float altitude_km) const=0;
			//! Get the loss factor of an object at position \a pos_km seen from \a view_pos_km.

			//! When an object has initial colour \a c, multiply each member of \a c by the corresponding member of
			//! the loss factor to get the colour as seen by the viewer.
			virtual float4 GetColourLossFactor	(const float4 &view_pos_km,const float4 &pos_km,bool earth_test=false,int steps=0) const=0;

			//! Get the inscatter factor of an object at position \a pos_km seen from \a view_pos_km.
			//virtual float4 GetIsotropicInscatterFactor	(const float4 &view_pos_km,const float4 &pos_km,bool earth_test=false,int steps=0) const=0;
			virtual float4 GetInscatter			(const float4 &view_pos_km,const float4 &pos_km,bool earth_test=false,int steps=0) const=0;

			//! Get the approximate inscatter, ignoring the angle to the sun.
			virtual float4 GetIsotropicInscatter		(float alt_km,float elevation,float dist_km,bool earth_test=false,int steps=0) const=0;

			//! Get the amount of ozone present at altitude \a alt_km. This is a dimensionless number.
			virtual float GetOzoneAtAltitude	(float alt_km) const=0;
			//! Get the amount of haze present at altitude \a alt_km.
			virtual float GetHazeAtAltitude		(float alt_km) const=0;
			//! Get the amount of haze present at altitude \a alt_km compared to the amount at sea level. This value is 
			//! independent of the overall Haze multiplier ( see GetHaze() ).
			virtual float GetHazeFactorAtAltitude(float alt_km) const=0;
			//! Get the strength of the overcast effect at a given altitude, determined by Overcast, OvercastBaseKm, and OvercastRangeKm.
			virtual float GetOvercastAtAltitude(float alt_km) const=0;

			//! Get the Rayleigh scattering coefficients for standard sea-level density (1.225 kg/m^3).
			virtual float4 GetRayleighCoefficients() const=0;
			//! Get the Mie scattering coefficients at sea level for a Haze value of 1.
			virtual float4 GetMieCoefficients	() const=0;
			virtual float4 GetSunlightColour	(const float4 &pos_km)	const=0;		//!< Return the colour of sunlight passing through the stated position.
			virtual float4 GetSunlightColour	(float alt_km)			const=0;		//!< Return the colour of sunlight at the stated altitude.
			virtual float4 GetSunlightFactor	(const float4 &pos_km)	const=0;		//!< Return the colour of sunlight passing through the stated position as a multiplier of the sun radiance in space.
			virtual float4 GetSunlightFactor	(float alt_km)			const=0;		//!< Return the colour of sunlight passing through the stated altitude as a multiplier of the sun radiance in space.
			
			virtual float4 GetApparentDirectionToSun() const=0;
			//! Get the moonlight that reaches the altitude \a altitude_km.
			virtual float4 GetMoonIrradiance(float altitude_km) const=0;
			virtual void GetMoonPosition		(float &az,float &el) const=0;			//!< Get the position of the moon.

      // LUKE
    private:
        float moonIrradianceCoef;
    public:
      SkyInterface(): moonIrradianceCoef(1.0f) {}
      float GetMoonIrradianceCoef() const {return moonIrradianceCoef;}
      void SetMoonIrradianceCoef(float val) {moonIrradianceCoef = val;}
		};
	}
}

#endif
