#ifndef NODESELECTIONINTERFACE_H
#define NODESELECTIONINTERFACE_H
#include "Simul/Graph/Meta/Node.h"
#include <string>

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class NodeSelectionInterface
			{
			public:
				virtual void Select(simul::graph::meta::Node *node)=0;
				virtual void DeleteSelected()=0;
				virtual void CutSelected()=0;
				virtual void PasteAsChildOfSelected()=0;
			};
		}
	}
}
#endif

