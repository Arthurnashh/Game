#ifndef SIMUL_META_SELECTORNODE_H
#define SIMUL_META_SELECTORNODE_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include "Simul/Graph/Meta/Tracker.h"
#include <map>
#include <boost/shared_ptr.hpp>
#include <assert.h>

template<class dataType> class TrackerInterface
{
public:
	// Your class can declare the following:
	virtual bool ShouldIncludeNode(simul::graph::meta::Node *,dataType *){return true;}
	virtual void InitNode(simul::graph::meta::Node *,dataType *){}
	virtual void HandleNode(simul::graph::meta::Node *,dataType *){}
	virtual void RemovingNode(simul::graph::meta::Node *,dataType *){}
	virtual void ModifyingNode(simul::graph::meta::Node *,dataType *){}
};

//! TrackerNode: A template for a node that keeps a list of child nodes of a particular type.

//! TrackerNode is a node that looks through its children for nodes of a certain type (nodeType), and keeps
//! a map of them with corresponding instances of a helper type (dataType).
//! dataType contains information particular to the TrackerNode; while nodeType has no knowledge of Selectors
//! that refer to it.
//! When TimeStep is called, it iterates through the list calling  HandleNode(nodeType*,dataType*).

//! Required for a TrackerNode:
//!
//! virtual void InitNode(Node *n,CarController *CC);
//!	virtual void HandleNode(Node *n,CarController *CC);
template<class nodeType,class dataType> class TrackerNode : public simul::graph::meta::Group, public TrackerInterface<dataType>
{
protected:
	struct DataTypeWrapper
	{
		dataType data;
		unsigned count;
	};
	typedef std::map<simul::graph::meta::Node*,boost::shared_ptr<DataTypeWrapper> > selection_map;
	selection_map selectionMap;		//< Map from the nodes to the DataTypeWrapper objects.
public:
	TrackerNode(){}
	virtual ~TrackerNode(){}
	//Macro_Node( (TrackerNode<nodeType,dataType>) , simul::graph::meta::Group )
    //! Inform this parent that a child has been added, and recurse up the graph
	//! In this case, we call InitNode
	virtual void ChildAdded(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(nt)
		{	
			if(ShouldIncludeNode(node,(dataType*)NULL))
			{	
				if(selectionMap.find(node)!=selectionMap.end())
				{
					selectionMap[node]->count++;
				}
				else
				{
					boost::shared_ptr<DataTypeWrapper> &p=selectionMap[node];
					p.reset(new DataTypeWrapper);
					dataType *dt=&(p->data);			// Add it to the list
					p->count=1;
					InitNode(node,dt);					// Initialize dt with info relevant to cn.
				}
			}
		}
		Group::ChildAdded(node,path);//< continue traversing.
	}
	virtual void ChildRemoved(simul::graph::meta::Node *node,simul::graph::meta::NodeStack &path)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(nt)
		{
			selection_map::iterator i=selectionMap.find(node);
			if(i!=selectionMap.end())
			{
				i->second->count--;
				if(!i->second->count)
				{
					dataType *dt=&(i->second->data);
					RemovingNode(node,dt);
					selectionMap.erase(i);
				}
			}
		}
		Group::ChildRemoved(node,path);//< continue traversing.
	}
	virtual void ChildModified(simul::graph::meta::Node *node)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(nt)
		{
			selection_map::iterator i=selectionMap.find(node);
			if(i==selectionMap.end())
				return;
			ModifyingNode(node,&(i->second->data));
		}
	}
	void UpdateSelection()
	{
		for(selection_map::iterator i=selectionMap.begin();i!=selectionMap.end();i++)
		{
			//i->second->updated=true;				// Updated - so don't remove it from the list!
			HandleNode(i->first,&(i->second->data));			// Do whatever this selector does.
		}
	}
};
#endif // ifndef SIMUL_META_SELECTOR_H

