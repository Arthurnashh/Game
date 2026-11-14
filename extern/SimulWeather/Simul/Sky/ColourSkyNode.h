#ifndef SIMUL_COLOUR_SKYNODE_H
#define SIMUL_COLOUR_SKYNODE_H
#include "Simul/Sky/ColourSky.h"
#include "Simul/Graph/Meta/Node.h"
#ifdef _MSC_VER
	
	#pragma warning(push)
	#pragma warning(disable:4541)
	#pragma warning(disable:4275)
#endif
namespace simul
{
	namespace sky
	{
		class Sky;
		//! A node class encapsulating the Sky class.
		SIMUL_SKY_EXPORT_CLASS ColourSkyNode : public simul::graph::meta::Node,
			public simul::sky::ColourSky
		{
		public:
			ColourSkyNode();
		protected:
			virtual void PrepareToDestroy();
			virtual ~ColourSkyNode();
		public:
			virtual int testSkyNode(){return 1;}
			Macro_EnableRecursiveStreaming(simul::graph::meta::Node)
			Macro_Node(ColourSkyNode)
			virtual bool TimeStep(float dt);
			ColourSky *GetSky(){return this;}
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
