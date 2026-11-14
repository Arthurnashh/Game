#ifndef SIMUL_TERRAIN_ROAD_H
#define SIMUL_TERRAIN_ROAD_H
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/Client.h"
#include "Export.h"
#include <string>
#include "Simul/Math/Vector3.h"
#include "Simul/Graph/StandardNodes/VisualDebugInterface.h"
#include "Simul/Terrain/RoadInterface.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
	#pragma warning(disable:4541)
#endif
namespace simul
{
	namespace terrain
	{
		SIMUL_TERRAIN_EXPORT_CLASS HeightMapInterface;
		SIMUL_TERRAIN_EXPORT_CLASS Road
			:public simul::graph::standardnodes::VisualDebugInterface
			,public RoadInterface
			,public simul::graph::meta::Node
		{
		public:
			struct SplineNode
			{
				simul::math::Vector3 pos;
			};
			std::vector<SplineNode> splineNodes;
			float length;
			mutable float *data;
			void GetSubSegment(unsigned spline,unsigned segment,float* x1,float* x2) const;
		public:
			Road(HeightMapInterface *hmi=NULL);
			virtual ~Road();
			META_BeginProperties
			META_EndProperties
			Macro_EnableRecursiveStreaming(simul::graph::meta::Node)
			META_ValueRangePropertyWithSetCall(float,Width,1.f,50.f,Changed,"Width of the road");
			Macro_VirtualNode(Road)
			void SetInputHeightMapInterface(HeightMapInterface *hmi);
			//VisualDebugInterface for tools:
			unsigned GetNumLines() const;
			void GetLine(unsigned,float*,float*,int*) const;
			bool Is2D() const {return false;}
			//RoadInterface
			virtual float GetLength() const;
			virtual int GetNumSegments() const;
			virtual int GetSegmentLength() const;
			virtual const float *GetSegmentData() const;
			virtual int GetNumSubSegments(int segment) const;
			virtual const float *GetCentreLine(float t) const;
			virtual const float *GetEdgeOffset(float t) const;
		protected:
			HeightMapInterface *inputHeightMapInterface;
			void Changed();
			const float *GetPosOnSegment(unsigned spline,float c) const;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif // ifndef WORLD_H

