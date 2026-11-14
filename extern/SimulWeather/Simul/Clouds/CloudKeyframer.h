#ifndef SIMUL_GRAPH_CLOUDKEYFRAMER_H
#define SIMUL_GRAPH_CLOUDKEYFRAMER_H
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Base/Referenced.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Math/Vector3.h"
#include "Simul/Clouds/Export.h"
#include "Simul/Clouds/CloudRenderCallback.h"
#include "Simul/Clouds/CloudKeyframerInterface.h"
#include "Simul/Sky/OvercastCallback.h"
#include "Simul/Sky/BaseKeyframer.h"
#include <vector>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

namespace simul
{
	namespace sky
	{
		class BaseSkyInterface;
	}
	namespace graph
	{
		namespace standardnodes
		{
			class ShowProgressInterface;
		}
	}
	namespace clouds
	{
		class LightningRenderInterface;
		class CloudInterface;
		//! A class to manage interpolation between cloud states over time.

		/*!
A class to manage interpolation between cloud states over time, and to call the CloudRenderCallback functions when
appropriate. Typically, a CloudKeyframer is created by a cloud node (FastCloudNode, etc), and obtained using GetCloudKeyframer().
Call Update() from within your main loop, and the keyframer will then call back to the object given it by SetRenderCallback()
with texture-update information.

The timescale is in arbitrary units - if time and daytime are linked in the SkyKeyframer, time is in days.

We use simul::clouds::CloudKeyframer::SetUse16Bit to inform the keyframer whether we want it to update 16-bit or 32-bit
volumetric textures. Note also, OpenGL arranges textures differently to DirectX, and to take account of this, use SetOpenGL(true).
*/

