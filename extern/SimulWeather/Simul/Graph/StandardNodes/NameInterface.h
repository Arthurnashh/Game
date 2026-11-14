#ifndef SIMUL_META_NAMEINTERFACE_H
#define SIMUL_META_NAMEINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
//! An interface for anything that has a name returnable as text.
			class NameInterface
			{
			public:
				virtual const char *GetNameAsCString()=0;
			};
		}
	}
}


#endif