#ifndef SIMUL_GRAPH_CLOUDS_CELLULARCLOUDNODE_H
#define SIMUL_GRAPH_CLOUDS_CELLULARCLOUDNODE_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/Resource.h"
#include "Simul/Graph/StandardNodes/ShowProgressInterface.h"
#include "Simul/Clouds/CellularCloudGrid.h"
#include "Simul/Graph/Meta/MemoryInterface.h"

#include "Simul/Math/Vector3.h"
#include "Simul/Graph/Meta/TrackerMacros.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
	#pragma warning(disable:4541)
#endif

namespace simul
{
	namespace math
	{
		class Vector3;
	}
}

namespace simul
{
	namespace clouds
	{
		//! A callback class to control the distribution of humidity in a cloud node. The value returned by
		//! GetHumidityMultiplier is multiplied by the calculated cloud density.
		//! Derive your own class from HumidityCallbackInterface and implement GetHumidityMultiplier,
		//! then call \link simul::clouds::FastCloudNode::AddHumidityCallback AddHumidityCallback \endlink
		//! to attach it to the cloud node.
		class HumidityCallbackInterface
		{
		public:
			//! The function that returns how dense clouds should be at the stated position, where x, y and z are between
			//! zero and one in the cloud volume.
			virtual float GetHumidityMultiplier(float x,float y,float z) const=0;
		};
		class TimedNoise3D;
		class DoLightSteps;
	}
}
namespace simul
{
	namespace clouds
	{
		//! A cloud class based on cellular automata.

