#ifndef DAMAGEINTERFACE_H
#define DAMAGEINTERFACE_H

#include "Simul/Graph/Meta/Group.h"
#include <string>

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class DamageInterface
			{
			public:
				virtual void DoDamage(float)=0;
			};
		}
	}
}
#endif // ifndef WORLD_H

