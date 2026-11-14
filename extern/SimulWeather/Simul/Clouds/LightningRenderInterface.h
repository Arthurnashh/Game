#ifndef SIMUL_GRAPH_CLOUDS_LIGHTNINGRENDERINTERFACE_H
#define SIMUL_GRAPH_CLOUDS_LIGHTNINGRENDERINTERFACE_H
#include "Simul/Clouds/Export.h"
#include "Simul/Base/PropertyMacros.h"

namespace simul
{
	namespace math
	{
		class Vector3;
	}
	namespace clouds
	{
		//! The virtual interface for classes that output data used to render lightning.
		class LightningRenderInterface
		{
		public:
			VIRTUAL_ReferenceGetAndSet(simul::math::Vector3	,LightningCentreX		,"Centre position.")
			VIRTUAL_ValueGetAndSet(float		,LightningZoneSize					,"Width of the lightning zone.")
			VIRTUAL_ValueGetAndSet(bool			,LightningEnabled					,"Whether to update lightning.")

		//! Returns the number of lightning bolts, usually between zero and four.
			virtual unsigned GetNumLightSources() const = 0;
		//! Returns the number of branches to be rendered for a given lightning source.
			virtual unsigned GetNumBranches(unsigned source_index) const=0;
		//! Get the width of the specified branch of the specified source. The result has an arbitrary scale.
			virtual float GetBranchWidth(unsigned source_index,unsigned branch_index) const = 0;
		//! Returns the number of linear segments in the branch.
			virtual unsigned GetNumSegments(unsigned source_index,unsigned branch_index) const = 0;
		//! Fills in the 3d vectors x1 and x2 with the start and end points of the segment specified.
			virtual void GetSegmentVertex(unsigned source_index,unsigned branch_index,unsigned segment_index,float &bright1,float x[]) const = 0;
		//! Fills in the 3d vectors x1 and x2 with the start and end points of the segment specified.
			virtual void GetSegmentVertexDirection(unsigned source_index,unsigned branch_index,unsigned segment_index,float d[]) const = 0;

		//! Get the light level at this point due to lightning illumination from the specified source.
			virtual float GetLightAt(unsigned source_index,const float x1[]) const = 0;
		//! Get the number that represents how far along its sequence a given bolt has progressed.
		//! This will be a function of time
			virtual float GetLightSourceProgress(unsigned source_index) const = 0;
			virtual float GetLightSourceBrightness(unsigned source_index) const = 0;

			virtual bool IsSourceStarted(unsigned source_index) const = 0;
			virtual bool CanStartSource(unsigned source_index) const = 0;
			virtual void StartSource(unsigned source_index) = 0;

			virtual const float *GetLightningColour() const = 0;

			virtual const float *GetIlluminationOrigin() const = 0;
			virtual const float *GetIlluminationScales() const = 0;
		};
	}
}

#endif
