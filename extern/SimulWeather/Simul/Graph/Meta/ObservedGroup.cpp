#include "precomp.h"
#include "Simul/Graph/Meta/ObservedGroup.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include <set>

using namespace simul::graph::meta;
using namespace simul::base;
using std::string;
typedef std::set<Observer*> ObserverSet;

ObservedGroup::ObservedGroup():Group()
{
}

ObservedGroup::~ObservedGroup()
{
}

void ObservedGroup::ChildAdded(Node *node,NodeStack &path)
{
    if(observers)
	{
		ObserverSet &observerSet=*(static_cast<ObserverSet*>(observers));
		for(ObserverSet::iterator i=observerSet.begin();i!=observerSet.end();i++)
		{
			InformedObserverPtr *p=dynamic_cast<InformedObserverPtr *>(*i);
			if(p)
				p->ObserveChildAdded(node,path);
		}
	}
	Group::ChildAdded(node,path);
}


void ObservedGroup::ChildRemoved(Node *node,simul::graph::meta::NodeStack &path)
{
    if(observers)
	{
		ObserverSet &observerSet=*(static_cast<ObserverSet*>(observers));
		for(ObserverSet::iterator i=observerSet.begin();i!=observerSet.end();i++)
		{
			InformedObserverPtr *p=dynamic_cast<InformedObserverPtr *>(*i);
			if(p)
				p->ObserveChildRemoved(node,path);
		}
	}
	Group::ChildRemoved(node,path);
}

