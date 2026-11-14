#ifndef SIMUL_XML_INTERFACE_H
#define SIMUL_XML_INTERFACE_H

#include "Simul/Graph/XML/Export.h"
#include <string>
namespace simul
{
	namespace graph
	{
		namespace meta
		{
			class Node;
		}
		namespace xml
		{
			SIMUL_XML_EXPORT_CLASS XMLInterface
			{
			public:
				virtual void SaveNode(simul::graph::meta::Node *node,const std::string &filename,bool include_subgraph)=0;
				virtual void LoadNode(simul::graph::meta::Node *node,const std::string &filename,bool include_subgraph)=0;
				virtual simul::graph::meta::Node *LoadNode(const std::string &filename,bool include_subgraph)=0;
			};
		}
	}
}
#endif