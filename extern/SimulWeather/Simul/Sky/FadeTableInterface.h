#ifndef SIMUL_FADETABLEINTERFACE_H
#define SIMUL_FADETABLEINTERFACE_H
#include "Simul/Sky/Float4.h"
#include "Simul/Base/PropertyMacros.h"
namespace simul
{
	namespace sky
	{
		class SkyTexturesCallback;
		SIMUL_SKY_EXPORT_CLASS FadeTableInterface
		{
		public:
			//! Get the colour loss (x=r, y=g, z=b) of an object at \p elevation radians above the horizon, \p dist_km kilometres from the viewer.
			virtual float4 GetIsotropicColourLossFactor(float elevation,float min_dist_km,float max_dist_km) const=0;
			//! Get the isotropic part of the inscatter (x=r, y=g, z=b) of an object at \p elevation radians above the horizon, \p dist_km kilometres from the viewer.
			//! the w component returned represents the proportion of Mie scattering.
			//! This must then be multiplied by the value of GetInscatterAngularMultiplier() from the corresponding SkyInterface - this
			//! latter step can be done in a pixel shader for best results.
			virtual  float4 GetIsotropicInscatterFactor(float elevation,float min_dist_km,float max_dist_km) const=0;
			//! Reset the tables - do this when conditions must change instantly.
			virtual void Reset()=0;
			//! Reset the tables - do this when conditions must change instantly.
			VIRTUAL_ValueGetAndSet(bool		,UseTbb						,"Use Intel(R) Threading Building Blocks(TM)?")
			VIRTUAL_ValueGetAndSet(bool		,EarthTest					,"Whether light should stop below the planet's radius")

			//! Assign the callback object. The fade table will inform this object of texture updates.
			virtual void SetCallback(SkyTexturesCallback *c)=0;
			virtual float GetAltitudeTexCoord() const=0;
			/// SkyInterface replacement:
			virtual const float4 &GetAmbientLight(float altitude_km) const=0;

		};
	}
}
#endif
