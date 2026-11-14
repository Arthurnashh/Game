#ifndef SIMUL_SKYNODE_H
#define SIMUL_SKYNODE_H
#include "Simul/Sky/Sky.h"
#include "Simul/Graph/Meta/Node.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	#pragma warning(disable:4541)
#endif
namespace simul
{
	namespace sky
	{
		class Sky;
		//! A node class encapsulating the Sky class.
		SIMUL_SKY_EXPORT_CLASS SkyNode : public simul::graph::meta::Node,
			public simul::sky::Sky
		{
		public:
			SkyNode();
		protected:
			virtual void PrepareToDestroy();
			virtual ~SkyNode();
		public:
			META_BeginProperties
				META_ValueProperty(float,TimeMultiplier,"How fast time flows relative to real time.")
			META_EndProperties
			virtual int testSkyNode(){return 1;}
			Macro_EnableRecursiveStreaming(simul::graph::meta::Node)
			Macro_Node(SkyNode)
			virtual bool TimeStep(float dt);
			Sky *GetSky(){return this;}
		//! Stream/save the node out to the std::osteam os.
			std::ostream &StreamOut(std::ostream &os) const;
		//! Stream/load the node from the std::isteam is.
			std::istream &StreamIn(std::istream &is);
		//! Stream/save the node out to the std::osteam os.
			std::ostream &StateStreamOut(std::ostream &os) const;
		//! Stream/load the node from the std::isteam is.
			std::istream &StateStreamIn(std::istream &is);
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif
