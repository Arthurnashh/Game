#ifndef SIMUL_COLOURSKY_H
#define SIMUL_COLOURSKY_H


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
/*! ColourSky generates sky and fade data from a few colour and other inputs.
*/
		SIMUL_SKY_EXPORT_CLASS ColourSky : public BaseSky, public OpticalLengthInterface, public AtmosphereDensityInterface
		{
		public:
			ColourSky();
			virtual ~ColourSky();
			META_BeginProperties
				META_ReferenceProperty(float4,Horizon,"The colour of the horizon.")
				META_ReferenceProperty(float4,Zenith,"The colour of the zenith.")
				META_ReferenceProperty(float4,HazeColour,"The colour of haze.")
				META_ReferenceProperty(float4,Sunlight,"The colour of sunlight.")
			META_EndProperties
			META_BeginStates 
			META_EndStates
			void TimeStep(float dt);
			unsigned GetAtmosphericPropertiesChecksum() const;
			unsigned GetAtmosphericStateChecksum() const;
			float4 GetSkyColour(const float4 &dir,float altitude_km) const;
			float4 GetColourLossFactor(const float4 &,const float4 &,bool,int) const;
			float4 GetInscatter(const float4 &,const float4 &,bool,int) const;
			float4 GetIsotropicColourLossFactor(float,float,float,float,bool,int) const;
			float4 GetIsotropicInscatterFactor(float alt_km,float elevation,float min_dist_km,float max_dist_km,bool earth_test=false,int steps=0) const;
			float4 GetIsotropicInscatter(float alt_km,float elevation,float dist_km,bool earth_test=false,int steps=0) const;
			float4 GetInscatterAngularMultiplier(float cos0,float mie_factor,float alt) const;
			float GetOzoneAtAltitude(float) const;
			float4 GetLocalIrradiance(float) const;
			float4 GetRayleighCoefficients() const;
			float4 GetMieCoefficients() const;
			float4 GetMieRayleighRatio() const;
			float4 GetSunlightFactor(float) const;
			float4 GetSunlightFactor(const float4 &) const;
			float4 GetApparentDirectionToSun() const;
			void GetMoonPosition(float &,float &) const;
			// OpticalLengthInterface
			float GetOpticalLength(float Elevation,float h_km,float distance_to_edge) const;
			float GetOpticalLength(float Elevation,float h_km) const;
			float GetHazeFactorOpticalLength(float Elevation,float h_km) const;
			float GetOzoneOpticalLength(float Elevation,float h_km) const;
			// AtmosphereDensityInterface
			float GetDensityFactorAtAltitude(float h_km)		const;
			float GetDensityAtAltitude(float h_km)				const;
		protected:
			float4 GetSunlightFactor(const float4 &Pos,const float4 &DirectionToLight) const;
			float GetOvercastFactor(float Elevation,float h_km,float distance_to_edge) const;
			float4 GetCoefficients(float alt_km,float elevation,float dist_km,bool earth_test,int steps) const;
			float4 ColourToScatteringCoefficients(float4 colour,float opt_thickness_km,float elevation) const;
			virtual void TimeChanged();
			mutable Texture2D4 optical_lengths_lookup;
			mutable bool opt_lookup_valid;
			bool MakeOpticalLengthLookupTable() const;
			void LookupOpticalLengths(float Elevation,float h_km,float &opt_len,float &vis,float &ozone_opt_len,float &haze_opt_len) const;
		};
		typedef ColourSky ColorSky;
	}
}
#endif

