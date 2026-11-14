#ifndef SIMUL_META_OBSERVEDGROUP_H
#define SIMUL_META_OBSERVEDGROUP_H
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

#endif
#include <vector>
#include <stack>
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Base/ObserverPtr.h"
namespace simul
{
	namespace graph
	{
		namespace meta
		{
//! A \ref Group which informs its observers when descendants are added or removed.
			SIMUL_META_EXPORT_CLASS ObservedGroup : public Group
			{
			public:
				ObservedGroup();
				virtual ~ObservedGroup();
				Macro_Node(ObservedGroup)
			//! Inform this parent that a child has been added, and recurse up the graph
				virtual void ChildAdded(Node *node,NodeStack &path);
			//! Inform this parent that a child has been removed, and recurse up the graph
				virtual void ChildRemoved(Node *node,simul::graph::meta::NodeStack &path);
			protected:
				Macro_DefaultStreaming
			};
//! An observer pointer to an ObservedGroup which is informed by that group when a child is
//! added to or removed from the group's subgraph.
			class InformedObserverPtr : public simul::base::ObserverPtr<ObservedGroup>
			{
			public:
//! Called when \a node is added to the subgraph of the ObservedGroup on \a path.
				virtual void ObserveChildAdded(Node *node,NodeStack &path)=0;
//! Called when \a node is remnoved from the subgraph of the ObservedGroup.
				virtual void ObserveChildRemoved(Node *node,simul::graph::meta::NodeStack &path)=0;
			};
		}
	}
}
#endif
