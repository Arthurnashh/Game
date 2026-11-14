#ifndef SIMUL_GRAPH_CLOUDS_GEOCLOUDNODE_H
#define SIMUL_GRAPH_CLOUDS_GEOCLOUDNODE_H
#include "Simul/Clouds/ThunderCloudNode.h"

#ifdef _MSC_VER
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
	#pragma warning(disable:4541)
#endif

namespace simul
{
	namespace clouds
	{
		//! A GeoCloud is like a FastCloud, only it is considered to be a window onto a larger 2D
		//! cloud map, accessed via the HumidityCallback Interface.
		SIMUL_CLOUDS_EXPORT_CLASS GeoCloudNode : public ThunderCloudNode
		{
		public:
			GeoCloudNode();
			virtual ~GeoCloudNode();
			META_BeginProperties
				META_ValuePropertyWithSetCall(int	,WindowX		,WindowChanged,"Window X position.")
				META_ValuePropertyWithSetCall(int	,WindowY		,WindowChanged,"Window Y position.")
			META_EndProperties
			Macro_EnableRecursiveStreaming(ThunderCloudNode)
			Macro_Node(GeoCloudNode)
			virtual float GetCloud(unsigned x,unsigned y,unsigned z) const;
		protected:
			void WindowChanged();
		};
	}
}
#endif