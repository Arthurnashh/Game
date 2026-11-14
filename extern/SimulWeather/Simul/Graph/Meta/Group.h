#ifndef SIMUL_META_GROUP_H
#define SIMUL_META_GROUP_H
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

#endif
#include <vector>
#include <stack>

#include "Simul/Base/SmartPtr.h"
#include "Simul/Graph/Meta/Node.h"
#include "Simul/Graph/Meta/NodeStack.h"
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:
#pragma warning( push )
#pragma warning( disable : 4251)		// disable dllexport warning for typedefs derived from stl classes!
#endif

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			//typedef std::vector< simul::base::SmartPtr<Node> > NodeList;
			typedef std::vector< simul::base::SmartPtr<Node> > SIMUL_META_EXPORT NodeList;
	//! A \ref Node which may have zero or more children - may be used as a concrete or base class.
			SIMUL_META_EXPORT_CLASS Group : public Node
			{
				NodeList children;
			public:
				Group();
			protected:
				virtual void PrepareToDestroy();
				virtual ~Group();
			public:
				void ClearChildren();
				Macro_Node(Group)
				unsigned CountInstancesOfDescendant(Node *node);
			//! Inform this parent that a child has been added, and recurse up the graph
				virtual void ChildAdded(Node *node,NodeStack &path);
				void RecurseChildAdded(Node *node,NodeStack &path);
				void RecurseChildAdded(Node *node);
			//! Inform this parent that a child has been removed, and recurse up the graph
				virtual void ChildRemoved(Node *node,NodeStack &path);
				void RecurseChildRemoved(Node *node,NodeStack &path);
				void RecurseChildRemoved(Node *node);
			//! Inform this parent that a child has been modified, and recurse up the graph
				virtual void ChildModified(Node *node);
			//! Visitor Pattern : calls the apply method of a NodeVisitor with this node's type.
				virtual void Accept(NodeVisitor& nv);
				virtual void Traverse(NodeVisitor& nv);
			//! Visitor Pattern : calls the apply method of a ConstNodeVisitor with this node's type.
				virtual void Accept(ConstNodeVisitor& nv) const;
				virtual void Traverse(ConstNodeVisitor& nv) const;
				unsigned GetNumChildren() const;
				Node *GetChild(unsigned i);
				const Node *GetChild(unsigned i) const;
				simul::base::SmartPtr<Node> &GetChildPtr(unsigned i);
				bool AddChild(simul::base::SmartPtr<Node> &c);
				bool InsertChild( unsigned index, simul::base::SmartPtr<Node> &c );
				bool AddChild(Node *N);
				bool InsertChild( unsigned index, Node *child );
			//! Remove Node from Group.
				virtual bool RemoveChild(Node *child);
				virtual bool RemoveChild(simul::base::SmartPtr<Node> &c);
				virtual bool RemoveChild(unsigned pos,unsigned numChildrenToRemove=1);
			/** Get the index number of child, return a value between
			  * 0 and _children.size()-1 if found, if not found then
			  * return _children.size().
			*/
			inline unsigned GetChildIndex( const Node* node ) const
			{
				for (unsigned int childNum=0;childNum<children.size();++childNum)
				{
					if ((children[childNum]).get()==node) return childNum;
				}
				return (unsigned)children.size(); // node not found.
			}
			virtual std::ostream &RecursiveStreamOut(std::ostream &os,bool include_subgraph) const;
			virtual std::istream &RecursiveStreamIn(std::istream &is,bool include_subgraph);
			protected:
				Macro_DefaultStreaming
				Macro_DefaultStateStreaming
			};
		}
	}
}
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:
#pragma warning( pop )
#endif
#endif
