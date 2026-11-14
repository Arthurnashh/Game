#ifndef SIMUL_GRAPH_CLOUDS_THUNDERCLOUDNODE_H
#define SIMUL_GRAPH_CLOUDS_THUNDERCLOUDNODE_H
#include "Simul/Clouds/FastCloudNode.h"
#include "Simul/Clouds/LightningRenderInterface.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
	#pragma warning(disable:4541)
	#pragma warning(disable:4275)
#endif

namespace simul
{
	namespace math
	{
		class Vector3;
		class Noise1D;
	}
	namespace geometry
	{
		class SimulOrientation;
	}
	namespace clouds
	{
		//! \brief A cloud class that generates lightning.

		//! The ThunderCloudNode is a specialization of CellularCloudNode that implements
		//! the LightningRenderInterface.
		SIMUL_CLOUDS_EXPORT_CLASS ThunderCloudNode :
			 public FastCloudNode
			,public LightningRenderInterface
		{
		public:
			ThunderCloudNode();

			META_BeginProperties
			META_PropertyCategory(Lightning)
				META_ValueRangeProperty(unsigned	,MaxBranches		,12,0,24							,"Max total lightning branches")
				META_ValueRangeProperty(float		,BranchAngle		,3.141f/6.f		,0	,3.141f/2.f		,"Mean angle between one branch and its sub-branch.")
				META_ValueRangeProperty(float		,BranchAngleVariance,3.141f/12.f	,0	,3.141f/4.f		,"Variance of the angle between one branch and its sub-branch.")
				META_ValueRangeProperty(float		,BranchWidthFactor	,0.7f			,0	,1.f			,"Ratio of the width of a branch to its parent.")
				
				META_ValueRangeProperty(float		,Straighten			,0.45f			,0	,1.f			,"How straight to keep the branches.")
				META_ValueRangeProperty(float		,KinkAngle			,0.4f*3.141f	,0	,3.141f/2.f		,"Maximum Angle each branch segment can turn from the previous by elevation.")
				META_ValueRangeProperty(float		,AzimuthKinkAngle	,0.1f*3.141f	,0	,3.141f/8.f		,"Maximum Angle each branch segment can turn from the previous by azimuth.")

				META_ReferenceProperty(simul::math::Vector3
													,LightningCentreX										,"Centre position.")
				META_ValueRangeProperty(float		,LightningZoneSize	,10000.f		,0	,200000.f		,"Width of the lightning zone.")
				META_ValueRangeProperty(float		,LightningZoneMin	,30000.f		,0	,100000.f		,"Minimu distance of lightning.")
			META_EndPropertyCategory(Lightning)
			META_EndProperties
			
			META_BeginStates
				META_ValueState(bool		,LightningEnabled	,"Whether to update lightning.")
			META_EndStates
			Macro_EnableRecursiveStreaming(FastCloudNode)
			Macro_Node(ThunderCloudNode)

			void GenerateLightning();
			bool UpdateLightning();
			virtual bool TimeStep(float dt);
		// The virtual interface for that outputs data used to render lightning.
		// LightningRenderInterface
			virtual unsigned GetNumBranches(unsigned source_index) const;
			virtual float GetBranchWidth(unsigned source_index,unsigned branch_index) const;
			virtual unsigned GetNumSegments(unsigned source_index,unsigned branch_index) const;
			virtual void GetSegmentVertex(unsigned source_index,unsigned branch_index,unsigned segment_index,float &bright1,float x[]) const;
			virtual void GetSegmentVertexDirection(unsigned source_index,unsigned branch_index,unsigned segment_index,float d[]) const;
			virtual unsigned GetNumLightSources() const;
			virtual float GetLightAt(unsigned source_index,const float x1[]) const;
			virtual float GetLightSourceProgress(unsigned source_index) const;
			virtual float GetLightSourceBrightness(unsigned source_index) const;

			virtual bool IsSourceStarted(unsigned source_index) const;
			virtual bool CanStartSource(unsigned source_index) const;
			virtual void StartSource(unsigned source_index);

			virtual const float *GetLightningColour() const;

			virtual const float *GetIlluminationOrigin() const;
			virtual const float *GetIlluminationScales() const;
		protected:
			float SegmentLength;
			float lightning_colour[4];
			simul::math::Noise1D *noise1d;
			virtual void PrepareToDestroy();
			~ThunderCloudNode();
			SIMUL_CLOUDS_EXPORT_STRUCT Segment
			{
				simul::math::Vector3 x1,x2;
				float distance;
				mutable float bright1;
				mutable float bright2;
			};
			SIMUL_CLOUDS_EXPORT_STRUCT Branch
			{
				std::vector<Segment> segments;
				std::vector<Branch*> branches;
				unsigned branch_limit;
				simul::math::Vector3 root_pos;
				float root_distance;
				float width;
				float spread;
				float root_elevation;
				float azimuth;
			};
			struct Source
			{
				std::vector<Branch> branches;
				float progress;
				float noise_multiplier;
				
				float return_stroke;
				float return_spread;

				float end;
				bool finished;
				bool building;
				bool started;
				mutable float light_source_brightness;
				simul::math::Vector3 root_pos;
				Source()
				{
					started=false;
					finished=false;
					building=true;
					progress=0.f;
					end=10.f;
					light_source_brightness=0.f;
					root_pos.Define(0,0,0);
				}
				~Source()
				{
					branches.clear();
				}
			};
			std::vector<Source> sources;
			Source *building_source;
			std::vector<Branch*> building_branches_stack;
			int building_max_segments;
			float building_length;
			std::vector<unsigned> light_source_num_branches;
			simul::math::RandomNumberGenerator random;
			bool GenerateBranch(CloudInterface *ci);
			Source *SeedLightning();
			bool BranchLightning();
			float time_step;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif
