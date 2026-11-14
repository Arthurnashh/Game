#ifndef SIMUL_META_PATHTRACKER_H
#define SIMUL_META_PATHTRACKER_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include <map>
#include <assert.h>
#include <stack>

/*template<class dataType> class PathTrackerInterface
{
public:
	// Your class can declare the following:
	virtual void InitPathToNode(simul::graph::meta::Node *,dataType *,const simul::graph::meta::NodeStack &){}
	virtual void RemovingPathToNode(simul::graph::meta::Node *,dataType *,const simul::graph::meta::NodeStack &){}
	virtual void InitNode(simul::graph::meta::Node *,dataType *,const simul::graph::meta::NodeStack &){}
	virtual void HandleNode(simul::graph::meta::Node *,dataType *){}
	virtual void RemovingNode(simul::graph::meta::Node *,dataType *){}
	virtual void ModifyingNode(simul::graph::meta::Node *,dataType *){}
};*/
//! PathTracker: A template for a node that keeps a list of child nodes of a particular type.

//! PathTracker is a node that looks through its children for nodes of a certain type (nodeType), and keeps
//! a map of them with corresponding instances of a helper type (dataType).
//! dataType contains information particular to the Selector; while nodeType has no knowledge of Selectors
//! that refer to it.
//! When TimeStep is called, it iterates through the list calling  HandleNode(nodeType*,dataType*).
//! Unlike Selector, PathTracker keeps track of the node path between its owner and the selected node.
template<class ownerType,class nodeType,class dataType> class PathTracker
{
protected:
	struct DataTypeWrapper
	{
		dataType data;
		unsigned count;
		bool operator==(const DataTypeWrapper &W)
		{
			return(W.data==data);
		}
	};
public:
	typedef std::map<simul::graph::meta::Node*,DataTypeWrapper* > selection_map;
	selection_map selectionMap;		//! Map from the nodes to the CarSounds

    //! Inform this parent that a child has been added, and recurse up the graph
	//! In this case, we call InitNode
	virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node,
		const simul::graph::meta::NodeStack &path)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(!nt)
			return;
		if(selectionMap.find(node)!=selectionMap.end())
		{
			selectionMap[node]->count++;
			si->InitPathToNode(node,&(selectionMap[node]->data),path);
			return;
		}
		DataTypeWrapper *p=new DataTypeWrapper;
		selectionMap[node]=p;
		dataType *dt=&(p->data);							// Add it to the list
		p->count=1;
		si->InitNode(node,dt,path);							// Initialize dt with info relevant to cn.
	}
	virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node,
		const simul::graph::meta::NodeStack &)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(nt)
		{
			typename PathTracker<ownerType,nodeType,dataType>::selection_map::iterator i=selectionMap.find(node);
			assert(i!=selectionMap.end());
			selectionMap[node]->count--;
			if(selectionMap[node]->count)
			{
				si->ModifyingNode(node,&(i->second->data));
				return;
			}
			dataType *dt=&(i->second->data);
			si->RemovingNode(node,dt);
			delete selectionMap[node];
			selectionMap.erase(i);
		}
	}
	virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(nt)
		{
			typename selection_map::iterator i=selectionMap.find(node);
			if(i==selectionMap.end())
				return;
			si->ModifyingNode(node,&(i->second->data));
		}
	}
	void UpdateSelection(ownerType *si)
	{
		for(typename selection_map::iterator i=selectionMap.begin();i!=selectionMap.end();)
		{
			typename selection_map::iterator j=i;			// Do it this way in case we destroy i.
			j++;
			si->HandleNode(i->first,&(i->second->data));			// Do whatever this selector does.
			i=j;
		}
	}
};
#endif // ifndef SIMUL_META_PATHSELECTOR_H
