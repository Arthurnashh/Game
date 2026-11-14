#ifndef SIMUL_META_SELECTOR_H
#define SIMUL_META_SELECTOR_H
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include <map>
#include <assert.h>

//! A template for a node that keeps a list of child nodes of a particular type.

//! Tracker is a node that looks through its children for nodes of a certain type (nodeType), and keeps
//! a map of them with corresponding instances of a helper type (dataType).
//! dataType contains information particular to the Tracker; while nodeType has no knowledge of Selectors
//! that refer to it.
//! When TimeStep is called, it iterates through the list calling  HandleNode(nodeType*,dataType*).
template<class ownerType,class nodeType,class dataType> class Tracker
{
protected:
	struct DataTypeWrapper
	{
		dataType data;
		nodeType *tracked;
		unsigned count;
	};
public:
	typedef std::map<simul::graph::meta::Node*,DataTypeWrapper* > selection_map;
	selection_map selectionMap;		//! Map from the nodes to the CarSounds

	simul::graph::meta::Node *FindNode(dataType d)
	{
		typename selection_map::iterator i;
		for(i=selectionMap.begin();i!=selectionMap.end();i++)
		{
			if(i->second->data==d)
				return i->first;
		}
		return NULL;
	}
    //! Inform this parent that a child has been added, and recurse up the graph
	//! In this case, we call InitNode
	virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node)
	{
 		nodeType *nt=dynamic_cast<nodeType*>(node);
		 if(!nt)
			return;
		if(!si->ShouldIncludeNode(node,(dataType*)NULL))
			return;
		if(selectionMap.find(node)!=selectionMap.end())
		{
			selectionMap[node]->count++;
			return;
		}
		DataTypeWrapper *p=new DataTypeWrapper;
		selectionMap[node]=p;
		dataType *dt=&(p->data);		// Add it to the list
		p->tracked=nt;
		p->count=1;
		si->InitNode(node,dt);							// Initialize dt with info relevant to cn.
	}
	virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(!nt)
			return;
		typename selection_map::iterator i=selectionMap.find(node);
		if(i==selectionMap.end())
			return;
		i->second->count--;
		if(i->second->count)
			return;
		si->RemovingNode(node,&(i->second->data));
		selectionMap.erase(i);
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
			si->HandleNode(i->first,&(i->second->data));			// Do whatever this Tracker does.
			i=j;
		}
	}
};

//! A template for a node that keeps a list of child nodes of a particular type.

//! SimpleTracker is a node that looks through its children for nodes of a certain type (nodeType), and keeps
//! a map of them. Unlike Tracker, it does not use a user-defined helper type, instead passing
//! the pointer to the node type.
template<class ownerType,class nodeType> class SimpleTracker
{
public:
	struct DataTypeWrapper
	{
		nodeType *data;
		unsigned count;
	};
	typedef std::map<simul::graph::meta::Node*,DataTypeWrapper* > selection_map;
	selection_map selectionMap;
    //! Inform this parent that a child has been added, and recurse up the graph
	//! In this case, we call InitNode
	virtual void ChildAdded(ownerType *si,simul::graph::meta::Node *node)
	{
 		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(!nt)
			return;
		if(!si->ShouldIncludeNode(node,(nodeType*)NULL))
			return;
		if(selectionMap.find(node)!=selectionMap.end())
		{
			selectionMap[node]->count++;
			return;
		}
		DataTypeWrapper *p=new DataTypeWrapper;
		selectionMap[node]=p;
		p->data=nt;
		nodeType *dt=(p->data);		// Add it to the list
		p->count=1;
		si->InitNode(node,dt);							// Initialize dt with info relevant to cn.
	}
	virtual void ChildRemoved(ownerType *si,simul::graph::meta::Node *node)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(!nt)
			return;
		typename selection_map::iterator i=selectionMap.find(node);
		if(i==selectionMap.end())
			return;
		i->second->count--;
		if(i->second->count)
			return;
		si->RemovingNode(node,(i->second->data));
					delete selectionMap[node];
		selectionMap.erase(i);
	}
	virtual void ChildModified(ownerType *si,simul::graph::meta::Node *node)
	{
		nodeType *nt=dynamic_cast<nodeType*>(node);
		if(nt)
		{
			typename selection_map::iterator i=selectionMap.find(node);
			if(i==selectionMap.end())
				return;
			si->ModifyingNode(node,(i->second->data));
		}
	}
	void UpdateSelection(ownerType *si)
	{
		for(typename selection_map::iterator i=selectionMap.begin();i!=selectionMap.end();)
		{
			typename selection_map::iterator j=i;							// Do it this way in case we destroy i.
			j++;
			si->HandleNode(i->first,(i->second->data));			// Do whatever this Tracker does.
			i=j;
		}
	}
};
#endif // ifndef SIMUL_META_SELECTOR_H

