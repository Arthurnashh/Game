#ifndef SIMUL_SKY_BASESKYRENDERER_H
#define SIMUL_SKY_BASESKYRENDERER_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Sky/SkyTexturesCallback.h"
#include "Simul/Sky/SkyKeyframer.h"
#include "Simul/Sky/Export.h"
#include "Simul/Graph/Camera/LensFlare.h"
#include <map>
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace sky
	{
		SIMUL_SKY_EXPORT_STRUCT PlanetStruct
		{
			void* pTexturePtr;
			float dir[3];
			float colour[3];
			float angular_radius;
			bool do_lighting;
		};
		class SkyInterface;
		class SiderealSkyInterface;
		class SkyNode;
		class SkyKeyframer;
		class FadeTableInterface;
		class OvercastCallback;
		//! The base class for real-time sky renderers. Derive a class from BaseSkyRenderer to use its built-in features.
		//! Source code is provided for this class
		SIMUL_SKY_EXPORT_CLASS BaseSkyRenderer : public simul::sky::SkyTexturesCallback,public simul::graph::meta::Group
		{
		public:
			BaseSkyRenderer();
			virtual ~BaseSkyRenderer();
			META_BeginProperties
				META_ValueRangePropertyWithSetCall(float,MaxFadeDistanceKm,10.f,5000.f	,MaxFadeDistanceChanged	,"The maximum distance for the fade.")
				META_ValueRangePropertyWithSetCall(unsigned,NumElevations,2,256		,GridChanged	,"How many elevations to use for sky and fade textures; use a power of 2.")		
				META_ValueRangePropertyWithSetCall(unsigned,NumDistances,2,64		,GridChanged	,"How many distances to use for fade textures; use a power of 2.")		
				META_ValueRangePropertyWithSetCall(unsigned,NumAltitudes,1,16		,GridChanged	,"How many altitudes to use for sky and fade textures; use a power of 2.")		
				
				META_ReferenceProperty(std::string,MoonTexture,"Texture filename for the moon.")	
				META_ReferenceProperty(std::string,SunTexture,"Texture filename for the sun.")			
			META_EndProperties
			/*! \name Functions	*/	
			/*\*/
			//! Colour sky being used?
			bool IsColourSkyEnabled();
			void EnableColourSky(bool value);
			//! Get a pointer to the fade table.
			simul::sky::SkyKeyframer *GetSkyKeyframer();
			simul::sky::FadeTableInterface *GetFadeTableInterface();
			//! Get the interface to the sky object so that other classes can use it for lighting, distance fades etc.
			simul::sky::BaseSkyInterface *GetBaseSkyInterface();
			simul::sky::SkyInterface *GetSkyInterface();
			//! Get the sidereal sky interface - if using one, returns NULL otherwise.
			simul::sky::SiderealSkyInterface *GetSiderealSkyInterface();
			//! Inform the sky renderer of a callback to use to get overcast information.
			void SetOvercastCallback(simul::sky::OvercastCallback *ocb);
			//! The texture-coordinate, constant per frame.
			float GetAltitudeTextureCoordinate() const;

			//! Set the camera position - normally called from the render functions.
			void SetCameraPosition(float x,float y,float z);

			//! Returns the current directional light colour as a float4 (x=red, y=green, z=blue, w unused), i.e. sunlight or moonlight. This is a high-dynamic range value.
			simul::sky::float4 GetLightColour();
			//! Returns the current ambient light colour as a float4 (x=red, y=green, z=blue, w unused), i.e. sunlight or moonlight. This is a high-dynamic range value.
			simul::sky::float4 GetAmbientColour();
			//! Returns the direction to the current light source (sun or moon).
			simul::sky::float4 GetDirectionToLight();
			//! Returns the direction to the sun.
			simul::sky::float4 GetDirectionToSun();
			//! Get a value, from zero to one, which represents how much of the sun is visible.
			//! Call this when the current rendering surface is the one that has obscuring
			//! objects like mountains etc. in it, and make sure these have already been drawn.
			//! GetSunOcclusion executes a pseudo-render of an invisible billboard, then
			//! uses a hardware occlusion query to see how many pixels have passed the z-test.
			virtual float CalcSunOcclusion(float cloud_occlusion=0.f);
			//! Get a value, from zero to one, which represents how much of the sun is visible.
			float GetSunOcclusion() const{return sun_occlusion;}
			//! Call this once per frame to update the sky.
			void Update(float dt);

			//! Draw planets in the sky
			void RenderPlanets();
			//! Render a planet with texture \a tex, angular radius \a rad radians, in direction \a dir (x,y,z), with colour \a colr.
			//! If \a do_lighting is true, the planet will be directionally-lit - e.g. moon phases.
			virtual bool RenderPlanet(void* tex,float rad,const float *dir,const float *colr,bool do_lighting)=0;
			//! Get the planet structure identified by \a index.
			PlanetStruct *GetPlanet(int index);
			//! The moon is distinct from other planets because its position is taken from the skyInterface. When the moon is enabled, it will be added
			//! to the planets list and updated automatically.
			void EnableMoon(bool val);
			//! Is the moon being shown?
			bool IsMoonEnabled() const;
			//! Set the direction to the planet identified by \a index.
			void SetPlanetDirection(int index,const float *pos);
			//! Set the colour of the planet identified by \a index.
			void SetPlanetColour(int index,const float *c3);
			//! Set the angular radius of the planet identified by \a index.
			void SetPlanetRadius(int index,float radians);
			//! Create a planet, identified by the \a index, which can be any unique number.
			//! The texture \a tex is an API-specific texture identifier, e.g. a GLuint, or LPDIRECT3D9TEXTURE.
			//! The angular radius will be \a rad, in radians. If \a do_lighting is true, the planet should be directionally lit by the sun
			//! to produce phases.
			void SetPlanet(int index,void* tex,float rad,bool do_lighting);
			//! Set the texture for planet \a index to \a tex, an API-specific texture identifier, e.g. a GLuint, or LPDIRECT3D9TEXTURE.
			void SetPlanetImage(int index,void* tex);

			//! Save a sky sequence, e.g. to a .seq file.
			std::ostream &Save(std::ostream &os) const;
			//! Load a sky sequence, e.g. from a .seq file.
			std::istream &Load(std::istream &is) const;
			//! Clear the sequence data in this renderer, e.g. the sky keyframes.
			void New();
			/*\*/
		protected:
			void MaxFadeDistanceChanged();
			float flare_magnitude,flare_angular_size,sun_angular_size;
			bool moon_enabled;
			std::map<int,PlanetStruct> planets;
#ifdef _MSC_VER
			simul::graph::camera::LensFlare lensFlare;
#endif
			simul::base::SmartPtr<simul::sky::SkyKeyframer> skyKeyframer;
			simul::base::SmartPtr<simul::graph::meta::Node> skyNode;

			simul::sky::SkyInterface *skyInterface;
			simul::sky::FadeTableInterface *fadeTableInterface;
			float sun_occlusion;
			unsigned skyTexSize;
			unsigned skyTexIndex;
			unsigned numAltitudes;
			unsigned fadeTexWidth,fadeTexHeight;
			void GridChanged();
			simul::sky::float4 cam_pos;

			virtual bool IsYVertical()=0;

			int moon_index;

			void* moon_texture;
		};
	}
}

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif
