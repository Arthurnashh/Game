#ifndef SIMUL_META_NODEVISITOR_H
#define SIMUL_META_NODEVISITOR_H
#include "Simul/Graph/Meta/Node.h"


namespace simul
{
	namespace graph
	{
		namespace meta
		{
			//! A visitor class that traverses a subgraph
			SIMUL_META_EXPORT_CLASS NodeVisitor
			{
				bool traverse_parents;
			public:
				NodeVisitor(bool tp=false);
			public:
				virtual ~NodeVisitor();
				//! Traverse the subgraph of \a n, return true if the traversal should continue.
				virtual bool Apply(Node *n)=0;
				//! This is called after traversal of the subgraph of \a n.
				virtual void PostTraversal(Node *){}
				//! Returns true if traversal includes parents.
				bool ShouldTraverseParents() const {return traverse_parents;}
				//! Returns true if traversal includes children.
				bool ShouldTraverseChildren() const {return !traverse_parents;}
			};
			SIMUL_META_EXPORT_CLASS ConstNodeVisitor
			{
				bool traverse_parents;
			public:
				ConstNodeVisitor(bool tp=false);
			public:
				virtual ~ConstNodeVisitor();
				//! Traverse the subgraph of \a n, return true if the traversal should continue.
				virtual bool Apply(const Node *n)=0;
				//! This is called after traversal of the subgraph of \a n.
				virtual void PostTraversal(const Node *){}
				//! Returns true if traversal includes parents.
				bool ShouldTraverseParents() const {return traverse_parents;}
				//! Returns true if traversal includes children.
				bool ShouldTraverseChildren() const {return !traverse_parents;}
			};

			//! A NodeVisitor that traverses a subgraph, calling each node's TimeStep method.
			SIMUL_META_EXPORT_CLASS TimeStepVisitor : public NodeVisitor
			{
				float delta_t;
			public:
				TimeStepVisitor();
			public:
				virtual ~TimeStepVisitor();
				virtual bool Apply(Node *n);
				//! Set the time step in seconds.
				void SetTimeStep(float dt){delta_t=dt;}
				//! Get the time step in seconds.
				float GetTimeStep(){return delta_t;}
			};
		}
	}
}
/*
This is an example of a visitor for you to copy and paste.
#include "Simul/Graph/Meta/NodeVisitor.h"

SomeFunction()
{
	...

	class SomeVisitor : public simul::graph::meta::NodeVisitor
	{
	public:
		SomeVisitor(simul::graph::meta::Node *root)
		{
			root->Accept(*this);
		}
		virtual ~SomeVisitor(){}
		virtual bool Apply(simul::graph::meta::Node *n)
		{
			// Do whatever this visitor does here...

			// Then keep traversing:
			return true;							
		}
	};

	// Instantiate and execute in one command
	SomeVisitor(some_node);

}
*/
#endif
