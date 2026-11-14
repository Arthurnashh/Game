#ifndef SIMUL_META_XYCOLLIDERINTERFACE_H
#define SIMUL_META_XYCOLLIDERINTERFACE_H

namespace simul
{
	namespace graph
	{
			namespace standardnodes
	{
		class Node;
// Graph
// 

//! An interface for a 2D collision object
		class XYColliderInterface
		{
		public:
//! Test for collision with Node n, and perform any necessary response.
			virtual void CollideWith(simul::graph::meta::Node *n){}
		};
// 
		}
	}
}

#endif // ifndef SIMUL_META_XYCOLLIDERINTERFACE_H

