#ifndef SIMUL_GRAPH_CLOUDS_CLOUDINTERFACE_H
#define SIMUL_GRAPH_CLOUDS_CLOUDINTERFACE_H
#include "Simul/Math/Vector3.h"
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Clouds/Export.h"
#include "Simul/Base/PropertyMacros.h"

namespace simul
{
	namespace clouds
	{
		SIMUL_CLOUDS_EXPORT_CLASS CloudClientInterface
		{
		public:
			virtual ~CloudClientInterface(){}
			virtual void Regenerate(){}
			virtual void Relight(){}
			virtual void Rerender(){}
		};
		class HumidityCallbackInterface;
		//! The virtual interface for controlling and reading cloud data.
		SIMUL_CLOUDS_EXPORT_CLASS CloudInterface
		{
		public:
			virtual ~CloudInterface(){}
		// The following macros define a virtual Get... and a virtual Set... function with value semantics:
		// e.g. unsigned GetGridLength() const;
		// and  void SetGridLength(unsigned);

			VIRTUAL_ValueGetAndSet(float	,CloudWidth					,"Width in X (East-West) direction (metres).")
			VIRTUAL_ValueGetAndSet(float	,CloudLength				,"Length in Y (North-South) direction (metres)")
			VIRTUAL_ValueGetAndSet(float	,CloudHeight				,"Height of cloud volume(metres)")

			VIRTUAL_ValueGetAndSet(float	,Humidity					,"Humidity coefficient: more humid = more clouds. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::cloudiness.")
			VIRTUAL_ValueGetAndSet(bool		,Wrap						,"Wrap the cloud data horizontally")

			VIRTUAL_ValueGetAndSet(float	,CentreX					,"X (Eastward) centre of cloud volume (metres)")
			VIRTUAL_ValueGetAndSet(float	,CentreY					,"Y (Northward) centre of cloud volume (metres)")
			VIRTUAL_ValueGetAndSet(float	,CloudBaseZ					,"Z (Vertical) base of cloud volume (metres)")

			VIRTUAL_ValueGetAndSet(unsigned	,GridLength					,"Y (North-South) axis grid size of volumetric texture.")
			VIRTUAL_ValueGetAndSet(unsigned	,GridWidth					,"X (East-West) axis grid size of volumetric texture.")
			VIRTUAL_ValueGetAndSet(unsigned	,GridHeight					,"Z (Up-Down) axis grid size of volumetric texture.")

			VIRTUAL_ValueGetAndSet(unsigned	,RandomSeed					,"Integer random number seed for cloud generation")
			VIRTUAL_ValueGetAndSet(unsigned	,NoiseResolution			,"Resolution of the 3D Perlin noise used to generate clouds")
			VIRTUAL_ValueGetAndSet(unsigned	,NoiseOctaves				,"Number of octaves in the Perlin noise function. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::octaves")
			VIRTUAL_ValueGetAndSet(float	,NoisePersistence			,"Persistence over successive fractal scales of the cloud-generation noise function. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::persistence")
			VIRTUAL_ValueGetAndSet(float	,NoisePeriod				,"Period (in daytime units) over which noise changes")

			VIRTUAL_ValueGetAndSet(float	,OpticalDensity				,"Density of clouds for rendering. Not used by all renderers.")
			VIRTUAL_ValueGetAndSet(float	,FractalAmplitude			,"Size of fractal offsets relative to cloud texels. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::fractal_amplitude")
			VIRTUAL_ValueGetAndSet(float	,FractalWavelength			,"Repeat length of fractal offsets relative to cloud texels.")

			VIRTUAL_ValueGetAndSet(float	,AlphaSharpness				,"How sharply defined the cloud edges are, a setting used by some renderers.")
			VIRTUAL_ValueGetAndSet(	bool	,ThinLayer					,"Draw these clouds as a thin 2D layer? Used by renderers.")

			VIRTUAL_ValueGetAndSet(float	,Extinction					,"Density for lighting calculations (usually close to Optical Density value), typically 0.17 per km. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::extinction")
			VIRTUAL_ValueGetAndSet(float	,LightResponse				,"How brightly the cloud scatters sunlight in all directions. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::direct_light")
			VIRTUAL_ValueGetAndSet(float	,AmbientLightResponse		,"How brightly the cloud scatters ambient light. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::ambient_light")
			VIRTUAL_ValueGetAndSet(float	,SecondaryLightResponse		,"Light contribution from secondary (internal) scattering. A cloud keyframer will override this setting; see simul::clouds::CloudKeyframer::Keyframe::indirect_light")
			VIRTUAL_ValueGetAndSet(float	,SelfShadowScale			,"Scale of self-shadowing, zero=complete,one=none.")
			VIRTUAL_ValueGetAndSet(int		,LightDetail				,"Detail level for lighting.")
			VIRTUAL_ValueGetAndSet(float	,MieAsymmetry				,"Asymmetry parameter, between -1 and 1, for scattering. For clouds, values between 0.5 and 0.9 are most common.")
			VIRTUAL_ValueGetAndSet(float	,MinimumLightElevationDegrees	,"Minimum elevation for lighting - if light direction is below this angle, it will be raised - useful for sunsets.")
			VIRTUAL_ValueGetAndSet(float	,Diffusivity				,"Diffusivity, sharpness of cloud definition.")

			VIRTUAL_ReferenceGetAndSet(simul::math::Vector3	,WindOffset	,"3D wind offset, in metres.")
			VIRTUAL_ValueGetAndSet(	bool	,UseTbb						,"Use Intel(R) Threading Building Blocks(TM)?")
			VIRTUAL_ValueGetAndSet(	bool	,ThreadSafety				,"Enforce thread-safety? Use if calling the same interface from multiple threads.")

			VIRTUAL_ValueGetAndSet(float	,Time						,"A time value, for noise animation. A cloud keyframer will override this setting; see simul::sky::BaseKeyframer::Keyframe::time")
		//! get the effective origin in world co-ordinates, including wind offsets:
			virtual const simul::math::Vector3 &GetOrigin() const=0;
		//! get the effective inverse scales - inverse width, length, height - for texture co-ordinates:
			virtual const simul::math::Vector3 &GetInverseScales() const=0;
		//! Create the cloud volume density data from the seeding values. The return value is legacy, should always return true.
			virtual bool Generate()=0;
		//! Relight the entire cloud volume. Returns false if unable to relight, true if successful or not needed.
			virtual bool ReLight()=0;
		//! Calculate the cloud volume ambient and direct light values based on the given light (sun) direction.
			virtual bool ReLight(const simul::math::Vector3 &lightDir)=0;
		//! A number that is changed every time the clouds are regenerated.
			virtual int GetGenerationNumber() const=0;
		//! A number that is changed every time the clouds are relit.
			virtual int GetRelightNumber() const=0;
		//! What proportion of the sky is covered by cloud, when viewed from the sun.
			virtual float GetCoverage() const=0;
		//! At the 3D Position \a X, find the optical density of cloud in the direction \a dir.
			virtual float GetOpticalPathLength(const simul::math::Vector3 &X,const simul::math::Vector3 &dir)=0;
      //! find the optical density of cloud in on the line startPoint->endPoint
      virtual float GetOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint)=0;
		//! Get the ambient light value at \a X, between 0 and 1.
			virtual float GetAmbient(const simul::math::Vector3 &X)=0;
		//! Get the directional light value at \a X, between 0 and 1.
			virtual float GetLight(const simul::math::Vector3 &X) const =0;
		//! Get the secondary light value due to directional lighting at \a X, between 0 and 1.
			virtual float GetSecondaryLight(const simul::math::Vector3 &X) const=0;
		//! Get the light direction last used to relight the clouds.
			virtual const simul::math::Vector3 &GetLightDir()=0;
		//! Get the 'light vector' - which is like the light direction, but rescaled to be unity on the light axis.
			virtual const simul::math::Vector3 &GetLightVector() const=0;
		//! Get the lighting main axis - the closest of xyz to the light direction.
			virtual unsigned GetLightAxis() const=0;
		//! Get the physical extents of the cloud volume in the x, y and z directions.
			virtual void GetExtents(simul::math::Vector3 &X1,simul::math::Vector3 &X2) const=0;
		//! Get the normalized cloud density at real-world position X.
			virtual float GetCloudDensity(const simul::math::Vector3 &X) const =0;
		//! Get density and light values at the given grid position.
			virtual void GetGridValues(float &density,float &light,float &ambient,float &second,int x,int y,int z) const=0;
			//! Get density at the given grid position
			virtual float GetCloud(int x,int y,int z) const=0;
		//! Get the grid width (x size), length (y size) and height (z size).
			virtual void GetGrid(unsigned &width,unsigned &length,unsigned &height) const =0;
		//! Set the grid width (x size), length (y size) and height (z size).
			virtual void SetGrid(unsigned width,unsigned length,unsigned height) =0;

