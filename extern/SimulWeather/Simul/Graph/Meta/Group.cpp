#include "precomp.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/NodeVisitor.h"
#include <algorithm>
#include <math.h>
#include <assert.h>

using namespace simul::graph::meta;
using std::string;

Group::Group()
{
}

void Group::PrepareToDestroy()
{
	ClearChildren();
}

Group::~Group()
{
}

void Group::ClearChildren()
{
	RemoveChild(0,GetNumChildren());
}

void Group::Accept(NodeVisitor& nv)
{
	if(nv.Apply(this)&&nv.ShouldTraverseChildren())
		Traverse(nv);
	nv.PostTraversal(this);
}

void Group::Accept(ConstNodeVisitor& nv) const
{
	if(nv.Apply(this)&&nv.ShouldTraverseChildren())
		Traverse(nv);
	nv.PostTraversal(this);
}

void Group::Traverse(NodeVisitor& nv)
{
	for(unsigned i=0;i<children.size();i++)
    {
		simul::base::SmartPtr<Node> itr=children[i];
        (itr)->Accept(nv);
    }
}


void Group::Traverse(ConstNodeVisitor& nv) const
{
	for(unsigned i=0;i<children.size();i++)
    {
		children[i]->Accept(nv);
    }
}

unsigned Group::GetNumChildren() const
{
	return (unsigned)children.size();
}

Node *Group::GetChild(unsigned i)
{
	if(i>=children.size())
		return NULL;
	return children[i].get();
}

const Node *Group::GetChild(unsigned i) const
{
	if(i>=children.size())
		return NULL;
	return children[i].get();
}

simul::base::SmartPtr<Node>& Group::GetChildPtr(unsigned i)
{
	return children[i];
}
bool Group::AddChild(Node *child)
{
	assert(child!=(Node*)this);
#ifdef _DEBUG
	if((child==(Node*)this))
		return false;
#endif
    return Group::InsertChild((unsigned)children.size(), child );
}

bool Group::AddChild(simul::base::SmartPtr<Node> &c)
{
	return InsertChild((unsigned)children.size(),c);
}
void Group::ChildAdded(Node *node,NodeStack &path)
{
	assert(node!=(Node*)this);
	path.push_back(this);
	for(std::vector<Group*>::iterator i=parents.begin();i!=parents.end();i++)
		(*i)->ChildAdded(node,path);
	path.pop_back();
}

unsigned Group::CountInstancesOfDescendant(Node *node)
{
	unsigned num=0;
	for(NodeList::iterator i=children.begin();i!=children.end();i++)
	{
#ifdef _CPPRTTI
		Group *g=dynamic_cast<Group *>(i->get());
#else
		Group *g=NULL;
#endif
		if(g)
			num+=g->CountInstancesOfDescendant(node);
		if(i->get()==node)
			num++;
	}
	return num;
}

// Inform of a child, also add its children.
void Group::RecurseChildAdded(Node *node)
{
	NodeStack path;
	ChildAdded(node,path);
#ifdef _CPPRTTI
	Group *group=dynamic_cast<Group *>(node);
#else
	Group *group=NULL;
#endif
	if(!group)
		return;
	path.insert(path.begin(),(Node*)group);
	//recursively add the node's children to this group.
	for(unsigned i=0;i<group->GetNumChildren();i++)
	{
		RecurseChildAdded(group->GetChild(i),path);
	}
}
// Inform of a child, also add its children.
void Group::RecurseChildAdded(Node *node,NodeStack &path)
{
	ChildAdded(node,path);
#ifdef _CPPRTTI
	Group *group=dynamic_cast<Group *>(node);
#else
	Group *group=NULL;
#endif
	if(!group)
		return;
	path.insert(path.begin(),group);
	//recursively add the node's children to this group.
	for(unsigned i=0;i<group->GetNumChildren();i++)
	{
		RecurseChildAdded(group->GetChild(i),path);
	}
	path.erase(path.begin());
}


void Group::ChildRemoved(Node *node,NodeStack &path)
{
	assert(node!=(Node*)this);
	path.push_back(this);
	for(std::vector<Group*>::iterator i=parents.begin();i!=parents.end();i++)
		(*i)->ChildRemoved(node,path);
}

