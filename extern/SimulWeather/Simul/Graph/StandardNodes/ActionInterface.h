#ifndef ACTIONINTERFACE_H
#define ACTIONINTERFACE_H

#include "Simul/Graph/Meta/Group.h"
#include <string>

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class ActionInterface
			{
			public:
				virtual const char *GetNameAsCString();
			};
		}
	}
}
#endif // ifndef WORLD_H