		//! Don't create a CellularCloudNode directly, instead, use the derived class FastCloudNode.
SIMUL_CLOUDS_EXPORT_CLASS CellularCloudNode: public CellularCloudGrid,
		public simul::graph::meta::Group,
		public simul::graph::meta::ResourceUser<
				simul::graph::standardnodes::ShowProgressInterface>,
		public simul::graph::meta::ResourceUser<
				simul::graph::meta::MemoryInterface>
		{
		public:
			CellularCloudNode();
		public:
			META_BeginProperties
			META_PropertyCategory(Presets)
				DECLARE_PropertyAction(Cumulus,"Preset: create cumulus (fluffy) clouds")
				DECLARE_PropertyAction(Stratus,"Preset: create stratus (overcast) clouds")
				DECLARE_PropertyAction(Cirrus,"Preset: create cirrus (high, thin) clouds")
			META_EndPropertyCategory(Presets)
			META_PropertyCategory(Generation)
				META_ValueRangePropertyWithSetCall(float	,NoisePersistence			,0.f	,1.f	,Recalculate			,"Persistence over successive fractal scales of the cloud-generation noise function.")
				META_ValueRangePropertyWithSetCall(float	,Diffusivity				,0.02f	,1.f	,NeedRelight			,"How diffused the cloud density is.")
			META_EndPropertyCategory(Generation)
			META_PropertyCategory(Position)
				META_ValueRangePropertyWithSetCall(float	,CentreX					,-50000,50000	,ChangedCloudPosition	,"X (Eastward) centre of cloud volume (metres)")
				META_ValueRangePropertyWithSetCall(float	,CentreY					,-50000,50000	,ChangedCloudPosition	,"Y (Northward) centre of cloud volume (metres)")
				META_ValueRangePropertyWithSetCall(float	,CloudBaseZ					,0		,32000	,ChangedCloudPosition	,"Z (Vertical) base of cloud volume (metres)")
			META_EndPropertyCategory(Position)
			META_PropertyCategory(Size)
				META_ValueRangePropertyWithSetCall(float	,CloudWidth					,2000	,500000	,Recalculate		,"Width in X (East-West) direction (metres)")
				META_ValueRangePropertyWithSetCall(float	,CloudLength				,2000	,500000	,Recalculate		,"Length in Y (North-South) direction (metres)")
				META_ValueRangePropertyWithSetCall(float	,CloudHeight				,0		,25000	,Recalculate		,"Height of cloud volume(metres)")
			META_EndPropertyCategory(Size)
			META_PropertyCategory(Lighting)
				META_ValueRangePropertyWithSetCall(float	,Extinction					,0	,5.f		,NeedRelight		,"Density for lighting calculations (usually close to Density value)")
				META_ValueRangePropertyWithSetCall(float	,LightResponse				,0	,2.f		,NeedRender			,"How brightly the cloud scatters sunlight in all directions")
				META_ValueRangePropertyWithSetCall(float	,MieAsymmetry				,0	,0.9f		,NeedRender			,"Mie scattering eccentricity.")
				META_ValueRangePropertyWithSetCall(float	,AmbientLightResponse		,0	,5.f		,NeedRender			,"How brightly the cloud scatters ambient light")
				META_ValueRangePropertyWithSetCall(float	,SecondaryLightResponse		,0	,2.f		,ChangedSec			,"Light contribution from secondary (internal) scattering")
				META_ValueProperty(bool	,SeparateSecondaryLight,"Obsolete.");
				META_ValueRangePropertyWithSetCall(float	,SelfShadowScale			,0	,1.f		,NeedRelight		,"Extent to which self-shadowing fades away.");
				META_ValueRangePropertyWithSetCall(float	,LightExtend				,0	,1.f		,NeedRelight		,"When lighting at low angles, how far back to extend the light volume as a proportion of the the cloud volume, default 0.3. Use this to prevent sharp boundary artifacts near sunrise/sunset.");
				META_ValueRangePropertyWithSetCall(int		,LightDetail				,1	,8			,NeedRelight		,"DEPRECATED: How much detail to use in lighting = how much memory it uses.");
				META_ValueRangePropertyWithSetCall(float	,MinimumLightElevationDegrees,-90.f	,90.f	,NeedRelight		,"Minimum elevation for lighting , default 3 degrees - if light direction is below this angle, it will be raised - useful for sunsets.");
				META_ValueProperty(bool						,UseCUDA														,"Whether to use CUDA (if available, PC only).");
			META_EndPropertyCategory(Lighting)
			META_PropertyCategory(Appearance)
				META_ValueRangePropertyWithSetCall(float	,OpticalDensity				,0	,12.f		,NeedRender			,"Density of clouds for rendering")
				META_ValueRangePropertyWithSetCall(float	,FractalAmplitude			,0	,20.f		,NeedRender			,"Size of fractal offsets relative to cloud texels, default 4.")
				META_ValueRangePropertyWithSetCall(float	,FractalWavelength			,0	,400.f		,NeedRender			,"Repeat length of fractal offsets relative to cloud texels, default 150.")
				META_ValueRangePropertyWithSetCall(float	,AlphaSharpness				,0	,.8f		,NeedRender			,"How sharply defined the cloud edges are")
				META_ValueProperty(					bool	,ThinLayer														,"Draw these clouds as a thin 2D layer?")
			META_EndPropertyCategory(Appearance)
			META_PropertyCategory(Animation)
				META_ValueRangeProperty(			float	,NoisePeriod				,0		,0		,1e23f				,"Period (in daytime units, one day = one unit) over which noise changes")
				META_ReferenceProperty(simul::math::Vector3	,WindOffset														,"3D wind offset, in metres.")
			META_EndPropertyCategory(Animation)
			META_EndProperties
			META_BeginStates
				META_ValueStateWithSetCall(float,Time,DaytimeChanged,"The noise interpolation variable for cloud densities.")
			META_EndStates
			Macro_EnableRecursiveStreaming(simul::graph::meta::Group)
			Macro_VirtualNode(CellularCloudNode)

			BEGIN_TRACKERS
				DECLARE_SINGLE_TRACKER(CellularCloudNode,HumidityCallbackInterface,humidityCallback);							
			END_TRACKERS
			DEFAULT_TRACKER_HANDLING
		public:
			//! Get a 4x4 matrix to transform from real space to lightspace.
			const float *GetTransformToLightspaceMatrix(unsigned axis,const simul::math::Vector3 &dir) const;
			//! Get a 4x4 matrix to transform from lightspace to cloudspace.
			const float *GetTransformLightspaceToCloudspaceMatrix(unsigned axis,const simul::math::Vector3 &dir) const;
			const unsigned *GetLightGridsizes() const
			{
				return light_gridsizes;
			}
			const float *GetLightspaceExtinctions() const;
			void SetLightValues(const float *target_direct_grid,const float *target_indirect_grid,const float *target_ambient_grid);
			simul::math::NoiseInterface *GetNoiseInterface();
	void SetRecalculate(bool r)
	{
		changed = r;
	}
			//! Force relighting of the volume. Usually unnecessary.
			void SetRelight();
	bool NeedsRecalculation() const
	{
		return changed;
	}
	bool NeedsRelight() const
	{
				return GetRelightChecksum()!=relit_checksum;;
	}

			virtual void SaveGrid(std::ostream &os) const;
			virtual void LoadGrid(std::istream &is);

		// These are inherited from CloudInterface	
			virtual bool Generate();
			virtual bool ReLight();
			virtual int GetGenerationNumber() const;
			virtual int GetRelightNumber() const;
			virtual float GetCoverage() const;
			virtual bool ReLight(const simul::math::Vector3 &lightDir);
	virtual float GetOpticalPathLength(const simul::math::Vector3 &X,
			const simul::math::Vector3 &dir);
      virtual float GetOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint);
			virtual float GetAmbient(const simul::math::Vector3 &X);
			virtual float GetLight(const simul::math::Vector3 &X) const;
			virtual float GetSecondaryLight(const simul::math::Vector3 &X) const;
			virtual const simul::math::Vector3 &GetLightDir();
			virtual const simul::math::Vector3 &GetLightVector() const;
			virtual unsigned GetLightAxis() const;
			virtual void GetExtents(simul::math::Vector3 &X1,simul::math::Vector3 &X2) const;
			virtual const simul::math::Vector3 &GetOrigin() const;
			virtual const simul::math::Vector3 &GetInverseScales() const;
			virtual float GetCloudDensity(const simul::math::Vector3 &X) const;
			virtual void GetGrid(unsigned &w,unsigned &l,unsigned &h) const;
			virtual void SetGrid(unsigned w,unsigned l,unsigned h);
			virtual void Seed(unsigned x,unsigned y,bool val);
			virtual void GetGridValues(float &density,float &light,float &ambient,
										float &second,int x,int y,int z) const;
			virtual float GetFractalOffsetScale() const;
			virtual float GetFractalRepeatLength() const;