void Group::RecurseChildRemoved(Node *node)
{
	NodeStack path;
	ChildRemoved(node,path);
#ifdef _CPPRTTI
	Group *group=dynamic_cast<Group *>(node);
#else
	Group *group=NULL;
#endif
	if(!group)
		return;
	path.insert(path.begin(),(Node*)group);
	//recursively add the node's children to this group.
	for(unsigned i=0;i<group->GetNumChildren();i++)
	{
		RecurseChildRemoved(group->GetChild(i),path);
	}
}

void Group::RecurseChildRemoved(Node *node,NodeStack &path)
{
	ChildRemoved(node,path);
	// does this child have children?
#ifdef _CPPRTTI
	Group *group=dynamic_cast<Group *>(node);
#else
	Group *group=NULL;
#endif
	if(!group)
		return;
	//recursively add the node's children to this group.
	for(unsigned i=0;i<group->GetNumChildren();i++)
		RecurseChildRemoved(group->GetChild(i));
}

//! Inform this parent that a child has been modified, and recurse up the graph
void Group::ChildModified(Node *node)
{
	for(std::vector<Group*>::iterator i=parents.begin();i!=parents.end();i++)
		(*i)->ChildModified(node);
}

bool Group::InsertChild(unsigned index,simul::base::SmartPtr<Node> &c)
{
	if(!c.get())
		return false;
#ifdef ENSURE_CHILD_IS_UNIQUE    
    if (containsNode(child))
    {
        notify(WARN)<<"Adding non unique child to osg::Group, ignoring call"<<std::endl;
        return false;
    }
#endif
#ifdef _DEBUG
	if(HasAncestor(c.get()))
		return false;
#endif
    if(index>=children.size())
		children.push_back(c);
    else
        children.insert(children.begin()+index,c);
    // register as parent of child.
    c->AddParent(this);
	return true;
}
bool Group::InsertChild( unsigned index,Node *child )
{
    if(!child)
		return false;
    // note ref_ptr<> automatically handles incrementing child's reference count.
    simul::base::SmartPtr<Node> ip(child);
	return InsertChild(index,ip);
}

bool Group::RemoveChild( simul::base::SmartPtr<Node> &c )
{
    return RemoveChild(GetChildIndex(c.get()));
}

bool Group::RemoveChild( Node *child )
{
    return RemoveChild(GetChildIndex(child));
}

bool Group::RemoveChild(unsigned pos,unsigned numChildrenToRemove)
{
    if (pos<children.size()&&numChildrenToRemove>0)
    {
        unsigned endOfRemoveRange=pos+numChildrenToRemove;
        if (endOfRemoveRange>(unsigned)children.size())
            endOfRemoveRange=(unsigned)children.size();
        for(unsigned i=pos;i<endOfRemoveRange;++i)
        {
            Node* child = children[i].get();
            // remove this Geode from the child parent list.
            child->RemoveParent(this);
        }
		children.erase(children.begin()+pos,children.begin()+endOfRemoveRange);
       // dirtyBound();
        return true;
    }
    else return false;
}


std::ostream &Group::RecursiveStreamOut(std::ostream &os,bool include_subgraph) const
{
	StreamOut(os);
	if(!include_subgraph)
		return os;
	unsigned num=(unsigned)GetNumChildren();
	os.write((const char*)&num,sizeof(num));
	for(NodeList::const_iterator i=children.begin();i!=children.end();i++)
	{
		(*i)->RecursiveStreamOut(os,include_subgraph);
	}
	return os;
}

std::istream &Group::RecursiveStreamIn(std::istream &is,bool include_subgraph)
{
	StreamIn(is);
	if(!include_subgraph)
		return is;
/*	unsigned num_children=0;
	is.read((char*)&num_children,sizeof(num_children));
	for(unsigned i=0;i<num_children;i++)
	{
		Node *N=Node::LoadFromStream(is,include_subgraph);
		assert(N);
		AddChild(N);
	}*/
	return is;
}
