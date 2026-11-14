#ifndef SIMUL_XML_XMLNODE_H
#define SIMUL_XML_XMLNODE_H

#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/TrackerMacros.h"
#include "Simul/Graph/XML/XMLInterface.h"
#pragma warning( push )
#pragma warning( disable : 4251)		// disable dllexport warning for typedefs derived from stl classes!

namespace simul
{
	namespace graph
	{
		namespace xml
		{
			SIMUL_XML_EXPORT_CLASS XMLNode : public simul::graph::meta::Group, public XMLInterface
			{
			public:
				typedef unsigned long TrackId;
				XMLNode();
			protected:
				virtual ~XMLNode();
			public:
				BEGIN_TRACKERS
					DECLARE_TRACKER(XMLNode,simul::graph::meta::Node,TrackId,tracker)
				END_TRACKERS
				DEFAULT_TRACKER_HANDLING
				void SaveNode(simul::graph::meta::Node *node,const std::string &filename,bool include_subgraph);
				void LoadNode(simul::graph::meta::Node *node,const std::string &filename,bool include_subgraph=false);
				simul::graph::meta::Node *LoadNode(const std::string &filename,bool include_subgraph);
			};
		}
	}
}
#pragma warning( pop )
#endif