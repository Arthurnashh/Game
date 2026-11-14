#ifndef NODEFINDERVISITOR_H
#define NODEFINDERVISITOR_H
#include "Simul/Graph/Meta/NodeVisitor.h"
#include <set>

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			//! A NodeVisitor that finds all instances of class \a T in a subgraph.
			template<class T> class NodeFinderVisitor : public NodeVisitor
			{
				std::set<T*> Results;
				bool find_all;
				virtual bool Apply(Node *n)
				{
					T *t=dynamic_cast<T*>(n);
					if(t)
					{
						Results.insert(t);
						return find_all;
					}
					return true;
				}
			public:
				NodeFinderVisitor():NodeVisitor(){}
				//! Constructor: If \a all is true, find all instances of class \a T in the subgraph of \a n.
				//! If \a all is false, find the first instance.
				NodeFinderVisitor(Node *n,bool all=true):NodeVisitor()
				{
					find_all=all;
					n->Accept(*this);
				}
				virtual ~NodeFinderVisitor(){}
				//! Get the set of instances of \a T that have been found.
				std::set<T*> &GetResults()
				{
					return Results;
				}
			};
			template<class T> class ConstNodeFinderVisitor : public ConstNodeVisitor
			{
				std::set<const T*> Results;
				bool find_all;
				virtual bool Apply(const Node *n)
				{
					const T *t=dynamic_cast<const T*>(n);
					if(t)
					{
						Results.insert(t);
						return find_all;
					}
					return true;
				}
			public:
				ConstNodeFinderVisitor():ConstNodeVisitor(){}
				//! Constructor: If \a all is true, find all instances of class \a T in the subgraph of \a n.
				//! If \a all is false, find the first instance.
				ConstNodeFinderVisitor(const Node *n,bool all=true):ConstNodeVisitor()
				{
					find_all=all;
					n->Accept(*this);
				}
				virtual ~ConstNodeFinderVisitor(){}
				//! Get the set of instances of \a T that have been found.
				std::set<const T*> &GetResults()
				{
					return Results;
				}
			};
			//! Find the first instance of class \a T in the subgraph of \a n.
			template<class T> std::set<T*> FindAll(Node *n)
			{
				NodeFinderVisitor<T> NFV(n,false);
				return NFV.GetResults();
			}
			//! Find the first instance of class \a T in the subgraph of \a n.
			template<class T> T* FindFirst(Node *n)
			{
				if(!n)
					return NULL;
				NodeFinderVisitor<T> NFV(n,false);
				if(!NFV.GetResults().size())
					return NULL;
				return *(NFV.GetResults().begin());
			}
			//! Find the first instance of class \a T in the subgraph of \a n.
			template<class T> const T* FindFirst(const Node *n)
			{
				if(!n)
					return NULL;
				ConstNodeFinderVisitor<T> NFV(n,false);
				if(!NFV.GetResults().size())
					return NULL;
				return *(NFV.GetResults().begin());
			}
		}
	}
}

#endif // ifndef NODEFINDERVISITOR_H

