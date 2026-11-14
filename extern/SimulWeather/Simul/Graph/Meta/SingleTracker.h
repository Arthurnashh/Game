#ifndef SIMUL_META_SINGLESELECTOR_H
#define SIMUL_META_SINGLESELECTOR_H
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/NodeStack.h"
#include <map>
#include <assert.h>

namespace simul
{
	namespace graph
	{
		namespace meta
		{

			//! SingleTracker: A template for a node that looks for a single child node of a particular type.

			//! SingleTracker is a node that looks through its children for a node of a certain type (nodeType).
			//! If found, SingleTracker keeps a pointer to that node until the connection is broken.
			//! Distinct from Selector, we may only have ONE node of that type as a descendant. Otherwise
			//! an assertion is thrown.
			//! When TimeStep is called, it calls  HandleNode(nodeType*,dataType*).
			template<class ownerType,class interfaceType> class SingleTracker
			{
			protected:
				interfaceType *selected;
				simul::graph::meta::Node *sel_node;
				unsigned path_count;
			public:
				SingleTracker()
				{
					selected=NULL;
					sel_node=NULL;
					path_count=0;
				}
				interfaceType *GetSelected(){return selected;}
				const interfaceType *GetSelected() const {return selected;}
				//! Inform this parent that a child has been added, and recurse up the graph
				//! In this case, we call InitNode
				virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node)
				{
					interfaceType *nt=dynamic_cast<interfaceType*>(node);
					if(!nt)
						return;
					if(!si->ShouldIncludeNode(node,nt))
						return;
					path_count++;
					if(selected==nt)
						return;
					// First-come, first served:
					if(selected)
						return;
					selected=nt;
					sel_node=node;
					si->InitNode(node,nt);							// Initialize dt with info relevant to cn.
				}
				virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node)
				{
					interfaceType *nt=dynamic_cast<interfaceType*>(node);
					if(!nt)
						return;
					if(selected!=nt)
						return;
					path_count--;
					if(path_count)
						return;
					// no more paths to this node!
					si->RemovingNode(node,nt);
					selected=NULL;
					sel_node=NULL;
				}
				virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node)
				{
					interfaceType *nt=dynamic_cast<interfaceType*>(node);
					if(!nt)
						return;
					si->ModifyingNode(node,nt);
				}
				void UpdateSelection(ownerType *si)
				{
					if(selected)
						si->HandleNode(sel_node,selected);
				}
			};
		}
	}
}
#endif // ifndef SIMUL_META_SINGLESELECTOR_H

