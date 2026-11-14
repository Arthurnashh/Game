#ifndef SIMUL_META_NODESTACK_H
#define SIMUL_META_NODESTACK_H
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

#endif
#include "Simul/Graph/Meta/Node.h"

namespace simul
{
	namespace graph
	{
		namespace meta
		{
		//! A stack of pointers to nodes as a vector.
			typedef std::vector<simul::graph::meta::Node*> NodeStack;

		//! Find the nearest node that can cast to type T from the end of the stack.
			template<class T> T *FindHighestInStack(NodeStack &Stack)
			{
				T *t=NULL;
				for(NodeStack::reverse_iterator i=Stack.rbegin();i!=Stack.rend();i++)
				{
					T *tt=dynamic_cast<T*>(*i);
					if(tt)
					{
						t=tt;
						break;
					}
				}
				return t;
			}
		}
	}
}
#endif