		//! Get the fractal offset scale in metres.
			virtual float GetFractalOffsetScale() const=0;
		//! Get the fractal pattern scale in metres.
			virtual float GetFractalRepeatLength() const=0;

		//! Have properties changed that cause recalculation to be necessary?
			virtual bool NeedsRecalculation() const=0;
		//! Have properties changed that cause relighting to be necessary - either the light has moved
		//! or the cloud density grid has changed.
			virtual bool NeedsRelight() const=0;
		//! Force the recalculation flag to be equal to \param r .
			virtual void SetRecalculate(bool r)=0;
		//! Force relighting of the volume.
			virtual void SetRelight()=0;
			
			//! Set the light direction, but do not relight.
			virtual void SetLightDirection(const simul::math::Vector3 &dir_to_sun)=0;
			//! Initialize a cloud keyframe.
			virtual void StartMarching()=0;
			//! Do a partial update - this incorporates the effect of both MarchPartial and ReLightPartial, no
			//! need to call these in addition to UpdatePartial.
			virtual bool UpdatePartial(float proportion)=0;
			//! Do a partial relight, so that the lighting is complete up to \a proportion of the total, e.g. if \a proportion=1.0, finish the lighting. Returns false if the step is complete, i.e. proportion 1.0 was reached successfully; returns true if there is more still to do.
			virtual bool ReLightPartial(float proportion)=0;
			//! Get a 4x4 matrix to transform from real space to lightspace.
			virtual const float *GetTransformToLightspaceMatrix(unsigned axis,const simul::math::Vector3 &dir) const=0;
			//! Get a 4x4 matrix to transform from lightspace to cloudspace.
			virtual const float *GetTransformLightspaceToCloudspaceMatrix(unsigned axis,const simul::math::Vector3 &dir) const=0;
			//! Get the three (x,y,z) gridsizes for direct and indirect lightspace (for ambient use the density grid).
			virtual const unsigned *GetLightGridsizes() const=0;
			//! When lighting calculation is done externally, this copies the values into the cloud object's own storage.
			virtual void SetLightValues(const float *target_direct_grid,const float *target_indirect_grid,const float *target_ambient_grid)=0;
			//! Get the three (direct,indirect,ambient) extinction factors (light retained = exp(-extinction * density) * light_input for lightspace.
			virtual const float *GetLightspaceExtinctions() const=0;
			//! Assign a callback object to control humidity (cloud density) within the cloud volume.
			virtual void AddHumidityCallback(HumidityCallbackInterface *hc)=0;
			//! Remove the stated callback object from the list.
			virtual void RemoveHumidityCallback(HumidityCallbackInterface *hc)=0;
			//! Remove all humidity callbacks.
			virtual void ClearHumidityCallbacks()=0;
		};
	}
}

#endif
