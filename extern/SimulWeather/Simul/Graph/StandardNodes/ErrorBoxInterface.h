#ifndef SIMUL_META_ERRORBOXINTERFACE_H
#define SIMUL_META_ERRORBOXINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			class Node;
		}
		namespace standardnodes
		{
//! ErrorBoxInterface
			class ErrorBoxInterface
			{
			public:
				//! Get the user's choice, stating the given text.
				enum Choice
				{
					CONTINUE=0,CANCEL=1
				};
				virtual Choice GetErrorChoice(const simul::graph::meta::Node *,const char *)=0;
			};
		}
	}
}
#endif
