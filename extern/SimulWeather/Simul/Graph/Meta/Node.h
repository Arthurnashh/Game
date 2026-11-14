#ifndef SIMUL_META_NODE_H
#define SIMUL_META_NODE_H
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

#endif
#include <string>
#include <vector>
#include "Simul/Base/Referenced.h"
#include <iostream>		
#include <sstream>
#include "Simul/Base/PropertyMacros.h"
#include "Simul/Graph/Meta/Export.h"
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:
#pragma warning(push)
#pragma warning(disable:4251)		// disable dllexport warning for typedefs derived from stl classes!
#endif
							

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			SIMUL_META_EXPORT_CLASS NodeVisitor;
			SIMUL_META_EXPORT_CLASS ConstNodeVisitor;
			SIMUL_META_EXPORT_CLASS Group;
#ifdef _CPPRTTI
	#define Macro_VirtualNode(name)														\
		/** Copy constructor. */														\
			name(const simul::graph::meta::Node &E)										\
			{																			\
				_refCount=0;observers=0;operator=(E);									\
			}																			\
		/** Assignment operator; copy component C's properties to this component. */	\
			virtual void name::operator=(const simul::graph::meta::Node &C)				\
			{																			\
				const name *n=dynamic_cast<const name * > (&C);							\
				if(!n)																	\
					return;																\
				const name &N=*n;														\
				name::operator=(N);														\
			}
#else
	#define Macro_VirtualNode(name)
#endif

		#define Macro_Node(name)														\
			Macro_VirtualNode(name)														\
			/** Copy the properties of name S onto this instance. */					\
				const name &operator=(const name &S)									\
				{																		\
					std::stringstream output(std::stringstream::in|std::stringstream::out|std::stringstream::binary);	\
					S.StreamOut(output);												\
					StreamIn(output);													\
					return *this;														\
				}																		\
			/** Create a new component identical to this one. */						\
				virtual simul::graph::meta::Node* Clone() const							\
				{																		\
					name* node=new name();												\
					node->operator=(*this);												\
					return node;														\
				}																		\
			/** Create a new name. */													\
				static simul::graph::meta::Node* Creator() { return new name (); }


		#define Macro_DefaultStreaming													\
			std::ostream &StreamOut(std::ostream &os) const {return os;}				\
			std::istream &StreamIn(std::istream &is){return is;}				

		#define Macro_DefaultStateStreaming												\
			std::ostream &StateStreamOut(std::ostream &os) const {return os;}			\
			std::istream &StateStreamIn(std::istream &is){return is;}					

		// so we want to build up a list of calls



		//! The base class for all simulation-graph nodes.
			SIMUL_META_EXPORT_CLASS Node : public simul::base::Referenced
			{
			public:
				friend SIMUL_META_EXPORT_CLASS Group;
			//! A vector of Group pointers which stores the parents of the node.
				typedef std::vector<Group*> ParentList;
			protected:
				void AddParent(Group* node);
				void RemoveParent(Group* node);
			private:
			//! This component's parents.
				ParentList parents;
			public:
				Node();
				virtual ~Node();
			//! Create a new component identical to this one.
				virtual Node *Clone() const
				{
					return NULL;
				}
				///static Node* Creator() { return new Node (); }
			//! Reset the component to the default initial values.
				virtual void Clear();
			//! Visitor Pattern : calls the apply method of a NodeVisitor with this node's type.
				virtual void Accept(NodeVisitor& nv);
			//! Visitor Pattern : calls the apply method of a NodeVisitor with this node's type.
				virtual void Accept(ConstNodeVisitor& nv) const;
			//! Visitor Pattern : pass the NodeVisitor on to all our parents.
				virtual void TraverseParents(NodeVisitor& nv);
				virtual void TraverseParents(ConstNodeVisitor& nv) const;
			//! The standard update.
				virtual bool TimeStep(float delta_t);
			//! Assignment operator - if CT is of the same derived class as this, its values are copied. Otherwise there is no effect.
				virtual void operator=(const Node &CT);
			//! Get the node's parent list.
				inline const ParentList& GetParents() const { return parents; }
				inline Group* GetParent(unsigned int i)  { return parents[i]; }
			//! Get the number of parents of the node.
				inline unsigned GetNumParents() const { return (unsigned)parents.size(); }
			//! Remove this node from all its parents, possibly destroying it in the process.
			//! Preferentially only call this from an intrusive_ptr to the node so you don't
			//! get left with an invalid pointer.
				void RemoveFromAllParents();
				virtual std::ostream &RecursiveStreamOut(std::ostream &os,bool include_subgraph) const;
				virtual std::istream &RecursiveStreamIn(std::istream &is,bool include_subgraph);

				virtual std::ostream &RecursiveStateStreamOut(std::ostream &os,bool include_subgraph) const;
				virtual std::istream &RecursiveStateStreamIn(std::istream &is,bool include_subgraph);
				
			//! Stream the node N to os. This function calls the virtual N.StreamOut(os).
				friend SIMUL_META_EXPORT std::ostream &operator<<(std::ostream &os, const Node &N);
			//! Stream the node N from is. This function calls the virtual N.StreamIn(is).
				friend SIMUL_META_EXPORT std::istream &operator>>(std::istream &is, Node &N);
			protected:
			//! Stream/save the node out to the std::osteam os.
				std::ostream &StreamOut(std::ostream &os) const;
			//! Stream/load the node from the std::isteam is.
				std::istream &StreamIn(std::istream &is);
			//! Stream/save the node state out to the std::osteam os.
				std::ostream &StateStreamOut(std::ostream &os) const {return os;}
			//! Stream/load the node state from the std::isteam is.
				std::istream &StateStreamIn(std::istream &is) {return is;}
				//static Node *LoadFromStream(std::istream &is);
			public:
				bool ReParent(Group *new_parent);
				bool HasAncestor(Node *node);
				typedef int StreamBlockMarker;
				enum {NODE_TYPE,NODE_DATA};
			};
		}
	}
}
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:
#pragma warning(pop)
#endif
#endif