		SIMUL_CLOUDS_EXPORT_CLASS CloudKeyframer : public CloudKeyframerInterface, public simul::graph::meta::Group,public simul::sky::OvercastCallback, public sky::BaseKeyframer
		{
		public:
			CloudKeyframer(CloudInterface *ci,LightningRenderInterface *lri=NULL,bool make2d=false);
			~CloudKeyframer();
			META_BeginProperties
				META_ValuePropertyWithSetCall(float		,TimeFactor					,TimeFactorChanged				,"Time factor for wind etc, the default is 60*60*24, the number of seconds in a keyframe time unit.")
				META_ValueProperty(bool					,Use16Bit													,"Use 16-bit (2-byte) per texel textures? Otherwise use 32-bit.")
				META_ValueProperty(bool					,OpenGL														,"Use GL-style byte order? Otherwise, use DirectX-style.")
				META_ValueProperty(bool					,Make2DTextures												,"Make 2D textures? Otherwise make volume textures.")
				META_ValueProperty(bool					,FillTexturesAsBlocks										,"If true, the callback object should implement FillCloudTextureBlock instead of FillCloudTextureSequentially. This will depend on the graphics API.")
				META_ValuePropertyWithSetCall(float		,WindSpeed					,WindSpeedChanged				,"Speed of wind motion of the clouds, in metres per second.")
				META_ValuePropertyWithSetCall(float		,WindHeadingDegrees			,WindHeadingChanged				,"Heading in degrees of wind motion of the clouds, defines the horizontal direction the clouds move in.")
				META_ValueProperty(bool					,OverrideWind												,"If true, Wind is set by WindSpeed and WindHeading directly, not through keyframes.")
				META_ValueProperty(bool					,OverrideRain												,"If true, Precipitation is set directly, not through keyframes.")
			// Properties for user keyframing:
				META_ValuePropertyWithSetCall(bool		,UserKeyframes				,UserKeyframesChanged			,"Whether to use keyframes at set locations.")

				META_ValuePropertyWithSetCall(bool		,CacheClouds				,CacheKeyframes					,"Cache cloud lighting grid? Makes things faster.")
				META_ValueRangeProperty(float			,OvercastEffect				,3.f,0.f,50.f					,"How much light is obscured by an overcast cloud layer.");
				META_ValueRangePropertyWithSetCall(float,PrecalculatedGamma,0.f,1.f	,PrecalculatedGammaChanged		,"A gamma-correction power to apply to cloud lighting, ignored if equal to 1.0 or zero.");
				META_ValueProperty(float,Time0,"");
				META_ValueProperty(float,Time1,"");
				META_ValueProperty(float,Time2,"");
			META_EndProperties
			
			//! Apply the cloud's properties to all keyframes.
			void InitKeyframesFromClouds();
			//! Reset keyframes to 8, evenly spaced.
			void SetUniformKeyframes(int StepsPerDay,float range=1.f);
			//! The keyframe structure for clouds, used by simul::clouds::CloudKeyframer.
			SIMUL_CLOUDS_EXPORT_STRUCT Keyframe : public sky::BaseKeyframe
			{
				float wind_speed;			//< The wind speed in m/s
				float wind_direction;		//< The wind direction
				float precipitation;		//< The amount of rain/snow etc.
				float cloudiness;			//< The density of the cloud layer at this time.
				float cloud_base_km;		//< The base altitude of this cloud layer.
				float cloud_height_km;		//< The height of the cloud layer, above its base altitude.
				float direct_light;			//< The amount of direct light to be used for rendering.
				float indirect_light;		//< The amount of indirect or secondary light to be used for rendering.
				float ambient_light;		//< The amount of ambient light to be used for rendering
				int octaves;				//< The number of noise octaves used to generate the clouds; default 3.
				float persistence;			//< The fractal persistence for generating the clouds, default 0.45.
				float fractal_amplitude;	//< The strength of the fractal edge effect, default 1.8.
				float fractal_wavelength;	//< The wavelength of the fractal edge effect - longer wavelengths produce smoother clouds, default 140.
				float extinction;			//< The amount of light scattered per unit distance - larger values produce darker clouds, default 1.7.
				float rain_to_snow;			//< If zero, precipitation is interpreted as rain. If one, it is snow.
				float diffusivity;			//< How much the edges of clouds should be diffused.
				float lightning;			//< Amount of lightning at this time.
				bool cache_built;			//< Whether the cache has been filled for this keyframe.
				unsigned cache_size_bytes;	//< Size of the cloud cache.
				unsigned char *cache;		//< The cloud cache - this is automatically filled if enabled in CloudKeyframer.
				Keyframe():sky::BaseKeyframe()
					,wind_speed(0)
					,wind_direction(0)
					,cloudiness(0.5f)
					,cloud_base_km(3.f)
					,cloud_height_km(4.f)
					,direct_light(1.f)
					,indirect_light(1.f)
					,ambient_light(1.f)
					,octaves(3)
					,offsetx(0.f)
					,offsety(0.f)
					,precipitation(0)
					,persistence(0.45f)
					,fractal_amplitude(2.2f)
					,fractal_wavelength(220.f)
					,extinction(1.7f)
					,rain_to_snow(0.f)
					,diffusivity(0.01f)
					,cache_built(false)
					,cache_size_bytes(0)
					,cache(NULL)
					,lightning(0.f)
				{
				}
				~Keyframe()
				{
					delete [] cache;
				}
				virtual std::ostream &Save(std::ostream &os) const;
				virtual std::istream &Load(std::istream &is);
				virtual bool HasValue(const char *name) const;
				virtual float GetValue(const char *name) const;
				virtual void SetValue(const char *name,float val);
				virtual int GetInt(const char *name) const;
				virtual void SetInt(const char *name,int val);
				//! Check for properties that, if altered in the keyframe, would require regeneration of
				//! the cloud volume:
				virtual unsigned GetPropertiesChecksum() const;
				float GetOffsetX() const
				{
					return offsetx;
				}
				float GetOffsetY() const
				{
					return offsety;
				}
			protected:
				friend class CloudKeyframer;
				float offsetx;
				float offsety;
			};
			const Keyframe &GetInterpolatedKeyframe() const
			{
				return InterpolatedKeyframe;
			}
			void ApplyCloudKeyframe(const Keyframe *K);
			//! The next keyframe not partially built or being used. This keyframe's properties can be modified without any pause or rebuild.
			Keyframe *GetNextModifiableKeyframe();
			//Keyframe *GetKeyframe(int i);
			Keyframe *InsertKeyframe(float t);
			void DeleteKeyframe(int i);
			//! Number of keyframes.
			virtual int GetNumKeyframes() const;
			void SortKeyframes();
			// implementing OvercastCallback
			void GetOvercastData(float daytime,float &overcast,float &overcast_base_km,float &overcast_scale_km);
			//! Set the callback object. Typically, you would derive your platform-specific cloud-rendering object from CloudRenderCallback,
			//! and this object would also be the owner of the cloud node.
			void SetRenderCallback(CloudRenderCallback *cb);
			//! Set the lightning interface.
			void SetLightningInterface(LightningRenderInterface *lri);
			//! Set the cloud interface. As the constructor needs a CloudInterface, it is not usually necessary to call this.
			void SetCloudInterface(CloudInterface *ci);
			CloudInterface *GetCloudInterface();
			//! Set the sky - this is used to light the clouds over time.
			void SetSkyInterface(simul::sky::BaseSkyInterface *si);
			//! Get the optical path length between two points, taking into account interpolation. The more cloud between the
			//! points, the longer will be the path.
			float GetOpticalPathLength(const simul::math::Vector3 &X,const simul::math::Vector3 &dir);
      //! Get the optical path length between two points, taking into account interpolation. The more cloud between the
      //! points, the longer will be the path. 
      float GetOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint);
			//! Once-per-frame update for the keyframer, call this with an absolute time value (not a time step).
			void Update(float new_time);
			float GetVisibility(const float *pos,const float *dir);
      float GetVisibilityFromTo(const simul::math::Vector3 &startPoint, const simul::math::Vector3 &endPoint);
			//! Get the interpolation - between zero and one. This is based on InterpStepTime and the sky's Daytime property.
			//! When the interpolation reaches one, the textures will be cycled via CloudRenderCallback.
			float GetInterpolation() const
			{
				return interp;
			}
			//! For debugging purposes only.
			void ForceRelight();
			