			virtual void SetLightDirection(const simul::math::Vector3 &dir_to_sun);
			virtual void StartMarching();
			virtual bool UpdatePartial(float proportion);
			virtual bool ReLightPartial(float proportion);

		//! Stream/save to the std::osteam os.
			std::ostream &StreamOut(std::ostream &os) const;
		//! Stream/load from the std::isteam is.
			std::istream &StreamIn(std::istream &is);
		//! Stream/save to the std::osteam os.
			std::ostream &StateStreamOut(std::ostream &os) const;
		//! Stream/load from the std::isteam is.
			std::istream &StateStreamIn(std::istream &is);

		// inherited from MemoryUsageInterface
			virtual unsigned GetMemoryUsage() const;
			

			const simul::math::Vector3 &ConvertLightCoordsToReal(float x,float y,float z) const;
		protected:
			unsigned GetRelightChecksum() const;
			unsigned relit_checksum;
			float lit_humidity;
			float extinction;
			float coverage;
			float total_light_remaining;
			float under_light_multiplier;
			int GenerationNumber;
			int RelightNumber;
			virtual void PrepareToDestroy();
			virtual ~CellularCloudNode();
			unsigned secondary_gridsize;
			virtual void ChangedNoise();
			simul::math::RandomNumberGenerator secondary_random;
			void GetGridCoordsAndInterps(int &grid_x0,int &grid_y0,int &grid_z0,
												 float &interp_x,float &interp_y,float &interp_z,
			const simul::math::Vector3 &X, const simul::math::Vector3 &dir,
			unsigned axis, const unsigned g[3]) const;
			float InterpBooleanGridValues(int grid_x0,int grid_y0,int grid_z0,
											float interp_x,float interp_y,float interp_z) const;
			float InterpGridValues(int grid_x0,int grid_y0,int grid_z0,
											float interp_x,float interp_y,float interp_z,
											const unsigned char *lgrid,const unsigned *lgridsize) const;
			unsigned regen_checksum;
			simul::math::Vector3 X1,X2,DX;
			std::string filename;
	TimedNoise3D *timed_noise;
			unsigned light_gridsize;
			unsigned ambient_gridsize;
			float light_dir[3];
			unsigned char *light_grid;
			unsigned char *ambient_grid;
			unsigned char *secondary_grid;
			unsigned light_gridsizes[3],ambient_gridsizes[3];
	void CalcLightParameters(const simul::math::Vector3 &light, unsigned &axis,
										simul::math::Vector3 &light_vec);
			bool CUDALight(	unsigned char *&lightspace_grid,
			unsigned char *&secondary_grid,
							const simul::math::Vector3 &light_vec,
							const simul::math::Vector3 &orig,
			const simul::math::Vector3 &scales, unsigned axis,
			float global_multiplier, unsigned grid[3]);

			float DoQuicklightStep(	unsigned i,unsigned j,
									unsigned char *lightspace_grid,
									unsigned char *secondary_grid,
									bool separate_secondary,
								const simul::math::Vector3 &light_vec,
								const simul::math::Vector3 &orig,
									const simul::math::Vector3 &scales,
									unsigned axis,
									float primary_ratio,
									float global_multiplier,
									const unsigned grid[3]);
	simul::math::Vector3 ProjectToEdge(const simul::math::Vector3 &X0,
			const simul::math::Vector3 &D, const simul::math::Vector3 &x1,
			const simul::math::Vector3 &x2);

  float InternalCalcOpticalPathLengthFromTo(const simul::math::Vector3 &startPoint,const simul::math::Vector3 &endPoint);
	float InternalCalcOpticalPathLength(const simul::math::Vector3 &X,
			const simul::math::Vector3 &dir);
	void GetCoordsFromPosition(const simul::math::Vector3 &X, unsigned &x,
			unsigned &y, unsigned &z) const;
			void NeedRelight();
			void ChangedCloudPosition();
			void NeedRender();
			void ChangedSec();
			// virtual from grid;
			virtual void Recalculate();
			virtual void ChangedGrid();

			void QuicklightChanged();
			void ChangedCacheNoise();
			void DaytimeChanged();
			unsigned lighting_index;
			unsigned light_axis;
			simul::math::Vector3 light_vector;
			simul::math::Vector3 light_scales;
			simul::math::Vector3 light_origin;
	friend class simul::clouds::DoLightSteps;
			float secondary_extinction;
			void *access_mutex;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif
