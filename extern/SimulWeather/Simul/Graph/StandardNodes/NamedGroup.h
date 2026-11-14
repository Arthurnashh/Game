#ifndef SIMUL_META_NAMEDGROUP_H
#define SIMUL_META_NAMEDGROUP_H

#include <vector>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/StandardNodes/NameInterface.h"
#include <string>
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
//! A \ref Group which has a text name.
			class NamedGroup : public simul::graph::meta::Group, public NameInterface
			{
				std::string name;
			public:
				NamedGroup();
				NamedGroup(const std::string &n);
				virtual ~NamedGroup();
				Macro_Node(NamedGroup)
				virtual const char *GetNameAsCString()
				{
					return name.c_str();
				}
			};
		}
	}
}
#endif