			//! What is the interpolation value at the time specified? Depends on keyframe positions.
			float GetInterp(int k,float t) const;
			//! How overcast is it beneath the cloud layer? From zero to one. This is an interpolated value.
			float GetOvercastFactor() const
			{
				return overcast_factor;
			}
			//! How much precipitation - depends on cloud cover.
			float GetPrecipitation() const;
			enum GridVariable
			{
				DENSITY=0,BRIGHTNESS,SECONDARY,AMBIENT
			};
			void SetBits(unsigned density,unsigned brightness,unsigned secondary,unsigned ambient,unsigned Bytes_per_texel,bool bigendian);
			void SetBits(GridVariable x,GridVariable y,GridVariable z,GridVariable w);
			//! Make the wind offsets loop based on the loop time.
			void SetLoopWind();

			//! Save and load:
			std::ostream &Save(std::ostream &os) const;
			//! Save and load:
			std::istream &Load(std::istream &is);
			//! Clear keyframes.
			void New();
			//! Check for changed offsets and recalculate.
			void RecalculateOffsets();
			//! Cache all the keyframes so they don't need relit.
			void CacheKeyframes();
			//! Cache the specified keyframe.
			void CacheKeyframe(int k);
			void InvalidateKeyframeCache(int k);
		protected:
			simul::graph::standardnodes::ShowProgressInterface *FindProgressCallback();
			void WindSpeedChanged();
			void WindHeadingChanged();
			void UpdateLightning(float dt);
			bool IsKeyframeChecksumValid(const Keyframe *K) const;
			void CopyToCache(int i,int k);
			void CopyFromCache(int i,int k);
			Keyframe InterpolatedKeyframe;
			void PrecalculatedGammaChanged();
			void FillIlluminationTexture(int i,int texel_index,int num_texels,const unsigned char *uchar8_array);
			void FillCloudTexture(int i,int texel_index,int total_texels,const unsigned *ptr);
			virtual unsigned GetGlobalChecksum() const;
			void RestartNextKeyframe();
			void SendTextureToCallback(int i);
			void UserKeyframesChanged();
			void InterpKeyframe(Keyframe *K);
			void NotCompatibleWithUserKeyframes();
			void IncrementKeyframe();
			void DecrementKeyframe();
			void CycleTexturesForward();
			void CycleTexturesBackward();
			//! Checksum to determine whether any keyframe has altered its position due to wind speed or direction.
			unsigned GetOffsetChecksum() const;
			unsigned offset_checksum;
			void CalcOffsets();
			// Initialization function - call this when SetSkyInterface() has been called, and the sky has the appropriate state
			// that you want to use to initialize the cloud lighting. This will callback to set up the cloud textures.
			void Init();
			void ApplyBits();
			struct Bits
			{
				unsigned density;
				unsigned brightness;
				unsigned secondary;
				unsigned ambient;
				unsigned Bytes_per_texel;
				bool bigendian;
			};
			Bits bits;
			float InitLightDirection(int i);
			void InitTexture(int i);
			void InitCallback();
			void CalculateOvercastFactor();
			void UpdateCloudTexture();
			void TimeFactorChanged();
			CloudRenderCallback *renderCallback;
			CloudInterface *cloudInterface;
			LightningRenderInterface *lightningRenderInterface;
			simul::sky::BaseSkyInterface *skyInterface;
			float interp;
			float interp_time_1;
			unsigned char* illumination_texture[4];
			unsigned illumination_texel_index[4];
			unsigned *fill_texture[3];
			unsigned short *fill_texture_16bit[3];
			size_t fill_texture_size;
			unsigned texel_index;
			float overcast_factor,overcast_factor_1,overcast_factor_2;
			float precipitation,precipitation_1,precipitation_2;
			float last_time;
			simul::math::Vector3 next_light_direction;
			// Stored texture dimensions, to watch out for changes.
			unsigned sent_width;
			unsigned sent_length;
			unsigned sent_height;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif