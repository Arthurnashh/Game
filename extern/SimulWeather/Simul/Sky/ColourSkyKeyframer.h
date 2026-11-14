#ifndef SIMUL_ColourSkyKeyframer_H
#define SIMUL_ColourSkyKeyframer_H

#include "Simul/Sky/Export.h"
#include "Simul/Sky/Binding.h"
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Base/SmartPtr.h"
#include "Simul/Sky/Float4.h"
#include "Simul/Sky/SkyInterface.h"
#include "Simul/Sky/ColourSky.h"
#include "Simul/Sky/Texture1D4.h"
#include "Simul/Sky/Texture2D4.h"
#include "Simul/Sky/SkyTexturesCallback.h"
#include "Simul/Sky/SkyKeyframer.h"

#include "Simul/Base/Referenced.h"
#include "Simul/Sky/ColourSkyNode.h"
#include <vector>
#include <iostream>		
namespace simul
{
	namespace sky
	{
		class SkyInterface;
//! A class that maintains tables of distance-fade values.

//! The ColourSkyKeyframer is used only to control Colour skies (see simul::sky::ColourSky).
		SIMUL_SKY_EXPORT_CLASS ColourSkyKeyframer : public SkyKeyframer
		{
		public:
			ColourSkyKeyframer(SkyInterface *si,SkyTexturesCallback *c=NULL,int fade_height=256,int fade_width=32,float max_dist_km=300.f);
			virtual ~ColourSkyKeyframer();
			virtual void Update();
			struct ColourKeyframe : public SkyKeyframe
			{
				float4 Horizon;
				float4 Zenith;
				float4 Sunlight;
				float4 HazeColour;
				ColourKeyframe():SkyKeyframe(),Horizon(0,0,0,0),Zenith(0,0,0,0),Sunlight(1,1,1,0),HazeColour(1,1,1,0)
				{
				}
				virtual ~ColourKeyframe(){};
				std::ostream &Save(std::ostream &os) const;
				std::istream &Load(std::istream &is);
				virtual bool HasValue(const char *name) const;
				virtual float4 GetColour(const char *name) const;
				virtual void SetColour(const char *name,float4 val);
				virtual unsigned GetPropertiesChecksum() const;
			};
			BaseKeyframe *InsertKeyframe(float t);
			static void GenColours(ColourKeyframe *K);
			void GenColours();
			//
			virtual float4 GetKeyframeHorizonColour(int k);
			virtual float4 GetKeyframeZenithColour(int k);
			virtual float4 GetKeyframeSunlightColour(int k);
			// Implementing BaseSkyInterface:
			virtual float4 GetLocalIrradiance(float altitude_km) const;
			virtual const float4 &GetAmbientLight(float altitude_km) const;
		protected:
			virtual void StoreSky();
			virtual void ApplySkyKeyframe(int i);
			virtual void RestoreSky();
			ColourSky* colourSky;
			void InterpKeyframe(ColourKeyframe *K);
		};
		typedef ColourSkyKeyframer ColorFadeTable;
	}
}

#endif