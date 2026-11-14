#ifndef SIMUL_META_WORLDINTERFACE_H
#define SIMUL_META_WORLDINTERFACE_H
#include <string>

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
//! An interface for a world or level.
			class WorldInterface
			{
			public:
				virtual int GetStamp()=0;
				virtual const std::string &GetSkyName()=0;
				virtual void SetSkyName(const std::string &)=0;
				virtual const std::string &GetWorldName()=0;
				virtual void SetWorldName(const std::string &)=0;
			};
		}
	}
}


#endif