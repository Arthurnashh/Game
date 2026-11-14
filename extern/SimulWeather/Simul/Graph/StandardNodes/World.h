#ifndef WORLD_H
#define WORLD_H

#include "Simul/Graph/StandardNodes/NameInterface.h"
#include "Simul/Graph/StandardNodes/WorldInterface.h"
#include "Simul/Graph/StandardNodes/Export.h"
#include "Simul/Graph/Meta/Group.h"
#include <string>

#ifdef _MSC_VER
// C4251 "class '...' needs to have dll-interface to be used by clients of ..."
// This warning can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			SIMUL_STD_EXPORT_CLASS World:public simul::graph::meta::Group,
				public simul::graph::standardnodes::NameInterface,
				public simul::graph::standardnodes::WorldInterface
			{
				std::string Name;
				std::string sky_name;
			public:
				World(std::string name="",std::string skyname="");
				virtual ~World();
			//! NameInterface
				virtual const char *GetNameAsCString();
			//! WorldInterface
				virtual int GetStamp();
				virtual const std::string &GetSkyName();
				virtual void SetSkyName(const std::string &){}
				virtual const std::string &GetWorldName();
				virtual void SetWorldName(const std::string &name);
			};
		}
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif // ifndef WORLD_H

