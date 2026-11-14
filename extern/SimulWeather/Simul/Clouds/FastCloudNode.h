#ifndef SIMUL_GRAPH_CLOUDS_FASTCLOUDNODE_H
#define SIMUL_GRAPH_CLOUDS_FASTCLOUDNODE_H
#include "Simul/Clouds/CellularCloudNode.h"

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
	namespace geometry
	{
		class SimulOrientation;
	}
	namespace clouds
	{
		//! A fast cloud class.

		//! Create an instance of FastCloudNode to generate cloud data for rendering.
		//! See \ref CloudRenderer.
		//! This class can use a custom memory allocator, because it is derived from
		//! a ResourceUser<> of simul::graph::meta::MemoryInterface. To use this feature
		//! call SetResource( simul::graph::meta::MemoryInterface *) with a pointer to a
		//! an instance of a class derived from MemoryInterface.
		SIMUL_CLOUDS_EXPORT_CLASS FastCloudNode : public CellularCloudNode
		{
		public:
			FastCloudNode();
		public:
			Macro_EnableRecursiveStreaming(CellularCloudNode)
			Macro_Node(FastCloudNode)
		public:
			//Cellular
			virtual bool Generate();
			virtual bool UpdatePartial(float proportion);
			// Grid
			virtual float GetCloud(int x,int y,int z) const;
			virtual float GetCloud(unsigned x,unsigned y,unsigned z) const;
			virtual float GetCloudDensity(const simul::math::Vector3 &X) const;
			//! Stream/save to the std::osteam os.
			std::ostream &StreamOut(std::ostream &os) const;
			//! Stream/load from the std::isteam is.
			std::istream &StreamIn(std::istream &is);
			//! Stream/save to the std::osteam os.
			std::ostream &StateStreamOut(std::ostream &os) const;
			//! Stream/load from the std::isteam is.
			std::istream &StateStreamIn(std::istream &is);
			//! Assign a callback object to control humidity (cloud density) within the cloud volume.
			void AddHumidityCallback(HumidityCallbackInterface *hc);
			//! Remove the stated callback object from the list.
			void RemoveHumidityCallback(HumidityCallbackInterface *hc);
			//! Remove all humidity callbacks.
			void ClearHumidityCallbacks();
		protected:
			std::vector<HumidityCallbackInterface *> humidityCallbacks;
		protected:
			virtual void PrepareToDestroy();
			virtual ~FastCloudNode();
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif
